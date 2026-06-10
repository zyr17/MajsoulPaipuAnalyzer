// ==UserScript==
// @name         Majsoul WebGL Paipu Bridge
// @namespace    https://github.com/zyr17/MajsoulPaipuAnalyzer
// @version      0.1.5
// @description  Capture Majsoul WebGL gateway sockets and expose read-only paipu RPC helpers.
// @match        https://game.maj-soul.com/1/*
// @match        https://game.mahjongsoul.com/*
// @match        https://mahjongsoul.game.yo-star.com/*
// @run-at       document-start
// @grant        none
// @inject-into  page
// ==/UserScript==

/*
 * How to use
 * - Export metadata for the current account: open the game while logged in,
 *   wait until the panel shows at least one verified WebSocket, then click
 *   "Update Metadata". The script downloads a metadata JSONL file.
 * - Download paipu from exported metadata: click "Download Paipu", paste some
 *   or all lines from the metadata JSONL file, then click "Download ZIP".
 *   The script downloads one ZIP containing .bin paipu files and optional
 *   .header.json files.
 * - Download paipu from an existing UUID list: click "Download Paipu", paste
 *   one paipu UUID per line, then click "Download ZIP".
 *
 * The script only sends read-only Majsoul lobby RPCs over the WebSocket already
 * created by the official web client. It waits for a verified lobby WebSocket
 * before running metadata or paipu download actions.
 */

(function () {
    "use strict";

    const VERSION = "0.1.5";
    const CLIENT_VERSION = "WebGL_2022-0.16.232";
    const DEFAULT_PAGE_MODE = 10;
    const RECORD_METHODS = new Set([
        ".lq.Lobby.fetchGameRecord",
        ".lq.Lobby.readGameRecord",
        ".lq.Lobby.fetchGameRecordListV2",
        ".lq.Lobby.fetchNextGameRecordList",
        ".lq.Lobby.fetchGameRecordsDetail"
    ]);

    if (window.__MajsoulPaipuBridgeInstalled)
        return;
    window.__MajsoulPaipuBridgeInstalled = true;

    const NativeWebSocket = window.WebSocket;
    if (!NativeWebSocket)
        return;

    const textEncoder = new TextEncoder();
    const textDecoder = new TextDecoder("utf-8");
    const state = {
        sockets: [],
        nativeRequestIds: new Set(),
        pending: new Map(),
        preferredSocket: null,
        recordSocket: null,
        nextBridgeRequestId: 0xfffe,
        logs: [],
        badge: null,
        ui: {},
        collecting: false,
        downloading: false,
        doneMessage: ""
    };

    function toBytes(data) {
        if (data instanceof Uint8Array)
            return data;
        if (data instanceof ArrayBuffer)
            return new Uint8Array(data);
        if (ArrayBuffer.isView(data))
            return new Uint8Array(data.buffer, data.byteOffset, data.byteLength);
        if (Array.isArray(data))
            return Uint8Array.from(data);
        return null;
    }

    function concat(parts) {
        let total = 0;
        for (let part of parts)
            total += part.length;
        let out = new Uint8Array(total);
        let offset = 0;
        for (let part of parts) {
            out.set(part, offset);
            offset += part.length;
        }
        return out;
    }

    function encodeVarint(value) {
        let n = Number(value);
        let out = [];
        while (n > 0x7f) {
            out.push((n & 0x7f) | 0x80);
            n = Math.floor(n / 128);
        }
        out.push(n);
        return Uint8Array.from(out);
    }

    function readVarint(buf, offset = 0) {
        let value = 0;
        let shift = 0;
        let pos = offset;
        while (pos < buf.length) {
            let byte = buf[pos++];
            value += (byte & 0x7f) * Math.pow(2, shift);
            if ((byte & 0x80) == 0)
                return { value, offset: pos };
            shift += 7;
        }
        throw new Error("unterminated varint at " + offset);
    }

    function readVarintBig(buf, offset = 0) {
        let value = 0n;
        let shift = 0n;
        let pos = offset;
        while (pos < buf.length) {
            let byte = buf[pos++];
            value += BigInt(byte & 0x7f) << shift;
            if ((byte & 0x80) == 0)
                return { value, offset: pos };
            shift += 7n;
        }
        throw new Error("unterminated varint at " + offset);
    }

    const UINT64_SIGN_BIT = 1n << 63n;
    const UINT64_SIZE = 1n << 64n;

    function signedBigintToNumber(value) {
        return Number(value >= UINT64_SIGN_BIT ? value - UINT64_SIZE : value);
    }

    function fieldKey(field, wireType) {
        return encodeVarint(field * 8 + wireType);
    }

    function encodeBytes(field, data) {
        let body = toBytes(data) || new Uint8Array(0);
        return concat([fieldKey(field, 2), encodeVarint(body.length), body]);
    }

    function encodeString(field, value) {
        return encodeBytes(field, textEncoder.encode(String(value)));
    }

    function encodeUint(field, value) {
        return concat([fieldKey(field, 0), encodeVarint(value)]);
    }

    function decodeFields(data) {
        let buf = toBytes(data) || new Uint8Array(0);
        let fields = [];
        let offset = 0;
        while (offset < buf.length) {
            let start = offset;
            let key = readVarintBig(buf, offset);
            offset = key.offset;
            let field = Number(key.value >> 3n);
            let wireType = Number(key.value & 7n);
            let value;
            let signedValue;
            if (wireType == 0) {
                let got = readVarintBig(buf, offset);
                value = Number(got.value);
                signedValue = signedBigintToNumber(got.value);
                offset = got.offset;
            }
            else if (wireType == 1) {
                value = buf.slice(offset, offset + 8);
                offset += 8;
            }
            else if (wireType == 2) {
                let len = readVarint(buf, offset);
                offset = len.offset;
                value = buf.slice(offset, offset + len.value);
                offset += len.value;
            }
            else if (wireType == 5) {
                value = buf.slice(offset, offset + 4);
                offset += 4;
            }
            else {
                throw new Error("unsupported protobuf wire type " + wireType + " at " + start);
            }
            fields.push({ field, wireType, value, signedValue });
        }
        return fields;
    }

    function firstField(fields, field, wireType) {
        return fields.find((item) => item.field == field && (wireType === undefined || item.wireType == wireType));
    }

    function allFields(fields, field, wireType) {
        return fields.filter((item) => item.field == field && (wireType === undefined || item.wireType == wireType));
    }

    function maybeString(data) {
        if (!data)
            return undefined;
        try {
            return textDecoder.decode(toBytes(data));
        }
        catch {
            return undefined;
        }
    }

    function uintValue(fields, field) {
        let item = firstField(fields, field, 0);
        return item ? item.value : undefined;
    }

    function signedValue(fields, field) {
        let item = firstField(fields, field, 0);
        return item ? item.signedValue : undefined;
    }

    function stringValue(fields, field) {
        return maybeString(firstField(fields, field, 2)?.value);
    }

    function bytesValue(fields, field) {
        return firstField(fields, field, 2)?.value;
    }

    function encodeWrapper(name, data) {
        return concat([
            encodeString(1, name),
            encodeBytes(2, data)
        ]);
    }

    function encodeRequestFrame(requestId, name, data) {
        let head = new Uint8Array([0x02, requestId & 0xff, (requestId >> 8) & 0xff]);
        return concat([head, encodeWrapper(name, data)]);
    }

    function decodeWrapper(data) {
        let fields = decodeFields(data);
        return {
            name: stringValue(fields, 1) || "",
            data: bytesValue(fields, 2) || new Uint8Array(0)
        };
    }

    function decodeFrame(frame) {
        let buf = toBytes(frame);
        if (!buf || buf.length < 3)
            throw new Error("frame too short");
        return {
            type: buf[0],
            requestId: buf[1] + buf[2] * 256,
            wrapper: decodeWrapper(buf.slice(3))
        };
    }

    function encodeReqGameRecord(uuid, clientVersionString) {
        return concat([
            encodeString(1, uuid),
            encodeString(2, clientVersionString)
        ]);
    }

    function decodeResGameRecord(data) {
        let fields = decodeFields(data);
        return {
            error: firstField(fields, 1),
            head: bytesValue(fields, 3) || new Uint8Array(0),
            data: bytesValue(fields, 4) || new Uint8Array(0)
        };
    }

    function encodeReqGameRecordListV2() {
        return concat([
            encodeUint(1, 0),
            encodeUint(2, 0),
            encodeUint(3, 0),
            encodeUint(4, 1),
            encodeUint(4, 2),
            encodeUint(4, 3),
            encodeUint(4, 4),
            encodeUint(5, 3),
            encodeUint(5, 4),
            encodeUint(6, 0),
            encodeUint(7, 1),
            encodeUint(7, 2),
            encodeUint(7, 3),
            encodeUint(7, 4),
            encodeUint(7, 6)
        ]);
    }

    function decodeResGameRecordListV2(data) {
        let fields = decodeFields(data);
        return { cursor: stringValue(fields, 2) || "" };
    }

    function encodeReqNextGameRecordList(cursor, mode = DEFAULT_PAGE_MODE) {
        return concat([
            encodeString(1, cursor),
            encodeUint(2, mode)
        ]);
    }

    function decodeListPlayer(data) {
        let fields = decodeFields(data);
        let player = {
            seat: uintValue(fields, 1),
            account_id: uintValue(fields, 2),
            nickname: stringValue(fields, 3)
        };
        let grading = signedValue(fields, 6);
        let partPoint = signedValue(fields, 8);
        if (grading !== undefined)
            player.grading_score = grading;
        if (partPoint !== undefined)
            player.part_point_1 = partPoint;
        return player;
    }

    function decodeListRecord(data) {
        let fields = decodeFields(data);
        return {
            uuid: stringValue(fields, 2) || "",
            start_time: uintValue(fields, 3),
            end_time: uintValue(fields, 4),
            category: uintValue(fields, 5),
            mode_id: uintValue(fields, 6),
            players: allFields(fields, 7, 2).map((item) => decodeListPlayer(item.value))
        };
    }

    function decodeResNextGameRecordList(data) {
        let fields = decodeFields(data);
        return { records: allFields(fields, 3, 2).map((item) => decodeListRecord(item.value)) };
    }

    function encodeReqGameRecordsDetail(uuidList) {
        return concat((uuidList || []).map((uuid) => encodeString(1, uuid)));
    }

    function decodeDetailRule(data) {
        let fields = decodeFields(data);
        let rule = {};
        let names = {
            1: "dora_count",
            2: "time_fixed",
            3: "time_add",
            4: "shiduan",
            5: "init_point",
            6: "fandian",
            7: "have_zimosun"
        };
        for (let item of fields) {
            if (item.wireType != 0)
                continue;
            rule[names[item.field] || ("field" + item.field)] = item.signedValue;
        }
        return rule;
    }

    function decodeMode(data) {
        let fields = decodeFields(data);
        let mode = {
            mode: uintValue(fields, 1),
            ai: uintValue(fields, 4) || 0
        };
        let detailRule = bytesValue(fields, 6);
        if (detailRule)
            mode.detail_rule = decodeDetailRule(detailRule);
        let extendInfo = stringValue(fields, 7);
        if (extendInfo)
            mode.extendinfo = extendInfo;
        return mode;
    }

    function decodeConfig(data) {
        let fields = decodeFields(data);
        let modeData = bytesValue(fields, 2);
        let mode = modeData ? decodeMode(modeData) : { mode: 0, ai: 0 };
        let config = {
            category: uintValue(fields, 1),
            mode,
            meta: {}
        };
        let metaData = bytesValue(fields, 3);
        if (metaData) {
            let metaFields = decodeFields(metaData);
            let contestUid = uintValue(metaFields, 1);
            if (contestUid !== undefined)
                config.meta.contest_uid = contestUid;
        }
        config.meta.mode_id = mode.mode;
        return config;
    }

    function decodeLevel(data) {
        let fields = decodeFields(data);
        let score = signedValue(fields, 3);
        if (score === undefined)
            score = signedValue(fields, 2);
        return {
            id: uintValue(fields, 1) || 0,
            score: score || 0
        };
    }

    function decodeAccount(data) {
        let fields = decodeFields(data);
        let account = {
            account_id: uintValue(fields, 1),
            seat: uintValue(fields, 2),
            nickname: stringValue(fields, 3) || "",
            level: { id: 0, score: 0 }
        };
        let avatarId = uintValue(fields, 4);
        if (avatarId !== undefined)
            account.avatar_id = avatarId;
        let levelData = bytesValue(fields, 5);
        if (levelData)
            account.level = decodeLevel(levelData);
        return account;
    }

    function decodeResultPlayer(data) {
        let fields = decodeFields(data);
        return {
            seat: uintValue(fields, 1),
            part_point_1: signedValue(fields, 2) || 0,
            grading_score: signedValue(fields, 3) || 0
        };
    }

    function decodeResult(data) {
        let fields = decodeFields(data);
        return { players: allFields(fields, 1, 2).map((item) => decodeResultPlayer(item.value)) };
    }

    function decodeDetailedGameRecord(data) {
        let fields = decodeFields(data);
        let configData = bytesValue(fields, 5);
        let resultData = bytesValue(fields, 12);
        return {
            uuid: stringValue(fields, 1) || "",
            start_time: uintValue(fields, 2),
            end_time: uintValue(fields, 3),
            config: configData ? decodeConfig(configData) : { category: 0, mode: { mode: 0, ai: 0 }, meta: {} },
            accounts: allFields(fields, 11, 2).map((item) => decodeAccount(item.value)),
            result: resultData ? decodeResult(resultData) : { players: [] }
        };
    }

    function decodeResGameRecordsDetail(data) {
        let fields = decodeFields(data);
        return { record_list: allFields(fields, 2, 2).map((item) => decodeDetailedGameRecord(item.value)) };
    }

    function bytesToBase64(data) {
        let buf = toBytes(data) || new Uint8Array(0);
        let chunk = 0x8000;
        let text = "";
        for (let i = 0; i < buf.length; i += chunk)
            text += String.fromCharCode.apply(null, buf.subarray(i, i + chunk));
        return btoa(text);
    }

    function isGateway(url) {
        return /^wss:\/\/[^/]+\/gateway(?:$|\?)/.test(String(url || ""));
    }

    function log(message, detail) {
        let item = {
            time: new Date().toISOString(),
            message,
            detail: detail || null
        };
        state.logs.push(item);
        if (state.logs.length > 200)
            state.logs.shift();
        console.log("[MajsoulPaipuBridge] " + message, detail || "");
        updateBadge();
        renderLogs();
    }

    function markDone(message, detail) {
        state.doneMessage = message;
        log("DONE: " + message, detail);
        if (state.ui.title)
            state.ui.title.style.color = "#ffffff";
        if (state.ui.statusLine) {
            state.ui.statusLine.style.background = "#17642a";
            state.ui.statusLine.style.color = "#ffffff";
            state.ui.statusLine.style.padding = "2px 4px";
            state.ui.statusLine.style.borderRadius = "4px";
        }
        setTimeout(() => {
            state.doneMessage = "";
            if (state.ui.title)
                state.ui.title.style.color = "#8ff0a4";
            if (state.ui.statusLine) {
                state.ui.statusLine.style.background = "transparent";
                state.ui.statusLine.style.color = "#b9f6c4";
                state.ui.statusLine.style.padding = "0";
                state.ui.statusLine.style.borderRadius = "0";
            }
            updateBadge();
        }, 15000);
    }

    function socketInfo(socket) {
        return state.sockets.find((item) => item.socket === socket);
    }

    function markSocket(socket, kind) {
        if (kind == "record") {
            if (state.recordSocket !== socket)
                log("record socket selected", socket.url);
            state.recordSocket = socket;
        }
        state.preferredSocket = socket;
    }

    function rememberNativeFrame(data, socket) {
        let buf = toBytes(data);
        if (!buf || buf.length < 3 || buf[0] != 0x02)
            return;
        let id = buf[1] + buf[2] * 256;
        state.nativeRequestIds.add(id);
        let info = socketInfo(socket);
        if (info) {
            info.sent++;
            info.lastNativeSentAt = Date.now();
        }
        try {
            let frame = decodeFrame(buf);
            let name = frame.wrapper.name;
            if (info && name) {
                info.lastMethod = name;
                if (!info.methods.includes(name)) {
                    info.methods.push(name);
                    if (info.methods.length > 30)
                        info.methods.shift();
                }
            }
            if (name && name.indexOf(".lq.Lobby.") == 0) {
                if (info)
                    info.lobbyMethods++;
                markSocket(socket, "preferred");
            }
            if (RECORD_METHODS.has(name)) {
                if (info)
                    info.recordMethods++;
                markSocket(socket, "record");
            }
        }
        catch (err) {
            log("failed to inspect native websocket frame", String(err && err.message || err));
        }
    }

    function nextRequestId() {
        for (let i = 0; i < 0x10000; ++i) {
            let id = state.nextBridgeRequestId;
            state.nextBridgeRequestId = (state.nextBridgeRequestId - 1) & 0xffff;
            if (id >= 0xff00 && !state.nativeRequestIds.has(id) && !state.pending.has(id))
                return id;
        }
        throw new Error("no available bridge request id");
    }

    function getOpenSocket() {
        if (state.recordSocket && state.recordSocket.readyState == NativeWebSocket.OPEN)
            return state.recordSocket;
        if (state.preferredSocket && state.preferredSocket.readyState == NativeWebSocket.OPEN)
            return state.preferredSocket;
        let verified = state.sockets
            .filter((item) => item.socket.readyState == NativeWebSocket.OPEN && item.lobbyMethods > 0)
            .sort((a, b) => (b.recordMethods - a.recordMethods) || (b.lobbyMethods - a.lobbyMethods) || (b.lastNativeSentAt - a.lastNativeSentAt));
        if (verified.length)
            return verified[0].socket;
        let open = state.sockets.filter((item) => item.socket.readyState == NativeWebSocket.OPEN);
        return open.length == 1 ? open[0].socket : null;
    }

    function addSocketCandidate(list, socket) {
        if (!socket || socket.readyState != NativeWebSocket.OPEN || list.includes(socket))
            return;
        list.push(socket);
    }

    function openSocketCandidates() {
        let list = [];
        addSocketCandidate(list, state.recordSocket);
        addSocketCandidate(list, state.preferredSocket);
        let open = state.sockets
            .filter((item) => item.socket.readyState == NativeWebSocket.OPEN && item.lobbyMethods > 0 && !list.includes(item.socket))
            .sort((a, b) => (b.recordMethods - a.recordMethods) || (b.lobbyMethods - a.lobbyMethods) || (b.lastNativeSentAt - a.lastNativeSentAt));
        for (let item of open)
            addSocketCandidate(list, item.socket);
        if (!list.length) {
            let allOpen = state.sockets.filter((item) => item.socket.readyState == NativeWebSocket.OPEN);
            if (allOpen.length == 1)
                addSocketCandidate(list, allOpen[0].socket);
        }
        return list;
    }

    function sendBridgeRequest(name, data, timeoutMs = 30000, socketOverride = null) {
        let socket = socketOverride || getOpenSocket();
        if (!socket)
            return Promise.reject(new Error("no verified lobby websocket; wait until the game reaches lobby or open a paipu once"));
        let requestId = nextRequestId();
        let frame = encodeRequestFrame(requestId, name, data);
        return new Promise((resolve, reject) => {
            let timer = setTimeout(() => {
                state.pending.delete(requestId);
                reject(new Error("bridge request timeout: " + name));
            }, timeoutMs);
            state.pending.set(requestId, { name, resolve, reject, timer });
            socket.__majsoulPaipuNativeSend(frame);
            log("bridge request sent", { name, requestId, socket: socket.url });
        });
    }

    function handleIncoming(data, event) {
        let buf = toBytes(data);
        if (!buf || buf.length < 3 || buf[0] != 0x03)
            return false;
        let requestId = buf[1] + buf[2] * 256;
        let item = state.pending.get(requestId);
        if (!item)
            return false;
        state.pending.delete(requestId);
        clearTimeout(item.timer);
        try {
            item.resolve(decodeFrame(buf));
            log("bridge response received", { name: item.name, requestId });
        }
        catch (err) {
            item.reject(err);
        }
        if (event) {
            event.stopImmediatePropagation();
            event.preventDefault();
        }
        updateBadge();
        return true;
    }

    function patchSocket(socket, url) {
        let id = state.sockets.length + 1;
        state.sockets.push({
            id,
            socket,
            url,
            createdAt: Date.now(),
            openedAt: null,
            closedAt: null,
            sent: 0,
            received: 0,
            lobbyMethods: 0,
            recordMethods: 0,
            lastNativeSentAt: 0,
            lastMethod: "",
            methods: []
        });
        socket.__majsoulPaipuNativeSend = socket.send.bind(socket);
        socket.send = function (data) {
            rememberNativeFrame(data, socket);
            return socket.__majsoulPaipuNativeSend(data);
        };
        socket.addEventListener("open", () => {
            let info = socketInfo(socket);
            if (info)
                info.openedAt = Date.now();
            log("gateway websocket opened", { id, url });
        });
        socket.addEventListener("close", () => {
            let info = socketInfo(socket);
            if (info)
                info.closedAt = Date.now();
            log("gateway websocket closed", { id, url });
        });
        socket.addEventListener("message", (event) => {
            let info = socketInfo(socket);
            if (info)
                info.received++;
            handleIncoming(event.data, event);
        }, true);
        log("gateway websocket created", { id, url });
        updateBadge();
    }

    function PatchedWebSocket(url, protocols) {
        let socket = protocols === undefined ? new NativeWebSocket(url) : new NativeWebSocket(url, protocols);
        if (isGateway(url))
            patchSocket(socket, String(url));
        return socket;
    }

    PatchedWebSocket.prototype = NativeWebSocket.prototype;
    for (let key of ["CONNECTING", "OPEN", "CLOSING", "CLOSED"]) {
        Object.defineProperty(PatchedWebSocket, key, {
            configurable: false,
            enumerable: true,
            value: NativeWebSocket[key]
        });
    }
    Object.defineProperty(window, "WebSocket", {
        configurable: true,
        writable: true,
        value: PatchedWebSocket
    });

    async function fetchGameRecord(uuid, clientVersionString = CLIENT_VERSION) {
        let requestData = encodeReqGameRecord(uuid, clientVersionString);
        let candidates = openSocketCandidates();
        let last = null;
        for (let socket of candidates) {
            let info = socketInfo(socket);
            let frame = await sendBridgeRequest(
                ".lq.Lobby.fetchGameRecord",
                requestData,
                30000,
                socket
            );
            let res = decodeResGameRecord(frame.wrapper.data);
            last = {
                uuid,
                requestId: frame.requestId,
                socketId: info?.id || null,
                socketUrl: socket.url,
                headBase64: bytesToBase64(res.head),
                dataBase64: bytesToBase64(res.data),
                headBytes: res.head.length,
                dataBytes: res.data.length
            };
            if (res.data.length > 0) {
                markSocket(socket, "record");
                return last;
            }
            log("paipu data returned empty; trying next socket", {
                requestId: frame.requestId,
                socketId: info?.id || null,
                socket: socket.url,
                uuid
            });
        }
        throw new Error("fetchGameRecord returned 0 bytes on all open sockets: " + uuid + (last ? " lastSocket=" + last.socketUrl : ""));
    }

    async function fetchGameRecordListCursor() {
        let frame = await sendBridgeRequest(
            ".lq.Lobby.fetchGameRecordListV2",
            encodeReqGameRecordListV2()
        );
        let res = decodeResGameRecordListV2(frame.wrapper.data);
        if (!res.cursor)
            throw new Error("fetchGameRecordListV2 returned empty cursor");
        return { cursor: res.cursor, requestId: frame.requestId };
    }

    async function fetchNextGameRecordList(cursor, mode = DEFAULT_PAGE_MODE) {
        let frame = await sendBridgeRequest(
            ".lq.Lobby.fetchNextGameRecordList",
            encodeReqNextGameRecordList(cursor, mode)
        );
        let res = decodeResNextGameRecordList(frame.wrapper.data);
        return { records: res.records, requestId: frame.requestId };
    }

    async function fetchGameRecordsDetail(uuidList) {
        let frame = await sendBridgeRequest(
            ".lq.Lobby.fetchGameRecordsDetail",
            encodeReqGameRecordsDetail(uuidList || [])
        );
        let res = decodeResGameRecordsDetail(frame.wrapper.data);
        return { record_list: res.record_list, requestId: frame.requestId };
    }

    async function fetchGameRecordsDetailWithFallback(uuidList) {
        let batch = uuidList || [];
        let candidates = openSocketCandidates();
        let last = null;
        for (let socket of candidates) {
            let info = socketInfo(socket);
            let frame = await sendBridgeRequest(
                ".lq.Lobby.fetchGameRecordsDetail",
                encodeReqGameRecordsDetail(batch),
                30000,
                socket
            );
            let res = decodeResGameRecordsDetail(frame.wrapper.data);
            last = { record_list: res.record_list, requestId: frame.requestId, socketId: info?.id || null, socketUrl: socket.url };
            if (res.record_list.length > 0 || batch.length == 0) {
                markSocket(socket, "record");
                return last;
            }
            log("metadata detail returned empty; trying next socket", {
                requestId: frame.requestId,
                socketId: info?.id || null,
                socket: socket.url,
                batchSize: batch.length
            });
        }
        return last || { record_list: [], requestId: null, socketId: null, socketUrl: null };
    }

    async function collectRecordUuids(options = {}) {
        let maxPages = Number(options.maxPages || 200);
        let delayMs = Number(options.delayMs || 5000);
        let mode = Number(options.mode || DEFAULT_PAGE_MODE);
        let cursor = (await fetchGameRecordListCursor()).cursor;
        let seen = new Set();
        let records = [];
        for (let page = 0; page < maxPages; ++page) {
            if (page > 0 || options.delayBeforeFirstPage !== false)
                await new Promise((resolve) => setTimeout(resolve, delayMs));
            let pageRes = await fetchNextGameRecordList(cursor, mode);
            if (!pageRes.records.length)
                break;
            let newCount = 0;
            for (let record of pageRes.records) {
                if (!record.uuid || seen.has(record.uuid))
                    continue;
                seen.add(record.uuid);
                records.push(record);
                newCount++;
            }
            log("metadata page collected", { page: page + 1, pageRecords: pageRes.records.length, total: records.length });
            if (newCount == 0)
                break;
        }
        return records;
    }

    async function collectRecordDetails(options = {}) {
        let listRecords = await collectRecordUuids(options);
        let uuids = listRecords.map((record) => record.uuid).filter(Boolean);
        if (!uuids.length)
            throw new Error("no metadata UUIDs were collected");
        let batchSize = Number(options.detailBatchSize || 20);
        let detailDelayMs = Number(options.detailDelayMs || 5000);
        let records = [];
        for (let i = 0; i < uuids.length; i += batchSize) {
            if (i > 0)
                await new Promise((resolve) => setTimeout(resolve, detailDelayMs));
            let batch = uuids.slice(i, i + batchSize);
            let detail = await fetchGameRecordsDetailWithFallback(batch);
            if (!detail.record_list.length)
                throw new Error("metadata detail returned 0 records for a non-empty batch; no usable record socket found");
            records.push(...detail.record_list);
            log("metadata detail batch collected", {
                batch: Math.floor(i / batchSize) + 1,
                batchRecords: detail.record_list.length,
                total: records.length,
                socketId: detail.socketId
            });
        }
        return records;
    }

    function jsonl(records) {
        return records.map((record) => JSON.stringify(record)).join("\n") + (records.length ? "\n" : "");
    }

    function safeFilePart(value) {
        return String(value || "majsoul").replace(/[\\/:*?"<>|]+/g, "_");
    }

    function timestamp() {
        let d = new Date();
        let pad = (n) => String(n).padStart(2, "0");
        return d.getFullYear() + pad(d.getMonth() + 1) + pad(d.getDate()) + "-" + pad(d.getHours()) + pad(d.getMinutes()) + pad(d.getSeconds());
    }

    function downloadBlob(filename, blob) {
        let url = URL.createObjectURL(blob);
        let anchor = document.createElement("a");
        anchor.href = url;
        anchor.download = filename;
        anchor.style.display = "none";
        document.body.appendChild(anchor);
        anchor.click();
        setTimeout(() => {
            URL.revokeObjectURL(url);
            anchor.remove();
        }, 1000);
    }

    function base64ToBytes(value) {
        let text = atob(value || "");
        let out = new Uint8Array(text.length);
        for (let i = 0; i < text.length; ++i)
            out[i] = text.charCodeAt(i);
        return out;
    }

    function currentDelayMs() {
        let value = Number(state.ui.delayInput?.value || 5000);
        if (!Number.isFinite(value) || value < 1000)
            value = 5000;
        return value;
    }

    const CRC32_TABLE = (() => {
        let table = new Uint32Array(256);
        for (let i = 0; i < 256; ++i) {
            let c = i;
            for (let j = 0; j < 8; ++j)
                c = (c & 1) ? (0xedb88320 ^ (c >>> 1)) : (c >>> 1);
            table[i] = c >>> 0;
        }
        return table;
    })();

    function crc32(data) {
        let crc = 0xffffffff;
        for (let i = 0; i < data.length; ++i)
            crc = CRC32_TABLE[(crc ^ data[i]) & 0xff] ^ (crc >>> 8);
        return (crc ^ 0xffffffff) >>> 0;
    }

    function dosDateTime(date = new Date()) {
        let year = Math.max(1980, date.getFullYear());
        let dosTime = (date.getHours() << 11) | (date.getMinutes() << 5) | Math.floor(date.getSeconds() / 2);
        let dosDate = ((year - 1980) << 9) | ((date.getMonth() + 1) << 5) | date.getDate();
        return { dosTime, dosDate };
    }

    function u16(value) {
        let out = new Uint8Array(2);
        out[0] = value & 0xff;
        out[1] = (value >>> 8) & 0xff;
        return out;
    }

    function u32(value) {
        let out = new Uint8Array(4);
        out[0] = value & 0xff;
        out[1] = (value >>> 8) & 0xff;
        out[2] = (value >>> 16) & 0xff;
        out[3] = (value >>> 24) & 0xff;
        return out;
    }

    function makeZipBlob(entries) {
        let localParts = [];
        let centralParts = [];
        let offset = 0;
        let now = dosDateTime();
        for (let entry of entries) {
            let nameBytes = textEncoder.encode(entry.name);
            let data = toBytes(entry.data) || new Uint8Array(0);
            let crc = crc32(data);
            if (data.length > 0xffffffff || offset > 0xffffffff)
                throw new Error("zip64 is not supported by this userscript");

            let localHeader = concat([
                u32(0x04034b50),
                u16(20),
                u16(0x0800),
                u16(0),
                u16(now.dosTime),
                u16(now.dosDate),
                u32(crc),
                u32(data.length),
                u32(data.length),
                u16(nameBytes.length),
                u16(0),
                nameBytes
            ]);
            localParts.push(localHeader, data);

            let centralHeader = concat([
                u32(0x02014b50),
                u16(20),
                u16(20),
                u16(0x0800),
                u16(0),
                u16(now.dosTime),
                u16(now.dosDate),
                u32(crc),
                u32(data.length),
                u32(data.length),
                u16(nameBytes.length),
                u16(0),
                u16(0),
                u16(0),
                u16(0),
                u32(0),
                u32(offset),
                nameBytes
            ]);
            centralParts.push(centralHeader);
            offset += localHeader.length + data.length;
        }

        let centralOffset = offset;
        let centralSize = centralParts.reduce((sum, part) => sum + part.length, 0);
        if (entries.length > 0xffff)
            throw new Error("too many files for non-zip64 zip");
        let end = concat([
            u32(0x06054b50),
            u16(0),
            u16(0),
            u16(entries.length),
            u16(entries.length),
            u32(centralSize),
            u32(centralOffset),
            u16(0)
        ]);
        return new Blob([...localParts, ...centralParts, end], { type: "application/zip" });
    }

    function parseUuidInput(text) {
        let uuids = [];
        let seen = new Set();
        for (let line of String(text || "").split(/\r?\n/)) {
            let raw = line.trim();
            if (!raw)
                continue;
            let uuid = "";
            if (raw[0] == "{") {
                try {
                    let obj = JSON.parse(raw);
                    uuid = obj.uuid || obj.game_uuid || obj.gameUuid || "";
                }
                catch {
                    uuid = "";
                }
            }
            if (!uuid) {
                let match = raw.match(/\b\d{6}-[0-9a-fA-F-]{36}\b/);
                uuid = match ? match[0] : raw;
            }
            if (!uuid || seen.has(uuid))
                continue;
            seen.add(uuid);
            uuids.push(uuid);
        }
        return uuids;
    }

    async function startMetadataExport() {
        if (state.collecting)
            return;
        state.collecting = true;
        setButtonsDisabled(true);
        try {
            let delayMs = currentDelayMs();
            log("metadata export started");
            let records = await collectRecordDetails({
                maxPages: 200,
                delayMs,
                detailBatchSize: 20,
                detailDelayMs: delayMs
            });
            if (!records.length)
                throw new Error("metadata export produced 0 records; not saving an empty JSONL");
            let body = jsonl(records);
            downloadBlob("majsoul-paipu-metadata-" + timestamp() + ".jsonl", new Blob([body], { type: "application/x-ndjson;charset=utf-8" }));
            markDone("metadata JSONL download created", { records: records.length });
        }
        catch (err) {
            log("metadata export failed", String(err && err.message || err));
        }
        finally {
            state.collecting = false;
            setButtonsDisabled(false);
            updateBadge();
        }
    }

    async function startPaipuDownload(text, options = {}) {
        if (state.downloading)
            return;
        let uuids = parseUuidInput(text);
        if (!uuids.length) {
            log("download input has no uuid");
            return;
        }
        state.downloading = true;
        setButtonsDisabled(true);
        try {
            let delayMs = Number(options.delayMs || 5000);
            let saveHeader = options.saveHeader !== false;
            let zipEntries = [];
            log("paipu download started", { count: uuids.length, delayMs });
            for (let i = 0; i < uuids.length; ++i) {
                if (i > 0)
                    await new Promise((resolve) => setTimeout(resolve, delayMs));
                let uuid = uuids[i];
                log("paipu downloading", { index: i + 1, count: uuids.length, uuid });
                let res = await fetchGameRecord(uuid);
                let baseName = safeFilePart(uuid);
                zipEntries.push({
                    name: baseName + ".bin",
                    data: base64ToBytes(res.dataBase64)
                });
                if (saveHeader) {
                    let header = {
                        webgl: true,
                        uuid,
                        requestId: res.requestId,
                        head_wire_base64: res.headBase64,
                        data_bytes: res.dataBytes
                    };
                    zipEntries.push({
                        name: baseName + ".header.json",
                        data: textEncoder.encode(JSON.stringify(header, null, 2))
                    });
                }
                log("paipu fetched", { index: i + 1, count: uuids.length, uuid, dataBytes: res.dataBytes });
            }
            let zipName = "majsoul-paipus-" + timestamp() + ".zip";
            let zipBlob = makeZipBlob(zipEntries);
            downloadBlob(zipName, zipBlob);
            markDone("paipu ZIP download created", { count: uuids.length, files: zipEntries.length, zipName, bytes: zipBlob.size });
        }
        catch (err) {
            log("paipu download failed", String(err && err.message || err));
        }
        finally {
            state.downloading = false;
            setButtonsDisabled(false);
            updateBadge();
        }
    }

    function status() {
        return {
            installed: true,
            version: VERSION,
            href: location.href,
            sockets: state.sockets.map((item) => ({
                id: item.id,
                url: item.url,
                readyState: item.socket.readyState,
                binaryType: item.socket.binaryType,
                sent: item.sent,
                received: item.received,
                lobbyMethods: item.lobbyMethods,
                recordMethods: item.recordMethods,
                lastMethod: item.lastMethod,
                methods: item.methods.slice(-8)
            })),
            preferredSocketId: socketInfo(state.preferredSocket)?.id || null,
            recordSocketId: socketInfo(state.recordSocket)?.id || null,
            pending: state.pending.size,
            nativeRequestIds: state.nativeRequestIds.size,
            lastLogs: state.logs.slice(-10)
        };
    }

    function setButtonsDisabled(disabled) {
        for (let button of [state.ui.updateButton, state.ui.downloadButton, state.ui.downloadStartButton]) {
            if (!button)
                continue;
            button.disabled = !!disabled;
            button.style.opacity = disabled ? ".45" : "1";
            button.style.cursor = disabled ? "not-allowed" : "pointer";
            button.style.background = disabled ? "#1a1d1a" : "#132016";
        }
        if (state.ui.delayInput)
            state.ui.delayInput.disabled = !!disabled;
    }

    function renderLogs() {
        if (!state.ui.log)
            return;
        state.ui.log.textContent = state.logs.slice(-28).map((item) => {
            let detail = item.detail == null ? "" : " " + (typeof item.detail == "string" ? item.detail : JSON.stringify(item.detail));
            return item.time.slice(11, 19) + " " + item.message + detail;
        }).join("\n");
        state.ui.log.scrollTop = state.ui.log.scrollHeight;
    }

    function button(label) {
        let node = document.createElement("button");
        node.type = "button";
        node.textContent = label;
        node.style.cssText = [
            "appearance:none",
            "border:1px solid rgba(143,240,164,.45)",
            "background:#132016",
            "color:#d7ffe0",
            "border-radius:4px",
            "padding:5px 8px",
            "font:12px/1.2 sans-serif",
            "cursor:pointer"
        ].join(";");
        node.addEventListener("mouseenter", () => {
            if (!node.disabled)
                node.style.background = "#1a3320";
        });
        node.addEventListener("mouseleave", () => {
            node.style.background = node.disabled ? "#1a1d1a" : "#132016";
        });
        return node;
    }

    function inputNumber(value) {
        let node = document.createElement("input");
        node.type = "number";
        node.min = "1000";
        node.step = "1000";
        node.value = String(value);
        node.style.cssText = [
            "width:88px",
            "border:1px solid rgba(143,240,164,.35)",
            "background:#07100a",
            "color:#d7ffe0",
            "border-radius:4px",
            "padding:4px",
            "font:12px monospace"
        ].join(";");
        return node;
    }

    function openDownloadDialog() {
        if (state.ui.modal) {
            state.ui.modal.style.display = "block";
            return;
        }

        let modal = document.createElement("div");
        modal.id = "MajsoulPaipuBridgeDownloadDialog";
        modal.style.cssText = [
            "position:fixed",
            "left:8px",
            "top:192px",
            "z-index:2147483647",
            "width:360px",
            "background:rgba(3,10,6,.94)",
            "color:#d7ffe0",
            "border:1px solid rgba(143,240,164,.55)",
            "border-radius:6px",
            "box-shadow:0 8px 24px rgba(0,0,0,.35)",
            "padding:8px",
            "font:12px/1.35 sans-serif"
        ].join(";");

        let title = document.createElement("div");
        title.textContent = "Download Paipu ZIP";
        title.style.cssText = "font-weight:700;margin-bottom:6px";

        let textarea = document.createElement("textarea");
        textarea.placeholder = "Paste metadata JSONL, or one UUID per line";
        textarea.style.cssText = [
            "box-sizing:border-box",
            "width:100%",
            "height:150px",
            "resize:vertical",
            "border:1px solid rgba(143,240,164,.35)",
            "background:#07100a",
            "color:#d7ffe0",
            "border-radius:4px",
            "padding:6px",
            "font:12px/1.35 monospace"
        ].join(";");

        let controls = document.createElement("div");
        controls.style.cssText = "display:flex;align-items:center;gap:6px;margin-top:6px;flex-wrap:wrap";

        let delayLabel = document.createElement("label");
        delayLabel.textContent = "Delay ms";
        delayLabel.style.cssText = "display:flex;align-items:center;gap:4px";
        let delayInput = inputNumber(currentDelayMs());
        delayLabel.appendChild(delayInput);

        let headerLabel = document.createElement("label");
        headerLabel.style.cssText = "display:flex;align-items:center;gap:4px";
        let headerInput = document.createElement("input");
        headerInput.type = "checkbox";
        headerInput.checked = true;
        headerLabel.appendChild(headerInput);
        headerLabel.appendChild(document.createTextNode("Save headers"));

        let start = button("Download ZIP");
        state.ui.downloadStartButton = start;
        start.addEventListener("click", () => {
            startPaipuDownload(textarea.value, {
                delayMs: Number(delayInput.value || 5000),
                saveHeader: headerInput.checked
            });
        });

        let close = button("Close");
        close.addEventListener("click", () => {
            modal.style.display = "none";
        });

        controls.appendChild(delayLabel);
        controls.appendChild(headerLabel);
        controls.appendChild(start);
        controls.appendChild(close);
        modal.appendChild(title);
        modal.appendChild(textarea);
        modal.appendChild(controls);
        document.body.appendChild(modal);
        state.ui.modal = modal;
    }

    function ensureBadge() {
        if (state.badge || !document.body)
            return;
        let panel = document.createElement("div");
        panel.id = "MajsoulPaipuBridgePanel";
        panel.style.cssText = [
            "position:fixed",
            "left:8px",
            "top:8px",
            "z-index:2147483647",
            "width:360px",
            "font:12px/1.35 sans-serif",
            "background:rgba(3,10,6,.88)",
            "color:#d7ffe0",
            "padding:8px",
            "border:1px solid rgba(143,240,164,.55)",
            "border-radius:6px",
            "box-shadow:0 8px 24px rgba(0,0,0,.35)",
            "pointer-events:auto"
        ].join(";");

        let title = document.createElement("div");
        title.textContent = "PaipuBridge " + VERSION;
        title.style.cssText = "font-weight:700;margin-bottom:4px;color:#8ff0a4";

        let statusLine = document.createElement("div");
        statusLine.style.cssText = "font:12px/1.35 monospace;margin-bottom:6px;color:#b9f6c4";

        let controls = document.createElement("div");
        controls.style.cssText = "display:flex;gap:6px;margin-bottom:6px;flex-wrap:wrap";

        let settings = document.createElement("div");
        settings.style.cssText = "display:flex;align-items:center;gap:6px;margin-bottom:6px;flex-wrap:wrap;color:#c9f6d1";

        let delayLabel = document.createElement("label");
        delayLabel.textContent = "Delay ms";
        delayLabel.style.cssText = "display:flex;align-items:center;gap:4px";
        let delayInput = inputNumber(5000);
        state.ui.delayInput = delayInput;
        delayLabel.appendChild(delayInput);
        settings.appendChild(delayLabel);

        let updateButton = button("Update Metadata");
        updateButton.addEventListener("click", startMetadataExport);

        let downloadButton = button("Download Paipu");
        downloadButton.addEventListener("click", openDownloadDialog);

        controls.appendChild(updateButton);
        controls.appendChild(downloadButton);

        let logBox = document.createElement("pre");
        logBox.style.cssText = [
            "box-sizing:border-box",
            "height:86px",
            "margin:0",
            "overflow:auto",
            "white-space:pre-wrap",
            "word-break:break-word",
            "background:#07100a",
            "color:#9be7aa",
            "border:1px solid rgba(143,240,164,.25)",
            "border-radius:4px",
            "padding:5px",
            "font:11px/1.35 monospace"
        ].join(";");

        panel.appendChild(title);
        panel.appendChild(statusLine);
        panel.appendChild(settings);
        panel.appendChild(controls);
        panel.appendChild(logBox);
        document.body.appendChild(panel);
        state.badge = panel;
        state.ui.title = title;
        state.ui.statusLine = statusLine;
        state.ui.log = logBox;
        state.ui.updateButton = updateButton;
        state.ui.downloadButton = downloadButton;
        updateBadge();
        renderLogs();
    }

    function updateBadge() {
        if (!state.badge)
            return;
        let open = state.sockets.filter((item) => item.socket.readyState == NativeWebSocket.OPEN).length;
        let verified = state.sockets.filter((item) => item.socket.readyState == NativeWebSocket.OPEN && item.lobbyMethods > 0).length;
        let recordId = socketInfo(state.recordSocket)?.id || "-";
        if (state.ui.statusLine) {
            let busy = state.collecting ? " collecting" : state.downloading ? " downloading" : "";
            let done = state.doneMessage ? " DONE: " + state.doneMessage : "";
            state.ui.statusLine.textContent = "ws " + open + "/" + state.sockets.length + " verified " + verified + " record " + recordId + " pending " + state.pending.size + busy + done;
        }
        setButtonsDisabled(state.collecting || state.downloading);
    }

    function installBadgeWhenReady() {
        if (document.body) {
            ensureBadge();
            return;
        }
        document.addEventListener("DOMContentLoaded", ensureBadge, { once: true });
    }

    window.MajsoulPaipuBridge = {
        version: VERSION,
        status,
        fetchGameRecord,
        fetchGameRecordListCursor,
        fetchNextGameRecordList,
        fetchGameRecordsDetail,
        collectRecordUuids,
        collectRecordDetails,
        startMetadataExport,
        startPaipuDownload,
        logs: () => state.logs.slice()
    };

    installBadgeWhenReady();
    setInterval(updateBadge, 1000);
    log("userscript installed");
})();
