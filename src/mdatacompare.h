#ifndef _MDATACOMPARE_H
#define _MDATACOMPARE_H

#include "header.h"
#include "tiles.h"
#include "algo.h"
#include "analyzer.h"


namespace MatchDataCompare{
    bool tilestr(std::string &inx, std::string &iny);
    void playerdata(CJsonObject &data1, CJsonObject &data2);
    void matchdata(CJsonObject &md1, CJsonObject &md2);
    void mdatacomparemain();
}

#endif