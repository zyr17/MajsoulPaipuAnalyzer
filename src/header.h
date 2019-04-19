#ifndef _HEADER_H
#define _HEADER_H

#include <bits/stdc++.h>
#ifdef linux
    #include <dirent.h>
    #include <sys/ioctl.h>
    #include <unistd.h>
#endif
#ifdef _WIN32
    #include <windows.h>
#endif
#include "lib/CJsonObject.hpp"

#define JSONBUFFERSIZE 1048576

#define PAUSE { std::cout << I18N::get("MISC", "ENTEREXIT") << '\n'; for (; getchar() != '\n'; ); }

#define OutputVector(vec) {for (auto &i: vec) std::cout << i << ' '; std::cout << std::endl;}

//#define MATCHDATAOUTPUT
//#define SAVEMATCHDATASTEP 11
//#define PRINTTENPAI

#ifdef DEBUG

#else
    #undef assert
    #define assert(content) {}
#endif

using namespace neb;

#endif