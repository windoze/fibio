cmake_minimum_required(VERSION 2.8)
project(fibio)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -ftemplate-depth=256")
if(APPLE)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-deprecated-declarations -Wno-deprecated-register")
    set(CMAKE_LINK_FLAGS "${CMAKE_LINK_FLAGS} -stdlib=libc++")
endif(APPLE)

find_package(Threads REQUIRED)
find_package(Boost 1.56.0 COMPONENTS system coroutine context iostreams thread REQUIRED)

IF((CMAKE_BUILD_TYPE MATCHES Debug) OR (NOT CMAKE_BUILD_TYPE))
    add_definitions(-DHAVE_VALGRIND_H)
ENDIF((CMAKE_BUILD_TYPE MATCHES Debug) OR (NOT CMAKE_BUILD_TYPE))

find_package(OpenSSL)
IF(OPENSSL_FOUND)
    add_definitions(-DHAVE_SSL)
ENDIF(OPENSSL_FOUND)

find_package(ZLIB)
IF(ZLIB_FOUND)
	add_definitions(-DHAVE_ZLIB)
ENDIF(ZLIB_FOUND)

INCLUDE_DIRECTORIES(
    ${Boost_INCLUDE_DIRS}
    ${OPENSSL_INCLUDE_DIR}
    ${ZLIB_INCLUDE_DIR}
    ${CMAKE_SOURCE_DIR}/include
)

set(FIBIO_LIBS)
list(APPEND FIBIO_LIBS fibio ${Boost_LIBRARIES} ${OPENSSL_LIBRARIES} ${ZLIB_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})

ADD_SUBDIRECTORY(src)
ADD_SUBDIRECTORY(test)

INCLUDE("CPackLists.txt")

enable_testing()
