#include "main.h"

namespace MAIN{
    
    std::string source, id;
    CJsonObject config;

    std::vector<std::string> findid(const std::string &dataprefix, const std::string &source, std::string &id){
        std::vector<std::string> ids;
        #ifdef _WIN32
            _finddata_t finddata;
            int findi1, findi2;
            findi1 = findi2 = _findfirst((dataprefix + "data/" + source + "/*").c_str(), &finddata);
            while (~findi1){
                ids.push_back(finddata.name);
                if (*ids.rbegin() == "." || *ids.rbegin() == "..")
                    ids.pop_back();
                findi1 = _findnext(findi2, &finddata);
            }
            _findclose(findi2);
        #elif linux
            DIR *dirptr = opendir((dataprefix + "data/" + source).c_str());
            dirent *entry;
            while (entry = readdir(dirptr)){
                ids.push_back(entry -> d_name);
                if (*ids.rbegin() == "." || *ids.rbegin() == "..")
                    ids.pop_back();
            }
            closedir(dirptr);
        #else
            Out::cout << I18N::get("MAIN", "MACROUNDEFINED") << '\n';
            PAUSEEXIT;
        #endif
        if (ids.size() && !id.size())
            id = ids[0];
        return ids;
    }

    int readconfig(){
        if (Algo::Access("config.json", 0) == -1){
            Out::cout << "Can't find config.json!\n";
            PAUSEEXIT;
            return 1;
        }
        config = Algo::ReadJSON("config.json");
        config.Get("dataprefix", Algo::dataprefix);
        std::string lang;
        config.Get("language", lang);
        I18N::I18NInit(lang);
        if (Algo::Access("data/", 0) == -1){
            Out::cout << I18N::get("MAIN", "CANTDATA/") << '\n';
            PAUSEEXIT;
            return 1;
        }
        config.Get("source", source);
        if (Algo::Access(("data/" + source).c_str(), 0) == -1){
            Out::cout << I18N::get("MISC", "ERROR") + I18N::get("MAIN", "CANTDATA/SRC") + source + I18N::get("MAIN", "CANTDATA/SRCAFT") + "\n";
            PAUSEEXIT;
            return 1;
        }
        config.Get("id", id);
        std::vector<std::string> ids = findid(Algo::dataprefix, source, id);
        if (!ids.size()){
            Out::cout << I18N::get("MISC", "ERROR") + I18N::get("MAIN", "CANTDATA/SRC/*") + source + I18N::get("MAIN", "CANTDATA/SRC/*AFT") + "\n";
            PAUSEEXIT;
            return 1;
        }
        Out::cout << '\n';
        Out::cout << I18N::get("MAIN", "SRC") + I18N::get("MISC", "COLON") + source + "\n";
        Out::cout << I18N::get("MAIN", "ID") + I18N::get("MISC", "COLON") + id + "\n";
        return 0;
    }

}

using namespace MAIN;

int main(){

    //Algo::testshanten(); return 0;
    //Algo::testtenpai(); return 0;

    auto rcres = readconfig();
    if (rcres == 1) return 0;
    PA::analyzemain("data/", source, id, config);
    //MatchDataCompare::mdatacomparemain();
    return 0;
}