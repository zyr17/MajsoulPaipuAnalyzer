#include "main.h"

#ifdef SAVEMATCHDATASTEP
    CJsonObject TotalStep("[]"), GameStep("[]"), RoundStep("[]");
#endif

namespace PA{

Consts::Consts(){
    for (int i = 0; i < TILENUM; i ++ )
        tile2nummap[num2tile[i]] = i;
}

int Consts::tile2num(const std::string &key){
    auto it = tile2nummap.find(key);
    if (it != tile2nummap.end())
        return it -> second;
    return -1;
}

long long Consts::strptime(const std::string &str){
    int y, M, d, H, m, s;
    sscanf(str.c_str(), "%d-%d-%d %d:%d:%d", &y, &M, &d, &H, &m, &s);
    tm time;
    time.tm_year = y - 1900;
    time.tm_mon = M - 1;
    time.tm_mday = d;
    time.tm_hour = H;
    time.tm_min = m;
    time.tm_sec = s;
    return mktime(&time);
}

std::vector<std::string> Consts::split(std::string &str, char c){
    std::vector<std::string> res;
    std::string ts = "";
    for (auto i : str)
        if (i == c){
            res.push_back(ts);
            ts = "";
        }
        else ts += i;
    res.push_back(ts);
    return res;
}

void Consts::changevec(std::vector<int> &vec, int pos, int replace){
    vec[pos] = replace;
    auto swap = [] (int &x, int &y) { int t = x; x = y; y = t; };
    for (int j = pos; j && vec[j - 1] > vec[j]; j -- )
        swap(vec[j - 1], vec[j]);
    for (unsigned j = pos; j + 1 < vec.size() && vec[j] > vec[j + 1]; j ++ )
        swap(vec[j], vec[j + 1]);
    if (replace == INT_MAX)
        vec.pop_back();
}

std::vector<int> Consts::calctensu(int fu, int han, int honba, int kyoutaku, bool oya, bool tsumo){
    std::vector<int> res;
    res.resize(3);
    int base = fu * 4;
    for (int i = han; i -- ; base *= 2);
    if (base > 2000) base = 2000;
    if (han > 5) base = 3000;
    if (han > 7) base = 4000;
    if (han > 10) base = 6000;
    if (han > 12) base = 8000 * (han / 13);
    if (oya && tsumo){
        res[2] = ((base * 2 - 1) / 100 + 1) * 100 + honba * 100;
        res[0] = res[2] * 3 + kyoutaku * 1000;
    }
    else if (oya){
        res[2] = ((base * 6 - 1) / 100 + 1) * 100 + honba * 300;
        res[0] = res[2] + kyoutaku * 1000;
    }
    else if (tsumo){
        res[2] = ((base - 1) / 100 + 1) * 100 + honba * 100;
        res[1] = ((base * 2 - 1) / 100 + 1) * 100 + honba * 100;
        res[0] = res[1] + res[2] * 2 + kyoutaku * 1000;
    }
    else{
        res[1] = res[2] = ((base * 4 - 1) / 100 + 1) * 100 + honba * 300;
        res[0] = res[2] + kyoutaku * 1000;
    }
    return res;
}

int Consts::getrank(std::vector<int> points, int who, int initial){
    for (int res = 1, len = points.size(); ; res ++ ){
        int nowmax = initial;
        for (int i = 0; i < len; i ++ )
            if (points[nowmax] < points[(initial + i) % len])
                nowmax = (initial + i) % len;
        points[nowmax] = INT_MIN;
        if (nowmax == who)
            return res;
    }
}

int Consts::calcshanten(MatchPlayerData &pdata, bool chitoikokushi){
    ////TODO:
    return 3;
}

Consts consts;

MatchPlayerData::MatchPlayerData(){
    score = reach = 0;
    get = -1;
}

void MatchPlayerData::clear(){
    reach = score = 0;
    get = -1;
    hand.clear();
    table.clear();
    show.clear();
}

CJsonObject MatchPlayerData::tojson(){
    CJsonObject co("{}");
    CJsonObject arrhand("[]"), arrshow("[]"), arrtable("[]");
    for (auto i : hand)
        arrhand.Add(consts.num2tile[i]);
    for (auto &i : show){
        std::string str = "";
        if (i.size() == 5){
            //ankan
            str = "[";
            for (int j = 0; j < 4; j ++ )
                str += consts.num2tile[i[j]];
            str += "]";
        }
        else for (auto j : i)
            str += consts.num2tile[j];
        arrshow.Add(str);
    }
    for (auto i : table)
        arrtable.Add(consts.num2tile[i % consts.TILESYS]);
    co.Add("hand", arrhand);
    co.Add("get", get == -1 ? "" : consts.num2tile[get]);
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
    int tile = consts.tile2num(str.substr(2, 2));
    bool tsumokiri = str[4] == '1';
    int reach = str[5] - '0';
    std::vector<int> dora;
    for (unsigned i = 6; i < str.size(); i += 2)
        dora.push_back(consts.tile2num(str.substr(i, 2)));

    //std::cout << data[who].get << '\n';
    //OutputVector(data[who].hand);
    if (tsumokiri && data[who].hand.size() != 14){
        assert(data[who].get == tile);
        data[who].get = -1;
    }
    else{
        for (unsigned i = 0; i < data[who].hand.size(); i ++ )
            if (data[who].hand[i] == tile){
                consts.changevec(data[who].hand, i, data[who].get == -1 ? INT_MAX : data[who].get);
                data[who].get = -1;
                break;
            }
        assert(data[who].get == -1);
    }
    data[who].table.push_back(tile);
    if (dora.size()) this -> dora = dora;
    if (reach){
        assert(!data[who].reach);
        assert(needkyoutaku = -1);
        data[who].reach = reach;
        needkyoutaku = who;
    }
    //OutputVector(data[who].hand);
    
    #ifdef MATCHDATAOUTPUT
        std::cout << "-----\n" << str << '\n' << who << ' ' << tile << ' ' << tsumokiri << ' ' << reach << '\n';
        OutputVector(dora);
    #endif
}

void MatchData::IDealTile(std::string &str){
    int who = str[1] - '0';
    int tile = consts.tile2num(str.substr(2, 2));
    std::vector<int> dora;
    for (unsigned i = 4; i < str.size(); i += 2)
        dora.push_back(consts.tile2num(str.substr(i, 2)));

    now = who;
    assert(data[who].get == -1);
    assert(remain -- );
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
    tiles.push_back(consts.tile2num(str.substr(2, 2)));
    std::vector<int> belong;
    belong.push_back(from);
    for (unsigned i = 5; i < str.size(); i += 2){
        tiles.push_back(consts.tile2num(str.substr(i, 2)));
        belong.push_back(who);
    }

    IPutKyoutaku();
    now = who;
    data[who].show.insert(data[who].show.begin(), tiles);
    assert(data[from].table.size() && *data[from].table.rbegin() == tiles[0]);
    *data[from].table.rbegin() += consts.TILESYS;
    for (unsigned t = 1; t < tiles.size(); t ++ ){
        auto tile = tiles[t];
        for (unsigned i = 0; i < data[who].hand.size(); i ++ )
            if (data[who].hand[i] == tile){
                consts.changevec(data[who].hand, i);
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
    int tile = consts.tile2num(str.substr(2, 2));
    //bool isankan = str[4] == '1';

    if (data[who].get != -1){
        data[who].hand.push_back(data[who].get);
        consts.changevec(data[who].hand, data[who].hand.size() - 1, data[who].get);
        data[who].get = -1;
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
            if (data[who].hand[i] == tile) consts.changevec(data[who].hand, i);
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
                        consts.changevec(data[who].hand, j);
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
    auto str = consts.split(actstr, ',');
    int who = str[1][0] - '0';
    std::vector<int> hai, naki, ankan;
    int get = consts.tile2num(str[5]);
    std::vector<int> dora, ura;
    int fu = atoi(str[8].c_str());
    std::vector<std::pair<int, int>> han;
    bool tsumo = str[10] == "-1";
    int from = atoi(str[10].c_str());
    int bao = atoi(str[11].c_str());

    for (unsigned i = 0; i < str[2].size(); i += 2)
        hai.push_back(consts.tile2num(str[2].substr(i, 2)));
    if (str[3].size() != 0){
        auto nakis = consts.split(str[3], '|');
        for (auto naki1 : nakis){
            int nownum = 0;
            for (unsigned i = 0; i < naki1.size(); i += 2)
                nownum = nownum * consts.TILESYS + consts.tile2num(naki1.substr(i, 2));
            naki.push_back(nownum);
        }
    }
    if (str[4].size() != 0){
        auto ankans = consts.split(str[4], '|');
        for (auto ankan1 : ankans){
            int nownum = 0;
            for (unsigned i = 0; i < ankan1.size(); i += 2)
                nownum = nownum * consts.TILESYS + consts.tile2num(ankan1.substr(i, 2));
            ankan.push_back(nownum);
        }
    }
    for (unsigned i = 0; i < str[6].size(); i ++ )
        dora.push_back(consts.tile2num(str[6].substr(i, 2)));
    for (unsigned i = 0; i < str[7].size(); i ++ )
        ura.push_back(consts.tile2num(str[7].substr(i, 2)));
    auto hans = consts.split(str[9], '|');
    for (unsigned i = 0; i < hans.size(); i += 2)
        han.push_back(std::make_pair(atoi(hans[i].c_str()), atoi(hans[i + 1].c_str())));
    
    int totalhan = 0;
    for (auto &i: han)
        totalhan += i.second;
    //TODO: 多条和牌记录时对于本场和供托的处理,直接data置零是否有风险
    std::vector<int> dpoint, dsudian;
    dpoint.resize(data.size());
    dsudian.resize(dpoint.size());
    auto calcres = consts.calctensu(fu, totalhan, honba, kyoutaku, who == east, tsumo);
    for (int i = 0; i < (int)dpoint.size(); i ++ )
        if (i == who) dpoint[i] = calcres[0];
        else if (!tsumo) dpoint[i] = - calcres[2] * (i == from);
        else dpoint[i] = - calcres[2 - (i == east)];
    calcres = consts.calctensu(fu, totalhan, 0, 0, who == east, tsumo);
    for (int i = 0; i < (int)dsudian.size(); i ++ )
        if (i == who) dsudian[i] = calcres[0];
        else if (!tsumo) dsudian[i] = - calcres[2] * (i == from);
        else dsudian[i] = - calcres[2 - (i == east)];
    int pointlevel[] = {3900, 7700, 11600, INT_MAX};
    
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

    auto &adata = *analyzedata;
    {
        int basenum = 10;
        #ifdef CHECKANALYZENAME
            assert(adata.num2basedataname[basenum] == "HULE");
        #endif
        adata.basedata[basenum] += who == adata.me;

        basenum = 11;
        #ifdef CHECKANALYZENAME
            assert(adata.num2basedataname[basenum] == "ZIMO");
        #endif
        adata.basedata[basenum] += who == adata.me && tsumo;

        basenum = 12;
        #ifdef CHECKANALYZENAME
            assert(adata.num2basedataname[basenum] == "FANGCHONG");
        #endif
        adata.basedata[basenum] += from == adata.me;

        basenum = 18;
        #ifdef CHECKANALYZENAME
            assert(adata.num2basedataname[basenum] == "FULU1HULE");
        #endif
        if (who == adata.me && data[who].show.size()) adata.basedata[basenum - 1 + data[who].show.size()] ++ ;

        basenum = 22;
        #ifdef CHECKANALYZENAME
            assert(adata.num2basedataname[basenum] == "FULU1FANGCHONG");
        #endif
        if (from == adata.me && data[from].show.size()) adata.basedata[basenum - 1 + data[from].show.size()] ++ ;
        
        basenum = 26;
        #ifdef CHECKANALYZENAME
            assert(adata.num2basedataname[basenum] == "HULEPOINT");
        #endif
        if (who == adata.me) adata.basedata[basenum] += dpoint[who];
        
        basenum = 27;
        #ifdef CHECKANALYZENAME
            assert(adata.num2basedataname[basenum] == "FANGCHONGPOINT");
        #endif
        if (from == adata.me) adata.basedata[basenum] += dpoint[from];

        basenum = 28;
        #ifdef CHECKANALYZENAME
            assert(adata.num2basedataname[basenum] == "HULESUDIAN");
        #endif
        if (who == adata.me) adata.basedata[basenum] += dsudian[who];
        
        basenum = 29;
        #ifdef CHECKANALYZENAME
            assert(adata.num2basedataname[basenum] == "FANGCHONGSUDIAN");
        #endif
        if (from == adata.me) adata.basedata[basenum] += dsudian[from];

        basenum = 30;
        #ifdef CHECKANALYZENAME
            assert(adata.num2basedataname[basenum] == "DAMAHULE");
        #endif
        adata.basedata[basenum] += who == adata.me && !data[who].show.size() && !data[who].reach;
        
        basenum = 31;
        #ifdef CHECKANALYZENAME
            assert(adata.num2basedataname[basenum] == "CHONGLEDAMA");
        #endif
        adata.basedata[basenum] += from == adata.me && !data[who].show.size() && !data[who].reach;
        
        basenum = 32;
        #ifdef CHECKANALYZENAME
            assert(adata.num2basedataname[basenum] == "DAMAHULEPOINT");
        #endif
        if (who == adata.me && !data[who].show.size() && !data[who].reach) adata.basedata[basenum] += dpoint[who];
        
        basenum = 33;
        #ifdef CHECKANALYZENAME
            assert(adata.num2basedataname[basenum] == "CHONGLEDAMAPOINT");
        #endif
        if (from == adata.me && !data[who].show.size() && !data[who].reach) adata.basedata[basenum] += dpoint[from];
        
        basenum = 34;
        #ifdef CHECKANALYZENAME
            assert(adata.num2basedataname[basenum] == "HULE3900+");
        #endif
        if (adata.me == who){
            int nowpoint = dpoint[adata.me];
            for (int i = 0; nowpoint >= pointlevel[i]; i ++ , basenum ++ );
            adata.basedata[basenum - 1] += nowpoint >= pointlevel[0];
        }
        
        basenum = 37;
        #ifdef CHECKANALYZENAME
            assert(adata.num2basedataname[basenum] == "FANGCHONG3900+");
        #endif
        if (adata.me == from){
            int nowpoint = - dpoint[adata.me];
            for (int i = 0; nowpoint >= pointlevel[i]; i ++ , basenum ++ );
            adata.basedata[basenum - 1] += nowpoint >= pointlevel[0];
        }

        basenum = 40;
        #ifdef CHECKANALYZENAME
            assert(adata.num2basedataname[basenum] == "HULECIRCLE");
        #endif
        if (who == adata.me) adata.basedata[basenum] += data[who].table.size();
        
        basenum = 41;
        #ifdef CHECKANALYZENAME
            assert(adata.num2basedataname[basenum] == "FANGCHONGMYCIRCLE");
        #endif
        if (from == adata.me) adata.basedata[basenum] += data[from].table.size();
        
        basenum = 42;
        #ifdef CHECKANALYZENAME
            assert(adata.num2basedataname[basenum] == "FANGCHONGHISCIRCLE");
        #endif
        if (from == adata.me) adata.basedata[basenum] += data[who].table.size();
        
        basenum = 43;
        #ifdef CHECKANALYZENAME
            assert(adata.num2basedataname[basenum] == "FANGCHONGSHANTEN");
        #endif
        if (from == adata.me) adata.basedata[basenum] += consts.calcshanten(data[from]);
    }
}

void MatchData::INoTile(std::string &str){
    std::vector<bool> tenpai, mangan;
    mangan.resize(4);
    for (int i = 0; i < 4; i ++ )
        tenpai.push_back(str[i + 2] == '1');
    for (unsigned i = 6; i < str.size(); i ++ )
        mangan[str[i] - '0'] = true;
    
    #ifdef MATCHDATAOUTPUT
        std::cout << "-----\n" << str << '\n';
        for (auto i : tenpai) std::cout << i << ' ';
        for (auto i : mangan) std::cout << i << ' ';
        std::cout << '\n';
    #endif
}

void MatchData::IFinalScore(std::string &str){
    std::vector<int> score;
    auto ss = consts.split(str, '|');
    ss[0].erase(0, 1);
    for (auto &i : ss)
        score.push_back(atoi(i.c_str()));

    for (unsigned i = 0; i < score.size(); i ++ )
        data[i].score = score[i];

    #ifdef MATCHDATAOUTPUT
        std::cout << "-----\n" << str << '\n';
        OutputVector(score);
    #endif
}

MatchData::MatchData(){
    kyoutaku = honba = east = now = remain = nowround = 0;
    needkyoutaku = -1;
}

CJsonObject MatchData::tojson(){
    CJsonObject arrdata("[]");
    std::string dorastr;
    for (auto &i : data)
        arrdata.Add(i.tojson());
    for (auto i : dora)
        dorastr = dorastr + consts.num2tile[i];
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
        yama.push_back(consts.tile2num(yamastr.substr(i, 2)));
    auto pointarr = record["point"];
    for (int i = 0; i < pointarr.GetArraySize(); i ++ ){
        int pp;
        pointarr.Get(i, pp);
        point.push_back(pp);
    }
    std::string dorastr;
    record.Get("dora", dorastr);
    for (unsigned i = 0; i < dorastr.size(); i += 2)
        dora.push_back(consts.tile2num(dorastr.substr(i, 2)));
    auto jsonhand = record["hand"];
    for (int i = 0; i < jsonhand.GetArraySize(); i ++ ){
        std::vector<int> vec;
        std::string hstr;
        jsonhand.Get(i, hstr);
        for (unsigned j = 0; j < hstr.size(); j += 2)
            vec.push_back(consts.tile2num(hstr.substr(j, 2)));
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

AnalyzeData::AnalyzeData(){
    basedata.clear();
    basedata.resize(BASEDATALENGTH);
    yakudata.clear();
    yakudata.resize(YAKUDATALENGTH);
    for (auto &i : yakudata)
        i.resize(YAKUNUMBER);
}

void AnalyzeData::calcresult(){
    result.clear();
    result.resize(RESULTNAMELENGTH);
    
    int resnum, bnum1, bnum2, bnum3, bnum4, bnum5;

    resnum = 0;
    bnum1 = 0;
    bnum2 = 1;
    #ifdef CHECKANALYZENAME
        assert(num2resultnameE[resnum] == "#1");
        assert(num2basedataname[bnum1] == "TOTALGAME");
        assert(num2basedataname[bnum2] == "#1");
    #endif
    for (int i = 0; i < 4; i ++ )
        result[resnum + i] = 1.0 * basedata[bnum2 + i] / basedata[bnum1];

    resnum = 4;
    bnum1 = 44;
    bnum2 = 5;
    #ifdef CHECKANALYZENAME
        assert(num2resultnameE[resnum] == "AL keep #1");
        assert(num2basedataname[bnum1] == "AL#1");
        assert(num2basedataname[bnum2] == "ALBAO1");
    #endif
    result[resnum] = 1.0 * basedata[bnum2] / basedata[bnum1];

    resnum = 5;
    bnum1 = 45;
    bnum2 = 6;
    #ifdef CHECKANALYZENAME
        assert(num2resultnameE[resnum] == "AL reach #1");
        assert(num2basedataname[bnum1] == "AL#2");
        assert(num2basedataname[bnum2] == "ALNI1");
    #endif
    result[resnum] = 1.0 * basedata[bnum2] / (basedata[bnum1] + basedata[bnum1 + 1] + basedata[bnum1 + 2]);

    resnum = 6;
    bnum1 = 47;
    bnum2 = 7;
    #ifdef CHECKANALYZENAME
        assert(num2resultnameE[resnum] == "AL prevent from #4");
        assert(num2basedataname[bnum1] == "AL#4");
        assert(num2basedataname[bnum2] == "ALBI4");
    #endif
    result[resnum] = 1.0 * basedata[bnum2] / basedata[bnum1];

    resnum = 7;
    bnum1 = 0;
    bnum2 = 8;
    #ifdef CHECKANALYZENAME
        assert(num2resultnameE[resnum] == "AL more than 2 times");
        assert(num2basedataname[bnum1] == "TOTALGAME");
        assert(num2basedataname[bnum2] == "ALMULTITIME");
    #endif
    result[resnum] = 1.0 * basedata[bnum2] / basedata[bnum1];
    
    resnum = 8;
    bnum1 = 9;
    bnum2 = 10;
    #ifdef CHECKANALYZENAME
        assert(num2resultnameE[resnum] == "hule rate");
        assert(num2basedataname[bnum1] == "TOTALROUND");
        assert(num2basedataname[bnum2] == "HULE");
    #endif
    for (int i = 0; i < 4; i ++ )
        result[resnum + i] = 1.0 * basedata[bnum2 + i] / basedata[bnum1];

    resnum = 9;
    bnum1 = 10;
    bnum2 = 11;
    #ifdef CHECKANALYZENAME
        assert(num2resultnameE[resnum] == "tsumo rate");
        assert(num2basedataname[bnum1] == "HULE");
        assert(num2basedataname[bnum2] == "ZIMO");
    #endif
    result[resnum] = 1.0 * basedata[bnum2] / basedata[bnum1];

    resnum = 12;
    bnum1 = 9;
    bnum2 = 14;
    #ifdef CHECKANALYZENAME
        assert(num2resultnameE[resnum] == "fulu rate");
        assert(num2basedataname[bnum1] == "TOTALROUND");
        assert(num2basedataname[bnum2] == "FULU1");
    #endif
    for (int i = 0; i < 4; i ++ )
        result[resnum] += 1.0 * basedata[bnum2 + i] / basedata[bnum1];

    resnum = 13;
    bnum1 = 10;
    bnum2 = 30;
    #ifdef CHECKANALYZENAME
        assert(num2resultnameE[resnum] == "dama hule rate");
        assert(num2basedataname[bnum1] == "HULE");
        assert(num2basedataname[bnum2] == "DAMAHULE");
    #endif
    result[resnum] = 1.0 * basedata[bnum2] / basedata[bnum1];

    resnum = 14;
    bnum1 = 12;
    bnum2 = 31;
    #ifdef CHECKANALYZENAME
        assert(num2resultnameE[resnum] == "fangchong dama rate");
        assert(num2basedataname[bnum1] == "FANGCHONG");
        assert(num2basedataname[bnum2] == "CHONGLEDAMA");
    #endif
    result[resnum] = 1.0 * basedata[bnum2] / basedata[bnum1];

    resnum = 15;
    bnum1 = 10;
    bnum2 = 18;
    #ifdef CHECKANALYZENAME
        assert(num2resultnameE[resnum] == "fulu hule rate");
        assert(num2basedataname[bnum1] == "HULE");
        assert(num2basedataname[bnum2] == "FULU1HULE");
    #endif
    for (int i = 0; i < 4; i ++ )
        result[resnum] += 1.0 * basedata[bnum2 + i] / basedata[bnum1];
        
    resnum = 16;
    bnum1 = 12;
    bnum2 = 22;
    #ifdef CHECKANALYZENAME
        assert(num2resultnameE[resnum] == "fulu fangchong rate");
        assert(num2basedataname[bnum1] == "FANGCHONG");
        assert(num2basedataname[bnum2] == "FULU1FANGCHONG");
    #endif
    for (int i = 0; i < 4; i ++ )
        result[resnum] += 1.0 * basedata[bnum2 + i] / basedata[bnum1];

    resnum = 17;
    bnum1 = 10;
    bnum2 = 26;
    #ifdef CHECKANALYZENAME
        assert(num2resultnameE[resnum] == "hule point");
        assert(num2basedataname[bnum1] == "HULE");
        assert(num2basedataname[bnum2] == "HULEPOINT");
    #endif
    result[resnum] += 1.0 * basedata[bnum2] / basedata[bnum1];
    
    resnum = 18;
    bnum1 = 12;
    bnum2 = 27;
    #ifdef CHECKANALYZENAME
        assert(num2resultnameE[resnum] == "fangchong point");
        assert(num2basedataname[bnum1] == "FANGCHONG");
        assert(num2basedataname[bnum2] == "FANGCHONGPOINT");
    #endif
    result[resnum] += 1.0 * basedata[bnum2] / basedata[bnum1];
    
    resnum = 19;
    bnum1 = 10;
    bnum2 = 28;
    #ifdef CHECKANALYZENAME
        assert(num2resultnameE[resnum] == "hule sudian");
        assert(num2basedataname[bnum1] == "HULE");
        assert(num2basedataname[bnum2] == "HULESUDIAN");
    #endif
    result[resnum] += 1.0 * basedata[bnum2] / basedata[bnum1];
    
    resnum = 20;
    bnum1 = 12;
    bnum2 = 29;
    #ifdef CHECKANALYZENAME
        assert(num2resultnameE[resnum] == "fangchong sudian");
        assert(num2basedataname[bnum1] == "FANGCHONG");
        assert(num2basedataname[bnum2] == "FANGCHONGSUDIAN");
    #endif
    result[resnum] += 1.0 * basedata[bnum2] / basedata[bnum1];

    resnum = 21;
    bnum1 = 30;
    bnum2 = 32;
    #ifdef CHECKANALYZENAME
        assert(num2resultnameE[resnum] == "dama hule point");
        assert(num2basedataname[bnum1] == "DAMAHULE");
        assert(num2basedataname[bnum2] == "DAMAHULEPOINT");
    #endif
    result[resnum] = 1.0 * basedata[bnum2] / basedata[bnum1];

    resnum = 22;
    bnum1 = 31;
    bnum2 = 33;
    #ifdef CHECKANALYZENAME
        assert(num2resultnameE[resnum] == "fangchong dama point");
        assert(num2basedataname[bnum1] == "CHONGLEDAMA");
        assert(num2basedataname[bnum2] == "CHONGLEDAMAPOINT");
    #endif
    result[resnum] = 1.0 * basedata[bnum2] / basedata[bnum1];

    resnum = 23;
    bnum1 = 10;
    bnum2 = 34;
    #ifdef CHECKANALYZENAME
        assert(num2resultnameE[resnum] == "hule 3900+ rate");
        assert(num2basedataname[bnum1] == "HULE");
        assert(num2basedataname[bnum2] == "HULE3900+");
    #endif
    for (int i = 0; i < 3; i ++ )
        result[resnum + i] = 1.0 * basedata[bnum2 + i] / basedata[bnum1];

    resnum = 26;
    bnum1 = 12;
    bnum2 = 37;
    #ifdef CHECKANALYZENAME
        assert(num2resultnameE[resnum] == "fangchong 3900+ rate");
        assert(num2basedataname[bnum1] == "FANGCHONG");
        assert(num2basedataname[bnum2] == "FANGCHONG3900+");
    #endif
    for (int i = 0; i < 3; i ++ )
        result[resnum + i] = 1.0 * basedata[bnum2 + i] / basedata[bnum1];

    resnum = 29;
    bnum1 = 10;
    bnum2 = 40;
    #ifdef CHECKANALYZENAME
        assert(num2resultnameE[resnum] == "hule circle");
        assert(num2basedataname[bnum1] == "HULE");
        assert(num2basedataname[bnum2] == "HULECIRCLE");
    #endif
    result[resnum] = 1.0 * basedata[bnum2] / basedata[bnum1];

    resnum = 30;
    bnum1 = 12;
    bnum2 = 41;
    #ifdef CHECKANALYZENAME
        assert(num2resultnameE[resnum] == "fangchong my circle");
        assert(num2basedataname[bnum1] == "FANGCHONG");
        assert(num2basedataname[bnum2] == "FANGCHONGMYCIRCLE");
    #endif
    result[resnum] = 1.0 * basedata[bnum2] / basedata[bnum1];

    resnum = 31;
    bnum1 = 12;
    bnum2 = 42;
    #ifdef CHECKANALYZENAME
        assert(num2resultnameE[resnum] == "fangchong his circle");
        assert(num2basedataname[bnum1] == "FANGCHONG");
        assert(num2basedataname[bnum2] == "FANGCHONGHISCIRCLE");
    #endif
    result[resnum] = 1.0 * basedata[bnum2] / basedata[bnum1];
    
    resnum = 32;
    bnum1 = 12;
    bnum2 = 43;
    #ifdef CHECKANALYZENAME
        assert(num2resultnameE[resnum] == "fangchong shanten");
        assert(num2basedataname[bnum1] == "FANGCHONG");
        assert(num2basedataname[bnum2] == "FANGCHONGSHANTEN");
    #endif
    result[resnum] = 1.0 * basedata[bnum2] / basedata[bnum1];

    std::cout << "Basedata:\n";
    for (unsigned i = 0; i < BASEDATALENGTH; i ++ )
        std::cout << "    " << num2basedataname[i] << ": " << basedata[i] << '\n';
    std::cout << "-----\nResult:\n";
    for (unsigned i = 0; i < RESULTNAMELENGTH; i ++ )
        std::cout << "    " << num2resultnameE[i] << ": " << result[i] << '\n';
    PAUSE;
}

void PaipuAnalyzer::initializeresult(){
    ////TODO:
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
    ta = consts.strptime(tas);
    tb = consts.strptime(tbs);
    result = result && (tb >= time && ta <= time);
    exclude.Get("timebefore", tbs);
    exclude.Get("timeafter", tas);
    ta = consts.strptime(tas);
    tb = consts.strptime(tbs);
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
    ////TODO:
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
    ////TODO:
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
            #ifdef CHECKANALYZENAME
                assert(adata.num2basedataname[basenum] == "TOTALROUND");
            #endif
            adata.basedata[basenum] ++ ;

            basenum = 13;
            #ifdef CHECKANALYZENAME
                assert(adata.num2basedataname[basenum] == "REACH");
            #endif
            adata.basedata[basenum] += !!matchdata.data[adata.me].reach;

            basenum = 14;
            #ifdef CHECKANALYZENAME
                assert(adata.num2basedataname[basenum] == "FULU1");
            #endif
            int fulu = matchdata.data[adata.me].show.size();
            if (fulu) adata.basedata[basenum - 1 + fulu] ++ ;
            
        }

    }
    #ifdef SAVEMATCHDATASTEP
        TotalStep.Add(GameStep);
    #endif

    {
        int basenum = 0;
        #ifdef CHECKANALYZENAME
            assert(adata.num2basedataname[basenum] == "TOTALGAME");
        #endif
        adata.basedata[basenum] ++ ;
        
        basenum = 1;
        #ifdef CHECKANALYZENAME
            assert(adata.num2basedataname[basenum] == "#1");
        #endif
        std::vector<int> vec;
        for (auto i : matchdata.data)
            vec.push_back(i.score);
        auto finalrank = consts.getrank(vec, adata.me);
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
        auto alrank = consts.getrank(vec, adata.me);

        basenum = 5;
        #ifdef CHECKANALYZENAME
            assert(adata.num2basedataname[basenum] == "ALBAO1");
        #endif
        adata.basedata[basenum] += alrank == 1 && finalrank == 1;

        basenum = 6;
        #ifdef CHECKANALYZENAME
            assert(adata.num2basedataname[basenum] == "ALNI1");
        #endif
        adata.basedata[basenum] += alrank != 1 && finalrank == 1;
        
        basenum = 7;
        #ifdef CHECKANALYZENAME
            assert(adata.num2basedataname[basenum] == "ALBI4");
        #endif
        adata.basedata[basenum] += alrank == 4 && finalrank != 4;

        basenum = 8;
        #ifdef CHECKANALYZENAME
            assert(adata.num2basedataname[basenum] == "ALMULTITIME");
        #endif
        adata.basedata[basenum] += altime > 1;

        basenum = 44;
        #ifdef CHECKANALYZENAME
            assert(adata.num2basedataname[basenum] == "AL#1");
        #endif
        adata.basedata[basenum - 1 + alrank] ++ ;
    }

    return true;
}

CJsonObject ReadJSON(std::string filename){
    std::string jsonstr;
    char *buffer = new char[JSONBUFFERSIZE];
    auto f = fopen(filename.c_str(), "r");
    for (; ; ){
        int length = fread(buffer, 1, JSONBUFFERSIZE - 1, f);
        buffer[length] = '\0';
        jsonstr += buffer;
        if (!length)
            break;
    }
    delete[] buffer;
    fclose(f);
    return CJsonObject(jsonstr);
}

std::vector<CJsonObject> ReadLineJSON(std::string filename, std::string prefix, std::string suffix){
    auto f = fopen(filename.c_str(), "r");
    std::vector<CJsonObject> res;
    for (; ; ){
        std::string ts = prefix;
        char buf[JSONBUFFERSIZE];
        if (fgets(buf, JSONBUFFERSIZE, f) == NULL) break;
        ts += buf;
        ts += suffix;
        res.push_back(CJsonObject(ts));
    }
    fclose(f);
    return res;
}

void analyzemain(std::string dataf, std::string source, std::string id, CJsonObject &config){
    auto filter = config["filter"];
    std::vector<CJsonObject> paipus;
    auto paipuarr = PA::ReadJSON(dataf + "/" + source + "/" + id + "/paipus.txt");
    for (int i = 0; i < paipuarr.GetArraySize(); i ++ )
        paipus.push_back(paipuarr[i]);
    PA::PaipuAnalyzer pa = PA::PaipuAnalyzer(filter);
    int paipunum = pa.analyze(paipus);
    #ifdef SAVEMATCHDATASTEP
        std::cout << TotalStep.ToString();
    #endif
    std::cout << "analyzed " << paipunum << " paipus\n-----\n";
    pa.analyzedata -> calcresult();
}

}


bool MatchDataCompare::tilestr(std::string &inx, std::string &iny){
    std::string x, y;
    if ((inx.size() == 10 && inx[0] == '[' && inx[9] == ']') || 
        (iny.size() == 10 && iny[0] == '[' && iny[9] == ']'))
        //存在暗杠
        if (inx.size() != iny.size() || inx[0] != iny[0] || inx[9] != iny[9])
            return false;
    for (auto i : inx)
        if (i != '[' && i != ']') x += i;
    for (auto i : iny)
        if (i != '[' && i != ']') y += i;
    //std::cout << '[' << inx << '|' << iny << '|' << x << '|' << y << ']';
    std::vector<int> bu;
    bu.resize(PA::consts.TILENUM);
    for (unsigned i = 0; i < x.size(); i += 2)
        bu[PA::consts.tile2num(x.substr(i, 2))] ++ ;
    for (unsigned i = 0; i < y.size(); i += 2)
        bu[PA::consts.tile2num(y.substr(i, 2))] -- ;
    for (auto i : bu)
        if (i) return false;
    return true;
}

void MatchDataCompare::playerdata(CJsonObject &data1, CJsonObject &data2){
    assert(data1["get"] == data2["get"]);
    assert(data1["score"] == data2["score"]);
    assert(data1["reach"] == data2["reach"]);
    std::string ts, str1, str2;
    auto hand1 = data1["hand"], hand2 = data2["hand"];
    for (int i = 0; i < hand1.GetArraySize(); i ++ ){
        hand1.Get(i, ts);
        str1 += ts;
    }
    for (int i = 0; i < hand2.GetArraySize(); i ++ ){
        hand2.Get(i, ts);
        str2 += ts;
    }
    assert(tilestr(str1, str2));
    auto show1 = data1["show"], show2 = data2["show"];
    assert(show1.GetArraySize() == show2.GetArraySize());
    for (int i = 0; i < show1.GetArraySize(); i ++ ){
        show1.Get(i, str1);
        show2.Get(i, str2);
        assert(tilestr(str1, str2));
    }
    auto table1 = data1["table"], table2 = data2["table"];
    assert(table1.GetArraySize() == table2.GetArraySize());
    for (int i = 0; i < table1.GetArraySize(); i ++ ){
        table1.Get(i, str1);
        table2.Get(i, str2);
        assert(str1.substr(0, 2) == str2.substr(0, 2));
    }
}

void MatchDataCompare::matchdata(CJsonObject &md1, CJsonObject &md2){
    //std::cout << md1.ToFormattedString() << "\n------\n" << md2.ToFormattedString() << '\n';
    //assert(md1["title"] == md2["title"]);
    assert(md1["kyoutaku"] == md2["kyoutaku"]);
    assert(md1["honba"] == md2["honba"]);
    assert(md1["now"] == md2["now"]);
    assert(md1["east"] == md2["east"]);
    assert(md1["remain"] == md2["remain"]);
    assert(md1["nowround"] == md2["nowround"]);
    assert(md1["dora"] == md2["dora"]);
    auto data1 = md1["data"], data2 = md2["data"];
    assert(data1.GetArraySize() == data2.GetArraySize());
    for (int i = 0; i < data1.GetArraySize(); i ++ )
        playerdata(data1[i], data2[i]);
}

void MatchDataCompare::main(){
    //auto json1 = PA::ReadJSON("../data/compareS.js.txt"), json2 = PA::ReadJSON("../data/compareS.cpp.txt");
    auto json1 = PA::ReadJSON("../data/compare.js.txt"), json2 = PA::ReadJSON("../data/compare.cpp.txt");
    assert(json1.GetArraySize() == json2.GetArraySize());
    for (int i = 0; i < json1.GetArraySize(); i ++ ){
        std::cout << "paipu #" << i << '\n';
        auto paipu1 = json1[i], paipu2 = json2[i];
        assert(paipu1.GetArraySize() == paipu2.GetArraySize());
        for (int j = 0; j < paipu1.GetArraySize(); j ++ ){
            auto round1 = paipu1[j], round2 = paipu2[j];
            std::cout << "    round #" << j << "\n        ";
            assert(round1.GetArraySize() == round2.GetArraySize());
            for (int k = 0; k < round1.GetArraySize() - 1; k ++ ){
                auto mdata1 = round1[k], mdata2 = round2[k];
                std::cout << "matchdata #" << k << ' ';
                matchdata(mdata1, mdata2);
            }
            std::cout << '\n';
        }
    }
    std::cout << "Passed!\n";
}

int main(){
    if (access("data/config.json", 0) == -1){
        std::cout << "Can't find data/config.json!\n";
        PAUSE;
        return 0;
    }
    auto config = PA::ReadJSON("data/config.json");
    std::string source, id;
    config.Get("source", source);
    if (access(("data/" + source).c_str(), 0) == -1){
        std::cout << "Can't find data/" + source + "!\n";
        PAUSE;
        return 0;
    }
    config.Get("id", id);
    std::vector<std::string> findid;
    #ifdef _WIN32
        _finddata_t finddata;
        int findi1, findi2;
        findi1 = findi2 = _findfirst(("data/" + source + "/*").c_str(), &finddata);
        while (~findi1){
            findid.push_back(finddata.name);
            if (*findid.rbegin() == "." || *findid.rbegin() == "..")
                findid.pop_back();
            findi1 = _findnext(findi2, &finddata);
        }
        _findclose(findi2);
    #elif linux
        DIR *dirptr = opendir(("data/" + source).c_str());
        int count = 1;
        dirent *entry;
        while (entry = readdir(dirptr)){
            findid.push_back(entry -> d_name);
            if (*findid.rbegin() == "." || *findid.rbegin() == "..")
                findid.pop_back();
        }
        closedir(dirptr);
    #else
        std::cout << "both macro _WIN32 and linux are not defined!\n";
        PAUSE;
    #endif
    if (!findid.size()){
        std::cout << "Can't find any folder under data/" + source + "!\n";
        PAUSE;
        return 0;
    }
    if (findid.size() && !id.size())
        id = findid[0];
    std::cout << "Source: " + source + "\nID: " + id + "\n-----\n";
    PA::analyzemain("data/", source, id, config);
    //MatchDataCompare::main();
    return 0;
}