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
    reach = score = 0;
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

CJsonObject MatchPlayerData::tojson(){
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
    }

    //reachbasedata
    auto &adata = *analyzedata;
    auto &num2reachbasedata = adata.ADN.base["REACHBASEDATA"];
    auto &num2reachtype = adata.ADN.base["REACHTYPE"];
    if (reach && who == adata.me){

        int reachtype = 1 - Algo::tenpaiquality(data[who]), basenum;
        assert(reachtype == 0 || reachtype == 1);
        assert(num2reachtype[0] == "GOOD");

        basenum = 0;
        assert(num2reachbasedata[basenum] == "REACH");
        adata.reachbasedata[basenum][reachtype] ++ ;

        basenum = 1;
        assert(num2reachbasedata[basenum] == "CIRCLE");
        adata.reachbasedata[basenum][reachtype] += data[who].table.size();

        basenum = 3;
        assert(num2reachbasedata[basenum] == "TANYAO");
        if (Algo::istanyao(data[who])) adata.reachbasedata[basenum][reachtype] ++ ;

        basenum = 4;
        assert(num2reachbasedata[basenum] == "PINFU");
        if (Algo::ispinfu(data[who])) adata.reachbasedata[basenum][reachtype] ++ ;

        basenum = 5;
        assert(num2reachbasedata[basenum] == "DORA0");
        adata.reachbasedata[basenum + Algo::countdora(data[who], this -> dora)][reachtype] ++ ;

        basenum = 32;
        assert(num2reachbasedata[basenum] == "#1");
        int nowmany = 0;
        for (int i = 0; i < 4; i ++ )
            if (data[i].reach) nowmany ++ ;
        adata.reachbasedata[basenum + nowmany - 1][reachtype] ++ ;

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
}

void MatchData::IAnGangAddGang(std::string &str){
    int who = str[1] - '0';
    int tile = Tiles::tile2num(str.substr(2, 2));
    //bool isankan = str[4] == '1';

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
    if (handnum == 4){
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
    for (unsigned i = 0; i < str[6].size(); i ++ )
        dora.push_back(Tiles::tile2num(str[6].substr(i, 2)));
    for (unsigned i = 0; i < str[7].size(); i ++ )
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
    auto num2hulebasedata = adata.ADN.base["HULEBASEDATA"];
    auto num2yakudata = adata.ADN.base["YAKUDATA"];
    auto num2basedata = adata.ADN.base["BASEDATA"];
    auto num2huleyakubasedata = adata.ADN.base["HULEYAKUBASEDATA"];
    auto num2reachbasedata = adata.ADN.base["REACHBASEDATA"];
    auto num2reachtype = adata.ADN.base["REACHTYPE"];
    {
        auto whotype = adata.gethandtype(data[who]), fromtype = tsumo ? -1 : adata.gethandtype(data[from]);
        auto metype = adata.gethandtype(data[adata.me]);
        //int whotype = 0, fromtype = 0, metype = 0;
        int basenum;

        basenum = 0;
        assert(num2hulebasedata[basenum] == "HULE");
        if (who == adata.me) adata.hulebasedata[basenum][whotype] ++ ;
        //if (who == adata.me) std::cout << whotype;

        basenum = 1;
        assert(num2hulebasedata[basenum] == "FANGCHONG");
        if (from == adata.me) adata.hulebasedata[basenum][fromtype] ++ ;

        basenum = 2;
        assert(num2hulebasedata[basenum] == "ZIMO");
        if (who == adata.me && tsumo) adata.hulebasedata[basenum][whotype] ++ ;

        basenum = 3;
        assert(num2hulebasedata[basenum] == "BEIZIMO");
        if (who != adata.me && tsumo) adata.hulebasedata[basenum][metype] ++ ;

        basenum = 4;
        assert(num2hulebasedata[basenum] == "HULEPOINT");
        if (who == adata.me) adata.hulebasedata[basenum][whotype] += dpoint[who];

        basenum = 5;
        assert(num2hulebasedata[basenum] == "HULESUDIAN");
        if (who == adata.me) adata.hulebasedata[basenum][whotype] += dsudian[who];

        basenum = 6;
        assert(num2hulebasedata[basenum] == "FANGCHONGPOINT");
        if (from == adata.me) adata.hulebasedata[basenum][fromtype] += dpoint[from];

        basenum = 7;
        assert(num2hulebasedata[basenum] == "FANGCHONGSUDIAN");
        if (from == adata.me) adata.hulebasedata[basenum][fromtype] += dsudian[from];

        basenum = 8;
        assert(num2hulebasedata[basenum] == "ZIMOPOINT");
        if (who == adata.me && tsumo) adata.hulebasedata[basenum][whotype] += dpoint[who];

        basenum = 9;
        assert(num2hulebasedata[basenum] == "ZIMOSUDIAN");
        if (who == adata.me && tsumo) adata.hulebasedata[basenum][whotype] += dsudian[who];

        basenum = 10;
        assert(num2hulebasedata[basenum] == "BEIZIMOPOINT");
        if (who != adata.me && tsumo) adata.hulebasedata[basenum][whotype] += dpoint[adata.me];

        basenum = 11;
        assert(num2hulebasedata[basenum] == "BEIZIMOSUDIAN");
        if (who != adata.me && tsumo) adata.hulebasedata[basenum][whotype] += dsudian[adata.me];

        basenum = 12;
        assert(num2hulebasedata[basenum] == "HULE3900");
        if (adata.me == who){
            int nowpoint = dpoint[adata.me];
            for (int i = 0; nowpoint >= pointlevel[i]; i ++ , basenum ++ );
            if (nowpoint >= pointlevel[0]) adata.hulebasedata[basenum - 1][whotype] ++ ;
        }

        basenum = 15;
        assert(num2hulebasedata[basenum] == "FANGCHONG3900");
        if (adata.me == from){
            int nowpoint = - dpoint[adata.me];
            for (int i = 0; nowpoint >= pointlevel[i]; i ++ , basenum ++ );
            if (nowpoint >= pointlevel[0]) adata.hulebasedata[basenum - 1][fromtype] ++ ;
        }

        basenum = 18;
        assert(num2hulebasedata[basenum] == "HULECIRCLE");
        if (who == adata.me) adata.hulebasedata[basenum][whotype] += data[who].table.size();

        basenum = 19;
        assert(num2hulebasedata[basenum] == "FANGCHONGMYCIRCLE");
        if (from == adata.me) adata.hulebasedata[basenum][fromtype] += data[from].table.size();

        basenum = 20;
        assert(num2hulebasedata[basenum] == "ZIMOCIRCLE");
        if (who == adata.me && tsumo) adata.hulebasedata[basenum][whotype] += data[who].table.size();

        basenum = 21;
        assert(num2hulebasedata[basenum] == "BEIZIMOMYCIRCLE");
        if (who != adata.me && tsumo) adata.hulebasedata[basenum][metype] += data[adata.me].table.size();

        basenum = 22;
        assert(num2hulebasedata[basenum] == "DORATIME");
        if (who == adata.me){
        bool hasdoras[3] = {0};
            for (auto &i : han){
                int hannum = 52;
                assert(num2yakudata[hannum] == "DORA");
                if (i.first >= hannum && i.first < hannum + 3) hasdoras[i.first - hannum] = true;
            }
            for (int i = 0; i < 3; i ++ ){
                assert(num2hulebasedata[basenum + i] == num2yakudata[basenum + 30 + i] + "TIME");
                if (hasdoras[i]) adata.hulebasedata[basenum][metype] ++ ;
            }
        }

        basenum = 25;
        assert(num2hulebasedata[basenum] == "ZHUANGHULE");
        if (who == adata.me && who == east) adata.hulebasedata[basenum][whotype] ++ ;

        basenum = 26;
        assert(num2hulebasedata[basenum] == "ZHUANGZIMO");
        if (who == adata.me && who == east && tsumo) adata.hulebasedata[basenum][whotype] ++ ;

        basenum = 27;
        assert(num2hulebasedata[basenum] == "ZHAZHUANG");
        if (who != adata.me && who == east && tsumo) adata.hulebasedata[basenum][metype] ++ ;

        basenum = 28;
        assert(num2hulebasedata[basenum] == "ZHAZHUANGPOINT");
        if (who != adata.me && who == east && tsumo) adata.hulebasedata[basenum][metype] += dpoint[adata.me];

        basenum = 29;
        assert(num2hulebasedata[basenum] == "CHONGLEZHUANG");
        if (from == adata.me && who == east) adata.hulebasedata[basenum][fromtype] ++ ;

        basenum = 30;
        assert(num2hulebasedata[basenum] == "CHONGLEZHUANGPOINT");
        if (from == adata.me && who == east) adata.hulebasedata[basenum][fromtype] += dpoint[from];

        basenum = 31;
        assert(num2hulebasedata[basenum] == "FANGCHONGHISCIRCLE");
        if (from == adata.me) adata.hulebasedata[basenum][fromtype] += data[who].table.size();

        basenum = 32;
        assert(num2hulebasedata[basenum] == "BEIZIMOHISCIRCLE");
        if (who != adata.me && tsumo) adata.hulebasedata[basenum][metype] += data[who].table.size();

        basenum = 33;
        assert(num2hulebasedata[basenum] == "CHONGLEDAMA");
        if (from == adata.me && !data[who].reach) adata.hulebasedata[basenum + data[who].fulu()][fromtype] ++ ;

        basenum = 38;
        assert(num2hulebasedata[basenum] == "CHONGLEDAMAPOINT");
        if (from == adata.me && !data[who].reach) adata.hulebasedata[basenum + data[who].fulu()][fromtype] += dpoint[from];

        basenum = 43;
        assert(num2hulebasedata[basenum] == "ZHUANGMEIHU");
        if (east == adata.me && who != adata.me) adata.hulebasedata[basenum][fromtype] ++ ;

        //向听仍使用basedata
        basenum = 15;
        assert(num2basedata[basenum] == "FANGCHONGSHANTEN0");
        if (from == adata.me) adata.basedata[basenum + Algo::calcshanten(data[from])] ++ ;

        //统计役种
        int doranum = 52, akanum = 54, uranum = 53;
        basenum = 0;
        assert(num2huleyakubasedata[basenum] == "HULEYAKU");
        assert(num2yakudata[doranum] == "DORA");
        assert(num2yakudata[akanum] == "AKA");
        assert(num2yakudata[uranum] == "URA");
        if (who == adata.me)
            for (auto &i : han){
                if (i.first == doranum || i.first == akanum || i.first == uranum)
                    adata.huleyakubasedata[basenum][i.first][whotype] += i.second;
                else adata.huleyakubasedata[basenum][i.first][whotype] ++ ;
            }

        basenum = 1;
        assert(num2huleyakubasedata[basenum] == "CHONGLEYAKU");
        if (from == adata.me)
            for (auto &i : han){
                if (i.first == doranum || i.first == akanum || i.first == uranum)
                    adata.huleyakubasedata[basenum][i.first][whotype] += i.second;
                else adata.huleyakubasedata[basenum][i.first][whotype] ++ ;
            }

        basenum = 2;
        assert(num2huleyakubasedata[basenum] == "BEIZIMOYAKU");
        if (who != adata.me && tsumo)
            for (auto &i : han){
                if (i.first == doranum || i.first == akanum || i.first == uranum)
                    adata.huleyakubasedata[basenum][i.first][whotype] += i.second;
                else adata.huleyakubasedata[basenum][i.first][whotype] ++ ;
            }

        //立直宣言牌放铳，需要在Hule中判定
        basenum = 2;
        assert(num2reachbasedata[basenum] == "REACHDECLEARRON");
        if (needkyoutaku == from && from == adata.me){
            int reachtype = 1 - Algo::tenpaiquality(data[who]);
            assert(reachtype == 0 || reachtype == 1);
            assert(num2reachtype[0] == "GOOD");
            adata.reachbasedata[basenum][reachtype] ++ ;
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
    auto num2basedata = adata.ADN.base["BASEDATA"];
    {
        int basenum;

        basenum = 26;
        assert(num2basedata[basenum] == "NORMALLIUJU");
        adata.basedata[basenum] ++ ;

        basenum = 27;
        assert(num2basedata[basenum] == "LIUJUTENPAI");
        if (tenpai[adata.me]) adata.basedata[basenum] ++ ;

        basenum = 28;
        assert(num2basedata[basenum] == "LIUJUNOTEN");
        if (!tenpai[adata.me]) adata.basedata[basenum] ++ ;

        basenum = 29;
        assert(num2basedata[basenum] == "LIUJUTENPAIPOINT");
        if (tenpai[adata.me]) adata.basedata[basenum] += in[tenpainum];

        basenum = 30;
        assert(num2basedata[basenum] == "LIUJUNOTENPOINT");
        if (!tenpai[adata.me]) adata.basedata[basenum] += out[tenpainum];

        basenum = 31;
        assert(num2basedata[basenum] == "LIUJUPOINT");
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

CJsonObject MatchData::tojson(){
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
    record["gamedata"]["roomdata"].Get("startpoint", startscore);
    record["gamedata"]["roomdata"].Get("player", player);

    clear();
    data.resize(player);
    for (int i = 0; i < player; i ++ )
        data[i].score = startscore;

    #ifdef MATCHDATAOUTPUT
        CJsonObject out(record);
        out.Delete("record");
        std::cout << "-----\n" << out.ToFormattedString() << "\n-----\n";
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
    auto pointarr = record["point"];
    for (int i = 0; i < pointarr.GetArraySize(); i ++ ){
        int pp;
        pointarr.Get(i, pp);
        point.push_back(pp);
    }
    std::string dorastr;
    record.Get("dora", dorastr);
    for (unsigned i = 0; i < dorastr.size(); i += 2)
        dora.push_back(Tiles::tile2num(dorastr.substr(i, 2)));
    auto jsonhand = record["hand"];
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
    auto basejson = json["base"];
    while (basejson.GetKey(key)){
        if (key == DESCRIPTION) continue;
        auto arr = basejson[key];
        std::vector<std::string> vec;
        for (int i = 0; i < arr.GetArraySize(); i ++ ){
            std::string s;
            arr.Get(i, s);
            vec.push_back(s);
        }
        base[key] = vec;
    }

    //result
    auto resultjson = json["result"];
    while (resultjson.GetKey(key)){
        if (key == DESCRIPTION) continue;
        std::string value;
        resultjson.Get(key, value);
        result.push_back(key);
        resultexpr.push_back(value);
    }

    //resultgroup
    auto grouporderjson = json["resultgroup"]["order"];
    for (int i = 0; i < grouporderjson.GetArraySize(); i ++ ){
        std::string s;
        grouporderjson.Get(i, s);
        resultgrouporder.push_back(s);
    }
    auto groupdatajson = json["resultgroup"]["data"];
    while (groupdatajson.GetKey(key)){
        auto arr = groupdatajson[key];
        std::vector<std::string> vec;
        for (int i = 0; i < arr.GetArraySize(); i ++ ){
            std::string s;
            arr.Get(i, s);
            vec.push_back(s);
        }
        resultgroupmap[key] = vec;
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
    //引用基本统计结果
    auto strs = Algo::split(s, '_');
    if (strs[0] == "BASE"){
        return getdata(adata -> basedata, strs[1], adata -> ADN.base["BASEDATA"]);
    }
    else if (strs[0] == "REACH"){
        return getdata(adata -> reachbasedata, strs[1], adata -> ADN.base["REACHBASEDATA"], strs[2], adata -> ADN.base["REACHTYPE"]);
    }
    else if (strs[0] == "HULE"){
        return getdata(adata -> hulebasedata, strs[1], adata -> ADN.base["HULEBASEDATA"], strs[2], adata -> ADN.base["HULEHANDTYPE"]);
    }
    else if (strs[0] == "HULEYAKU"){
        return getdata(adata -> huleyakubasedata, strs[1], adata -> ADN.base["HULEYAKUBASEDATA"], strs[2], adata -> ADN.base["YAKUDATA"], strs[3], adata -> ADN.base["HULEHANDTYPE"]);
    }
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

void AnalyzeData::outputonerect(const std::string &title, const std::vector<std::string> &res, int col){
    std::vector<std::string> str;
    std::vector<double> data;
    for (auto &i : res){
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

AnalyzeData::AnalyzeData() : AE(this) {
    basedata.clear();
    basedata.resize(ADN.base["BASEDATA"].size());
    yakudata.clear();
    yakudata.resize(ADN.base["YAKUDATA"].size());
    for (auto &i : yakudata)
        i.resize(ADN.base["YAKUDATA"].size());
    
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
    me = -1;
}

int AnalyzeData::gethandtype(const MatchPlayerData &pdata){
    int res = -1;
    int tenpaiq = Algo::tenpaiquality(pdata);
    auto num2hulehandtype = ADN.base["HULEHANDTYPE"];
    if (pdata.reach){
        if (tenpaiq == 1){
            res = 0;
            assert(num2hulehandtype[res] == "REACHGOOD");
            return res;
        }
        res = 1;
        assert(num2hulehandtype[res] == "REACHBAD");
        return res;
    }
    if (tenpaiq != -1){
        res = 2 + pdata.fulu();
        assert(num2hulehandtype[res] == (res == 2 ? "DAMA" : (std::string("FULU") + char('0' - 2 + res))));
        return res;
    }
    res = 7 + pdata.fulu();
    assert(num2hulehandtype[res] == (res == 7 ? "NTDAMA" : (std::string("NTFULU") + char('0' - 7 + res))));
    return res;
}

void AnalyzeData::makehanddata(std::vector<long long> &vec){
    int calcnum, fromnum;
    auto num2hulehandtype = ADN.base["HULEHANDTYPE"];

    calcnum = 12;
    assert(num2hulehandtype[calcnum] == "FULU");
    for (int j = calcnum; j < (int)vec.size(); j ++ )
        vec[j] = 0;

    calcnum = 12;
    fromnum = 3;
    assert(num2hulehandtype[calcnum] == "FULU" && num2hulehandtype[fromnum] == "FULU1");
    for (int j = 0; j < 4; j ++ )
        vec[calcnum] += vec[fromnum + j];

    calcnum = 13;
    fromnum = 8;
    assert(num2hulehandtype[calcnum] == "NTFULU" && num2hulehandtype[fromnum] == "NTFULU1");
    for (int j = 0; j < 4; j ++ )
        vec[calcnum] += vec[fromnum + j];

    calcnum = 14;
    fromnum = 12;
    assert(num2hulehandtype[calcnum] == "ALLFULU" && num2hulehandtype[fromnum] == "FULU");
    vec[calcnum] += vec[fromnum];

    calcnum = 14;
    fromnum = 13;
    assert(num2hulehandtype[calcnum] == "ALLFULU" && num2hulehandtype[fromnum] == "NTFULU");
    vec[calcnum] += vec[fromnum];

    calcnum = 15;
    fromnum = 2;
    assert(num2hulehandtype[calcnum] == "ALLDAMA" && num2hulehandtype[fromnum] == "DAMA");
    vec[calcnum] += vec[fromnum];

    calcnum = 15;
    fromnum = 7;
    assert(num2hulehandtype[calcnum] == "ALLDAMA" && num2hulehandtype[fromnum] == "NTDAMA");
    vec[calcnum] += vec[fromnum];

    calcnum = 16;
    fromnum = 0;
    assert(num2hulehandtype[calcnum] == "ALLREACH" && num2hulehandtype[fromnum] == "REACHGOOD");
    vec[calcnum] += vec[fromnum];

    calcnum = 16;
    fromnum = 1;
    assert(num2hulehandtype[calcnum] == "ALLREACH" && num2hulehandtype[fromnum] == "REACHBAD");
    vec[calcnum] += vec[fromnum];

    calcnum = 17;
    fromnum = 14;
    assert(num2hulehandtype[calcnum] == "ALL" && num2hulehandtype[fromnum] == "ALLFULU");
    vec[calcnum] += vec[fromnum];

    calcnum = 17;
    fromnum = 15;
    assert(num2hulehandtype[calcnum] == "ALL" && num2hulehandtype[fromnum] == "ALLDAMA");
    vec[calcnum] += vec[fromnum];

    calcnum = 17;
    fromnum = 16;
    assert(num2hulehandtype[calcnum] == "ALL" && num2hulehandtype[fromnum] == "ALLREACH");
    vec[calcnum] += vec[fromnum];
}

void AnalyzeData::calcresult(){
    result.clear();
    result.resize(ADN.result.size());

    for (auto &i : hulebasedata)
        makehanddata(i);
    for (auto &i : huleyakubasedata)
        for (auto &j : i)
            makehanddata(j);
    auto num2reachtype = ADN.base["REACHTYPE"];
    for (auto &i : reachbasedata){
        int b0 = 0, b1 = 1, b2 = 2;
        assert(num2reachtype[b0] == "GOOD");
        assert(num2reachtype[b1] == "BAD");
        assert(num2reachtype[b2] == "ALL");
        i[b2] = i[b0] + i[b1];
    }

    for (unsigned i = 0; i < ADN.result.size(); i ++ )
        result[i] = AE.calcexpr(ADN.resultexpr[i]);
}

void AnalyzeData::outputresult(){
    std::cout << std::fixed << std::setprecision(4);
    int row, col;
    Algo::getconsolesize(row, col);
    for (auto key : ADN.resultgrouporder)
        outputonerect(I18N::get("ANALYZER", key) + I18N::get("MISC", "COLON"), ADN.resultgroupmap[key], col);
    PAUSEEXIT;
}

void PaipuAnalyzer::initializeresult(){
    analyzedata = new AnalyzeData();
    matchdata.analyzedata = analyzedata;
}

bool PaipuAnalyzer::filterinclude(const CJsonObject &p, CJsonObject &f, bool emptyresult){
    assert(f.IsArray());
    int arrsize = f.GetArraySize();
    bool result = (arrsize == 0 ? emptyresult : false);
    for (int i = 0; i < arrsize; i ++ )
        result = result || (p == f[i]);
    return result;
}

bool PaipuAnalyzer::filterexclude(const CJsonObject &p, CJsonObject &f){
    return filterinclude(p, f, false);
}

bool PaipuAnalyzer::filtercheck(const CJsonObject &paipu){
    bool result = true, excluderesult = false;
    
    CJsonObject gamedata, roomdata, include, exclude, p, f;
    paipu.Get("gamedata", gamedata);
    roomdata = gamedata["roomdata"];
    include = filter["include"];
    exclude = filter["exclude"];
    
    p = gamedata["source"];
    f = include["source"];
    result = result && filterinclude(p, f);
    f = exclude["source"];
    excluderesult = excluderesult || filterexclude(p, f);
    
    p = roomdata["room"];
    f = include["room"];
    result = result && filterinclude(p, f);
    f = exclude["room"];
    excluderesult = excluderesult || filterexclude(p, f);
    
    p = roomdata["player"];
    f = include["player"];
    result = result && filterinclude(p, f);
    f = exclude["player"];
    excluderesult = excluderesult || filterexclude(p, f);
    
    p = roomdata["round"];
    f = include["round"];
    result = result && filterinclude(p, f);
    f = exclude["round"];
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
    roomdata.Get("timeone", tll);
    CJsonObject tone("\"" + map[tll] + "\"");
    f = include["speed"];
    result = result && filterinclude(tone, f);
    f = exclude["speed"];
    excluderesult = excluderesult || filterexclude(tone, f);

    CJsonObject playerdata = gamedata["playerdata"];
    bool bb;
    f = include["id"];
    bb = playerdata.GetArraySize() == 0;
    for (int i = 0, gsize = playerdata.GetArraySize(); i < gsize; i ++ ){
        CJsonObject p = playerdata[i]["id"];
        bb = bb || filterinclude(p, f);
    }
    result = result && bb;
    f = exclude["id"];
    bb = false;
    for (int i = 0, gsize = playerdata.GetArraySize(); i < gsize; i ++ ){
        CJsonObject p = playerdata[i]["id"];
        bb = bb || filterexclude(p, f);
    }
    excluderesult = excluderesult || bb;

    f = include["name"];
    bb = false;
    for (int i = 0, gsize = playerdata.GetArraySize(); i < gsize; i ++ ){
        CJsonObject p = playerdata[i]["name"];
        bb = bb || filterinclude(p, f);
    }
    result = result && bb;
    f = exclude["name"];
    bb = false;
    for (int i = 0, gsize = playerdata.GetArraySize(); i < gsize; i ++ ){
        CJsonObject p = playerdata[i]["name"];
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

bool PaipuAnalyzer::analyze(std::string &paipu){
    auto paipujson = CJsonObject(paipu);
    return analyze(paipujson);
}

bool PaipuAnalyzer::analyze(CJsonObject &paipu){
    if (!filtercheck(paipu)) return false;
    auto &adata = *analyzedata;
    auto num2basedata = adata.ADN.base["BASEDATA"];
    adata.me = -1;
    auto accountid = paipu["gamedata"]["accountid"];
    auto pdata = paipu["gamedata"]["playerdata"];
    for (int i = 0; i < 4; i ++ )
        if (accountid == pdata[i]["id"])
            adata.me = i;
    //如果玩家数据中没找到对应ID就跳过该牌谱
    if (adata.me == -1) return false;
    matchdata.INewGame(paipu);
    #ifdef SAVEMATCHDATASTEP
        GameStep = CJsonObject("[]");
    #endif
    auto records = paipu["record"];
    auto rlen = records.GetArraySize();
    for (int i = 0; i < rlen; i ++ ){
        auto oner = records[i];
        matchdata.INewRound(oner);
        #ifdef SAVEMATCHDATASTEP
            RoundStep = CJsonObject("[]");
            RoundStep.Add(matchdata.tojson());
        #endif
        auto action = oner["action"];
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

        {
            int basenum = 9;
            assert(num2basedata[basenum] == "TOTALROUND");
            adata.basedata[basenum] ++ ;

            basenum = 10;
            assert(num2basedata[basenum] == "REACH");
            adata.basedata[basenum] += !!matchdata.data[adata.me].reach;

            basenum = 11;
            assert(num2basedata[basenum] == "FULU1");
            int fulu = matchdata.data[adata.me].fulu();
            if (fulu) adata.basedata[basenum - 1 + fulu] ++ ;

            basenum = 33;
            assert(num2basedata[basenum] == "ZHUANG");
            if (matchdata.east == adata.me) adata.basedata[basenum] ++ ;
            
        }

    }
    #ifdef SAVEMATCHDATASTEP
        TotalStep.Add(GameStep);
    #endif

    {
        int basenum = 0;
        assert(num2basedata[basenum] == "TOTALGAME");
        adata.basedata[basenum] ++ ;
        
        basenum = 1;
        assert(num2basedata[basenum] == "#1");
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
                auto points = records[i]["point"];
                for (int j = 0; j < points.GetArraySize(); j ++ ){
                    int point;
                    points.Get(j, point);
                    vec.push_back(point);
                }
            }
            else break;
        }
        auto alrank = Algo::getrank(vec, adata.me);

        basenum = 5;
        assert(num2basedata[basenum] == "ALBAO1");
        adata.basedata[basenum] += alrank == 1 && finalrank == 1;

        basenum = 6;
        assert(num2basedata[basenum] == "ALNI1");
        adata.basedata[basenum] += alrank != 1 && finalrank == 1;
        
        basenum = 7;
        assert(num2basedata[basenum] == "ALBI4");
        adata.basedata[basenum] += alrank == 4 && finalrank != 4;

        basenum = 8;
        assert(num2basedata[basenum] == "ALMULTITIME");
        adata.basedata[basenum] += altime > 1;

        basenum = 22;
        assert(num2basedata[basenum] == "AL#1");
        adata.basedata[basenum - 1 + alrank] ++ ;

        basenum = 32;
        assert(num2basedata[basenum] == "ALLPOINT");
        int stp, endp;
        paipu["gamedata"]["roomdata"].Get("startpoint", stp);
        pdata[adata.me].Get("finalpoint", endp);
        adata.basedata[basenum] += endp - stp;
    }

    return true;
}

void analyzemain(const std::string &dataf, const std::string &source, const std::string &id, CJsonObject &config){
    auto filter = config["filter"];
    std::vector<CJsonObject> paipus;
    auto paipuarr = Algo::ReadJSON(dataf + "/" + source + "/" + id + "/paipus.txt");
    for (int i = 0; i < paipuarr.GetArraySize(); i ++ )
        paipus.push_back(paipuarr[i]);
    PaipuAnalyzer pa = PaipuAnalyzer(filter);
    int paipunum = pa.analyze(paipus);
    #ifdef SAVEMATCHDATASTEP
        std::cout << TotalStep.ToString();
    #endif
    Out::cout << I18N::get("ANALYZER", "ANALYZEPAIPUNUM") << paipunum << I18N::get("ANALYZER", "ANALYZEPAIPUNUMAFT") << '\n';
    Out::cout << I18N::get("MISC", "DASH") << '\n';
    pa.analyzedata -> calcresult();
    pa.analyzedata -> outputresult();
}

}