#include "analyzer.h"

namespace PA{

using namespace AnalyzeResultName;

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
        assert(num2hulebasedata[basenum] == "HULE3900+");
        if (adata.me == who){
            int nowpoint = dpoint[adata.me];
            for (int i = 0; nowpoint >= pointlevel[i]; i ++ , basenum ++ );
            if (nowpoint >= pointlevel[0]) adata.hulebasedata[basenum - 1][whotype] ++ ;
        }

        basenum = 15;
        assert(num2hulebasedata[basenum] == "FANGCHONG3900+");
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

void AnalyzeData::outputonerect(const std::string &title, const std::string *res, const int length, int col){
    std::vector<std::string> str;
    std::vector<double> data;
    for (int I = 0; I < length; I ++ ){
        auto &i = res[I];
        str.push_back(I18N::get("ADRESULT", i) + I18N::get("MISC", "COLON"));
        for (unsigned j = 0; j < RESULTNAMENUM; j ++ )
            if (i == num2result[j]){
                data.push_back(result[j]);
                break;
            }
    }

    assert(str.size() == data.size());
    std::cout << title << "\n";
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
        std::cout << (i ? '-' : ' ');
    for (int I = 0; I < iinline * lines; I ++ ){
        if (!(I % iinline))
            std::cout << "\n|";
        int i = I / iinline + lines * (I % iinline);
        if (i >= len){
            for (int i = 0; i < maxwidth; i ++ )
                std::cout << (i == maxwidth - 1 ? '|' : ' ');
            continue;
        }
        int width = Algo::getdisplaywidth(str[i]) + Algo::getdisplaywidth(data[i]) + 1;
        std::cout << str[i];
        for (int i = maxwidth - width; i; i -- )
            std::cout << ' ';
        isfinite(data[i]) ? std::cout << data[i] : std::cout << '-';
        std::cout << '|';
    }
    std::cout << '\n';
    for (int i = 0; i < maxwidth * iinline; i ++ )
        std::cout << (i ? '-' : ' ');
    std::cout << '\n';

}

AnalyzeData::AnalyzeData(){
    basedata.clear();
    basedata.resize(BASEDATANUM);
    yakudata.clear();
    yakudata.resize(YAKUDATANUM);
    for (auto &i : yakudata)
        i.resize(YAKUDATANUM);
    
    hulebasedata.clear();
    hulebasedata.resize(HULEBASEDATANUM);
    for (auto &i : hulebasedata)
        i.resize(HULEHANDTYPENUM);
    reachbasedata.clear();
    reachbasedata.resize(REACHBASEDATANUM);
    for (auto &i : reachbasedata)
        i.resize(REACHTYPENUM);
    huleyakubasedata.clear();
    huleyakubasedata.resize(HULEYAKUBASEDATANUM);
    for (auto &i : huleyakubasedata){
        i.resize(YAKUDATANUM);
        for (auto &j : i)
            j.resize(HULEHANDTYPENUM);
    }
    me = -1;
}

int AnalyzeData::gethandtype(const MatchPlayerData &pdata){
    int res = -1;
    int tenpaiq = Algo::tenpaiquality(pdata);
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
    result.resize(RESULTNAMENUM);

    for (auto &i : hulebasedata)
        makehanddata(i);
    for (auto &i : huleyakubasedata)
        for (auto &j : i)
            makehanddata(j);
    for (auto &i : reachbasedata){
        int b0 = 0, b1 = 1, b2 = 2;
        assert(num2reachtype[b0] == "GOOD");
        assert(num2reachtype[b1] == "BAD");
        assert(num2reachtype[b2] == "ALL");
        i[b2] = i[b0] + i[b1];
    }
    
    int resnum, bnum1, bnum2;
    int hunum11, hunum12, hunum21, hunum22, yakunum1, yakunum2, yakunum3;
    int rnum11, rnum12, rnum21, rnum22;

    resnum = 0;
    bnum1 = 0;
    bnum2 = 1;
    assert(num2result[resnum] == "#1R");
    assert(num2basedata[bnum1] == "TOTALGAME");
    assert(num2basedata[bnum2] == "#1");
    for (int i = 0; i < 4; i ++ )
        result[resnum + i] = 1.0 * basedata[bnum2 + i] / basedata[bnum1];
    resnum = bnum1 = bnum2 = hunum11 = hunum12 = hunum21 = hunum22 = yakunum1 = yakunum2 = yakunum3 = INT_MAX;

    resnum = 4;
    bnum1 = 22;
    bnum2 = 5;
    assert(num2result[resnum] == "AL#1#1R");
    assert(num2basedata[bnum1] == "AL#1");
    assert(num2basedata[bnum2] == "ALBAO1");
    result[resnum] = 1.0 * basedata[bnum2] / basedata[bnum1];
    resnum = bnum1 = bnum2 = hunum11 = hunum12 = hunum21 = hunum22 = yakunum1 = yakunum2 = yakunum3 = INT_MAX;

    resnum = 5;
    bnum1 = 23;
    bnum2 = 6;
    assert(num2result[resnum] == "AL#234#1R");
    assert(num2basedata[bnum1] == "AL#2");
    assert(num2basedata[bnum2] == "ALNI1");
    result[resnum] = 1.0 * basedata[bnum2] / (basedata[bnum1] + basedata[bnum1 + 1] + basedata[bnum1 + 2]);
    resnum = bnum1 = bnum2 = hunum11 = hunum12 = hunum21 = hunum22 = yakunum1 = yakunum2 = yakunum3 = INT_MAX;

    resnum = 6;
    bnum1 = 25;
    bnum2 = 7;
    assert(num2result[resnum] == "AL#4#123R");
    assert(num2basedata[bnum1] == "AL#4");
    assert(num2basedata[bnum2] == "ALBI4");
    result[resnum] = 1.0 * basedata[bnum2] / basedata[bnum1];
    resnum = bnum1 = bnum2 = hunum11 = hunum12 = hunum21 = hunum22 = yakunum1 = yakunum2 = yakunum3 = INT_MAX;

    resnum = 7;
    bnum1 = 0;
    bnum2 = 8;
    assert(num2result[resnum] == "ALAL+R");
    assert(num2basedata[bnum1] == "TOTALGAME");
    assert(num2basedata[bnum2] == "ALMULTITIME");
    result[resnum] = 1.0 * basedata[bnum2] / basedata[bnum1];
    resnum = bnum1 = bnum2 = hunum11 = hunum12 = hunum21 = hunum22 = yakunum1 = yakunum2 = yakunum3 = INT_MAX;
    
    resnum = 8;
    bnum1 = 9;
    hunum11 = 0;
    hunum12 = 17;
    assert(num2result[resnum] == "HULER");
    assert(num2basedata[bnum1] == "TOTALROUND");
    assert(num2hulebasedata[hunum11] == "HULE");
    assert(num2hulehandtype[hunum12] == "ALL");
    result[resnum] = 1.0 * hulebasedata[hunum11][hunum12] / basedata[bnum1];
    resnum = bnum1 = bnum2 = hunum11 = hunum12 = hunum21 = hunum22 = yakunum1 = yakunum2 = yakunum3 = INT_MAX;

    resnum = 9;
    hunum11 = 0;
    hunum12 = 17;
    hunum21 = 2;
    hunum22 = 17;
    assert(num2result[resnum] == "ZIMOR");
    assert(num2hulebasedata[hunum11] == "HULE");
    assert(num2hulehandtype[hunum12] == "ALL");
    assert(num2hulebasedata[hunum21] == "ZIMO");
    assert(num2hulehandtype[hunum22] == "ALL");
    result[resnum] = 1.0 * hulebasedata[hunum21][hunum22] / hulebasedata[hunum11][hunum12];
    resnum = bnum1 = bnum2 = hunum11 = hunum12 = hunum21 = hunum22 = yakunum1 = yakunum2 = yakunum3 = INT_MAX;

    resnum = 10;
    bnum1 = 9;
    hunum11 = 1;
    hunum12 = 17;
    assert(num2result[resnum] == "CHONGR");
    assert(num2basedata[bnum1] == "TOTALROUND");
    assert(num2hulebasedata[hunum11] == "FANGCHONG");
    assert(num2hulehandtype[hunum12] == "ALL");
    result[resnum] = 1.0 * hulebasedata[hunum11][hunum12] / basedata[bnum1];
    resnum = bnum1 = bnum2 = hunum11 = hunum12 = hunum21 = hunum22 = yakunum1 = yakunum2 = yakunum3 = INT_MAX;

    resnum = 11;
    bnum1 = 9;
    bnum2 = 10;
    assert(num2result[resnum] == "REACHR");
    assert(num2basedata[bnum1] == "TOTALROUND");
    assert(num2basedata[bnum2] == "REACH");
    result[resnum] += 1.0 * basedata[bnum2] / basedata[bnum1];
    resnum = bnum1 = bnum2 = hunum11 = hunum12 = hunum21 = hunum22 = yakunum1 = yakunum2 = yakunum3 = INT_MAX;

    resnum = 12;
    bnum1 = 9;
    bnum2 = 11;
    assert(num2result[resnum] == "FULUR");
    assert(num2basedata[bnum1] == "TOTALROUND");
    assert(num2basedata[bnum2] == "FULU1");
    for (int i = 0; i < 4; i ++ )
        result[resnum] += 1.0 * basedata[bnum2 + i] / basedata[bnum1];
    resnum = bnum1 = bnum2 = hunum11 = hunum12 = hunum21 = hunum22 = yakunum1 = yakunum2 = yakunum3 = INT_MAX;

    resnum = 13;
    hunum11 = 0;
    hunum12 = 17;
    hunum21 = 0;
    hunum22 = 2;
    assert(num2result[resnum] == "DAMAHULER");
    assert(num2hulebasedata[hunum11] == "HULE");
    assert(num2hulehandtype[hunum12] == "ALL");
    assert(num2hulebasedata[hunum21] == "HULE");
    assert(num2hulehandtype[hunum22] == "DAMA");
    result[resnum] = 1.0 * hulebasedata[hunum21][hunum22] / hulebasedata[hunum11][hunum12];
    resnum = bnum1 = bnum2 = hunum11 = hunum12 = hunum21 = hunum22 = yakunum1 = yakunum2 = yakunum3 = INT_MAX;

    resnum = 14;
    hunum11 = 1;
    hunum12 = 17;
    hunum21 = 33;
    hunum22 = 17;
    assert(num2result[resnum] == "CHONGLEDAMAR");
    assert(num2hulebasedata[hunum11] == "FANGCHONG");
    assert(num2hulehandtype[hunum12] == "ALL");
    assert(num2hulebasedata[hunum21] == "CHONGLEDAMA");
    assert(num2hulehandtype[hunum22] == "ALL");
    result[resnum] = 1.0 * hulebasedata[hunum21][hunum22] / hulebasedata[hunum11][hunum12];
    resnum = bnum1 = bnum2 = hunum11 = hunum12 = hunum21 = hunum22 = yakunum1 = yakunum2 = yakunum3 = INT_MAX;

    resnum = 15;
    hunum11 = 0;
    hunum12 = 17;
    hunum21 = 0;
    hunum22 = 12;
    assert(num2result[resnum] == "FULUHULER");
    assert(num2hulebasedata[hunum11] == "HULE");
    assert(num2hulehandtype[hunum12] == "ALL");
    assert(num2hulebasedata[hunum21] == "HULE");
    assert(num2hulehandtype[hunum22] == "FULU");
    result[resnum] = 1.0 * hulebasedata[hunum21][hunum22] / hulebasedata[hunum11][hunum12];
    resnum = bnum1 = bnum2 = hunum11 = hunum12 = hunum21 = hunum22 = yakunum1 = yakunum2 = yakunum3 = INT_MAX;
        
    resnum = 16;
    hunum11 = 1;
    hunum12 = 17;
    hunum21 = 1;
    hunum22 = 14;
    assert(num2result[resnum] == "FULUCHONGR");
    assert(num2hulebasedata[hunum11] == "FANGCHONG");
    assert(num2hulehandtype[hunum12] == "ALL");
    assert(num2hulebasedata[hunum21] == "FANGCHONG");
    assert(num2hulehandtype[hunum22] == "ALLFULU");
    result[resnum] = 1.0 * hulebasedata[hunum21][hunum22] / hulebasedata[hunum11][hunum12];
    resnum = bnum1 = bnum2 = hunum11 = hunum12 = hunum21 = hunum22 = yakunum1 = yakunum2 = yakunum3 = INT_MAX;

    resnum = 17;
    hunum11 = 0;
    hunum12 = 17;
    hunum21 = 4;
    hunum22 = 17;
    assert(num2result[resnum] == "HULEP");
    assert(num2hulebasedata[hunum11] == "HULE");
    assert(num2hulehandtype[hunum12] == "ALL");
    assert(num2hulebasedata[hunum21] == "HULEPOINT");
    assert(num2hulehandtype[hunum22] == "ALL");
    result[resnum] = 1.0 * hulebasedata[hunum21][hunum22] / hulebasedata[hunum11][hunum12];
    resnum = bnum1 = bnum2 = hunum11 = hunum12 = hunum21 = hunum22 = yakunum1 = yakunum2 = yakunum3 = INT_MAX;
    
    resnum = 18;
    hunum11 = 1;
    hunum12 = 17;
    hunum21 = 6;
    hunum22 = 17;
    assert(num2result[resnum] == "CHONGP");
    assert(num2hulebasedata[hunum11] == "FANGCHONG");
    assert(num2hulehandtype[hunum12] == "ALL");
    assert(num2hulebasedata[hunum21] == "FANGCHONGPOINT");
    assert(num2hulehandtype[hunum22] == "ALL");
    result[resnum] = 1.0 * hulebasedata[hunum21][hunum22] / hulebasedata[hunum11][hunum12];
    resnum = bnum1 = bnum2 = hunum11 = hunum12 = hunum21 = hunum22 = yakunum1 = yakunum2 = yakunum3 = INT_MAX;
    
    resnum = 19;
    hunum11 = 0;
    hunum12 = 17;
    hunum21 = 5;
    hunum22 = 17;
    assert(num2result[resnum] == "HULESU");
    assert(num2hulebasedata[hunum11] == "HULE");
    assert(num2hulehandtype[hunum12] == "ALL");
    assert(num2hulebasedata[hunum21] == "HULESUDIAN");
    assert(num2hulehandtype[hunum22] == "ALL");
    result[resnum] = 1.0 * hulebasedata[hunum21][hunum22] / hulebasedata[hunum11][hunum12];
    resnum = bnum1 = bnum2 = hunum11 = hunum12 = hunum21 = hunum22 = yakunum1 = yakunum2 = yakunum3 = INT_MAX;
    
    resnum = 20;
    hunum11 = 1;
    hunum12 = 17;
    hunum21 = 7;
    hunum22 = 17;
    assert(num2result[resnum] == "CHONGSU");
    assert(num2hulebasedata[hunum11] == "FANGCHONG");
    assert(num2hulehandtype[hunum12] == "ALL");
    assert(num2hulebasedata[hunum21] == "FANGCHONGSUDIAN");
    assert(num2hulehandtype[hunum22] == "ALL");
    result[resnum] = 1.0 * hulebasedata[hunum21][hunum22] / hulebasedata[hunum11][hunum12];
    resnum = bnum1 = bnum2 = hunum11 = hunum12 = hunum21 = hunum22 = yakunum1 = yakunum2 = yakunum3 = INT_MAX;

    resnum = 21;
    hunum11 = 0;
    hunum12 = 2;
    hunum21 = 4;
    hunum22 = 2;
    assert(num2result[resnum] == "DAMAHULEP");
    assert(num2hulebasedata[hunum11] == "HULE");
    assert(num2hulehandtype[hunum12] == "DAMA");
    assert(num2hulebasedata[hunum21] == "HULEPOINT");
    assert(num2hulehandtype[hunum22] == "DAMA");
    result[resnum] = 1.0 * hulebasedata[hunum21][hunum22] / hulebasedata[hunum11][hunum12];
    resnum = bnum1 = bnum2 = hunum11 = hunum12 = hunum21 = hunum22 = yakunum1 = yakunum2 = yakunum3 = INT_MAX;

    resnum = 22;
    hunum11 = 33;
    hunum12 = 17;
    hunum21 = 38;
    hunum22 = 17;
    assert(num2result[resnum] == "CHONGLEDAMAP");
    assert(num2hulebasedata[hunum11] == "CHONGLEDAMA");
    assert(num2hulehandtype[hunum12] == "ALL");
    assert(num2hulebasedata[hunum21] == "CHONGLEDAMAPOINT");
    assert(num2hulehandtype[hunum22] == "ALL");
    result[resnum] = 1.0 * hulebasedata[hunum21][hunum22] / hulebasedata[hunum11][hunum12];
    resnum = bnum1 = bnum2 = hunum11 = hunum12 = hunum21 = hunum22 = yakunum1 = yakunum2 = yakunum3 = INT_MAX;

    resnum = 23;
    hunum11 = 0;
    hunum12 = 17;
    hunum21 = 12;
    hunum22 = 17;
    assert(num2result[resnum] == "HULE3900+R");
    assert(num2hulebasedata[hunum11] == "HULE");
    assert(num2hulehandtype[hunum12] == "ALL");
    assert(num2hulebasedata[hunum21] == "HULE3900+");
    assert(num2hulehandtype[hunum22] == "ALL");
    for (int i = 0; i < 3; i ++ )
        result[resnum + i] = 1.0 * hulebasedata[hunum21 + i][hunum22] / hulebasedata[hunum11][hunum12];
    for (int i = 2; i >= 0; i -- )
        result[resnum + i] += result[resnum + i + 1];
    resnum = bnum1 = bnum2 = hunum11 = hunum12 = hunum21 = hunum22 = yakunum1 = yakunum2 = yakunum3 = INT_MAX;

    resnum = 26;
    hunum11 = 1;
    hunum12 = 17;
    hunum21 = 15;
    hunum22 = 17;
    assert(num2result[resnum] == "CHONG3900+R");
    assert(num2hulebasedata[hunum11] == "FANGCHONG");
    assert(num2hulehandtype[hunum12] == "ALL");
    assert(num2hulebasedata[hunum21] == "FANGCHONG3900+");
    assert(num2hulehandtype[hunum22] == "ALL");
    for (int i = 0; i < 3; i ++ )
        result[resnum + i] = 1.0 * hulebasedata[hunum21 + i][hunum22] / hulebasedata[hunum11][hunum12];
    for (int i = 2; i >= 0; i -- )
        result[resnum + i] += result[resnum + i + 1];
    resnum = bnum1 = bnum2 = hunum11 = hunum12 = hunum21 = hunum22 = yakunum1 = yakunum2 = yakunum3 = INT_MAX;

    resnum = 29;
    hunum11 = 0;
    hunum12 = 17;
    hunum21 = 18;
    hunum22 = 17;
    assert(num2result[resnum] == "HULECC");
    assert(num2hulebasedata[hunum11] == "HULE");
    assert(num2hulehandtype[hunum12] == "ALL");
    assert(num2hulebasedata[hunum21] == "HULECIRCLE");
    assert(num2hulehandtype[hunum22] == "ALL");
    result[resnum] = 1.0 * hulebasedata[hunum21][hunum22] / hulebasedata[hunum11][hunum12];
    resnum = bnum1 = bnum2 = hunum11 = hunum12 = hunum21 = hunum22 = yakunum1 = yakunum2 = yakunum3 = INT_MAX;

    resnum = 30;
    hunum11 = 1;
    hunum12 = 17;
    hunum21 = 19;
    hunum22 = 17;
    assert(num2result[resnum] == "CHONGMYCC");
    assert(num2hulebasedata[hunum11] == "FANGCHONG");
    assert(num2hulehandtype[hunum12] == "ALL");
    assert(num2hulebasedata[hunum21] == "FANGCHONGMYCIRCLE");
    assert(num2hulehandtype[hunum22] == "ALL");
    result[resnum] = 1.0 * hulebasedata[hunum21][hunum22] / hulebasedata[hunum11][hunum12];
    resnum = bnum1 = bnum2 = hunum11 = hunum12 = hunum21 = hunum22 = yakunum1 = yakunum2 = yakunum3 = INT_MAX;

    resnum = 31;
    hunum11 = 1;
    hunum12 = 17;
    hunum21 = 31;
    hunum22 = 17;
    assert(num2result[resnum] == "CHONGHISCC");
    assert(num2hulebasedata[hunum11] == "FANGCHONG");
    assert(num2hulehandtype[hunum12] == "ALL");
    assert(num2hulebasedata[hunum21] == "FANGCHONGHISCIRCLE");
    assert(num2hulehandtype[hunum22] == "ALL");
    result[resnum] = 1.0 * hulebasedata[hunum21][hunum22] / hulebasedata[hunum11][hunum12];
    resnum = bnum1 = bnum2 = hunum11 = hunum12 = hunum21 = hunum22 = yakunum1 = yakunum2 = yakunum3 = INT_MAX;
    
    resnum = 32;
    hunum11 = 1;
    hunum12 = 17;
    bnum2 = 15;
    assert(num2result[resnum] == "CHONGSHANTEN");
    assert(num2hulebasedata[hunum11] == "FANGCHONG");
    assert(num2hulehandtype[hunum12] == "ALL");
    assert(num2basedata[bnum2] == "FANGCHONGSHANTEN0");
    for (int i = 0; i <= 6; i ++ )
        result[resnum] += 1.0 * i * basedata[bnum2 + i] / hulebasedata[hunum11][hunum12];
    resnum = bnum1 = bnum2 = hunum11 = hunum12 = hunum21 = hunum22 = yakunum1 = yakunum2 = yakunum3 = INT_MAX;
    
    resnum = 33;
    hunum11 = 0;
    hunum12 = 12;
    hunum21 = 18;
    hunum22 = 12;
    assert(num2result[resnum] == "FULUHULECC");
    assert(num2hulebasedata[hunum11] == "HULE");
    assert(num2hulehandtype[hunum12] == "FULU");
    assert(num2hulebasedata[hunum21] == "HULECIRCLE");
    assert(num2hulehandtype[hunum22] == "FULU");
    result[resnum] = 1.0 * hulebasedata[hunum21][hunum22] / hulebasedata[hunum11][hunum12];
    resnum = bnum1 = bnum2 = hunum11 = hunum12 = hunum21 = hunum22 = yakunum1 = yakunum2 = yakunum3 = INT_MAX;
    
    resnum = 34;
    hunum11 = 0;
    hunum12 = 15;
    hunum21 = 18;
    hunum22 = 15;
    assert(num2result[resnum] == "MENQINGHULECC");
    assert(num2hulebasedata[hunum11] == "HULE");
    assert(num2hulehandtype[hunum12] == "ALLDAMA");
    assert(num2hulebasedata[hunum21] == "HULECIRCLE");
    assert(num2hulehandtype[hunum22] == "ALLDAMA");
    result[resnum] = 1.0 * (hulebasedata[hunum21][hunum22] + hulebasedata[hunum21][hunum22 + 1]) / 
                           (hulebasedata[hunum11][hunum12] + hulebasedata[hunum11][hunum12 + 1]);
    resnum = bnum1 = bnum2 = hunum11 = hunum12 = hunum21 = hunum22 = yakunum1 = yakunum2 = yakunum3 = INT_MAX;
    
    resnum = 35;
    hunum11 = 0;
    hunum12 = 17;
    hunum21 = 0;
    hunum22 = 16;
    assert(num2result[resnum] == "REACHINHULER");
    assert(num2hulebasedata[hunum11] == "HULE");
    assert(num2hulehandtype[hunum12] == "ALL");
    assert(num2hulebasedata[hunum21] == "HULE");
    assert(num2hulehandtype[hunum22] == "ALLREACH");
    result[resnum] += 1.0 * hulebasedata[hunum21][hunum22] / hulebasedata[hunum11][hunum12];
    resnum = bnum1 = bnum2 = hunum11 = hunum12 = hunum21 = hunum22 = yakunum1 = yakunum2 = yakunum3 = INT_MAX;

    resnum = 36;
    hunum11 = 0;
    hunum12 = 17;
    yakunum1 = 0;
    yakunum2 = 8;
    yakunum3 = 17;
    assert(num2result[resnum] == "TANYAOHULER");
    assert(num2hulebasedata[hunum11] == "HULE");
    assert(num2hulehandtype[hunum12] == "ALL");
    assert(num2huleyakubasedata[yakunum1] == "HULEYAKU");
    assert(num2yakudata[yakunum2] == "TANYAO");
    assert(num2hulehandtype[yakunum3] == "ALL");
    result[resnum] = 1.0 * huleyakubasedata[yakunum1][yakunum2][yakunum3] / hulebasedata[hunum11][hunum12];
    resnum = bnum1 = bnum2 = hunum11 = hunum12 = hunum21 = hunum22 = yakunum1 = yakunum2 = yakunum3 = INT_MAX;

    resnum = 37;
    hunum11 = 0;
    hunum12 = 17;
    yakunum1 = 0;
    yakunum2 = 7;
    yakunum3 = 17;
    assert(num2result[resnum] == "PINFUHULER");
    assert(num2hulebasedata[hunum11] == "HULE");
    assert(num2hulehandtype[hunum12] == "ALL");
    assert(num2huleyakubasedata[yakunum1] == "HULEYAKU");
    assert(num2yakudata[yakunum2] == "PINFU");
    assert(num2hulehandtype[yakunum3] == "ALL");
    result[resnum] = 1.0 * huleyakubasedata[yakunum1][yakunum2][yakunum3] / hulebasedata[hunum11][hunum12];
    resnum = bnum1 = bnum2 = hunum11 = hunum12 = hunum21 = hunum22 = yakunum1 = yakunum2 = yakunum3 = INT_MAX;

    resnum = 38;
    hunum11 = 0;
    hunum12 = 17;
    yakunum1 = 0;
    yakunum2 = 22;
    yakunum3 = 17;
    assert(num2result[resnum] == "CHITOIHULER");
    assert(num2hulebasedata[hunum11] == "HULE");
    assert(num2hulehandtype[hunum12] == "ALL");
    assert(num2huleyakubasedata[yakunum1] == "HULEYAKU");
    assert(num2yakudata[yakunum2] == "CHITOITSU");
    assert(num2hulehandtype[yakunum3] == "ALL");
    result[resnum] = 1.0 * huleyakubasedata[yakunum1][yakunum2][yakunum3] / hulebasedata[hunum11][hunum12];
    resnum = bnum1 = bnum2 = hunum11 = hunum12 = hunum21 = hunum22 = yakunum1 = yakunum2 = yakunum3 = INT_MAX;

    resnum = 39;
    hunum11 = 0;
    hunum12 = 17;
    yakunum1 = 0;
    yakunum2 = 28;
    yakunum3 = 17;
    assert(num2result[resnum] == "TOITOIHULER");
    assert(num2hulebasedata[hunum11] == "HULE");
    assert(num2hulehandtype[hunum12] == "ALL");
    assert(num2huleyakubasedata[yakunum1] == "HULEYAKU");
    assert(num2yakudata[yakunum2] == "TOITOI");
    assert(num2hulehandtype[yakunum3] == "ALL");
    result[resnum] = 1.0 * huleyakubasedata[yakunum1][yakunum2][yakunum3] / hulebasedata[hunum11][hunum12];
    resnum = bnum1 = bnum2 = hunum11 = hunum12 = hunum21 = hunum22 = yakunum1 = yakunum2 = yakunum3 = INT_MAX;

    resnum = 40;
    hunum11 = 0;
    hunum12 = 17;
    yakunum1 = 0;
    yakunum2 = 34;
    yakunum3 = 17;
    assert(num2result[resnum] == "RANSHOUHULER");
    assert(num2hulebasedata[hunum11] == "HULE");
    assert(num2hulehandtype[hunum12] == "ALL");
    assert(num2huleyakubasedata[yakunum1] == "HULEYAKU");
    assert(num2yakudata[yakunum2] == "HONYITSU");
    assert(num2hulehandtype[yakunum3] == "ALL");
    result[resnum] = 1.0 * (huleyakubasedata[yakunum1][yakunum2][yakunum3] + huleyakubasedata[yakunum1][yakunum2 + 1][yakunum3])
                         / hulebasedata[hunum11][hunum12];
    resnum = bnum1 = bnum2 = hunum11 = hunum12 = hunum21 = hunum22 = yakunum1 = yakunum2 = yakunum3 = INT_MAX;

    resnum = 41;
    hunum11 = 0;
    hunum12 = 17;
    yakunum1 = 0;
    yakunum2 = 54;
    yakunum3 = 17;
    assert(num2result[resnum] == "AKAA");
    assert(num2hulebasedata[hunum11] == "HULE");
    assert(num2hulehandtype[hunum12] == "ALL");
    assert(num2huleyakubasedata[yakunum1] == "HULEYAKU");
    assert(num2yakudata[yakunum2] == "AKA");
    assert(num2hulehandtype[yakunum3] == "ALL");
    result[resnum] = 1.0 * huleyakubasedata[yakunum1][yakunum2][yakunum3] / hulebasedata[hunum11][hunum12];
    resnum = bnum1 = bnum2 = hunum11 = hunum12 = hunum21 = hunum22 = yakunum1 = yakunum2 = yakunum3 = INT_MAX;

    resnum = 42;
    hunum11 = 0;
    hunum12 = 17;
    yakunum1 = 0;
    yakunum2 = 52;
    yakunum3 = 17;
    assert(num2result[resnum] == "DORAA");
    assert(num2hulebasedata[hunum11] == "HULE");
    assert(num2hulehandtype[hunum12] == "ALL");
    assert(num2huleyakubasedata[yakunum1] == "HULEYAKU");
    assert(num2yakudata[yakunum2] == "DORA");
    assert(num2hulehandtype[yakunum3] == "ALL");
    result[resnum] = 1.0 * huleyakubasedata[yakunum1][yakunum2][yakunum3] / hulebasedata[hunum11][hunum12];
    resnum = bnum1 = bnum2 = hunum11 = hunum12 = hunum21 = hunum22 = yakunum1 = yakunum2 = yakunum3 = INT_MAX;

    resnum = 43;
    hunum11 = 0;
    hunum12 = 16;
    yakunum1 = 0;
    yakunum2 = 53;
    yakunum3 = 16;
    assert(num2result[resnum] == "URAA");
    assert(num2hulebasedata[hunum11] == "HULE");
    assert(num2hulehandtype[hunum12] == "ALLREACH");
    assert(num2huleyakubasedata[yakunum1] == "HULEYAKU");
    assert(num2yakudata[yakunum2] == "URA");
    assert(num2hulehandtype[yakunum3] == "ALLREACH");
    result[resnum] = 1.0 * huleyakubasedata[yakunum1][yakunum2][yakunum3] / hulebasedata[hunum11][hunum12];
    resnum = bnum1 = bnum2 = hunum11 = hunum12 = hunum21 = hunum22 = yakunum1 = yakunum2 = yakunum3 = INT_MAX;

    resnum = 44;
    hunum11 = 0;
    hunum12 = 17;
    yakunum1 = 0;
    yakunum2 = 52;
    yakunum3 = 17;
    assert(num2result[resnum] == "ALLDORAA");
    assert(num2hulebasedata[hunum11] == "HULE");
    assert(num2hulehandtype[hunum12] == "ALL");
    assert(num2huleyakubasedata[yakunum1] == "HULEYAKU");
    assert(num2yakudata[yakunum2] == "DORA");
    assert(num2hulehandtype[yakunum3] == "ALL");
    for (int i = 0; i < 3; i ++ )
        result[resnum] += 1.0 * huleyakubasedata[yakunum1][yakunum2 + i][yakunum3] / hulebasedata[hunum11][hunum12];
    resnum = bnum1 = bnum2 = hunum11 = hunum12 = hunum21 = hunum22 = yakunum1 = yakunum2 = yakunum3 = INT_MAX;

    resnum = 45;
    hunum11 = 0;
    hunum12 = 16;
    yakunum1 = 0;
    yakunum2 = 2;
    yakunum3 = 17;
    assert(num2result[resnum] == "YIPATSUHULER");
    assert(num2hulebasedata[hunum11] == "HULE");
    assert(num2hulehandtype[hunum12] == "ALLREACH");
    assert(num2huleyakubasedata[yakunum1] == "HULEYAKU");
    assert(num2yakudata[yakunum2] == "YIPATSU");
    assert(num2hulehandtype[yakunum3] == "ALL");
    result[resnum] = 1.0 * huleyakubasedata[yakunum1][yakunum2][yakunum3] / hulebasedata[hunum11][hunum12];
    resnum = bnum1 = bnum2 = hunum11 = hunum12 = hunum21 = hunum22 = yakunum1 = yakunum2 = yakunum3 = INT_MAX;

    resnum = 46;
    hunum11 = 0;
    hunum12 = 17;
    hunum21 = 25;
    hunum22 = 17;
    assert(num2result[resnum] == "OYAHULER");
    assert(num2hulebasedata[hunum11] == "HULE");
    assert(num2hulehandtype[hunum12] == "ALL");
    assert(num2hulebasedata[hunum21] == "ZHUANGHULE");
    assert(num2hulehandtype[hunum22] == "ALL");
    result[resnum] = 1.0 * hulebasedata[hunum21][hunum22] / hulebasedata[hunum11][hunum12];
    resnum = bnum1 = bnum2 = hunum11 = hunum12 = hunum21 = hunum22 = yakunum1 = yakunum2 = yakunum3 = INT_MAX;

    resnum = 47;
    hunum11 = 1;
    hunum12 = 17;
    yakunum1 = 1;
    yakunum2 = 1;
    yakunum3 = 17;
    assert(num2result[resnum] == "CHONGLEREACHR");
    assert(num2hulebasedata[hunum11] == "FANGCHONG");
    assert(num2hulehandtype[hunum12] == "ALL");
    assert(num2huleyakubasedata[yakunum1] == "CHONGLEYAKU");
    assert(num2yakudata[yakunum2] == "REACH");
    assert(num2hulehandtype[yakunum3] == "ALL");
    result[resnum] = 1.0 * huleyakubasedata[yakunum1][yakunum2][yakunum3] / hulebasedata[hunum11][hunum12];
    resnum = bnum1 = bnum2 = hunum11 = hunum12 = hunum21 = hunum22 = yakunum1 = yakunum2 = yakunum3 = INT_MAX;

    resnum = 48;
    hunum11 = 1;
    hunum12 = 17;
    yakunum1 = 1;
    yakunum2 = 7;
    yakunum3 = 17;
    assert(num2result[resnum] == "CHONGLEPINFUR");
    assert(num2hulebasedata[hunum11] == "FANGCHONG");
    assert(num2hulehandtype[hunum12] == "ALL");
    assert(num2huleyakubasedata[yakunum1] == "CHONGLEYAKU");
    assert(num2yakudata[yakunum2] == "PINFU");
    assert(num2hulehandtype[yakunum3] == "ALL");
    result[resnum] = 1.0 * huleyakubasedata[yakunum1][yakunum2][yakunum3] / hulebasedata[hunum11][hunum12];
    resnum = bnum1 = bnum2 = hunum11 = hunum12 = hunum21 = hunum22 = yakunum1 = yakunum2 = yakunum3 = INT_MAX;

    resnum = 49;
    hunum11 = 1;
    hunum12 = 17;
    yakunum1 = 1;
    yakunum2 = 22;
    yakunum3 = 17;
    assert(num2result[resnum] == "CHONGLECHITOIR");
    assert(num2hulebasedata[hunum11] == "FANGCHONG");
    assert(num2hulehandtype[hunum12] == "ALL");
    assert(num2huleyakubasedata[yakunum1] == "CHONGLEYAKU");
    assert(num2yakudata[yakunum2] == "CHITOITSU");
    assert(num2hulehandtype[yakunum3] == "ALL");
    result[resnum] = 1.0 * huleyakubasedata[yakunum1][yakunum2][yakunum3] / hulebasedata[hunum11][hunum12];
    resnum = bnum1 = bnum2 = hunum11 = hunum12 = hunum21 = hunum22 = yakunum1 = yakunum2 = yakunum3 = INT_MAX;

    resnum = 50;
    hunum11 = 1;
    hunum12 = 17;
    yakunum1 = 1;
    yakunum2 = 28;
    yakunum3 = 17;
    assert(num2result[resnum] == "CHONGLETOITOIR");
    assert(num2hulebasedata[hunum11] == "FANGCHONG");
    assert(num2hulehandtype[hunum12] == "ALL");
    assert(num2huleyakubasedata[yakunum1] == "CHONGLEYAKU");
    assert(num2yakudata[yakunum2] == "TOITOI");
    assert(num2hulehandtype[yakunum3] == "ALL");
    result[resnum] = 1.0 * huleyakubasedata[yakunum1][yakunum2][yakunum3] / hulebasedata[hunum11][hunum12];
    resnum = bnum1 = bnum2 = hunum11 = hunum12 = hunum21 = hunum22 = yakunum1 = yakunum2 = yakunum3 = INT_MAX;

    resnum = 51;
    hunum11 = 1;
    hunum12 = 17;
    yakunum1 = 1;
    yakunum2 = 8;
    yakunum3 = 17;
    assert(num2result[resnum] == "CHONGLETANYAOR");
    assert(num2hulebasedata[hunum11] == "FANGCHONG");
    assert(num2hulehandtype[hunum12] == "ALL");
    assert(num2huleyakubasedata[yakunum1] == "CHONGLEYAKU");
    assert(num2yakudata[yakunum2] == "TANYAO");
    assert(num2hulehandtype[yakunum3] == "ALL");
    result[resnum] = 1.0 * huleyakubasedata[yakunum1][yakunum2][yakunum3] / hulebasedata[hunum11][hunum12];
    resnum = bnum1 = bnum2 = hunum11 = hunum12 = hunum21 = hunum22 = yakunum1 = yakunum2 = yakunum3 = INT_MAX;

    resnum = 52;
    hunum11 = 1;
    hunum12 = 17;
    yakunum1 = 1;
    yakunum2 = 34;
    yakunum3 = 17;
    assert(num2result[resnum] == "CHONGLERANSHOUR");
    assert(num2hulebasedata[hunum11] == "FANGCHONG");
    assert(num2hulehandtype[hunum12] == "ALL");
    assert(num2huleyakubasedata[yakunum1] == "CHONGLEYAKU");
    assert(num2yakudata[yakunum2] == "HONYITSU");
    assert(num2hulehandtype[yakunum3] == "ALL");
    result[resnum] = 1.0 * (huleyakubasedata[yakunum1][yakunum2][yakunum3] + huleyakubasedata[yakunum1][yakunum2 + 1][yakunum3])
                         / hulebasedata[hunum11][hunum12];
    resnum = bnum1 = bnum2 = hunum11 = hunum12 = hunum21 = hunum22 = yakunum1 = yakunum2 = yakunum3 = INT_MAX;
    
    resnum = 53;
    hunum11 = 1;
    hunum12 = 17;
    hunum21 = 1;
    hunum22 = 16;
    assert(num2result[resnum] == "REACHINCHONGR");
    assert(num2hulebasedata[hunum11] == "FANGCHONG");
    assert(num2hulehandtype[hunum12] == "ALL");
    assert(num2hulebasedata[hunum21] == "FANGCHONG");
    assert(num2hulehandtype[hunum22] == "ALLREACH");
    result[resnum] += 1.0 * hulebasedata[hunum21][hunum22] / hulebasedata[hunum11][hunum12];
    resnum = bnum1 = bnum2 = hunum11 = hunum12 = hunum21 = hunum22 = yakunum1 = yakunum2 = yakunum3 = INT_MAX;

    resnum = 54;
    hunum11 = 1;
    hunum12 = 17;
    hunum21 = 29;
    hunum22 = 17;
    assert(num2result[resnum] == "CHONGLEOYAR");
    assert(num2hulebasedata[hunum11] == "FANGCHONG");
    assert(num2hulehandtype[hunum12] == "ALL");
    assert(num2hulebasedata[hunum21] == "CHONGLEZHUANG");
    assert(num2hulehandtype[hunum22] == "ALL");
    result[resnum] = 1.0 * hulebasedata[hunum21][hunum22] / hulebasedata[hunum11][hunum12];
    resnum = bnum1 = bnum2 = hunum11 = hunum12 = hunum21 = hunum22 = yakunum1 = yakunum2 = yakunum3 = INT_MAX;

    resnum = 55;
    hunum11 = 1;
    hunum12 = 17;
    yakunum1 = 1;
    yakunum2 = 2;
    yakunum3 = 17;
    assert(num2result[resnum] == "CHONGLEYIPATSUR");
    assert(num2hulebasedata[hunum11] == "FANGCHONG");
    assert(num2hulehandtype[hunum12] == "ALL");
    assert(num2huleyakubasedata[yakunum1] == "CHONGLEYAKU");
    assert(num2yakudata[yakunum2] == "YIPATSU");
    assert(num2hulehandtype[yakunum3] == "ALL");
    result[resnum] = 1.0 * huleyakubasedata[yakunum1][yakunum2][yakunum3] / hulebasedata[hunum11][hunum12];
    resnum = bnum1 = bnum2 = hunum11 = hunum12 = hunum21 = hunum22 = yakunum1 = yakunum2 = yakunum3 = INT_MAX;

    resnum = 56;
    hunum11 = 1;
    hunum12 = 17;
    hunum21 = 34;
    hunum22 = 17;
    assert(num2result[resnum] == "CHONGLEFULUR");
    assert(num2hulebasedata[hunum11] == "FANGCHONG");
    assert(num2hulehandtype[hunum12] == "ALL");
    assert(num2hulebasedata[hunum21] == "CHONGLEFULU1");
    assert(num2hulehandtype[hunum22] == "ALL");
    for (int i = 0; i < 4; i ++ )
        result[resnum] += 1.0 * hulebasedata[hunum21 + i][hunum22] / hulebasedata[hunum11][hunum12];
    resnum = bnum1 = bnum2 = hunum11 = hunum12 = hunum21 = hunum22 = yakunum1 = yakunum2 = yakunum3 = INT_MAX;
    
    resnum = 57;
    bnum1 = 9;
    hunum11 = 3;
    hunum12 = 17;
    assert(num2result[resnum] == "BEIZIMOR");
    assert(num2basedata[bnum1] == "TOTALROUND");
    assert(num2hulebasedata[hunum11] == "BEIZIMO");
    assert(num2hulehandtype[hunum12] == "ALL");
    result[resnum] = 1.0 * hulebasedata[hunum11][hunum12] / basedata[bnum1];
    resnum = bnum1 = bnum2 = hunum11 = hunum12 = hunum21 = hunum22 = yakunum1 = yakunum2 = yakunum3 = INT_MAX;

    resnum = 58;
    hunum11 = 3;
    hunum12 = 17;
    hunum21 = 10;
    hunum22 = 17;
    assert(num2result[resnum] == "BEIZIMOP");
    assert(num2hulebasedata[hunum11] == "BEIZIMO");
    assert(num2hulehandtype[hunum12] == "ALL");
    assert(num2hulebasedata[hunum21] == "BEIZIMOPOINT");
    assert(num2hulehandtype[hunum22] == "ALL");
    result[resnum] = 1.0 * hulebasedata[hunum21][hunum22] / hulebasedata[hunum11][hunum12];
    resnum = bnum1 = bnum2 = hunum11 = hunum12 = hunum21 = hunum22 = yakunum1 = yakunum2 = yakunum3 = INT_MAX;

    resnum = 59;
    hunum11 = 3;
    hunum12 = 17;
    hunum21 = 21;
    hunum22 = 17;
    assert(num2result[resnum] == "BEIZIMOMYCC");
    assert(num2hulebasedata[hunum11] == "BEIZIMO");
    assert(num2hulehandtype[hunum12] == "ALL");
    assert(num2hulebasedata[hunum21] == "BEIZIMOMYCIRCLE");
    assert(num2hulehandtype[hunum22] == "ALL");
    result[resnum] = 1.0 * hulebasedata[hunum21][hunum22] / hulebasedata[hunum11][hunum12];
    resnum = bnum1 = bnum2 = hunum11 = hunum12 = hunum21 = hunum22 = yakunum1 = yakunum2 = yakunum3 = INT_MAX;

    resnum = 60;
    hunum11 = 43;
    hunum12 = 17;
    hunum21 = 27;
    hunum22 = 17;
    assert(num2result[resnum] == "ZHAZHUANGR");
    assert(num2hulebasedata[hunum11] == "ZHUANGMEIHU");
    assert(num2hulehandtype[hunum12] == "ALL");
    assert(num2hulebasedata[hunum21] == "ZHAZHUANG");
    assert(num2hulehandtype[hunum22] == "ALL");
    result[resnum] = 1.0 * hulebasedata[hunum21][hunum22] / hulebasedata[hunum11][hunum12];
    resnum = bnum1 = bnum2 = hunum11 = hunum12 = hunum21 = hunum22 = yakunum1 = yakunum2 = yakunum3 = INT_MAX;

    resnum = 61;
    hunum11 = 27;
    hunum12 = 17;
    hunum21 = 28;
    hunum22 = 17;
    assert(num2result[resnum] == "ZHAZHUANGP");
    assert(num2hulebasedata[hunum11] == "ZHAZHUANG");
    assert(num2hulehandtype[hunum12] == "ALL");
    assert(num2hulebasedata[hunum21] == "ZHAZHUANGPOINT");
    assert(num2hulehandtype[hunum22] == "ALL");
    result[resnum] = 1.0 * hulebasedata[hunum21][hunum22] / hulebasedata[hunum11][hunum12];
    resnum = bnum1 = bnum2 = hunum11 = hunum12 = hunum21 = hunum22 = yakunum1 = yakunum2 = yakunum3 = INT_MAX;

    bnum1 = 10;
    rnum21 = 0;
    rnum22 = 2;
    assert(num2basedata[bnum1] == "REACH");
    assert(num2reachbasedata[rnum21] == "REACH");
    assert(num2reachtype[rnum22] == "ALL");
    assert(basedata[bnum1] == reachbasedata[rnum21][rnum22]);
    resnum = bnum1 = bnum2 = hunum11 = hunum12 = hunum21 = hunum22 = yakunum1 = yakunum2 = yakunum3 = rnum11 = rnum12 = rnum21 = rnum22 = INT_MAX;

    resnum = 62;
    hunum11 = 0;
    hunum12 = 16;
    hunum21 = 4;
    hunum22 = 16;
    assert(num2result[resnum] == "REACHHULEP");
    assert(num2hulebasedata[hunum11] == "HULE");
    assert(num2hulehandtype[hunum12] == "ALLREACH");
    assert(num2hulebasedata[hunum21] == "HULEPOINT");
    assert(num2hulehandtype[hunum22] == "ALLREACH");
    result[resnum] = 1.0 * hulebasedata[hunum21][hunum22] / hulebasedata[hunum11][hunum12];
    resnum = bnum1 = bnum2 = hunum11 = hunum12 = hunum21 = hunum22 = yakunum1 = yakunum2 = yakunum3 = rnum11 = rnum12 = rnum21 = rnum22 = INT_MAX;

    resnum = 63;
    hunum11 = 0;
    hunum12 = 16;
    hunum21 = 5;
    hunum22 = 16;
    assert(num2result[resnum] == "REACHHULESU");
    assert(num2hulebasedata[hunum11] == "HULE");
    assert(num2hulehandtype[hunum12] == "ALLREACH");
    assert(num2hulebasedata[hunum21] == "HULESUDIAN");
    assert(num2hulehandtype[hunum22] == "ALLREACH");
    result[resnum] = 1.0 * hulebasedata[hunum21][hunum22] / hulebasedata[hunum11][hunum12];
    resnum = bnum1 = bnum2 = hunum11 = hunum12 = hunum21 = hunum22 = yakunum1 = yakunum2 = yakunum3 = rnum11 = rnum12 = rnum21 = rnum22 = INT_MAX;

    resnum = 64;
    hunum11 = 0;
    hunum12 = 16;
    hunum21 = 12;
    hunum22 = 16;
    assert(num2result[resnum] == "REACH3900+R");
    assert(num2hulebasedata[hunum11] == "HULE");
    assert(num2hulehandtype[hunum12] == "ALLREACH");
    assert(num2hulebasedata[hunum21] == "HULE3900+");
    assert(num2hulehandtype[hunum22] == "ALLREACH");
    for (int i = 0; i < 3; i ++ )
        result[resnum + i] = 1.0 * hulebasedata[hunum21 + i][hunum22] / hulebasedata[hunum11][hunum12];
    for (int i = 2; i >= 0; i -- )
        result[resnum + i] += result[resnum + i + 1];
    resnum = bnum1 = bnum2 = hunum11 = hunum12 = hunum21 = hunum22 = yakunum1 = yakunum2 = yakunum3 = rnum11 = rnum12 = rnum21 = rnum22 = INT_MAX;

    resnum = 67;
    rnum11 = 0;
    rnum12 = 2;
    rnum21 = 1;
    rnum22 = 2;
    assert(num2result[resnum] == "REACHCC");
    assert(num2reachbasedata[rnum11] == "REACH");
    assert(num2reachtype[rnum12] == "ALL");
    assert(num2reachbasedata[rnum21] == "CIRCLE");
    assert(num2reachtype[rnum22] == "ALL");
    result[resnum] = 1.0 * reachbasedata[rnum21][rnum22] / reachbasedata[rnum11][rnum12];
    resnum = bnum1 = bnum2 = hunum11 = hunum12 = hunum21 = hunum22 = yakunum1 = yakunum2 = yakunum3 = rnum11 = rnum12 = rnum21 = rnum22 = INT_MAX;

    resnum = 68;
    rnum11 = 0;
    rnum12 = 2;
    rnum21 = 4;
    rnum22 = 2;
    assert(num2result[resnum] == "REACHPINFUR");
    assert(num2reachbasedata[rnum11] == "REACH");
    assert(num2reachtype[rnum12] == "ALL");
    assert(num2reachbasedata[rnum21] == "PINFU");
    assert(num2reachtype[rnum22] == "ALL");
    result[resnum] = 1.0 * reachbasedata[rnum21][rnum22] / reachbasedata[rnum11][rnum12];
    resnum = bnum1 = bnum2 = hunum11 = hunum12 = hunum21 = hunum22 = yakunum1 = yakunum2 = yakunum3 = rnum11 = rnum12 = rnum21 = rnum22 = INT_MAX;

    resnum = 69;
    rnum11 = 0;
    rnum12 = 2;
    rnum21 = 3;
    rnum22 = 2;
    assert(num2result[resnum] == "REACHTANYAOR");
    assert(num2reachbasedata[rnum11] == "REACH");
    assert(num2reachtype[rnum12] == "ALL");
    assert(num2reachbasedata[rnum21] == "TANYAO");
    assert(num2reachtype[rnum22] == "ALL");
    result[resnum] = 1.0 * reachbasedata[rnum21][rnum22] / reachbasedata[rnum11][rnum12];
    resnum = bnum1 = bnum2 = hunum11 = hunum12 = hunum21 = hunum22 = yakunum1 = yakunum2 = yakunum3 = rnum11 = rnum12 = rnum21 = rnum22 = INT_MAX;

    resnum = 70;
    rnum11 = 0;
    rnum12 = 2;
    rnum21 = 5;
    rnum22 = 2;
    assert(num2result[resnum] == "REACHDORA2+R");
    assert(num2reachbasedata[rnum11] == "REACH");
    assert(num2reachtype[rnum12] == "ALL");
    assert(num2reachbasedata[rnum21] == "DORA0");
    assert(num2reachtype[rnum22] == "ALL");
    for (int i = 2; i <= 26; i ++ )
        result[resnum] += 1.0 * reachbasedata[rnum21 + i][rnum22] / reachbasedata[rnum11][rnum12];
    resnum = bnum1 = bnum2 = hunum11 = hunum12 = hunum21 = hunum22 = yakunum1 = yakunum2 = yakunum3 = rnum11 = rnum12 = rnum21 = rnum22 = INT_MAX;

    resnum = 71;
    rnum11 = 0;
    rnum12 = 2;
    rnum21 = 5;
    rnum22 = 2;
    assert(num2result[resnum] == "REACHDORA3+R");
    assert(num2reachbasedata[rnum11] == "REACH");
    assert(num2reachtype[rnum12] == "ALL");
    assert(num2reachbasedata[rnum21] == "DORA0");
    assert(num2reachtype[rnum22] == "ALL");
    for (int i = 3; i <= 26; i ++ )
        result[resnum] += 1.0 * reachbasedata[rnum21 + i][rnum22] / reachbasedata[rnum11][rnum12];
    resnum = bnum1 = bnum2 = hunum11 = hunum12 = hunum21 = hunum22 = yakunum1 = yakunum2 = yakunum3 = rnum11 = rnum12 = rnum21 = rnum22 = INT_MAX;

    resnum = 72;
    rnum11 = 0;
    rnum12 = 2;
    rnum21 = 5;
    rnum22 = 2;
    assert(num2result[resnum] == "REACHDORAA");
    assert(num2reachbasedata[rnum11] == "REACH");
    assert(num2reachtype[rnum12] == "ALL");
    assert(num2reachbasedata[rnum21] == "DORA0");
    assert(num2reachtype[rnum22] == "ALL");
    for (int i = 0; i <= 26; i ++ )
        result[resnum] += 1.0 * i * reachbasedata[rnum21 + i][rnum22] / reachbasedata[rnum11][rnum12];
    resnum = bnum1 = bnum2 = hunum11 = hunum12 = hunum21 = hunum22 = yakunum1 = yakunum2 = yakunum3 = rnum11 = rnum12 = rnum21 = rnum22 = INT_MAX;

    resnum = 73;
    rnum11 = 0;
    rnum12 = 2;
    rnum21 = 32;
    rnum22 = 2;
    assert(num2result[resnum] == "FIRSTREACHR");
    assert(num2reachbasedata[rnum11] == "REACH");
    assert(num2reachtype[rnum12] == "ALL");
    assert(num2reachbasedata[rnum21] == "#1");
    assert(num2reachtype[rnum22] == "ALL");
    result[resnum] = 1.0 * reachbasedata[rnum21][rnum22] / reachbasedata[rnum11][rnum12];
    resnum = bnum1 = bnum2 = hunum11 = hunum12 = hunum21 = hunum22 = yakunum1 = yakunum2 = yakunum3 = rnum11 = rnum12 = rnum21 = rnum22 = INT_MAX;

    resnum = 74;
    hunum11 = 0;
    hunum12 = 16;
    hunum21 = 2;
    hunum22 = 16;
    assert(num2result[resnum] == "ZIMOINREACHHULER");
    assert(num2hulebasedata[hunum11] == "HULE");
    assert(num2hulehandtype[hunum12] == "ALLREACH");
    assert(num2hulebasedata[hunum21] == "ZIMO");
    assert(num2hulehandtype[hunum22] == "ALLREACH");
    result[resnum] = 1.0 * hulebasedata[hunum21][hunum22] / hulebasedata[hunum11][hunum12];
    resnum = bnum1 = bnum2 = hunum11 = hunum12 = hunum21 = hunum22 = yakunum1 = yakunum2 = yakunum3 = rnum11 = rnum12 = rnum21 = rnum22 = INT_MAX;

    resnum = 75;
    rnum11 = 0;
    rnum12 = 2;
    rnum21 = 0;
    rnum22 = 0;
    assert(num2result[resnum] == "REACHGOODR");
    assert(num2reachbasedata[rnum11] == "REACH");
    assert(num2reachtype[rnum12] == "ALL");
    assert(num2reachbasedata[rnum21] == "REACH");
    assert(num2reachtype[rnum22] == "GOOD");
    result[resnum] = 1.0 * reachbasedata[rnum21][rnum22] / reachbasedata[rnum11][rnum12];
    resnum = bnum1 = bnum2 = hunum11 = hunum12 = hunum21 = hunum22 = yakunum1 = yakunum2 = yakunum3 = rnum11 = rnum12 = rnum21 = rnum22 = INT_MAX;

    resnum = 76;
    rnum11 = 0;
    rnum12 = 0;
    hunum21 = 0;
    hunum22 = 0;
    assert(num2result[resnum] == "REACHGOODHULER");
    assert(num2reachbasedata[rnum11] == "REACH");
    assert(num2reachtype[rnum12] == "GOOD");
    assert(num2hulebasedata[hunum21] == "HULE");
    assert(num2hulehandtype[hunum22] == "REACHGOOD");
    result[resnum] = 1.0 * hulebasedata[hunum21][hunum22] / reachbasedata[rnum11][rnum12];
    resnum = bnum1 = bnum2 = hunum11 = hunum12 = hunum21 = hunum22 = yakunum1 = yakunum2 = yakunum3 = rnum11 = rnum12 = rnum21 = rnum22 = INT_MAX;

    resnum = 77;
    hunum11 = 0;
    hunum12 = 0;
    hunum21 = 4;
    hunum22 = 0;
    assert(num2result[resnum] == "REACHGOODHULEP");
    assert(num2hulebasedata[hunum11] == "HULE");
    assert(num2hulehandtype[hunum12] == "REACHGOOD");
    assert(num2hulebasedata[hunum21] == "HULEPOINT");
    assert(num2hulehandtype[hunum22] == "REACHGOOD");
    result[resnum] = 1.0 * hulebasedata[hunum21][hunum22] / hulebasedata[hunum11][hunum12];
    resnum = bnum1 = bnum2 = hunum11 = hunum12 = hunum21 = hunum22 = yakunum1 = yakunum2 = yakunum3 = rnum11 = rnum12 = rnum21 = rnum22 = INT_MAX;

    resnum = 78;
    rnum11 = 0;
    rnum12 = 0;
    hunum21 = 1;
    hunum22 = 0;
    assert(num2result[resnum] == "REACHGOODCHONGR");
    assert(num2reachbasedata[rnum11] == "REACH");
    assert(num2reachtype[rnum12] == "GOOD");
    assert(num2hulebasedata[hunum21] == "FANGCHONG");
    assert(num2hulehandtype[hunum22] == "REACHGOOD");
    result[resnum] = 1.0 * hulebasedata[hunum21][hunum22] / reachbasedata[rnum11][rnum12];
    resnum = bnum1 = bnum2 = hunum11 = hunum12 = hunum21 = hunum22 = yakunum1 = yakunum2 = yakunum3 = rnum11 = rnum12 = rnum21 = rnum22 = INT_MAX;

    resnum = 79;
    hunum11 = 1;
    hunum12 = 0;
    hunum21 = 6;
    hunum22 = 0;
    assert(num2result[resnum] == "REACHGOODCHONGP");
    assert(num2hulebasedata[hunum11] == "FANGCHONG");
    assert(num2hulehandtype[hunum12] == "REACHGOOD");
    assert(num2hulebasedata[hunum21] == "FANGCHONGPOINT");
    assert(num2hulehandtype[hunum22] == "REACHGOOD");
    result[resnum] = 1.0 * hulebasedata[hunum21][hunum22] / hulebasedata[hunum11][hunum12];
    resnum = bnum1 = bnum2 = hunum11 = hunum12 = hunum21 = hunum22 = yakunum1 = yakunum2 = yakunum3 = rnum11 = rnum12 = rnum21 = rnum22 = INT_MAX;

    resnum = 80;
    assert(num2result[resnum] == "REACHGOODPROFIT");
    rnum11 = 2;
    rnum12 = 0;
    assert(num2reachbasedata[rnum11] == "REACHDECLEARRON");
    assert(num2reachtype[rnum12] == "GOOD");
    result[resnum] += 1000.0 * reachbasedata[rnum11][rnum12];
    hunum11 = 4;
    hunum12 = 0;
    assert(num2hulebasedata[hunum11] == "HULEPOINT");
    assert(num2hulehandtype[hunum12] == "REACHGOOD");
    result[resnum] += 1.0 * hulebasedata[hunum11][hunum12];
    hunum11 = 6;
    hunum12 = 0;
    assert(num2hulebasedata[hunum11] == "FANGCHONGPOINT");
    assert(num2hulehandtype[hunum12] == "REACHGOOD");
    result[resnum] += 1.0 * hulebasedata[hunum11][hunum12];
    rnum11 = 0;
    rnum12 = 0;
    assert(num2reachbasedata[rnum11] == "REACH");
    assert(num2reachtype[rnum12] == "GOOD");
    result[resnum] /= reachbasedata[rnum11][rnum12];
    result[resnum] -= 1000;
    resnum = bnum1 = bnum2 = hunum11 = hunum12 = hunum21 = hunum22 = yakunum1 = yakunum2 = yakunum3 = rnum11 = rnum12 = rnum21 = rnum22 = INT_MAX;

    resnum = 81;
    rnum11 = 0;
    rnum12 = 2;
    rnum21 = 0;
    rnum22 = 1;
    assert(num2result[resnum] == "REACHBADR");
    assert(num2reachbasedata[rnum11] == "REACH");
    assert(num2reachtype[rnum12] == "ALL");
    assert(num2reachbasedata[rnum21] == "REACH");
    assert(num2reachtype[rnum22] == "BAD");
    result[resnum] = 1.0 * reachbasedata[rnum21][rnum22] / reachbasedata[rnum11][rnum12];
    resnum = bnum1 = bnum2 = hunum11 = hunum12 = hunum21 = hunum22 = yakunum1 = yakunum2 = yakunum3 = rnum11 = rnum12 = rnum21 = rnum22 = INT_MAX;

    resnum = 82;
    rnum11 = 0;
    rnum12 = 1;
    hunum21 = 0;
    hunum22 = 1;
    assert(num2result[resnum] == "REACHBADHULER");
    assert(num2reachbasedata[rnum11] == "REACH");
    assert(num2reachtype[rnum12] == "BAD");
    assert(num2hulebasedata[hunum21] == "HULE");
    assert(num2hulehandtype[hunum22] == "REACHBAD");
    result[resnum] = 1.0 * hulebasedata[hunum21][hunum22] / reachbasedata[rnum11][rnum12];
    resnum = bnum1 = bnum2 = hunum11 = hunum12 = hunum21 = hunum22 = yakunum1 = yakunum2 = yakunum3 = rnum11 = rnum12 = rnum21 = rnum22 = INT_MAX;

    resnum = 83;
    hunum11 = 0;
    hunum12 = 1;
    hunum21 = 4;
    hunum22 = 1;
    assert(num2result[resnum] == "REACHBADHULEP");
    assert(num2hulebasedata[hunum11] == "HULE");
    assert(num2hulehandtype[hunum12] == "REACHBAD");
    assert(num2hulebasedata[hunum21] == "HULEPOINT");
    assert(num2hulehandtype[hunum22] == "REACHBAD");
    result[resnum] = 1.0 * hulebasedata[hunum21][hunum22] / hulebasedata[hunum11][hunum12];
    resnum = bnum1 = bnum2 = hunum11 = hunum12 = hunum21 = hunum22 = yakunum1 = yakunum2 = yakunum3 = rnum11 = rnum12 = rnum21 = rnum22 = INT_MAX;

    resnum = 84;
    rnum11 = 0;
    rnum12 = 1;
    hunum21 = 1;
    hunum22 = 1;
    assert(num2result[resnum] == "REACHBADCHONGR");
    assert(num2reachbasedata[rnum11] == "REACH");
    assert(num2reachtype[rnum12] == "BAD");
    assert(num2hulebasedata[hunum21] == "FANGCHONG");
    assert(num2hulehandtype[hunum22] == "REACHBAD");
    result[resnum] = 1.0 * hulebasedata[hunum21][hunum22] / reachbasedata[rnum11][rnum12];
    resnum = bnum1 = bnum2 = hunum11 = hunum12 = hunum21 = hunum22 = yakunum1 = yakunum2 = yakunum3 = rnum11 = rnum12 = rnum21 = rnum22 = INT_MAX;

    resnum = 85;
    hunum11 = 1;
    hunum12 = 1;
    hunum21 = 6;
    hunum22 = 1;
    assert(num2result[resnum] == "REACHBADCHONGP");
    assert(num2hulebasedata[hunum11] == "FANGCHONG");
    assert(num2hulehandtype[hunum12] == "REACHBAD");
    assert(num2hulebasedata[hunum21] == "FANGCHONGPOINT");
    assert(num2hulehandtype[hunum22] == "REACHBAD");
    result[resnum] = 1.0 * hulebasedata[hunum21][hunum22] / hulebasedata[hunum11][hunum12];
    resnum = bnum1 = bnum2 = hunum11 = hunum12 = hunum21 = hunum22 = yakunum1 = yakunum2 = yakunum3 = rnum11 = rnum12 = rnum21 = rnum22 = INT_MAX;

    resnum = 86;
    assert(num2result[resnum] == "REACHBADPROFIT");
    rnum11 = 2;
    rnum12 = 1;
    assert(num2reachbasedata[rnum11] == "REACHDECLEARRON");
    assert(num2reachtype[rnum12] == "BAD");
    result[resnum] += 1000.0 * reachbasedata[rnum11][rnum12];
    hunum11 = 4;
    hunum12 = 1;
    assert(num2hulebasedata[hunum11] == "HULEPOINT");
    assert(num2hulehandtype[hunum12] == "REACHBAD");
    result[resnum] += 1.0 * hulebasedata[hunum11][hunum12];
    hunum11 = 6;
    hunum12 = 1;
    assert(num2hulebasedata[hunum11] == "FANGCHONGPOINT");
    assert(num2hulehandtype[hunum12] == "REACHBAD");
    result[resnum] += 1.0 * hulebasedata[hunum11][hunum12];
    rnum11 = 0;
    rnum12 = 1;
    assert(num2reachbasedata[rnum11] == "REACH");
    assert(num2reachtype[rnum12] == "BAD");
    result[resnum] /= reachbasedata[rnum11][rnum12];
    result[resnum] -= 1000;
    resnum = bnum1 = bnum2 = hunum11 = hunum12 = hunum21 = hunum22 = yakunum1 = yakunum2 = yakunum3 = rnum11 = rnum12 = rnum21 = rnum22 = INT_MAX;

    resnum = 87;
    assert(num2result[resnum] == "REACHPROFIT");
    rnum11 = 2;
    rnum12 = 2;
    assert(num2reachbasedata[rnum11] == "REACHDECLEARRON");
    assert(num2reachtype[rnum12] == "ALL");
    result[resnum] += 1000.0 * reachbasedata[rnum11][rnum12];
    hunum11 = 4;
    hunum12 = 16;
    assert(num2hulebasedata[hunum11] == "HULEPOINT");
    assert(num2hulehandtype[hunum12] == "ALLREACH");
    result[resnum] += 1.0 * hulebasedata[hunum11][hunum12];
    hunum11 = 6;
    hunum12 = 16;
    assert(num2hulebasedata[hunum11] == "FANGCHONGPOINT");
    assert(num2hulehandtype[hunum12] == "ALLREACH");
    result[resnum] += 1.0 * hulebasedata[hunum11][hunum12];
    rnum11 = 0;
    rnum12 = 2;
    assert(num2reachbasedata[rnum11] == "REACH");
    assert(num2reachtype[rnum12] == "ALL");
    result[resnum] /= reachbasedata[rnum11][rnum12];
    result[resnum] -= 1000;
    resnum = bnum1 = bnum2 = hunum11 = hunum12 = hunum21 = hunum22 = yakunum1 = yakunum2 = yakunum3 = rnum11 = rnum12 = rnum21 = rnum22 = INT_MAX;

    resnum = 88;
    bnum1 = 10;
    hunum21 = 1;
    hunum22 = 16;
    assert(num2result[resnum] == "CHONGINREACHR");
    assert(num2basedata[bnum1] == "REACH");
    assert(num2hulebasedata[hunum21] == "FANGCHONG");
    assert(num2hulehandtype[hunum22] == "ALLREACH");
    result[resnum] += 1.0 * hulebasedata[hunum21][hunum22] / basedata[bnum1];
    resnum = bnum1 = bnum2 = hunum11 = hunum12 = hunum21 = hunum22 = yakunum1 = yakunum2 = yakunum3 = rnum11 = rnum12 = rnum21 = rnum22 = INT_MAX;

    resnum = 89;
    bnum1 = 10;
    hunum21 = 0;
    hunum22 = 16;
    assert(num2result[resnum] == "HULEINREACHR");
    assert(num2basedata[bnum1] == "REACH");
    assert(num2hulebasedata[hunum21] == "HULE");
    assert(num2hulehandtype[hunum22] == "ALLREACH");
    result[resnum] += 1.0 * hulebasedata[hunum21][hunum22] / basedata[bnum1];
    resnum = bnum1 = bnum2 = hunum11 = hunum12 = hunum21 = hunum22 = yakunum1 = yakunum2 = yakunum3 = rnum11 = rnum12 = rnum21 = rnum22 = INT_MAX;

    resnum = 90;
    bnum1 = 26;
    bnum2 = 27;
    assert(num2result[resnum] == "LIUJUTENPAIR");
    assert(num2basedata[bnum1] == "NORMALLIUJU");
    assert(num2basedata[bnum2] == "LIUJUTENPAI");
    result[resnum] = 1.0 * basedata[bnum2] / basedata[bnum1];
    resnum = bnum1 = bnum2 = hunum11 = hunum12 = hunum21 = hunum22 = yakunum1 = yakunum2 = yakunum3 = INT_MAX;

    resnum = 91;
    bnum1 = 27;
    bnum2 = 29;
    assert(num2result[resnum] == "LIUJUINP");
    assert(num2basedata[bnum1] == "LIUJUTENPAI");
    assert(num2basedata[bnum2] == "LIUJUTENPAIPOINT");
    result[resnum] = 1.0 * basedata[bnum2] / basedata[bnum1];
    resnum = bnum1 = bnum2 = hunum11 = hunum12 = hunum21 = hunum22 = yakunum1 = yakunum2 = yakunum3 = INT_MAX;

    resnum = 92;
    bnum1 = 26;
    bnum2 = 28;
    assert(num2result[resnum] == "LIUJUNOTENR");
    assert(num2basedata[bnum1] == "NORMALLIUJU");
    assert(num2basedata[bnum2] == "LIUJUNOTEN");
    result[resnum] = 1.0 * basedata[bnum2] / basedata[bnum1];
    resnum = bnum1 = bnum2 = hunum11 = hunum12 = hunum21 = hunum22 = yakunum1 = yakunum2 = yakunum3 = INT_MAX;

    resnum = 93;
    bnum1 = 28;
    bnum2 = 30;
    assert(num2result[resnum] == "LIUJUOUTP");
    assert(num2basedata[bnum1] == "LIUJUNOTEN");
    assert(num2basedata[bnum2] == "LIUJUNOTENPOINT");
    result[resnum] = 1.0 * basedata[bnum2] / basedata[bnum1];
    resnum = bnum1 = bnum2 = hunum11 = hunum12 = hunum21 = hunum22 = yakunum1 = yakunum2 = yakunum3 = INT_MAX;

    resnum = 94;
    bnum1 = 26;
    bnum2 = 31;
    assert(num2result[resnum] == "LIUJUPROFIT");
    assert(num2basedata[bnum1] == "NORMALLIUJU");
    assert(num2basedata[bnum2] == "LIUJUPOINT");
    result[resnum] = 1.0 * basedata[bnum2] / basedata[bnum1];
    resnum = bnum1 = bnum2 = hunum11 = hunum12 = hunum21 = hunum22 = yakunum1 = yakunum2 = yakunum3 = INT_MAX;

    resnum = 95;
    bnum1 = 0;
    bnum2 = 1;
    assert(num2result[resnum] == "#A");
    assert(num2basedata[bnum1] == "TOTALGAME");
    assert(num2basedata[bnum2] == "#1");
    for (int i = 1; i <= 4; i ++ )
        result[resnum] += 1.0 * i * basedata[bnum2 + i - 1] / basedata[bnum1];
    resnum = bnum1 = bnum2 = hunum11 = hunum12 = hunum21 = hunum22 = yakunum1 = yakunum2 = yakunum3 = INT_MAX;

    resnum = 96;
    bnum1 = 9;
    assert(num2result[resnum] == "TOTALROUND");
    assert(num2basedata[bnum1] == "TOTALROUND");
    result[resnum] = basedata[bnum1];
    resnum = bnum1 = bnum2 = hunum11 = hunum12 = hunum21 = hunum22 = yakunum1 = yakunum2 = yakunum3 = INT_MAX;

    resnum = 97;
    bnum1 = 0;
    assert(num2result[resnum] == "TOTALGAME");
    assert(num2basedata[bnum1] == "TOTALGAME");
    result[resnum] = basedata[bnum1];
    resnum = bnum1 = bnum2 = hunum11 = hunum12 = hunum21 = hunum22 = yakunum1 = yakunum2 = yakunum3 = INT_MAX;

    resnum = 98;
    bnum1 = 9;
    bnum2 = 32;
    assert(num2result[resnum] == "ROUNDPROFIT");
    assert(num2basedata[bnum1] == "TOTALROUND");
    assert(num2basedata[bnum2] == "ALLPOINT");
    result[resnum] = 1.0 * basedata[bnum2] / basedata[bnum1];
    resnum = bnum1 = bnum2 = hunum11 = hunum12 = hunum21 = hunum22 = yakunum1 = yakunum2 = yakunum3 = INT_MAX;

    resnum = 99;
    bnum1 = 0;
    bnum2 = 32;
    assert(num2result[resnum] == "GAMEPROFIT");
    assert(num2basedata[bnum1] == "TOTALGAME");
    assert(num2basedata[bnum2] == "ALLPOINT");
    result[resnum] = 1.0 * basedata[bnum2] / basedata[bnum1];
    resnum = bnum1 = bnum2 = hunum11 = hunum12 = hunum21 = hunum22 = yakunum1 = yakunum2 = yakunum3 = INT_MAX;

    resnum = 100;
    bnum1 = 9;
    bnum2 = 26;
    assert(num2result[resnum] == "LIUJUR");
    assert(num2basedata[bnum1] == "TOTALROUND");
    assert(num2basedata[bnum2] == "NORMALLIUJU");
    result[resnum] = 1.0 * basedata[bnum2] / basedata[bnum1];
    resnum = bnum1 = bnum2 = hunum11 = hunum12 = hunum21 = hunum22 = yakunum1 = yakunum2 = yakunum3 = INT_MAX;
    
}

void AnalyzeData::outputresult(){
    std::cout << std::fixed << std::setprecision(4);
    int row, col;
    Algo::getconsolesize(row, col);
/* 
    int nowcol = 4;
    std::cout << I18N::get("ANALYZER", "BASEDATA") << I18N::get("MISC", "COLON") << "\n    ";
    for (unsigned i = 0; i < BASEDATANUM; i ++ ){
        auto str = num2basedata[i] + ":";
        auto num = basedata[i];
        int width = Algo::getdisplaywidth(str) + Algo::getdisplaywidth(num) + 1;
        if (width + nowcol >= col){
            std::cout << "\n    ";
            nowcol = 4;
        }
        nowcol += width;
        std::cout << str << num << ' ';
    }
    std::cout << '\n';
    std::cout << I18N::get("MISC", "DASH") << "\n";
 */
    //outputonerect(I18N::get("ANALYZER", "ALLRESULT") + I18N::get("MISC", "COLON"), num2result, RESULTNAMENUM, col);
    outputonerect(I18N::get("ANALYZER", "OVRESULT") + I18N::get("MISC", "COLON"), overviewresult, OVERVIEWRESULTNUM, col);
    outputonerect(I18N::get("ANALYZER", "HULERESULT") + I18N::get("MISC", "COLON"), huleresult, HULERESULTNUM, col);
    outputonerect(I18N::get("ANALYZER", "CHONGRESULT") + I18N::get("MISC", "COLON"), chongresult, CHONGRESULTNUM, col);
    outputonerect(I18N::get("ANALYZER", "REACHRESULT") + I18N::get("MISC", "COLON"), reachresult, REACHRESULTNUM, col);
    outputonerect(I18N::get("ANALYZER", "LIUJURESULT") + I18N::get("MISC", "COLON"), liujuresult, LIUJURESULTNUM, col);
    outputonerect(I18N::get("ANALYZER", "FULURESULT") + I18N::get("MISC", "COLON"), fuluresult, FULURESULTNUM, col);
    outputonerect(I18N::get("ANALYZER", "ALRESULT") + I18N::get("MISC", "COLON"), alresult, ALRESULTNUM, col);

    PAUSE;
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
    adata.me = -1;
    auto accountid = paipu["gamedata"]["accountid"];
    auto pdata = paipu["gamedata"]["playerdata"];
    for (int i = 0; i < 4; i ++ )
        if (accountid == pdata[i]["id"])
            adata.me = i;
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

        auto &adata = *analyzedata;
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
    PA::PaipuAnalyzer pa = PA::PaipuAnalyzer(filter);
    int paipunum = pa.analyze(paipus);
    #ifdef SAVEMATCHDATASTEP
        std::cout << TotalStep.ToString();
    #endif
    std::cout << I18N::get("ANALYZER", "ANALYZEPAIPUNUM") << paipunum << I18N::get("ANALYZER", "ANALYZEPAIPUNUMAFT") << '\n';
    std::cout << I18N::get("MISC", "DASH") << '\n';
    pa.analyzedata -> calcresult();
    pa.analyzedata -> outputresult();
}

}