#ifndef _TILES_H
#define _TILES_H

#include "header.h"

//牌和数字转换
namespace Tiles{
    const int EMPTY = -1;
    const int TILENUM = 38;
    const int TILESYS = 64;
    const std::string num2tile[TILENUM] = {
        "1m", "2m", "3m", "4m", "5m", "0m", "6m", "7m", "8m", "9m", 
        "1p", "2p", "3p", "4p", "5p", "0p", "6p", "7p", "8p", "9p", 
        "1s", "2s", "3s", "4s", "5s", "0s", "6s", "7s", "8s", "9s", 
        "1z", "2z", "3z", "4z", "5z", "6z", "7z", "?x"
    };
    const int nexttile[TILENUM] = {
        1, 2, 3, 4, 6, 6, 7, 8, 9, 0, 
        11, 12, 13, 14, 16, 16, 17, 18, 19, 10, 
        21, 22, 23, 24, 26, 26, 27, 28, 29, 20, 
        31, 32, 33, 30, 35, 36, 34
    };
    void TilesInit();
    int tile2num(const std::string &key);
};

#endif