#include "i18n.h"

namespace I18N{

    std::string language;
    std::map<std::string, std::string> langmap;
    bool invalidlang = true, warnlanginv = false, warnwordnotexist = false;

    void I18NInit(const std::string &lang){
        language = lang;
        if (access(("i18n/" + lang + ".json").c_str(), 0) == -1){
            std::cout << "Error: i18n/" + lang + ".json not exist! language translation won't work.\n";
            return;
        }
        auto json = Algo::ReadJSON("i18n/" + lang + ".json");
        std::string word1, word2;
        while (json.GetKey(word1)){
            auto jsonp = json[word1];
            while (jsonp.GetKey(word2)){
                std::string s;
                jsonp.Get(word2, s);
                #ifdef _WIN32
                    s = Algo::UTF82GBK(s);
                #endif
                langmap[word1 + "|" + word2] = s;
            }
        }
        invalidlang = false;
    }

    std::string get(const std::string &word1, const std::string &word2){
        auto str = word1 + "|" + word2;
        if (invalidlang && !warnlanginv){
            warnlanginv = true;
            std::cout << "Error: language code invalid or not initialized!\n";
            return word2;
        }
        auto it = langmap.find(str);
        if (it != langmap.end())
            return it -> second;
        if (!warnwordnotexist){
            warnwordnotexist = true;
            std::cout << get("MISC", "ERROR") + get("MISC", "WORDNOTEXIST") + '|' + word1 + '|' + word2 + '\n';
        }
        if (word2 == "ERROR") return "Error: ";
        return word2;
    }

}