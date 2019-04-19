#include "mdatacompare.h"

namespace MatchDataCompare{

    bool tilestr(std::string &inx, std::string &iny){
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
        bu.resize(Tiles::TILENUM);
        for (unsigned i = 0; i < x.size(); i += 2)
            bu[Tiles::tile2num(x.substr(i, 2))] ++ ;
        for (unsigned i = 0; i < y.size(); i += 2)
            bu[Tiles::tile2num(y.substr(i, 2))] -- ;
        for (auto i : bu)
            if (i) return false;
        return true;
    }

    void playerdata(CJsonObject &data1, CJsonObject &data2){
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

    void matchdata(CJsonObject &md1, CJsonObject &md2){
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

    void mdatacomparemain(){
        //auto json1 = Algo::ReadJSON("../data/compareS.js.txt"), json2 = Algo::ReadJSON("../data/compareS.cpp.txt");
        auto json1 = Algo::ReadJSON("../data/compare.js.txt"), json2 = Algo::ReadJSON("../data/compare.cpp.txt");
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

}