(function () {

var electron = require("electron");
var ipcr = electron.ipcRenderer;

const NAMEPREF   = 0;     //2 for english, 1 for sane amount of weeb, 0 for japanese
const VERBOSELOG = false; //dump mjs records to output - will make the file too large for tenhou.net/5 viewer
const SHOWFU     = false; //always show fu/han for scoring - even for limit hands

//words that can end up in log, some are mandatory kanji in places
const JPNAME = 0;
const RONAME = 1;
const ENNAME = 2;
const RUNES  = {
    /*hand limits*/
    "mangan"         : ["満貫",         "Mangan ",         "Mangan "               ],
    "haneman"        : ["跳満",         "Haneman ",        "Haneman "              ],
    "baiman"         : ["倍満",         "Baiman ",         "Baiman "               ],
    "sanbaiman"      : ["三倍満",       "Sanbaiman ",      "Sanbaiman "            ],
    "yakuman"        : ["役満",         "Yakuman ",        "Yakuman "              ],
    "kazoeyakuman"   : ["数え役満",     "Kazoe Yakuman ",  "Counted Yakuman "      ],
    "kiriagemangan"  : ["切り上げ満貫", "Kiriage Mangan ", "Rounded Mangan "       ],
    /*round enders*/
    "agari"          : ["和了",         "Agari",           "Agari"                 ],
    "ryuukyoku"      : ["流局",         "Ryuukyoku",       "Exhaustive Draw"       ],
    "nagashimangan"  : ["流し満貫",     "Nagashi Mangan",  "Mangan at Draw"        ],
    "suukaikan"      : ["四開槓",       "Suukaikan",       "Four Kan Abortion"     ],
    "sanchahou"      : ["三家和",       "Sanchahou",       "Three Ron Abortion"    ],
    "kyuushukyuuhai" : ["九種九牌",     "Kyuushu Kyuuhai", "Nine Terminal Abortion"],
    "suufonrenda"    : ["四風連打",     "Suufon Renda",    "Four Wind Abortion"    ],
    "suuchariichi"   : ["四家立直",     "Suucha Riichi",   "Four Riichi Abortion"  ],
    /*scoring*/
    "fu"             : ["符",           /*"Fu",*/"符",     "Fu"                    ],
    "han"            : ["飜",           /*"Han",*/"飜",    "Han"                   ],
    "points"         : ["点",           /*"Points",*/"点", "Points"                ],
    "all"            : ["∀",            "∀",               "∀"                     ],
    "pao"            : ["包",           "pao",             "Responsibility"        ],
    /*rooms*/
    "tonpuu"         : ["東喰",         " East",           " East"                 ],
    "hanchan"        : ["南喰",         " South",          " South"                ],
    "friendly"       : ["友人戦",       "Friendly",        "Friendly"              ],
    "tournament"     : ["大会戦",       "Tounament",       "Tournament"            ],
    "sanma"          : ["三",           "3-Player ",       "3-Player "             ],
    "red"            : ["赤",           " Red",            " Red Fives"            ],
    "nored"          : ["",             " Aka Nashi",      " No Red Fives"         ]
};

//senkinin barai yaku - please don't change, yostar..
const DAISANGEN = 37; //daisangen cfg.fan.fan.map_ index
const DAISUUSHI = 50;

const TSUMOGIRI = 60; //tenhou tsumogiri symbol

//global variables - don't touch
let ALLOW_KIRIAGE = false; //potentially allow this to be true
let TSUMOLOSSOFF  = false; //sanma tsumo loss, is set true for sanma when tsumo loss off

//pad a to length l with f, needed to pad log for >sanma
const pad_right = (a, l, f) =>
    !Array.from({length: l - a.length})
    .map(_ => a.push(f)) || a;

//take '2m' and return 2 + 10 etc.
function tm2t(str)
{   //tenhou's tile encoding:
    //   11-19    - 1-9 man
    //   21-29    - 1-9 pin
    //   31-39    - 1-9 sou
    //   41-47    - ESWN WGR
    //   51,52,53 - aka 5 man, pin, sou
    let num = parseInt(str[0]);
    const tcon = { m : 1, p : 2, s : 3, z : 4 };

    return num ? 10 * tcon[str[1]] + num : 50 + tcon[str[1]];
}

//return normal tile from aka, tenhou rep
function deaka(til)
{   //alternativly - use strings
    if (5 == ~~(til/10))
        return 10*(til%10)+(~~(til/10));

    return til;
}

//return aka version of tile
function makeaka(til)
{
    if (5 == (til%10)) //is a five (or haku)
        return 10*(til%10)+(~~(til/10));
    return til; //can't be/already is aka
}

//round up to nearest hundred iff TSUMOLOSSOFF == true otherwise return 0
function tlround(x)
{
    return TSUMOLOSSOFF ? 100*Math.ceil(x/100) : 0;
}

//parse mjs hule into tenhou agari list
function parsehule(h, kyoku)
{   //tenhou log viewer requires 点, 飜) or 役満) to end strings, rest of scoring string is entirely optional
    //who won, points from (self if tsumo), who won or if pao: who's responsible
    let res    = [h.seat, h.zimo ? h.seat : kyoku.ldseat, h.seat];
    let delta  = []; //we need to compute the delta ourselves to handle double/triple ron
    let points = 0;
    let rp     = (-1 != kyoku.nriichi) ? 1000 * (kyoku.nriichi + kyoku.round[2]) : 0; //riichi stick points, -1 means already taken
    let hb     = 100 * kyoku.round[1]; //base honba payment

    //sekinin barai logic
    let pao         = false;
    let liableseat  = -1;
    let liablefor   = 0;

    if (h.yiman)
    {   //only worth checking yakuman hands
        h.fans.forEach(e =>
        {
            if (DAISUUSHI == e.id && (-1 != kyoku.paowind))
            {   //daisuushi pao
                pao        = true;
                liableseat = kyoku.paowind;
                liablefor += e.val; //realistically can only be liable once
            }
            else if (DAISANGEN == e.id && (-1 != kyoku.paodrag))
            {
                pao        = true;
                liableseat = kyoku.paodrag;
                liablefor += e.val;
            }
        });
    }

    if (h.zimo)
    {   //ko-oya payment for non-dealer tsumo
        //delta  = [...new Array(kyoku.nplayers)].map(()=> (-hb - h.point_zimo_xian));
        delta =  new Array(kyoku.nplayers).fill(-hb - h.point_zimo_xian - tlround((1/2) * (h.point_zimo_xian)))
        if (h.seat == kyoku.dealerseat) //oya tsumo
        {
            delta[h.seat] = rp + (kyoku.nplayers - 1) * (hb + h.point_zimo_xian) + 2 * tlround((1/2) * (h.point_zimo_xian));
            points = h.point_zimo_xian + tlround((1/2) * (h.point_zimo_xian));
        }
        else  //ko tsumo
        {
            delta[h.seat]       = rp + hb + h.point_zimo_qin + (kyoku.nplayers - 2) * (hb + h.point_zimo_xian) + 2 * tlround((1/2) * (h.point_zimo_xian));
            delta[kyoku.dealerseat] = -hb - h.point_zimo_qin - tlround((1/2) * (h.point_zimo_xian));
            points = h.point_zimo_xian + "-" + h.point_zimo_qin;
        }
    }
    else
    {   //ron
        delta =  new Array(kyoku.nplayers).fill(0.)
        delta[h.seat]       = rp + (kyoku.nplayers - 1) * hb + h.point_rong;
        delta[kyoku.ldseat] = -(kyoku.nplayers - 1) * hb - h.point_rong;
        points = h.point_rong;
        kyoku.nriichi = -1; //mark the sticks as taken, in case of double ron
    }

    //sekinin barai payments
    //    treat pao as the liable player paying back the other players - safe for multiple yakuman
    const OYA    = 0;
    const KO     = 1;
    const RON    = 2;
    const YSCORE = [ //yakuman scoring table
        //oya,    ko,   ron  pays
        [0,    16000, 48000], //oya wins
        [16000, 8000, 32000]  //ko  wins
    ];

    if (pao)
    {
        res[2] = liableseat; //this is how tenhou does it - doesn't really seem to matter to akochan or tenhou.net/5

        if (h.zimo) //liable player needs to payback n yakuman tsumo payments
        {
            if (h.qinjia) //dealer tsumo
            {   //should treat tsumo loss as ron, luckily all yakuman values round safely for north bisection
                delta[liableseat] -= 2 * hb + liablefor * 2 * YSCORE[OYA][KO] + tlround((1/2) * liablefor *  YSCORE[OYA][KO]); // 1? only paying back other ko
                delta.forEach((e, i) =>
                {
                    if (liableseat != i && h.seat != i &&  kyoku.nplayers >= i)
                        delta[i] += hb + liablefor * YSCORE[OYA][KO] + tlround((1/2) * liablefor * (YSCORE[OYA][KO]));
                });
                if (3 == kyoku.nplayers) //dealer should get north's payment from liable
                    delta[h.seat] += (TSUMOLOSSOFF ? 0 : liablefor * YSCORE[OYA][KO]);
            }
            else  //non-dealer tsumo
            {
                delta[liableseat] -= (kyoku.nplayers - 2) * hb + liablefor * (YSCORE[KO][OYA] + YSCORE[KO][KO]) + tlround((1/2) * liablefor *  YSCORE[KO][KO]); //^^same 1st, but ko
                delta.forEach((e, i) =>
                {
                    if (liableseat != i && h.seat != i &&  kyoku.nplayers >= i)
                    {
                        if (kyoku.dealerseat == i)
                            delta[i] += hb + liablefor * YSCORE[KO][OYA] + tlround((1/2) * liablefor *  YSCORE[KO][KO]); //^^same 1st ...
                        else
                            delta[i] += hb + liablefor * YSCORE[KO][KO] + tlround((1/2) * liablefor *  YSCORE[KO][KO]); //^^same 1st ...
                    }
                });
            }
        }
        else      //ron
        {
            //liable seat pays the deal-in seat 1/2 yakuman + full honba
            delta[liableseat]   -= (kyoku.nplayers - 1) * hb + (1/2) * liablefor * YSCORE[h.qinjia ? OYA : KO][RON];
            delta[kyoku.ldseat] += (kyoku.nplayers - 1) * hb + (1/2) * liablefor * YSCORE[h.qinjia ? OYA : KO][RON];
        }
    } //if pao
    //append point symbol
    points += RUNES.points[JPNAME] + ((h.zimo && h.qinjia) ? RUNES.all[NAMEPREF]: "");

    //score string
    let fuhan = h.fu + RUNES.fu[NAMEPREF] + h.count + RUNES.han[NAMEPREF];
    if (h.yiman) //yakuman
        res.push((SHOWFU ? fuhan : "") + RUNES.yakuman[NAMEPREF] + points);
    else if (13 <= h.count) //kazoe
        res.push((SHOWFU ? fuhan : "") + RUNES.kazoeyakuman[NAMEPREF] + points);
    else if (11 <= h.count) //sanbaiman
        res.push((SHOWFU ? fuhan : "") + RUNES.sanbaiman[NAMEPREF] + points);
    else if (8 <= h.count) //baiman
        res.push((SHOWFU ? fuhan : "") + RUNES.baiman[NAMEPREF] + points);
    else if (6 <= h.count) //haneman
        res.push((SHOWFU ? fuhan : "") + RUNES.haneman[NAMEPREF] + points);
    else if (5 <= h.count || (4 <= h.count && 40 <= h.fu) || (3 <= h.count && 70 <= h.fu)) //mangan
        res.push((SHOWFU ? fuhan : "") + RUNES.mangan[NAMEPREF] + points);
    else if (ALLOW_KIRIAGE && ((4 == h.count && 30 == h.fu) || (3 == h.count && 60 == h.fu))) //kiriage
        res.push((SHOWFU ? fuhan : "") + RUNES.kiriagemangan[NAMEPREF] + points);
    else //ordinary hand
        res.push(fuhan + points);

    h.fans.forEach(e => {
        // old paipus have no id, just chs name in it.
        let id = undefined;
        if (e.id === undefined || e.id === 0) {
            for (let i in cfg.fan.fan.map_)
                if (cfg.fan.fan.map_[i].name_chs === e.name) {
                    id = i;
                    break;
                }
        }
        else id = e.id;
        res.push(
            (JPNAME == NAMEPREF ? cfg.fan.fan.map_[id].name_jp : cfg.fan.fan.map_[id].name_en)
            + "(" + (h.yiman ? (RUNES.yakuman[JPNAME]) : (e.val + RUNES.han[JPNAME]) )+ ")"
        )
    });

    return [pad_right(delta, 4, 0.), res];
}

//round information, to be reset every RecordNewRound
let kyoku = [];
kyoku.init = function(leaf)
{                      //[kyoku, honba, riichi sticks] - NOTE: 4 mult. works for sanma
    this.nplayers    = leaf.scores.length;
    this.round       = [4 * leaf.chang + leaf.ju, leaf.ben, leaf.liqibang];
    this.initscores  = leaf.scores; pad_right(this.initscores, 4, 0);
    this.doras       = leaf.dora ? [tm2t(leaf.dora)] : leaf.doras.map(e => tm2t(e));
    this.draws       = [[],[],[],[]];
    this.discards    = [[],[],[],[]];
    this.haipais     = this.draws.map( (_, i) => leaf["tiles" + i].map( f => tm2t(f)));

    //treat the last tile in the dealer's hand as a drawn tile
    this.poppedtile  = this.haipais[leaf.ju].pop();
    this.draws[leaf.ju].push(this.poppedtile);
    //information we need, but can't expect in every record
    this.dealerseat  = leaf.ju;
    this.ldseat      = -1; //who dealt the last tile
    this.nriichi     = 0; //number of current riichis - needed for scores, abort workaround
    this.nkan        = 0; //number of current kans - only for abort workaround
    //pao rule
    this.nowinds     = new Array(4).fill(0);//counter for each players open wind pons/kans
    this.nodrags     = new Array(4).fill(0);
    this.paowind     = -1; //seat of who dealt the final wind, -1 if no one is responsible
    this.paodrag     = -1;

    return this;
};

//dump round informaion
kyoku.dump = function(uras)
{   //NOTE: doras,uras are the indicators
    let entry = [];
    entry.push(kyoku.round);
    entry.push(kyoku.initscores);
    entry.push(kyoku.doras);
    entry.push(uras);
    kyoku.haipais.forEach((f,i) =>
    {
        entry.push(f);
        entry.push(kyoku.draws[i]);
        entry.push(kyoku.discards[i]);
    });

    return entry;
}

//sekinin barai tiles
const WINDS = ["1z", "2z", "3z", "4z"].map(e => tm2t(e));
const DRAGS = ["5z", "6z", "7z", "0z"].map(e => tm2t(e)); //0z would be aka haku

//senkinin barai incrementer - to be called every pon, daiminkan, ankan
kyoku.countpao = function(tile, owner, feeder)
{   //owner and feeder are seats, tile should be tenhou
    if (WINDS.includes(tile))
    {
        if(4 == ++this.nowinds[owner])
            this.paowind = feeder;
    }
    else if (DRAGS.includes(tile))
    {
        if(3 == ++this.nodrags[owner])
            this.paodrag = feeder;
    }

    return;
}

//seat1 is seat0's x
function relativeseating(seat0, seat1)
{   //0: kamicha, 1: toimen, 2: if shimocha
    return (seat0 - seat1 + 4 - 1) % 4;
}

//convert mjs records to tenhou log
function generatelog(mjslog)
{
    let log = [];
    mjslog.forEach((e, leafidx) =>
    {
        switch (e.constructor.name)
        {
            case "RecordNewRound":
            {   //new round
                kyoku.init(e);
                return;
            }
            case "RecordDiscardTile":
            {   //discard - marking tsumogiri and riichi
                let symbol = e.moqie ? TSUMOGIRI : tm2t(e.tile);

                //we pretend that the dealer's initial 14th tile is drawn - so we need to manually check the first discard
                if (e.seat == kyoku.dealerseat
                    && !kyoku.discards[e.seat].length && symbol == kyoku.poppedtile)
                    symbol = TSUMOGIRI;

                if (e.is_liqi) //riichi delcaration
                {
                    kyoku.nriichi++;
                    symbol = "r" + symbol;
                }
                kyoku.discards[e.seat].push(symbol);
                kyoku.ldseat = e.seat; //for ron, pon etc.

                //sometimes we get dora passed here
                if (e.doras && e.doras.length > kyoku.doras.length)
                    kyoku.doras = e.doras.map(f => tm2t(f));

                return;
            }
            case "RecordDealTile":
            {   //draw - after kan this gets passed the new dora
                if (e.doras && e.doras.length > kyoku.doras.length)
                    kyoku.doras = e.doras.map(f => tm2t(f));

                kyoku.draws[e.seat].push(tm2t(e.tile));

                return;
            }
            case "RecordChiPengGang":
            {   //call - chi, pon, daiminkan
                switch (e.type)
                {
                    case 0:
                    {   //chii
                        kyoku.draws[e.seat].push(
                            "c" +
                            tm2t(e.tiles[2]) +
                            tm2t(e.tiles[0]) +
                            tm2t(e.tiles[1])
                        );

                        return;
                    }
                    case 1:
                    {   //pon
                        let worktiles = e.tiles.map(f => tm2t(f));
                        let idx = relativeseating(e.seat, kyoku.ldseat);
                        kyoku.countpao(worktiles[0], e.seat, kyoku.ldseat);
                        //pop the called tile a preprend 'p'
                        worktiles.splice(idx, 0, "p" + worktiles.pop());
                        kyoku.draws[e.seat].push(worktiles.join(""));

                        return;
                    }
                    case 2:
                    {   ///////////////////////////////////////////////////
                        // kan naki:
                        //   daiminkan:
                        //     kamicha   "m39393939" (0)
                        //     toimen    "39m393939" (1)
                        //     shimocha  "222222m22" (3)
                        //     (writes to draws; 0 to discards)
                        //   shouminkan: (same order as pon; immediate tile after k is the added tile)
                        //     kamicha   "k37373737" (0)
                        //     toimen    "31k313131" (1)
                        //     shimocha  "3737k3737" (2)
                        //     (writes to discards)
                        //   ankan:
                        //     "121212a12" (3)
                        //     (writes to discards)
                        ///////////////////////////////////////////////////
                        //daiminkan
                        let calltiles = e.tiles.map(f => tm2t(f));
                        // < kamicha 0 | toimen 1 | shimocha 3 >
                        let idx = relativeseating(e.seat, kyoku.ldseat);

                        kyoku.countpao(calltiles[0], e.seat, kyoku.ldseat);
                        calltiles.splice( 2 == idx ? 3 : idx, 0, "m" + calltiles.pop());
                        kyoku.draws[e.seat].push(calltiles.join(""));
                        //tenhou drops a 0 in discards for this
                        kyoku.discards[e.seat].push(0);
                        //register kan
                        kyoku.nkan++;

                        return;
                    }
                    default:
                        console.log(
                            "didn't know what to do with " +
                            e.constructor.name + "(" + leafidx + ")"
                        );

                    return;
                }
            }
            case "RecordAnGangAddGang" :
            {   //kan - shouminkan 'k', ankan 'a'
                //NOTE: e.tiles here is a single tile; naki is placed in discards
                let til =  tm2t(e.tiles);
                kyoku.ldseat = e.seat; // for chankan, no conflict as last discard has passed
                switch (e.type)
                {
                    case 3:
                    {   //ankan
                        ////////////////////
                        // mjs chun ankan example record:
                        //{"seat":0,"type":3,"tiles":"7z"}
                        ////////////////////
                        kyoku.countpao(til, e.seat, -1); //count the group as visible, but don't set pao
                        //get the tiles from haipai and draws that
                        //are involved in ankan, dumb
                        //because n aka might be involved
                        let ankantiles = kyoku.haipais[e.seat].filter(t => (deaka(t) == deaka(til) ? true : false))
                            .concat(kyoku.draws[e.seat].filter(t => (deaka(t) == deaka(til) ? true : false)) );
                        til = ankantiles.pop(); //doesn't really matter which tile we mark ankan with - chosing last drawn
                        kyoku.discards[e.seat].push(ankantiles.join("") + "a" + til); //push naki
                        kyoku.nkan++;

                        return;
                    }
                    case 2:
                    {   //shouminkan
                        //get pon naki from .draws and swap in new symbol
                        let nakis = kyoku.draws[e.seat].filter(w =>
                        {
                            if ('string' === typeof w) //naki
                                return w.includes("p" + deaka(til)) || w.includes("p" + makeaka(til)); //pon involves same tile type
                            else
                                return false;
                        });

                        kyoku.discards[e.seat].push(nakis[0].replace(/p/, "k" + til)); //push naki
                        kyoku.nkan++;

                        return;
                    }
                    default:
                    {
                        console.log("didn't know what to do with "
                            + e.constructor.name + " type: " + e.type);

                        return;
                    }
                }

                return;
            }
            case "RecordBaBei" :
            {   //kita - this record (only) gives {seat, moqie}
                //NOTE: tenhou doesn't mark its kita based on when they were drawn, so we won't
                //if (e.moqie)
                //    kyoku.discards[e.seat].push("f" + TSUMOGIRI);
                //else
                kyoku.discards[e.seat].push("f44");

                return;
            }
            /////////////////////////////////////////////////////
            // round enders:
            // "RecordNoTile" - ryuukyoku
            // "RecordHule"   - agari - ron/tsumo
            // "RecordLiuJu"  - abortion
            //////////////////////////////////////////////////////
            case "RecordLiuJu" :
            {   //abortion
                let entry = kyoku.dump([]);

                if (1 == e.type)
                    entry.push([RUNES.kyuushukyuuhai[NAMEPREF]]); //kyuushukyuhai
                else if (2 == e.type)
                    entry.push([RUNES.suufonrenda[NAMEPREF]]); //suufon renda
                else if (4 == kyoku.nriichi) //TODO: actually get the type code
                    entry.push([RUNES.suuchariichi[NAMEPREF]]); //4 riichi
                else if (4 <= kyoku.nkan) //TODO: actually get type code
                    entry.push([RUNES.suukaikan[NAMEPREF]]); //4 kan, potentially false positive on 3 ron with 4 kans
                else
                    entry.push([RUNES.sanchahou[NAMEPREF]]); //3 ron - can't actually get this in mjs

                log.push(entry);

                return;
            }
            case "RecordNoTile" :
            {   //ryuukyoku
                let entry = kyoku.dump([]);
                let delta = new Array(4).fill(0.);

                //NOTE: mjs wll not give delta_scores if everyone is (no)ten - TODO: minimize the autism
                if (e.scores && e.scores[0] && e.scores[0].delta_scores && e.scores[0].delta_scores.length)
                    e.scores.forEach(f => f.delta_scores.forEach((g, i) => delta[i] += g)); //for the rare case of multiple nagashi, we sum the arrays

                if (e.liujumanguan) //nagashi mangan
                    entry.push([RUNES.nagashimangan[NAMEPREF], delta])
                else    //normal ryuukyoku
                    entry.push([RUNES.ryuukyoku[NAMEPREF], delta]);
                log.push(entry);

                return;
            }
            case "RecordHule":
            {   //agari
                let agari = [];
                let ura = [];
                e.hules.forEach( f =>
                {
                    if (ura.length < (f.li_doras ? f.li_doras.length : 0)) //take the longest ura list - double ron with riichi + dama
                        ura = f.li_doras.map(g => tm2t(g));
                    agari.push(parsehule(f, kyoku));
                });
                let entry = kyoku.dump(ura);

                entry.push( [RUNES.agari[JPNAME]].concat(agari.flat()) ); //needs the japanese agari
                log.push(entry);

                return;
            }
            default:
                console.log(
                    "didn't know what to do with " + e.constructor.name + "(" + leafidx + ")"
                );

            return;
        }
    });

    return log;
}

//this is the json struct that we write to file
function parse(record)
{
    let res        = {};
    let ruledisp   = "";
    let lobby      = ""; //usually 0, is the custom lobby number
    let nplayers   = record.head.result.players.length;
    let nakas      = nplayers - 1; //default
    // anon edit 1 start
    var mjslog = [];
    var msg = net.MessageWrapper.decodeMessage(record.data);
    if (msg.actions.length !== 0) {
        msg.actions.forEach(e => {if(e.result.length!==0)mjslog.push(net.MessageWrapper.decodeMessage(e.result))});
    } else {
        msg.records.forEach(e => {if(e.length!==0)mjslog.push(net.MessageWrapper.decodeMessage(e))});
    }
    // anon edit 1 end

    res["ver"]     = "2.3"; // mlog version number
    res["ref"]     = record.head.uuid; // game id - copy and paste into "other" on the log page to view
    res["log"]     = generatelog(mjslog);
    //PF4 is yonma, PF3 is sanma
    res["ratingc"] = "PF" + nplayers;

    //rule display
    if (3 == nplayers && JPNAME == NAMEPREF)
        ruledisp += RUNES.sanma[JPNAME];
    if (record.head.config.meta.mode_id) //ranked or casual
        ruledisp += (JPNAME == NAMEPREF) ?
            cfg.desktop.matchmode.map_[record.head.config.meta.mode_id].room_name_jp
            : cfg.desktop.matchmode.map_[record.head.config.meta.mode_id].room_name_en;
    else if (record.head.config.meta.room_id) //friendly
    {
        lobby    = ": " + record.head.config.meta.room_id; //can set room number as lobby number
        ruledisp += RUNES.friendly[NAMEPREF]; //"Friendly";
        nakas    = record.head.config.mode.detail_rule.dora_count; // TODO: for very old paipus, no detail_rule. currently raise exception. 
        TSUMOLOSSOFF = (3 == nplayers) ? ! record.head.config.mode.detail_rule.have_zimosun : false;
    }
    else if (record.head.config.meta.contest_uid) //tourney
    {
        lobby    = ": " + record.head.config.meta.contest_uid;
        ruledisp += RUNES.tournament[NAMEPREF]; //"Tournament";
        nakas    = record.head.config.mode.detail_rule.dora_count;
        TSUMOLOSSOFF = (3 == nplayers) ? ! record.head.config.mode.detail_rule.have_zimosun : false;
    }
    if (1 == record.head.config.mode.mode)
    {
        ruledisp += RUNES.tonpuu[NAMEPREF]; //" East";
    }
    else if (2 == record.head.config.mode.mode)
    {
        ruledisp += RUNES.hanchan[NAMEPREF]; //" South";
    }
    if (! record.head.config.meta.mode_id && ! record.head.config.mode.detail_rule.dora_count)
    {
        if (JPNAME != NAMEPREF)
            ruledisp += RUNES.nored[NAMEPREF];
        res["rule"] = {"disp" : ruledisp, "aka53" : 0, "aka52" : 0, "aka51": 0};
    }
    else
    {
        if (JPNAME == NAMEPREF)
            ruledisp += RUNES.red[JPNAME];
        res["rule"] = {"disp" : ruledisp, "aka53" : 1, "aka52" : (4 == nakas ? 2 : 1), "aka51": (4 == nplayers ?  1 : 0)};
    }

    res["lobby"] = 0; //tenhou custom lobby - could be tourney id or friendly room for mjs. appending to title instead to avoid 3->C etc. in tenhou.net/5
    // autism to fix logs with AI
    // ranks
    res["dan"]  = new Array(4).fill('');
    record.head.accounts.forEach(e =>
        res["dan"][e.seat] = (JPNAME == NAMEPREF) ?
            cfg.level_definition.level_definition.map_[e.level.id].full_name_jp
            : cfg.level_definition.level_definition.map_[e.level.id].full_name_en
    );
    // level score, no real analog to rate
    res["rate"] = new Array(4).fill('');
    record.head.accounts.forEach(e => res["rate"][e.seat] = e.level.score); //level score, closest thing to rate
    // sex
    res["sx"]  = new Array(4).fill('C')
    record.head.accounts.forEach(e => {
        let charid = undefined;
        if (e.character === undefined) // old paipus use avatar_id to represent character
            charid = e.avatar_id;
        else charid = e.character.charid;
        let sex = cfg.item_definition.character.map_[charid].sex;
        res["sx"][e.seat] = ( 1 == sex ) ? "F" : (2 == sex ? "M" : "C");
    });
    // >names
    res["name"] = new Array(4).fill('AI');
    record.head.accounts.forEach(e => res["name"][e.seat] = e.nickname);
    // clean up for sanma AI
    if (3 == nplayers)
    {
        res["name"][3] = "";
        res["sx"][3]   = "";
    }
    // scores
    let scores   = record.head.result.players
        .map(e => [e.seat, e.part_point_1, e.total_point / 1000]);
    res["sc"]    = new Array(8).fill(0);
    scores.forEach((e, i) => {res["sc"][2 * e[0]] = e[1]; res["sc"][2 * e[0] + 1] = e[2];});
    //optional title - why not give the room and put the timestamp here; 1000 for unix to .js timestamp convention
    res["title"] = [ ruledisp + lobby,
    (new Date(record.head.end_time * 1000)).toLocaleString()
    ];
    //optionally dump mjs records NOTE: this will likely make the file too large for tenhou.net/5 viewer
    if (VERBOSELOG)
    {
        res["mjshead"]        = record.head;
        res["mjslog"]         = mjslog;
        res["mjsrecordtypes"] = mjslog.map(e => e.constructor.name);
    }

    return res;
}

ipcr.on('converttenhoulog', (event, gamedata, paipubytes) => {
    console.log('converttenhoulog', gamedata)
    let record = {
        head: gamedata,
        data: paipubytes
    };
    let res = undefined;
    try {
        res = parse(record);
    }
    finally {
        console.log(gamedata, res);
        ipcr.send('converttenhoulogcallback', gamedata.uuid, res);
    }
});

})()
