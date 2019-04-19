#ifndef _ANALYZER_H
#define _ANALYZER_H

#include "header.h"
#include "tiles.h"

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
        int fulu() const;
        CJsonObject tojson();
    };

}

#include "algo.h"
#include "i18n.h"

namespace PA{

    //计算数据及结果的名称
    namespace AnalyzeResultName{

        //自己立直时，判断是愚形还是好形
        const int REACHTYPENUM = 3;
        const std::string num2reachtype[REACHTYPENUM] = {
            "GOOD", "BAD", "ALL"
        };

        //自己立直时相关的数据
        const int REACHBASEDATANUM = 36;
        const std::string num2reachbasedata[REACHBASEDATANUM] = {
            //0
            "REACH", "CIRCLE", "REACHDECLEARRON", "TANYAO", "PINFU", 
            "DORA0", "DORA1", "DORA2", "DORA3", "DORA4", 
            //10
            "DORA5", "DORA6", "DORA7", "DORA8", "DORA9", 
            "DORA10", "DORA11", "DORA12", "DORA13", "DORA14", 
            //20
            "DORA15", "DORA16", "DORA17", "DORA18", "DORA19", 
            "DORA20", "DORA21", "DORA22", "DORA23", "DORA24", 
            //30
            "DORA25", "DORA26", "#1", "#2", "#3", 
            "#4"
        };
        
        //有人和牌时对于一副手牌类型的判断；12个之后为将某些类型求和
        const int HULEHANDTYPENUM = 18;
        const std::string num2hulehandtype[HULEHANDTYPENUM] = {
            //0
            "REACHGOOD", "REACHBAD", 
            //2
            "DAMA", "FULU1", "FULU2", "FULU3", "FULU4", 
            //7
            "NTDAMA", "NTFULU1", "NTFULU2", "NTFULU3", "NTFULU4", 
            //12
            "FULU", "NTFULU", "ALLFULU", "ALLDAMA", "ALLREACH", "ALL"
        };

        //有人和牌时需要考虑的基础数据类型，并按照玩家不同手牌类型统计
        const int HULEBASEDATANUM = 44;
        const std::string num2hulebasedata[HULEBASEDATANUM] = {
            //0
            "HULE", "FANGCHONG", "ZIMO", "BEIZIMO", "HULEPOINT", 
            "HULESUDIAN", "FANGCHONGPOINT", "FANGCHONGSUDIAN", "ZIMOPOINT", "ZIMOSUDIAN", 
            //10
            "BEIZIMOPOINT", "BEIZIMOSUDIAN", "HULE3900+", "HULE7700+", "HULE11600+", 
            "FANGCHONG3900+", "FANGCHONG7700+", "FANGCHONG11600+", "HULECIRCLE", "FANGCHONGMYCIRCLE", 
            //20
            "ZIMOCIRCLE", "BEIZIMOMYCIRCLE", "DORATIME", "URATIME", "AKATIME", 
            "ZHUANGHULE", "ZHUANGZIMO", "ZHAZHUANG", "ZHAZHUANGPOINT", "CHONGLEZHUANG", 
            //30
            "CHONGLEZHUANGPOINT", "FANGCHONGHISCIRCLE", "BEIZIMOHISCIRCLE", "CHONGLEDAMA", "CHONGLEFULU1", 
            "CHONGLEFULU2", "CHONGLEFULU3", "CHONGLEFULU4", "CHONGLEDAMAPOINT", "CHONGLEFULU1POINT", 
            //40
            "CHONGLEFULU2POINT", "CHONGLEFULU3POINT", "CHONGLEFULU4POINT", "ZHUANGMEIHU"
        };

        //和牌时统计的役的分布
        const int HULEYAKUBASEDATANUM = 3;
        const std::string num2huleyakubasedata[HULEYAKUBASEDATANUM] = {
            "HULEYAKU", "CHONGLEYAKU", "BEIZIMOYAKU"
        };

        //基础数据
        const int BASEDATANUM = 33;
        const std::string num2basedata[BASEDATANUM] = {
            //0
            "TOTALGAME", "#1", "#2", "#3", "#4", 
            "ALBAO1", "ALNI1", "ALBI4", "ALMULTITIME", "TOTALROUND", 
            //10
            "REACH", "FULU1", "FULU2", "FULU3", "FULU4", 
            "FANGCHONGSHANTEN0", "FANGCHONGSHANTEN1", "FANGCHONGSHANTEN2", "FANGCHONGSHANTEN3", "FANGCHONGSHANTEN4", 
            //20
            "FANGCHONGSHANTEN5", "FANGCHONGSHANTEN6", "AL#1", "AL#2", "AL#3", 
            "AL#4", "NORMALLIUJU", "LIUJUTENPAI", "LIUJUNOTEN", "LIUJUTENPAIPOINT",
            //30
            "LIUJUNOTENPOINT", "LIUJUPOINT", "ALLPOINT"
        };

        //役名称
        const int YAKUDATANUM = 55;
        const std::string num2yakudata[YAKUDATANUM] = {
            //0
            "ZIMO", "REACH", "YIPATSU", "CHANKAN", "RINSHAN", 
            "HAITEI", "HOUTEI", "PINFU", "TANYAO", "YIPEIKOU", 
            //10
            "JIFUDON", "JIFUNAN", "JIFUSHA", "JIFUPEI", "BAFUDON", 
            "BAFUNAN", "BAFUSHA", "BAFUPEI", "HAKU", "HATSU", 
            //20
            "CHUN", "WREACH", "CHITOITSU", "CHANTA", "YITSU", 
            "SANSHOKU", "SANSHOKUDOUKO", "SANKANTSU", "TOITOI", "SANANKO", 
            //30
            "SHOUSANGEN", "HONROUTOU", "RYANPEIKOU", "JUNCHAN", "HONYITSU", 
            "CHINYITSU", "RENHOU", "TENHOU", "CHIHOU", "DAISANGEN", 
            //40
            "SUANKO", "SUANKOTANKI", "TSUYISOU", "RYOUYISOU", "CHINROUTOU", 
            "CHUREN", "JUNSEICHUREN", "KOKUSHI", "KOKUSHIJUSAN", "DAISUSHI", 
            //50
            "SHOUSUSHI", "SUKANTSU", "DORA", "URA", "AKA"
        };

        //分析结果
        const int RESULTNAMENUM = 101;
        //上次结果：90
        //增加后需要变动：以下分类数据；语言json；加入计算；图片上标记已计算
        //               1            1        0        0
        const std::string num2result[RESULTNAMENUM] = {
            //0
            "#1R", "#2R", "#3R", "#4R", "AL#1#1R", 
            "AL#234#1R", "AL#4#123R", "ALAL+R", "HULER", "ZIMOR",
            //10
            "CHONGR", "REACHR", "FULUR", "DAMAHULER", "CHONGLEDAMAR", 
            "FULUHULER", "FULUCHONGR", "HULEP", "CHONGP", "HULESU",
            //20
            "CHONGSU", "DAMAHULEP", "CHONGLEDAMAP", "HULE3900+R", "HULE7700+R",
            "HULE11600+R", "CHONG3900+R", "CHONG7700+R", "CHONG11600+R", "HULECC",
            //30
            "CHONGMYCC", "CHONGHISCC", "CHONGSHANTEN", "FULUHULECC", "MENQINGHULECC", 
            "REACHINHULER", "TANYAOHULER", "PINFUHULER", "CHITOIHULER", "TOITOIHULER", 
            //40
            "RANSHOUHULER", "AKAA", "DORAA", "URAA", "ALLDORAA", 
            "YIPATSUHULER", "OYAHULER", "CHONGLEREACHR", "CHONGLEPINFUR", "CHONGLECHITOIR", 
            //50
            "CHONGLETOITOIR", "CHONGLETANYAOR", "CHONGLERANSHOUR", "REACHINCHONGR", "CHONGLEOYAR", 
            "CHONGLEYIPATSUR", "CHONGLEFULUR", "BEIZIMOR", "BEIZIMOP", "BEIZIMOMYCC", 
            //60
            "ZHAZHUANGR", "ZHAZHUANGP", "REACHHULEP", "REACHHULESU", "REACH3900+R", 
            "REACH7700+R", "REACH11600+R", "REACHCC", "REACHPINFUR", "REACHTANYAOR", 
            //70
            "REACHDORA2+R", "REACHDORA3+R", "REACHDORAA", "FIRSTREACHR", "ZIMOINREACHHULER", 
            "REACHGOODR", "REACHGOODHULER", "REACHGOODHULEP", "REACHGOODCHONGR","REACHGOODCHONGP", 
            //80
            "REACHGOODPROFIT", "REACHBADR", "REACHBADHULER", "REACHBADHULEP", "REACHBADCHONGR", 
            "REACHBADCHONGP", "REACHBADPROFIT", "REACHPROFIT", "CHONGINREACHR", "HULEINREACHR",
            //90
            "LIUJUTENPAIR", "LIUJUINP", "LIUJUNOTENR", "LIUJUOUTP", "LIUJUPROFIT", 
            "#A", "TOTALROUND", "TOTALGAME", "ROUNDPROFIT", "GAMEPROFIT", 
            //100
            "LIUJUR"
        };
        //综合数据
        const int OVERVIEWRESULTNUM = 15;
        const std::string overviewresult[OVERVIEWRESULTNUM] = {
            "TOTALGAME", "TOTALROUND", "#1R", "#2R", "#3R", 
            "#4R", "#A", "HULER", "ZIMOR", "CHONGR", 
            "REACHR", "FULUR", "LIUJUTENPAIR", "GAMEPROFIT", "ROUNDPROFIT"
        };
        //和牌相关数据
        const int HULERESULTNUM = 25;
        const std::string huleresult[HULERESULTNUM] = {
            "HULER", "ZIMOR", "DAMAHULER", "FULUHULER", "HULEP", 
            "HULESU", "DAMAHULEP", "HULE3900+R", "HULE7700+R", "HULE11600+R",
            "HULECC", "FULUHULECC", "MENQINGHULECC", "REACHINHULER", "TANYAOHULER", 
            "PINFUHULER", "CHITOIHULER", "TOITOIHULER", "RANSHOUHULER", "AKAA", 
            "DORAA", "URAA", "ALLDORAA", "YIPATSUHULER", "OYAHULER"
        };
        //放铳相关数据
        const int CHONGRESULTNUM = 27;
        const std::string chongresult[CHONGRESULTNUM] = {
            "CHONGR", "CHONGLEDAMAR", "FULUCHONGR", "CHONGP", "CHONGSU", 
            "CHONGLEDAMAP", "CHONG3900+R", "CHONG7700+R", "CHONG11600+R", "CHONGMYCC", 
            "CHONGHISCC", "CHONGSHANTEN", "CHONGLEREACHR", "CHONGLEPINFUR", "CHONGLECHITOIR", 
            "CHONGLETOITOIR", "CHONGLETANYAOR", "CHONGLERANSHOUR", "REACHINCHONGR", "CHONGLEOYAR", 
            "CHONGLEYIPATSUR", "CHONGLEFULUR", "BEIZIMOR", "BEIZIMOP", "BEIZIMOMYCC", 
            "ZHAZHUANGR", "ZHAZHUANGP"
        };
        //立直相关数据
        const int REACHRESULTNUM = 29;
        const std::string reachresult[REACHRESULTNUM] = {
            "REACHR", "REACHHULEP", "REACHHULESU", "REACH3900+R", "REACH7700+R", 
            "REACH11600+R", "REACHCC", "REACHPINFUR", "REACHTANYAOR", "REACHDORA2+R", 
            "REACHDORA3+R", "REACHDORAA", "FIRSTREACHR", "HULEINREACHR", "CHONGINREACHR", 
            "ZIMOINREACHHULER", "REACHGOODR", "REACHGOODHULER", "REACHGOODHULEP", "REACHGOODCHONGR", 
            "REACHGOODCHONGP", "REACHGOODPROFIT", "REACHBADR", "REACHBADHULER", "REACHBADHULEP", 
            "REACHBADCHONGR", "REACHBADCHONGP", "REACHBADPROFIT", "REACHPROFIT"
        };
        //流局相关数据
        const int LIUJURESULTNUM = 6;
        const std::string liujuresult[LIUJURESULTNUM] = {
            "LIUJUR", "LIUJUTENPAIR", "LIUJUINP", "LIUJUNOTENR", "LIUJUOUTP", 
            "LIUJUPROFIT"
        };
        //副露相关数据。需要做不同副露数量区分
        const int FULURESULTNUM = 4;
        const std::string fuluresult[FULURESULTNUM] = {
            "FULUR", "FULUHULER", "FULUCHONGR", "FULUHULECC"
        };
        //AL数据
        const int ALRESULTNUM = 4;
        const std::string alresult[ALRESULTNUM] = {
            "AL#1#1R", "AL#234#1R", "AL#4#123R", "ALAL+R"
        };
    }

    //数据分析
    class AnalyzeData{
    private:
        void outputonerect(const std::string &title, const std::string *res, const int length, int col);
        void makehanddata(std::vector<long long> &vec);
    public:
        int me;
        std::vector<long long> basedata;
        std::vector<std::vector<long long>> yakudata;
        std::vector<double> result;

        //[HULEBASEDATA, HULEHANDTYPE]
        std::vector<std::vector<long long>> hulebasedata;
        //[REACHBASEDATA, REACHTYPE]
        std::vector<std::vector<long long>> reachbasedata;
        //[HULEYAKUBASEDATA, YAKUDATA, HULEHANDTYPE]
        std::vector<std::vector<std::vector<long long>>> huleyakubasedata;

        AnalyzeData();
        int gethandtype(const MatchPlayerData &pdata);
        void calcresult();
        void outputresult();
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
    ////TODO: 在实现天凤数据分析后可以用天鳳の牌譜解析プログラム验证正确性
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

    void analyzemain(const std::string &dataf, const std::string &source, const std::string &id, CJsonObject &config);

}

#endif