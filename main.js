"use strict";

try {
    require.resolve('electron');
} catch (e) {
    console.error(`Electron is not installed. Make sure 'npm install' has been successfully run.`);
    process.exit(1);
}

const { app, BrowserWindow, dialog, Menu, ipcMain } = require('electron');

const path                           = require('path').join,
      fs                             = require('fs'),
      url                            = require('url'),
      { config, checknewestversion } = require('./lib/config.js');

const { analyze, paipugamedata, analyzeGameRecord, getUserID, setUserID, reporterror } = require('./lib/majsoul/analyze');

var appPath = app.getAppPath();
app.setPath('userData', appPath + '/UserData');
let dataPath = 'data/';

//app.disableHardwareAcceleration();

const ready = () => {
    var newWindow = new BrowserWindow({
        width: 800,
        height: 600,
        title: `Simple Mahjong`,
        show: false,
        webPreferences: {
            nodeIntegration: true
        }
    });
    var browseWindow = new BrowserWindow({
        title: `browser`,
        show: false,
        webPreferences: {
            nodeIntegration: true
        }
    });

    browseWindow.nowingamepage = true;

    browseWindow.webContents.on('dom-ready', function (){
        //browseinject();
        if (!/^https:\/\/(?:majsoul|game.mahjongsoul|mahjongsoul)/.test(browseWindow.webContents.getURL())){
            if (browseWindow.nowingamepage)
                dialog.showMessageBox({
                    type: 'info',
                    noLink: true,
                    buttons: ['确定'],
                    title: '第三方账户登录提示',
                    message: '您已离开游戏页面进行第三方账户登录。如果出现无法正常登录的情况，请点击网页-登录专用窗口进行登录，在登录完成后关闭专用窗口并刷新本页面。'
                });
            browseWindow.nowingamepage = false;
        }
        else browseWindow.nowingamepage = true;
    });

    function browseinject() {
        fs.readFile(path(__dirname, 'lib', 'jquery.js'), function (error, jdata) {
            if (error) {
                console.log('read jquery.js error: ' + error);
            }
            else {
                fs.readFile(path(__dirname, 'lib', 'wshook.js'), function (error, wsdata) {
                    if (error) {
                        console.log('read wshook.js error: ' + error);
                    }
                    else fs.readFile(path(__dirname, 'lib', 'majsoul', 'browseinject.js'), function (error, browsedata) {
                        if (error) {
                            console.log('read browseinject.js error: ' + error);
                        }
                        else{
                            browsedata = String(browsedata).replace(/\/\/%jqueryjs%/, jdata);
                            browsedata = browsedata.replace(/\/\/%wshookjs%/, wsdata);
                            browseWindow.webContents.executeJavaScript(browsedata);
                            if (/https:\/\/open\.weixin\.qq\.com/.test(browseWindow.webContents.getURL()))
                                browseWindow.webContents.executeJavaScript(`
                                    console.log('try to run weixin script again');
                                    ss = $('script');
                                    for (let i = 0; i < ss.length; i ++ )
                                        if (/majsoul/.test($(ss[i]).html()))
                                            eval(ss[i].textContent);
                                `);
                        }

                    });
                });
            }
        });
    }
    
    function bwindowload(url){
        browseWindow.injectfinish = false;
        for (let i in paipugamedata)
            paipugamedata[i] = undefined;
        if (url == undefined)
            browseWindow.reload();
        else browseWindow.loadURL(url);
        browseinject();
    }

    function showcantgetIDmsg(){
        dialog.showMessageBox({
            type: 'error',
            noLink: true,
            buttons: ['确定'],
            title: '错误',
            message: '获取信息错误！无法获取用户ID，请确认已经进入大厅。或者尝试刷新页面。'
        });
    }
    
    function checkpaipugamedata(){
        let msgstr = '', root, paipu4 = 0, paipu3 = 0, downloaded = 0, converted = 0, userid = getUserID();
        if (userid <= 0){
            showcantgetIDmsg();
            return;
        }
        root = path(dataPath, 'majsoul', userid.toString());
        let rawdirdata = new Set(fs.readdirSync(path(root, 'raw')));
        let paipusdirdata = new Set(fs.readdirSync(path(root, 'paipus')));
        for (let id in paipugamedata){
            let data = paipugamedata[id];
            if (data.roomdata.player == 3) paipu3 ++ ;
            else paipu4 ++ ;
            if (rawdirdata.has(id)) downloaded ++ ;
            if (paipusdirdata.has(id)) converted ++ ;
        }
        msgstr = '用户ID: ' + userid + '\n4人牌谱: ' + paipu4 + '\n3人牌谱: ' + paipu3 + '\n已下载: ' + downloaded + '\n已转换: ' + converted;
        dialog.showMessageBox({
            type: 'info',
            noLink: true,
            buttons: ['确定'],
            title: '牌谱情报',
            message: msgstr
        });
    }

    var downloadconvertlist = undefined, downloadconvertresult = undefined;

    function downloadconvertpaipu(){
        let userid = getUserID();
        if (userid <= 0){
            showcantgetIDmsg();
            return;
        }
        let root = path(dataPath, 'majsoul', userid.toString());
        let rawdirdata = new Set(fs.readdirSync(path(root, 'raw')));
        let paipusdirdata = new Set(fs.readdirSync(path(root, 'paipus')));
        let gamedatatxt = path(root, 'gamedata.txt');
        fs.writeFileSync(gamedatatxt, '');
        for (let id in paipugamedata)
            fs.appendFileSync(gamedatatxt, JSON.stringify(paipugamedata[id]) + '\n');
        if (downloadconvertlist != undefined){
            dialog.showMessageBox({
                type: 'error',
                noLink: true,
                buttons: ['确定'],
                title: '错误',
                message: '存在正在执行的下载转换任务！'
            });
            return;
        }
        downloadconvertlist = [];
        downloadconvertresult = [0, 0, 0, 0];
        for (let id in paipugamedata){
            if (paipugamedata[id].roomdata.player == 4 && (!rawdirdata.has(id) || !paipusdirdata.has(id))){
                downloadconvertlist.push(id);
                downloadconvertresult[2] ++ ;
            }
        }
        dialog.showMessageBox({
            type: 'info',
            noLink: true,
            buttons: ['确定'],
            title: '开始下载转换',
            message: '共有 ' + downloadconvertlist.length + ' 个牌谱需要下载转换\n在模拟窗口左上角可以看到进度'
        });
        nextdownloadconvert();
    }

    function nextdownloadconvert(){
        if (downloadconvertlist.length == 0){
            newWindow.webContents.send('downloadconvert', {}, downloadconvertresult);
            setTimeout(function () { finaldownloadconvert(); }, 100);
            return;
        }
        let id = downloadconvertlist[0];
        newWindow.webContents.send('downloadconvert', paipugamedata[id], downloadconvertresult);
    }

    function finaldownloadconvert(){
        //把牌谱连成一个数组减少分析时磁盘读取次数
        let paipus = [];
        let userid = getUserID();
        let paipudir = path(dataPath, 'majsoul', userid.toString(), 'paipus');
        let paipulist = fs.readdirSync(paipudir);
        for (let i in paipulist)
            paipus.push(JSON.parse(fs.readFileSync(path(paipudir, paipulist[i]))));
        fs.writeFileSync(path(dataPath, 'majsoul', userid.toString(), 'paipus.txt'), JSON.stringify(paipus));

        let d = new Date();
        d.setTime(downloadconvertresult[3] * 1000);
        let timestr = d.toString();
        dialog.showMessageBox({
            type: 'info',
            noLink: true,
            buttons: ['确定'],
            title: '下载转换完成',
            message: '完成 ' + downloadconvertresult[0] + '/' + downloadconvertresult[2] + ' 个下载转换任务。下载成功牌谱的最晚时间是' + timestr + '\n-----\n注意：由于技术原因，最近2-4天的牌谱可能暂时无法获取，请在几天后再次尝试。'
        });
        downloadconvertresult = undefined;
        downloadconvertlist = undefined;
    }

    function downloadconvertcallback(data){
        let userid = getUserID();
        downloadconvertresult[1] ++ ;
        let rawp = path(dataPath, 'majsoul', userid.toString(), 'raw', downloadconvertlist[0]);
        let paipup = path(dataPath, 'majsoul', userid.toString(), 'paipus', downloadconvertlist[0]);
        downloadconvertlist.splice(0, 1);
        let raw = data.raw, paipu = data.paipu;
        if (raw != undefined){
            downloadconvertresult[0] ++ ;
            fs.writeFileSync(rawp, raw);
            fs.writeFileSync(paipup, JSON.stringify(paipu));
            if (paipu.gamedata.endtime > downloadconvertresult[3])
                downloadconvertresult[3] = paipu.gamedata.endtime;
        }
        nextdownloadconvert();
    }

    function gotonewpage(str){
        config.set('DefaultURL', str);
        bwindowload(str);
    }

    function collectpaipucallback(err, option, data){
        if (err != undefined){
            dialog.showMessageBox({
                type: 'error',
                noLink: true,
                buttons: ['确定'],
                title: '错误',
                message: err.type == 'UserID' ? cantgetIDstr : 'uiscript未定义，请确认已经进入雀魂。'
            });
            return;
        }
        analyzeGameRecord(data);
        if (option == 'checkpaipugamedata')
            checkpaipugamedata();
        else if (option == 'downloadconvertpaipu')
            downloadconvertpaipu();
    }

    ipcMain.on('collectpaipucallback', (event, err, option, data) => {
        collectpaipucallback(err, option, data);
    });

    function bwindowsendmessage(cmd, d1, d2, d3, d4){ //先放4个参数，不够再加
        if (!browseWindow.injectfinish){
            dialog.showMessageBox({
                type: 'error',
                noLink: true,
                buttons: ['确定'],
                title: '错误',
                message: '还未完成脚本注入，请稍后重试。'
            });
            return;
        }
        browseWindow.webContents.send(cmd, d1, d2, d3, d4);
    }

    var menutemplate = [{
        label: '牌谱',
        submenu: [{
            label: '查看已有牌谱情报',
            click: function () {
                bwindowsendmessage('collectpaipu', 'checkpaipugamedata');
            }
        }, {
            label: '下载&转换牌谱',
            click: function () {
                bwindowsendmessage('collectpaipu', 'downloadconvertpaipu');
            }
        }]
    }, {
        label: '网页',
        submenu: [{
            label: '刷新',
            click: function () {
                bwindowload();
            }
        }, {
            label: '进入国服',
            click: function () {
                gotonewpage('https://majsoul.union-game.com/0/');
            }
        }, {
            label: '进入日服',
            click: function () {
                gotonewpage('https://game.mahjongsoul.com');
            }
        }, {
            label: '进入国际服',
            click: function () {
                gotonewpage('https://mahjongsoul.game.yo-star.com');
/*                 dialog.showMessageBox({
                    type: 'info',
                    noLink: true,
                    buttons: ['确定'],
                    title: '国际服提示',
                    message: '由于技术原因，使用国际服时请确保当前网络能够较为通畅的访问Google, FaceBook等，否则很可能无法正确获取牌谱数据。'
                }); */
            }
        }, {
            label: '登录专用窗口',
            click: function () {
                var loginWindow = new BrowserWindow({
                    title: `login`,
                    show: false
                });
                let str = config.get('DefaultURL');
                loginWindow.loadURL(str);
                loginWindow.show();
            }
        }]
    }, {
        label: '其他',
        submenu: [{
            label: '清除配置数据',
            click: function() {
                dialog.showMessageBox({
                    type: 'warning',
                    title: '警告',
                    message: '是否清除配置数据？\n错误上报、默认服务器、更新检查等相关配置会被清除，data文件夹和config.json不会改动。',
                    cancelId: 1,
                    noLink: true,
                    buttons: ['是', '否']
                }, function (response) {
                    if (response == 0){
                        config.clear();
                    }
                });
            }
        }, {
            label: '关于',
            click: function() {
                dialog.showMessageBox({
                    type: 'info', 
                    title: '关于',
                    noLink: true,
                    buttons: ['确定'],
                    message: `MajsoulPaipuCrawler v${config.get('Version')}\nContact: jzjqz17@gmail.com\nGithub: https://github.com/zyr17/MajsoulPaipuAnalyzer`
                });
            }
        }]
    }];

    var bmenu = Menu.buildFromTemplate(menutemplate);
    Menu.setApplicationMenu(bmenu);

    newWindow.loadURL(url.format({
        protocol: 'file:',
        slashes: true,
        pathname: path(__dirname, 'SimpleMahjong', 'index.html')
    }));
    newWindow.show();
    newWindow.openDevTools();
    bwindowload(config.get('DefaultURL'));
    browseWindow.show();
    browseWindow.maximize();
    browseWindow.openDevTools();
    function savegamedata(userid){
        let root = path(dataPath, 'majsoul', userid.toString());
        if (!fs.existsSync(dataPath)) fs.mkdirSync(dataPath);
        if (!fs.existsSync(path(dataPath, 'majsoul'))) fs.mkdirSync(path(dataPath, 'majsoul'));
        if (!fs.existsSync(root)) fs.mkdirSync(root);
        let ppp = path(root, 'raw');
        if (!fs.existsSync(ppp)) fs.mkdirSync(ppp);
        ppp = path(root, 'paipus');
        if (!fs.existsSync(ppp)) fs.mkdirSync(ppp);
        
        let gamedatatxt = path(root, 'gamedata.txt');
        let gamedatas = [];
        if (fs.existsSync(gamedatatxt))
            gamedatas = fs.readFileSync(gamedatatxt).toString().split('\n');
        for (let i in gamedatas){
            let gamedata = gamedatas[i];
            if (gamedata.length < 2) continue;
            let gdata = JSON.parse(gamedata);
            let id = gdata.uuid;
            if (paipugamedata[id] == undefined)
                paipugamedata[id] = gdata;
        }
    }
    ipcMain.on('downloadconvertresult', (event, data) => {
        downloadconvertcallback(data);
    });
    ipcMain.on('reporterror', (event, data) => {
        reporterror(data);
    });
    ipcMain.on('userid', (event, data) => {
        setUserID(data);
        savegamedata(data);
        newWindow.webContents.send('userid', data);
    });
    ipcMain.on('bwindowinjectfinish', (event) => {
        browseWindow.injectfinish = true;
    });
    
    setTimeout(checknewestversion, 3000);
};

app.on('ready', ready);

app.on('window-all-closed', () => app.quit());
