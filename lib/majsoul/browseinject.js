console.log("try add jquery trigger");
var electron = require("electron");
var ipcr = electron.ipcRenderer;
function addjquery() {
    //console.log("check jquery");
    var flag = 1;
    if (typeof (jQuery) == "undefined") {
        flag = 0;
        console.log("try add jquery"); //%jqueryjs%
    }
    if (typeof (wsHook) == "undefined") {
        flag = 0;
        console.log("try add wsHook"); //%wshookjs%
        wsHook.before = function (data, url) {
            ipcr.send('wshook', 'client', Buffer.from(data));
        }
        wsHook.after = function (messageEvent, url, wsObject) {
            ipcr.send('wshook', 'server', Buffer.from(messageEvent.data));
            return messageEvent;
        }
    }
    addjqueryintervalremain -= flag;
    if (addjqueryintervalremain <= 0) {
        clearInterval(addjqueryintervalid);
        console.log('stop add-jquery interval');
    }
}
if (global.addjqueryintervalid == undefined){
    global.addjqueryintervalid = setInterval(addjquery, 1000);
    global.addjqueryintervalremain = 30;
    addjquery();
}
function getAccountID(){
    if (GameMgr == undefined || GameMgr.Inst == undefined || GameMgr.Inst.account_id == -1)
        setTimeout(getAccountID, 1000);
    else ipcr.send('userid', GameMgr.Inst.account_id);
}
setTimeout(getAccountID, 1000);
ipcr.on('collectpaipu', (event, operation) => {
    if (GameMgr == undefined || GameMgr.Inst == undefined || GameMgr.Inst.account_id == -1){
        ipcr.send('collectpaipucallback', { type: 'UserID' });
        return;
    }
    if (uiscript == undefined || uiscript.UI_PaiPu == undefined){
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