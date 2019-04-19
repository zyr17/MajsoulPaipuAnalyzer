#ifndef _I18N_H
#define _I18N_H

#include "header.h"
#include "algo.h"

namespace I18N{
    void I18NInit(const std::string &language);
    std::string get(const std::string &word1, const std::string &word2);
}

#endif