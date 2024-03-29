#ifndef _MAIN_H
#define _MAIN_H

#include "header.h"
#include "algo.h"
#include "analyzer.h"
#include "mdatacompare.h"
#include "i18n.h"

#ifdef __APPLE__
#include <sysdir.h>  // for sysdir_start_search_path_enumeration
#include <glob.h>    // for glob needed to expand ~ to user dir
#include <iostream>  // for std::cout
#include <string>    // for std::string
#include <exception> // for std::exception
#include <limits.h>  // for PATH_MAX
#endif

#endif