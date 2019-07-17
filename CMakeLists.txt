cmake_minimum_required(VERSION 2.8)

project(PaipuAnalyzer)

set (CMAKE_C_COMPILER g++)

aux_source_directory(./src DIR_MAIN)
aux_source_directory(./src/CJsonObject DIR_LIB)
add_executable(PaipuAnalyzer ${DIR_MAIN} ${DIR_LIB})
SET(CMAKE_CXX_FLAGS_DEBUG "$ENV{CXXFLAGS} -std=c++11 -O0 -Wall -g -ggdb -D DEBUG")
IF(APPLE)
    SET(CMAKE_CXX_FLAGS_RELEASE "$ENV{CXXFLAGS} -std=c++11 -O3")
ELSE()
    SET(CMAKE_CXX_FLAGS_RELEASE "$ENV{CXXFLAGS} -std=c++11 -O3 -static")
ENDIF()
