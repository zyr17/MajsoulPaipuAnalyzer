#include <bits/stdc++.h>
#include "lib/CJsonObject.hpp"

#ifdef linux
    #include <dirent.h>
#endif

#define JSONBUFFERSIZE 1048576

#define PAUSE {std::cout << "Press Enter to exit.\n";getchar();}

#define DEBUG

#ifdef DEBUG
    #define CHECKANALYZENAME
    //#define MATCHDATAOUTPUT
    //#define SAVEMATCHDATASTEP 11
    #define OutputVector(vec) {for (auto &i: vec) std::cout << i << ' '; std::cout << std::endl;}
#endif

using namespace neb;

namespace PA{

//对局用户数据
class MatchPlayerData{
public:
    const int ANKANNUM = -9;
    int get, score, reach;
    //不记录从谁那里鸣牌；对于暗杠，在四张牌后添加数字ANKANNUM表示
    ////TODO: 可以添加数字-1~-4表示该鸣牌确定了包牌
    std::vector<std::vector<int>> show;
    //table中数字+Consts.TILESYS表示该牌被鸣牌
    std::vector<int> hand, table;
    MatchPlayerData();
    void clear();
    CJsonObject tojson();
};

//常量存放
class Consts{
private:
    std::map<std::string, int> tile2nummap;
    static int chitoishanten(MatchPlayerData &pdata);
    static int kokushishanten(MatchPlayerData &pdata);
public:
    static const int TILENUM = 38;
    static const int TILESYS = 64;
    const std::string num2tile[TILENUM] = {
        "1m", "2m", "3m", "4m", "5m", "0m", "6m", "7m", "8m", "9m", 
        "1p", "2p", "3p", "4p", "5p", "0p", "6p", "7p", "8p", "9p", 
        "1s", "2s", "3s", "4s", "5s", "0s", "6s", "7s", "8s", "9s", 
        "1z", "2z", "3z", "4z", "5z", "6z", "7z", "?x"
    };
    Consts();
    int tile2num(const std::string &key);
    static long long strptime(const std::string &str);
    static std::vector<std::string> split(std::string &str, char c);
    static void changevec(std::vector<int> &vec, int pos, int replace = INT_MAX);
    //返回一个包含三个元素的数组，0为和牌人得点，1为庄家失点，2为闲家失点。庄家和牌1为0，闲家荣牌12一样。
    static std::vector<int> calctensu(int fu, int han, int honba, int kyoutaku, bool oya, bool tsumo);
    static int getrank(std::vector<int> points, int who, int initial = 0);
    static int calcshanten(MatchPlayerData &pdata, bool chitoikokushi = true);
};

//数据分析
class AnalyzeData{
public:
    int me;
    std::vector<long long> basedata;
    std::vector<std::vector<long long>> yakudata;
    static const int BASEDATALENGTH = 48;
    const std::string num2basedataname[BASEDATALENGTH] = {
        //0
        "TOTALGAME", "#1", "#2", "#3", "#4", 
        "ALBAO1", "ALNI1", "ALBI4", "ALMULTITIME", "TOTALROUND", 
        //10
        "HULE", "ZIMO", "FANGCHONG", "REACH", "FULU1", 
        "FULU2", "FULU3", "FULU4", "FULU1HULE", "FULU2HULE", 
        //20
        "FULU3HULE", "FULU4HULE", "FULU1FANGCHONG", "FULU2FANGCHONG", "FULU3FANGCHONG", 
        "FULU4FANGCHONG", "HULEPOINT", "FANGCHONGPOINT", "HULESUDIAN", "FANGCHONGSUDIAN", 
        //30
        "DAMAHULE", "CHONGLEDAMA", "DAMAHULEPOINT", "CHONGLEDAMAPOINT", "HULE3900+",
        "HULE7700+", "HULE11600+", "FANGCHONG3900+", "FANGCHONG7700+", "FANGCHONG11600+",
        //40
        "HULECIRCLE", "FANGCHONGMYCIRCLE", "FANGCHONGHISCIRCLE", "FANGCHONGSHANTEN", "AL#1", 
        "AL#2", "AL#3", "AL#4"
    };
    static const int YAKUNUMBER = 56;
    static const int YAKUDATALENGTH = 1;
    const std::string num2yakudataname[YAKUDATALENGTH] = {
        "HULE"
    };
    /*
    {
        int basenum = 0;
#ifdef CHECKDATANAME
        assert(adata.num2basedataname[basenum] == "TOTALGAME");
#endif
    }
    */
    std::vector<double> result;
    static const int RESULTNAMELENGTH = 33;
    const std::string num2resultnameE[RESULTNAMELENGTH] = {
        //0
        "#1", "#2", "#3", "#4", "AL keep #1", 
        "AL reach #1", "AL prevent from #4", "AL more than 2 times", "hule rate", "tsumo rate",
        //10
        "fangchong rate", "reach rate", "fulu rate", "dama hule rate", "fangchong dama rate", 
        "fulu hule rate", "fulu fangchong rate", "hule point", "fangchong point", "hule sudian",
        //20
        "fangchong sudian", "dama hule point", "fangchong dama point", "hule 3900+ rate", "hule 7700+ rate",
        "hule 11600+ rate", "fangchong 3900+ rate", "fangchong 7700+ rate", "fangchong 11600+ rate", "hule circle",
        //30
        "fangchong my circle", "fangchong his circle", "fangchong shanten"
    };
    AnalyzeData();
    void calcresult();
};

//对局数据
class MatchData{
private:
    void IPutKyoutaku();
    void IDiscardTile(std::string &str);
    void IDealTile(std::string &str);
    void IChiPengGang(std::string &str);
    void IAnGangAddGang(std::string &str);
    void ILiuJu(std::string &str);
    void IHule(std::string &str);
    void INoTile(std::string &str);
    void IFinalScore(std::string &str);

public:
    int kyoutaku, honba, now, east, remain, nowround, needkyoutaku;
    std::vector<int> dora;
    std::vector<MatchPlayerData> data;
    AnalyzeData *analyzedata;
    MatchData();
    CJsonObject tojson();
    void clear();
    void INewGame(CJsonObject &record);
    void INewRound(CJsonObject &record);
    void action(std::vector<std::string> &strvec);
    void action(std::string &actstr);
};

//牌谱个人数据分析
////在实现天凤数据分析后可以用天鳳の牌譜解析プログラム验证正确性
class PaipuAnalyzer{
private:
    CJsonObject filter;
    MatchData matchdata;
    void initializeresult();
    bool filterinclude(const CJsonObject &p, CJsonObject &f, bool emptyresult = true);
    bool filterexclude(const CJsonObject &p, CJsonObject &f);
    bool filtercheck(const CJsonObject &paipu);

public:
    AnalyzeData *analyzedata;
    PaipuAnalyzer(std::string filterstr = "{}");
    PaipuAnalyzer(const CJsonObject &filterjson);
    void setfilter(std::string &filterstr);
    void setfilter(const CJsonObject &filterjson);
    void clearresult();
    int analyze(std::vector<std::string> &paipus);
    int analyze(std::vector<CJsonObject> &paipus);
    bool analyze(std::string &paipu);
    bool analyze(CJsonObject &paipu);
};

CJsonObject ReadJSON(std::string filename);
std::vector<CJsonObject> ReadLineJSON(std::string filename, std::string prefix = "", std::string suffix = "");

}

namespace MatchDataCompare{
    bool tilestr(std::string &inx, std::string &iny);
    void playerdata(CJsonObject &data1, CJsonObject &data2);
    void matchdata(CJsonObject &md1, CJsonObject &md2);
    void main();
}