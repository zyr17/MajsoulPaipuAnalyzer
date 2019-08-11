#ifndef _ANALYZER_H
#define _ANALYZER_H

#include "header.h"
#include "tiles.h"

#define BASENUM2VECEVAL(base, num, num2vec, str) {base = num; assert(num2vec[base] == str);}

namespace PA{

    //对局用户数据
    class MatchPlayerData{
    public:
        const int ANKANNUM = -9;
        int get, score, reach, reachrank;
        //不记录从谁那里鸣牌；对于暗杠，在四张牌后添加数字ANKANNUM表示
        ////TODO: 可以添加数字-1~-4表示该鸣牌确定了包牌
        std::vector<std::vector<int>> show;
        //table中数字+Consts.TILESYS表示该牌被鸣牌
        std::vector<int> hand, table;
        MatchPlayerData();
        void clear();
        int fulu() const;
        CJsonObject tojson() const;
    };

}

#include "algo.h"
#include "i18n.h"

namespace PA{


    //PAADData.json中数据：基础数据名称，统计量名称表达式，统计量展示分组信息。
    class AnalyzeResultName{
    private: 
        CJsonObject json;
        const std::string DESCRIPTION = "description";
    public:
        std::map<std::string, std::vector<std::string>> base;
        
        /*
        特殊情况：当result或resultgroup名称带有!时表示扩展统计，需要将该条根据规则拆成多条统计量
        使用规则：
                在一个名称中只能出现一个该特征；该特征可以指代连续自然数。
                使用时格式为!startnum!endnum!，表示形成所有之间的项目
                例：FULU!1!4!CIRCLE = FULU1CIRCLE FULU2CIRCLE FULU3CIRCLE FULU4CIRCLE
                同时，在表达式中也会用到若干!，对于某个项目，所有!均和统计量的数字同步
                例：FULU!1!4!CIRCLE: FULU_CIRCLE_FULU! / FULU_FULU_FULU!
                                        =
                    FULU1CIRCLE: FULU_CIRCLE_FULU1 / FULU_FULU_FULU1
                    FULU2CIRCLE: FULU_CIRCLE_FULU2 / FULU_FULU_FULU2 ... 
        */
        std::vector<std::string> result, resultexpr;

        const std::string ALLRESULT = "ALLRESULT";
        std::map<std::string, std::vector<std::string>> resultgroupmap;
        std::vector<std::string> resultgrouporder;

        AnalyzeResultName();
    };

    class AnalyzeData;

    //基本统计结果对应的项目分析结果
    struct AnalyzeExprNumberList{
        bool QQ;
        int startnum;
        std::vector<int> list;
        AnalyzeExprNumberList();
    };

    //对结果表达式求值
    class AnalyzeExpr{
    private:
        const int SPACE = -1;
        int oprprevilige[256];
        AnalyzeData *adata;

        void setoperator();
        std::string gettoken(const std::string &expr, int &k);
        void makecalc(std::vector<double> &num, std::vector<int> &opr);
        AnalyzeExprNumberList getnumberlist(const std::vector<std::string> &list, const std::string &str);
        double getdata(std::vector<long long> &data, std::string kw1, const std::vector<std::string> &kw1list);
        double getdata(std::vector<double> &data, std::string kw1, const std::vector<std::string> &kw1list);
        double getdata(std::vector<std::vector<long long>> &data, std::string kw1, const std::vector<std::string> &kw1list, std::string kw2, const std::vector<std::string> &kw2list);
        double getdata(std::vector<std::vector<std::vector<long long>>> &data, std::string kw1, const std::vector<std::string> &kw1list, std::string kw2, const std::vector<std::string> &kw2list, std::string kw3, const std::vector<std::string> &kw3list);
        /* 
        基本统计结果格式说明
        根据下划线拆分变量名称；由若干部分组成。基本统计来源_统计名称1_统计名称2_...
        基本统计来源自行在PAADData.json和该函数中统一。推荐取代码相关位置或者设计基本量数组名为名字
        例：BASE_AL#1 REACH_TANYAO_ALL HULE_ZHAZHUANG_DAMA HULEYAKU_BEIZIMOYAKU_SUANKO_FULU4
        特别的，对于副露，宝牌，顺位等由数字结尾的名称可用?代指所有名称和，??代指自乘和
        例：[#? = #1 + #2 + #3 + #4] [DORA?? = DORA0 * 0 + DORA1 * 1 + DORA2 * 2 + ... ]
        */
        //根据输入返回数值或基本统计结果
        double getvalue(const std::string &s);
    public:
        AnalyzeExpr(AnalyzeData *adata);
        double calcexpr(std::string expr);
    };

    //数据分析
    class AnalyzeData{
    private:
        AnalyzeExpr AE;

        void outputonerect(const std::string &title, const std::string &key, int col);
        void makehanddata(std::vector<long long> &vec);
    public:
        AnalyzeResultName ADN;
        
        std::vector<std::string> &num2reachbasedata;
        std::vector<std::string> &num2reachtype;
        std::vector<std::string> &num2fulubasedata;
        std::vector<std::string> &num2hulebasedata;
        std::vector<std::string> &num2yakudata;
        std::vector<std::string> &num2basedata;
        std::vector<std::string> &num2huleyakubasedata;
        std::vector<std::string> &num2fulutype;
        std::vector<std::string> &num2hulehandtype;
        std::vector<std::string> &num2floatdata;
        
        int me;
        std::vector<long long> basedata;
        std::vector<double> result;

        //[HULEBASEDATA, HULEHANDTYPE]
        std::vector<std::vector<long long>> hulebasedata;
        //[REACHBASEDATA, REACHTYPE]
        std::vector<std::vector<long long>> reachbasedata;
        //[HULEYAKUBASEDATA, YAKUDATA, HULEHANDTYPE]
        std::vector<std::vector<std::vector<long long>>> huleyakubasedata;
        //[FULUBASEDATA, FULUTYPE]
        std::vector<std::vector<long long>> fulubasedata;

        std::vector<double> floatdata;

        CJsonObject resultjson;

        AnalyzeData();
        int gethandtype(const MatchPlayerData &pdata);
        void calcresult();
        void outputbase();
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
        CJsonObject tojson() const;
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
        bool filterinclude(CJsonObject *p, CJsonObject *f, bool emptyresult = true);
        bool filterexclude(CJsonObject *p, CJsonObject *f);

    public:
        AnalyzeData *analyzedata;
        PaipuAnalyzer(std::string filterstr = "{\"include\":{\"id\":[],\"name\":[],\"source\":[],\"room\":[],\"player\":[],\"round\":[],\"speed\":[],\"timebefore\":\"2099-12-31 23:59:59\",\"timeafter\":\"2000-01-01 00:00:00\"},\"exclude\":{\"id\":[],\"name\":[],\"source\":[],\"room\":[],\"player\":[],\"round\":[],\"speed\":[],\"timebefore\":\"2000-01-01 00:00:00\",\"timeafter\":\"2099-12-31 23:59:59\"}}");
        PaipuAnalyzer(const CJsonObject &filterjson);
        void setfilter(std::string &filterstr);
        void setfilter(const CJsonObject &filterjson);
        bool filtercheck(CJsonObject &paipu);
        void clearresult();
        int analyze(std::vector<std::string> &paipus);
        int analyze(std::vector<CJsonObject*> &paipus);
        int analyze(std::vector<CJsonObject> &paipus);
        bool analyze(std::string &paipu);
        bool analyze(CJsonObject *paipu);
        bool analyze(CJsonObject &paipu);
    };

    void analyzebasedata(const std::string &savepath);
    void analyzemain(const std::string &dataf, const std::string &source, const std::string &id, CJsonObject &config);

}

#endif