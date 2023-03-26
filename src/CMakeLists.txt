cmake_minimum_required(VERSION 3.24)
project(rocks)

set(CMAKE_CXX_STANDARD 20)

set(default_build_type "Debug")

include_directories(../include)

file(GLOB SOURCES
        "${PROJECT_SOURCE_DIR}/api/*.cpp"
        "${PROJECT_SOURCE_DIR}/api/*.c"
        "${PROJECT_SOURCE_DIR}/impl/*.cpp"
        "${PROJECT_SOURCE_DIR}/impl/*.c"
        "${PROJECT_SOURCE_DIR}/client/*.cpp"
        "${PROJECT_SOURCE_DIR}/client/*.c"
        "${PROJECT_SOURCE_DIR}/../include/api/*.h"
        "${PROJECT_SOURCE_DIR}/../include/impl/*.h"
        "${PROJECT_SOURCE_DIR}/../include/client/*.h"
        )

add_compile_definitions(LIBROCKS_EXPORTS=1)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -static")
set_target_properties(${LIBRARY_NAME} PROPERTIES POSITION_INDEPENDENT_CODE ON)

link_directories(../lib/debug)

add_library(rocks SHARED ${SOURCES})
target_link_libraries(rocks rocksdb zstd snappy lz4 zlibstatic shlwapi rpcrt4)
