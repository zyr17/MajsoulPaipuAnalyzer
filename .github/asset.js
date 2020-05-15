const fs = require('fs');

console.log(process.argv);

let github_event = JSON.parse(fs.readFileSync(process.argv[3]).toString());

let token = process.argv[2];
let id = github_event.release.id;
let repo = process.argv[4];
let filename = process.argv[5];
let filetype = process.argv[6] || 'application/octet-stream';
let length = fs.statSync(filename).size;

let cmd = 'curl -v -X POST'
          + ' -H "Authorization: token ' + token + '"'
          + ' -H "Content-Length: ' + length + '"'
          + ' -H "Content-Type: ' + filetype + '"'
          + ' --upload-file ' + filename
          + ' https://uploads.github.com/repos/' + repo + '/releases/' + id + '/assets?name=' + filename;

console.log(cmd.replace(token, 'b2c736a8de4c71f45429acc0775671d9b5e452c3'));

var exec = require('child_process').exec;

exec(cmd, function(error, stdout, stderr) {
    if(error){
        console.error(error);
    }
    else{
        console.log("curl success, stdout:\n" + stdout);
    }
});

