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
    collectallpaipu(null, function () {
        let res = [];
        for (let i in uiscript.UI_PaiPu.record_map)
            res.push(uiscript.UI_PaiPu.record_map[i]);
        ipcr.send('collectpaipucallback', undefined, operation, res);
    });
});

ipcr.on('fetchpaipudata', (event, uuid) => {
    //console.log('fetching paipu, uuid=' + uuid);
    app.NetAgent.sendReq2Lobby("Lobby","fetchGameRecord", {game_uuid: uuid}, function (t, a) {
        ipcr.send('fetchpaipudatacallback', a.error, a.data, a.data_url);
        //console.log('fetch result:', a.data, a.data_url);
    });
});

function collectallpaipu(event, callback) {
    const get_more = function (result,start,type="ALL",count=30) {
        app.NetAgent.sendReq2Lobby("Lobby", "fetchGameRecordList", {
                    start,
                    count,
                    type
                },
                function(n, {record_list}) {
                    console.log(start,record_list);
                    if(record_list.length>0){
                        get_more(result.concat(record_list),start+count)
                    }else{
                        m = uiscript.UI_PaiPu.record_map;
                        result.forEach((item)=>{
                            m[item.uuid] = item
                        });
                        let count = 0;
                        for (let i in uiscript.UI_PaiPu.record_map)
                            count ++ ;
                        alert("已自动收集牌谱！牌谱个数：" + count);
                        callback && callback();
                    }
                });
    }
    let count = 0;
    for (let i in uiscript.UI_PaiPu.record_map)
        count ++ ;
    get_more([],count);
}

ipcr.on('collectallpaipu', collectallpaipu);

var InjectOverNode = document.createElement('p');
InjectOverNode.innerHTML = '脚本加载完成，点击隐藏';
InjectOverNode.setAttribute('style', 'color: #2D2;z-index: 999;position: absolute;left: 0px;top: 0px;font-weight: bold;');
InjectOverNode.setAttribute('id', 'InjectOverNode');
InjectOverNode.onclick = function () {
    this.setAttribute('style', 'visibility: collapse');
};
document.getElementsByTagName('body')[0].appendChild(InjectOverNode);

ipcr.send('bwindowinjectfinish');