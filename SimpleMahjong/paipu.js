class majsoulpaipuanalyze{

    constructor(steppause = false, selfpause = -1, oneroundpause = false){
        this.initialoya = 0;
        this.nowround = 0;
        this.steppause = steppause;
        this.selfpause = selfpause;
        this.oneroundpause = oneroundpause;
    }

    decodeUTF8(data, start = undefined, length = undefined){
        if (start == undefined){
            start = 0;
            length = data.length;
        }
        let str = '';
        for (let i = start; i < start + length; i ++ )
            str += '%' + data[i].toString(16);
        return [decodeURIComponent(str), start == undefined ? start : start + length];
    }
    
    sendmessage(msg, data){
        //console.log(msg);
        //console.log(data);
        var string = 'View.I' + data.IType + '.bind(View)';
        var func = eval(string);
        if (func != undefined && func != null){
            func(data);
        }
        //if (data.agari != undefined) console.log(data.agari[0].han);
    }
    
    getnumber(arr, start){
        var res = 0, mul = 1, str = '';
        function addzero(num, len){
            let res = num.toString(2);
            if (len > res.length)
                res = Array(len - res.length + 1).join('0') + res;
            return res;
        }
        for (; arr[start] > 127; start ++ , mul *= 128)
            str = addzero(arr[start] - 128, 7) + str;
        str = addzero(arr[start], 8) + str;
        let reverse = '0';
        if (str.length >= 64){
            reverse = '1';
            str = str.slice(str.length - 64, 999);
        }
        //console.log(str, reverse);
        for (let i = 0; i < str.length; i ++ ){
            res *= 2;
            res += reverse == str[i] ? 0 : 1;
        }
        if (reverse == '1'){
            res += 1;
            res *= -1;
        }
        return [res, start + 1];
    }
    
    getstring(arr, start, length){
        var res = ''
        for (var i = 0; i < length; i ++ )
            res += String.fromCharCode(arr[i + start]);
        return [res, start + length];
    }
    
    rotatearr(arr, delta){
        console.assert(arr.length == 4, 'in rotatearr, array length not 4');
        let res = [undefined, undefined, undefined, undefined];
        for (let i = 0; i < 4; i ++ )
            res[(i + delta) % 4] = arr[i];
        return res;
    }
    
    analyzenewround(arr, start){
        let totlength, kyoutaku = 0, honba = 0;
        [totlength, start] = this.getnumber(arr, start + 1);
        console.assert(start + totlength == arr.length, 'in AnalyzeNewRound: length not match');
        this.nowround = arr[start + 1] * 4 + arr[start + 3];
        honba = arr[start + 5];
        start += 6;
        let hands = ['', '', '', ''], dora = '', yama = '';
        let noweast = (this.initialoya + this.nowround) % 4;
        let score = [0, 0, 0, 0];
        let unknownnumber = '';
        for (; start < arr.length; ){
            if (arr[start] == 0x22){
                dora += String.fromCharCode(arr[start + 2]) + String.fromCharCode(arr[start + 3]);
                start += 4;
            }
            else if (arr[start] == 0x2a){
                let len;
                [len, start] = this.getnumber(arr, start + 1);
                for (var i = 0; i < 4; i ++ )
                    [score[i], start] = this.getnumber(arr, start);
            }
            else if (arr[start] == 0x30){
                kyoutaku = arr[start + 1];
                start += 2;
            }
            else if (arr[start] == 0x3a){
                hands[0] += String.fromCharCode(arr[start + 2]) + String.fromCharCode(arr[start + 3]);
                start += 4;
            }
            else if (arr[start] == 0x42){
                hands[1] += String.fromCharCode(arr[start + 2]) + String.fromCharCode(arr[start + 3]);
                start += 4;
            }
            else if (arr[start] == 0x4a){
                hands[2] += String.fromCharCode(arr[start + 2]) + String.fromCharCode(arr[start + 3]);
                start += 4;
            }
            else if (arr[start] == 0x52){
                hands[3] += String.fromCharCode(arr[start + 2]) + String.fromCharCode(arr[start + 3]);
                start += 4;
            }
            else if (arr[start] == 0x5a){
                //按钮?
                let len;
                [len, start] = this.getnumber(arr, start + 1);
                start += len;
            }
            else if (arr[start] == 0x72){
                let len, str;
                [len, start] = this.getnumber(arr, start + 1);
                [str, start] = this.getstring(arr, start, len);
                yama = str;
                start += len;
            }
            else{
                let cmd;
                [cmd, start] = this.getnumber(arr, start);
                let addlen = cmd % 8 == 2;
                unknownnumber += ' ' + cmd;
                [cmd, start] = this.getnumber(arr, start);
                if (addlen)
                    start += cmd;
            }
        }
        score = this.rotatearr(score, this.initialoya);
        yama = hands[noweast].slice(26, 28) + yama;
        for (let i = -1; i >= -4; i -- )
            yama = hands[(40 + noweast + i) % 4].slice(0, 26) + yama;
        var str = 'new game start, tile in hands: ' + hands + '| dora pointer: ' + dora + ' noweast ' + noweast + ' kyoutaku ' + kyoutaku + ' honba ' + honba + ' this.nowround ' + this.nowround + ' score ' + score.join(',');
        var result = {
            'IType': 'NewRound',
            'allhand': hands,
            'dora': dora,
            'east': noweast,
            'kyoutaku': kyoutaku,
            'honba': honba,
            'nowround': this.nowround,
            'score': score,
            'yama': yama,
            'dice': -1,

            ////TODO: 考虑三麻；通过手牌数量计算
            'remaintile': 69
        };
        this.sendmessage(str, result);
    }
    
    analyzediscardtile(arr, start){
        var totlength;
        [totlength, start] = this.getnumber(arr, start + 1);
        console.assert(start + totlength == arr.length, 'in AnalyzeDiscardTile: length not match');
        var num2who = ['east', 'south', 'west', 'north'];
        var num2nakiway = ['??', '??', 'chi', 'pon', '??', '??', '??', '??', '??', 'ron'];
        var whonum = arr[start + 1];
        whonum = (whonum + this.initialoya) % 4;
        start += 4;
        var discardtile = String.fromCharCode(arr[start]) + String.fromCharCode(arr[start + 1]);
        start += 2;
        var reach = 0, moqie = false, naki = false, dora = '';
        var nakiwaynum = null, nakihandtile = null;
        var unknownnumber = '';
        for (; start < arr.length; ){
            if (arr[start] == 0x18){
                if (arr[start + 1] == 1) reach = 1;
                start += 2;
            }
            else if (arr[start] == 0x52){
                naki = true;
                var nakilength, nakihandlength;
                [nakilength, start] = this.getnumber(arr, start + 1);
                var nakiend = start + nakilength;
                var nakiwaynum = arr[start + 5];
                [nakihandlength, start] = this.getnumber(arr, start + 7);
                [nakihandtile, start] = this.getstring(arr, start, nakihandlength);
                start = nakiend;
            }
            else if (arr[start] == 0x28){
                if (arr[start + 1] == 1) moqie = true;
                start += 2;
            }
            else if (arr[start] == 0x42){
                dora += String.fromCharCode(arr[start + 2]) + String.fromCharCode(arr[start + 3]);
                start += 4;
            }
            else{
                let cmd;
                [cmd, start] = this.getnumber(arr, start);
                let addlen = cmd % 8 == 2;
                unknownnumber += ' ' + cmd;
                [cmd, start] = this.getnumber(arr, start);
                if (addlen)
                    start += cmd;
            }
        }
        var str = num2who[whonum] +    ' discard tile ' + discardtile + (reach ? '' : ' not') +    ' reach,' + (moqie ? '' : ' not') +    ' moqie ' + (dora.length > 0 ? 'dora ' + dora + ' ' : '') + (naki ? 'can naki, nakiway is ' + num2nakiway[nakiwaynum] + ', nakitile is ' + nakihandtile + ' ' : '') + 'unknown numbers are ' + unknownnumber;
        var result = {
            'IType': 'DiscardTile',
            'who': whonum,
            'tile': discardtile,
            'reach': reach,
            'tsumokiri': moqie,
            'dora': dora
        };
        this.sendmessage(str, result);
        return whonum;
    }
    
    analyzedealtile(arr, start){
        var totlength;
        [totlength, start] = this.getnumber(arr, start + 1);
        console.assert(start + totlength == arr.length, 'in AnalyzeDealTile: length not match');
        var num2who = ['east', 'south', 'west', 'north'];
        var whonum = arr[start + 1];
        whonum = (whonum + this.initialoya) % 4;
        start += 2;
        var dealtile = 'unknown', unknownnumber = '', dora = '';
        var remain = 0;
        for (; start < arr.length; ){
            if (arr[start] == 0x12){
                dealtile = String.fromCharCode(arr[start + 2]) + String.fromCharCode(arr[start + 3]);
                start += 4;
            }
            else if (arr[start] == 0x18){
                remain = arr[start + 1];
                start += 2;
            }
            else if (arr[start] == 0x32){
                dora += String.fromCharCode(arr[start + 2]) + String.fromCharCode(arr[start + 3]);
                start += 4;
            }
            else{
                let cmd;
                [cmd, start] = this.getnumber(arr, start);
                let addlen = cmd % 8 == 2;
                unknownnumber += ' ' + cmd;
                [cmd, start] = this.getnumber(arr, start);
                if (addlen)
                    start += cmd;
            }
        }
        var str = num2who[whonum] + ' deal tile ' + dealtile + ' remain tile number ' + remain + ' dora is ' + dora + ' (if changed) unknown numbers are ' + unknownnumber;
        var result = {
            'IType': 'DealTile',
            'who': whonum,
            'tile': dealtile == 'unknown' ? '?x' : dealtile,
            'remaintile': remain,
            'dora': dora
        };
        this.sendmessage(str, result);
        return whonum;
    }
    
    analyzechipenggang(arr, start){
        var totlength;
        [totlength, start] = this.getnumber(arr, start + 1);
        console.assert(start + totlength == arr.length, 'in AnalyzeChiPengGang: length not match');
        var num2who = ['east', 'south', 'west', 'north'];
        var num2naki = ['chi', 'pon', 'kan', 'ankan'];
        var whonum = arr[start + 1];
        whonum = (whonum + this.initialoya) % 4;
        start += 2;
        var nakinum = 0, unknownnumber = '', tiles = '', tilefrom = '';
        var tilefromarr = [];
        for (; start < arr.length; ){
            if (arr[start] == 0x10){
                nakinum = arr[start + 1];
                start += 2;
            }
            else if (arr[start] == 0x1a){
                tiles += String.fromCharCode(arr[start + 2]) + String.fromCharCode(arr[start + 3]);
                start += 4;
            }
            else if (arr[start] == 0x22){
                var t2, t3, t4, t5;
                t2 = (arr[start + 2] + this.initialoya) % 4;
                t3 = (arr[start + 3] + this.initialoya) % 4;
                t4 = (arr[start + 4] + this.initialoya) % 4;
                tilefrom += num2who[t2];
                tilefrom += ' ' + num2who[t3];
                tilefrom += ' ' + num2who[t4];
                tilefromarr.push(t2);
                tilefromarr.push(t3);
                tilefromarr.push(t4);
                if (arr[start + 1] == 4){
                    t5 = (arr[start + 5] + this.initialoya) % 4;
                    tilefrom += ' ' + num2who[t5];
                    tilefromarr.push(t5);
                }
                start += arr[start + 1] + 2;
            }
            else if (arr[start] == 0x2a){
                //鸣了别人立直，表示谁立直以及点数变化
                let len;
                [len, start] = this.getnumber(arr, start + 1);
                start += len;
            }
            else if (arr[start] == 0x42){
                //吃碰杠以后不能打的牌
                var length;
                [length, start] = this.getnumber(arr, start + 1);
                start += length;
            }
            else{
                let cmd;
                [cmd, start] = this.getnumber(arr, start);
                let addlen = cmd % 8 == 2;
                unknownnumber += ' ' + cmd;
                [cmd, start] = this.getnumber(arr, start);
                if (addlen)
                    start += cmd;
            }
        }
        var str = num2who[whonum] + ' ' + num2naki[nakinum] + ' tiles ' + tiles + ' belonging ' + tilefrom + ' unknown numbers are ' + unknownnumber;
        var result = {
            'IType': 'ChiPengGang',
            'who': whonum,
            'naki': num2naki[nakinum],
            'tiles': tiles,
            'belong': tilefromarr
        };
        this.sendmessage(str, result);
        return whonum;
    }
    
    analyzeangangaddgang(arr, start){
        var totlength;
        [totlength, start] = this.getnumber(arr, start + 1);
        console.assert(start + totlength == arr.length, 'in AnalyzeAnGangAddGang: length not match');
        var num2who = ['east', 'south', 'west', 'north'];
        var num2naki = [undefined, undefined, 'kakan', 'ankan'];
        var whonum = arr[start + 1];
        whonum = (whonum + 4 + this.initialoya) % 4;
        start += 2;
        var nakinum = 0, unknownnumber = '', tiles = '', tilefrom = '';
        for (; start < arr.length; ){
            if (arr[start] == 0x10){
                nakinum = arr[start + 1];
                start += 2;
            }
            else if (arr[start] == 0x1a){
                tiles += String.fromCharCode(arr[start + 2]) + String.fromCharCode(arr[start + 3]);
                start += 4;
            }
            else{
                let cmd;
                [cmd, start] = this.getnumber(arr, start);
                let addlen = cmd % 8 == 2;
                unknownnumber += ' ' + cmd;
                [cmd, start] = this.getnumber(arr, start);
                if (addlen)
                    start += cmd;
            }
        }
        var str = num2who[whonum] + ' ' + num2naki[nakinum] + ' tiles ' + tiles + ' unknown numbers are ' + unknownnumber;
        var result = {
            'IType': 'AnGangAddGang',
            'who': whonum,
            'naki': num2naki[nakinum],
            'tiles': tiles,
        };
        this.sendmessage(str, result);
        return whonum;
    }

    analyzeliuju(arr, start){
        let totlength;
        [totlength, start] = this.getnumber(arr, start + 1);
        console.assert(start + totlength == arr.length, 'in AnalyzeLiuJu: length not match');
        let typenum, whonum, tiles = '', unknownnumber = '';
        for (; start < arr.length; ){
            if (arr[start] == 0x08){
                typenum = arr[start + 1];
                start += 2;
            }
            else if (arr[start] == 0x18){
                whonum = arr[start + 1];
                whonum = (whonum + 4 + this.initialoya) % 4;
                start += 2;
            }
            else if (arr[start] == 0x22){
                tiles += String.fromCharCode(arr[start + 2]) + String.fromCharCode(arr[start + 3]);
                start += 4;
            }
            else{
                let cmd;
                [cmd, start] = this.getnumber(arr, start);
                let addlen = cmd % 8 == 2;
                unknownnumber += ' ' + cmd;
                [cmd, start] = this.getnumber(arr, start);
                if (addlen)
                    start += cmd;
            }
        }
        let typename = ['00', '99', '4F', '4K', '4R'];
        let result = {
            IType: 'LiuJu',
            typenum: typenum,
            who: whonum,
            tiles: tiles
        };
        //console.log(this.onepaipu.gamedata.extra.id, whonum, typename[typenum], View.gamerecord.length);
        if (typenum < 1 || typenum > 3) this.sendmessage(whonum + ' LiuJu, ' + typename[typenum], result);
        return whonum;
    }
    
    analyzehule(arr, start){
        var totlength;
        [totlength, start] = this.getnumber(arr, start + 1);
        console.assert(start + totlength == arr.length, 'in AnalyzeHule: length not match');
        var result = {
            'IType': 'Hule',
            agari: [],
            oldscore: [],
            deltascore: [],
            finalscore: []
        };
        let unknownnumber = '';
        for (; start < arr.length; ){
            ////TODO: 包牌
            if (arr[start] == 0x0a){
                //和牌信息 可能有多个
                let agarires = {
                    hai: '',
                    naki: [],
                    ankan: [],
                    get: '',
                    dora: '',
                    ura: '',
                    han: [],
                    fu: 0,
                    who: -1,
                    tsumo: false,
                    bao: -1
                };
                let alen;
                [alen, start] = this.getnumber(arr, start + 1);
                let aend = alen + start;
                let unknownnumber = '';
                let yakumanyaku = false;
                for (; start < aend; ){
                    if (arr[start] == 0x0a){
                        //手牌
                        agarires.hai += String.fromCharCode(arr[start + 2]);
                        agarires.hai += String.fromCharCode(arr[start + 3]);
                        start += 4;
                    }
                    else if (arr[start] == 0x12){
                        //鸣牌
                        let len, str, tiles;
                        [len, start] = this.getnumber(arr, start + 1);
                        [str, start] = this.getstring(arr, start, len);
                        tiles = /\((.*?)\)/.exec(str);
                        let oldtype = tiles == null;
                        if (oldtype) tiles = str;
                        else tiles = tiles[1];
                        tiles = tiles.replace(/[,|]/g, '');
                        if ((oldtype && tiles.length == 2) || str.slice(0, 6) == 'angang'){
                            //暗杠
                            if (tiles.length == 2)
                                tiles = tiles + tiles + tiles + tiles;
                            agarires.ankan.push(tiles);
                        }
                        else{
                            agarires.naki.push(tiles);
                        }
                    }
                    else if (arr[start] == 0x1a){
                        //得牌
                        agarires.get += String.fromCharCode(arr[start + 2]);
                        agarires.get += String.fromCharCode(arr[start + 3]);
                        start += 4;
                    }
                    else if (arr[start] == 0x20){
                        //哪家和了
                        agarires.who = arr[start + 1];
                        start += 2;
                    }
                    else if (arr[start] == 0x28){
                        //是否自摸
                        agarires.tsumo = arr[start + 1] == 1;
                        start += 2;
                    }
                    else if (arr[start] == 0x30){
                        //是否为庄
                        start += 2;
                    }
                    else if (arr[start] == 0x38){
                        //是否出CI（满贯以上）
                        start += 2;
                    }
                    else if (arr[start] == 0x42){
                        //宝牌
                        agarires.dora += String.fromCharCode(arr[start + 2]);
                        agarires.dora += String.fromCharCode(arr[start + 3]);
                        start += 4;
                    }
                    else if (arr[start] == 0x4a){
                        //里宝
                        agarires.ura += String.fromCharCode(arr[start + 2]);
                        agarires.ura += String.fromCharCode(arr[start + 3]);
                        start += 4;
                    }
                    else if (arr[start] == 0x50){
                        //是否有役满役 役满役的番数均为1
                        if (arr[start + 1] == 1)
                            yakumanyaku = true;
                        start += 2;
                    }
                    else if (arr[start] == 0x58){
                        //番数
                        start += 2;
                    }
                    else if (arr[start] == 0x62){
                        //番种
                        let len6, len, name;
                        [len6, start] = this.getnumber(arr, start + 1);
                        [len, start] = this.getnumber(arr, start + 1);
                        [name, start] = this.decodeUTF8(arr, start, len);
                        let han = arr[start + 1];
                        if (
                            name == '四暗刻单骑' || 
                            name == '国士无双十三面' || 
                            name == '大四喜' || 
                            name == '纯正九莲宝灯'
                        )
                            han = 2;
                        if (yakumanyaku)
                            han *= 13;
                        agarires.han.push([name, han]);
                        start += 2;
                    }
                    else if (arr[start] == 0x68){
                        //符
                        let num;
                        [num, start] = this.getnumber(arr, start + 1);
                        agarires.fu = num;
                    }
                    else if (arr[start] == 0x72){
                        //满贯以上名称
                        let len, str;
                        [len, start] = this.getnumber(arr, start + 1);
                        [str, start] = this.decodeUTF8(arr, start, len);
                        //console.log(str);
                    }
                    else if (arr[start] == 0x78){
                        //荣点数
                        let num;
                        [num, start] = this.getnumber(arr, start + 1);
                    }
                    else if (arr[start] == 0x80 && arr[start + 1] == 0x01){
                        //自摸庄家给点
                        let num;
                        [num, start] = this.getnumber(arr, start + 2);
                    }
                    else if (arr[start] == 0x88 && arr[start + 1] == 0x01){
                        //自摸闲家给点
                        let num;
                        [num, start] = this.getnumber(arr, start + 2);
                    }
                    else{
                        let cmd;
                        [cmd, start] = this.getnumber(arr, start);
                        let addlen = cmd % 8 == 2;
                        unknownnumber += ' ' + cmd;
                        [cmd, start] = this.getnumber(arr, start);
                        if (addlen)
                            start += cmd;
                    }
                }
                result.agari.push(agarires);
            }
            else if (arr[start] == 0x12){
                let len, nums = [];
                [len, start] = this.getnumber(arr, start + 1);
                for (let i = len + start; start < i; ){
                    let tmp;
                    [tmp, start] = this.getnumber(arr, start);
                    nums.push(tmp);
                }
                result.oldscore = this.rotatearr(nums, this.initialoya);
            }
            else if (arr[start] == 0x1a){
                let len, nums = [];
                [len, start] = this.getnumber(arr, start + 1);
                for (let i = len + start; start < i; ){
                    let tmp;
                    [tmp, start] = this.getnumber(arr, start);
                    nums.push(tmp);
                }
                result.deltascore = this.rotatearr(nums, this.initialoya);
            }
            else if (arr[start] == 0x2a){
                let len, nums = [];
                [len, start] = this.getnumber(arr, start + 1);
                for (let i = len + start; start < i; ){
                    let tmp;
                    [tmp, start] = this.getnumber(arr, start);
                    nums.push(tmp);
                }
                result.finalscore = this.rotatearr(nums, this.initialoya);
            }
            else if (arr[start] == 0x3a){
                //仅在远古牌谱出现？明杠领上更新宝牌
                start += 4;
            }
            else{
                let cmd;
                [cmd, start] = this.getnumber(arr, start);
                let addlen = cmd % 8 == 2;
                unknownnumber += ' ' + cmd;
                [cmd, start] = this.getnumber(arr, start);
                if (addlen)
                    start += cmd;
            }
        }
        this.sendmessage('somebody hule.', result);
    }
    
    analyzenotile(arr, start){
        var totlength;
        [totlength, start] = this.getnumber(arr, start + 1);
        console.assert(start + totlength == arr.length, 'in AnalyzeNoTile: length not match');
        var result = {
            'IType': 'NoTile',
            oldscore: [],
            deltascore: [0, 0, 0, 0],
            mangan: [false, false, false, false],
            player: [],
        };
        let unknownnumber = '';
        for (; start < arr.length; ){
            if (arr[start] == 0x08){
                start += 2;
            }
            else if (arr[start] == 0x12){
                let alen, tres = [false, ''];
                [alen, start] = this.getnumber(arr, start + 1);
                let unknownnumber = '';
                for (let aend = start + alen; start < aend; ){
                    if (arr[start] == 0x18){
                        tres[0] = arr[start + 1] == 1;
                        start += 2;
                    }
                    else if (arr[start] == 0x22){
                        tres[1] += String.fromCharCode(arr[start + 2]);
                        tres[1] += String.fromCharCode(arr[start + 3]);
                        start += 4;
                    }
                    else if (arr[start] == 0x2a){
                        let len;
                        [len, start] = this.getnumber(arr,start + 1);
                        start += len;
                    }
                    else{
                        let cmd;
                        [cmd, start] = this.getnumber(arr, start);
                        let addlen = cmd % 8 == 2;
                        unknownnumber += ' ' + cmd;
                        [cmd, start] = this.getnumber(arr, start);
                        if (addlen)
                            start += cmd;
                    }
                }
                result.player.push(tres);
            }
            else if (arr[start] == 0x1a){
                let alen;
                [alen, start] = this.getnumber(arr, start + 1);
                let unknownnumber = '';
                for (let aend = alen + start; start < aend; ){
                    if (arr[start] == 0x08){
                        ////TODO: 两家流满会怎么样
                        result.mangan[arr[start + 1]] = true;
                        start += 2;
                    }
                    else if (arr[start] == 0x12){
                        let len, nums = [];
                        [len, start] = this.getnumber(arr, start + 1);
                        for (let i = len + start; start < i; ){
                            let tmp;
                            [tmp, start] = this.getnumber(arr, start);
                            nums.push(tmp);
                        }
                        result.oldscore = this.rotatearr(nums, this.initialoya);
                    }
                    else if (arr[start] == 0x1a){
                        let len, nums = [];
                        [len, start] = this.getnumber(arr, start + 1);
                        for (let i = len + start; start < i; ){
                            let tmp;
                            [tmp, start] = this.getnumber(arr, start);
                            nums.push(tmp);
                        }
                        result.deltascore = this.rotatearr(nums, this.initialoya);
                    }
                    else if (arr[start] == 0x22){
                        //手牌
                        start += 4;
                    }
                    else if (arr[start] == 0x2a){
                        //TODO: 待验证 很可能是鸣牌
                        let len;
                        [len, start] = this.getnumber(arr, start + 1);
                        start += len;
                    }
                    else if (arr[start] == 0x32){
                        //宝牌
                        start += 4;
                    }
                    else if (arr[start] == 0x38){
                        //得点
                        let num;
                        [num, start] = this.getnumber(arr, start + 1);
                    }
                    else{
                        let cmd;
                        [cmd, start] = this.getnumber(arr, start);
                        let addlen = cmd % 8 == 2;
                        unknownnumber += ' ' + cmd;
                        [cmd, start] = this.getnumber(arr, start);
                        if (addlen)
                            start += cmd;
                    }
                }
            }
            else{
                let cmd;
                [cmd, start] = this.getnumber(arr, start);
                let addlen = cmd % 8 == 2;
                unknownnumber += ' ' + cmd;
                [cmd, start] = this.getnumber(arr, start);
                if (addlen)
                    start += cmd;
            }
        }
        let finalscore = result.oldscore.slice(0, 4);
        for (let i = 0; i < finalscore.length; i ++ )
            finalscore[i] += result.deltascore[i];
        result.finalscore = finalscore;
        this.sendmessage('notile.', result);
    }
    
    analyzerecord(arr, start){
        var totlength, length, string;
        [totlength, start] = this.getnumber(arr, start + 1);
        console.assert(start + totlength == arr.length, 'in AnalyzeRecord: length not match');
        [length, start] = this.getnumber(arr, start + 1);
        [string, start] = this.getstring(arr, start, length);
        string = string.slice(4, 999);

        let pause = this.steppause;
        
        if (string == 'RecordNewRound'){
            this.analyzenewround(arr, start);
            if (this.selfpause != -1 || this.oneroundpause)
                pause = true;
        }
        else if (string == 'RecordDiscardTile'){
            this.analyzediscardtile(arr, start);
        }
        else if (string == 'RecordDealTile'){
            let who = this.analyzedealtile(arr, start);
            if (who == this.selfpause) pause = true;
        }
        else if (string == 'RecordChiPengGang'){
            let who = this.analyzechipenggang(arr, start);
            if (who == this.selfpause) pause = true;
        }
        else if (string == 'RecordAnGangAddGang'){
            this.analyzeangangaddgang(arr, start);
        }
        else if (string == 'RecordLiuJu'){
            this.analyzeliuju(arr, start);
            if (this.selfpause != -1 || this.oneroundpause)
                pause = true;
        }
        else if (string == 'RecordHule'){
            this.analyzehule(arr, start);
            if (this.selfpause != -1 || this.oneroundpause)
                pause = true;
        }
        else if (string == 'RecordNoTile'){
            this.analyzenotile(arr, start);
            if (this.selfpause != -1 || this.oneroundpause)
                pause = true;
        }
        else this.sendmessage('in AnalyzeRecord, ' + string, {});

        return pause;
    }

    analyzenextpart(){
        if (this.analyzeparts == undefined || this.analyzeparts.length == 0)
            return;
        let pause = this.analyzerecord(this.analyzeparts[0], 0);
        this.analyzeparts.splice(0, 1);
        if (pause) return;
        this.analyzenextpart();
    }
    
    paipuanalyze(arr){
        console.assert(arr[0] == 0x0a, 'in paipuanalyze, not start with 0x0a');
        this.initialoya = 0;
        let funclen, start = 1, funcname;
        [funclen, start] = this.getnumber(arr, start);
        [funcname, start] = this.getstring(arr, start, funclen);
        console.assert(funcname == '.lq.GameDetailRecords', 'in paipuanalyze, funcname not correct');
        console.assert(arr[start] == 0x12, 'in paipuanalyze, not 0x12');
        [funclen, start] = this.getnumber(arr, start + 1);
        console.assert(funclen + start == arr.length, 'in paipuanalyze, length not match');
        this.analyzeparts = [];
        for (; start < arr.length; ){
            if (arr[start] == 0x0a){
                let len, ss;
                [len, ss] = this.getnumber(arr, start + 1);
                this.analyzeparts.push(arr.slice(start, len + ss));
                start = ss + len;
            }
            else console.error('in paipuanalyze, unknown number ' + arr[start]);
        }
        this.analyzenextpart();
    }

    getfile(filelink){
        if (this.onepaipu == undefined){
            this.onepaipu = {
                gamedata: {
                    extra: {
                        id: filelink
                    }
                }
            };
        }
        View.INewGame({startscore: 25000});
        let request = new XMLHttpRequest();
        request.open('GET', filelink, true);
        request.responseType = 'blob';
        function loadfunc () {
            let reader = new FileReader();
            reader.readAsArrayBuffer(request.response);
            function rloadfunc (e) {
                let data = e.target.result;
                data = new Uint8Array(data);
                this.rawdata = data;
                let arr = [];
                for (let i = 0; i < data.byteLength; i ++ )
                    arr.push(data[i]);
                //console.log(arr, this.nowgamedata, this.gamedatas[this.nowgamedata].extra.id, this.gamedatas[this.nowgamedata]);
                let d = new Date();
                d.setTime(this.gamedatas[this.nowgamedata].endtime * 1000);
                if (arr.length > 99){
                    //太短是因为牌谱未被归档导致返回错误信息
                    this.paipuanalyze(arr);
                    this.onepaipu.record = JSON.parse(JSON.stringify(View.gamerecord));
                    this.paipus.push(JSON.parse(JSON.stringify(this.onepaipu)));
                    console.log('paipu #' + this.nowgamedata + ' complete. ' + d.toString());
                }
                else{
                    this.rawdata = undefined;
                    console.log('paipu #' + this.nowgamedata + ' download failed. ' + d.toString());
                }
                this.nowgamedata ++ ;
                this.converttoonepaipu();
            }
            reader.onload = rloadfunc.bind(this);
        }
        request.onload = loadfunc.bind(this);
        request.send();
    }

    converttoonepaipu(){
        if (this.nowgamedata >= this.gamedatas.length){
            console.log("paipu convert over");
            if (this.ipcrsend != undefined && this.ipcrsend && ipcr != undefined){
                let data = {
                    raw: this.rawdata,
                    paipu: this.onepaipu
                };
                ipcr.send('downloadconvertresult', data);
                this.ipcrsend = undefined;
            }
            return;
        }
        let gamedata = this.gamedatas[this.nowgamedata];
        if (gamedata.roomdata.player == 3){
            console.log('paipu #' + this.nowgamedata + ' is 3 player mahjong, skip');
            this.nowgamedata ++ ;
            this.converttoonepaipu();
            return;
        }
        let linkprefix = this.linkprefix;
        let link = linkprefix + gamedata.extra.id;
        this.onepaipu = {
            gamedata: JSON.parse(JSON.stringify(gamedata))
        };
        View.INewGame({startscore: gamedata.startpoint});
        this.getfile(link);
    }

    convertsomepaipu(gamedatas, linkprefix = 'https://mj-srv-3.majsoul.com:7343/majsoul/game_record/'){
        if (linkprefix == 't')
            linkprefix = 'http://t.zyr17.cn/static/majsoulpaipu/';
        this.linkprefix = linkprefix;
        this.paipus = [];
        this.gamedatas = gamedatas;
        this.nowgamedata = 0;
        this.converttoonepaipu();
    }

    getgamedatafromlink(filelink, startnum = 0, endnum = 10){
        let request = new XMLHttpRequest();
        request.open('GET', filelink, true);
        function loadfunc () {
            let ss = request.response.split('\n');
            let gamedatas = [];
            for (let i = 0; i < ss.length; i ++ )
                if (ss[i].length > 0)
                    gamedatas.push(JSON.parse(ss[i]));
            gamedatas = gamedatas.slice(startnum, endnum);
            //console.log(gamedatas);
            this.convertsomepaipu(gamedatas, 't');
        }
        request.onload = loadfunc.bind(this);
        request.send();
    }

}

var majsoulpaipuanalyzer = new majsoulpaipuanalyze();

class paipureplay{

    constructor(steppause = false, selfpause = -1, oneroundpause = false){
        this.steppause = steppause;
        this.selfpause = selfpause;
        this.oneroundpause = oneroundpause;
        this.debugcheck = false;
    }

    sendmessage(data){
        var string = 'View.I' + data.IType + '.bind(View)';
        var func = eval(string);
        if (func != undefined && func != null){
            func(data);
        }
    }

    INewRound(round){
        let nowr = this.nowpaipu.record[round];
        this.remaintile = nowr.remain;
        let data = {
            IType: 'NewRound',
            nowround: nowr.round,
            yama: nowr.yama,
            dice: nowr.dice,
            score: nowr.point.slice(0, 999),
            east: nowr.east,
            allhand: nowr.hand.slice(0, 999),
            dora: nowr.dora,
            honba: nowr.honba,
            kyoutaku: nowr.kyoutaku,
            remaintile: this.remaintile
        };
        this.sendmessage(data);
    }

    IDiscardTile(str){
        let data = {
            IType: 'DiscardTile',
            who: parseInt(str[1]),
            tile: str[2] + str[3],
            tsumokiri: str[4] == 1,
            reach: parseInt(str[5]),
            dora: str.slice(6, 999)
        };
        this.sendmessage(data);
        return data.who;
    }

    IDealTile(str){
        let data = {
            IType: 'DealTile',
            who: parseInt(str[1]),
            tile: str[2] + str[3],
            dora: str.slice(4, 999),
            remaintile: -- this.remaintile
        };
        this.sendmessage(data);
        return data.who;
    }

    IChiPengGang(str){
        let data = {
            IType: 'ChiPengGang',
            who: parseInt(str[4]),
            tiles: str[2] + str[3] + str.slice(5, 999),
            belong: [parseInt(str[1])]
        };
        for (; data.belong.length * 2 < data.tiles.length; data.belong.push(data.who));
        this.sendmessage(data);
        return data.who;
    }

    IAnGangAddGang(str){
        let data = {
            IType: 'AnGangAddGang',
            who: parseInt(str[1]),
            tiles: str[2] + str[3],
            naki: str[4] == '1' ? 'ankan' : 'kakan'
        };
        this.sendmessage(data);
        return data.who;
    }

    ILiuJu(str){
        let char2type = {
            '9': 1,
            '3': 0,
            'F': 2,
            'K': 3,
            'R': 4
        };
        this.roundenddata = {
            IType: 'LiuJu',
            typenum: char2type[str[1]]
        };
    }

    IHule(str){
        if (this.roundenddata == undefined)
            this.roundenddata = {
                IType: 'Hule',
                agari: []
            };
        str = str.split(',');
        let agari = {
            who: parseInt(str[1]),
            hai: str[2],
            naki: [],
            ankan: [],
            get: str[5],
            dora: str[6],
            ura: str[7],
            fu: parseInt(str[8]),
            han: [],
            tsumo: str[10] == '-1',
            bao: parseInt(str[11])
        }
        if (str[3].length != 0)
            agari.naki = str[3].split('|');
        if (str[4].length != 0)
            agari.ankan = str[4].split('|');
        let hans = str[9].split('|');
        for (let i = 0; i < hans.length; i += 2)
            agari.han.push([hans[i], parseInt(hans[i + 1])]);
        this.roundenddata.agari.push(agari);
    }

    INoTile(str){
        if (this.roundenddata == undefined)
            this.roundenddata = {
                IType: 'NoTile',
                player: [[true, ''], [true, ''], [true, ''], [true, '']],
                mangan: [false, false, false, false]
            };
        for (let i = 0; i < 4; i ++ )
            this.roundenddata.player[i][0] = str[i + 2] == '1';
        for (let i = 0; i + 6 < str.length; i ++ )
            this.roundenddata.mangan[parseInt(str[i + 6])] = true;
    }

    IFinalScore(str){
        let score = str.slice(1, 999).split('|');
        for (let i = 0; i < score.length; i ++ )
            score[i] = parseInt(score[i]);
        this.roundenddata.finalscore = score;
        this.sendmessage(this.roundenddata);
        this.roundenddata = undefined;
    }

    IOneAct(actstr){
        let pause;
        if (actstr[0] == 'A'){
            this.IDiscardTile(actstr);
        }
        else if (actstr[0] == 'B'){
            let who = this.IDealTile(actstr);
            if (who == this.selfpause) pause = true;
        }
        else if (actstr[0] == 'C'){
            let who = this.IChiPengGang(actstr);
            if (who == this.selfpause) pause = true;
        }
        else if (actstr[0] == 'D'){
            this.IAnGangAddGang(actstr);
        }
        else if (actstr[0] == 'X'){
            this.IHule(actstr);
        }
        else if (actstr[0] == 'Y'){
            if (actstr[1] == 'N'){
                this.INoTile(actstr);
            }
            else if (actstr[1] == 'F'){
                this.ILiuJu(actstr);
            }
            else if (actstr[1] == 'K'){
                this.ILiuJu(actstr);
            }
            else if (actstr[1] == 'R'){
                this.ILiuJu(actstr);
            }
            else if (actstr[1] == '9'){
                this.ILiuJu(actstr);
            }
            else if (actstr[1] == '3'){
                this.ILiuJu(actstr);
            }
        }
        else if (actstr[0] == 'Z'){
            this.IFinalScore(actstr);
            if (this.selfpause != -1 || this.oneroundpause)
                pause = true;
        }
        else this.sendmessage('in AnalyzeRecord, ' + string, {});
        return pause;
    }

    IReplayEnd(){
        if (this.debugcheck){
            this.checkpaipu.record = JSON.parse(JSON.stringify(View.gamerecord));
            this.checkresult = JSON.stringify(this.checkpaipu) == JSON.stringify(this.nowpaipu);
        }
    }

    IOneStep(){
        let round, act;
        [round, act] = this.replaypos;
        if (round == -1)
            return;
        if (round == this.nowpaipu.record.length){
            this.IReplayEnd();
            return;
        }
        let pause = this.steppause;
        if (act == -1){
            this.INewRound(round);
            if (this.selfpause != -1 || this.oneroundpause)
                pause = true;
        }
        else{
            pause = this.IOneAct(this.nowpaipu.record[round].action[act]);
        }
        act ++ ;
        if (this.step != undefined && act % this.step.step == 0)
            this.step.roundstep.push(JSON.parse(JSON.stringify(View.matchdata)));
        if (act >= this.nowpaipu.record[round].action.length){
            act = -1;
            round ++ ;
            if (this.step != undefined){
                this.step.roundstep.push(JSON.parse(JSON.stringify(View.matchdata)));
                this.step.gamestep.push(this.step.roundstep);
                this.step.roundstep = [];
            }
        }
        this.replaypos = [round, act];
        if (pause) return;
        this.IOneStep();
    }

    replay(paipu){
        this.nowpaipu = JSON.parse(JSON.stringify(paipu));
        if (this.debugcheck){
            this.checkpaipu = {
                gamedata: JSON.parse(JSON.stringify(paipu.gamedata))
            };
        }
        this.sendmessage({IType: 'NewGame', startscore: paipu.gamedata.startpoint});
        this.replaypos = [0, -1];
        this.IOneStep();
    }

    recordstep(paipus, step = 11){
        this.step = {
            step: step,
            totalstep: []
        };
        for (let i in paipus){
            let paipu = paipus[i];
            console.log('recordstep #' + i + ', total: ' + paipus.length);
            this.step.gamestep = [];
            this.step.roundstep = [];
            this.replay(paipu);
            this.step.totalstep.push(this.step.gamestep);
        }
    }

}

var paipureplayer = new paipureplay();