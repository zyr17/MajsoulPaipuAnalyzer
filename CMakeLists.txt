cmake_minimum_required(VERSION 3.5)

project(PaipuAnalyzer)

set (CMAKE_C_COMPILER g++)

aux_source_directory(./src DIR_MAIN)
aux_source_directory(./src/CJsonObject DIR_LIB)
add_executable(PaipuAnalyzer ${DIR_MAIN} ${DIR_LIB})
IF(APPLE)
    SET(CMAKE_CXX_FLAGS_RELEASE "$ENV{CXXFLAGS} -std=c++11 -O3")
    SET(CMAKE_CXX_FLAGS_DEBUG "$ENV{CXXFLAGS} -std=c++11 -O0 -Wall -g -ggdb -D DEBUG")
ELSE()
    SET(CMAKE_CXX_FLAGS_RELEASE "$ENV{CXXFLAGS} -std=c++11 -O3 -static")
    SET(CMAKE_CXX_FLAGS_DEBUG "$ENV{CXXFLAGS} -std=c++11 -O0 -Wall -g -ggdb -D DEBUG -static")
ENDIF()

set(FAKE_TARGET fakeTarget)
add_custom_target(fakeTarget
    node ${CMAKE_CURRENT_SOURCE_DIR}/src/resulthtml_generate.js
    COMMENT "Generate resulthtml.cpp Over"
)
add_dependencies(${PROJECT_NAME} ${FAKE_TARGET})