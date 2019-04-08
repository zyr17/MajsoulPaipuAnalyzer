cmake_minimum_required(VERSION 2.8)

project(PaipuAnalyzer)

set (CMAKE_C_COMPILER g++)

aux_source_directory(. DIR_MAIN)
aux_source_directory(./lib DIR_LIB)
add_executable(PaipuAnalyzer ${DIR_MAIN} ${DIR_LIB})
SET(CMAKE_CXX_FLAGS_DEBUG "$ENV{CXXFLAGS} -O0 -Wall -g -ggdb")
SET(CMAKE_CXX_FLAGS_RELEASE "$ENV{CXXFLAGS} -O3 -static")