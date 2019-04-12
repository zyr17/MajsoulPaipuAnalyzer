# MajsoulPaipuAnalyzer

自制雀魂牌谱分析工具，拥有：体积庞大，使用复杂，功能单一，界面垃圾，懒癌发作，代码混乱的特点。

目前仅支持四人麻将牌谱分析，分析项目从天鳳の牌譜解析プログラム中选了很少量感兴趣+好实现的。出于各种原因，目前分析项目名称使用幼儿园英语+拼音+罗马音混合的模式，更加正常的分析项目名称将在之后完成。

## 安装

### 编译/测试环境

Windows: Windows 10, nodejs v10.13.0, MinGW-W64 8.1.0

Linux: Ubuntu 18.04 x64, nodejs v8.1.0, g++ 7.4.0

### 二进制文件

Windows: [![Build status](https://ci.appveyor.com/api/projects/status/fyirnuhsunq73brc?svg=true)](https://ci.appveyor.com/project/zyr17/majsoulpaipuanalyzer) Linux: [![Build status](https://ci.appveyor.com/api/projects/status/i22ex7a644qasmxx?svg=true)](https://ci.appveyor.com/project/zyr17/majsoulpaipuanalyzer-ko5wy)

也可在[releases](https://github.com/zyr17/MajsoulPaipuAnalyzer/releases)中下载已经编译好的文件。提供的可执行文件在Windows 10和Ubuntu18.04编译及测试。

### 依赖

需要npm, g++, cmake, make/mingw32-make

### 安装

Windows脚本使用PowerShell。路径不能出现中文。

#### Windows:
    
    git clone https://github.com/zyr17/MajsoulPaipuAnalyzer
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
    cp config.json result/data

#### Linux:

    git clone https://github.com/zyr17/MajsoulPaipuAnalyzer
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
    cp config.json result/data/

result中即为结果。

## 使用

### 牌谱获取及转换

首先运行Electron应用MajsoulPaipuCrawler。程序会显示雀魂窗口和SimpleMahjong窗口。为了能够分析bug，默认两个页面会打开开发者工具。由于 ~~懒~~ 技术原因，首次登陆雀魂以后需要刷新界面(杂项-刷新)才能正常解析牌谱。

在成功登陆后，可以选择牌谱-查看已有牌谱情报。如果没有出现错误那么说明已经可以开始获取牌谱。然后进入牌谱界面，此时再次查看牌谱情报应该会成功获取到约10个牌谱情报。由于雀魂牌谱采用每次加载约10个且滚到牌谱页面底部才触发加载的方式，为了获取全部牌谱需要将滚动条滚到最底部。可以采用鼠标拖动滚动条并在底部抽搐的方式。

在确认所有牌谱已被加载出来后，点击牌谱-下载&转换牌谱来对获取到的牌谱进行下载和格式转换。转换会跳过三麻牌谱。转换进度显示在SimpleMahjong窗口的左上角。需要注意的是，由于 ~~真的是~~ 技术原因，最近几天打完的牌谱目前并不能成功获取，请过几天再次尝试。

每个账户会有自己的独立ID，这个ID和加好友时候的那个ID是不一样的，游戏里大概不能直接看到？如果登陆多个账户，会将每个账户的资料按照ID分别存储，不会混在一起。牌谱下载及转换内容存储于data文件夹，避免重复下载和转换，每次进行下载仅会尝试下载转换未下载的牌谱。存储方式不再赘述，感兴趣的人翻一翻大概就能明白了。

### 牌谱分析

完成牌谱收集后即可关闭Electron应用。然后使用PaipuAnalyzer进行牌谱分析。

在进行分析前，请确认data/config.json文件。在该文件中存储了牌谱分析的各种配置。关于配置文件各项内容的含义请参照[这里](doc/config.md)。

确认配置无误后，运行PaipuAnalyzer即可得到结果。结果由三部分组成，每部分由横线隔开。第一部分是确认牌谱来源和用户ID。目前只能解析雀魂的牌谱。第二部分是一些内置统计结果，用来计算分析数据。这些内容为调试使用，用来确认统计项是否出错如果有想计算但是没有在分析结果中列出来的数据(如不同副露数下的结果)可以自行计算。第三部分是分析结果，名称使用英语+拼音+罗马音混合而成。由于 ~~懒~~ 为了简便，一些统计规则和雀魂官方有所出入(我不知道官方是怎么算的，反正结果和官方差了一点)。每一项的意义及特殊说明请参照[这里](doc/result.md)。

## 已知问题

Ubuntu高版本中可能会出现GUI将可执行文件当做动态链接库的情况。目前没有找到解决方法，请使用Terminal执行。

如果运行Electron时出现游戏界面黑屏、白屏、崩溃等问题，有可能是显卡对WebGL的支持问题，可以尝试将resources/app/main.js文件第21行的双斜杠删去，关闭硬件加速运行。

由于计算向听数 ~~懒得实现~~ 实现较为复杂，目前放铳向听数永远为3。将在之后修复。

由于本人较菜没有上圣没法打王座（虽然好像上了也没人打），同时没有参加过任何比赛场，所以这两块数据有所缺失，可能会出现bug。如果有大佬能够提供比赛场或者王座场的牌谱链接的话非常感谢。

## 联系

如果发现任何bug，可以通过github或是jzjqz17@gmail.com说明。

## 致谢

该项目使用或曾经使用了这些项目的代码，感谢他们。

[wsHook](https://github.com/skepticfx/wshook)

[CJsonObject](https://github.com/Bwar/CJsonObject)

[wssip](https://github.com/nccgroup/wssip)