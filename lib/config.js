"use strict";

const fs = require('fs');
const path = require('path');

var config = undefined;

function loadconfig(){
    let p = path.join(__dirname, 'config.json');
    if (fs.existsSync(p))
        config = JSON.parse(fs.readFileSync(p).toString());
    else{
        config = {
            DefaultURL: 'https://majsoul.union-game.com/0/'
        };
        fs.writeFileSync(p, JSON.stringify(config));
    }
}

var configF = {
    get: function (id){
        if (config == undefined)
            loadconfig();
        return config[id];
    },
    set: function (id, val, save = true){
        config[id] = val;
        if (save) fs.writeFileSync(path.join(__dirname, 'config.json'), JSON.stringify(config));
    }
}

module.exports = {
    config: configF
};