#include "output.h"

namespace Out{

    MyCout cout;

    void MyCout::output(){
        std::cout << buffer;
        buffer.clear();
    }

    MyCout::MyCout(){
        buffer = "";
        memset(cc, 0, sizeof cc);
    }

    MyCout::~MyCout(){
        output();
    }

    MyCout& MyCout::operator<< (const std::string &k){
        buffer += k;
        if (buffer.size() >= maxlength)
            output();
        return *this;
    }

    MyCout& MyCout::operator<< (const char* k){
        buffer += k;
        if (buffer.size() >= maxlength)
            output();
        return *this;
    }

    MyCout& MyCout::operator<< (const char k){
        buffer += k;
        if (buffer.size() >= maxlength)
            output();
        return *this;
    }

    MyCout& MyCout::operator<< (const int k){
        sprintf(cc, "%d", k);
        buffer += cc;
        if (buffer.size() >= maxlength)
            output();
        return *this;
    }

    MyCout& MyCout::operator<< (const long long k){
        sprintf(cc, "%lld", k);
        buffer += cc;
        if (buffer.size() >= maxlength)
            output();
        return *this;
    }

    MyCout& MyCout::operator<< (const unsigned k){
        sprintf(cc, "%u", k);
        buffer += cc;
        if (buffer.size() >= maxlength)
            output();
        return *this;
    }

    MyCout& MyCout::operator<< (const unsigned long long k){
        sprintf(cc, "%llu", k);
        buffer += cc;
        if (buffer.size() >= maxlength)
            output();
        return *this;
    }

    MyCout& MyCout::operator<< (const double k){
        char dd[22];
        sprintf(dd, "%%.%lldf", floatnum);
        sprintf(cc, dd, k);
        buffer += cc;
        if (buffer.size() >= maxlength)
            output();
        return *this;
    }

    void MyCout::flush(){
        output();
    }
}