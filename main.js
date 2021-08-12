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
      prompt                         = require('electron-prompt'),
      { config, checknewestversion } = require('./lib/config.js');

const { analyze, paipugamedata, analyzeGameRecord, getUserID, setUserID, AnalyzeInit, Protobuf2Object, reporterror } = require('./lib/majsoul/analyze');

let InMacOS = process.platform == 'darwin';
let activate_devtool = 0;

var paipuversion = undefined;
var appPath = app.getAppPath();
app.setPath('userData', appPath + '/UserData');
let dataPath = 'data/';

if (InMacOS) dataPath = __dirname + '/../../../../' + dataPath;

//app.disableHardwareAcceleration();

const ready = () => {
    var newWindow = new BrowserWindow({
        width: 650,
        height: 700,
        title: `Simple Mahjong`,
        show: false,
        webPreferences: {
            nodeIntegration: true
        }
    });
    var browseWindow = new BrowserWindow({
        width: 1280,
        height: 770,
        title: `browser`,
        show: false,
        webPreferences: {
            nodeIntegration: true
        }
    });

    browseWindow.nowingamepage = true;

    browseWindow.webContents.on('dom-ready', function (){
        //browseinject();
        if (!/^https:\/\/(?:(?:www\.)?majsoul|game.mah?jo?n?g?-?soul|mahjongsoul)/.test(browseWindow.webContents.getURL())){
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

    browseWindow.webContents.on('did-navigate', function (){
        if (browseWindow.injectfinish == 2){
            //如果跳转页面时已注入，那么标记为未注入并调用browseinject
            browseWindow.injectfinish = 0;
            for (let i in paipugamedata)
                delete paipugamedata[i];
            paipuversion = undefined;
            browseinject();
        }
    });

    function browseinject() {
        if (browseWindow.injectfinish != 0)
            //1 2对应注入请求发送未执行；注入完成。均不需要尝试注入
            return;
        fs.readFile(path(__dirname, 'lib', 'majsoul', 'browseinject.js'), function (error, browsedata) {
            if (error) console.log('read browseinject.js error: ' + error);
            else{
                browseWindow.injectfinish = 1;
                browseWindow.webContents.executeJavaScript(String(browsedata));
            }
        });
    }
    
    function bwindowload(url){
        if (browseWindow.injectfinish == 2 || browseWindow.injectfinish == undefined)
            browseWindow.injectfinish = 0;
        for (let i in paipugamedata)
            delete paipugamedata[i];
        paipuversion = undefined;
        if (url == undefined)
            browseWindow.reload();
        else browseWindow.loadURL(url, {userAgent: 'Chrome'});
        browseinject();
    }

    var cantgetIDstr = '获取信息错误！无法获取用户ID，请确认已经进入大厅。或者尝试刷新页面。';

    function showcantgetIDmsg(){
        dialog.showMessageBox({
            type: 'error',
            noLink: true,
            buttons: ['确定'],
            title: '错误',
            message: cantgetIDstr
        });
    }

    function isspecialrule(roomdata){
        return roomdata.fanfu > 1 //>1番缚
            || roomdata.guyi_mode //古役
            || roomdata.begin_open_mode //配牌明牌
            || roomdata.xuezhandaodi //血战到底
            || roomdata.huansanzhang //换三张
        ;
    }

    function iserrorpaipu(gamedata){
        // 空数据，无房间号，房间号>=100是活动场
        return !gamedata //空数据
            || !gamedata.roomdata //空房间数据
            || gamedata.roomdata.room == undefined //无房间号
            || gamedata.roomdata.room >= 100 //房间号>=100是活动场
            || isspecialrule(gamedata.roomdata) //包含特殊规则
        ;
    }
    
    function checkpaipugamedata(){
        let msgstr = '', root, paipu4 = 0, paipu3 = 0, downloaded = 0, converted = 0, userid = getUserID(), errordata = 0;
        if (userid <= 0){
            showcantgetIDmsg();
            return;
        }
        root = path(dataPath, 'majsoul', userid.toString());
        let rawdirdata = new Set(fs.readdirSync(path(root, 'raw')));
        let paipusdirdata = new Set(fs.readdirSync(path(root, 'paipus')));
        for (let id in paipugamedata){
            let data = paipugamedata[id];
            if (iserrorpaipu(data)) errordata ++ ;
            else if (data.roomdata.player == 3) paipu3 ++ ;
            else paipu4 ++ ;
            if (rawdirdata.has(id)) downloaded ++ ;
            if (paipusdirdata.has(id)) converted ++ ;
        }
        msgstr = '用户ID: ' + userid 
               + '\n4人牌谱: ' + paipu4 
               + '\n3人牌谱: ' + paipu3 
               + '\n未识别(无法分析，活动规则)牌谱: ' + errordata
               + '\n已下载: ' + downloaded 
               + '\n已转换: ' + converted;
        dialog.showMessageBox({
            type: 'info',
            noLink: true,
            buttons: ['确定'],
            title: '牌谱情报',
            message: msgstr
        });
    }

    var downloadconvertlist = undefined, downloadconvertresult = undefined;
    var downloadnumber = 0, convertnumber = 0, downloadcount = 0, convertcount = 0;

    function downloadconvertpaipu(){
        let userid = getUserID();
        if (userid <= 0){
            showcantgetIDmsg();
            return;
        }
        let root = path(dataPath, 'majsoul', userid.toString());
        let rawdirdata = new Set(fs.readdirSync(path(root, 'raw')));
        let paipusdirdata = new Set(fs.readdirSync(path(root, 'paipus')));
        if (downloadconvertlist != undefined){
            dialog.showMessageBox({
                type: 'error',
                noLink: true,
                buttons: ['确定'],
                title: '错误',
                message: '存在正在执行的下载转换任务！如果转换异常停止请重启工具。'
            });
            return;
        }
        downloadconvertlist = [];
        downloadconvertresult = [0, 0, 0, 0, 0]; // convert-success, index, total, time
        downloadnumber = convertnumber = downloadcount = convertcount = 0;
        for (let id in paipugamedata){
            let needpaipu = !iserrorpaipu(paipugamedata[id]) && paipugamedata[id].roomdata.player == 4;
            if (!rawdirdata.has(id) || !paipusdirdata.has(id) && needpaipu){
                let needconvert = needpaipu && !paipusdirdata.has(id);
                // id, (false: only download, true: download and convert)
                downloadconvertlist.push([id, needconvert]);
                if (needconvert) convertnumber ++ ;
                downloadconvertresult[2] ++ ;
            }
            if (!needpaipu && paipusdirdata.has(id)){
                // no need for convert and has converted, remove
                console.log('find no need convert paipu, delete it', id, needpaipu);
                fs.unlinkSync(path(root, 'paipus', id));
            }
        }
        downloadnumber = downloadconvertresult[2];
        dialog.showMessageBox({
            type: 'info',
            noLink: true,
            buttons: ['确定'],
            title: '开始下载转换',
            message: '共有 ' + downloadconvertlist.length + ' 个牌谱需要下载\n有 ' +
            convertnumber + ' 个牌谱需要转换\n在模拟窗口左上角可以看到进度'
        });
        nextdownloadconvert();
    }

    function nextdownloadconvert(){
        if (downloadconvertlist.length == 0){
            //显示转换完成正在组合
            newWindow.webContents.send('downloadconvert', {}, true, downloadconvertresult);
            setTimeout(function () { finaldownloadconvert(); }, 100);
            return;
        }
        let id = downloadconvertlist[0][0];
        browseWindow.webContents.send('fetchpaipudata', paipugamedata[id].uuid);
        //newWindow.webContents.send('downloadconvert', paipugamedata[id], downloadconvertresult);
    }

    function fetchpaipudatacallback(res){
        let error = res.error, bytearr = res.data, url = res.data_url;
        if (error){
            if (error) console.log('read browseinject.js error: ' + JSON.stringify(error));
            downloadconvertlist.splice(0, 1);
            nextdownloadconvert();
            return;
        }
        let id = downloadconvertlist[0][0], isconvert = downloadconvertlist[0][1];
        let gamedata = paipugamedata[id];
        if (url){
            gamedata.url = url;
            newWindow.webContents.send('downloadconvert', gamedata, isconvert, downloadconvertresult);
        }
        else{
            newWindow.webContents.send('downloadconvert', gamedata, isconvert, downloadconvertresult, bytearr);
        }
    }

    ipcMain.on('fetchpaipudatacallback', (event, uuid, res) => {
        fetchpaipudatacallback(res);
    });

    function fetchpaipudatasave(uuid,res){
        let error = res.error, head = res.head, bytearr = res.data, url = res.data_url;
        if (error){
            dialog.showMessageBox({
                type: 'error',
                noLink: true,
                buttons: ['确定'],
                title: '下载失败',
                message: '错误信息：' + error
            });
            return;
        }
        let path = dialog.showSaveDialogSync({
            title: '存储位置',
            defaultPath: uuid
        });
        if (!path) return;
        if (url){
            dialog.showMessageBox({
                type: 'info',
                noLink: true,
                buttons: ['确定'],
                title: '下载地址',
                message: '获取到牌谱下载地址，已将地址保存于指定位置，请自行下载'
            });
        }
        fs.writeFileSync(path, url ? url : bytearr);
        fs.writeFileSync(path + '.header', JSON.stringify(head, null, 4));
        let p2obj = Protobuf2Object(bytearr);
        console.log(JSON.stringify(head, null, 4));
    }

    ipcMain.on('fetchpaipudatasave', (event, uuid, res) => {
        fetchpaipudatasave(uuid, res);
    });

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
            message: '完成 ' + downloadcount + '/' + downloadnumber + ' 个下载任务，完成 '
                     + convertcount + '/' + convertnumber + ' 个转换任务。\n下载成功牌谱的最晚时间是' + timestr
        });
        downloadconvertresult = undefined;
        downloadconvertlist = undefined;
    }

    function downloadconvertcallback(data){
        let userid = getUserID();
        downloadconvertresult[1] ++ ;
        let nowconvert = downloadconvertlist[0][0];
        let rawp = path(dataPath, 'majsoul', userid.toString(), 'raw', nowconvert);
        let paipup = path(dataPath, 'majsoul', userid.toString(), 'paipus', nowconvert);
        downloadconvertlist.splice(0, 1);
        let raw = data.raw, paipu = data.paipu;
        if (raw != undefined){
            fs.writeFileSync(rawp, raw);
            downloadcount ++ ;
            if (paipu){
                downloadconvertresult[0] ++ ;
                convertcount ++ ;
                fs.writeFileSync(paipup, JSON.stringify(paipu));
                if (paipu.gamedata.endtime > downloadconvertresult[3])
                    downloadconvertresult[3] = paipu.gamedata.endtime;
                paipuversion[nowconvert] = config.get('Version');
                let paipuversiontxt = path(dataPath, 'majsoul', userid.toString(), 'paipuversion.txt');
                fs.writeFileSync(paipuversiontxt, JSON.stringify(paipuversion));
            }
            else{
                // convert fail or only download, remove paipu to avoid error
                if (fs.existsSync(paipup)){
                    fs.unlinkSync(paipup);
                    console.log('paipup', paipup);
                }
            }
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
        let gamedatatxt = path(dataPath, 'majsoul', getUserID().toString(), 'gamedata.txt');
        fs.writeFileSync(gamedatatxt, '');
        for (let id in paipugamedata)
            fs.appendFileSync(gamedatatxt, JSON.stringify(paipugamedata[id]) + '\n');
        if (option == 'checkpaipugamedata')
            checkpaipugamedata();
        else if (option == 'downloadconvertpaipu')
            downloadconvertpaipu();
    }

    ipcMain.on('collectpaipucallback', (event, err, option, data) => {
        collectpaipucallback(err, option, data);
    });

    function bwindowsendmessage(cmd, d1, d2, d3, d4){ //先放4个参数，不够再加
        if (browseWindow.injectfinish != 2){
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
            label: '自动获取牌谱数据',
            click: function () {
                bwindowsendmessage('collectallpaipu');
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
            label: '进入中文服',
            click: function () {
                gotonewpage('https://game.maj-soul.com/1/');
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
            label: '进入国服（已关服）',
            click: function () {
                gotonewpage('https://www.majsoul.com/1/');
            }
        }, {
            label: '登录专用窗口',
            click: function () {
                var loginWindow = new BrowserWindow({
                    title: `login`,
                    show: false,
                    webPreferences: {
                        nodeIntegration: false
                    }
                });
                let str = config.get('DefaultURL');
                loginWindow.loadURL(str, {userAgent: 'Chrome'});
                loginWindow.show();
            }
        }]
    }, {
        label: '其他',
        submenu: [{
            label: '清除配置数据',
            click: function() {
                let response = dialog.showMessageBoxSync({
                    type: 'warning',
                    title: '警告',
                    message: '是否清除配置数据？\n错误上报、默认服务器、更新检查等相关配置会被清除，data文件夹和config.json不会改动。',
                    cancelId: 1,
                    noLink: true,
                    buttons: ['是', '否']
                });
                if (response == 0){
                    config.clear();
                }
            }
        }, {
            label: '打开开发者工具',
            click: function() {
                newWindow.openDevTools();
                browseWindow.openDevTools();
            }
        }, {
            label: '下载牌谱并保存',
            click: function() {
                if (activate_devtool != 3)
                    return;
                prompt({
                    title: '输入牌谱号',
                    label: '牌谱号：',
                }).then((res) => {
                    let uuid = /\d{6}-[a-z0-9-]*/.exec(res).toString();
                    browseWindow.webContents.send('fetchpaipudata', uuid, 'fetchpaipudatasave');
                });
            }
        }, {
            label: '选择牌谱并保存元信息',
            click: function() {
                if (activate_devtool != 3)
                    return;
                let path = dialog.showOpenDialogSync({
                    title: '牌谱文件',
                    multiSelections: true
                });
                if (!path)
                    return;
                for (let i in path){
                    let data = fs.readFileSync(path[i]);
                    data = analyze(data);
                    fs.writeFileSync(path[i] + '.json', JSON.stringify(data.toJSON()));
                }
            }
        }, {
            label: '关于',
            click: function() {
                let response = dialog.showMessageBoxSync({
                    type: 'info', 
                    title: '关于',
                    noLink: true,
                    buttons: ['确定', '取消'],
                    message: `MajsoulPaipuCrawler v${config.get('Version')}\nContact: jzjqz17@gmail.com\nGithub: https://github.com/zyr17/MajsoulPaipuAnalyzer`
                });
                activate_devtool |= response + 1;
            }
        }]
    }];

    if (InMacOS) menutemplate.splice(0, 0, {label: 'MacOS', submenu: []});

    var bmenu = Menu.buildFromTemplate(menutemplate);
    Menu.setApplicationMenu(bmenu);

    newWindow.loadURL(url.format({
        protocol: 'file:',
        slashes: true,
        pathname: path(__dirname, 'SimpleMahjong', 'index.html')
    }));
    newWindow.show();
    bwindowload(config.get('DefaultURL'));
    browseWindow.show();
    //browseWindow.maximize();
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
        let gamedatas = [], oldgamedatas = [];
        if (fs.existsSync(gamedatatxt))
            gamedatas = fs.readFileSync(gamedatatxt).toString().split('\n');
        for (let i in gamedatas){
            let gamedata = gamedatas[i];
            if (gamedata.length < 2) continue;
            let gdata = JSON.parse(gamedata);
            if (gdata.version == undefined || config.versionconvert(gdata.version) < config.versionconvert(config.get('GamedataMinVersion'))){
                oldgamedatas.push(gdata);
                continue;
            }
            let id = gdata.uuid;
            if (paipugamedata[id] == undefined)
                paipugamedata[id] = gdata;
        }
        if (oldgamedatas.length > 0)
            dialog.showMessageBox({
                type: 'info',
                noLink: true,
                buttons: ['确定'],
                title: '发现旧牌谱',
                message: `发现 ${oldgamedatas.length} 个旧版本获取的牌谱数据，请前往 牌谱 界面重新获取一遍牌谱数据。`
            });

        const paipu_bk_folder_name = 'old_paipu_backup';
        paipuversion = {};
        ppp = path(root, 'paipuversion.txt');
        if (fs.existsSync(ppp))
            paipuversion = JSON.parse(String(fs.readFileSync(ppp)));
        let paipusdirdata = new Set(fs.readdirSync(path(root, 'paipus')));
        let oldpaipus = [], dirite = paipusdirdata.keys();
        for (;;){
            let i = dirite.next();
            if (i.done) break;
            i = i.value;
            if (i == paipu_bk_folder_name)
                continue;
            let add = false;
            if (!paipuversion.hasOwnProperty(i)){
                let paipudata = JSON.parse(String(fs.readFileSync(path(root, 'paipus', i))));
                if (paipudata.gamedata && paipudata.gamedata.version)
                    paipuversion[i] = paipudata.gamedata.version;
            }
            if (!paipuversion.hasOwnProperty(i) || config.versionconvert(paipuversion[i]) < config.versionconvert(config.get('PaipuMinVersion')))
                    add = true;
            if (add)
                oldpaipus.push(i);
        }
        fs.writeFileSync(ppp, JSON.stringify(paipuversion));

        if (oldpaipus.length == 0) return;

        dialog.showMessageBoxSync({
            type: 'info',
            noLink: true,
            buttons: ['确定'],
            title: '发现旧牌谱',
            message: `发现 ${oldpaipus.length} 个旧版本生成的牌谱，需要重新生成，会将旧牌谱移至备份文件夹，可能需要一定时间。`
        });
        let bkpaipupath = path(root, 'paipus', paipu_bk_folder_name);
        if (fs.existsSync(bkpaipupath)){
            /*
            no need to do anything
            dialog.showMessageBoxSync({
                type: 'warning',
                noLink: true,
                buttons: ['确定'],
                title: '备份文件夹已存在',
                message: '备份文件夹已存在，可能由之前的牌谱备份操作创建，'
            });
            */
        }
        else{
            fs.mkdirSync(bkpaipupath);
        }
        for (let i in oldpaipus){
            let ppp = path(root, 'paipus', oldpaipus[i]);
            let ttt = path(bkpaipupath, oldpaipus[i]);
            fs.renameSync(ppp, ttt);
        }
        dialog.showMessageBox({
            type: 'info',
            noLink: true,
            buttons: ['确定'],
            title: '发现旧牌谱',
            message: `旧牌谱转移完成。`
        });
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
        browseWindow.injectfinish = 2;
    });

    ipcMain.on('fetchliqijsoncallback', (event, data, room) => {
        AnalyzeInit(data, room);
        newWindow.webContents.send('fetchliqijsoncallback', data, room);
    });
    
    setTimeout(checknewestversion, 3000);
};

app.on('ready', ready);

app.on('window-all-closed', () => app.quit());
