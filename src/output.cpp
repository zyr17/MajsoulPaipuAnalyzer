#include "output.h"
#include "resulthtml.h"

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

    void outputhtml(const std::string &jsonstring, const std::string &language){
        std::string result = "";
        for (int i = 0; i < resulthtml.size(); i ++ ){
            if (resulthtml[i] == '/' && i < resulthtml.size() - 8 && resulthtml.substr(i, 8) == "/*DATA*/"){
                result += "DATA = " + jsonstring + ";";
                i += 7;
                continue;
            }
            else if (resulthtml[i] == '/' && i < resulthtml.size() - 8 && resulthtml.substr(i, 15) == "/*USELANGUAGE*/"){
                result += "use_language = '" + language + "';";
                i += 14;
                continue;
            }
            result += resulthtml[i];
        }
        auto f = fopen((Header::rootfolderprefix + htmlname).c_str(), "w");
        fprintf(f, "%s", result.c_str());
        fclose(f);
        cout << I18N::get("MISC", "HTMLHINT") << '\n';
        //char c = getchar();
        //if (c == 'x' || c == 'X') return;
        std::string command = "PaipuAnalyzeResult.html";
        #ifdef __linux
            cout << I18N::get("MISC", "HTMLOPENLINUX") << '\n';
            command = "firefox " + command + " &";
        #elif __APPLE__
            cout << I18N::get("MISC", "HTMLOPENAPPLE") << '\n';
            command = "open -a Safari " + Header::rootfolderprefix + command;
        #elif _WIN32
            cout << I18N::get("MISC", "HTMLOPENWIN") << '\n';
        #endif
        system(command.c_str());
    }
}