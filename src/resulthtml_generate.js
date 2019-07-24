const fs = require('fs');

let savename = __dirname + '/resulthtml.h';
let filename = __dirname + '/../result.html';
let result = 
`const std::string resulthtml = 
`;
let text = fs.readFileSync(filename).toString().replace(/\\/g, '\\\\').replace(/"/g, '\\"').split(/\r?\n/);
for (let i = 0; i < text.length; i ++ )
    result += '"' + text[i] + '\\n"\n';
fs.writeFileSync(savename, result + ';');