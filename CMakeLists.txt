cmake_minimum_required(VERSION 2.8)

project(PaipuAnalyzer)

set (CMAKE_C_COMPILER g++)

aux_source_directory(./src DIR_MAIN)
aux_source_directory(./src/lib DIR_LIB)
add_executable(PaipuAnalyzer ${DIR_MAIN} ${DIR_LIB})
SET(CMAKE_CXX_FLAGS_DEBUG "$ENV{CXXFLAGS} -O0 -Wall -g -ggdb -D DEBUG")
SET(CMAKE_CXX_FLAGS_RELEASE "$ENV{CXXFLAGS} -O3 -static")