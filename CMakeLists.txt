cmake_minimum_required(VERSION 3.7.1)
project(cpp_streamer)

#set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -std=c++11 -g -Wno-deprecated -Wno-deprecated-declarations -Wno-reorder -Wall -fexceptions -frtti -D__STDC_FORMAT_MACROS -fPIC")
set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -std=c++11 -g -Wno-deprecated -Wno-deprecated-declarations -Wall -fexceptions -frtti -D__STDC_FORMAT_MACROS -fPIC")


set(CMAKE_OUTPUT_BASE ${CMAKE_BINARY_DIR}/output)
set(BUILD_OUTPUT_BASE ${CMAKE_BINARY_DIR}/output)
set(PREFIX_DIR "${BUILD_OUTPUT_BASE}")
set(INSTALL_RPATH "${PREFIX_DIR}/lib")

# set output static libary
SET(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR}/output/lib)

# set pkgconfig path
set(CMAKE_PREFIX_PATH "${CMAKE_BINARY_DIR}/output/lib/pkgconfig")

if (CMAKE_SYSTEM_NAME STREQUAL "Darwin")
if (CMAKE_SYSTEM_PROCESSOR STREQUAL "arm64")
message("-----This is MacOs arm64")
else()
message("-----This is MacOs Intel")
endif()
else()
message("-----This is system ${CMAKE_SYSTEM_NAME}")
endif()
message("")

# set include path
include_directories(${CMAKE_BINARY_DIR}/output/include
                    ${CMAKE_BINARY_DIR}/output/include/openssl
                    src
                    src/net
                    src/net/http
                    src/net/tcp
                    src/utils
                    3rdparty/libuv/include
                    /usr/local/include)

# set lib path
IF (APPLE)
link_directories(${CMAKE_BINARY_DIR}/output/lib /usr/local/lib)
ELSEIF (UNIX)
link_directories(${CMAKE_BINARY_DIR}/output/lib /usr/local/lib64 /usr/local/lib)
ENDIF ()

add_subdirectory(3rdparty)
add_subdirectory(src/tools)
