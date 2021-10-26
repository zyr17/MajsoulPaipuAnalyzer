var electron = require("electron");
var ipcr = electron.ipcRenderer;
function getAccountID(){
    if (global.GameMgr == undefined || GameMgr.Inst == undefined || GameMgr.Inst.account_id == -1)
        setTimeout(getAccountID, 1000);
    else ipcr.send('userid', GameMgr.Inst.account_id);
}
setTimeout(getAccountID, 1000);
ipcr.on('collectpaipu', (event, operation) => {
    if (global.GameMgr == undefined || GameMgr.Inst == undefined || GameMgr.Inst.account_id == -1){
        ipcr.send('collectpaipucallback', { type: 'UserID' });
        return;
    }
    if (global.uiscript == undefined || uiscript.UI_PaiPu == undefined){
        ipcr.send('collectpaipucallback', { type: 'uisciprt' });
        return;
    }
    let res = [];
    for (let i in uiscript.UI_PaiPu.record_map)
        res.push(uiscript.UI_PaiPu.record_map[i]);
    ipcr.send('collectpaipucallback', undefined, operation, res);
});

ipcr.on('fetchpaipudata', (event, uuid, callback = 'fetchpaipudatacallback') => {
    console.log('fetching paipu, uuid=' + uuid);
    app.NetAgent.sendReq2Lobby("Lobby","fetchGameRecord", {game_uuid: uuid, client_version_string: GameMgr.prototype.getClientVersion()}, function (t, a) {
        // console.log('fetch result:', a);
        ipcr.send(callback, uuid, a);
    });
});

setTimeout(() => {
    console.log('fetchliqijson');
    let liqiintervalid = setInterval( function () {
        let res = Laya.loader.getRes("res/proto/liqi.json");
        // console.log('liqi', res);
        if (res) {
            ipcr.send('fetchliqijsoncallback', res, cfg.desktop.matchmode.map_);
            clearInterval(liqiintervalid);
        }
    }, 1000);
}, 1000);

function collectallpaipu(event, callback) {
    alert("将开始自动收集牌谱基本数据！如果牌谱较多需要一定时间。1000牌谱收集耗时约1分钟。");
    let TYPES = {0: 'ALL', 1: 'FRIEND', 2: 'RANK', 4: 'MATCH', 100: 'COLLECT'};
    let types = []
    for (let i in TYPES) types.push(i);
    const get_more = function (result,start,types=[0],count=30) {
        let type = types[0];
        app.NetAgent.sendReq2Lobby("Lobby", "fetchGameRecordList", {
                    start,
                    count,
                    type
                },
                function(n, {record_list}) {
                    console.log(start,record_list, types);
                    m = uiscript.UI_PaiPu.record_map;
                    if(record_list.length>0 && !m[record_list[0].uuid]){
                        get_more(result.concat(record_list),start+count,types)
                    }else{
                        types = types.slice(1);
                        console.log(types);
                        if (types.length) get_more(result.concat(record_list),0,types);
                        else{
                            result.forEach((item)=>{
                                m[item.uuid] = item
                            });
                            let count = 0;
                            for (let i in uiscript.UI_PaiPu.record_map)
                                count ++ ;
                            alert("已自动收集牌谱基本数据！牌谱个数：" + count);
                            callback && callback();
                        }
                    }
                });
    }
    get_more([], 0, types);
}

ipcr.on('collectallpaipu', (event, callback) => {
    collectallpaipu(event, () => {
        ipcr.send(callback);
    })
});

ipcr.on('collectmetadata', (event, uuid_list) => {
    console.log('fetching paipu metadata ' + uuid_list.length);
    app.NetAgent.sendReq2Lobby("Lobby","fetchGameRecordsDetail", {uuid_list}, function (t, a) {
        console.log('fetch result:', a);
        ipcr.send('collectmetadatacallback', a.record_list);
    });
});


var InjectOverNode = document.createElement('p');
InjectOverNode.innerHTML = '脚本加载完成，点击隐藏';
InjectOverNode.setAttribute('style', 'color: #2D2;z-index: 999;position: absolute;left: 0px;top: 0px;font-weight: bold;');
InjectOverNode.setAttribute('id', 'InjectOverNode');
InjectOverNode.onclick = function () {
    this.setAttribute('style', 'visibility: collapse');
};
document.getElementsByTagName('body')[0].appendChild(InjectOverNode);

ipcr.send('bwindowinjectfinish');