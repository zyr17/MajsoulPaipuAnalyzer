#ifndef _HEADER_H
#define _HEADER_H

#include <iostream>
#include <vector>
#include <cassert>
#include <ctime>
#include <iomanip>
#include <cmath>
#if defined(linux) || defined(__APPLE__)
    #include <dirent.h>
    #include <sys/ioctl.h>
    #include <unistd.h>
#endif
#ifdef _WIN32
    #include <windows.h>
#endif

#define OUTBUFFERSIZE 1048576
#define FLOATAFTERPOINTNUM 4

#include "lib/CJsonObject.hpp"
#include "output.h"

#define JSONBUFFERSIZE 1048576

#define PAUSEEXIT { Out::cout << I18N::get("MISC", "ENTEREXIT") << '\n'; Out::cout.flush(); for (; getchar() != '\n'; ); }

#define OutputVector(vec) {for (auto &i: vec) std::cout << i << ' '; std::cout << std::endl;}

//#define MATCHDATAOUTPUT
//#define SAVEMATCHDATASTEP 11
//#define PRINTTENPAI

#ifdef DEBUG
    #undef OUTBUFFERSIZE
    #define OUTBUFFERSIZE 0
#else
    #undef assert
    #define assert(content) {}
#endif

using namespace neb;

#endif