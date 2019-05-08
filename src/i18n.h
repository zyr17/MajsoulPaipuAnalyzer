#ifndef _I18N_H
#define _I18N_H

#include "header.h"
#include "algo.h"

namespace I18N{
    void I18NInit(const std::string &language);
    //查找本地化单词。如果Word2没有完全匹配则可能是扩展型，尝试删去末尾所有数字并换成!匹配。
    std::string get(const std::string &word1, const std::string &word2);
}

#endif