"use strict";

const electron = require("electron"); 
const ipcr = electron.ipcRenderer; 

ipcr.on('downloadconvert', function (event, gamedata, isconvert, arr, bytearr = null) {
    if (arr[1] == arr[2]){
        $('#cornerinformation').text('DOWNLOAD & CONVERT COMPLETE, COMBINING...');
        return;
    }
    $('#cornerinformation').text(arr[1] + '/' + arr[2] + ', ' + arr[0] + ' SUCCESS, ' + (arr[1] - arr[0]) + ' ONLY DOWNLOAD');
    majsoulpaipuanalyzer.ipcrsend = true;
    majsoulpaipuanalyzer.onlydownload = !isconvert;
    if (!bytearr){
        if (gamedata.url){
            let prefix = gamedata.url.replace(gamedata.uuid, '');
            //console.log(gamedata.url, gamedata.uuid, prefix);
            if (prefix == '/') prefix = 'https://record-old.maj-soul.com:9443/majsoul/game_record/';
            majsoulpaipuanalyzer.convertsomepaipu([gamedata], prefix);
        }
        else majsoulpaipuanalyzer.convertsomepaipu([gamedata]);
    }
    else majsoulpaipuanalyzer.convertpaipuwithbytes(gamedata, bytearr);
});

ipcr.on('fetchliqijsoncallback', (event, data, room) => {
    AnalyzeInit(data, room);
});