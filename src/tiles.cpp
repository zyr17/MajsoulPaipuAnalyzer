#include "tiles.h"

namespace Tiles{
    std::map<std::string, int> tile2nummap;

    void TilesInit(){
        for (int i = 0; i < TILENUM; i ++ )
            tile2nummap[num2tile[i]] = i;
    }

    int tile2num(const std::string &key){
        if (!tile2nummap.size())
            TilesInit();

        auto it = tile2nummap.find(key);
        if (it != tile2nummap.end())
            return it -> second;
        return -1;
    }
}