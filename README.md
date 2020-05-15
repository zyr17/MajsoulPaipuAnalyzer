# MajsoulPaipuAnalyzer

自制雀魂牌谱分析工具。支持国服、日服、国际服。提供Windows，Linux，macOS版本。

目前仅支持四人麻将牌谱分析，分析项目参考天鳳の牌譜解析プログラム的项目实现。目前实现了除被鸣牌和门清听牌大类外，剩余大类中的大部分数据。

在结果展示网页中带有分析天凤凤凰桌牌谱得到的数据，可以用于和自己的数据进行比较。

[更新内容说明文件](doc/release-notes.txt)

## 免责声明
该工具为个人制作，未对雀魂代码逻辑进行改动，但是为了获取牌谱数据使用了极少量前端代码的API，包括**获取用户牌谱列表**和**获取指定牌谱内容**。所有相关操作代码均在[browseinject.js](lib/majsoul/browseinject.js)中，工具不会代替用户执行任何其他交互性操作。请自行判断使用该工具的风险。如果使用该工具或（不存在的）该工具的衍生物产生的一切后果作者均不会承担任何责任。

## 下载

APPVeyor下载最新版本：

Windows: [![Build status](https://ci.appveyor.com/api/projects/status/fyirnuhsunq73brc?svg=true)](https://ci.appveyor.com/project/zyr17/majsoulpaipuanalyzer) Linux: [![Build status](https://ci.appveyor.com/api/projects/status/i22ex7a644qasmxx?svg=true)](https://ci.appveyor.com/project/zyr17/majsoulpaipuanalyzer-ko5wy)

由于APPVeyor无法编译macOS下软件，因此macOS版本自行使用苹果虚拟机编译，在网盘和releases提供下载。

[releases](https://github.com/zyr17/MajsoulPaipuAnalyzer/releases)中可下载已经编译好的文件。

[百度网盘](https://pan.baidu.com/s/1mu31kzaF7aHkY2IeBjCLyg) 提取码：k34a

## 界面

工具分为两部分，MajsoulPaipuCrawler和PaipuAnalyzer。

MajsoulPaipuCrawler使用Electron编写，用于从雀魂收集牌谱并转换成自用牌谱格式。

![MajsoulPaipuCrawler](doc/img/MPC.png)

PaipuAnalyzer使用C++编写，分析收集到的牌谱数据并在命令行窗口中展示数据。

![PaipuAnalyzer](doc/img/PA.png)

同时PaipuAnalyzer会生成用于查看和比较数据的网页PaipuAnalyzeResult.html。

![PaipuAnalyzeResult.html](doc/img/PaipuAnalyzeResult.png)

## 安装

### 编译/测试环境

Windows: Windows 10, nodejs v10.13.0, MinGW-w64 8.1.0

Linux: Ubuntu 18.04 x64, nodejs v8.1.0, g++ 7.4.0

macOS: 10.14 (VMware Workstation 15), nodejs v12.4.0, g++ 4.2.1(映射到clang-1001.0.46.4)

AppVeyor环境请参考[其网站](https://www.appveyor.com/docs/build-environment/)

### 依赖

需要npm, g++, cmake, make/mingw32-make

### 安装脚本

Windows脚本使用PowerShell。路径不能出现中文。

#### Windows
    
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

#### Linux

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

#### macOS

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

## 使用

[这里](doc/README.txt)提供了简易使用说明。

### 牌谱获取及转换

首先运行Electron应用MajsoulPaipuCrawler。程序会显示雀魂窗口和SimpleMahjong窗口。由于技术原因，第三方账户登陆可能出现问题，请使用网页-登陆专用窗口登陆后刷新雀魂窗口。默认为国服，如果要切换到其他服请在网页菜单中选择。

在成功登陆后，可以选择**牌谱-查看已有牌谱**情报。如果加载未完成会弹出错误对话框，否则会开始自动收集账户的牌谱基本信息，并展示收集到的牌谱类型。

在确认所有牌谱已井在牌谱情报中展示后，点击**牌谱-下载&转换牌谱**来对获取到的牌谱进行下载和格式转换。转换会跳过三麻和比赛牌谱。转换进度显示在SimpleMahjong窗口的左上角。

每个账户会有自己的独立ID，这个ID和加好友时候的那个ID是不一样的，游戏里大概不能直接看到？如果登陆多个账户，会将每个账户的资料按照ID分别存储，不会混在一起。牌谱下载及转换内容存储于data文件夹，避免重复下载和转换，每次进行下载仅会尝试下载转换未下载的牌谱。存储方式不再赘述，感兴趣的人翻一翻大概就能明白了。

### 牌谱分析

完成牌谱收集后即可关闭Electron应用。然后使用PaipuAnalyzer进行牌谱分析。

在进行分析前，请确认data/config.json文件。在该文件中存储了牌谱分析的各种配置。关于配置文件各项内容的含义请参照[这里](doc/config.md)。

确认配置无误后，运行PaipuAnalyzer即可得到结果。部分统计规则和雀魂官方有所出入(我不知道官方是怎么算的，反正结果和官方差了一点)。一些项目的计算方式和特殊说明请参照[这里](doc/result.md)。

## 已知问题

由于雀魂官方的牌谱查看只支持1000个牌谱，而MajsoulPaipuCrawler基于官方操作，如果超过了1000个牌谱目前没有办法获取。如果有好办法请至[issue](https://github.com/zyr17/MajsoulPaipuAnalyzer/issues/5)中指导作者。

由于作者统计和概率论水平不行，目前安定段位计算为有偏估计。目前安定段位及置信区间的计算参照[这里](doc/stable-rank.md)，欢迎数学好的大佬们在[issue](https://github.com/zyr17/MajsoulPaipuAnalyzer/issues/4)中或其他联系渠道提供正确的安定段位和置信区间的计算方式。

Ubuntu高版本中可能会出现GUI将可执行文件当做动态链接库的情况。目前没有找到解决方法，请使用Terminal执行。

如果运行Electron时出现游戏界面黑屏、白屏、崩溃等问题，有可能是显卡对WebGL的支持问题，可以尝试将resources/app/main.js文件第24行附近的app.isableHardwareAcceleration双斜杠删去，关闭硬件加速运行。

由于设计时从未参加过比赛场，发现问题后开了个比赛场测试并发现比赛场数据和想象中有较大出入，目前会直接忽略比赛场牌谱数据，在以后做了相关实现后加入。

由于本人较菜没有上圣没法打王座，相关牌谱可能会出现bug。

没钱买苹果电脑，所以用VMware做了个黑苹果编译测试，安装系统版本10.14。在真机上可能会出现问题。

## 联系

如果发现任何bug，可以通过[github](https://github.com/zyr17/MajsoulPaipuAnalyzer/issues)或是jzjqz17@gmail.com说明。

## 致谢

该项目使用或曾经使用了这些项目的资源，感谢他们。

[CJsonObject](https://github.com/Bwar/CJsonObject)

[mt19937ar-MersenneTwister-JS](https://github.com/neetsdkasu/mt19937ar-MersenneTwister-JS)

[js-sha512](https://github.com/emn178/js-sha512)

[牌面图像](https://mj-king.net/sozai/)

[wsHook](https://github.com/skepticfx/wshook)

[wssip](https://github.com/nccgroup/wssip)
