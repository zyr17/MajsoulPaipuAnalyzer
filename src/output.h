#ifndef _OUTPUT_H
#define _OUTPUT_H

#include "header.h"

namespace Out{
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
}

#endif