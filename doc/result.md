#分析结果说明

对于每项分析结果的意义如下

    #1: 一位率
    #2: 二位率
    #3: 三位率
    #4: 四位率
    AL keep #1: 进入AL时为一位，并且保持一位结束[5][10]
    AL reach #1: 进入AL时不是一位，最后成功逆一
    AL prevent from #4: 进入AL是四位，最后成功避四
    AL more than 2 times: AL持续了超过1局
    hule rate: 和了率
    tsumo rate: 和了时自摸率
    fangchong rate: 铳率[1]
    reach rate: 立直率
    fulu rate: 副露率[6]
    dama hule rate: 默听和了占和了比率
    fangchong dama rate: 别人默听然后铳了别人占放铳的比率
    fulu hule rate: 和了时有副露率[9]
    fulu fangchong rate: 放铳时有副露率
    hule point: 和了点数[3]
    fangchong point: 放铳失点
    hule sudian: 和了素点[4]
    fangchong sudian: 放铳素点
    dama hule point: 默听和了点数[7][8]
    fangchong dama point: 别人默听铳了失点
    hule 3900+ rate: 和了3900+ 但是不到7700+的比例
    hule 7700+ rate: 和了7700+ 但是不到11600+的比例
    hule 11600+ rate: 和了11600+的比例
    fangchong 3900+ rate: 铳了3900+ 但是不到7700+的比例[2]
    fangchong 7700+ rate: 铳了7700+ 但是不到11600+的比例
    fangchong 11600+ rate: 铳了11600+的比例
    hule circle: 和了巡数
    fangchong my circle: 铳了的时候自己的巡数
    fangchong his circle: 铳了的时候别人的巡数
    fangchong shanten: 铳了的时候自己手牌的向听数。[11]
    
#统计方式、名称意义、特殊情况的说明

[1] 双响在铳了局数中算铳了两局；三响如果流局了不算铳（雀魂不存在）
[2] 双响铳了5200+7700，不把铳点相加仅分开计数
[3] 和了收入指包含本场供托；包括自己放上去的自己的立直棒
[4] 素点即扣了本场和供托
[5] AL相关以刚进入AL时的数据为基准；北起AL4位连庄+西入飞人是避四+逆一
[6] 统计时副露数相关均分为4档分别记录，分析时为了简便将结果累加，详情目前可参考内置统计结果
[7] 默听指门清未立听牌
[8] 暗杠算副露，不算门清；因此会形成立直+副露
[9] 副露和了指和了的局数中副露了的局数占比；非与总副露局数比
[10] 中途飞人视为AL位序不变
[11] 还未实现，目前恒为3