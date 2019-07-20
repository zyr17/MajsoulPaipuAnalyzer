#include "algo.h"

using namespace PA;

namespace Algo{

#ifdef _WIN32
    std::string UTF82GBK(const std::string &str){
        auto src_str = str.c_str();
        int len = MultiByteToWideChar(CP_UTF8, 0, src_str, -1, NULL, 0);
        wchar_t* wszGBK = new wchar_t[len + 1];
        memset(wszGBK, 0, len * 2 + 2);
        MultiByteToWideChar(CP_UTF8, 0, src_str, -1, wszGBK, len);
        len = WideCharToMultiByte(CP_ACP, 0, wszGBK, -1, NULL, 0, NULL, NULL);
        char* szGBK = new char[len + 1];
        memset(szGBK, 0, len + 1);
        WideCharToMultiByte(CP_ACP, 0, wszGBK, -1, szGBK, len, NULL, NULL);
        std::string strTemp(szGBK);
        if (wszGBK) delete[] wszGBK;
        if (szGBK) delete[] szGBK;
        return strTemp;
    }
#endif

long long strptime(const std::string &str){
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

std::vector<std::string> split(const std::string &str, char c){
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

void changevec(std::vector<int> &vec, int pos, int replace){
    vec[pos] = replace;
    auto swap = [] (int &x, int &y) { int t = x; x = y; y = t; };
    for (int j = pos; j && vec[j - 1] > vec[j]; j -- )
        swap(vec[j - 1], vec[j]);
    for (unsigned j = pos; j + 1 < vec.size() && vec[j] > vec[j + 1]; j ++ )
        swap(vec[j], vec[j + 1]);
    if (replace == INT_MAX)
        vec.pop_back();
}

std::vector<int> calctensu(int fu, int han, int honba, int kyoutaku, bool oya, bool tsumo){
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

int getrank(std::vector<int> points, int who, int initial){
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

int pdata2tilevectile[20];

std::vector<int> pdata2tilevec(const MatchPlayerData &pdata) {
    std::vector<int> res;
    res.resize(Tiles::TILENUM);
    auto tile = pdata2tilevectile;
    int tileend = 0;
    for (auto i : pdata.hand) tile[tileend++] = i;
    if (pdata.get != Tiles::EMPTY) tile[tileend++] = pdata.get;
    assert(tileend <= 20);
    for (int k = 0; k < tileend; k++) {
        int i = tile[k];
        if (++res[i - (i == 5 || i == 15 || i == 25)] > 4) {
            //某张牌出现5张，数据有问题返回空数组
            res.clear();
            res.resize(Tiles::TILENUM);
            return res;
        }
    }
    return res;
}

std::vector<int> shantenmap;
const int SHANTENSYS = 4, TILESYS = 3;
int listshantenarr[9], makeshantenarr[5];
int Td[5], Ttvec[5];

void makeshantenmap(int &mapval, int k = 0, int num = 0){
    if (k == 9){
        int dazi = 0, tarr[9];
        for (int i = 0; i < 9; i ++ )
            tarr[i] = listshantenarr[i];
        for (int i = 0; i < 9; i ++ ){
            if (tarr[i] >= 2){
                tarr[i] -= 2;
                dazi ++ ;
            }
            if (i < 8 && tarr[i] > 0 && tarr[i + 1] > 0){
                tarr[i] -- ;
                tarr[i + 1] -- ;
                dazi ++ ;
            }
            if (i < 7 && tarr[i] > 0 && tarr[i + 2] > 0){
                tarr[i] -- ;
                tarr[i + 2] -- ;
                dazi ++ ;
            }
        }
        if (makeshantenarr[num] >= 1 << (SHANTENSYS - 1) || makeshantenarr[num] < dazi){
            makeshantenarr[num] = dazi;
            mapval = 0;
            for (auto i : makeshantenarr)
                mapval = (mapval << SHANTENSYS) + i;
        }
        return;
    }
    makeshantenmap(mapval, k + 1, num);
    auto &arr = listshantenarr;
    if (arr[k] >= 3){
        arr[k] -= 3;
        makeshantenmap(mapval, k, num + 1);
        arr[k] += 3;
    }
    if (k < 7 && arr[k] && arr[k + 1] && arr[k + 2]){
        arr[k] -- ;
        arr[k + 1] -- ;
        arr[k + 2] -- ;
        makeshantenmap(mapval, k, num + 1);
        arr[k] ++ ;
        arr[k + 1] ++ ;
        arr[k + 2] ++ ;
    }
}

void listshantenmap(int k = 0, int num = 0){
    if (k == 9){
        for (auto &i : makeshantenarr)
            i = 1 << (SHANTENSYS - 1);
        long long index = 0;
        for (auto i : listshantenarr)
            index = (index << TILESYS) + i;
        int &mapval = shantenmap[index];
        makeshantenmap(mapval);
        return;
    }
    for (int i = 0; i <= 4 && i + num <= 14; i ++ ){
        listshantenarr[k] = i;
        listshantenmap(k + 1, num + i);
    }
    listshantenarr[k] = 0;
}

int chitoishanten(const std::vector<int> &bu){
    int count = 6, over = 0, total = 0;
    for (auto i : bu){
        total += i;
        if (i > 1){
            count -- ;
            over += i - 2;
        }
    }
    over -= (total == 14);
    return over > count ? over : count;
}

int kokushishanten(const std::vector<int> &bu){
    int type = 0, two = 0;
    for (unsigned i = 0; i < bu.size(); i ++ )
        if (bu[i] && (i % 10 == 0 || i % 10 == 9 || i > 29)){
            type ++ ;
            if (bu[i] > 1) two = 1;
        }
    return 13 - type - two;
}

int calcmentsu(const std::vector<int> &bu, int mentsu){
    int toi = 0, res = INT_MAX;
    for (int i = 30; i < 37; i ++ )
        if (bu[i] >= 3) mentsu ++ ;
        else if (bu[i] == 2) toi ++ ;
    auto &d = Td, &tvec = Ttvec;
    for (int i = 0; i < 5; i ++ )
        d[i] = -100;
    d[0] = 0;
    for (int i = 0; i < 30; i += 10){
        long long index = 0;
        for (int j = 0; j < 10; j ++ )
            if (j != 5) index = (index << TILESYS) + bu[i + j];
        int cint = shantenmap[index];
        for (int k = 4; k >= 0; k -- ){
            tvec[k] = cint & ((1 << SHANTENSYS) - 1);
            cint >>= SHANTENSYS;
        }
        for (int j = int(5) - 1; j >= 0; j -- )
            for (int k = int(5 - 1) - j; k >= 0; k -- )
                if (!(tvec[k] >> (SHANTENSYS - 1)) && d[j] + tvec[k] > d[j + k])
                    d[j + k] = d[j] + tvec[k];
    }
    for (int I = 0; I < int(5); I ++ ){
        int i = I + mentsu, j = toi + d[I];
        int tt = 8 - i * 2 - j;
        if (i + j > 4) tt = 4 - i;
        if (tt < res) res = tt;
    }
    return res;
}

int calcshanten(const MatchPlayerData &pdata, bool chitoikokushi){
    if (!shantenmap.size()){
        shantenmap.resize(84000000);
        listshantenmap();
    }
    
    auto tilevec = pdata2tilevec(pdata);
    int chitoi = (pdata.show.size() || !chitoikokushi) ? INT_MAX : chitoishanten(tilevec);
    int kokushi = (pdata.show.size() || !chitoikokushi) ? INT_MAX : kokushishanten(tilevec);
    int normalres = calcmentsu(tilevec, pdata.show.size());
    for (auto &i : tilevec)
        if (i > 1){
            i -= 2;
            int tres = calcmentsu(tilevec, pdata.show.size()) - 1;
            if (tres < normalres)
                normalres = tres;
            i += 2;
        }
    int res = normalres;

    if (chitoi < res) res = chitoi;
    if (kokushi < res) res = kokushi;
    return res;
    
}

void testshanten(){
    PA::MatchPlayerData pdata;
    auto makecc = clock();
    Algo::calcshanten(pdata);
    makecc = clock() - makecc;
    int cc[] = {
        0, 1, 2, 3, 4, 6, 7, 8, 9,
        10, 11, 12, 13, 14, 16, 17, 18, 19,
        20, 21, 22, 23, 24, 26, 27, 28, 29,
        30, 31, 32, 33, 34, 35, 36
    };
    std::string filename[4] = {
        "data/p_normal_10000.txt",
        "data/p_tin_10000.txt",
        "data/p_hon_10000.txt",
        "data/p_koku_10000.txt",
    };
    std::vector<std::vector<int>> tiles, ress;
    for (int __ = 0; __ < 4; __ ++ ){
        freopen(filename[__].c_str(), "r", stdin);
        for (int _ = 10000; _ -- ; ){
            pdata.hand.clear();
            int tmp;
            std::vector<int> tt, rr;
            for (int i = 0; i < 14; i ++ ){
                scanf("%d", &tmp);
                tt.push_back(cc[tmp]);
                if (i) pdata.hand.push_back(cc[tmp]);
                else pdata.get = cc[tmp];
            }
            for (int i = 0; i < 3; i ++ ){
                scanf("%d", &tmp);
                rr.push_back(tmp);
            }
            tiles.push_back(tt);
            ress.push_back(rr);
        }
    }
    for (int i = 0; tiles.size() < 1000000; i ++ ){
        tiles.push_back(tiles[i]);
        ress.push_back(ress[i]);
    }
    auto calccc = clock();
    for (unsigned i = 0; i < tiles.size(); i ++ ){
        auto &rr = ress[i], &tt = tiles[i];
        pdata.hand.clear();
        for (unsigned i = 0; i < tt.size(); i ++ )
            if (i) pdata.hand.push_back(tt[i]);
            else pdata.get = tt[i];
        auto tilevec = Algo::pdata2tilevec(pdata);
        int res[] = {Algo::calcshanten(pdata, false), Algo::kokushishanten(tilevec), Algo::chitoishanten(tilevec)};
        for (int i = 0; i < 3; i ++ )
            assert(rr[i] == res[i]);
    }
    calccc = clock() - calccc;
    std::cout <<  makecc << ' ' << calccc << '-' << tiles.size();
}

int Access(const char *filename, int mode){
    return access((Header::datafolderprefix + filename).c_str(), mode);
}

CJsonObject ReadJSON(const std::string &filename){
    std::string jsonstr;
    char *buffer = new char[JSONBUFFERSIZE];
    auto f = fopen((Header::datafolderprefix + filename).c_str(), "r");
    for (; ; ){
        int length = fread(buffer, 1, JSONBUFFERSIZE - 1, f);
        buffer[length] = '\0';
        jsonstr += buffer;
        if (!length)
            break;
    }
    delete[] buffer;
    fclose(f);
    if ((unsigned char)jsonstr[0] == 0xef && (unsigned char)jsonstr[1] == 0xbb && (unsigned char)jsonstr[2] == 0xbf) jsonstr.erase(0, 3);
    return CJsonObject(jsonstr);
}

std::vector<CJsonObject> ReadLineJSON(const std::string &filename, const std::string &prefix, const std::string &suffix){
    auto f = fopen((Header::datafolderprefix + filename).c_str(), "r");
    std::vector<CJsonObject> res;
    for (; ; ){
        std::string ts = prefix;
        char buf[JSONBUFFERSIZE];
        if (fgets(buf, JSONBUFFERSIZE, f) == NULL) break;
        ts += buf;
        ts += suffix;
        if ((unsigned char)ts[0] == 0xef && (unsigned char)ts[1] == 0xbb && (unsigned char)ts[2] == 0xbf) ts.erase(0, 3);
        res.push_back(CJsonObject(ts));
    }
    fclose(f);
    return res;
}

void WriteJSON(const std::string &filename, const CJsonObject json){
    FILE *f = fopen((Header::datafolderprefix + filename).c_str(), "w");
    fprintf(f, "%s", json.ToString().c_str());
}

void getconsolesize(int &row, int&col){
    #ifdef _WIN32
        CONSOLE_SCREEN_BUFFER_INFO csbi;
        GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
        col = csbi.srWindow.Right - csbi.srWindow.Left + 1;
        row = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
    #elif defined(__linux) || defined(__APPLE__)
        winsize w;
        ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
        row = w.ws_row;
        col = w.ws_col;
    #endif
}

int getdisplaywidth(const std::string &str){

    #ifdef _WIN32
        return str.size();
    #endif

    int res = 0, i = 0, j = 0;
    for (; i < (int)str.size(); i ++ , j -- ){
        if (j == 0){
            unsigned char ii = ~str[i];
            assert(ii > 1);
            j = 1 + (ii < 0x40) + (ii < 0x20) + (ii < 0x10) + (ii < 8) + (ii < 4);
            res += j == 1 ? 1 : 2;
        }
        else assert((unsigned char)str[i] >= 0x80 && (unsigned char)str[i] < 0xc0);
    }
    return res;
}

int getdisplaywidth(long long num){
    int res = num < 0;
    if (num == 0) return 1;
    if (num < 0) num = - num;
    for (int i = 1; i <= num; i *= 10)
        res ++ ;
    return res;
}

int getdisplaywidth(double num){
    if (!isfinite(num)) return 1;
    int res = (num < 0) + 1 + FLOATAFTERPOINTNUM;
    if (num < 0) num = - num;
    res += num < 1;
    for (; num >= 1; num /= 10)
        res ++ ;
    return res;
}

std::vector<int> calctenpai(const PA::MatchPlayerData &oldpdata){
    PA::MatchPlayerData pdata(oldpdata);
    pdata.get = Tiles::EMPTY;
    std::vector<int> check, res;
    check.resize(Tiles::TILENUM);
    int kokushi = 0;
    for (auto j : pdata.hand){
        j -= j == 5 || j == 15 || j == 25;
        if (j % 10 && j < 30) check[j - 1 - (j == 6 || j == 16 || j == 26)] = 1;
        check[j] = 1;
        if (j < 29) check[j + 1 + (j == 4 || j == 14 || j == 24)] = 1;
        kokushi += j % 10 == 0 || j % 10 == 9 || j >= 30;
    }
    if (kokushi == 13)
        for (int i = 0; i < 37; i ++ )
            if (i % 10 == 0 || i % 10 == 9 || i >= 30)
                check[i] = 1;
    for (unsigned i = 0; i < check.size(); i ++ )
        if (check[i]){
            pdata.get = i;
            if (!~calcshanten(pdata)) res.push_back(i);
        }
    return res;
}

int tenpaiquality(const PA::MatchPlayerData &oldpdata, const std::vector<int> &tenpai){
    PA::MatchPlayerData pdata(oldpdata);
    pdata.get = Tiles::EMPTY;
    //当tenpai.size不为0时，认为该牌已经听牌，不计算calcshanten，减少运行时间
    if (!tenpai.size() && calcshanten(pdata)) return -1;
    std::vector<int> bu;
    bu.resize(Tiles::TILENUM);
    for (auto j : pdata.hand){
        j -= j == 5 || j == 15 || j == 25;
        bu[j] ++ ;
    }
    for (auto &i : pdata.show)
        for (auto j : i)
            if (j != pdata.ANKANNUM)
                bu[j - (j == 5 || j == 15 || j == 25)] ++ ;
    int waitnum = 0;
    for (auto i : calctenpai(oldpdata))
        waitnum += 4 - bu[i] + 100 * (i >= 30);
    return waitnum >= 6;
}

bool isfuritenbu[Tiles::TILENUM];

bool isfuriten(const PA::MatchPlayerData &pdata, const std::vector<int> &intenpai){
    assert(intenpai.size() || pdata.get == Tiles::EMPTY);
    auto &tenpai = (intenpai.size() ? intenpai : calctenpai(pdata));
    memset(isfuritenbu, 0, sizeof isfuritenbu);
    for (auto i : pdata.table){
        i %= Tiles::TILESYS;
        i -= i == 5 || i == 15 || i == 25;
        isfuritenbu[i] = true;
    }
    for (auto i : tenpai)
        if (isfuritenbu[i]) return true;
    return false;
}

void testtenpai(){
    PA::MatchPlayerData pdata;
    auto makecc = clock();
    Algo::calcshanten(pdata);
    makecc = clock() - makecc;
    std::string filename[1] = {
        "data/tenpai_7373.txt"
    };

    std::vector<std::vector<std::vector<int>>> shows;
    std::vector<std::vector<int>> tiles;
    for (auto &__ : filename){
        freopen(__.c_str(), "r", stdin);
        int length;
        for (scanf("%d", &length); length -- ; ){
            pdata.hand.clear();
            int tmp, len;
            std::vector<int> tt;
            scanf("%d", &len);
            for (; len -- ; ){
                scanf("%d", &tmp);
                tt.push_back(tmp);
            }
            std::vector<std::vector<int>> show;
            for (; ; ){
                scanf("%d", &len);
                if (len > 0){
                    std::vector<int> oneshow;
                    for (; len -- ; ){
                        scanf("%d", &tmp);
                        oneshow.push_back(tmp);
                    }
                    show.push_back(oneshow);
                }
                else{
                    break;
                }

            }
            shows.push_back(show);
            tiles.push_back(tt);
        }
    }
    /* for (int i = 0; tiles.size() < 1000000; i ++ ){
        tiles.push_back(tiles[i]);
        shows.push_back(shows[i]);
    } */
    auto calccc = clock();

    for (unsigned i = 0; i < tiles.size(); i ++ ){
        auto &tt = tiles[i];
        auto &show = shows[i];
        pdata.hand = tt;
        pdata.show = show;
        pdata.get = Tiles::EMPTY;
        auto res = calctenpai(pdata);
/* 
        for (auto j : tt)
            std::cout << Tiles::num2tile[j];
        std::cout << '|';
        for (auto &j : show){
            for (auto k : j)
                std::cout << Tiles::num2tile[k];
            std::cout << '|';
        }
        for (auto j : res)
            std::cout << Tiles::num2tile[j];
        std::cout << '|' << tenpaiquality(pdata) << '\n';
 */
    }
    calccc = clock() - calccc;
    std::cout <<  makecc << ' ' << calccc << '-' << tiles.size();
}

void shantendistributioncheck(const std::string &source, const std::vector<std::string> &ids, CJsonObject &config){

    srand(unsigned(time(NULL)));
    std::vector<int> yama;
    for (int i = 0; i < 37; i ++ )
        for (int j = 0; j < (i == 5 || i == 15 || i == 25 ? 1 : i == 4 || i == 14 || i == 24 ? 3 : 4); j ++ )
            yama.push_back(i);

    //39起手摸15张牌做国士
    /* int tot = 0, success = 0, nowyamapos = 0;
    std::random_shuffle(yama.begin(), yama.end());
    for (int T = 10000000; success != 20 ; ){
        std::vector<int> bu;
        bu.resize(Tiles::TILENUM);
        if (nowyamapos + 28 >= yama.size()){
            nowyamapos = 0;
            std::random_shuffle(yama.begin(), yama.end());
        }
        for (int i = 0; i < 13; i ++ )
            bu[yama[nowyamapos + i]] ++ ;
        nowyamapos += 13;
        if (kokushishanten(bu) != 10) continue;
        tot ++ ;
        for (int i = 0; i < 15; i ++ )
            bu[yama[nowyamapos + i]] ++ ;
        nowyamapos += 15;
        if (kokushishanten(bu) == -1) success ++ ;
    }
    std::cout << "39 start " << tot << " times, " << success << " success\n";
    return; */

    PaipuAnalyzer pa(config["filter"]);
    std::vector<double> shantens, shantensno713, tiles;
    shantens.resize(7);
    shantensno713.resize(9);
    tiles.resize(Tiles::TILENUM);
    int times = 0;
    for (auto id : ids){
        std::string paipufile = "data/" + source + "/" + id + "/paipus.txt";
        //std::cout << "paipu file pos: " + paipufile + '\n';
        auto paipus = ReadJSON(paipufile);
        std::cout << "check " << paipus.GetArraySize() << " paipus, id " << id << '\n';
        int percent = 0;
        for (int i = 0; i < paipus.GetArraySize(); i ++ ){
            if (pa.filtercheck(paipus[i])){
                auto &gamerecord = paipus[i]["record"];
                for (int ii = 0; ii < gamerecord.GetArraySize(); ii ++ ){
                    auto hands = gamerecord[ii]["hand"];
                    for (int j = 0; j < hands.GetArraySize(); j ++ ){
                        std::string handstr;
                        hands.Get(j, handstr);
                        assert(handstr.size() >= 26);
                        //庄家发了14张。由于雀魂自动洗牌所以忽略庄家起手
                        if (handstr.size() == 28) continue;
                        std::vector<int> hand;
                        for (int k = 0; k < 26; k += 2)
                            hand.push_back(Tiles::tile2num(handstr.substr(k, 2)));
                        
                        /* hand.clear();
                        std::random_shuffle(yama.begin(), yama.end());
                        for (int i = 0; i < 13; i ++ )
                            hand.push_back(yama[i]); */

                        for (auto k : hand)
                            tiles[k] += 1;
                        MatchPlayerData mpdata;
                        mpdata.hand = hand;
                        shantens[calcshanten(mpdata)] += 1;
                        shantensno713[calcshanten(mpdata, false)] += 1;
                        times ++ ;
                    }
                }
            }

            int nowp = i * 100 / paipus.GetArraySize();
            if (nowp % 100 == 0 && nowp > percent){
                percent = nowp;
                std::cout << percent << "% complete\n";
            }
        }
    }
    for (auto &i : shantens) i /= times;
    for (auto &i : shantensno713) i /= times;
    for (auto &i : tiles) i /= times;
    std::cout << times << " start hand analyzed\n";
    std::cout << "shanten result: \n";
    OutputVector(shantens);
    std::cout << "shanten without chitoi kokushi result: \n";
    OutputVector(shantensno713);
    std::cout << "tiles result: \n";
    for (int i = 0; i < Tiles::TILENUM; i ++ ){
        std::cout << '[' << Tiles::num2tile[i] << ' ' << tiles[i] << "] ";
        if (i % 10 == 9) std::cout << '\n';
    }
}

int countdora(const PA::MatchPlayerData &pdata, const std::vector<int> &dora){
    int res = 0;
    std::vector<int> tiles;
    for (auto i : pdata.hand)
        tiles.push_back(i);
    for (auto &one : pdata.show)
        for (auto i : one)
            tiles.push_back(i);
    for (auto &i : tiles){
        if (i == 5 || i == 15 || i == 25){
            res ++ ;
            i -- ;
        }
        for (auto j : dora)
            res += Tiles::nexttile[j] == i;
    }
/* 
    std::cout << '|';
    for (auto i : dora) std::cout << ' ' << Tiles::num2tile[i];
    std::cout << " | " << res << '\n';
 */
    return res;
}

bool istanyao(const PA::MatchPlayerData &pdata){
    auto tenpai = calctenpai(pdata);
    std::vector<int> tiles;
    for (auto i : tenpai)
        tiles.push_back(i);
    for (auto i : pdata.hand)
        tiles.push_back(i);
    for (auto &one : pdata.show)
        for (auto i : one)
            tiles.push_back(i);
    for (auto i : tiles)
        if (i % 10 == 0 || i % 10 == 9 || i >= 30) return false;
    return true;
}


bool ispinfu(const PA::MatchPlayerData &pdata){
    //TODO: 专门实现听牌平和好烦都差不多可以顺便实现役种判断了。。先搁置，全返回false。
    return false;
}

bool isyakuhai(const PA::MatchPlayerData &pdata, int wind, int round){
    std::vector<int> bu;
    bu.resize(Tiles::TILENUM);
    for (auto i : pdata.hand)
        bu[i] ++ ;
    for (auto &i : pdata.show)
        for (auto j : i)
            if (j != pdata.ANKANNUM) bu[j] ++ ;
    return bu[30 + wind] >= 3 || bu[30 + round] >= 3 || bu[34] >= 3 || bu[35] >= 3 || bu[36] >= 3;
}

namespace SR{

    RoundData S(roombasesouth, roomdeltasouth), E(roombaseeast, roomdeltaeast);

    double tdist(double x, long long v){
        //v = 0时分布不存在，返回nan
        if (v == 0) return NAN;
        int col = 0;
        for (int i = 1; i < T_DIST_COL; i ++ )
            if (x == T_DIST_TABLE[0][i]) col = i;
        assert(col);
        assert(v > 0);
        for (int i = 1; i < T_DIST_ROW; i ++ )
            if (T_DIST_TABLE[i][0] == v) return T_DIST_TABLE[i][col];
            else if (T_DIST_TABLE[i][0] > v){
                double resl = T_DIST_TABLE[i - 1][col], resr = T_DIST_TABLE[i][col];
                double part = (v - T_DIST_TABLE[i - 1][0]) / (T_DIST_TABLE[i][0] - T_DIST_TABLE[i - 1][0]);
                return resl * (1 - part) + resr * part;
            }
        assert(0);
        return NAN;
    }

    std::pair<double, double> confidenceinterval(const std::vector<double> &sample, double alpha){
        double avg = 0, var = 0;
        long long n = sample.size();
        for (auto i : sample)
            avg += i;
        avg /= n;
        for (auto i : sample)
            var += (i - avg) * (i - avg);
        var /= n - 1;
        double delta = std::sqrt(var / n) * tdist(alpha / 2, n - 1);
        //std::cout << avg << ' ' << var << ' ' << n << ' ' << std::sqrt(var / n) << ' ' << tdist(alpha / 2, n - 1) << '\n';
        return std::make_pair(avg - delta, avg + delta);
    }

    void stablerank(int round, double &stablerank, std::pair<double, double> &CI){
        assert(round == 4 || round == 8);
        RoundData &rd(round == 4 ? E : S);
        if (!(rd.pt123.size() + rd.pt4.size())){
            stablerank = CI.first = CI.second = NAN;
            return;
        }

        std::vector<double> pt = rd.pt123;
        double total = 0;
        for (auto i : rd.pt123)
            total += i;
        for (auto i : rd.pt4)
            total += i;
        double r4 = 1.0 * rd.pt4.size() / (rd.pt123.size() + rd.pt4.size());
        double every4 = total / rd.pt4.size();
        for (auto i : rd.pt4)
            pt.push_back(i - every4);
        CI = confidenceinterval(pt);
        stablerank = (every4 - rd.roombase[rd.room]) / rd.roomdelta[rd.room];
        CI.first = CI.first / rd.roomdelta[rd.room] / r4 + stablerank;
        CI.second = CI.second / rd.roomdelta[rd.room] / r4 + stablerank;
        //std::cout << total << ' ' << r4 << ' ' << every4 << ' ' << stablerank << ' ' << CI.first << ' ' << CI.second << '\n';
    }

    void addgamedata(int nroom, int round, int rank, int pt, int point){
        assert(nroom >= 0 && nroom <= 5);
        if (round != 4 && round != 8) return;
        RoundData &rd(round == 4 ? E : S);
        if (!considerroom[nroom]) return;
        if (nroom < rd.room) return;
        if (nroom > rd.room){
            rd.room = nroom;
            rd.pt123.clear();
            rd.pt4.clear();
        }
        if (rank != 4) rd.pt123.push_back(pt);
        else{
            pt = (point - 25000) / 1000;
            rd.pt4.push_back(pt);
        }
    }

    int getroom(int round){
        if (round != 4 && round != 8) return INVALIDROOM;
        int room = round == 4 ? E.room : S.room;
        if (room < 0 || room > 5 || !considerroom[room]) return INVALIDROOM;
        return room;
    }

}

}