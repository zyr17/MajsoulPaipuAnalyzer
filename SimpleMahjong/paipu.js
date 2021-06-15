class majsoulpaipuanalyze{

    constructor(steppause = false, selfpause = -1, oneroundpause = false){
        this.initialoya = 0;
        this.nowround = 0;
        this.steppause = steppause;
        this.selfpause = selfpause;
        this.oneroundpause = oneroundpause;
        this.onlydownload = false;
    }
    
    analyzeandpause(data){
        let string = data.constructor.name;
        let pause = this.steppause;
        let who = data.seat == undefined ? undefined : (data.seat + InitialOya) % 4;

        View.IAction(data);
        
        if (string == 'RecordNewRound'){
            if (this.selfpause != -1 || this.oneroundpause)
                pause = true;
        }
        else if (string == 'RecordDiscardTile'){

        }
        else if (string == 'RecordDealTile'){
            if (who != undefined && who == this.selfpause) pause = true;
        }
        else if (string == 'RecordChiPengGang'){
            if (who != undefined && who == this.selfpause) pause = true;
        }
        else if (string == 'RecordAnGangAddGang'){

        }
        else if (string == 'RecordLiuJu'){
            if (this.selfpause != -1 || this.oneroundpause)
                pause = true;
        }
        else if (string == 'RecordHule'){
            if (this.selfpause != -1 || this.oneroundpause)
                pause = true;
        }
        else if (string == 'RecordNoTile'){
            if (this.selfpause != -1 || this.oneroundpause)
                pause = true;
        }
        else console.log('in AnalyzeRecord, ' + string, {});

        return pause;
    }

    analyzenextpart(){
        if (this.pdatarecords == undefined || this.pdatarecords.length == 0)
            return;
        let pause = this.analyzeandpause(this.pdatarecords[0]);
        this.pdatarecords.splice(0, 1);
        if (pause) return;
        this.analyzenextpart();
    }

    getfile(filelink, bytearr = null){
        if (this.onepaipu == undefined){
            this.onepaipu = { gamedata: { extra: { id: filelink } } };
        }
        InitialOya = 0;
        View.INewGame({startscore: 25000});
        let request;
        function loadfunc (resp, bytearr) {
            function rloadfunc (e) {
                try{
                    let data;
                    if (e.target){
                        data = e.target.result;
                        data = new Uint8Array(data);
                    }
                    else{
                        data = e;
                    }

                    this.rawdata = data;
                    if (this.onlydownload) throw "only download this paipu";
                    //console.log(arr, this.nowgamedata, this.gamedatas[this.nowgamedata].extra.id, this.gamedatas[this.nowgamedata]);
                    let d = new Date();
                    if (this.paipus != undefined) d.setTime(this.gamedatas[this.nowgamedata].endtime * 1000);
                    if (data.byteLength > 250){
                        //太短是因为牌谱未被归档导致返回错误信息
                        let pdata = Protobuf2Object(data);
                        this.pdatarecords = pdata.records;
                        //for (let i in pdata.records) View.IAction(pdata.records[i]);
                        this.analyzenextpart();
                        if (this.paipus != undefined){
                            this.onepaipu.record = JSON.parse(JSON.stringify(View.gamerecord));
                            this.paipus.push(JSON.parse(JSON.stringify(this.onepaipu)));
                        }
                        console.log('paipu #' + this.nowgamedata + ' complete. ' + d.toString());
                        if (this.rawdata == undefined){
                            console.log("but it's an unsupported paipu, skip it for now.");
                        }
                    }
                    else{
                        this.rawdata = undefined;
                        console.log('paipu #' + this.nowgamedata + ' download failed. ' + d.toString());
                    }
                }
                catch (err){
                    if (this.onlydownload){
                        console.log(err);
                    }
                    else {
                        if (this.ipcrsend != undefined && this.ipcrsend && ipcr != undefined){
                            let data = {
                                position: 'SimpleMahjong-paipu-majsoulpaipuanalyzer',
                                message: '牌谱分析错误',
                                gamedata: this.gamedatas[this.nowgamedata]
                            };
                            ipcr.send('reporterror', data);
                        }
                        console.error(err);
                    }
                    this.onepaipu = null;
                }
                this.nowgamedata ++ ;
                if (this.paipus != undefined) this.converttoonepaipu();
            }
            if (!bytearr){
                let reader = new FileReader();
                reader.readAsArrayBuffer(request.response);
                reader.onload = rloadfunc.bind(this);
            }
            else{
                rloadfunc.bind(this)(bytearr);
            }
        }
        if (filelink){
            request = new XMLHttpRequest();
            request.open('GET', filelink, true);
            request.responseType = 'blob';
            request.onload = loadfunc.bind(this);
            request.send();
        }
        else{
            if (!bytearr) console.error('filelink and bytearr all null');
            else{
                loadfunc.bind(this)(null, bytearr);
            }
        }
    }

    converttoonepaipu(bytearr = null){
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
        /*
        if (gamedata.roomdata.player == 3){
            console.log('paipu #' + this.nowgamedata + ' is 3 player mahjong, skip');
            this.nowgamedata ++ ;
            this.converttoonepaipu();
            return;
        }
        */
        let linkprefix = this.linkprefix;
        let link = linkprefix + gamedata.uuid;
        this.onepaipu = {
            gamedata: JSON.parse(JSON.stringify(gamedata))
        };
        UserID = gamedata.accountid;
        InitialOya = 0;
        View.INewGame({startscore: gamedata.roomdata.init_point});
        this.getfile(bytearr ? null : link, bytearr);
    }

    protobufinit(){
        if (this.protobufready) return;
        let request = new XMLHttpRequest();
        request.open('GET', 'https://t.zyr17.cn/static/majsoulpaipu/liqi.json');
        function loadfunc() { 
            let liqi = JSON.parse(request.response);
            AnalyzeInit(liqi);
            this.protobufready = true;
        }
        request.onload = loadfunc.bind(this);
        request.send();
    }

    convertsomepaipu(gamedatas, linkprefix = 'https://mj-srv-3.majsoul.com:7343/majsoul/game_record/'){
        if (linkprefix == 't')
            linkprefix = 'https://t.zyr17.cn/static/majsoulpaipu/';
        this.linkprefix = linkprefix;
        this.paipus = [];
        this.gamedatas = gamedatas;
        this.nowgamedata = 0;
        this.converttoonepaipu();
    }

    convertpaipuwithbytes(gamedata, bytearr){
        this.paipus = [];
        this.gamedatas = [gamedata];
        this.nowgamedata = 0;
        this.converttoonepaipu(bytearr);
    }

    getgamedatafromlink(filelink, startnum = 0, endnum = 10, linkprefix = 'https://mj-srv-3.majsoul.com:7343/majsoul/game_record/'){
        this.protobufinit();
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
            this.convertsomepaipu(gamedatas, linkprefix);
        }
        request.onload = loadfunc.bind(this);
        let protobufafterid = setInterval(() => {
            if (!this.protobufready) return;
            clearInterval(protobufafterid);
            request.send();
        }, 200);
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
        var func = View['I' + data.Itype];
        if (func != undefined){
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
            tsumokiri: str[4] == '1',
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
        };
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
        this.sendmessage({IType: 'NewGame', startscore: paipu.gamedata.init_point});
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