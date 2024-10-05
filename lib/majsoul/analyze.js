"use strict";

var reporterror;

/* liqiJS start */

if (!protobuf){
    // in web, will include in HTML; otherwise in node, use require.
    var protobuf = require("protobufjs");
}

//cfg.desktop.matchmode
let modeid2room = {
    0:'',
    1:'铜之间', 2:'铜之间', 3:'铜之间', 17:'铜之间', 18:'铜之间', // dou
    4:'银之间', 5:'银之间', 6:'银之间', 19:'银之间', 20:'银之间', // gin
    7:'金之间', 8:'金之间', 9:'金之间', 21:'金之间', 22:'金之间', // kin
    10:'玉之间', 11:'玉之间', 12:'玉之间', 23:'玉之间', 24:'玉之间', // gyoku 
    //13:'乱斗之间', 14:'乱斗之间', // furu
    15:'王座间', 16:'王座间', 25:'王座间', 26:'王座间', // ouza
    // 33:'宝牌狂热', // kyounetu
    // 40:'修罗之战', // syura
    // 41:'赤羽之战', // chuanma
};

const roomname2num = {
    '友人场': 0,
    '铜之间': 1,
    '银之间': 2,
    '金之间': 3,
    '玉之间': 4,
    '王座间': 5,
    '比赛场': 100,
    '乱斗之间': 101,
    '修罗之战': 102,
    '宝牌狂热': 103,
    '赤羽之战': 104,
}

var ProtobufRoot, ProtobufWrapper, ProtobufRequest;
function AnalyzeInit(liqijson, roomid = null){
    ProtobufRoot = protobuf.Root.fromJSON(liqijson);
    ProtobufRoot.resolveAll();
    console.log('protobuf data load complete');
    ProtobufWrapper = ProtobufRoot.lookupType('.lq.Wrapper');
    ProtobufRequest = {};
    if (roomid){
        for (let i in modeid2room)
            delete modeid2room[i];
        for (let i in roomid)
            modeid2room[roomid[i].id] = roomid[i].room_name_chs;
    }
    // console.log(ProtobufWrapper, ProtobufRequest);
}

function Protobuf2Object(data){
    if (data.constructor.name != "Uint8Array" && !Array.isArray(data) && protobuf.util.isNode)
        //不是array或者Uint8Array且在Node环境，尝试用Buffer封装一下进行转换
        data = Buffer.from(data);
    let arr8 = new Uint8Array(data);
    if (arr8.length == 0){
        console.log('Protobuf2Object: zero length, ' + arr8);
        return undefined;
    }
    if (arr8[0] <= 0x03){
        //带有数据类型（server推送；client请求；server响应）的内容
        //由于没有id为0的内容所以应该没问题。同时会直接把这是什么类型给丢掉
        if (arr8[0] == 0x01){
            //server message
            return ProtobufDecode(arr8.slice(1));
        }
        else if (arr8[0] == 0x02){
            //client request
            let id = arr8[2] * 256 + arr8[1];
            let data = ProtobufDecode(arr8.slice(3), false);
            let method = ProtobufRoot.lookup(data.name);
            let request = method.resolvedRequestType;
            let response = method.resolvedResponseType;
            data = ProtobufDecode(data.data, true, request);
            ProtobufRequest[id] = [response, data];
            return data;
        }
        else if (arr8[0] == 0x03){
            //server response
            let id = arr8[2] * 256 + arr8[1];
            let req = ProtobufRequest[id];
            if (req == undefined){
                console.log('Protobuf2Object: request not find, ' + id);
                return undefined;
            }
            let data = ProtobufDecode(arr8.slice(3), false);
            data = ProtobufDecode(data.data, true, req[0]);
            return data;
        }
        else{
            console.log('Protobuf2Object: start error, ' + arr8);
            return undefined;
        }
    }
    else return ProtobufDecode(arr8);
}

//给一个protobuf数组转换成object，默认用Wrapper解析。同时解析完成后会递归将bytes解析成对应object
function ProtobufDecode(arr8, recursive = true, ProtobufType = ProtobufWrapper){
    //console.log(arr8.toString(), '\n', ProtobufType.name);
    let data = ProtobufType.decode(arr8);
    if (recursive){
        if (ProtobufType.name == 'Wrapper' || ProtobufType.name == 'ActionPrototype'){
            //如果是Wrapper或者ActionPrototype，根据name解析data并移除该层
            let isWrapper = ProtobufType.name == 'Wrapper';
            ProtobufType = ProtobufRoot.lookupType(data.name);
            if (ProtobufType != undefined && ProtobufType.name != ''){
                data.data = ProtobufDecode(data.data, recursive, ProtobufType);
                data = data.data;
            }
        }
        //分析是否有bytes项
        for (let key in ProtobufType.fields)
            if (ProtobufType.fields[key].type == 'bytes'){
                if (data[key] == undefined)
                    continue;
                if (Array.isArray(data[key])){
                    for (let i in data[key]){
                        if (data[key][i].constructor.name == 'Uint8Array')
                            data[key][i] = ProtobufDecode(data[key][i]);
                    }
                }
                else{
                    if (data[key].constructor.name == 'Uint8Array')
                        data[key] = ProtobufDecode(data[key]);
                }
            }
            else if (ProtobufType.fields[key].type == 'GameLiveUnit'){
                for (let i in data[key]){
                    if (data[key][i].action_data.constructor.name == 'Uint8Array')
                        data[key][i].action_data = ProtobufDecode(data[key][i].action_data);
                }
            }
            else if (ProtobufType.fields[key].type == 'GameRestore' && data[key] != undefined){
                for (let i in data[key].actions){
                    if (data[key].actions[i].data.constructor.name == 'Uint8Array')
                        data[key].actions[i] = ProtobufDecode(data[key].actions[i].data, recursive, ProtobufRoot.lookupType(data[key].actions[i].name));
                }
            }
            else if (ProtobufType.fields[key].type == 'GameAction'){
                console.log(data[key].length);
                for (let i in data[key]){
                    if (data[key][i].result.constructor.name == 'String' || data[key][i].result.constructor.name == 'Uint8Array'){
                        let buffer = new Buffer(data[key][i].result, 'base64');
                        data[key][i].result = ProtobufDecode(new Uint8Array(buffer));
                    }
                    else console.log(data[key][i].result.constructor.name, JSON.stringify(data[key][i].result));
                }
            }
    }
    return data;
}

function ProtobufObject2Object(obj){
    let res = {};
    if (Array.isArray(obj)){
        res = [];
        for (let i in obj)
            res.push(ProtobufObject2Object(obj[i]));
        return res;
    }
    let objtype = typeof obj;
    if (objtype == 'number' || objtype == 'boolean' || objtype == 'string')
        return obj;
    let name = obj.constructor.name;
    if (name == 'Object')
        return obj;
    return obj.toJSON();
    let PT = ProtobufRoot.lookupType(name);
    for (let key in PT.fields){
        //console.log(key, obj[key]);
        if (obj[key] != undefined)
            res[key] = ProtobufObject2Object(obj[key]);
    }
    return res;
}

/* liqiJS end */

if (protobuf.util.isNode){

    const request = require('request');
    //SimpleMahjong启动位置会不对，尝试两种require
    var config;
    try{
        config = require('../config.js').config;
    }
    catch{
        config = require('../lib/config.js').config;
    }
    const { dialog } = require('electron');

    reporterror = function(data){
        data.version = config.get('Version');
        data.time = (new Date()).toString();
        let errconf = config.get('SendError');
        let always = false, send = true;
        if (errconf != undefined)
            [send, always] = errconf;
        
        function sendmain(data){
            request({
                url: `https://t.zyr17.cn/MajsoulPaipuAnalyzer/ErrorCollect`, 
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
                cancelId: 1,
                buttons: ['上传', '不上传']
            }).then((response) => {
                let checked = response.checkboxChecked;
                response = response.response;
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

}

var UserID = -1;
var paipugamedata = {};
var paipugamedata0 = {};

function analyzeGameRecord(record_list){
    // collect errors and report once
    let target_gamedata = null;
    if (UserID == 0) target_gamedata = paipugamedata0;
    else target_gamedata = paipugamedata;
    let errors = [];
    function reporterrors(data){
        errors.push(data);
    }
    for (let i = 0; i < record_list.length; i ++ ){
        let record = record_list[i];
        // TODO: 对于超过1000个的，当前获取不到元数据的牌谱，需要更新。可以升级元数据牌谱版本号然后对其更新
        //if (target_gamedata[record.uuid] != undefined)
            //已有该数据，不继续分析。以后可以考虑数据的生成版本
            //continue;
        target_gamedata[record.uuid] = null;
        //console.log(record.uuid);
        let gamedata = {
            source: 'majsoul',
            accountid: UserID,
            starttime: record.start_time,
            endtime: record.end_time,
            uuid: record.uuid,
            version: null,
            playerdata: [],
            roomdata: {}
        };
        //对于旧式数据的载入
        if (record.config.mode.extendinfo != undefined && record.config.mode.extendinfo.length > 0){
            gamedata.roomdata = JSON.parse(record.config.mode.extendinfo);
        }
        if (record.config.mode.detail_rule != undefined){
            //console.log(record.constructor.name, JSON.stringify(record.config.mode.detail_rule), record.config.mode.detail_rule);
            gamedata.roomdata = record.constructor.name == 'Object' ? record.config.mode.detail_rule : record.config.mode.detail_rule.toJSON();
        }
        //加上version项，记录生成牌谱的版本以方便以后更新。如果是在浏览器生成则设置为0.0.0.17
        let version = '0.0.0.17';
        if (protobuf.util.isNode)
            version = config.get('Version');
        gamedata.version = version;
        gamedata.roomdata.has_ai = record.config.mode.ai;
        //默认的必要设置
        if (gamedata.roomdata.init_point == undefined) gamedata.roomdata.init_point = 25000;
        if (gamedata.roomdata.fandian == undefined) gamedata.roomdata.fandian = 30000;
        if (gamedata.roomdata.time_fixed == undefined) gamedata.roomdata.time_fixed = 5;
        if (gamedata.roomdata.time_add == undefined) gamedata.roomdata.time_add = 20;
        if (gamedata.roomdata.dora_count == undefined) gamedata.roomdata.dora_count = 3;
        if (gamedata.roomdata.shiduan == undefined) gamedata.roomdata.shiduan = 1;
        //把食断改为布尔值
        gamedata.roomdata.shiduan = gamedata.roomdata.shiduan == 1;
        if (record.config.category == 1){
            //友人场
            gamedata.roomdata.room = roomname2num['友人场'];
        }
        else if (record.config.category == 2){
            //段位场
            let roomname = modeid2room[record.config.meta.mode_id];
            if (roomname) gamedata.roomdata.room = roomname2num[roomname];
            // console.log(record.config.meta.mode_id, gamedata.roomdata.room);
            // if (!gamedata.roomdata.room) console.log(record.config.meta.mode_id, roomname);
            if (!roomname){
                reporterrors({ message: '未知房间编号' + record.config.meta.mode_id, data: record});
            }
            if (gamedata.roomdata.room == 101){
                //古役场
            }
            else if (gamedata.roomdata.room == 102){
                //修罗模式
            }
            else if (gamedata.roomdata.room == 103){
                //宝牌狂热
            }
            else if (gamedata.roomdata.room == 104){
                //川麻
            }
            else if (roomname && !gamedata.roomdata.room){
                //查到了房间名字，但是不认识
            }
        }
        else if (record.config.category == 4){
            //比赛场
            gamedata.roomdata.room = roomname2num['比赛场'];
            gamedata.roomdata.contest_id = record.config.meta.contest_uid;
        }
        else{
            reporterrors({ message: '未知牌谱类别编号' + record.config.category, data: record});
        }
        gamedata.roomdata.player = record.config.mode.mode > 10 ? 3 : 4;
        let roundtype = record.config.mode.mode % 10;
        gamedata.roomdata.round = roundtype == 1 ? 4 : roundtype == 2 ? 8 : 1;
        if (record.config.mode.mode >= 20 || roundtype > 4){
            reporterrors({message: '未知房间模式编号' + record.config.mode.mode, data: record});
        }
        for (let i = 0; i < gamedata.roomdata.player; i ++ )
            gamedata.playerdata.push({
                id: 0,
                name: '电脑',
                rank: 1,
                pt: 0
            });
        if (gamedata.roomdata.player != gamedata.playerdata.length || 
            gamedata.roomdata.player != record.result.players.length){
                reporterrors({message: '玩家数目相关条目不统一', data: data});
        }
        for (let i = 0; i < record.accounts.length; i ++ ){
            let acc = record.accounts[i];
            let pos = acc.seat || 0;
            let pdata = gamedata.playerdata[pos];
            pdata.id = acc.account_id;
            pdata.name = acc.nickname;
            let idstr = String(acc.level.id);
            pdata.rank = parseInt(idstr[2]) * 3 + parseInt(idstr[4]) - 3;
            pdata.pt = acc.level.score;
        }
        for (let i = 0; i < gamedata.roomdata.player; i ++ ){
            let point = record.result.players[i];
            let pos = point.seat || 0;
            let pdata = gamedata.playerdata[pos];
            pdata.finalpoint = point.part_point_1;
            pdata.deltapt = point.grading_score;
        }

        target_gamedata[gamedata.uuid] = gamedata;
        //console.log(JSON.stringify(gamedata));
    }
    if (errors.length) reporterror({message: '牌谱元数据分析错误', data: errors});
}

 //输入：数据 输出：解析结果
function analyze(data){
    let res = Protobuf2Object(data);
    return res;
}

var InitialOya = -1; // For records, seat 0 is always oya. For actions, seat is based on player and should count oya by ResAuthGame

function rotatearr(arr, delta){
    console.assert(arr.length == 4, 'in rotatearr, array length not 4');
    let res = [undefined, undefined, undefined, undefined];
    for (let i = 0; i < 4; i ++ )
        res[(i + delta) % 4] = arr[i];
    return res;
}

function majsouldataconvert(srcdata, action){

    function getoneoperation(list){
        if (list == undefined || !Array.isArray(list.operation_list)) return {};
        let res = {};
        for (let i in list.operation_list){
            let opt = list.operation_list[i];
            let button = majsouloption2button[opt.type];
            if (button == 'none' || button == 'dapai' || button == undefined)
                continue;
            let choice = [];
            for (let j in opt.combination)
                choice.push(opt.combination[j].split('|'));
            res[button] = choice;
        }
        return res;
    }

    function getoperation(data){
        let oprs = undefined;
        if (data.operation != undefined)
            oprs = [data.operation];
        else if (Array.isArray(data.operations) && data.operations.length > 0)
            oprs = data.operations;
        let res = [{}, {}, {}, {}];
        for (let i in oprs){
            let who = (oprs[i].seat + InitialOya) % 4;
            res[who] = getoneoperation(oprs[i]);
        }
        //if (JSON.stringify(res) != '[{},{},{},{}]') console.log(res);
        return res;
    }

    let data = { operation: getoperation(srcdata) };
    if (action == 'NewRound'){
        data = {};
        data.nowround = srcdata.chang * 4 + srcdata.ju;
        data.dice = -1;
        data.score = rotatearr(srcdata.scores, InitialOya);
        data.east = (data.nowround + InitialOya) % 4;
        data.yama = '';
        if (srcdata.tiles != undefined)
            data.hand = srcdata.tiles.join('');
        else{
            data.allhand = [
                srcdata.tiles0.join(''),
                srcdata.tiles1.join(''),
                srcdata.tiles2.join(''),
                srcdata.tiles3.join('')
            ];
            for (let i = 0; i < 4; i ++ )
                data.yama += data.allhand[(i + data.east) % 4].slice(0, 26);
            for (let i = 0; i < 4; i ++ )
                data.yama += data.allhand[i].slice(26);
            if (srcdata.paishan != undefined)
                data.yama += srcdata.paishan;
        }
        data.dora = srcdata.dora;
        if (srcdata.doras.length) data.dora = srcdata.doras.join('');
        data.honba = srcdata.ben;
        data.kyoutaku = srcdata.liqibang;
        data.remaintile = srcdata.left_tile_count;
        if (data.remaintile == 0 || data.remaintile == undefined)
            data.remaintile = 69;
    }
    else if (action == 'DiscardTile'){
        data.who = (srcdata.seat + InitialOya) % 4;
        data.tile = srcdata.tile;
        data.tsumokiri = srcdata.moqie;
        data.reach = srcdata.is_wliqi ? 2 : srcdata.is_liqi ? 1 : 0;
        data.dora = srcdata.doras.join('');
        //tingpais zhenting
    }
    else if (action == 'DealTile'){
        data.who = (srcdata.seat + InitialOya) % 4;
        data.tile = srcdata.tile;
        if (data.tile == undefined) data.tile = '?x'; //不知道摸了啥，用?x 
        data.dora = srcdata.doras.join('');
        data.remaintile = srcdata.left_tile_count;
        //TODO: 如果有人需要摆棒子会有这一行。可以考虑利用
        data.reachput = srcdata.liqi == undefined ? undefined : (srcdata.liqi.seat + InitialOya) % 4;
    }
    else if (action == 'ChiPengGang'){
        data.who = (srcdata.seat + InitialOya) % 4;
        data.tiles = srcdata.tiles.join('');
        data.belong = [];
        for (let i in srcdata.froms)
            data.belong.push((srcdata.froms[i] + InitialOya) % 4);
        data.reachput = srcdata.liqi == undefined ? undefined : (srcdata.liqi.seat + InitialOya) % 4;
    }
    else if (action == 'AnGangAddGang'){
        data.who = (srcdata.seat + InitialOya) % 4;
        data.tiles = srcdata.tiles;
        data.naki = srcdata.type == 3 ? 'ankan' : 'kakan';
        //有直翻宝牌的设定？可能会用到？
        data.dora = srcdata.doras.join('');
        //Record相比Action还少了zhenting
    }
    else if (action == 'LiuJu'){
        data.who = (srcdata.seat + InitialOya) % 4;
        data.tiles = srcdata.tiles.join('');
        data.typenum = srcdata.type;
    }
    else if (action == 'Hule'){
        data.oldscore = rotatearr(srcdata.old_scores, InitialOya);
        data.deltascore = rotatearr(srcdata.delta_scores, InitialOya);
        data.finalscore = rotatearr(srcdata.scores, InitialOya);
        if (srcdata.doras)
            data.dora = srcdata.doras.join('');
        data.agari = [];
        for (let i in srcdata.hules){
            let hule = srcdata.hules[i];
            let res = { han: [], naki: [], ankan: [] };
            //役种处理
            for (let j in hule.fans){
                let fanshu = hule.fans[j].val;
                if (hule.fans[j].name == undefined || hule.fans[j].name.length == 0)
                    hule.fans[j].name = majsoulfanid2name[hule.fans[j].id]; //新格式，找到役名
                let name = hule.fans[j].name
                if (
                    name == '四暗刻单骑' || 
                    name == '国士无双十三面' || 
                    name == '大四喜' || 
                    name == '纯正九莲宝灯'
                ) fanshu = 2;
                if (hule.yiman) fanshu *= 13;
                res.han.push([name, fanshu]);
            }
            res.who = (hule.seat + InitialOya) % 4;
            res.hai = hule.hand.join('');
            //鸣牌处理
            for (let j in hule.ming){
                let str = hule.ming[j];
                let tiles = /\((.*?)\)/.exec(str);
                let oldtype = tiles == null;
                if (oldtype) tiles = str;
                else tiles = tiles[1];
                tiles = tiles.replace(/[,|]/g, '');
                if ((oldtype && tiles.length == 2) || str.slice(0, 6) == 'angang'){
                    //暗杠
                    if (tiles.length == 2)
                        tiles = tiles + tiles + tiles + tiles;
                    res.ankan.push(tiles);
                }
                else{
                    res.naki.push(tiles);
                }
            }
            res.get = hule.hu_tile;
            res.dora = hule.doras.join('');
            res.ura = hule.li_doras.join('');
            res.fu = hule.fu;
            res.tsumo = hule.zimo;
            //TODO: 包牌
            res.bao = -1;
            //未使用项目 count=fan, liqi, qinjia, point_rong, point_zimo_qin, point_zimo_xian
            data.agari.push(res);
        }
    }
    else if (action == 'NoTile'){
        data = {
            player: [],
            mangan: [false, false, false, false],
            deltascore: [0, 0, 0, 0],
            finalscore: []
        };
        data.oldscore = srcdata.scores[0].old_scores.slice();
        for (let i in srcdata.players)
            data.player.push([srcdata.players[i].tingpai, srcdata.players[i].hand.join('')]);
        for (let i in srcdata.scores){
            for (let j in srcdata.scores[i].delta_scores)
                data.deltascore[j] += srcdata.scores[i].delta_scores[j];
            if (srcdata.liujumanguan) data.mangan[(srcdata.scores[i].seat + InitialOya) % 4] = true;
        }
        data.oldscore = rotatearr(data.oldscore, InitialOya);
        data.deltascore = rotatearr(data.deltascore, InitialOya);
        for (let i in data.oldscore)
            data.finalscore.push(data.oldscore[i] + data.deltascore[i]);
        //TODO: 验证多个流满的情况是否正确
    }
    else if (action == 'ResAuthGame'){
        //通过座位列表确认；需要知道自己ID
        for (let i in srcdata.seat_list)
            if (srcdata.seat_list[i] == UserID){
                InitialOya = (4 - i) % 4;
                break;
            }
        //console.log(srcdata, UserID, InitialOya);
        data.init_point = srcdata.game_config.category == 2 ? 25000 : srcdata.game_config.mode.detail_rule.init_point;
    }
    else{
        if (action != 'MJStart')
            console.error('majsouldataconvert: unknown action ' + action);
        return undefined;
    }
    //console.log(action, data, srcdata);
    return data;
}

if (protobuf.util.isNode){

    function getUserID(){
        return UserID;
    }

    function setUserID(id){
        UserID = id;
    }

    if (module != undefined)
        module.exports = {
            analyze,
            paipugamedata,
            paipugamedata0,
            analyzeGameRecord,
            getUserID,
            setUserID,
            AnalyzeInit,
            Protobuf2Object,
            reporterror
        };

}