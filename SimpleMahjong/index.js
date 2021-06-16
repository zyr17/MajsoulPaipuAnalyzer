"use strict";

function changesize(){
    var num = window.innerHeight;
    if (window.innerWidth < num)
        num = window.innerWidth;
    num /= 1200.0;
    $('body > div > div').attr('style', 'position: relative; transform-origin: 0% 0%; transform: scale(' + parseFloat(num) + ', ' + parseFloat(num) + ');');
}

class Viewprototype{

    constructor(matchdata){
        this.matchdata = matchdata;
        this.GraphicUpdate = true;
    }

    updatehand(num){
        if (!this.GraphicUpdate)
            return;
        var ltrb = num2ltrb[num];
        var inhand = $('.' + ltrb + ' .inhand');
        var get = $('.' + ltrb + ' .get');
        var show = $('.' + ltrb + ' .show');
        inhand.text('');
        show.text('');
        var str = '';
        for (let i = 0; i < this.matchdata.data[num].hand.length; i ++ )
            str += this.matchdata.data[num].hand[i];
        inhand.text(str);
        get.text(this.matchdata.data[num].get);
        str = '';
        for (let i = 0; i < this.matchdata.data[num].show.length; i ++ ){
            if (i > 0) str += ' ';
            str += this.matchdata.data[num].show[i];
        }
        show.text(str);
    }
    
    updatetable(num, shownakitile = false){
        if (!this.GraphicUpdate)
            return;
        var ltrb = num2ltrb[num];
        var table = $('.' + ltrb + '.table');
        table.text('');
        var str = '';
        var count = 0;
        for (var i = 0; i < this.matchdata.data[num].table.length; i ++ ){
            var tile = this.matchdata.data[num].table[i];
            if (tile[tile.length - 1] == '*' && !shownakitile)
                continue;
            str += tile;
            if (count % 6 == 5) str += '\n';
            count ++ ;
        }
        table.text(str);
    }

    updatecentercorner(east){
        if (!this.GraphicUpdate)
            return;
        for (var i = 0; i < 4; i ++ )
            $('.corner.' + num2ltrb[(i + east) % 4]).text(fonname[i]);
    }
    
    updatecenter(){
        if (!this.GraphicUpdate)
            return;
        $('.corner').removeClass('light');
        $('.corner.' + num2ltrb[this.matchdata.now]).addClass('light');
        for (var i = 0; i < 4; i ++ ){
            var ltrb = num2ltrb[i];
            $('.score.' + ltrb).text(this.matchdata.data[i].score);
            $('.reach.' + ltrb).text(this.matchdata.data[i].reach != 0 ? '----.----' : '');
        }
        $('#matchtitle').text(this.matchdata.title);
        $('#remaintile').text(this.matchdata.remain);
        $('#dora').text(this.matchdata.dora);
        for (; $('#dora').text().length < 10; $('#dora').text($('#dora').text() + '?x'));
        $('#honba').text(this.matchdata.honba);
        $('#kyoutaku').text(this.matchdata.kyoutaku);
    }
    
    comparetile(x, y) {
        console.assert(tile2num[x] != null, 'tile ' + x + ' illegal');
        console.assert(tile2num[y] != null, 'tile ' + y + ' illegal');
        return tile2num[x] - tile2num[y];
    }
    
    handinit(ltrb, str){
        console.assert(str.length == 26 || str.length == 28, 'init length not 26 or 28');
        var num = ltrb2num[ltrb];
        this.matchdata.data[num].hand = [];
        for (var i = 0; i < str.length; i += 2)
            this.matchdata.data[num].hand.push(str.slice(i, i + 2));
        this.matchdata.data[num].hand.sort(this.comparetile);
        this.updatehand(num);
    }
    
    cleartable(){
        this.matchdata.data[0].table = [];
        this.matchdata.data[1].table = [];
        this.matchdata.data[2].table = [];
        this.matchdata.data[3].table = [];
        this.updatetable(0);
        this.updatetable(1);
        this.updatetable(2);
        this.updatetable(3);
    }
    
    removetile(num, str){
        console.assert(str.length == 2, 'removetile length not 2, ' + str);
        if (this.matchdata.data[num].get != '' && this.matchdata.data[num].get != null){
            this.matchdata.data[num].hand.push(this.matchdata.data[num].get);
            this.matchdata.data[num].get = '';
            this.matchdata.data[num].hand.sort(this.comparetile);
        }
        for (let i = 0; i < this.matchdata.data[num].hand.length; i ++ )
            if (this.matchdata.data[num].hand[i] == str){
                this.matchdata.data[num].hand.splice(i, 1);
                return;
            }
        for (let i = 0; i < this.matchdata.data[num].hand.length; i ++ )
            if (this.matchdata.data[num].hand[i] == '?x'){
                this.matchdata.data[num].hand.splice(i, 1);
                return;
            }
        console.assert(false, 'removetile not found', str, JSON.parse(JSON.stringify(this.matchdata)));
    }
    
    getonetile(ltrb, str){
        var num = ltrb2num[ltrb];
        this.matchdata.now = num;
        this.updatecenter();
        this.matchdata.data[num].get = str;
        this.updatehand(num);
    }

    gettablelast(num){
        if (this.matchdata.data[num].table.length == 0)
            return undefined;
        return this.matchdata.data[num].table[this.matchdata.data[num].table.length - 1];
    }
    
    naki(who, from, use1, use2, get, use3 = undefined){
        var numwho = ltrb2num[who];
        var numfrom = ltrb2num[from];
        this.matchdata.now = numwho;
        this.removetile(numwho, use1);
        this.removetile(numwho, use2);
        if (use3 != undefined) this.removetile(numwho, use3);
        var tablelast = this.gettablelast(numfrom);
        console.assert(tablelast.slice(0, 2) == get, 'naki get different with last table tile.', who, from, use1, use2, get, use3, JSON.parse(JSON.stringify(this.matchdata)));
        this.matchdata.data[numfrom].table.splice(this.matchdata.data[numfrom].table.length - 1, 1);
        this.matchdata.data[numfrom].table.push(tablelast + '*');
        var delta = (numfrom + 4 - numwho) % 4;
        console.assert(delta != 0, 'naki self tile');
        var str = '';
        if (use3 != undefined) use2 += use3;
        if (delta == 1) str = use1 + use2 + '[' + get + ']';
        if (delta == 2) str = use1 + '[' + get + ']' + use2;
        if (delta == 3) str = '[' + get + ']' + use1 + use2;
        this.matchdata.data[numwho].show.splice(0, 0, str);
        this.updatehand(numwho);
        this.updatetable(numfrom);
        this.updatecenter();
    }
    
    ankankakan(who, use, naki){
        var numwho = ltrb2num[who];
        var handnum = Algo.strmatchnum(this.matchdata.data[numwho].hand.join('') + this.matchdata.data[numwho].get, use);
        var nakinum = Algo.strmatchnum(this.matchdata.data[numwho].show.join('').replace(/[\[\]]/g, ''), use);
        if (handnum == 4){
            //ankan
            if (use[0] == '0' || use[0] == '5' && use[1] != 'z'){
                var show = '';
                var akanum = Algo.strmatchnumone(this.matchdata.data[numwho].hand.join('') + this.matchdata.data[numwho].get, '0' + use[1]);
                var othernum = 4 - akanum;
                for (let i = 0; i < akanum; i ++ ){
                    show += '0' + use[1];
                    this.removetile(numwho, '0' + use[1]);
                }
                for (let i = 0; i < othernum; i ++ ){
                    show += '5' + use[1];
                    this.removetile(numwho, '5' + use[1]);
                }
                this.matchdata.data[numwho].show.splice(0, 0, '[' + show + ']');
            }
            else{
                for (let i = 0; i < 4; i ++ )
                    this.removetile(numwho, use);
                this.matchdata.data[numwho].show.splice(0, 0, '[' + use + use + use + use + ']');
            }
        }
        else if (nakinum == 3){
            //kakan
            for (let i = 0; i < this.matchdata.data[numwho].show.length; i ++ ){
                let str = this.matchdata.data[numwho].show[i];
                let str2 = str.replace(/[\[\]]/g, '').slice(0, 2);
                if (str2[0] == '0')
                    str2 = '5' + str2[1];
                let use2 = use;
                if (use2[0] == '0')
                    use2 = '5' + use2[1];
                if (str2 == use2){
                    str2 = str.replace(/\]/, '][' + use + ']');
                    this.matchdata.data[numwho].show[i] = str2;
                    this.removetile(numwho, use);
                    break;
                }
            }
        }
        else if (this.matchdata.data[numwho].hand[0] == '?x'){
            if (naki == 'ankan'){
                for (let i = 0; i < 4; i ++ )
                    this.removetile(numwho, '?x');
                this.matchdata.data[numwho].show.splice(0, 0, '[' + use + use + use + use + ']');
            }
            else{
                this.removetile(numwho, '?x');
                for (let i = 0; i < this.matchdata.data[numwho].show.length; i ++ ){
                    let str = this.matchdata.data[numwho].show[i];
                    let str2 = str.replace(/[\[\]]/g, '').slice(0, 2);
                    if (str2[0] == '0')
                        str2 = '5' + str2[1];
                    let use2 = use;
                    if (use2[0] == '0')
                        use2 = '5' + use2[1];
                    if (str2 == use2){
                        str2 = str.replace(/\]/, '][' + use + ']');
                        this.matchdata.data[numwho].show[i] = str2;
                        this.removetile(numwho, use);
                        break;
                    }
                }
            }
        }
        else console.assert(false, 'ankan & kakan: num not match.', use);
        this.updatehand(numwho);
    }
    
    throwonetile(ltrb, str){
        var num = ltrb2num[ltrb];
        this.removetile(num, str);
        var tablelast = this.gettablelast(num);
        if (tablelast != undefined && tablelast.length == 4){
            //长度为4必定为立直牌+被鸣牌。因此需要补上立直标记。
            str += '-';
        }
        this.matchdata.data[num].table.push(str);
        this.updatehand(num);
        this.updatetable(num);
    }
    
    declarereach(ltrb, reach = 1){
        var num = ltrb2num[ltrb];
        console.assert(!this.matchdata.data[num].reach > 0, 'in declearreach: already reached');
        console.assert(this.needkyoutaku == undefined, 'in declearreach: someone not put kyoutaku');
        this.matchdata.data[num].reach = reach;
        this.needkyoutaku = [num, reach];
        var tablelast = this.gettablelast(num);
        this.matchdata.data[num].table.splice(this.matchdata.data[num].table.length - 1, 1);
        this.matchdata.data[num].table.push(tablelast + '-');
        this.updatecenter();
        this.updatetable(num);
    }

    putkyoutaku(){
        if (this.needkyoutaku == undefined)
            return;
        let num, reach;
        [num, reach] = this.needkyoutaku;
        this.needkyoutaku = undefined;
        this.matchdata.data[num].score -= 1000;
        this.matchdata.kyoutaku ++ ;
    }

    showkuikae(who, kuikae){
        let mdata = this.matchdata.data[who];
        let oldhand = JSON.parse(JSON.stringify(mdata.hand));
        for (let i in kuikae)
            for (let j in mdata.hand)
                if (kuikae[i] == mdata.hand[j])
                    mdata.hand[j] += '#';
        this.updatehand(who);
        mdata.hand = oldhand;
    }

    removekuikae(who){
        this.updatehand(who);
    }
    
    newgame(startscore = 25000){
        for (var i = 0; i < 4; i ++ ){
            this.matchdata.data[i].hand = '';
            this.matchdata.data[i].get = '';
            this.matchdata.data[i].reach = 0;
            this.matchdata.data[i].score = startscore;
            this.matchdata.data[i].show = [];
            this.matchdata.data[i].table = [];
        }
    }
    
    newround(bottom, right, top, left, get, score, east, nowround, dora, kyoutaku, honba, remain){
        for (let i = 0; i < 4; i ++ ){
            this.matchdata.data[i].hand = [];
            this.matchdata.data[i].get = '';
            this.matchdata.data[i].show = [];
            this.matchdata.data[i].table = [];
            this.matchdata.data[i].reach = 0;
            this.updatetable(i);
        }
        this.handinit('bottom', bottom);
        this.handinit('right', right);
        this.handinit('top', top);
        this.handinit('left', left);
        this.matchdata.now = 0;
        for (let i = 0; i < 4; i ++ )
            this.matchdata.data[i].score = score[i];
        this.matchdata.east = east;
        this.matchdata.now = east;
        this.matchdata.honba = honba;
        this.matchdata.kyoutaku = kyoutaku;
        this.matchdata.remain = remain;
        this.matchdata.nowround = nowround;
        var title = fonname[parseInt(nowround / 4) % 4] + suji[nowround % 4 + 1] + '局';
        this.matchdata.title = title;
        this.matchdata.dora = dora;
        this.needkyoutaku = undefined;
        this.updatecentercorner(east);
        this.updatecenter();
        if (get != undefined && get != '') this.getonetile(num2ltrb[east], get);
    }

    INewGame(data){
        this.newgame(data.startscore);
        this.gamerecord = [];
    }
    
    INewRound(data){
        this.roundrecord = {
            round: data.nowround,
            yama: data.yama.slice(),
            dice: data.dice,
            point: data.score.slice(),
            east: data.east,
            hand: data.allhand != undefined ? data.allhand.slice(0, 999) : ['', '', '', ''],
            dora: data.dora,
            honba: data.honba,
            kyoutaku: data.kyoutaku,
            remain: data.remaintile,
            action: []
        };
        var tmp = [
            '?x?x?x?x?x?x?x?x?x?x?x?x?x',
            '?x?x?x?x?x?x?x?x?x?x?x?x?x',
            '?x?x?x?x?x?x?x?x?x?x?x?x?x',
            '?x?x?x?x?x?x?x?x?x?x?x?x?x'
        ];
        tmp[0] = data.hand;
        if (data.allhand != null && data.allhand != undefined)
            tmp = data.allhand;
        if (tmp[data.east].slice(0, 2) == '?x' && tmp[data.east].length == 26)
            tmp[data.east] += '?x';
        this.newround(tmp[0], tmp[1], tmp[2], tmp[3], '', data.score, data.east, data.nowround, data.dora, data.kyoutaku, data.honba, data.remaintile);
    }
    
    IDiscardTile(data){
        let astr = 'A';
        astr += data.who;
        astr += data.tile;
        astr += data.tsumokiri ? 1 : 0;
        astr += data.reach;
        if (data.dora != undefined && data.dora.length > 0)
            astr += data.dora;
        this.roundrecord.action.push(astr);
        var ltrb = num2ltrb[data.who];
        this.throwonetile(ltrb, data.tile);
        if (data.dora != undefined && data.dora != ''){
            this.matchdata.dora = data.dora;
            this.updatecenter();
        }
        if (data.reach > 0) this.declarereach(ltrb, data.reach);
    }
    
    IDealTile(data){
        this.putkyoutaku();
        let astr = 'B';
        astr += data.who;
        astr += data.tile;
        if (data.dora != undefined && data.dora.length > 0)
            astr += data.dora;
        this.roundrecord.action.push(astr);
        var ltrb = num2ltrb[data.who];
        this.getonetile(ltrb, data.tile);
        this.matchdata.remain = data.remaintile;
        if (data.dora != undefined && data.dora != '')
            this.matchdata.dora = data.dora;
        this.updatecenter();
    }
    
    IChiPengGang(data){
        this.putkyoutaku();
        var who = data.who, from = 0;
        var get = '';
        var use = [];
        for (var i = 0; i < data.belong.length; i ++ ){
            if (data.belong[i] == who) use.push(data.tiles.slice(i * 2, i * 2 + 2));
            else{
                get = data.tiles.slice(i * 2, i * 2 + 2);
                from = data.belong[i];
            }
        }
        let astr = 'C';
        astr += from;
        astr += get;
        astr += who;
        astr += use.join('');
        this.roundrecord.action.push(astr);
        //console.log(data);
        this.naki(num2ltrb[who], num2ltrb[from], use[0], use[1], get, use[2]);
    }
    
    IAnGangAddGang(data){
        let astr = 'D';
        astr += data.who;
        astr += data.tiles;
        astr += data.naki == 'ankan' ? '1' : '0';
        this.roundrecord.action.push(astr);
        var ltrb = num2ltrb[data.who];
        this.ankankakan(ltrb, data.tiles.slice(0, 2), data.naki);
    }

    ILiuJu(data){
        this.putkyoutaku();
        let type2char = ['3', '9', 'F', 'K', 'R'];
        this.roundrecord.action.push('Y' + type2char[data.typenum]);
        let astr = 'Z';
        for (let i = 0; i < 4; i ++ )
            astr += this.matchdata.data[i].score + (i < 3 ? '|' : '');
        this.roundrecord.action.push(astr);
        if (this.gamerecord == undefined) this.gamerecord = [];
        this.gamerecord.push(JSON.parse(JSON.stringify(this.roundrecord)));
    }
    
    IHule(data){
        for (let i = 0; i < data.agari.length; i ++ ){
            let agari = data.agari[i], from = this.matchdata.now;
            let hanstr = [];
            for (let j = 0; j < agari.han.length; j ++ ){
                let nowhanname = agari.han[j][0];
                if (nowhanname == '役牌:门风牌'){
                    let jifunum = (agari.who - this.matchdata.east + 4) % 4;
                    nowhanname = '自风' + (jifunum == 0 ? '东' : fonname[jifunum]);
                }
                if (nowhanname == '役牌:场风牌'){
                    let bafunum = parseInt(this.matchdata.nowround / 4);
                    nowhanname = '场风' + (bafunum == 0 ? '东' : fonname[bafunum]);
                }
                if (nowhanname == '河底摸鱼'){
                    nowhanname = '河底捞鱼';
                }
                if (nowhanname == '海底捞月'){
                    nowhanname = '海底摸月';
                }
                let addflag = false;
                for (let i = 0; i < hanname.length; i ++ )
                    if (parseInt(nowhanname) == i || hanname[i][0] == nowhanname || hanname[i][1] == nowhanname){
                        hanstr.push(i);
                        addflag = true;
                        break;
                    }
                if (!addflag){
                    console.error('in IHule: unknown hanname', nowhanname);
                }
                hanstr.push(agari.han[j][1]);
            }
            hanstr = hanstr.join('|');
            let astr = ['X'];
            astr.push(agari.who);
            astr.push(agari.hai);
            astr.push(agari.naki.join('|'));
            astr.push(agari.ankan.join('|'));
            astr.push(agari.get);
            astr.push(agari.dora);
            astr.push(agari.ura);
            astr.push(agari.fu);
            astr.push(hanstr);
            astr.push(agari.tsumo ? -1 : from);
            astr.push(agari.bao);
            astr = astr.join(',');
            this.roundrecord.action.push(astr);
        }
        let astr = 'Z';
        astr += data.finalscore.join('|');
        this.roundrecord.action.push(astr);
        if (this.gamerecord == undefined) this.gamerecord = [];
        this.gamerecord.push(JSON.parse(JSON.stringify(this.roundrecord)));
        for (let i = 0; i < data.finalscore.length; i ++ )
            this.matchdata.data[i].score = data.finalscore[i];
    }
    
    INoTile(data){
        let astr = 'YN';
        for (let i = 0; i < 4; i ++ )
            astr += data.player[i][0] ? '1' : '0';
        for (let i = 0; i < 4; i ++ )
            if (data.mangan[i]) astr += i;
        this.roundrecord.action.push(astr);
        this.roundrecord.action.push('Z' + data.finalscore.join('|'));
        if (this.gamerecord == undefined) this.gamerecord = [];
        this.gamerecord.push(JSON.parse(JSON.stringify(this.roundrecord)));
        for (let i = 0; i < data.finalscore.length; i ++ )
            this.matchdata.data[i].score = data.finalscore[i];
    }

    IAction(data, objname){
        if (objname == undefined)
            objname = data.constructor.name;
        let action = objname.slice(6); //开头可能是Action或者Record。内部结构基本一样
        if (action == 'MJStart') return;
        if (objname == 'ResAuthGame' || objname == 'GameRecord')
            this.INewGame(majsouldataconvert(data, objname));
        else this['I' + action](majsouldataconvert(data, action));
        //console.log(action, ProtobufObject2Object(data));
    }
}

class ViewPic extends Viewprototype{
    constructor(matchdata){
        super(matchdata);
    }

    //根据字符串画出牌；可以画若干长度的普通牌，或一组鸣牌，或牌河中有立直牌；不能多组鸣牌或者和普通牌混合。
    gettilediv(str){
        let resdiv = $('<div></div>');
        resdiv.addClass('tilediv');
        let lastchild = undefined;
        for (let i = 0; i < str.length; i += 2){
            if (str[i] == '*'){
                //前一个元素半透明化处理，表示牌河中被鸣的牌
                lastchild.css('opacity', '0.5');
                i -- ;
            }
            else if (str[i] == '#'){
                //前一个元素半透明化处理，表示牌河中摸切牌和手牌因食替无法打出的牌
                //均为半透明化，因此只能同时采用一种
                lastchild.css('opacity', '0.5');
                i -- ;
            }
            else if (str[i] == '['){
                if (i == 0 && str.length == 10 && str[9] == ']'){
                    //ankan
                    let zerocount = 0;
                    let kannum = 0;
                    for (let j = 1; j < 9; j += 2)
                        if (str[j] == '0')
                            zerocount ++ ;
                        else
                            kannum = str[j];
                    let kantype = str[2];
                    let tdiv = $('<div></div>').appendTo(resdiv);
                    tdiv.addClass('tile');
                    tdiv.addClass('tbk');
                    tdiv = $('<div></div>').appendTo(resdiv);
                    tdiv.addClass('tile');
                    let tile = kannum + kantype;
                    if (zerocount > 0){
                        zerocount -- ;
                        tile = '0' + kantype;
                    }
                    tdiv.addClass('t' + tile);
                    tdiv = $('<div></div>').appendTo(resdiv);
                    tdiv.addClass('tile');
                    tile = kannum + kantype;
                    if (zerocount > 0){
                        zerocount -- ;
                        tile = '0' + kantype;
                    }
                    tdiv.addClass('t' + tile);
                    tdiv = $('<div></div>').appendTo(resdiv);
                    tdiv.addClass('tile');
                    tdiv.addClass('tbk');
                    lastchild = tdiv;
                    break;
                }
                else if (str[i + 4] == '['){
                    //kakan
                    let tiles = str[i + 1] + str[i + 2] + str[i + 5] + str[i + 6];
                    let tdiv = $('<div></div>').appendTo(resdiv);
                    tdiv.addClass('drtile');
                    tdiv.addClass('drt' + tiles);
                    lastchild = tdiv;
                    i += 6;
                }
                else{
                    //chi pon kan
                    let tile = str[i + 1] + str[i + 2];
                    let tdiv = $('<div></div>').appendTo(resdiv);
                    tdiv.addClass('rtile');
                    tdiv.addClass('rt' + tile);
                    lastchild = tdiv;
                    i += 2;
                }
            }
            else{
                let tile = str.slice(i, i + 2);
                let num = tile2num[tile];
                if (num == undefined){
                    console.error('in gettilediv: tile not exist', str, tile);
                    continue;
                }
                if (tile == '?x')
                    tile = 'bk';
                let newdiv = $('<div></div>').appendTo(resdiv);
                newdiv.addClass('tile');
                newdiv.addClass('t' + tile);
                lastchild = newdiv;
            }
        }
        return resdiv;
    }

    updatecenter(){
        if (!this.GraphicUpdate)
            return;
        $('.corner').removeClass('light');
        $('.reach').removeClass('tenbou');
        $('.corner.' + num2ltrb[this.matchdata.now]).addClass('light');
        for (var i = 0; i < 4; i ++ ){
            var ltrb = num2ltrb[i];
            $('.score.' + ltrb).text(this.matchdata.data[i].score);
            if (this.matchdata.data[i].reach != 0)
                $('.reach.' + ltrb).addClass('tenbou');
        }
        $('#matchtitle').text(this.matchdata.title);
        $('#remaintile').text(this.matchdata.remain);
        let dorastr = this.matchdata.dora;
        for (; dorastr.length < 10; dorastr += '?x');
        let doradiv = this.gettilediv(dorastr);
        doradiv.css('transform','scale(0.7, 0.7)');
        doradiv.css('z-index', '100');
        $('#dora').empty();
        doradiv.appendTo($('#dora'));
        $('#honba').text(this.matchdata.honba);
        $('#kyoutaku').text(this.matchdata.kyoutaku);
    }

    updatehand(num){
        if (!this.GraphicUpdate)
            return;
        let ltrb = num2ltrb[num];
        let hand = $('.' + ltrb + '.hand');
        let handleftdiv = $('.' + ltrb + '.hand > .leftorder');
        let inhanddiv = $('.' + ltrb + '.hand .inhand');
        let getdiv = $('.' + ltrb + '.hand .get');
        let showdiv = $('.' + ltrb + '.hand .show');
        let lastshownum = showdiv.children().length;
        // hand.empty();
        inhanddiv.empty();
        getdiv.empty();
        showdiv.empty();
        let inhandtilediv = this.gettilediv(this.matchdata.data[num].hand.join(''));
        inhandtilediv.appendTo(inhanddiv);
        let get = this.matchdata.data[num].get;
        let gettiledivi = this.gettilediv(get);
        if (get != undefined && get != null && get.length != 0){
            gettiledivi.appendTo(getdiv);
        }
        let showtiledivwidth = 0;
        for (var i = 0; i < this.matchdata.data[num].show.length; i ++ ){
            let showtilediv = this.gettilediv(this.matchdata.data[num].show[i]);
            showtilediv.appendTo(showdiv);
            showtiledivwidth += showtilediv.width();
            showtilediv.width(showtilediv.width());
        }
        if (lastshownum != this.matchdata.data[num].show.length){
            // set div width 
            inhanddiv.width(inhandtilediv.width());
            getdiv.width(gettiledivi.width());
            showdiv.width(showtiledivwidth);
            let handwidth = hand.width();
            showdiv.width(this.matchdata.data[num].show.length * 200);
            handleftdiv.width(handwidth - showdiv.width());
        }
    }

    updatetable(num, shownakitile = false){
        ////TODO: 牌带#表示该牌摸切。检查所有地方，检查标记是否存在时不默认标记顺序。
        if (!this.GraphicUpdate)
            return;
        let ltrb = num2ltrb[num];
        let table = $('.' + ltrb + '.table');
        table.empty();
        let str = '';
        let count = 0;
        for (let i = 0; i < this.matchdata.data[num].table.length; i ++ ){
            let tile = this.matchdata.data[num].table[i];
            if (tile[tile.length - 1] == '*' && !shownakitile)
                continue;
            let isnaki = tile[tile.length - 1] == '*';
            let isreach = tile.length > 2 && tile[2] == '-';
            tile = tile.slice(0, 2);
            if (isreach)
                tile = '[' + tile.slice(0, 2) + ']';
            if (isnaki)
                tile += '*';
            str += tile;
            if (count % 6 == 5){
                let tdiv = this.gettilediv(str);
                str = '';
                tdiv.appendTo(table);
            }
            count ++ ;
        }
        let tdiv = this.gettilediv(str);
        tdiv.appendTo(table);
    }
}

var View = new ViewPic(emptymatchdata());


$(document).ready(function(){
    changesize();
});

window.onresize = function(){
    changesize();
};