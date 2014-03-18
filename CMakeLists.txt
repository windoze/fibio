cmake_minimum_required(VERSION 2.8)
project(fibio)

set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} -DASIO_STANDALONE -std=c++11 -ftemplate-depth=256")
if(APPLE)
    set(CMAKE_LINK_FLAGS "${CMAKE_LINK_FLAGS} -stdlib=libc++")
endif(APPLE)

set(Boost_USE_STATIC_LIBS on)
find_package(Boost 1.55.0 COMPONENTS system coroutine context iostreams REQUIRED)
find_package(ZLIB REQUIRED)

INCLUDE_DIRECTORIES(
    ${Boost_INCLUDE_DIRS}
    ${ZLIB_INCLUDE_DIRS}
    ${CMAKE_SOURCE_DIR}/asio/asio/include
    ${CMAKE_SOURCE_DIR}/http-parser
    ${CMAKE_SOURCE_DIR}/include
)

ADD_SUBDIRECTORY(src)
ADD_SUBDIRECTORY(test)
