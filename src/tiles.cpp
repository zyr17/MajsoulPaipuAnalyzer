#include "tiles.h"

namespace Tiles{
    std::vector<int> tile2numvec;

    void TilesInit(){
        tile2numvec.resize(256 * 256);
        for (auto &i : tile2numvec)
            i = -1;
        for (int i = 0; i < TILENUM; i ++ )
            tile2numvec[num2tile[i][0] * 256 + num2tile[i][1]] = i;
    }

    int tile2num(const std::string &key){
        assert(key.size() == 2);
        if (!tile2numvec.size())
            TilesInit();

        return (tile2numvec[key[0] * 256 + key[1]]);
    }
}