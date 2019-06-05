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

var InjectOverNode = document.createElement('p');
InjectOverNode.innerHTML = '脚本加载完成，点击隐藏';
InjectOverNode.setAttribute('style', 'color: #2D2;z-index: 999;position: absolute;left: 0px;top: 0px;font-weight: bold;');
InjectOverNode.setAttribute('id', 'InjectOverNode');
InjectOverNode.onclick = function () {
    this.setAttribute('style', 'visibility: collapse');
};
document.getElementsByTagName('body')[0].appendChild(InjectOverNode);

ipcr.send('bwindowinjectfinish');