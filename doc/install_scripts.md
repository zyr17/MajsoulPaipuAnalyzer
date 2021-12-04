# 安装脚本

Windows脚本使用PowerShell。路径不能出现中文。

## Windows
    
    git clone --recurse-submodules https://github.com/zyr17/MajsoulPaipuAnalyzer
    cd MajsoulPaipuAnalyzer
    npm install
    npm run-script package-win
    mkdir bin
    mkdir bin/release
    cd bin/release
    cmake ../.. -DCMAKE_BUILD_TYPE=Release -G "MinGW Makefiles"
    mingw32-make
    cd ../..
    cp -r dist/MajsoulPaipuCrawler-win32-x64 result
    cp bin/release/PaipuAnalyzer.exe result/
    mkdir result/data
    cp config.json result/
    cp PAADData.json result/
    cp doc/README.txt result/
    cp doc/release-notes.txt result/
    cp -r i18n/ result/

## Linux

    git clone --recurse-submodules https://github.com/zyr17/MajsoulPaipuAnalyzer
    cd MajsoulPaipuAnalyzer
    npm install
    npm run-script package-linux
    mkdir bin
    mkdir bin/release
    cd bin/release
    cmake ../.. -DCMAKE_BUILD_TYPE=Release
    make
    cd ../..
    cp -r dist/MajsoulPaipuCrawler-linux-x64 result
    cp bin/release/PaipuAnalyzer result/
    mkdir result/data
    cp config.json result/
    cp PAADData.json result/
    cp doc/README.txt result/
    cp doc/release-notes.txt result/
    cp -r i18n/ result/

## macOS

    git clone --recurse-submodules https://github.com/zyr17/MajsoulPaipuAnalyzer
    cd MajsoulPaipuAnalyzer
    npm install
    npm run-script package-darwin
    mkdir bin
    mkdir bin/release
    cd bin/release
    cmake ../.. -DCMAKE_BUILD_TYPE=Release
    make
    cd ../..
    cp -r dist/MajsoulPaipuCrawler-darwin-x64 result
    cp bin/release/PaipuAnalyzer result/
    mkdir result/data
    cp config.json result/
    cp PAADData.json result/
    cp doc/README.txt result/
    cp doc/release-notes.txt result/
    cp -r i18n result/

result中即为结果。
