console.log("try add jquery trigger");
const electron = require("electron");
const ipcr = electron.ipcRenderer;
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
addjqueryintervalid = setInterval(addjquery, 1000);
addjqueryintervalremain = 30;