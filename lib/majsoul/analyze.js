"use strict";

var clientrequest = {};
var UserID = -1;
var paipugamedata = {};
var request = require('request');
var { config } = require('../config.js');
var { dialog } = require('electron');

function reporterror(data){
    data.version = config.get('Version');
    data.time = (new Date()).toString();
    let errconf = config.get('SendError');
    let always = false, send = true;
    if (errconf != undefined)
        [send, always] = errconf;
    
    function sendmain(data){
        request({
            url: `http://t.zyr17.cn/MajsoulPaipuAnalyzer/ErrorCollect`, 
            method: "POST", 
            json: true, 
            headers: { 
                "content-type": "application/json"
            }, 
            body: data 
        }, function (error, response, body) {
            if (!error && response.statusCode == 200) {
                console.log(body)
            }
            else console.error('Status Code: ' + response.statusCode);
        });
    }

    if (!always){
        dialog.showMessageBox({
            type: 'error',
            title: '错误',
            message: '出现错误，是否将错误数据上传以便于作者调试？' + (data.message != undefined ? '\n\n错误信息：\n' + data.message : ''),
            checkboxLabel: '之后总是执行该选择',
            noLink: true,
            buttons: ['上传', '不上传']
        }, function (response, checked) {
            always = checked;
            send = response == 0;
            if (always) config.set('SendError', [send, always], false);
            if (!send) return;
            sendmain(data);
        });
        return;
    }
    if (!send) return;
    sendmain(data);
}

function decodeUTF8(data, start = undefined, length = undefined){
    if (start == undefined){
        start = 0;
        length = data.length;
    }
    let str = '';
    for (let i = start; i < start + length; i ++ )
        str += '%' + data[i].toString(16);
    return [decodeURIComponent(str), start == undefined ? start : start + length];
}

function getnumber(arr, start){
    var res = 0, mul = 1, str = '';
    function addzero(num, len){
        let res = num.toString(2);
        if (len > res.length)
            res = Array(len - res.length + 1).join('0') + res;
        return res;
    }
    for (; arr[start] > 127; start ++ , mul *= 128)
        str = addzero(arr[start] - 128, 7) + str;
    str = addzero(arr[start], 8) + str;
    let reverse = '0';
    if (str.length >= 64){
        reverse = '1';
        str = str.slice(str.length - 64, 999);
    }
    //console.log(str, reverse);
    for (let i = 0; i < str.length; i ++ ){
        res *= 2;
        res += reverse == str[i] ? 0 : 1;
    }
    if (reverse == '1'){
        res += 1;
        res *= -1;
    }
    return [res, start + 1];
}

function getstring(arr, start, length){
    var res = ''
    for (var i = 0; i < length; i ++ )
        res += String.fromCharCode(arr[i + start]);
    return [res, start + length];
}

function analyzelogin(sender, arr, start){
    if (arr[start] != 0x12){
        reporterror({
            position: 'analyzelogin',
            message: 'in analyzelogin: not start with 0x12',
            data: {
                length: arr.length,
                arr: arr.slice(0, arr.length - 36),//为安全考虑，不上传最后36字节的access_token
                start
            }
        });
        return;
    }
    let length, flag = false, unknownnumber = '';
    [length, start] = getnumber(arr, start + 1);
    let data = [];
    for (let end = length + start; start < end; ){
        if (arr[start] == 0x10){
            let id;
            [id, start] = getnumber(arr, start + 1);
            UserID = id;
            data.push([0x08, UserID]);
            flag = true;
        }
        else if (arr[start] == 0x32){
            //access token, 不能上传
            [length, start] = getnumber(arr, start + 1);
            start += length;
        }
        else{
            let cmd, ostart = start;
            [cmd, start] = getnumber(arr, start);
            let addlen = cmd % 8 == 2;
            unknownnumber += ' ' + cmd;
            [cmd, start] = getnumber(arr, start);
            if (addlen)
                start += cmd;
            let dd = [];
            for (; ostart < start; dd.push(arr[ostart ++ ]));
            data.push(dd);
        }
    }
    if (!flag){
        reporterror({
            position: 'analyzelogin',
            message: 'in analyzelogin: not find ID',
            data: {
                length: arr.length,
                data
            }
        });
        return;
    }
}

function analyzepaipu(arr, start){
    if (arr[start] != 0x12){
        reporterror({
            position: 'analyzepaipu',
            message: 'in analyzepaipu: not start with 0x12',
            data: {
                arr,
                start
            }
        });
        return;
    }
    let length;
    [length, start] = getnumber(arr, start + 1);
    let unknownnumber = '';
    for (let ss = start; ss < arr.length;){
        let paipulength;
        if (arr[ss] == 0x1a){
            [paipulength, ss] = getnumber(arr, ss + 1);
        }
        else{
            let cmd;
            [cmd, ss] = getnumber(arr, ss);
            let addlen = cmd % 8 == 2;
            unknownnumber += ' ' + cmd;
            [cmd, ss] = getnumber(arr, ss);
            if (addlen)
                ss += cmd;
            continue;
        }
        let valid = true;
        let data = {
            source: 'majsoul',
            accountid: UserID,
            playerdata: [
                {id: 0, name: '电脑', rank: 0, pt: 0}, 
                {id: 0, name: '电脑', rank: 0, pt: 0}, 
                {id: 0, name: '电脑', rank: 0, pt: 0}, 
                {id: 0, name: '电脑', rank: 0, pt: 0}
            ],
            extra: {}
        };
        let unknownnumber = '';
        for (let i = ss; i < paipulength + ss && valid; ){
            //console.log(data, i.toString(16));
            if (arr[i] == 0x0a){
                //牌谱ID
                if (!(arr[i + 1] == 0x2b || arr[i + 1] == 0x24)){
                    valid = false;
                    break;
                }
                let idlength = arr[i + 1];
                i = i + 2;
                if (i + idlength > arr.length){
                    valid = false;
                    break;
                }
                let id = '';
                for (let j = 0; j < 0x2b; j ++ ){
                    let now = arr[i + j];
                    if (now == 0x2d || 
                        now >= 0x30 && now <= 0x39 || 
                        now >= 0x61 && now <= 0x66
                    )
                        id += String.fromCharCode(now);
                }
                valid = id.length == idlength;
                i += idlength;
                data.extra.id = id;
                data.extra.version = idlength == 0x2b ? 1 : 0;
            }
            else if (arr[i] == 0x10){
                let time;
                [time, i] = getnumber(arr, i + 1);
                data.starttime = time;
            }
            else if (arr[i] == 0x18){
                let time;
                [time, i] = getnumber(arr, i + 1);
                data.endtime = time;
            }
            else if (arr[i] == 0x2a){
                let length;
                [length, i] = getnumber(arr, i + 1);
                let roomres = {
                    aka: 3,
                    timeone: 5,
                    timeshare: 20,
                    startpoint: 25000,
                    minendpoint: 30000,
                    kuitan: true,
                    hint: true,
                    comdifficulty: 2
                };
                for (let aend = i + length; i < aend; ){
                    if (arr[i] == 0x08){
                        let roomnum;
                        [roomnum, i] = getnumber(arr, i + 1);
                        if (roomnum == 1)
                            roomres.type = 'friend';
                        else if (roomnum == 2)
                            roomres.type = 'match';
                        else if (roomnum == 4)
                            roomres.type = 'contest';
                        else
                            roomres.type = 'unknown' + roomnum;
                    }
                    else if (arr[i] == 0x12){
                        let alen;
                        [alen, i] = getnumber(arr, i + 1);
                        let unknownnumber = '';
                        for (let aaend = i + alen; i < aaend; ){
                            if (arr[i] == 0x08){
                                let number;
                                [number, i] = getnumber(arr, i + 1);
                                if (number % 10 == 1)
                                    //东
                                    roomres.round = 4;
                                else if (number % 10 == 2)
                                    //南
                                    roomres.round = 8;
                                else if (number % 10 == 3)
                                    //人机
                                    roomres.round = 1;
                                else if (number % 10 == 4)
                                    //一局
                                    roomres.round = 1;
                                if (number > 10)
                                    roomres.player = 3;
                                else
                                    roomres.player = 4;
                            }
                            else if (arr[i] == 0x20){
                                roomres.x20 = arr[i + 1];
                                i += 2;
                            }
                            else if (arr[i] == 0x2a){
                                let len, json;
                                [len, i] = getnumber(arr, i + 1);
                                [json, i] = getstring(arr, i, len);
                                //console.log(json);
                                json = JSON.parse(json);
                                roomres.timeone = json.time_fixed;
                                roomres.timeshare = json.time_add;
                                roomres.aka = json.dora_count;
                                roomres.kuitan = json.shiduan;
                                roomres.startpoint = json.init_point;
                                roomres.minendpoint = json.fandian;
                                roomres.hint = json.bianjietishi;
                                roomres.comdifficulty = 2;
                            }
                            else if (arr[i] == 0x32){
                                let len3;
                                [len3, i] = getnumber(arr, i + 1);
                                let unknownnumber = '';
                                for (let end3 = len3 + i; i < end3; ){
                                    if (arr[i] == 0x08){
                                        [roomres.timeone, i] = getnumber(arr, i + 1);
                                    }
                                    else if (arr[i] == 0x10){
                                        [roomres.timeshare, i] = getnumber(arr, i + 1);
                                    }
                                    else if (arr[i] == 0x18){
                                        [roomres.aka, i] = getnumber(arr, i + 1);
                                    }
                                    else if (arr[i] == 0x20){
                                        roomres.kuitan = arr[i + 1] == 1;
                                        i += 2;
                                    }
                                    else if (arr[i] == 0x28){
                                        [roomres.startpoint, i] = getnumber(arr, i + 1);
                                    }
                                    else if (arr[i] == 0x30){
                                        [roomres.minendpoint, i] = getnumber(arr, i + 1);
                                    }
                                    else if (arr[i] == 0xa8 && arr[i + 1] == 0x02){
                                        roomres.hint = arr[i + 2] == 1;
                                        i += 3;
                                    }
                                    else if (arr[i] == 0xb0 && arr[i + 1] == 0x02){
                                        roomres.comdifficulty = arr[i + 2];
                                        i += 3;
                                    }
                                    else if (arr[i] == 0xb8 && arr[i + 1] == 0x02){
                                        roomres.tsumoson = arr[i + 2] == 1;
                                        i += 3;
                                    }
                                    else{
                                        let cmd;
                                        [cmd, i] = getnumber(arr, i);
                                        let addlen = cmd % 8 == 2;
                                        unknownnumber += ' ' + cmd;
                                        [cmd, i] = getnumber(arr, i);
                                        if (addlen)
                                            i += cmd;
                                    }
                                }
                            }
                            else if (arr[i] == 0x3a){
                                roomres.x3a = arr[i + 1];
                                i += 2;
                            }
                            else{
                                let cmd;
                                [cmd, i] = getnumber(arr, i);
                                let addlen = cmd % 8 == 2;
                                unknownnumber += ' ' + cmd;
                                [cmd, i] = getnumber(arr, i);
                                if (addlen)
                                    i += cmd;
                            }
                        }
                    }
                    else if (arr[i] == 0x1a){
                        let len;
                        [len, i] = getnumber(arr, i + 1);
                        [len, i] = getnumber(arr, i + 1);
                        roomres['1a'] = len;
                    }
                    else{
                        let cmd;
                        [cmd, i] = getnumber(arr, i);
                        let addlen = cmd % 8 == 2;
                        unknownnumber += ' ' + cmd;
                        [cmd, i] = getnumber(arr, i);
                        if (addlen)
                            i += cmd;
                    }
                }
                let room = {
                    room: undefined,
                    player: roomres.player,
                    aka: roomres.aka,
                    timeone: roomres.timeone,
                    timeshare: roomres.timeshare,
                    startpoint: roomres.startpoint,
                    minendpoint: roomres.minendpoint,
                    kuitan: roomres.kuitan,
                    round: roomres.round,
                    tsumoson: roomres.tsumoson,
                    extra: undefined
                };
                if (roomres.type == 'friend'){
                    room.room = 0;
                    room.extra = {
                        hint: roomres.hint,
                        comdifficulty: roomres.comdifficulty,
                        roomnumber: roomres['1a'],
                        x20: roomres.x20,
                        x3a: roomres.x3a
                    };
                }
                else if (roomres.type == 'match'){
                    room.room = parseInt((roomres['1a'] - 1) / 3) + 1;
                    room.extra = {
                        hint: roomres['1a'] < 13,
                    };
                }
                else if (roomres.type == 'contest' || roomres.type.slice(0, 7) == 'unknown'){
                    data.extra.id = undefined;
                }
                data.roomdata = room;
            }
            else if (arr[i] == 0x5a){
                //玩家信息
                let player = {rank: 0, pt: 0}, pid, temp, len;
                [len, i] = getnumber(arr, i + 1);
                let j = i;
                for (; j < i + len; ){
                    //console.log(player, j.toString(16));
                    if (arr[j] == 0x08){
                        [temp, j] = getnumber(arr, j + 1);
                        player.id = temp;
                    }
                    else if (arr[j] == 0x10){
                        pid = arr[j + 1];
                        j += 2;
                    }
                    else if (arr[j] == 0x1a){
                        [temp, j] = getnumber(arr, j + 1);
                        [player.name, j] = decodeUTF8(arr, j, temp);
                    }
                    else if (arr[j] == 0x20){
                        [temp, j] = getnumber(arr, j + 1);
                    }
                    else if (arr[j] == 0x2a){
                        [temp, j] = getnumber(arr, j + 1);
                        j += temp;
                    }
                    else if (arr[j] == 0x30){
                        [temp, j] = getnumber(arr, j + 1);
                        j += temp;
                    }
                    else if (arr[j] == 0x3a){
                        [temp, j] = getnumber(arr, j + 1);
                        j += temp;
                    }
                }
                data.playerdata[pid] = player;
                i += len;
            }
            else if (arr[i] == 0x62){
                let pid, temp, len;
                [len, i] = getnumber(arr, i + 1);
                let j = i;
                for (; j < i + len; ){
                    if (arr[j] == 0x0a){
                        let score = {}, slen;
                        [slen, j] = getnumber(arr, j + 1);
                        let unknownnumber = '';
                        for (let k = j; k < slen + j; ){
                            //console.log(score, pid, k.toString(16));
                            if (arr[k] == 0x08){
                                pid = arr[k + 1];
                                k += 2;
                            }
                            else if (arr[k] == 0x10){
                                [temp, k] = getnumber(arr, k + 1);
                                score.delta = temp;
                            }
                            else if (arr[k] == 0x18){
                                [temp, k] = getnumber(arr, k + 1);
                                score.final = temp;
                            }
                            else if (arr[k] == 0x20){
                                k += 2;
                            }
                            else if (arr[k] == 0x28){
                                [temp, k] = getnumber(arr, k + 1);
                                score.pt = temp;
                            }
                            else if (arr[k] == 0x30){
                                [temp, k] = getnumber(arr, k + 1);
                            }
                            else{
                                let cmd;
                                [cmd, k] = getnumber(arr, k);
                                let addlen = cmd % 8 == 2;
                                unknownnumber += ' ' + cmd;
                                [cmd, k] = getnumber(arr, k);
                                if (addlen)
                                    k += cmd;
                            }
                        }
                        data.playerdata[pid].finalpoint = score.final;
                        data.playerdata[pid].deltapt = score.pt;
                        j += slen;
                    }
                }
                i += len;
            }
            else{
                let cmd;
                [cmd, i] = getnumber(arr, i);
                let addlen = cmd % 8 == 2;
                unknownnumber += ' ' + cmd;
                [cmd, i] = getnumber(arr, i);
                if (addlen)
                    i += cmd;
            }
        }
        //var pp = path.join(__dirname, '../../data/gamedata.txt');
        //if (valid && data.extra.id != undefined) fs.appendFileSync(pp, JSON.stringify(data) + '\n');
        if (data.extra.id != undefined && data.accountid >= 0) paipugamedata[data.extra.id] = data;
        ss += paipulength;
    }
}

function analyze(sender, data){
    var buf = Buffer.from(data);
    var arr8 = new Uint8Array(buf);
    if (sender == 'client'){
        if (arr8[0] == 0x02 && arr8[3] == 0x0a){
            let number = arr8[1] + arr8[2] * 256;
            let len, start = 4, str = '';
            [len, start] = getnumber(arr8, start);
            [str, start] = getstring(arr8, start, len);
            clientrequest[number] = str;
        }
        return;
    }
    if (arr8[0] == 0x03){
        let number = arr8[1] + arr8[2] * 256;
        let req = clientrequest[number];
        if (req == undefined){
            reporterror({
                position: 'analyze',
                message: 'request not find',
                data: {
                    servernumber: number,
                    clientrequest
                }
            });
            return;
        }
        if (req == '.lq.Lobby.oauth2Login'){
            analyzelogin(sender, arr8, 5);
        }
        else if (req == '.lq.Lobby.login'){
            analyzelogin(sender, arr8, 5);
        }
        else if (req == '.lq.Lobby.fetchGameRecordList'){
            try{
                analyzepaipu(arr8, 5);
            }
            catch (err){
                let arr = '';
                for (let i = 0; i < arr8.length; i ++ ){
                    let str = arr8[i].toString(16);
                    if (str.length == 1) str = '0' + str;
                    arr += str;
                }
                let data = {
                    position: 'electron-analyze-analyzepaipu',
                    message: '牌谱获取错误',
                    data: {
                        UserID,
                        arr
                    }
                };
                reporterror(data);
            }
        }
        return;
    }
}

function getUserID(){
    return UserID;
}

module.exports = {
    analyze,
    paipugamedata,
    getUserID,
    reporterror
};