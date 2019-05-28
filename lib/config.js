"use strict";

const { dialog, shell } = require('electron');
const fs = require('fs');
const path = require('path');
const request = require('request');
const { version, repository } = require('../package.json');

var config = undefined;
const configfilename = 'config.json';

function loadconfig(){
    let p = path.join(__dirname, configfilename);
    if (fs.existsSync(p))
        config = JSON.parse(fs.readFileSync(p).toString());
    else{
        config = {
            DefaultURL: 'https://majsoul.union-game.com/0/'
        };
    }
    config.Version = version;
    fs.writeFileSync(p, JSON.stringify(config));
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
    let pj = repository + '/raw/master/package.json';
    request(pj, function (error, response, body){
        if (error) checkversionresult(false, error);
        else if (response.statusCode != 200) checkversionresult(false, 'Error Code:' + response.statusCode);
        else{
            try{
                let json = JSON.parse(body);
                let nowver = config.Version.split('.'), remotever = json.version.split('.');
                if (nowver.length < 3 || remotever.length < 3) throw new Error();
                let havenew = false;
                for (let i = 0; i < 3; i ++ )
                    if (parseInt(nowver[i]) > parseInt(remotever[i])) break;
                    else if (parseInt(nowver[i]) < parseInt(remotever[i])) havenew = true;
                if (havenew) checkversionresult(true, json.version);
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

function configFset (id, val, save = true){
    config[id] = val;
    if (save) fs.writeFileSync(path.join(__dirname, configfilename), JSON.stringify(config));
}
function configFclear (){
    config = {};
    fs.unlinkSync(path.join(__dirname, configfilename));
    loadconfig();
}

var configF = {
    get: configFget,
    set: configFset,
    clear: configFclear
};

module.exports = {
    config: configF,
    checknewestversion
};