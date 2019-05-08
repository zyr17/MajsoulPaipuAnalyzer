#ifndef _ALGO_H
#define _ALGO_H

#include "header.h"
#include "analyzer.h"

namespace Algo{

    #ifdef _WIN32
        std::string UTF82GBK(const std::string &str);
    #endif

    long long strptime(const std::string &str);
    std::vector<std::string> split(const std::string &str, char c);
    void changevec(std::vector<int> &vec, int pos, int replace = INT_MAX);
    //返回一个包含三个元素的数组，0为和牌人得点，1为庄家失点，2为闲家失点。庄家和牌1为0，闲家荣牌12一样。
    std::vector<int> calctensu(int fu, int han, int honba, int kyoutaku, bool oya, bool tsumo);
    int getrank(std::vector<int> points, int who, int initial = 0);
    std::vector<int> pdata2tilevec(const PA::MatchPlayerData &pdata);

    int chitoishanten(const std::vector<int> &bu);
    int kokushishanten(const std::vector<int> &bu);
    int calcshanten(const PA::MatchPlayerData &pdata, bool chitoikokushi = true);
    void testshanten();

    //设置读取文件时的前缀，从config.json中获得，调试时用于定位数据文件位置。
    //需要保证所有文件相关均通过下列函数，不要自行使用fopen access等！
    extern std::string dataprefix;
    int Access(const char *filename, int mode);
    CJsonObject ReadJSON(const std::string &filename);
    std::vector<CJsonObject> ReadLineJSON(const std::string &filename, const std::string &prefix = "", const std::string &suffix = "");

    void getconsolesize(int &row, int &col);
    //获取UTF-8字符串的宽度；ASCII字符认为宽度为1，其余为2。出现非ASCII1宽度字体会误判，但是目前无影响。
    int getdisplaywidth(const std::string &str);
    int getdisplaywidth(long long num);
    //假设浮点数均采用正常输出，且保留4位小数。如果为inf/nan等则长度为1 (应输出'-')
    int getdisplaywidth(double num);

    std::vector<int> calctenpai(const PA::MatchPlayerData &pdata);
    //以当前手牌给出听牌质量。-1:未听牌 0:愚形 1:好形。好形包括不考虑自己手牌、副露外牌时听6张及以上的；以及听字牌的
    int tenpaiquality(const PA::MatchPlayerData &pdata);
    void testtenpai();

    int countdora(const PA::MatchPlayerData &pdata, const std::vector<int> &dora);
    //当听牌多面时，存在听幺九牌即认为无断幺役
    bool istanyao(const PA::MatchPlayerData &pdata);
    //存在不平和听牌即认为无平和役
    bool ispinfu(const PA::MatchPlayerData &pdata);
    bool isyakuhai(const PA::MatchPlayerData &pdata, int wind, int round);

}

#endif