const fs = require('fs');

let alltext = fs.readFileSync(__dirname + '/doc/release-notes.txt').toString();
let packagefile = JSON.parse(fs.readFileSync(__dirname + '/package.json').toString());
alltext = alltext.replace(/\r/g, '');
let text = '';
let package_version = 'v' + packagefile.version;
let version = '';
for (let i = 0; i < alltext.length; i ++ ){
    if (alltext[i] == '\n' && version.length == 0)
        version = 'v' + alltext.slice(0, i);
    if (alltext[i] == '\n' && alltext[i + 1] == '\n'){
        text = alltext.slice(0, i);
        break;
    }
}

if (package_version != version){
    console.error('version differ!', package_version, version);
    process.exit(1);
}

version = package_version;

let result = {
    "tag_name": version,
    "target_commitish": "master",
    "name": version,
    "body": text,
    "draft": false,
    "prerelease": false
};

result = JSON.stringify(result);

let token = process.argv[2];
let repo = process.argv[3];

let cmd = 'curl -X POST -H "Authorization: token ' + token + '"'
          + ' --data ' + "'" + result + "'" 
          + ' https://api.github.com/repos/' + repo + '/releases';

console.log(cmd.replace(token, 'b2c736a8de4c71f45429acc0775671d9b5e452c3'));

var exec = require('child_process').exec;

exec(cmd, function(error, stdout, stderr) {
    if(error){
        console.error(error);
        process.exit(1);
    }
    else{
        console.log("curl success, stdout:\n" + stdout);
    }
});

