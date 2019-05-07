"use strict"
const num2ltrb = ['bottom', 'right', 'top', 'left'];
const ltrb2num = { 'bottom': 0, 'right': 1, 'top': 2, 'left': 3 };
var tile2num = {
    '1m': 0, '2m': 1, '3m': 2, '4m': 3, '5m': 4, '0m': 5, '6m': 6, '7m': 7, '8m': 8, '9m': 9,
    '1p': 10, '2p': 11, '3p': 12, '4p': 13, '5p': 14, '0p': 15, '6p': 16, '7p': 17, '8p': 18, '9p': 19,
    '1s': 20, '2s': 21, '3s': 22, '4s': 23, '5s': 24, '0s': 25, '6s': 26, '7s': 27, '8s': 28, '9s': 29,
    '1z': 30, '2z': 31, '3z': 32, '4z': 33, '5z': 34, '6z': 35, '7z': 36, '?x': 37
};
const num2tile = [
    '1m', '2m', '3m', '4m', '5m', '0m', '6m', '7m', '8m', '9m', 
    '1p', '2p', '3p', '4p', '5p', '0p', '6p', '7p', '8p', '9p', 
    '1s', '2s', '3s', '4s', '5s', '0s', '6s', '7s', '8s', '9s', 
    '1z', '2z', '3z', '4z', '5z', '6z', '7z', '?x'
];
const mat2num = [
    [0, 1, 2, 3, 4, 6, 7, 8, 9],
    [10, 11, 12, 13, 14, 16, 17, 18, 19],
    [20, 21, 22, 23, 24, 26, 27, 28, 29],
    [30, 31, 32, 33, 34, 35, 36]
];
const num2maty = [0, 1, 2, 3, 4, 4, 5, 6, 7, 8];
const nexttile = [
    1, 2, 3, 4, 6, 6, 7, 8, 9, 0, 
    11, 12, 13, 14, 16, 16, 17, 18, 19, 10,
    21, 22, 23, 24, 26, 26, 27, 28, 29, 20,
    31, 32, 33, 30, 35, 36, 34               
];
const num2naki = ['chi', 'pon', 'kan', 'ankan', 'kakan'];
const naki2num = { 'chi': 0, 'pon': 1, 'kan': 2, 'ankan': 3, 'kakan': 4 };
const fonname = ['東', '南', '西', '北'];
const suji = ['零', '一', '二', '三', '四'];
const btnname = ['流局', '拔北', '吃', '碰', '杠', '加杠', '暗杠', '立直', '荣', '自摸'];
const hanname = [
    ['门前清自摸和', 'tsumo'], ['立直', 'reach'], ['一发', 'yipatsu'], ['枪杠', 'chankan'], 
    ['岭上开花', 'rinshan'], ['海底摸月', 'haitei'], ['河底捞鱼', 'houtei'], ['平和', 'pinfu'], 
    ['断幺九', 'tanyao'], ['一杯口', 'yipeikou'], ['自风东', 'jifudon'], ['自风南', 'jifunan'], 
    ['自风西', 'jifusha'], ['自风北', 'jifupei'], ['场风东', 'bafudon'], ['场风南', 'bafunan'], 
    ['场风西', 'bafusha'], ['场风北', 'bafupei'], ['役牌 白', 'haku'], ['役牌 发', 'hatsu'], 
    ['役牌 中', 'chun'], ['两立直', 'wreach'], ['七对子', 'chitoitsu'], ['混全带幺九', 'chanta'], 
    ['一气通贯', 'yitsu'], ['三色同顺', 'sanshoku'], ['三色同刻', 'sanshokudouko'], ['三杠子', 'sankantsu'], 
    ['对对和', 'toitoi'], ['三暗刻', 'sananko'], ['小三元', 'shousangen'], ['混老头', 'honroutou'], 
    ['二杯口', 'ryanpeikou'], ['纯全带幺九', 'junchan'], ['混一色', 'honyitsu'], ['清一色', 'chinyitsu'], 
    ['人和', 'renhou'], ['天和', 'tenhou'], ['地和', 'chihou'], ['大三元', 'daisangen'], 
    ['四暗刻', 'suanko'], ['四暗刻单骑', 'suankotanki'], ['字一色', 'tsuyisou'], ['绿一色', 'ryouyisou'], 
    ['清老头', 'chinroutou'], ['九莲宝灯', 'churen'], ['纯正九莲宝灯', 'junseichuren'], ['国士无双', 'kokushi'], 
    ['国士无双十三面', 'kokushijusan'], ['大四喜', 'daisushi'], ['小四喜', 'shousushi'], ['四杠子', 'sukantsu'], 
    ['宝牌', 'dora'], ['里宝牌', 'ura'], ['红宝牌', 'aka'], 
];
const majsoulfanid2name = [
    '',
    '门前清自摸和','立直','枪杠','岭上开花','海底摸月',
    '河底捞鱼','役牌 白','役牌 发','役牌 中','役牌:门风牌',
    '役牌:场风牌','断幺九','一杯口','平和','混全带幺九',
    '一气通贯','三色同顺','两立直','三色同刻','三杠子',
    '对对和','三暗刻','小三元','混老头','七对子',
    '纯全带幺九','混一色','二杯口','清一色','一发',
    '宝牌','红宝牌','里宝牌','拔北宝牌','天和',
    '地和','大三元','四暗刻','字一色','绿一色',
    '清老头','国士无双','小四喜','四杠子','九莲宝灯',
    '八连庄','纯正九莲宝灯','四暗刻单骑','国士无双十三面','大四喜'
];
function emptymatchdata(){
    return {
        'title': '',
        'kyoutaku': 0,
        'honba': 0,
        'data': [{ hand: [], get: '', show: [], table: [], score: 0, reach: 0},
            { hand: [], get: '', show: [], table: [], score: 0, reach: 0},
            { hand: [], get: '', show: [], table: [], score: 0, reach: 0},
            { hand: [], get: '', show: [], table: [], score: 0, reach: 0}
        ],
        'now': 0,
        'east': 0,
        'remain': 0,
        'nowround': 0,
        'dora': ''
    };
}