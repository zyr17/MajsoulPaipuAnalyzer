"use strict";

const { dialog, shell } = require('electron');
const fs = require('fs');
const path = require('path');
const request = require('request');
const { version, repository } = require('../package.json');

const defaultconfig = {
    DefaultURL: 'https://game.maj-soul.com/1/',
    PaipuMinVersion: '0.5.5',
    GamedataMinVersion: '0.4.11',
    ConfigMinVersion: '0.5.5'
};
var config = undefined;
const configfilename = 'config.json';

function getAppDataPath() {
    switch (process.platform) {
        case "darwin": {
            return path.join(process.env.HOME, "Library", "Application Support", "MajsoulPaipuAnalyzer");
        }
        case "win32": {
            return path.join(process.env.APPDATA, "MajsoulPaipuAnalyzer");
        }
        case "linux": {
            return path.join(process.env.HOME, "MajsoulPaipuAnalyzer");
        }
        default: {
            console.log("Unsupported platform! " + process.platform);
            process.exit(1);
        }
    }
}
  
function saveconfig(content) {
    const appDatatDirPath = getAppDataPath();
    
    // Create appDataDir if not exist
    if (!fs.existsSync(appDatatDirPath)) {
        fs.mkdirSync(appDatatDirPath);
    }

    const appDataFilePath = path.join(appDatatDirPath, configfilename);
    content = JSON.stringify(content);

    fs.writeFile(appDataFilePath, content, (err) => {
        if (err) {
            console.log("There was a problem saving data!");
            console.log(err);
        } else {
            console.log("config saved correctly!");
        }
    });
}

function loadconfig(){
    let configpath = path.join(getAppDataPath(), configfilename);
    if (fs.existsSync(configpath))
        config = JSON.parse(fs.readFileSync(configpath).toString());
    else{
        console.log('no config found, use defult')
        config = defaultconfig;
    }
    if (config.Version != undefined && configFversionconvert(config.Version) < configFversionconvert(defaultconfig.ConfigMinVersion))
        config = defaultconfig;
    config.Version = version;
    saveconfig(config);
}

function checkversionresult(success, data){
    dialog.showMessageBox({
        type: success ? 'info' : 'error',
        title: '新版本检查',
        message: success ? 
                 (`发现新版本，目前版本号 ${config.Version} ，最新版本号 ${data} ，是否打开GitHub主页查看更新？`) : 
                 ('检查更新出现错误！\n错误信息：' + data)
        ,
        noLink: true,
        cancelId: success ? 1 : 0,
        checkboxLabel: '之后不再检查新版本',
        buttons: success ? ['是', '否'] : ['确定']
    }, function (response, checked) {
        if (success && response == 0)
            shell.openItem(repository);
        if (checked) configFset('NeverCheckNewVersion', true);
    });
}

function checknewestversion(){
    if (config.NeverCheckNewVersion != undefined && config.NeverCheckNewVersion) return;
    let pj0 = repository + '/raw/master/package.json';
    let pj1 = repository.replace('https://github.com', 'https://fastly.jsdelivr.net/gh') + '/package.json';
    let pj2 = repository.replace('https://github.com', 'https://gitee.com') + '/raw/master/package.json';  // if jsdelivr fails frequently, can try gitee
    let pj = pj1;
    request(pj, function (error, response, body){
        if (error) checkversionresult(false, error);
        else if (response.statusCode != 200) checkversionresult(false, 'Error Code:' + response.statusCode);
        else{
            try{
                let json = JSON.parse(body);
                let nowver = config.Version, remotever = json.version;
                console.log('remote version information', remotever);
                if (configFversionconvert(nowver) < configFversionconvert(remotever))
                    checkversionresult(true, json.version);
            }
            catch (e){
                checkversionresult(false, 'Parse JSON error');
            }
        }
    });
}

function configFget(id){
    if (config == undefined)
        loadconfig();
    return config[id];
}

function configFset(id, val, save = true){
    config[id] = val;
    if (save) saveconfig(config);
}
function configFclear(){
    config = {};
    fs.unlinkSync(path.join(__dirname, configfilename));
    loadconfig();
}

function configFversionconvert(version){
    version = version.split('.');
    for (let i in version)
        version[i] = parseInt(version[i]) + 1000;
    //console.log(version.join('.'));
    return version.join('.');
}

var configF = {
    get: configFget,
    set: configFset,
    clear: configFclear,
    versionconvert: configFversionconvert
};

module.exports = {
    config: configF,
    checknewestversion
};