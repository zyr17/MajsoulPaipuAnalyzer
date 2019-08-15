#ifndef _HEADER_H
#define _HEADER_H

#include <iostream>
#include <vector>
#include <cassert>
#include <ctime>
#include <iomanip>
#include <cmath>
#include <unistd.h>
#if defined(__linux) || defined(__APPLE__)
    #include <dirent.h>
    #include <sys/ioctl.h>
#endif
#ifdef _WIN32
    #include <windows.h>
#endif
#ifdef __APPLE__
    #include <mach-o/dyld.h>
#endif

#define OUTBUFFERSIZE 1048576
#define FLOATAFTERPOINTNUM 4

#include "CJsonObject/CJsonObject.hpp"
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

namespace Header{

    /* 在进行文件操作时，必须在文件路径前添加两个前缀之一 */
    //设置可执行文件所在文件夹位置。对于Windows和Linux平台启动时pwd即为所在位置因此默认./即可；若为macOS需要获取位置并设置。
    extern std::string rootfolderprefix;
    //设置读取data时的前缀，从config.json中获得，调试时用于定位数据文件位置。在完成载入时会将rootfolder作为前缀加入。
    extern std::string datafolderprefix;
}

#endif