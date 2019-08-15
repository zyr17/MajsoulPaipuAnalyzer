#ifndef _OUTPUT_H
#define _OUTPUT_H

#include "header.h"

namespace I18N{
    //TODO:重整代码，避免恶心的include和声明
    std::string get(const std::string &word1, const std::string &word2);
}

namespace Out{
    const std::string htmlname = "PaipuAnalyzeResult.html";
    class MyCout{
    private:
        std::string buffer;
        char cc[400];
        unsigned long long maxlength = OUTBUFFERSIZE, floatnum = FLOATAFTERPOINTNUM;
        void output();

    public:
        MyCout();
        ~MyCout();
        MyCout& operator<< (const std::string &k);
        MyCout& operator<< (const char* k);
        MyCout& operator<< (const char k);
        MyCout& operator<< (const int k);
        MyCout& operator<< (const long long k);
        MyCout& operator<< (const unsigned k);
        MyCout& operator<< (const unsigned long long k);
        MyCout& operator<< (const double k);
        void flush();
    };
    extern MyCout cout;
    void outputhtml(const std::string &jsonstring, const std::string &language = "zh-CN");
}

#endif