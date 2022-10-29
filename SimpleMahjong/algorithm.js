"use strict";

class Algo{

    static emptycalcdata(
        hai = '', 
        agari = '', 
        naki = [], 
        ankan = [],
        dora = '',
        ura = '',
        tsumo = false,
        honba = 0,
        jifu = 0,
        bafu = 0,
        reach = 0,
        //yipatsu, rinshan/chankan, haitei/houtei, tenhou/chihou
        specialyaku = '0000'
    ){
        return {
            'hai': hai, 
            'agari': agari, 
            'naki': naki, 
            'ankan': ankan, 
            'dora': dora, 
            'ura': ura, 
            'tsumo': tsumo, 
            'honba': honba, 
            'jifu': jifu, 
            'bafu': bafu, 
            'reach': reach, 
            'yipatsu': specialyaku[0] != 0, 
            'rinshanchan': specialyaku[1] != 0, 
            'haihoutei': specialyaku[2] != 0, 
            'tenchi': specialyaku[3] != 0
        };
    }

    static num2matxy(num){
        let x = parseInt(num / 10);
        let y = num2maty[num % 10];
        if (x == 3)
            y = num % 10;
        //console.log(num, x, y);
        return [x, y];
    }

    static getnowleftvec(matchdata, who, aka = 3){
        console.assert(aka == 0 || aka == 3 || aka == 4, 'in getnowleftmat: aka num illegal');
        var mdata = matchdata.data[who];
        var resvec = [];
        for(let i = 0; i < 37; i ++ ){
            var tmp = 4;
            if (i % 10 == 4 && i < 30){
                tmp -= parseInt(aka / 3);
                if (num2tile[i] == '5p' && aka == 4)
                    tmp -- ;
            }
            else if (i % 10 == 5 && i < 30)
                tmp = 4 - resvec[i - 1];
            resvec.push(tmp);
        }
        for (let i = 0; i < mdata.hand.length; i ++ )
            resvec[tile2num[mdata.hand[i]]] -- ;
        if (mdata.get.length >= 2)
            resvec[tile2num[mdata.get.slice(0, 2)]] -- ;
        for (let i = 0; i < 4; i ++ ){
            mdata = matchdata.data[i];
            for (let j = 0; j < mdata.show.length; j ++ ){
                var str = mdata.show[j].split(/[\[\]]/).join('');
                for (let k = 0; k < str.length; k += 2){
                    let tile = str.slice(k, k + 2);
                    resvec[tile2num[tile]] -- ;
                }
            }
            for (let j = 0; j < mdata.table.length; j ++ ){
                if (mdata.table[j][mdata.table[j].length - 1] != '*')
                    resvec[tile2num[mdata.table[j].slice(0, 2)]] -- ;
            }
        }
        for (let i = 0; i < matchdata.dora.length; i += 2){
            let tile = matchdata.dora.slice(i, i + 2);
            resvec[tile2num[tile]] -- ;
        }
        return resvec;
    }

    static matchdata2algodata(matchdata, who){
        var data = matchdata.data[who];
        var naki, ankan;
        [naki, ankan] = this.show2nakiankan(data.show);
        var res = this.emptycalcdata(
            data.hand.join(''),
            data.get,
            naki,
            ankan,
            matchdata.dora,
            '',
            false,
            matchdata.honba,
            (who - matchdata.east + 4) % 4,
            matchdata.east,
            matchdata.data[who].reach
        );
        return res;
    }

    static show2nakiankan(show){
        var naki = [];
        var ankan = [];
        for (var i = 0; i < show.length; i ++ ){
            var tmp = show[i];
            tmp = tmp.split(/[\[\]]/).join('');
            if (show[i].length == 10 && show[i][0] == '[' && show[i][9] == ']')
                ankan.push(tmp);
            else
                naki.push(tmp);
        }
        return [naki, ankan];
    }

    static strmatchnumone(str, part){
        return (str.length - str.replace(new RegExp(part, 'g'), '').length) / part.length;
    }

    static strmatchnum(str, part){
        if (part[0] == '0' || part[0] == '5' && part[1] != 'z')
            return this.strmatchnumone(str, '0' + part[1]) + this.strmatchnumone(str, '5' + part[1]);
        return this.strmatchnumone(str, part);
    }

    static str2mat(str){
        var mat = new Array(new Array(9), new Array(9), new Array(9), new Array(9));
        for (let i = 0; i < mat.length; i ++ )
            for (var j = 0; j < mat[i].length; j ++ )
                mat[i][j] = 0;
        for (let i = 0; i < str.length; i += 2){
            var num = tile2num[str.substring(i, i + 2)];
            var matx = parseInt(num / 10);
            var maty = num2maty[num % 10];
            if (matx == 3) maty = num % 10;
            mat[matx][maty] ++ ;
        }
        return mat;
    }

    static getchinmapnum(arr){
        var res = 0;
        for (var i = 0; i < 9; i ++ )
            res = res * 8 + arr[i];
        return res;
    }

    static makechinmap(arr){
        var tarr = [-9, -9, -9, -9, -9];
        var num = this.getchinmapnum(arr);
        function calckouho(){
            var num = 0;
            var tarr = arr.slice(0, 9);
            for (var i = 0; i < 9; i ++ ){
                if (tarr[i] >= 2){
                    tarr[i] -= 2;
                    num ++ ;
                }
                if (i < 8 && tarr[i] > 0 && tarr[i + 1] > 0){
                    tarr[i] -- ;
                    tarr[i + 1] -- ;
                    num ++ ;
                }
                if (i < 7 && tarr[i] > 0 && tarr[i + 2] > 0){
                    tarr[i] -- ;
                    tarr[i + 2] -- ;
                    num ++ ;
                }
            }
            return num;
        }
        function dfs(k, num){
            if (k == 9){
                var now = calckouho();
                if (now > tarr[num])
                    tarr[num] = now;
                return;
            }
            dfs(k + 1, num);
            if (arr[k] >= 3){
                arr[k] -= 3;
                dfs(k, num + 1);
                arr[k] += 3;
            }
            if (k < 7 && arr[k] > 0 && arr[k + 1] > 0 && arr[k + 2] > 0){
                arr[k] -- ;
                arr[k + 1] -- ;
                arr[k + 2] -- ;
                dfs(k, num + 1);
                arr[k] ++ ;
                arr[k + 1] ++ ;
                arr[k + 2] ++ ;
            }
        }
        dfs(0, 0);
        this.chinmap[num] = tarr;
    }

    static listall(max = 14){
        var arr = [0, 0, 0, 0, 0, 0, 0, 0, 0];
        var res = [];
        function dfs(k, num){
            if (k == 9){
                //if (num > 0)
                    res.push(arr.slice(0, 9));
                return;
            }
            for (var i = 0; i <= 4 && i + num <= max; i ++ ){
                arr[k] = i;
                dfs(k + 1, num + i);
            }
            arr[k] = 0;
        }
        dfs(0, 0);
        return res;
    }

    static chitoishanten(data){
        if (data.naki.length + data.ankan.length > 0) return 999;
        var mat = this.str2mat(data.hai + data.agari);
        var count = 6, over = 0;
        for (var i = 0; i < 4; i ++ )
            for (var j = 0; j < 9; j ++ )
                if (mat[i][j] > 1){
                    count -- ;
                    over += mat[i][j] - 2;
                }
        if (data.hai.length + data.agari.length == 28)
            over -= 1;
        if (over > count)
            count = over;
        return count;
    }

    static kokushishanten(data){
        if (data.naki.length + data.ankan.length > 0) return 999;
        var mat = this.str2mat(data.hai + data.agari);
        var count = 0, double = 0;
        for (var i = 0; i < 4; i ++ )
            for (var j = 0; j < 9; j ++ )
                if (i == 3 || j == 0 || j == 8){
                    if (mat[i][j] > 0) count ++ ;
                    if (mat[i][j] > 1) double = 1;
                }
        return 13 - count - double;
    }

    static calcshanten(data, chitoikokushi = true){
        ////TODO: some tile have 4, can't get more
        var shanten = 999;
        if (chitoikokushi){
            var chitoi = this.chitoishanten(data);
            var kokushi = this.kokushishanten(data);
            if (chitoi < shanten)
                shanten = chitoi;
            if (kokushi < shanten)
                shanten = kokushi;
        }
        var nowmentsu = data.naki.length + data.ankan.length;
        var nowhai = data.hai;
        if (data.agari != undefined && data.agari.length > 0)
            nowhai += data.agari.slice(0, 2);
        //console.log(nowhai);
        var mat = this.str2mat(nowhai);
        function calcmentsu(mat, nowmentsu){
            var nowkouho = 0;
            var result = 999;
            for (let i = 0; i < 9; i ++ )
                if (mat[3][i] >= 3) nowmentsu ++ ;
                else if (mat[3][i] == 2) nowkouho ++ ;
            var num0 = Algo.getchinmapnum(mat[0]);
            var num1 = Algo.getchinmapnum(mat[1]);
            var num2 = Algo.getchinmapnum(mat[2]);
            for (let i = 0; i < 5; i ++ )
                if (Algo.chinmap[num0][i] >= 0)
                    for (var j = 0; j < 5; j ++ )
                        if (Algo.chinmap[num1][j] >= 0)
                            for (var k = 0; k < 5; k ++ )
                                if (Algo.chinmap[num2][k] >= 0){
                                    var t1 = nowmentsu + i + j + k;
                                    var t2 = nowkouho + Algo.chinmap[num0][i] + Algo.chinmap[num1][j] + Algo.chinmap[num2][k];
                                    if (t1 + t2 > 4) t2 = 4 - t1;
                                    var tres = 8 - t1 * 2 - t2;
                                    if (result > tres) result = tres;
                                }
            return result;
        }
        var res0 = calcmentsu(mat, nowmentsu);
        if (res0 < shanten)
            shanten = res0;
        for (let i = 0; i < 4; i ++ )
            for (let j = 0; j < 9; j ++ )
                if (mat[i][j] >= 2){
                    mat[i][j] -= 2;
                    var res1 = calcmentsu(mat, nowmentsu) - 1;
                    if (res1 < shanten)
                        shanten = res1;
                    mat[i][j] += 2;
                }
        return shanten;
    }

    static testshanten(arr){
        var cc = [
            0, 1, 2, 3, 4, 6, 7, 8, 9,
            10, 11, 12, 13, 14, 16, 17, 18, 19,
            20, 21, 22, 23, 24, 26, 27, 28, 29,
            30, 31, 32, 33, 34, 35, 36
        ];
        var data = this.emptycalcdata();
        for (var i = 0; i < arr.length; i ++ ){
            var res = arr[i].slice(14, 17);
            data.hai = '';
            for (let j = 0; j < 14; j ++ )
                data.hai += num2tile[cc[arr[i][j]]];
            console.log(data.hai);
            var rr = [this.calcshanten(data, false), this.kokushishanten(data), this.chitoishanten(data)];
            //console.log(rr, res);
            for (let j = 0; j < 3; j ++ )
                console.assert(rr[j] == res[j]);
        }
    }

    static calcyaku(data){
        var result = { 'yaku': 0, 'yakuname': [] };
        function getshun(str){
            if (str.length != 6) return -1;
            var n1 = tile2num[str.slice(0, 2)];
            var n2 = tile2num[str.slice(2, 4)];
            var n3 = tile2num[str.slice(4, 6)];
            if (n1 == n2) return -1;
            if (n2 < n1) n1 = n2;
            if (n3 < n1) n1 = n3;
            return n1;
        }

        function getkokan(str){
            if (str.length == 4) return -1;
            var n1 = tile2num[str.slice(0, 2)];
            var n2 = tile2num[str.slice(2, 4)];
            if (n1 != n2) return -1;
            return n1;
        }
        
        function pinfu(data){
            if (data.pinfu) return 1;
            return 0;
        }

        function tanyao(data){
            var str = data.tehai.join('') + data.agari + data.naki.join('') + data.ankan.join('');
            for (var i = 0; i < str.length; i += 2){
                var num = tile2num[str.slice(i, i + 2)];
                if (num % 10 == 0 || num % 10 == 9 || num > 29)
                    return 0;
            }
            return 1;
        }

        function xpeikou(data){
            if (data.naki.length > 0) return false;
            var agari = tile2num[data.agari.slice(0, 2)];
            var get1 = tile2num[data.agari.slice(2, 4)];
            var map = new Array(40);
            for (let i = 0; i < 40; i ++ )
                map[i] = 0;
            agari = getshun(data.agari);
            if (agari != -1) map[agari] ++ ;
            var result = 0;
            for (let i = 0; i < data.tehai.length; i ++ ){
                var tmp = getshun(data.tehai[i]);
                if (tmp != -1) map[tmp] ++ ;
                if (map[tmp] > 1){
                    result ++ ;
                    map[tmp] = 0;
                }
            }
            return result;
        }

        function yipeikou(data){
            if (xpeikou(data) == 1) return 1;
            return 0;
        }

        function reach(data){
            if (data.reach == 1) return 1;
            return 0;
        }

        function yipatsu(data){
            if (data.yipatsu) return 1;
            return 0;
        }

        function tsumo(data){
            if (data.naki.length == 0 && data.tsumo) return 1;
            return 0;
        }

        function yakuhai(data, num){
            if (getkokan(data.agari) == num) return 1;
            for (let i = 0; i < data.tehai.length; i ++ )
                if (getkokan(data.tehai[i]) == num) return 1;
            for (let i = 0; i < data.naki.length; i ++ )
                if (getkokan(data.naki[i]) == num) return 1;
            for (let i = 0; i < data.ankan.length; i ++ )
                if (getkokan(data.ankan[i]) == num) return 1;
            return 0;
        }

        function haku(data){
            return yakuhai(data, 34);
        }

        function hatsu(data){
            return yakuhai(data, 35);
        }

        function chun(data){
            return yakuhai(data, 36);
        }

        function jifudon(data){
            return data.jifu == 0 ? yakuhai(data, data.jifu + 30) : 0;
        }

        function bafudon(data){
            return data.bafu == 0 ? yakuhai(data, data.bafu + 30) : 0;
        }

        function jifunan(data){
            return data.jifu == 1 ? yakuhai(data, data.jifu + 30) : 0;
        }

        function bafunan(data){
            return data.bafu == 1 ? yakuhai(data, data.bafu + 30) : 0;
        }

        function jifusha(data){
            return data.jifu == 2 ? yakuhai(data, data.jifu + 30) : 0;
        }

        function bafusha(data){
            return data.bafu == 2 ? yakuhai(data, data.bafu + 30) : 0;
        }

        function jifupei(data){
            return data.jifu == 3 ? yakuhai(data, data.jifu + 30) : 0;
        }

        function bafupei(data){
            return data.bafu == 3 ? yakuhai(data, data.bafu + 30) : 0;
        }

        function rinshan(data){
            if (data.rinshanchan && data.tsumo) return 1;
            return 0;
        }

        function chankan(data){
            if (data.rinshanchan && !data.tsumo) return 1;
            return 0;
        }

        function haitei(data){
            if (data.haihoutei && data.tsumo) return 1;
            return 0;
        }

        function houtei(data){
            if (data.haihoutei && !data.tsumo) return 1;
            return 0;
        }

        function wreach(data){
            if (data.reach == 2) return 2;
            return 0;
        }

        function kuisagari(data){
            if (data.naki.length != 0) return 1;
            return 0;
        }

        function chanta(data){
            if (data.tehai.length == 6) return 0;
            var shun = {0:1, 7:1, 10:1, 17:1, 20:1, 27:1};
            var kokan = {0:1, 9:1, 10:1, 19:1, 20:1, 29:1, 30:1, 31:1, 32:1, 33:1, 34:1, 35:1, 36:1};
            var tarr = [data.agari].concat(data.tehai).concat(data.naki).concat(data.ankan);
            for (var i = 0; i < tarr.length; i ++ ){
                var s = getshun(tarr[i]);
                var k = getkokan(tarr[i]);
                if (s == -1 && k == -1)
                    k = tile2num[tarr[i].slice(0, 2)];
                if (s != -1 && !(s in shun)) return 0;
                if (k != -1 && !(k in kokan)) return 0;
            }
            return 2 - kuisagari(data);
        }

        function honroutou(data){
            var str = data.tehai.join('') + data.agari + data.naki.join('') + data.ankan.join('');
            for (var i = 0; i < str.length; i += 2){
                var num = tile2num[str.slice(i, i + 2)];
                if (!(num % 10 == 0 || num % 10 == 9 || num > 29))
                    return 0;
            }
            return 2;
        }

        function sanshoku(data){
            var tarr = [data.agari].concat(data.tehai).concat(data.naki);
            var shun = {};
            for (let i = 0; i < tarr.length; i ++ ){
                var s = getshun(tarr[i]);
                if (s != -1)
                    shun[s] = 1;
            }
            var num = [0, 0, 0, 0, 0, 0, 0, 0, 0, 0];
            for (let i = 0; i < 30; i ++ )
                if (shun[i] == 1) num[i % 10] ++ ;
            for (let i = 0; i < num.length; i ++ )
                if (num[i] == 3) return 2 - kuisagari(data);
            return 0;
        }

        function yitsu(data){
            var tarr = [data.agari].concat(data.tehai).concat(data.naki);
            var shun = {};
            for (var i = 0; i < tarr.length; i ++ ){
                var s = getshun(tarr[i]);
                if (s != -1)
                    shun[s] = 1;
            }
            if (shun[0] == 1 && shun[3] == 1 && shun[7] == 1) return 2 - kuisagari(data);
            if (shun[10] == 1 && shun[13] == 1 && shun[17] == 1) return 2 - kuisagari(data);
            if (shun[20] == 1 && shun[23] == 1 && shun[27] == 1) return 2 - kuisagari(data);
            return 0;
        }

        function toitoi(data){
            var tarr = [data.agari].concat(data.tehai).concat(data.naki);
            if (data.tehai.length == 6) return 0;
            for (var i = 0; i < tarr.length; i ++ )
                if (getshun(tarr[i]) != -1)
                    return 0;
            return 2;
        }

        function sanshokudouko(data){
            var tarr = [data.agari].concat(data.tehai).concat(data.naki).concat(data.ankan);
            var kokan = [0, 0, 0, 0, 0, 0, 0, 0, 0, 0];
            for (let i = 0; i < tarr.length; i ++ ){
                var k = getkokan(tarr[i]);
                if (k != -1)
                    kokan[k % 10] ++ ;
            }
            for (let i = 0; i < kokan.length; i ++ )
                if (kokan[i] == 3) return 2;
            return 0;
        }

        function xanko(data){
            var res = data.ankan.length;
            if (getkokan(data.agari) != -1 && data.tsumo)
                res ++ ;
            for (var i = 0; i < data.tehai.length; i ++ )
                if (getkokan(data.tehai[i]) != -1)
                    res ++ ;
            return res;
        }

        function sananko(data){
            if (xanko(data) == 3) return 2;
            return 0;
        }

        function xkantsu(data){
            var res = data.ankan.length;
            for (var i = 0; i < data.naki.length; i ++ )
                if (data.naki[i].length == 8)
                    res ++ ;
            return res;
        }

        function sankantsu(data){
            if (xkantsu(data) == 3) return 2;
            return 0;
        }

        function chitoitsu(data){
            if (data.tehai.length == 6) return 2;
            return 0;
        }

        function shousangen(data){
            var tarr = [data.agari].concat(data.tehai).concat(data.naki).concat(data.ankan);
            var num = 0;
            for (var i = 0; i < tarr.length; i ++ ){
                var tmp = getkokan(tarr[i]);
                if (tmp >= 34 && tmp <= 36)
                    num ++ ;
                if (tarr[i].length == 4){
                    var tmp2 = tile2num[tarr[i].slice(0, 2)];
                    if (tmp2 >= 34 && tmp2 <= 36)
                        num ++ ;
                    else return 0;
                }
            }
            if (num == 3) return 2;
            return 0;
        }

        function junchan(data){
            var shun = {0:1, 7:1, 10:1, 17:1, 20:1, 27:1};
            var kokan = {0:1, 9:1, 10:1, 19:1, 20:1, 29:1};
            var tarr = [data.agari].concat(data.tehai).concat(data.naki).concat(data.ankan);
            for (var i = 0; i < tarr.length; i ++ ){
                var s = getshun(tarr[i]);
                var k = getkokan(tarr[i]);
                if (s == -1 && k == -1)
                    k = tile2num[tarr[i].slice(0, 2)];
                if (s != -1 && !(s in shun)) return 0;
                if (k != -1 && !(k in kokan)) return 0;
            }
            return 3 - kuisagari(data);
        }

        function honyitsu(data){
            var last = -1;
            var str = data.tehai.join('') + data.agari + data.naki.join('') + data.ankan.join('');
            for (var i = 0; i < str.length; i += 2){
                var color = parseInt(tile2num[str.slice(i, i + 2)] / 10);
                if (color == 3) continue;
                if (last == -1) last = color;
                if (last != color) return 0;
            }
            if (last != -1) return 3 - kuisagari(data);
        }

        function ryanpeikou(data){
            if (xpeikou(data) == 2) return 3;
            return 0;
        }

        function chinyitsu(data){
            var last = -1;
            var str = data.tehai.join('') + data.agari + data.naki.join('') + data.ankan.join('');
            //console.log(str);
            for (var i = 0; i < str.length; i += 2){
                var color = parseInt(tile2num[str.slice(i, i + 2)] / 10);
                if (color == 3) return 0;
                if (last == -1) last = color;
                if (last != color) return 0;
            }
            if (last != -1) return 6 - kuisagari(data);
        }

        function tenhou(data){
            if (data.tenchi && data.jifu == 0) return 13;
            return 0;
        }

        function chihou(data){
            if (data.tenchi && data.jifu != 0) return 13;
            return 0;
        }

        function daisangen(data){
            var tarr = [data.agari].concat(data.tehai).concat(data.naki).concat(data.ankan);
            var num = 0;
            for (var i = 0; i < tarr.length; i ++ ){
                var tmp = getkokan(tarr[i]);
                if (tmp >= 34 && tmp <= 36)
                    num ++ ;
            }
            if (num == 3) return 13;
            return 0;
        }

        function daisushi(data){
            var tarr = [data.agari].concat(data.tehai).concat(data.naki).concat(data.ankan);
            var num = 0;
            for (var i = 0; i < tarr.length; i ++ ){
                var tmp = getkokan(tarr[i]);
                if (tmp >= 30 && tmp <= 33)
                    num ++ ;
            }
            if (num == 4) return 26;
            return 0;
        }

        function shousushi(data){
            var tarr = [data.agari].concat(data.tehai).concat(data.naki).concat(data.ankan);
            var num = 0;
            for (var i = 0; i < tarr.length; i ++ ){
                var tmp = getkokan(tarr[i]);
                if (tmp >= 30 && tmp <= 33)
                    num ++ ;
                if (tarr[i].length == 4){
                    var tmp2 = tile2num[tarr[i].slice(0, 2)];
                    if (tmp2 >= 30 && tmp2 <= 33)
                        num ++ ;
                    else return 0;
                }
            }
            if (num == 4) return 13;
            return 0;
        }

        function suanko(data){
            if (xanko(data) == 4) return 13;
            return 0;
        }

        function kokushi(data){
            if (data.agari.length == 28) return 13;
            return 0;
        }

        function chinroutou(data){
            var str = data.tehai.join('') + data.agari + data.naki.join('') + data.ankan.join('');
            for (var i = 0; i < str.length; i += 2){
                var num = tile2num[str.slice(i, i + 2)];
                if (!(num % 10 == 0 || num % 10 == 9))
                    return 0;
            }
            return 13;
        }

        function sukantsu(data){
            if (xkantsu(data) == 4) return 13;
            return 0;
        }

        function tsuyisou(data){
            var str = data.tehai.join('') + data.agari + data.naki.join('') + data.ankan.join('');
            for (var i = 0; i < str.length; i += 2){
                var num = tile2num[str.slice(i, i + 2)];
                if (!(num > 29))
                    return 0;
            }
            return 13;
        }

        function ryoyisou(data){
            var str = data.tehai.join('') + data.agari + data.naki.join('') + data.ankan.join('');
            for (var i = 0; i < str.length; i += 2){
                var num = tile2num[str.slice(i, i + 2)];
                if (!(num == 21 || num == 22 || num == 23 || num == 26 || num == 28 || num == 35))
                    return 0;
            }
            return 13;
        }

        function churen(data){
            if (chinyitsu(data) == 0) return 0;
            var num = [0, 0, 0, 0, 0, 0, 0, 0, 0, 0];
            var need = [3, 1, 1, 1, 1, 0, 1, 1, 1, 3];
            var str = data.tehai.join('') + data.agari;
            for (let i = 0; i < str.length; i += 2){
                var tmp = tile2num[str.slice(i, i + 2)];
                num[tmp % 10] ++ ;
            }
            var delta = 0;
            for (let i = 0; i < num.length; i ++ )
                if (num[i] != need[i]){
                    if (num[i] - 1 != need[i])
                        return 0;
                    delta ++ ;
                }
            if (delta != 1) return 0;
            return 13;
        }

        function suankotanki(data){
            if (xanko(data) == 4 && data.agari.length == 4) return 26;
            return 0;
        }

        function kokushijusan(data){
            if (!kokushi(data)) return 0;
            var mat = Algo.str2mat(data.hai);
            var pd = true;
            for (var i = 0; i < 4; i ++ )
                for (var j = 0; j < 9; j ++ )
                    if ((i == 3 && j < 7) || (i < 3 && (j == 0 || j == 8)))
                        if (mat[i][j] == 0) pd = false;
            if (pd) return 26;
            return 0;
        }

        function junseichuren(data){
            if (churen(data) == 0) return 0;
            var num = [0, 0, 0, 0, 0, 0, 0, 0, 0, 0];
            var need = [3, 1, 1, 1, 1, 0, 1, 1, 1, 3];
            var str = data.tehai.join('') + data.agari.slice(2, data.agari.length);
            for (let i = 0; i < str.length; i += 2){
                var tmp = tile2num[str.slice(i, i + 2)];
                num[tmp % 10] ++ ;
            }
            for (let i = 0; i < num.length; i ++ )
                if (num[i] != need[i]){
                    if (num[i] != need[i])
                        return 0;
                }
            return 26;
        }

        var normalyaku = {
            'chinyitsu': chinyitsu,
            'ryanpeikou': ryanpeikou,
            'junchan': junchan,
            'honyitsu': honyitsu,
            'wreach': wreach,
            'honroutou': honroutou,
            'toitoi': toitoi,
            'sanshokudouko': sanshokudouko,
            'sananko': sananko,
            'sankantsu': sankantsu,
            'chitoitsu': chitoitsu,
            'shousangen': shousangen,
            'chanta': chanta,
            'sanshoku': sanshoku,
            'yitsu': yitsu,
            'pinfu': pinfu,
            'tanyao': tanyao,
            'yipeikou': yipeikou,
            'reach': reach,
            'yipatsu': yipatsu,
            'tsumo': tsumo,
            'haku': haku,
            'hatsu': hatsu,
            'chun': chun,
            'jifudon': jifudon,
            'bafudon': bafudon,
            'jifunan': jifunan,
            'bafunan': bafunan,
            'jifusha': jifusha,
            'bafusha': bafusha,
            'jifupei': jifupei,
            'bafupei': bafupei,
            'rinshan': rinshan,
            'chankan': chankan,
            'haitei': haitei,
            'houtei': houtei
        };

        var yakuman = {
            'daisushi': daisushi,
            'suankotanki': suankotanki,
            'kokushijusan': kokushijusan,
            'junseichuren': junseichuren,
            'tenhou': tenhou,
            'chihou': chihou,
            'daisangen': daisangen,
            'suanko': suanko,
            'shousushi': shousushi,
            'kokushi': kokushi,
            'chinroutou': chinroutou,
            'sukantsu': sukantsu,
            'tsuyisou': tsuyisou,
            'ryoyisou': ryoyisou,
            'churen': churen
        };

        var preventlist = {
            'daisushi': 'shousushi',
            'suankotanki': 'suanko',
            'kokushijusan': 'kokushi',
            'junseichuren': 'churen',
            'ryanpeikou': 'yipeikou',
            'chinyitsu': 'honyitsu',
            'junchan': 'chanta',
            'honroutou': 'chanta'
        };

        var prevent = {};

        function enumerate(data, yakulist, prevent, result){
            for (var i in yakulist){
                if (i in prevent) continue;
                var tmp = yakulist[i](data);
                if (tmp != 0){
                    result.yaku += tmp;
                    result.yakuname.splice(0, 0, { 'name': i, 'han': tmp });
                    if (i in preventlist)
                        prevent[preventlist[i]] = 1;
                }
            }
            //console.log(result);
        }

        function checknumber(str, tile){
            var res = 0;
            for (var i = 0; i < str.length; i += 2)
                if (str.slice(i, i + 2) == tile) res ++ ;
            return res;
        }

        function dora(data, result){
            var str = data.tehai.join('') + data.agari + data.naki.join('') + data.ankan.join('');
            var tmp = 0;
            for (let i = 0; i < data.dora.length; i += 2)
                tmp += checknumber(str, num2tile[nexttile[tile2num[data.dora.slice(i, i + 2)]]]);
            if (tmp > 0){
                result.yaku += tmp;
                result.yakuname.push({ 'name': 'dora', 'han': tmp });
            }
            tmp = 0;
            if (data.aka.length > 0){
                result.yaku += data.aka.length;
                result.yakuname.push({ 'name': 'aka', 'han': data.aka.length });
            }
            if (data.reach > 0){
                for (let i = 0; i < data.ura.length; i += 2)
                    tmp += checknumber(str, num2tile[nexttile[tile2num[data.ura.slice(i, i + 2)]]]);
                result.yaku += tmp;
                result.yakuname.push({ 'name': 'ura', 'han': tmp });
            }
        }
        
        enumerate(data, yakuman, prevent, result);
        if (result.yaku > 0) return result;
        enumerate(data, normalyaku, prevent, result);
        if (result.yaku == 0) return result;
        dora(data, result);
        return result;

    }

    static calcfu(data){
        var fu = 20;
        if (data.naki.length == 0 && data.tsumo == false)
            fu += 10;
        if (data.tsumo == true)
            fu += 2;
        var tsumofu = fu;
        //console.log('tsumo', fu);
        {//agari
            let agari = tile2num[data.agari.slice(0, 2)];
            let get1 = tile2num[data.agari.slice(2, 4)];
            let get2 = '';
            if (data.agari.length == 4)
                fu += 2;
            else{
                get2 = tile2num[data.agari.slice(4, 6)];
                //console.log(get1, get2, num2maty[get1 % 10], num2maty[get2 % 10]);
                if (num2maty[get2 % 10] - num2maty[get1 % 10] == 2)
                    fu += 2;
                else if (get1 != get2 && (get2 % 10 == 9 || get1 % 10 == 0))
                    fu += 2;
            }
            if (data.agari.length == 4){
                if (agari > 29 && agari % 10 == data.jifu)
                    fu += 2;
                if (agari > 29 && agari % 10 == data.bafu)
                    fu += 2;
                if (agari > 33)
                    fu += 2;
            }
            else{
                if (agari == get1){
                    let tmp = 2;
                    if (agari % 10 == 0 || agari % 10 == 9 || agari > 29)
                        tmp *= 2;
                    if (data.tsumo == true)
                        tmp *= 2;
                    fu += tmp;
                }
            }
        }
        //console.log('agari', fu);
        let tarr = data.tehai.concat(data.naki);
        for (let i = 0; i < tarr.length; i ++ ){
            let get1 = tile2num[tarr[i].slice(0, 2)];
            let get2 = tile2num[tarr[i].slice(2, 4)];
            if (tarr[i].length == 4){
                if (get1 > 29 && get1 % 10 == data.jifu)
                    fu += 2;
                if (get1 > 29 && get1 % 10 == data.bafu)
                    fu += 2;
                if (get1 > 33)
                    fu += 2;
            }
            else{
                if (get1 == get2){
                    let tmp = 2;
                    if (tarr[i].length == 8)
                        tmp *= 4;
                    if (get1 % 10 == 0 || get1 % 10 == 9 || get1 > 29)
                        tmp *= 2;
                    if (i < data.tehai.length)
                        tmp *= 2;
                    fu += tmp;
                }
            }
            //console.log(tarr[i], fu);
        }
        for (let i = 0; i < data.ankan.length; i ++ ){
            let get1 = tile2num[data.ankan[i].slice(0, 2)];
            let tmp = 16;
            if (get1 % 10 == 0 || get1 % 10 == 9 || get1 > 29)
                tmp *= 2;
            fu += tmp;
        }
        if (fu == tsumofu) data.pinfu = true;
        else data.pinfu = false;
        return fu;
    }

    static calcyakufuone(data){
        var fu = this.calcfu(data);
        var result = this.calcyaku(data);
        result.fu = fu;
        var tmpname = {};
        for (var i = 0; i < result.yakuname.length; i ++ )
            tmpname[result.yakuname[i].name] = 1;
        result.calcfu = result.fu;
        if ('chitoitsu' in tmpname) result.fu = 25;
        else if ('tsumo' in tmpname && 'pinfu' in tmpname) result.fu = 20;
        else{
            if (result.fu == 20) result.fu = 30;
            result.fu = parseInt((result.fu - 1) / 10) * 10 + 10;
        }
        return result;
    }

    static splittehai(hai, agari){
        //TODO: verify: 没有摸牌时可能是庄家第一手，已有assert，需检查是否依然正常工作
        hai = hai + agari;
        let len = hai.length / 2;
        if (agari[0] == '0')
            agari = '5' + agari[1];
        console.assert(len % 3 == 2, 'splittehai: hai + agari length not match');
        console.assert(agari.length == 2 || agari.length == 0, 'splittehai: agari length not match');
        let mat = this.str2mat(hai);
        let splitstack = [];
        let splitresult = [];
        if (len == 14){
            let kokushi = 0;
            for (let i = 0; i < 4; i ++ )
                for (let j = 0; j < 9; j ++ )
                    if ((i == 3 && j < 7) || (i < 3 && (j == 0 || j == 8)))
                        if (mat[i][j] == 0) kokushi = -10000;
                        else kokushi += mat[i][j];
            if (kokushi == 14) splitresult = [[hai]];
            let nana = 0;
            let res = [];
            for (let i = 0; i < 4; i ++ )
                for (let j = 0; j < 9; j ++ )
                    if (mat[i][j] > 0 && mat[i][j] != 2) nana = -100000;
                    else if (mat[i][j] == 2){
                        nana ++ ;
                        let tile = num2tile[mat2num[i][j]];
                        res.push(tile + tile);
                    }
            if (nana == 7) splitresult = [res];
        }
        function splitdfs(k, konum = -1){
            if (k == 0){
                splitresult.push(splitstack.slice());
                return;
            }
            if (k % 3 == 2){//find head
                for (let i = 0; i < 4; i ++ )
                    for (let j = 0; j < 9; j ++ )
                        if (mat[i][j] > 1){
                            mat[i][j] -= 2;
                            let tile = num2tile[mat2num[i][j]];
                            splitstack.push(tile + tile);
                            splitdfs(k - 2);
                            splitstack.pop();
                            mat[i][j] += 2;
                        }
                return;
            }
            for (let i = 0; i < 4; i ++ )
                for (let j = 0; j < 9; j ++ )
                    if (mat[i][j] > 0){
                        if (mat2num[i][j] != konum && i < 3 && j < 7 && mat[i][j] > 0 && mat[i][j + 1] > 0 && mat[i][j + 2] > 0){
                            mat[i][j] -- ;
                            mat[i][j + 1] -- ;
                            mat[i][j + 2] -- ;
                            let tiles = num2tile[mat2num[i][j]] + num2tile[mat2num[i][j + 1]] + num2tile[mat2num[i][j + 2]];
                            splitstack.push(tiles);
                            splitdfs(k - 3);
                            splitstack.pop();
                            mat[i][j] ++ ;
                            mat[i][j + 1] ++ ;
                            mat[i][j + 2] ++ ;
                        }
                        if (mat[i][j] > 2){
                            mat[i][j] -= 3;
                            let tile = num2tile[mat2num[i][j]];
                            splitstack.push(tile + tile + tile);
                            splitdfs(k - 3, mat2num[i][j]);
                            splitstack.pop();
                            mat[i][j] += 3;
                        }
                        return;
                    }
        }
        splitdfs(len);
        let finalresult = [];
        for (let ii = 0; ii < splitresult.length; ii ++ ){
            let tres = splitresult[ii];
            for (let i = 0; i < tres.length; i ++ ){
                if (tres[i].search(agari) != -1 && (i == 0 || tres[i] != tres[i - 1])){
                    let tt = tres.slice();
                    let agaripart = tt.splice(i, 1)[0];
                    agaripart = agari + agaripart.replace(agari, '');
                    tt.splice(0, 0, agaripart);
                    finalresult.push(tt);
                }
            }
        }
        return finalresult;
    }

    static calctensu(data, oya, tsumo, honba){
        var one = data.fu * Math.pow(2, data.yaku + 2);
        if (one >= 2000 || data.yaku == 5) one = 2000;
        if (data.yaku > 5) one = 3000;
        if (data.yaku > 7) one = 4000;
        if (data.yaku > 10) one = 6000;
        //允许多倍累计役满
        if (data.yaku > 12) one = 8000 * parseInt(data.yaku / 13);
        var result = { 'tokuten': 0, 'oya': 0, 'ko': 0 };
        if (oya && tsumo){
            result.ko = parseInt((one * 2 - 1) / 100) * 100 + 100 + honba * 100;
            result.tokuten = result.ko * 3;
        }
        else if (oya){
            result.tokuten = parseInt((one * 6 - 1) / 100) * 100 + 100 + honba * 300;
        }
        else if (tsumo){
            result.ko = parseInt((one - 1) / 100) * 100 + 100 + honba * 100;
            result.oya = parseInt((one * 2 - 1) / 100) * 100 + 100 + honba * 100;
            result.tokuten = result.oya + result.ko * 2;
        }
        else{
            result.tokuten = parseInt((one * 4 - 1) / 100) * 100 + 100 + honba * 300;
        }
        //console.log(result);
        return result;
    }

    static updateaka(data){
        data.aka = [];
        var str = data.hai + data.agari + data.naki.join('') + data.ankan.join('');
        for (var i = 0; i < str.length; i += 2)
            if (str[i] == '0')
                data.aka.push(str[i + 1]);
    }

    static calcyakufu(data){
        this.updateaka(data);
        var tehais = this.splittehai(data.hai, data.agari);
        var result = {'yaku': 0, 'fu': 0, 'yakuname': []};
        var resulttensu = { 'tokuten': 0 };
        for (var i = 0; i < tehais.length; i ++ ){
            data.agari = tehais[i][0];
            data.tehai = tehais[i];
            data.tehai.splice(0, 1);
            var oneres = this.calcyakufuone(data);
            var onetensu = this.calctensu(oneres, data.jifu == 0, data.tsumo, data.honba);
            if (onetensu.tokuten + oneres.yaku > resulttensu.tokuten + result.yaku){
                resulttensu = onetensu;
                result = oneres;
            }
        }
        //if (tehais.length > 0) result.yaku = 1;
        //console.log(resulttensu);
        return [result, resulttensu];
    }

    //找到所有形式听牌
    static findtenpai(data){
        ////FIXME: 手持(不包括副露)4张同种牌，那么不能获取第五张该牌
        var result = [];
        for (var i = 0; i < 37; i ++ )
            if (num2tile[i][0] != '0'){
                data.agari = num2tile[i];
                var splitresult = this.splittehai(data.hai, data.agari);
                if (splitresult.length > 0) result.push(num2tile[i]);
            }
        return result;
    }

}

Algo.chinmap = {};

setTimeout(function () {
    let blablablalist = Algo.listall();
    for (let i = 0; i < blablablalist.length; i ++ )
        Algo.makechinmap(blablablalist[i]);
    console.log('chinmap calc done.');
}, 100);
