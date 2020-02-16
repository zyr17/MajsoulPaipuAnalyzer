#include "analyzer.h"

namespace PA{

#ifdef SAVEMATCHDATASTEP
    CJsonObject TotalStep("[]"), GameStep("[]"), RoundStep("[]");
#endif

MatchPlayerData::MatchPlayerData(){
    score = reach = 0;
    get = Tiles::EMPTY;
}

void MatchPlayerData::clear(){
    reach = score = reachrank = 0;
    get = Tiles::EMPTY;
    hand.clear();
    table.clear();
    show.clear();
}

int MatchPlayerData::fulu() const{
    int res = 0;
    for (auto &i: show)
        res += i.size() < 5;
    return res;
}

CJsonObject MatchPlayerData::tojson() const{
    CJsonObject co("{}");
    CJsonObject arrhand("[]"), arrshow("[]"), arrtable("[]");
    for (auto i : hand)
        arrhand.Add(Tiles::num2tile[i]);
    for (auto &i : show){
        std::string str = "";
        if (i.size() == 5){
            //ankan
            str = "[";
            for (int j = 0; j < 4; j ++ )
                str += Tiles::num2tile[i[j]];
            str += "]";
        }
        else for (auto j : i)
            str += Tiles::num2tile[j];
        arrshow.Add(str);
    }
    for (auto i : table)
        arrtable.Add(Tiles::num2tile[i % Tiles::TILESYS]);
    co.Add("hand", arrhand);
    co.Add("get", get == Tiles::EMPTY ? "" : Tiles::num2tile[get]);
    co.Add("show", arrshow);
    co.Add("table", arrtable);
    co.Add("score", score);
    co.Add("reach", reach);
    return co;
}

void MatchData::IPutKyoutaku(){
    if (needkyoutaku == -1) return;
    kyoutaku ++ ;
    assert(data[needkyoutaku].score > 999);
    data[needkyoutaku].score -= 1000;
    needkyoutaku = -1;
}

void MatchData::IDiscardTile(std::string &str){
    int who = str[1] - '0';
    int tile = Tiles::tile2num(str.substr(2, 2));
    bool tsumokiri = str[4] == '1';
    int reach = str[5] - '0';
    std::vector<int> dora;
    for (unsigned i = 6; i < str.size(); i += 2)
        dora.push_back(Tiles::tile2num(str.substr(i, 2)));

    if (tsumokiri && data[who].hand.size() != 14){
        assert(data[who].get == tile);
        //if (data[who].get != tile) throw "data[who].get != tile";
        data[who].get = Tiles::EMPTY;
    }
    else{
        for (unsigned i = 0; i < data[who].hand.size(); i ++ )
            if (data[who].hand[i] == tile){
                Algo::changevec(data[who].hand, i, data[who].get == Tiles::EMPTY ? INT_MAX : data[who].get);
                data[who].get = Tiles::EMPTY;
                break;
            }
        assert(data[who].get == Tiles::EMPTY);
    }
    data[who].table.push_back(tile);
    if (dora.size()) this -> dora = dora;
    if (reach){
        assert(!data[who].reach);
        assert(needkyoutaku == -1);
        data[who].reach = reach;
        needkyoutaku = who;
        for (auto &i : data)
            data[who].reachrank += !!i.reach;
    }

    //reachbasedata
    auto &adata = *analyzedata;
    if (reach && who == adata.me){

        auto tenpai = Algo::calctenpai(data[who]);
        assert(tenpai.size());
        int reachtype = 1 - Algo::tenpaiquality(data[who], tenpai), basenum;
        assert(reachtype == 0 || reachtype == 1);
        assert(adata.num2reachtype[0] == "GOOD");

        BASENUM2VECEVAL(basenum, 0, adata.num2reachbasedata, "REACH");
        adata.reachbasedata[basenum][reachtype] ++ ;

        BASENUM2VECEVAL(basenum, 1, adata.num2reachbasedata, "CIRCLE");
        adata.reachbasedata[basenum][reachtype] += data[who].table.size();

        BASENUM2VECEVAL(basenum, 3, adata.num2reachbasedata, "TANYAO");
        if (Algo::istanyao(data[who])) adata.reachbasedata[basenum][reachtype] ++ ;

        BASENUM2VECEVAL(basenum, 4, adata.num2reachbasedata, "PINFU");
        if (Algo::ispinfu(data[who])) adata.reachbasedata[basenum][reachtype] ++ ;

        BASENUM2VECEVAL(basenum, 5, adata.num2reachbasedata, "DORA0");
        adata.reachbasedata[basenum + Algo::countdora(data[who], this -> dora)][reachtype] ++ ;

        BASENUM2VECEVAL(basenum, 32, adata.num2reachbasedata, "#1");
        adata.reachbasedata[basenum + data[adata.me].reachrank - 1][reachtype] ++ ;
        
        if (Algo::isfuriten(data[who], tenpai)){
            //振听立直
            BASENUM2VECEVAL(basenum, 45, adata.num2reachbasedata, "FURITEN");
            adata.reachbasedata[basenum][reachtype] ++ ;
        }

    }

    if (reach && who != adata.me){

        int basenum;

        //由于目前只有一个判断，因此将tenpaiquality加到if后减少计算次数
        //可以考虑将这个记录到MatchPlayerData或者AnalyzeData来减少调用
        BASENUM2VECEVAL(basenum, 40, adata.num2reachbasedata, "BEIZHUILI");
        if (data[adata.me].reach) adata.reachbasedata[basenum][1 - Algo::tenpaiquality(data[adata.me])] ++ ;

    }
    
    #ifdef MATCHDATAOUTPUT
        std::cout << "-----\n" << str << '\n' << who << ' ' << tile << ' ' << tsumokiri << ' ' << reach << '\n';
        OutputVector(dora);
    #endif
}

void MatchData::IDealTile(std::string &str){
    int who = str[1] - '0';
    int tile = Tiles::tile2num(str.substr(2, 2));
    std::vector<int> dora;
    for (unsigned i = 4; i < str.size(); i += 2)
        dora.push_back(Tiles::tile2num(str.substr(i, 2)));

    now = who;
    assert(data[who].get == Tiles::EMPTY);
    assert(remain);
    remain -- ;
    data[who].get = tile;
    IPutKyoutaku();
    if (dora.size()) this -> dora = dora;

    #ifdef MATCHDATAOUTPUT
        std::cout << "-----\n" << str << '\n' << who << ' ' << tile << '\n';
        OutputVector(dora);
    #endif
}

void MatchData::IChiPengGang(std::string &str){
    int who = str[4] - '0', from = str[1] - '0';
    std::vector<int> tiles;
    tiles.push_back(Tiles::tile2num(str.substr(2, 2)));
    std::vector<int> belong;
    belong.push_back(from);
    for (unsigned i = 5; i < str.size(); i += 2){
        tiles.push_back(Tiles::tile2num(str.substr(i, 2)));
        belong.push_back(who);
    }

    IPutKyoutaku();
    now = who;
    data[who].show.insert(data[who].show.begin(), tiles);
    assert(data[from].table.size() && *data[from].table.rbegin() == tiles[0]);
    *data[from].table.rbegin() += Tiles::TILESYS;
    for (unsigned t = 1; t < tiles.size(); t ++ ){
        auto tile = tiles[t];
        for (unsigned i = 0; i < data[who].hand.size(); i ++ )
            if (data[who].hand[i] == tile){
                Algo::changevec(data[who].hand, i);
                break;
            }
    }
    
    #ifdef MATCHDATAOUTPUT
        std::cout << "-----\n" << str << '\n' << who << '\n';
        OutputVector(tiles);
        OutputVector(belong);
    #endif

    auto &adata = *analyzedata;
    if (who == adata.me) {
        int basenum, fulunum = data[who].fulu();

        BASENUM2VECEVAL(basenum, 0, adata.num2fulubasedata, "FULU");
        adata.fulubasedata[basenum][fulunum] ++ ;

        BASENUM2VECEVAL(basenum, 1, adata.num2fulubasedata, "#1");
        int fulup = 0;
        for (int i = 0; i < 4; i ++ )
            fulup += !!data[i].fulu();
        adata.fulubasedata[basenum + fulup - 1][fulunum] ++ ;

        BASENUM2VECEVAL(basenum, 5, adata.num2fulubasedata, "DORA0");
        adata.fulubasedata[basenum + Algo::countdora(data[who], dora)][fulunum] ++ ;

        //TODO: 计算消一发
        BASENUM2VECEVAL(basenum, 32, adata.num2fulubasedata, "YIPATSUKESHI");
        adata.fulubasedata[basenum][fulunum] += 0;

        BASENUM2VECEVAL(basenum, 33, adata.num2fulubasedata, "CIRCLE");
        adata.fulubasedata[basenum][fulunum] += data[who].table.size();

        BASENUM2VECEVAL(basenum, 34, adata.num2fulubasedata, "TANYAO");
        if (Algo::istanyao(data[adata.me])) adata.fulubasedata[basenum][fulunum] ++ ;

        BASENUM2VECEVAL(basenum, 35, adata.num2fulubasedata, "YAKUHAI");
        if (Algo::isyakuhai(data[adata.me], (adata.me - east + 4) % 4, nowround / 4)) adata.fulubasedata[basenum][fulunum] ++ ;

        BASENUM2VECEVAL(basenum, 34, adata.num2basedata, "DAMINGGANG");
        if (tiles.size() == 4) adata.basedata[basenum] ++ ;

    }
}

void MatchData::IAnGangAddGang(std::string &str){
    int who = str[1] - '0';
    int tile = Tiles::tile2num(str.substr(2, 2));
    bool isankan = str[4] == '1';

    if (data[who].get != Tiles::EMPTY){
        data[who].hand.push_back(data[who].get);
        Algo::changevec(data[who].hand, data[who].hand.size() - 1, data[who].get);
        data[who].get = Tiles::EMPTY;
    }
    if (tile < 30 && tile % 10 == 5) tile -- ;
    int akanum = 0, handnum = 0;
    for (auto &i : data[who].hand){
        if (i == tile + 1 && tile < 30 && tile % 10 == 4){
            i -- ;
            akanum ++ ;
        }
        handnum += i == tile;
    }
    assert(handnum == 4 || handnum == 1);
    assert((handnum == 4) == isankan);
    if (isankan){
        //ankan
        std::vector<int> vec;
        for (; akanum -- ; vec.push_back(tile + 1));
        for (; vec.size() < 4; vec.push_back(tile));
        for (unsigned i = 0; i < data[who].hand.size(); )
            if (data[who].hand[i] == tile) Algo::changevec(data[who].hand, i);
            else i ++ ;
        vec.push_back(data[who].ANKANNUM);
        data[who].show.insert(data[who].show.begin(), vec);
    }
    else{
        //kakan
        bool pass = 1;
        for (auto &i : data[who].show){
            pass = 1;
            for (auto j : i)
                if (!(j == tile || (tile < 30 && (tile % 10 == 4 && j == tile + 1))))
                    pass = 0;
            if (pass){
                i.push_back(tile + akanum);
                for (unsigned j = 0; j < data[who].hand.size(); j ++ )
                    if (data[who].hand[j] == tile){
                        Algo::changevec(data[who].hand, j);
                        break;
                    }
                break;
            }
        }
        assert(pass);
    }

    auto &adata = *analyzedata;
    if (who == adata.me){

        int basenum;

        BASENUM2VECEVAL(basenum, 35, adata.num2basedata, "ANGANG");
        if (isankan) adata.basedata[basenum] ++ ;

        BASENUM2VECEVAL(basenum, 36, adata.num2basedata, "JIAGANG");
        if (!isankan) adata.basedata[basenum] ++ ;

        BASENUM2VECEVAL(basenum, 37, adata.num2basedata, "REACHANGANG");
        if (isankan && data[adata.me].reach) adata.basedata[basenum] ++ ;

    }
    
    #ifdef MATCHDATAOUTPUT
        std::cout << "-----\n" << str << '\n' << who << ' ' << tile << '\n';
    #endif
}

void MatchData::ILiuJu(std::string &str){
    char type2char[] = "39FKR";
    int typenum = -1;
    for (int i = 0; i < 5; i ++ )
        if (type2char[i] == str[1])
            typenum = i;
    
    #ifdef MATCHDATAOUTPUT
        std::cout << "-----\n" << str << '\n' << typenum << '\n';
    #endif
}

void MatchData::IHule(std::string &actstr){
    ////TODO: 由于多个和牌分开表示，考虑分别处理是否影响最后统计；需要额外记录哪些共享信息
    auto str = Algo::split(actstr, ',');
    int who = str[1][0] - '0';
    std::vector<int> hai, naki, ankan;
    int get = Tiles::tile2num(str[5]);
    std::vector<int> dora, ura;
    int fu = atoi(str[8].c_str());
    std::vector<std::pair<int, int>> han;
    bool tsumo = str[10] == "-1";
    int from = atoi(str[10].c_str());
    int bao = atoi(str[11].c_str());

    for (unsigned i = 0; i < str[2].size(); i += 2)
        hai.push_back(Tiles::tile2num(str[2].substr(i, 2)));
    if (str[3].size() != 0){
        auto nakis = Algo::split(str[3], '|');
        for (auto naki1 : nakis){
            int nownum = 0;
            for (unsigned i = 0; i < naki1.size(); i += 2)
                nownum = nownum * Tiles::TILESYS + Tiles::tile2num(naki1.substr(i, 2));
            naki.push_back(nownum);
        }
    }
    if (str[4].size() != 0){
        auto ankans = Algo::split(str[4], '|');
        for (auto ankan1 : ankans){
            int nownum = 0;
            for (unsigned i = 0; i < ankan1.size(); i += 2)
                nownum = nownum * Tiles::TILESYS + Tiles::tile2num(ankan1.substr(i, 2));
            ankan.push_back(nownum);
        }
    }
    for (unsigned i = 0; i < str[6].size(); i += 2)
        dora.push_back(Tiles::tile2num(str[6].substr(i, 2)));
    for (unsigned i = 0; i < str[7].size(); i += 2)
        ura.push_back(Tiles::tile2num(str[7].substr(i, 2)));
    auto hans = Algo::split(str[9], '|');
    for (unsigned i = 0; i < hans.size(); i += 2)
        han.push_back(std::make_pair(atoi(hans[i].c_str()), atoi(hans[i + 1].c_str())));
    
    #ifdef MATCHDATAOUTPUT
        std::cout << "-----\n" << actstr << '\n' << who << '\n';
        OutputVector(hai);
        OutputVector(naki);
        OutputVector(ankan);
        std::cout << get << '\n';
        OutputVector(dora);
        OutputVector(ura);
        std::cout << fu << '\n';
        for (auto &i : han)
            std::cout << i.first << '+' << i.second << ' ';
        std::cout << '\n' << tsumo << ' ' << from << ' ' << bao << '\n';
    #endif
    
    int totalhan = 0;
    for (auto &i: han)
        totalhan += i.second;
    //TODO: 多条和牌记录时对于本场和供托的处理,直接data置零是否有风险
    std::vector<int> dpoint, dsudian;
    dpoint.resize(data.size());
    dsudian.resize(dpoint.size());
    auto calcres = Algo::calctensu(fu, totalhan, honba, kyoutaku, who == east, tsumo);
    for (int i = 0; i < (int)dpoint.size(); i ++ )
        if (i == who) dpoint[i] = calcres[0];
        else if (!tsumo) dpoint[i] = - calcres[2] * (i == from);
        else dpoint[i] = - calcres[2 - (i == east)];
    calcres = Algo::calctensu(fu, totalhan, 0, 0, who == east, tsumo);
    for (int i = 0; i < (int)dsudian.size(); i ++ )
        if (i == who) dsudian[i] = calcres[0];
        else if (!tsumo) dsudian[i] = - calcres[2] * (i == from);
        else dsudian[i] = - calcres[2 - (i == east)];
    int pointlevel[] = {3900, 7700, 11600, INT_MAX};

    auto &adata = *analyzedata;
    {
        auto metype = adata.gethandtype(data[adata.me]);
        int basenum;

        BASENUM2VECEVAL(basenum, 0, adata.num2hulebasedata, "HULE");
        if (who == adata.me) adata.hulebasedata[basenum][metype] ++ ;

        BASENUM2VECEVAL(basenum, 1, adata.num2hulebasedata, "FANGCHONG");
        if (from == adata.me) adata.hulebasedata[basenum][metype] ++ ;

        BASENUM2VECEVAL(basenum, 2, adata.num2hulebasedata, "ZIMO");
        if (who == adata.me && tsumo) adata.hulebasedata[basenum][metype] ++ ;

        BASENUM2VECEVAL(basenum, 3, adata.num2hulebasedata, "BEIZIMO");
        if (who != adata.me && tsumo) adata.hulebasedata[basenum][metype] ++ ;

        BASENUM2VECEVAL(basenum, 4, adata.num2hulebasedata, "HULEPOINT");
        if (who == adata.me) adata.hulebasedata[basenum][metype] += dpoint[who];

        BASENUM2VECEVAL(basenum, 5, adata.num2hulebasedata, "HULESUDIAN");
        if (who == adata.me) adata.hulebasedata[basenum][metype] += dsudian[who];

        BASENUM2VECEVAL(basenum, 6, adata.num2hulebasedata, "FANGCHONGPOINT");
        if (from == adata.me) adata.hulebasedata[basenum][metype] += dpoint[from];

        BASENUM2VECEVAL(basenum, 7, adata.num2hulebasedata, "FANGCHONGSUDIAN");
        if (from == adata.me) adata.hulebasedata[basenum][metype] += dsudian[from];

        BASENUM2VECEVAL(basenum, 8, adata.num2hulebasedata, "ZIMOPOINT");
        if (who == adata.me && tsumo) adata.hulebasedata[basenum][metype] += dpoint[who];

        BASENUM2VECEVAL(basenum, 9, adata.num2hulebasedata, "ZIMOSUDIAN");
        if (who == adata.me && tsumo) adata.hulebasedata[basenum][metype] += dsudian[who];

        BASENUM2VECEVAL(basenum, 10, adata.num2hulebasedata, "BEIZIMOPOINT");
        if (who != adata.me && tsumo) adata.hulebasedata[basenum][metype] += dpoint[adata.me];

        BASENUM2VECEVAL(basenum, 11, adata.num2hulebasedata, "BEIZIMOSUDIAN");
        if (who != adata.me && tsumo) adata.hulebasedata[basenum][metype] += dsudian[adata.me];

        BASENUM2VECEVAL(basenum, 12, adata.num2hulebasedata, "HULE3900");
        if (adata.me == who){
            int nowpoint = dpoint[adata.me];
            for (int i = 0; nowpoint >= pointlevel[i]; i ++ , basenum ++ );
            if (nowpoint >= pointlevel[0]) adata.hulebasedata[basenum - 1][metype] ++ ;
        }

        BASENUM2VECEVAL(basenum, 15, adata.num2hulebasedata, "FANGCHONG3900");
        if (adata.me == from){
            int nowpoint = - dpoint[adata.me];
            for (int i = 0; nowpoint >= pointlevel[i]; i ++ , basenum ++ );
            if (nowpoint >= pointlevel[0]) adata.hulebasedata[basenum - 1][metype] ++ ;
        }

        BASENUM2VECEVAL(basenum, 18, adata.num2hulebasedata, "HULECIRCLE");
        if (who == adata.me) adata.hulebasedata[basenum][metype] += data[who].table.size();

        BASENUM2VECEVAL(basenum, 19, adata.num2hulebasedata, "FANGCHONGMYCIRCLE");
        if (from == adata.me) adata.hulebasedata[basenum][metype] += data[from].table.size();

        BASENUM2VECEVAL(basenum, 20, adata.num2hulebasedata, "ZIMOCIRCLE");
        if (who == adata.me && tsumo) adata.hulebasedata[basenum][metype] += data[who].table.size();

        BASENUM2VECEVAL(basenum, 21, adata.num2hulebasedata, "BEIZIMOMYCIRCLE");
        if (who != adata.me && tsumo) adata.hulebasedata[basenum][metype] += data[adata.me].table.size();

        BASENUM2VECEVAL(basenum, 22, adata.num2hulebasedata, "DORATIME");
        if (who == adata.me){
        bool hasdoras[3] = {0};
            for (auto &i : han){
                int hannum;
                BASENUM2VECEVAL(hannum, 52, adata.num2yakudata, "DORA");
                if (i.first >= hannum && i.first < hannum + 3) hasdoras[i.first - hannum] = true;
            }
            for (int i = 0; i < 3; i ++ ){
                assert(adata.num2hulebasedata[basenum + i] == adata.num2yakudata[basenum + 30 + i] + "TIME");
                if (hasdoras[i]) adata.hulebasedata[basenum][metype] ++ ;
            }
        }

        BASENUM2VECEVAL(basenum, 25, adata.num2hulebasedata, "ZHUANGHULE");
        if (who == adata.me && who == east) adata.hulebasedata[basenum][metype] ++ ;

        BASENUM2VECEVAL(basenum, 26, adata.num2hulebasedata, "ZHUANGZIMO");
        if (who == adata.me && who == east && tsumo) adata.hulebasedata[basenum][metype] ++ ;

        BASENUM2VECEVAL(basenum, 27, adata.num2hulebasedata, "ZHAZHUANG");
        if (who != adata.me && adata.me == east && tsumo) adata.hulebasedata[basenum][metype] ++ ;

        BASENUM2VECEVAL(basenum, 28, adata.num2hulebasedata, "ZHAZHUANGPOINT");
        if (who != adata.me && adata.me == east && tsumo) adata.hulebasedata[basenum][metype] += dpoint[adata.me];

        BASENUM2VECEVAL(basenum, 29, adata.num2hulebasedata, "CHONGLEZHUANG");
        if (from == adata.me && who == east) adata.hulebasedata[basenum][metype] ++ ;

        BASENUM2VECEVAL(basenum, 30, adata.num2hulebasedata, "CHONGLEZHUANGPOINT");
        if (from == adata.me && who == east) adata.hulebasedata[basenum][metype] += dpoint[from];

        BASENUM2VECEVAL(basenum, 31, adata.num2hulebasedata, "FANGCHONGHISCIRCLE");
        if (from == adata.me) adata.hulebasedata[basenum][metype] += data[who].table.size();

        BASENUM2VECEVAL(basenum, 32, adata.num2hulebasedata, "BEIZIMOHISCIRCLE");
        if (who != adata.me && tsumo) adata.hulebasedata[basenum][metype] += data[who].table.size();

        BASENUM2VECEVAL(basenum, 33, adata.num2hulebasedata, "CHONGLEDAMA");
        if (from == adata.me && !data[who].reach) adata.hulebasedata[basenum + data[who].fulu()][metype] ++ ;

        BASENUM2VECEVAL(basenum, 38, adata.num2hulebasedata, "CHONGLEDAMAPOINT");
        if (from == adata.me && !data[who].reach) adata.hulebasedata[basenum + data[who].fulu()][metype] += dpoint[from];

        BASENUM2VECEVAL(basenum, 43, adata.num2hulebasedata, "ZHUANGMEIHU");
        if (east == adata.me && who != adata.me) adata.hulebasedata[basenum][metype] ++ ;

        int gang = 0;
        for (auto &i : data[adata.me].show)
            gang += i.size() >= 4;
        if (gang){
            //这盘杠过了
            BASENUM2VECEVAL(basenum, 44, adata.num2hulebasedata, "GANGHULE");
            if (who == adata.me) adata.hulebasedata[basenum][metype] ++ ;

            BASENUM2VECEVAL(basenum, 45, adata.num2hulebasedata, "GANGHULEPOINT");
            if (who == adata.me) adata.hulebasedata[basenum][metype] += dpoint[adata.me];

            BASENUM2VECEVAL(basenum, 46, adata.num2hulebasedata, "GANGFANGCHONG");
            if (from == adata.me) adata.hulebasedata[basenum][metype] ++ ;

            BASENUM2VECEVAL(basenum, 47, adata.num2hulebasedata, "GANGFANGCHONGPOINT");
            if (from == adata.me) adata.hulebasedata[basenum][metype] += dpoint[adata.me];

            BASENUM2VECEVAL(basenum, 49, adata.num2hulebasedata, "GANGTOTALPOINT");
            adata.hulebasedata[basenum][metype] += dpoint[adata.me];
        }

        BASENUM2VECEVAL(basenum, 48, adata.num2hulebasedata, "TOTALPOINT");
        adata.hulebasedata[basenum][metype] += dpoint[adata.me];

        //向听仍使用basedata
        BASENUM2VECEVAL(basenum, 15, adata.num2basedata, "FANGCHONGSHANTEN0");
        if (from == adata.me) adata.basedata[basenum + Algo::calcshanten(data[from])] ++ ;

        //统计役种
        int doranum, akanum, uranum;
        BASENUM2VECEVAL(basenum, 0, adata.num2huleyakubasedata, "HULEYAKU");
        BASENUM2VECEVAL(doranum, 52, adata.num2yakudata, "DORA");
        BASENUM2VECEVAL(akanum, 54, adata.num2yakudata, "AKA");
        BASENUM2VECEVAL(uranum, 53, adata.num2yakudata, "URA");
        if (who == adata.me)
            for (auto &i : han){
                if (i.first == doranum || i.first == akanum || i.first == uranum)
                    adata.huleyakubasedata[basenum][i.first][metype] += i.second;
                else adata.huleyakubasedata[basenum][i.first][metype] ++ ;
            }

        BASENUM2VECEVAL(basenum, 1, adata.num2huleyakubasedata, "CHONGLEYAKU");
        if (from == adata.me)
            for (auto &i : han){
                if (i.first == doranum || i.first == akanum || i.first == uranum)
                    adata.huleyakubasedata[basenum][i.first][metype] += i.second;
                else adata.huleyakubasedata[basenum][i.first][metype] ++ ;
            }

        BASENUM2VECEVAL(basenum, 2, adata.num2huleyakubasedata, "BEIZIMOYAKU");
        if (who != adata.me && tsumo)
            for (auto &i : han){
                if (i.first == doranum || i.first == akanum || i.first == uranum)
                    adata.huleyakubasedata[basenum][i.first][metype] += i.second;
                else adata.huleyakubasedata[basenum][i.first][metype] ++ ;
            }

        int reachtype = -1;
        std::vector<int> tenpai;

        //立直宣言牌放铳，需要在Hule中判定
        BASENUM2VECEVAL(basenum, 2, adata.num2reachbasedata, "REACHDECLEARRON");
        if (needkyoutaku == from && from == adata.me){
            if (reachtype == -1){
                tenpai = Algo::calctenpai(data[adata.me]);
                reachtype = 1 - Algo::tenpaiquality(data[adata.me], tenpai);
                assert(reachtype == 0 || reachtype == 1);
                assert(adata.num2reachtype[0] == "GOOD");
            }
            adata.reachbasedata[basenum][reachtype] ++ ;
        }

        //针对追立&被追立和牌数据，在Hule中判定
        int reachnum = 0;
        for (auto &i : data)
            reachnum += !!i.reach;

        if (data[adata.me].reachrank > 1){
            if (reachtype == -1){
                tenpai = Algo::calctenpai(data[adata.me]);
                reachtype = 1 - Algo::tenpaiquality(data[adata.me], tenpai);
                assert(reachtype == 0 || reachtype == 1);
                assert(adata.num2reachtype[0] == "GOOD");
            }

            BASENUM2VECEVAL(basenum, 36, adata.num2reachbasedata, "ZHUILIHULE");
            if (adata.me == who) adata.reachbasedata[basenum][reachtype] ++ ;

            BASENUM2VECEVAL(basenum, 37, adata.num2reachbasedata, "ZHUILIHULEPOINT");
            if (adata.me == who) adata.reachbasedata[basenum][reachtype] += dpoint[who];

            BASENUM2VECEVAL(basenum, 38, adata.num2reachbasedata, "ZHUILIFANGCHONG");
            if (adata.me == from) adata.reachbasedata[basenum][reachtype] ++ ;

            BASENUM2VECEVAL(basenum, 39, adata.num2reachbasedata, "ZHUILIFANGCHONGPOINT");
            if (adata.me == from) adata.reachbasedata[basenum][reachtype] += dpoint[from];

            BASENUM2VECEVAL(basenum, 50, adata.num2reachbasedata, "ZHUILITOTALPOINT");
            adata.reachbasedata[basenum][reachtype] += dpoint[adata.me];

            BASENUM2VECEVAL(basenum, 52, adata.num2reachbasedata, "ZHUILIDECLEARRON");
            if (needkyoutaku == from && from == adata.me) adata.reachbasedata[basenum][reachtype] ++ ;
        }

        if (data[adata.me].reach && reachnum > data[adata.me].reachrank){
            if (reachtype == -1){
                tenpai = Algo::calctenpai(data[adata.me]);
                reachtype = 1 - Algo::tenpaiquality(data[adata.me], tenpai);
                assert(reachtype == 0 || reachtype == 1);
                assert(adata.num2reachtype[0] == "GOOD");
            }

            BASENUM2VECEVAL(basenum, 41, adata.num2reachbasedata, "BEIZHUILIHULE");
            if (adata.me == who) adata.reachbasedata[basenum][reachtype] ++ ;

            BASENUM2VECEVAL(basenum, 42, adata.num2reachbasedata, "BEIZHUILIHULEPOINT");
            if (adata.me == who) adata.reachbasedata[basenum][reachtype] += dpoint[who];

            BASENUM2VECEVAL(basenum, 43, adata.num2reachbasedata, "BEIZHUILIFANGCHONG");
            if (adata.me == from) adata.reachbasedata[basenum][reachtype] ++ ;

            BASENUM2VECEVAL(basenum, 44, adata.num2reachbasedata, "BEIZHUILIFANGCHONGPOINT");
            if (adata.me == from) adata.reachbasedata[basenum][reachtype] += dpoint[from];

            BASENUM2VECEVAL(basenum, 51, adata.num2reachbasedata, "BEIZHUILITOTALPOINT");
            adata.reachbasedata[basenum][reachtype] += dpoint[adata.me];
        }

        if (data[adata.me].reach){
            if (!tenpai.size())
                tenpai = Algo::calctenpai(data[adata.me]);
            if (Algo::isfuriten(data[adata.me], tenpai)){
                //自己立直且振听了
                if (reachtype == -1){
                    reachtype = 1 - Algo::tenpaiquality(data[adata.me], tenpai);
                    assert(reachtype == 0 || reachtype == 1);
                    assert(adata.num2reachtype[0] == "GOOD");
                }

                BASENUM2VECEVAL(basenum, 46, adata.num2reachbasedata, "FURITENHULE");
                if (adata.me == who) adata.reachbasedata[basenum][reachtype] ++ ;

                BASENUM2VECEVAL(basenum, 47, adata.num2reachbasedata, "FURITENHULEPOINT");
                if (adata.me == who) adata.reachbasedata[basenum][reachtype] += dpoint[who];

                BASENUM2VECEVAL(basenum, 48, adata.num2reachbasedata, "FURITENFANGCHONG");
                if (adata.me == from) adata.reachbasedata[basenum][reachtype] ++ ;

                BASENUM2VECEVAL(basenum, 49, adata.num2reachbasedata, "FURITENFANGCHONGPOINT");
                if (adata.me == from) adata.reachbasedata[basenum][reachtype] += dpoint[from];
            }
        }

    }

}

void MatchData::INoTile(std::string &str){
    std::vector<bool> tenpai, mangan;
    mangan.resize(4);
    for (int i = 0; i < 4; i ++ )
        tenpai.push_back(str[i + 2] == '1');
    for (unsigned i = 6; i < str.size(); i ++ )
        mangan[str[i] - '0'] = true;

    int in[5] = { 0, 3000, 1500, 1000, 0 };
    int out[5] = { 0, -1000, -1500, -3000, 0 };
    int tenpainum = 0;
    for (auto i : tenpai)
        tenpainum += i;

    auto &adata = *analyzedata;
    {
        int basenum;

        BASENUM2VECEVAL(basenum, 26, adata.num2basedata, "NORMALLIUJU");
        adata.basedata[basenum] ++ ;

        BASENUM2VECEVAL(basenum, 27, adata.num2basedata, "LIUJUTENPAI");
        if (tenpai[adata.me]) adata.basedata[basenum] ++ ;

        BASENUM2VECEVAL(basenum, 28, adata.num2basedata, "LIUJUNOTEN");
        if (!tenpai[adata.me]) adata.basedata[basenum] ++ ;

        BASENUM2VECEVAL(basenum, 29, adata.num2basedata, "LIUJUTENPAIPOINT");
        if (tenpai[adata.me]) adata.basedata[basenum] += in[tenpainum];

        BASENUM2VECEVAL(basenum, 30, adata.num2basedata, "LIUJUNOTENPOINT");
        if (!tenpai[adata.me]) adata.basedata[basenum] += out[tenpainum];

        BASENUM2VECEVAL(basenum, 31, adata.num2basedata, "LIUJUPOINT");
        if (tenpai[adata.me]) adata.basedata[basenum] += in[tenpainum];
        if (!tenpai[adata.me]) adata.basedata[basenum] += out[tenpainum];

    }
    
    #ifdef MATCHDATAOUTPUT
        std::cout << "-----\n" << str << '\n';
        for (auto i : tenpai) std::cout << i << ' ';
        for (auto i : mangan) std::cout << i << ' ';
        std::cout << '\n';
    #endif
}

void MatchData::IFinalScore(std::string &str){
    std::vector<int> score;
    auto ss = Algo::split(str, '|');
    ss[0].erase(0, 1);
    for (auto &i : ss)
        score.push_back(atoi(i.c_str()));

    for (unsigned i = 0; i < score.size(); i ++ )
        data[i].score = score[i];

    #ifdef PRINTTENPAI
        for (auto &i : data){
            if (Algo::calcshanten(i) > 0) continue;
            std::cout << i.hand.size();
            for (auto j : i.hand)
                std::cout << ' ' << j;
            for (auto &j : i.show){
                std::cout << ' ' << j.size();
                for (auto k : j)
                    std::cout << ' ' << k;
            }
            std::cout << ' 0\n';
        }
    #endif
    
    #ifdef MATCHDATAOUTPUT
        std::cout << "-----\n" << str << '\n';
        OutputVector(score);
    #endif
}

MatchData::MatchData(){
    kyoutaku = honba = east = now = remain = nowround = 0;
    needkyoutaku = -1;
    analyzedata = nullptr;
}

CJsonObject MatchData::tojson() const{
    CJsonObject arrdata("[]");
    std::string dorastr;
    for (auto &i : data)
        arrdata.Add(i.tojson());
    for (auto i : dora)
        dorastr = dorastr + Tiles::num2tile[i];
    CJsonObject co("{}");
    co.Add("title", "");
    co.Add("kyoutaku", kyoutaku);
    co.Add("honba", honba);
    co.Add("data", arrdata);
    co.Add("now", now);
    co.Add("east", east);
    co.Add("remain", remain);
    co.Add("nowround", nowround);
    co.Add("dora", dorastr);
    return co;
}

void MatchData::clear(){
    for (auto &i: data)
        i.clear();
    dora.clear();
    kyoutaku = honba = now = east = remain = nowround = 0;
    needkyoutaku = -1;
}

void MatchData::INewGame(CJsonObject &record){
    int startscore, player;
    record["gamedata"]["roomdata"].Get("init_point", startscore);
    record["gamedata"]["roomdata"].Get("player", player);

    clear();
    data.resize(player);
    for (int i = 0; i < player; i ++ )
        data[i].score = startscore;

    #ifdef MATCHDATAOUTPUT
        CJsonObject out(record);
        out.Delete("record");
        std::cout << "-----\n" << Algo::UTF82GBK(out.ToFormattedString()) << "\n-----\n";
        std::cout << startscore << "\n";
    #endif
}

void MatchData::INewRound(CJsonObject &record){
    int round, dice, east, honba, kyoutaku, remain;
    std::vector<int> yama, point, dora;
    std::vector<std::vector<int>> hand;
    record.Get("round", round);
    record.Get("dice", dice);
    record.Get("east", east);
    record.Get("honba", honba);
    record.Get("kyoutaku", kyoutaku);
    record.Get("remain", remain);
    std::string yamastr;
    record.Get("yama", yamastr);
    for (unsigned i = 0; i < yamastr.size(); i += 2)
        yama.push_back(Tiles::tile2num(yamastr.substr(i, 2)));
    auto &pointarr = record["point"];
    for (int i = 0; i < pointarr.GetArraySize(); i ++ ){
        int pp;
        pointarr.Get(i, pp);
        point.push_back(pp);
    }
    std::string dorastr;
    record.Get("dora", dorastr);
    for (unsigned i = 0; i < dorastr.size(); i += 2)
        dora.push_back(Tiles::tile2num(dorastr.substr(i, 2)));
    auto &jsonhand = record["hand"];
    for (int i = 0; i < jsonhand.GetArraySize(); i ++ ){
        std::vector<int> vec;
        std::string hstr;
        jsonhand.Get(i, hstr);
        for (unsigned j = 0; j < hstr.size(); j += 2)
            vec.push_back(Tiles::tile2num(hstr.substr(j, 2)));
        hand.push_back(vec);
    }

    for (unsigned i = 0; i < point.size(); i ++ )
        assert(point[i] == data[i].score);
    clear();
    this -> nowround = round;
    //this -> dice = dice;
    this -> east = now = east;
    this -> honba = honba;
    this -> kyoutaku = kyoutaku;
    //this -> yama = yama;
    for (unsigned i = 0; i < point.size(); i ++ )
        data[i].score = point[i];
    this -> dora = dora;
    for (unsigned i = 0; i < hand.size(); i ++ )
        data[i].hand = hand[i];
    this -> remain = remain;
    
    #ifdef MATCHDATAOUTPUT
        CJsonObject out(record);
        out.Delete("action");
        std::cout << "-----\n" << out.ToFormattedString() << "\n-----\n";
        std::cout << "round" << round << '\n';
        std::cout << "dice" << dice << '\n';
        std::cout << "east" << east << '\n';
        std::cout << "honba" << honba << '\n';
        std::cout << "kyoutaku" << kyoutaku << '\n';
        OutputVector(yama);
        OutputVector(point);
        OutputVector(dora);
        for (auto &v : hand)
            OutputVector(v);
    #endif
}

void MatchData::action(std::vector<std::string> &strvec){
    for (auto &i : strvec)
        action(i);
}

void MatchData::action(std::string &actstr){
    //std::cout << actstr << '\n';
    if (actstr[0] == 'A')
        IDiscardTile(actstr);
    else if (actstr[0] == 'B')
        IDealTile(actstr);
    else if (actstr[0] == 'C')
        IChiPengGang(actstr);
    else if (actstr[0] == 'D')
        IAnGangAddGang(actstr);
    else if (actstr[0] == 'X')
        IHule(actstr);
    else if (actstr[0] == 'Y'){
        if (actstr[1] == 'N')
            INoTile(actstr);
        else if (actstr[1] == 'F'){
            ILiuJu(actstr);
        }
        else if (actstr[1] == 'K'){
            ILiuJu(actstr);
        }
        else if (actstr[1] == 'R'){
            ILiuJu(actstr);
        }
        else if (actstr[1] == '9'){
            ILiuJu(actstr);
        }
        else if (actstr[1] == '3'){
            ILiuJu(actstr);
        }
    }
    else if (actstr[0] == 'Z')
        IFinalScore(actstr);
}

AnalyzeResultName::AnalyzeResultName(){
    json = Algo::ReadJSON("PAADData.json");
    std::string key;

    //base
    auto &basejson = json["base"];
    while (basejson.GetKey(key)){
        if (key == DESCRIPTION) continue;
        auto &arr = basejson[key];
        std::vector<std::string> vec;
        for (int i = 0; i < arr.GetArraySize(); i ++ ){
            std::string s;
            arr.Get(i, s);
            vec.push_back(s);
        }
        base[key] = vec;
    }

    //result
    auto &resultjson = json["result"];
    while (resultjson.GetKey(key)){
        if (key == DESCRIPTION) continue;
        std::string value;
        resultjson.Get(key, value);
        //! type
        bool flag = false;
        for (auto i : key)
            if (i == '!') flag = true;
        if (!flag){
            result.push_back(key);
            resultexpr.push_back(value);
        }
        else{
            //! type
            int start = INT_MAX, end = INT_MAX;
            int EPnum = 0;
            for (auto i : key){
                if (i == '!'){
                    EPnum ++ ;
                    if (EPnum == 1) start = 0;
                    else if (EPnum == 2) end = 0;
                }
                else if (EPnum == 1){
                    assert(i >= '0' && i <= '9');
                    start = start * 10 + i - '0';
                }
                else if (EPnum == 2){
                    assert(i >= '0' && i <= '9');
                    end = end * 10 + i - '0';
                }
            }
            assert(EPnum == 3);
            for (int i = start; i <= end; i ++ ){
                std::string okey, ovalue, num;
                for (int j = i; j; j /= 10)
                    num.insert(num.begin(), j % 10 + '0');
                if (!num.size()) num = "0";
                int EPnum = 0;
                for (auto j : key){
                    if (j == '!'){
                        EPnum ++ ;
                        if (EPnum == 1) okey += num;
                    }
                    else if (EPnum == 0 || EPnum == 3)
                        okey += j;
                }
                for (auto j : value)
                    if (j == '!') ovalue += num;
                    else ovalue += j;
                result.push_back(okey);
                resultexpr.push_back(ovalue);
            }
        }
    }

    //resultgroup
    auto &grouporderjson = json["resultgroup"]["order"];
    for (int i = 0; i < grouporderjson.GetArraySize(); i ++ ){
        std::string s;
        grouporderjson.Get(i, s);
        resultgrouporder.push_back(s);
    }
    auto &groupdatajson = json["resultgroup"]["data"];
    while (groupdatajson.GetKey(key)){
        auto &arr = groupdatajson[key];
        std::vector<std::string> vec;
        for (int i = 0; i < arr.GetArraySize(); i ++ ){
            std::string s;
            arr.Get(i, s);
            vec.push_back(s);
        }
        //! type
        bool flag = false;
        for (auto i : key)
            if (i == '!') flag = true;
        if (!flag){
            resultgroupmap[key] = vec;
        }
        else{
            //! type
            int start = INT_MAX, end = INT_MAX;
            int EPnum = 0;
            for (auto i : key){
                if (i == '!'){
                    EPnum ++ ;
                    if (EPnum == 1) start = 0;
                    else if (EPnum == 2) end = 0;
                }
                else if (EPnum == 1){
                    assert(i >= '0' && i <= '9');
                    start = start * 10 + i - '0';
                }
                else if (EPnum == 2){
                    assert(i >= '0' && i <= '9');
                    end = end * 10 + i - '0';
                }
            }
            assert(EPnum == 3);
            for (int i = start; i <= end; i ++ ){
                std::string okey, num;
                std::vector<std::string> ovec;
                for (int j = i; j; j /= 10)
                    num.insert(num.begin(), j % 10 + '0');
                if (!num.size()) num = "0";
                int EPnum = 0;
                for (auto j : key){
                    if (j == '!'){
                        EPnum ++ ;
                        if (EPnum == 1) okey += num;
                    }
                    else if (EPnum == 0 || EPnum == 3)
                        okey += j;
                }
                for (auto &j : vec){
                    std::string str;
                    for (auto k : j)
                        if (k == '!') str += num;
                        else str += k;
                    ovec.push_back(str);
                }
                resultgroupmap[okey] = ovec;
            }
        }
    }
    resultgroupmap[ALLRESULT] = result;
}

AnalyzeExprNumberList::AnalyzeExprNumberList(){
    QQ = false;
    startnum = -1;
}

void AnalyzeExpr::setoperator(){
    oprprevilige['+'] = 1;
    oprprevilige['-'] = 1;
    oprprevilige['*'] = 2;
    oprprevilige['/'] = 2;
    
    oprprevilige['('] = -100;
    oprprevilige[')'] = -100;

    oprprevilige['@'] = -100;

    oprprevilige[' '] = SPACE;
    oprprevilige['\r'] = SPACE;
    oprprevilige['\n'] = SPACE;
    oprprevilige['\t'] = SPACE;
}

std::string AnalyzeExpr::gettoken(const std::string &expr, int &k){
    for (; oprprevilige[(unsigned)expr[k]] == SPACE; k ++ );
    if (oprprevilige[(unsigned)expr[k]]) return std::string() + expr[k ++ ];
    std::string res;
    for (; !oprprevilige[(unsigned)expr[k]]; res += expr[k ++ ]);
    return res;
}

void AnalyzeExpr::makecalc(std::vector<double> &num, std::vector<int> &opr){
    assert(num.size() > 1 && opr.size());
    int o = *opr.rbegin();
    opr.pop_back();
    double num2 = *num.rbegin();
    num.pop_back();
    double num1 = *num.rbegin();
    num.pop_back();
    if (o == '+')
        num.push_back(num1 + num2);
    else if (o == '-')
        num.push_back(num1 - num2);
    else if (o == '*')
        num.push_back(num1 * num2);
    else if (o == '/')
        num.push_back(num1 / num2);
    else assert(0);
}

AnalyzeExprNumberList AnalyzeExpr::getnumberlist(const std::vector<std::string> &list, const std::string &str){
    AnalyzeExprNumberList res;
    if (*str.rbegin() == '?'){
        auto ss = str;
        ss.pop_back();
        if (*ss.rbegin() == '?'){
            res.QQ = true;
            ss.pop_back();
        }
        int startnum = -1;
        for (int i = 0; ; i ++ ){
            auto ss2 = ss;
            //假设不会出现三位数。应该不会吧。。
            if (i > 9) ss2 += '0' + i / 10;
            ss2 += '0' + i % 10;
            bool flag = false;
            for (unsigned j = 0; j < list.size(); j ++ )
                if (ss2 == list[j]){
                    res.list.push_back(j);
                    flag = true;
                    break;
                }
            if (flag && startnum == -1) startnum = i;
            //如果没有找到项目，且i不为0（假设序列连续，且以0或1开始），那么结束查找
            if (!flag && i) break;
        }
        if (!res.list.size()) std::cout << str << '\n';
        assert(res.list.size());
        res.startnum = startnum;
        return res;
    }
    for (unsigned i = 0; i < list.size(); i ++ )
        if (str == list[i]){
            res.list.push_back(i);
            break;
        }
    if (!res.list.size()) std::cout << str << '\n';
    assert(res.list.size());
    return res;
}

double AnalyzeExpr::getdata(std::vector<long long> &data, std::string kw1, const std::vector<std::string> &kw1list){
    auto list = getnumberlist(kw1list, kw1);
    if (list.list.size() == 1) return data[list.list[0]];
    double res = 0;
    for (unsigned i = 0; i < list.list.size(); i ++ )
        res += data[list.list[i]] * (list.QQ ? i + list.startnum : 1);
    return res;
}

double AnalyzeExpr::getdata(std::vector<double> &data, std::string kw1, const std::vector<std::string> &kw1list){
    auto list = getnumberlist(kw1list, kw1);
    if (list.list.size() == 1) return data[list.list[0]];
    double res = 0;
    for (unsigned i = 0; i < list.list.size(); i ++ )
        res += data[list.list[i]] * (list.QQ ? i + list.startnum : 1);
    return res;
}

double AnalyzeExpr::getdata(std::vector<std::vector<long long>> &data, std::string kw1, const std::vector<std::string> &kw1list, std::string kw2, const std::vector<std::string> &kw2list){
    auto list1 = getnumberlist(kw1list, kw1);
    auto list2 = getnumberlist(kw2list, kw2);
    double res = 0;
    for (unsigned i = 0; i < list1.list.size(); i ++ ){
        double mul = 1;
        if (list1.QQ) mul *= list1.startnum + i;
        for (unsigned j = 0; j < list2.list.size(); j ++ ){
            if (list2.QQ) mul *= list2.startnum + j;
            res += mul * data[list1.list[i]][list2.list[j]];
        }
    }
    return res;
}

double AnalyzeExpr::getdata(std::vector<std::vector<std::vector<long long>>> &data, std::string kw1, const std::vector<std::string> &kw1list, std::string kw2, const std::vector<std::string> &kw2list, std::string kw3, const std::vector<std::string> &kw3list){
    auto list1 = getnumberlist(kw1list, kw1);
    auto list2 = getnumberlist(kw2list, kw2);
    auto list3 = getnumberlist(kw3list, kw3);
    double res = 0;
    for (unsigned i = 0; i < list1.list.size(); i ++ ){
        double mul = 1;
        if (list1.QQ) mul *= list1.startnum + i;
        for (unsigned j = 0; j < list2.list.size(); j ++ ){
            if (list2.QQ) mul *= list2.startnum + j;
            for (unsigned k = 0; k < list3.list.size(); k ++ ){
                if (list3.QQ) mul *= list3.startnum + k;
                res += mul * data[list1.list[i]][list2.list[j]][list3.list[k]];
            }
        }
    }
    return res;
}

double AnalyzeExpr::getvalue(const std::string &s){
    assert(s.size());
    double res = 0, divide = 0;
    //数字
    if (s[0] >= '0' && s[0] <= '9'){
        for (auto i : s)
            if (i == '.') divide = 1;
            else{
                res = res * 10 + i - '0';
                divide *= 10;
            }
        if (divide) res /= divide;
        return res;
    }
    //引用基本统计结果和统计量
    // *注意* 引用统计量时一定要保证引用的统计量已经完成计算，否则会是0
    auto strs = Algo::split(s, '_');
    if (strs[0] == "RESULT")
        return getdata(adata -> result, strs[1], adata -> ADN.result);
    else if (strs[0] == "BASE")
        return getdata(adata -> basedata, strs[1], adata -> ADN.base["BASEDATA"]);
    else if (strs[0] == "REACH")
        return getdata(adata -> reachbasedata, strs[1], adata -> ADN.base["REACHBASEDATA"], strs[2], adata -> ADN.base["REACHTYPE"]);
    else if (strs[0] == "HULE")
        return getdata(adata -> hulebasedata, strs[1], adata -> ADN.base["HULEBASEDATA"], strs[2], adata -> ADN.base["HULEHANDTYPE"]);
    else if (strs[0] == "HULEYAKU")
        return getdata(adata -> huleyakubasedata, strs[1], adata -> ADN.base["HULEYAKUBASEDATA"], strs[2], adata -> ADN.base["YAKUDATA"], strs[3], adata -> ADN.base["HULEHANDTYPE"]);
    else if (strs[0] == "FULU")
        return getdata(adata -> fulubasedata, strs[1], adata -> ADN.base["FULUBASEDATA"], strs[2], adata -> ADN.base["FULUTYPE"]);
    else if (strs[0] == "FLOAT")
        return getdata(adata -> floatdata, strs[1], adata -> ADN.base["FLOATDATA"]);
    assert(0);
    return 0;
}

AnalyzeExpr::AnalyzeExpr(AnalyzeData *adata) : adata(adata) {
    memset(oprprevilige, 0, sizeof oprprevilige);
    setoperator();
}

double AnalyzeExpr::calcexpr(std::string expr){
    expr += '@';
    std::vector<double> num;
    std::vector<int> opr;
    for (int i = 0; i < (int)expr.size(); ){
        auto token = gettoken(expr, i);
        if (oprprevilige[(unsigned)token[0]]){
            if (token[0] == '(')
                    opr.push_back(token[0]);
            else if (token[0] == ')'){
                    for (; *opr.rbegin() != '('; makecalc(num, opr));
                    opr.pop_back();
            }
            else{
                    for (; opr.size() && oprprevilige[*opr.rbegin()] >= oprprevilige[(unsigned)token[0]]; makecalc(num, opr));
                    opr.push_back(token[0]);
            }
        }
        else{
            num.push_back(getvalue(token));
        }
    }
    assert(num.size() == 1);
    return num[0];
}

void AnalyzeData::outputonerect(const std::string &title, const std::string &key, int col){
    auto &res = ADN.resultgroupmap[key];
    std::vector<std::string> str;
    std::vector<double> data;

    if (key == "HULEYAKURESULT"){
        double hulenum = AE.calcexpr("HULE_HULE_ALL");
        for (unsigned i = 0; i < num2yakudata.size(); i ++ ){
            if (i == 36){
                assert(num2yakudata[i] == "RENHOU");
                continue;
            }
            str.push_back(I18N::get("YAKU", num2yakudata[i]) + I18N::get("MISC", "COLON"));
            data.push_back(AE.calcexpr("HULEYAKU_HULEYAKU_" + num2yakudata[i] + "_ALL") / hulenum);
        }
    }
    else if (key == "CHONGYAKURESULT"){
        double hulenum = AE.calcexpr("HULE_FANGCHONG_ALL");
        for (unsigned i = 0; i < num2yakudata.size(); i ++ ){
            if (i == 36){
                assert(num2yakudata[i] == "RENHOU");
                continue;
            }
            str.push_back(I18N::get("YAKU", num2yakudata[i]) + I18N::get("MISC", "COLON"));
            data.push_back(AE.calcexpr("HULEYAKU_CHONGLEYAKU_" + num2yakudata[i] + "_ALL") / hulenum);
        }
    }
    else for (auto &i : res){
        str.push_back(I18N::get("ADRESULT", i) + I18N::get("MISC", "COLON"));
        for (unsigned j = 0; j < ADN.result.size(); j ++ )
            if (i == ADN.result[j]){
                data.push_back(result[j]);
                break;
            }
    }

    assert(str.size() == data.size());
    Out::cout << title << "\n";
    int maxwidth = 0, len = str.size();
    for (int i = 0; i < len; i ++ ){
        int noww = Algo::getdisplaywidth(str[i]) + Algo::getdisplaywidth(data[i]) + 1;
        if (noww > maxwidth)
            maxwidth = noww;
    }
    if (maxwidth > col)
        col = maxwidth + 1;
    int iinline = (col - 1) / maxwidth;
    int lines = (len - 1) / iinline + 1;
    for (int i = 0; i < maxwidth * iinline; i ++ )
        Out::cout << (i ? '-' : ' ');
    for (int I = 0; I < iinline * lines; I ++ ){
        if (!(I % iinline))
            Out::cout << "\n|";
        int i = I / iinline + lines * (I % iinline);
        if (i >= len){
            for (int i = 0; i < maxwidth; i ++ )
                Out::cout << (i == maxwidth - 1 ? '|' : ' ');
            continue;
        }
        int width = Algo::getdisplaywidth(str[i]) + Algo::getdisplaywidth(data[i]) + 1;
        Out::cout << str[i];
        for (int i = maxwidth - width; i; i -- )
            Out::cout << ' ';
        isfinite(data[i]) ? Out::cout << data[i] : Out::cout << '-';
        Out::cout << '|';
    }
    Out::cout << '\n';
    for (int i = 0; i < maxwidth * iinline; i ++ )
        Out::cout << (i ? '-' : ' ');
    Out::cout << '\n';

}

AnalyzeData::AnalyzeData() : 
    AE(this),
    num2reachbasedata(ADN.base["REACHBASEDATA"]),
    num2reachtype(ADN.base["REACHTYPE"]),
    num2fulubasedata(ADN.base["FULUBASEDATA"]),
    num2hulebasedata(ADN.base["HULEBASEDATA"]),
    num2yakudata(ADN.base["YAKUDATA"]),
    num2basedata(ADN.base["BASEDATA"]),
    num2huleyakubasedata(ADN.base["HULEYAKUBASEDATA"]),
    num2fulutype(ADN.base["FULUTYPE"]),
    num2hulehandtype(ADN.base["HULEHANDTYPE"]),
    num2floatdata(ADN.base["FLOATDATA"])
{
    basedata.clear();
    basedata.resize(ADN.base["BASEDATA"].size());
    
    hulebasedata.clear();
    hulebasedata.resize(ADN.base["HULEBASEDATA"].size());
    for (auto &i : hulebasedata)
        i.resize(ADN.base["HULEHANDTYPE"].size());

    reachbasedata.clear();
    reachbasedata.resize(ADN.base["REACHBASEDATA"].size());
    for (auto &i : reachbasedata)
        i.resize(ADN.base["REACHTYPE"].size());

    huleyakubasedata.clear();
    huleyakubasedata.resize(ADN.base["HULEYAKUBASEDATA"].size());
    for (auto &i : huleyakubasedata){
        i.resize(ADN.base["YAKUDATA"].size());
        for (auto &j : i)
            j.resize(ADN.base["HULEHANDTYPE"].size());
    }

    fulubasedata.clear();
    fulubasedata.resize(ADN.base["FULUBASEDATA"].size());
    for (auto &i : fulubasedata)
        i.resize(ADN.base["FULUTYPE"].size());

    floatdata.clear();
    floatdata.resize(ADN.base["FLOATDATA"].size());

    me = -1;
}

int AnalyzeData::gethandtype(const MatchPlayerData &pdata){
    int res = -1;
    int tenpaiq = Algo::tenpaiquality(pdata);
    if (pdata.reach){
        if (tenpaiq == 1){
            BASENUM2VECEVAL(res, 0, num2hulehandtype, "REACHGOOD");
            return res;
        }
        BASENUM2VECEVAL(res, 1, num2hulehandtype, "REACHBAD");
        return res;
    }
    if (tenpaiq != -1){
        BASENUM2VECEVAL(res, 2 + pdata.fulu(), num2hulehandtype, (res == 2 ? "DAMA" : (std::string("FULU") + char('0' - 2 + res))));
        return res;
    }
    BASENUM2VECEVAL(res, 7 + pdata.fulu(), num2hulehandtype, (res == 7 ? "NTDAMA" : (std::string("NTFULU") + char('0' - 7 + res))));
    return res;
}

void AnalyzeData::makehanddata(std::vector<long long> &vec){
    int calcnum, fromnum;

    BASENUM2VECEVAL(calcnum, 12, num2hulehandtype, "FULUPLUS1");
    for (int j = calcnum; j < (int)vec.size(); j ++ )
        vec[j] = 0;

    BASENUM2VECEVAL(calcnum, 12, num2hulehandtype, "FULUPLUS1");
    BASENUM2VECEVAL(fromnum, 3, num2hulehandtype, "FULU1");
    for (int j = 4; j >= 1; j -- )
        vec[calcnum + j - 1] = vec[fromnum + j - 1] + (j == 4 ? 0 : vec[calcnum + j]);

    BASENUM2VECEVAL(calcnum, 16, num2hulehandtype, "NTFULUPLUS1");
    BASENUM2VECEVAL(fromnum, 8, num2hulehandtype, "NTFULU1");
    for (int j = 4; j >= 1; j -- )
        vec[calcnum + j - 1] = vec[fromnum + j - 1] + (j == 4 ? 0 : vec[calcnum + j]);

    BASENUM2VECEVAL(calcnum, 20, num2hulehandtype, "ALLFULU");
    BASENUM2VECEVAL(fromnum, 12, num2hulehandtype, "FULUPLUS1");
    vec[calcnum] += vec[fromnum];

    BASENUM2VECEVAL(calcnum, 20, num2hulehandtype, "ALLFULU");
    BASENUM2VECEVAL(fromnum, 16, num2hulehandtype, "NTFULUPLUS1");
    vec[calcnum] += vec[fromnum];

    BASENUM2VECEVAL(calcnum, 21, num2hulehandtype, "ALLDAMA");
    BASENUM2VECEVAL(fromnum, 2, num2hulehandtype, "DAMA");
    vec[calcnum] += vec[fromnum];

    BASENUM2VECEVAL(calcnum, 21, num2hulehandtype, "ALLDAMA");
    BASENUM2VECEVAL(fromnum, 7, num2hulehandtype, "NTDAMA");
    vec[calcnum] += vec[fromnum];

    BASENUM2VECEVAL(calcnum, 22, num2hulehandtype, "ALLREACH");
    BASENUM2VECEVAL(fromnum, 0, num2hulehandtype, "REACHGOOD");
    vec[calcnum] += vec[fromnum];

    BASENUM2VECEVAL(calcnum, 22, num2hulehandtype, "ALLREACH");
    BASENUM2VECEVAL(fromnum, 1, num2hulehandtype, "REACHBAD");
    vec[calcnum] += vec[fromnum];

    BASENUM2VECEVAL(calcnum, 23, num2hulehandtype, "ALL");
    BASENUM2VECEVAL(fromnum, 20, num2hulehandtype, "ALLFULU");
    vec[calcnum] += vec[fromnum];

    BASENUM2VECEVAL(calcnum, 23, num2hulehandtype, "ALL");
    BASENUM2VECEVAL(fromnum, 21, num2hulehandtype, "ALLDAMA");
    vec[calcnum] += vec[fromnum];

    BASENUM2VECEVAL(calcnum, 23, num2hulehandtype, "ALL");
    BASENUM2VECEVAL(fromnum, 22, num2hulehandtype, "ALLREACH");
    vec[calcnum] += vec[fromnum];
}

void AnalyzeData::calcresult(){
    result.clear();
    result.resize(ADN.result.size());
    resultjson("{}");

    for (auto &i : hulebasedata)
        makehanddata(i);

    for (auto &i : huleyakubasedata)
        for (auto &j : i)
            makehanddata(j);

    for (auto &i : reachbasedata){
        int b0, b1, b2;
        BASENUM2VECEVAL(b0, 0, num2reachtype, "GOOD");
        BASENUM2VECEVAL(b1, 1, num2reachtype, "BAD");
        BASENUM2VECEVAL(b2, 2, num2reachtype, "ALL");
        i[b2] = i[b0] + i[b1];
    }

    for (auto &i : fulubasedata){
        int b, r, r1;
        for (int k = 4; k >= 1; k -- ){
            if (k != 4) r1 = r;
            std::string str = "FULU0";
            str[4] = '0' + k;
            BASENUM2VECEVAL(b, k, num2fulutype, str);
            str = "FULUPLUS0";
            str[8]  ='0' + k;
            BASENUM2VECEVAL(r, k + 4, num2fulutype, str);
            i[r] = i[b];
            if (k != 4) i[r] += i[r1];
        }
    }

    {
        int basenum;

        double sr;
        std::pair<double, double> CI;
        Algo::SR::stablerank(4, sr, CI);

        BASENUM2VECEVAL(basenum, 0, num2floatdata, "EASTSTABLERANK");
        floatdata[basenum] = sr;

        BASENUM2VECEVAL(basenum, 1, num2floatdata, "EASTCILEFT");
        floatdata[basenum] = CI.first;

        BASENUM2VECEVAL(basenum, 2, num2floatdata, "EASTCIRIGHT");
        floatdata[basenum] = CI.second;

        Algo::SR::stablerank(8, sr, CI);

        BASENUM2VECEVAL(basenum, 3, num2floatdata, "SOUTHSTABLERANK");
        floatdata[basenum] = sr;

        BASENUM2VECEVAL(basenum, 4, num2floatdata, "SOUTHCILEFT");
        floatdata[basenum] = CI.first;

        BASENUM2VECEVAL(basenum, 5, num2floatdata, "SOUTHCIRIGHT");
        floatdata[basenum] = CI.second;
    }

    for (unsigned i = 0; i < ADN.result.size(); i ++ ){
        //std::cout << ADN.result[i] << ' ' << ADN.resultexpr[i] << '\n';
        result[i] = AE.calcexpr(ADN.resultexpr[i]);
    }

    //将分析数据存成JSON用于网页载入
    for (unsigned i = 0; i < ADN.result.size(); i ++ )
        if (isfinite(result[i])) resultjson.Add(ADN.result[i], result[i]);
        else resultjson.Add(ADN.result[i], "-");
    CJsonObject huleyaku("{}"), chongleyaku("{}");
    for (unsigned i = 0; i < num2yakudata.size(); i ++ ){
        if (i == 36){
            assert(num2yakudata[i] == "RENHOU");
            continue;
        }
        auto tmp = AE.calcexpr("HULEYAKU_HULEYAKU_" + num2yakudata[i] + "_ALL / HULE_HULE_ALL");
        if (isfinite(tmp)) huleyaku.Add(num2yakudata[i], tmp);
        else huleyaku.Add(num2yakudata[i], "-");
        tmp = AE.calcexpr("HULEYAKU_CHONGLEYAKU_" + num2yakudata[i] + "_ALL / HULE_FANGCHONG_ALL");
        if (isfinite(tmp) && num2yakudata[i] != "ZIMO" && num2yakudata[i] != "CHIHOU" && num2yakudata[i] != "TENHOU" && num2yakudata[i] != "SUANKO") chongleyaku.Add(num2yakudata[i], tmp);
        else chongleyaku.Add(num2yakudata[i], "-");
    }
    //网页空间浪费一点没关系，展示所有房间的安定星数和置信区间
    for (int i = 0; i < Algo::SR::ROOMNUMBER; i ++ )
        if (Algo::SR::considerroom[i]){
            double stablerank;
            std::pair<double, double> CI;
            Algo::SR::stablerank(4, stablerank, CI, i);
            std::string key = "EASTSR0";
            key[6] += i;
            if (isfinite(stablerank)) resultjson.Add(key, stablerank);
            else resultjson.Add(key, "-");
            key = "EASTCIL0";
            key[7] += i;
            if (isfinite(stablerank)) resultjson.Add(key, CI.first);
            else resultjson.Add(key, "-");
            key = "EASTCIR0";
            key[7] += i;
            if (isfinite(stablerank)) resultjson.Add(key, CI.second);
            else resultjson.Add(key, "-");

            Algo::SR::stablerank(8, stablerank, CI, i);
            key = "SOUTHSR0";
            key[7] += i;
            if (isfinite(stablerank)) resultjson.Add(key, stablerank);
            else resultjson.Add(key, "-");
            key = "SOUTHCIL0";
            key[8] += i;
            if (isfinite(stablerank)) resultjson.Add(key, CI.first);
            else resultjson.Add(key, "-");
            key = "SOUTHCIR0";
            key[8] += i;
            if (isfinite(stablerank)) resultjson.Add(key, CI.second);
            else resultjson.Add(key, "-");
        }
    //和牌役种和铳役种需要自行组装
    resultjson.Add("HULEYAKU", huleyaku);
    resultjson.Add("CHONGLEYAKU", chongleyaku);
    //std::cout << resultjson.ToFormattedString();
}

void AnalyzeData::outputbase(){
    auto &basedata = ADN.base["BASEDATA"];
    auto &hulebase = ADN.base["HULEBASEDATA"];
    auto &hulehandtype = ADN.base["HULEHANDTYPE"];
    auto &huleyakubase = ADN.base["HULEYAKUBASEDATA"];
    auto &yaku = ADN.base["YAKUDATA"];
    auto &reachbase = ADN.base["REACHBASEDATA"];
    auto &reachtype = ADN.base["REACHTYPE"];
    auto &fulubase = ADN.base["FULUBASEDATA"];
    auto &fulutype = ADN.base["FULUTYPE"];
    auto &floatdata = ADN.base["FLOATDATA"];
    for (auto &i : basedata){
        std::string str = "BASE_" + i;
        std::cout << str << ": " << AE.calcexpr(str) << '\n';
    }
    for (auto &i : hulebase)
        for (auto &j : hulehandtype){
            std::string str = "HULE_" + i + "_" + j;
            std::cout << str << ": " << AE.calcexpr(str) << '\n';
        }
    for (auto &i : huleyakubase)
        for (auto &j : yaku)
            for (auto &k : hulehandtype){
                std::string str = "HULEYAKU_" + i + "_" + j + "_" + k;
                std::cout << str << ": " << AE.calcexpr(str) << '\n';
            }
    for (auto &i : reachbase)
        for (auto &j : reachtype){
            std::string str = "REACH_" + i + "_" + j;
            std::cout << str << ": " << AE.calcexpr(str) << '\n';
        }
    for (auto &i : fulubase)
        for (auto &j : fulutype){
            std::string str = "FULU_" + i + "_" + j;
            std::cout << str << ": " << AE.calcexpr(str) << '\n';
        }
    for (auto &i : floatdata){
        std::string str = "FLOAT_" + i;
        std::cout << str << ": " << AE.calcexpr(str) << '\n';
    }
}

void AnalyzeData::outputresult(){
    std::cout << std::fixed << std::setprecision(4);
    int row, col;
    Algo::getconsolesize(row, col);
    for (auto key : ADN.resultgrouporder){
        auto title = I18N::get("ANALYZER", key) + I18N::get("MISC", "COLON");
        //命令行展示下节约空间不展示3 4副露结果
        if (key == "FULU3RESULT" || key == "FULU4RESULT")
            continue;
        if (key == "STABLERANKRESULT"){
            //对于安定段位标题还需要加上东风和南风统计的房间级别
            std::string room = "MAJSOULROOM";
            auto room4 = Algo::SR::getroom(4), room8 = Algo::SR::getroom(8);
            if (room4 != Algo::SR::INVALIDROOM)
                title += I18N::get("MISC", "ROUND4GAME") + I18N::get("MISC", room + (char)('0' + room4));
            if (room4 != Algo::SR::INVALIDROOM && room8 != Algo::SR::INVALIDROOM)
                title += I18N::get("MISC", "COMMA");
            if (room8 != Algo::SR::INVALIDROOM)
                title += I18N::get("MISC", "ROUND8GAME") + I18N::get("MISC", room + (char)('0' + room8));
            title += '\n';
            title += I18N::get("MISC", "MAJSOULSTABLERANKCOMMENT");
        }
        outputonerect(title, key, col);
    }
    Out::outputhtml(resultjson.ToString(), I18N::language);
    PAUSEEXIT;
}

void PaipuAnalyzer::initializeresult(){
    analyzedata = new AnalyzeData();
    matchdata.analyzedata = analyzedata;
}

bool PaipuAnalyzer::filterinclude(CJsonObject *p, CJsonObject *f, bool emptyresult){
    assert(f -> IsArray());
    int arrsize = f -> GetArraySize();
    bool result = (arrsize == 0 ? emptyresult : false);
    for (int i = 0; i < arrsize; i ++ )
        result = result || (*p == (*f)[i]);
    return result;
}

bool PaipuAnalyzer::filterexclude(CJsonObject *p, CJsonObject *f){
    return filterinclude(p, f, false);
}

bool PaipuAnalyzer::filtercheck(CJsonObject &paipu){
    bool result = true, excluderesult = false;
    
    CJsonObject *p, *f;
    auto &gamedata = paipu["gamedata"];
    auto &roomdata = gamedata["roomdata"];
    auto &include = filter["include"];
    auto &exclude = filter["exclude"];
    
    p = &gamedata["source"];
    f = &include["source"];
    result = result && filterinclude(p, f);
    f = &exclude["source"];
    excluderesult = excluderesult || filterexclude(p, f);
    
    p = &roomdata["room"];
    f = &include["room"];
    result = result && filterinclude(p, f);
    f = &exclude["room"];
    excluderesult = excluderesult || filterexclude(p, f);
    
    p = &roomdata["player"];
    f = &include["player"];
    result = result && filterinclude(p, f);
    f = &exclude["player"];
    excluderesult = excluderesult || filterexclude(p, f);
    
    p = &roomdata["round"];
    f = &include["round"];
    result = result && filterinclude(p, f);
    f = &exclude["round"];
    excluderesult = excluderesult || filterexclude(p, f);
    
    long long tb, ta, time;
    std::string tas, tbs;
    gamedata.Get("starttime", time);
    include.Get("timebefore", tbs);
    include.Get("timeafter", tas);
    ta = Algo::strptime(tas);
    tb = Algo::strptime(tbs);
    result = result && (tb >= time && ta <= time);
    exclude.Get("timebefore", tbs);
    exclude.Get("timeafter", tas);
    ta = Algo::strptime(tas);
    tb = Algo::strptime(tbs);
    excluderesult = excluderesult || (tb >= time || ta <= time);

    //分为三档速度 分别对应三个每圈时间，不考虑共享时间。
    long long tll;
    std::map<long long, std::string> map;
    map[3] = "fast";
    map[5] = "normal";
    map[60] = "slow";
    roomdata.Get("time_fixed", tll);
    CJsonObject tone("\"" + map[tll] + "\"");
    f = &include["speed"];
    result = result && filterinclude(&tone, f);
    f = &exclude["speed"];
    excluderesult = excluderesult || filterexclude(&tone, f);

    CJsonObject &playerdata = gamedata["playerdata"];
    bool bb;
    f = &include["id"];
    bb = playerdata.GetArraySize() == 0;
    for (int i = 0, gsize = playerdata.GetArraySize(); i < gsize; i ++ ){
        CJsonObject *p = &playerdata[i]["id"];
        bb = bb || filterinclude(p, f);
    }
    result = result && bb;
    f = &exclude["id"];
    bb = false;
    for (int i = 0, gsize = playerdata.GetArraySize(); i < gsize; i ++ ){
        CJsonObject *p = &playerdata[i]["id"];
        bb = bb || filterexclude(p, f);
    }
    excluderesult = excluderesult || bb;

    f = &include["name"];
    bb = false;
    for (int i = 0, gsize = playerdata.GetArraySize(); i < gsize; i ++ ){
        CJsonObject *p = &playerdata[i]["name"];
        bb = bb || filterinclude(p, f);
    }
    result = result && bb;
    f = &exclude["name"];
    bb = false;
    for (int i = 0, gsize = playerdata.GetArraySize(); i < gsize; i ++ ){
        CJsonObject *p = &playerdata[i]["name"];
        bb = bb || filterexclude(p, f);
    }
    excluderesult = excluderesult || bb;

    return result && !excluderesult;
}

PaipuAnalyzer::PaipuAnalyzer(std::string filterstr){
    setfilter(filterstr);
    initializeresult();
}

PaipuAnalyzer::PaipuAnalyzer(const CJsonObject &filterjson){
    setfilter(filterjson);
    initializeresult();
}

void PaipuAnalyzer::setfilter(std::string &filterstr){
    setfilter(CJsonObject(filterstr));
}

void PaipuAnalyzer::setfilter(const CJsonObject &filterjson){
    filter = filterjson;
}

void PaipuAnalyzer::clearresult(){
    initializeresult();
}

int PaipuAnalyzer::analyze(std::vector<std::string> &paipus){
    int count = 0;
    for (auto &i : paipus)
        if (analyze(i)) count ++ ;
    return count;
}

int PaipuAnalyzer::analyze(std::vector<CJsonObject> &paipus){
    int count = 0;
    for (auto &i : paipus)
        if (analyze(i)) count ++ ;
    return count;
}

int PaipuAnalyzer::analyze(std::vector<CJsonObject*> &paipus){
    int count = 0;
    for (auto &i : paipus)
        if (analyze(i)) count ++ ;
    return count;
}

bool PaipuAnalyzer::analyze(std::string &paipu){
    auto paipujson = CJsonObject(paipu);
    return analyze(paipujson);
}

bool PaipuAnalyzer::analyze(CJsonObject *paipu){
    return analyze(*paipu);
}

bool PaipuAnalyzer::analyze(CJsonObject &paipu){
    if (!filtercheck(paipu)) return false;
    auto &adata = *analyzedata;
    adata.me = -1;
    auto &accountid = paipu["gamedata"]["accountid"];
    auto &pdata = paipu["gamedata"]["playerdata"];
    for (int i = 0; i < 4; i ++ )
        if (accountid == pdata[i]["id"])
            adata.me = i;
    //如果玩家数据中没找到对应ID就跳过该牌谱
    //std::cout << Algo::UTF82GBK(paipu["gamedata"]["accountid"].ToString() + ' ' + accountid.ToString() + ' ' + paipu["gamedata"].ToString()) << '\n';
    if (adata.me == -1) return false;
    matchdata.INewGame(paipu);
    #ifdef SAVEMATCHDATASTEP
        GameStep = CJsonObject("[]");
    #endif
    auto &records = paipu["record"];
    auto rlen = records.GetArraySize();
    for (int i = 0; i < rlen; i ++ ){
        auto &oner = records[i];
        matchdata.INewRound(oner);
        #ifdef SAVEMATCHDATASTEP
            RoundStep = CJsonObject("[]");
            RoundStep.Add(matchdata.tojson());
        #endif
        auto &action = oner["action"];
        auto actlen = action.GetArraySize();
        for (int i = 0; i < actlen; i ++ ){
            std::string actstr;
            action.Get(i, actstr);
            matchdata.action(actstr);
            #ifdef SAVEMATCHDATASTEP
                if ((i + 1) % SAVEMATCHDATASTEP == 0)
                    RoundStep.Add(matchdata.tojson());
            #endif
        }
        #ifdef SAVEMATCHDATASTEP
            RoundStep.Add(matchdata.tojson());
            GameStep.Add(RoundStep);
        #endif
        /* 
        bool flag = 0;
        for (auto i : matchdata.data)
            for (auto j : i.show)
                if (j.size() == 4 && (Tiles::num2tile[j[0]][0] == '5' || Tiles::num2tile[j[0]][0] == '0')) flag = 1;
        if (flag) std::cout << paipu["gamedata"]["uuid"].ToString() << '\n';
         */
        {
            int basenum;

            BASENUM2VECEVAL(basenum, 9, adata.num2basedata, "TOTALROUND");
            adata.basedata[basenum] ++ ;

            BASENUM2VECEVAL(basenum, 10, adata.num2basedata, "REACH");
            adata.basedata[basenum] += !!matchdata.data[adata.me].reach;

            //BASENUM2VECEVAL(basenum, 11, adata.num2basedata, "FULU1");
            //int fulu = matchdata.data[adata.me].fulu();
            //if (fulu) adata.basedata[basenum - 1 + fulu] ++ ;

            BASENUM2VECEVAL(basenum, 33, adata.num2basedata, "ZHUANG");
            if (matchdata.east == adata.me) adata.basedata[basenum] ++ ;
            
        }

    }
    #ifdef SAVEMATCHDATASTEP
        TotalStep.Add(GameStep);
    #endif

    {
        int basenum; 
        
        BASENUM2VECEVAL(basenum, 0, adata.num2basedata, "TOTALGAME");
        adata.basedata[basenum] ++ ;
        
        BASENUM2VECEVAL(basenum, 1, adata.num2basedata, "#1");
        std::vector<int> vec;
        for (auto i : matchdata.data)
            vec.push_back(i.score);
        auto finalrank = Algo::getrank(vec, adata.me);
        adata.basedata[basenum - 1 + finalrank] ++ ;

        int totalround, altime = 0;
        paipu["gamedata"]["roomdata"].Get("round", totalround);
        for (int i = records.GetArraySize() - 1; i >= 0; i -- ){
            int nowround;
            records[i].Get("round", nowround);
            if (nowround >= totalround - 1){
                altime ++ ;
                vec.clear();
                auto &points = records[i]["point"];
                for (int j = 0; j < points.GetArraySize(); j ++ ){
                    int point;
                    points.Get(j, point);
                    vec.push_back(point);
                }
            }
            else break;
        }
        auto alrank = Algo::getrank(vec, adata.me);

        BASENUM2VECEVAL(basenum, 5, adata.num2basedata, "ALBAO1");
        adata.basedata[basenum] += alrank == 1 && finalrank == 1;

        BASENUM2VECEVAL(basenum, 6, adata.num2basedata, "ALNI1");
        adata.basedata[basenum] += alrank != 1 && finalrank == 1;
        
        BASENUM2VECEVAL(basenum, 7, adata.num2basedata, "ALBI4");
        adata.basedata[basenum] += alrank == 4 && finalrank != 4;

        BASENUM2VECEVAL(basenum, 8, adata.num2basedata, "ALMULTITIME");
        adata.basedata[basenum] += altime > 1;

        BASENUM2VECEVAL(basenum, 22, adata.num2basedata, "AL#1");
        adata.basedata[basenum - 1 + alrank] ++ ;

        BASENUM2VECEVAL(basenum, 32, adata.num2basedata, "ALLPOINT");
        int stp, endp;
        paipu["gamedata"]["roomdata"].Get("init_point", stp);
        pdata[adata.me].Get("finalpoint", endp);
        adata.basedata[basenum] += endp - stp;

        //天凤数据暂时不计算安定段位
        if (paipu["gamedata"]("source") == "tenhou")
            return true;
        //TODO: 天凤安定段位
        int room, pt;
        pdata[adata.me].Get("deltapt", pt);
        paipu["gamedata"]["roomdata"].Get("room", room);
        //std::cout << room << ' ' << totalround << ' ' << finalrank << ' ' << pt << ' ' << endp << '\n';
        Algo::SR::addgamedata(room, totalround, finalrank, pt, endp);
    }

    return true;
}

void analyzebasedata(const std::string &savepath){
    //东 南 全
    //全 各位天凤大佬
    const int TENHOUID_NUM = 16, ROUNDTYPE_NUM = 3;
    const std::string tenhouid[TENHOUID_NUM] = {
        "ALLHOUOU",
        "ALLTENHOU",
        "ASAPIN",
        "（≧▽≦）",
        "独歩",
        "すずめクレイジー",
        "太くないお",
        "タケオしゃん",
        "コーラ下さい",
        "かにマジン",
        "就活生@川村軍団",
        "ウルトラ立直",
        "トトリ先生19歳",
        "おかもと",
        "gousi",
        "お知らせ",
    };
    const std::string roundtype[ROUNDTYPE_NUM] = { "ALL", "EAST", "SOUTH" };
    PaipuAnalyzer pa[ROUNDTYPE_NUM][TENHOUID_NUM];
    int paipunum[ROUNDTYPE_NUM][TENHOUID_NUM] = {0};
    CJsonObject lastpaipu;
    std::string paipuf = "data/tenhou/combined/paipus/";
    std::string lastopen = "00000000.txt";
    for (int year = 2009; year < 2029; year ++ )
        for (int month = 1; month <= 12; month ++ )
            for (int day = 1; day <= 31; day ++ ){
                char buf[256] = {0};
                sprintf(buf, "%s%04d/%04d%02d%02d.txt", paipuf.c_str(), year, year, month, day);
                if (Algo::Access(buf, 0) != - 1){
                    std::cout << buf << std::endl;
                    auto paipus = Algo::ReadJSON(buf);
                    for (int i = 0; i < paipus.GetArraySize(); i ++ ){
                        auto &paipu = paipus[i];
                        int round;
                        paipu["gamedata"]["roomdata"].Get("round", round);
                        auto &playerdata = paipu["gamedata"]["playerdata"];
                        for (int j = 0; j < playerdata.GetArraySize(); j ++ ){
                            auto nowid = playerdata[j]("id");
                            int tenhouindex = -1;
                            for (int l = 2; l < TENHOUID_NUM; l ++ )
                                if (nowid == tenhouid[l])
                                    tenhouindex = l;
                            paipu["gamedata"].Replace("accountid", nowid);
                            for (int k = 0; k < 3; k ++ )
                                if (k * 4 == round || !k){
                                    paipunum[k][0] += pa[k][0].analyze(paipu);
                                    if (~tenhouindex){
                                        //std::cout << nowid << '\n';
                                        paipunum[k][1] += pa[k][1].analyze(paipu);
                                        paipunum[k][tenhouindex] += pa[k][tenhouindex].analyze(paipu);
                                    }
                                }
                        }
                    }
                }
            }
    /*
    for (auto &i : paipunum){
        for (auto &j : i)
            std::cout << j << ' ';
        std::cout << '\n';
    }
    */
    CJsonObject basedatajson("{}"), i18njson("{}"), resultgroupjson("{}"), resultcomparejson("{}");
    for (int i = 0; i < ROUNDTYPE_NUM; i ++ ){
        CJsonObject oneround("{}");
        for (int j = 0; j < TENHOUID_NUM; j ++ ){
            pa[i][j].analyzedata -> calcresult();
            if (paipunum[i][j] != 0) oneround.Add(tenhouid[j], pa[i][j].analyzedata -> resultjson);
        }
        basedatajson.Add(roundtype[i], oneround);
    }
    const std::string languages[] = { "zh-CN", "zh-TW", "en-US", "ja-JP", "fr-FR", "de-DE", "ko-KR" };
    for (auto &lang : languages)
        if (Algo::Access(("i18n/" + lang + ".json").c_str(), 0) != -1)
            i18njson.Add(lang, Algo::ReadJSON("i18n/" + lang + ".json"));
    CJsonObject rgorder("[]"), rgdata("{}");
    for (auto &order : pa[0][0].analyzedata -> ADN.resultgrouporder){
        rgorder.Add(order);
        CJsonObject onedata("[]");
        for (auto &item : pa[0][0].analyzedata -> ADN.resultgroupmap[order])
            onedata.Add(item);
        rgdata.Add(order, onedata);
    }
    resultgroupjson.Add("order", rgorder);
    resultgroupjson.Add("data", rgdata);
    resultcomparejson = Algo::ReadJSON("PAADData.json")["resultcompare"];
    std::string resultcode = "";
    resultcode += "BASEDATA = " + basedatajson.ToString() + ";\n\n";
    resultcode += "I18N = " + i18njson.ToString() + ";\n\n";
    resultcode += "RESULTGROUP = " + resultgroupjson.ToString() + ";\n\n";
    resultcode += "RESULTCOMPARE = " + resultcomparejson.ToString() + ";\n\n";
    auto f = fopen(savepath.c_str(), "w");
    fprintf(f, "%s", resultcode.c_str());
    std::cout << "saved in " + savepath + "\n";
}

void analyzetenhou(const std::string &dataf, const std::string &source, const std::string &id, CJsonObject &config){
    //TODO: 天凤大量牌谱分析专用，将相关代码移进来
}

void analyzemain(const std::string &dataf, const std::string &source, const std::string &id, CJsonObject &config){
    auto &filter = config["filter"];
    PaipuAnalyzer pa = PaipuAnalyzer(filter);
    int paipunum = 0;
    if (source == "tenhou"){
        //天凤数据不针对id，根据gamedata筛选相关条目
        //找到牌谱后直接计算；不先提取所有牌谱
        CJsonObject lastpaipu;
        auto gamedataf = dataf + "/tenhou/combined/gamedata/";
        std::string lastopen = "00000000.txt";
        int opencount = 0;
        for (int year = 2009; year < 2029; year ++ )
            for (int month = 1; month <= 12; month ++ ){
                char buf[256] = {0};
                sprintf(buf, "%s%04d%02d.txt", gamedataf.c_str(), year, month);
                if (Algo::Access(buf, 0) != - 1){
                    std::cout << buf << std::endl;
                    auto gamedatas = Algo::ReadLineJSON(buf);
                    for (unsigned i = 0; i < gamedatas.size(); i ++ ){
                        auto &gamedata = gamedatas[i];
                        auto &playerdata = gamedata["playerdata"];
                        for (int i = 0; i < playerdata.GetArraySize(); i ++ ){
                            std::string nowid;
                            playerdata[i].Get("id", nowid);
                            if (nowid == id){
                                std::string paipufilename, uuid;
                                gamedata.Get("uuid", uuid);
                                paipufilename = uuid.substr(0, 8) + ".txt";
                                //std::cout << uuid << ' ' << paipufilename << std::endl;
                                assert(lastopen <= paipufilename);
                                if (lastopen < paipufilename){
                                    char buf[256] = {0};
                                    sprintf(buf, "%s/tenhou/combined/paipus/%04d/%s", dataf.c_str(), year, paipufilename.c_str());
                                    lastpaipu = Algo::ReadJSON(buf);
                                    if (opencount) std::cout << lastopen << ' ' << opencount << std::endl;
                                    //std::cout << buf << '\n';
                                    lastopen = paipufilename;
                                    opencount = 0;
                                }
                                bool flag = false;
                                for (int i = 0; i < lastpaipu.GetArraySize(); i ++ ){
                                    std::string s;
                                    lastpaipu[i]["gamedata"].Get("uuid", s);
                                    //std::cout << s << '\n';
                                    if (s == uuid){
                                        //std::cout << uuid << '\n';
                                        lastpaipu[i]["gamedata"].Replace("accountid", nowid);
                                        //std::cout << Algo::UTF82GBK(nowid + ' ' + lastpaipu[i]["gamedata"]["accountid"].ToString()) << '\n';
                                        auto nowjson = lastpaipu[i].ToString();
                                        paipunum += pa.analyze(&lastpaipu[i]);
                                        //paipus.push_back(&lastpaipu[i]);
                                        opencount ++ ;
                                        flag = true;
                                        break;
                                    }
                                }
                                assert(flag);
                            }
                        }
                    }
                }
            }
    }
    else{
        std::vector<CJsonObject*> paipus;
        auto paipuarr = Algo::ReadJSON(dataf + "/" + source + "/" + id + "/paipus.txt");
        for (int i = 0; i < paipuarr.GetArraySize(); i ++ )
            paipus.push_back(&paipuarr[i]);
        //int step = paipus.size() - 1;
	    //for (; paipus.size() < 100000; paipus.push_back(*(paipus.rbegin() + step)));
        paipunum = pa.analyze(paipus);
    }
    #ifdef SAVEMATCHDATASTEP
        std::cout << TotalStep.ToString();
    #endif
    Out::cout << I18N::get("ANALYZER", "ANALYZEPAIPUNUM") << paipunum << I18N::get("ANALYZER", "ANALYZEPAIPUNUMAFT") << '\n';
    Out::cout << I18N::get("MISC", "DASH") << '\n';
    pa.analyzedata -> calcresult();

    /*
    CJsonObject rrr("{}");
    for (unsigned i = 0; i < pa.analyzedata -> result.size(); i ++ ){
        auto res = pa.analyzedata -> result[i];
        if (isfinite(res))
            rrr.Add(pa.analyzedata -> ADN.result[i], res);
        else
            rrr.Add(pa.analyzedata -> ADN.result[i], "-");
    }
    std::cout << id << ": " << rrr.ToString() << ",\n";
    */

    //std::cout << pa.analyzedata -> resultjson.ToString() << '\n';
    //pa.analyzedata -> outputbase();
    pa.analyzedata -> outputresult();
}

}