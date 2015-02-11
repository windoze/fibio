cmake_minimum_required(VERSION 2.8)
project(fibio)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -ftemplate-depth=256")
if(APPLE)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-deprecated-declarations -Wno-deprecated-register")
    set(CMAKE_LINK_FLAGS "${CMAKE_LINK_FLAGS} -stdlib=libc++")
endif(APPLE)

find_package(Threads REQUIRED)
find_package(Boost 1.56.0 COMPONENTS system coroutine context thread REQUIRED)

IF((CMAKE_BUILD_TYPE MATCHES Debug) OR (NOT CMAKE_BUILD_TYPE))
    add_definitions(-DHAVE_VALGRIND_H)
ENDIF((CMAKE_BUILD_TYPE MATCHES Debug) OR (NOT CMAKE_BUILD_TYPE))

INCLUDE_DIRECTORIES(${Boost_INCLUDE_DIRS})

set(FIBIO_LIBS)
list(APPEND FIBIO_LIBS fibio ${Boost_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})

find_package(OpenSSL)
IF(OPENSSL_FOUND)
    add_definitions(-DHAVE_SSL)
	INCLUDE_DIRECTORIES(AFTER ${OPENSSL_INCLUDE_DIR})
	list(APPEND FIBIO_LIBS ${OPENSSL_LIBRARIES})
ENDIF(OPENSSL_FOUND)

find_package(ZLIB)
find_package(Boost 1.56.0 COMPONENTS iostreams)
IF(ZLIB_FOUND AND Boost_FOUND)
	add_definitions(-DHAVE_ZLIB)
	INCLUDE_DIRECTORIES(AFTER ${ZLIB_INCLUDE_DIR})
	list(APPEND FIBIO_LIBS ${ZLIB_LIBRARIES})
	list(APPEND FIBIO_LIBS fibio ${Boost_LIBRARIES})
ENDIF(ZLIB_FOUND AND Boost_FOUND)
list(APPEND FIBIO_LIBS fibio)

INCLUDE_DIRECTORIES(AFTER ${CMAKE_SOURCE_DIR}/include)

IF(WIN32)
	list(APPEND FIBIO_LIBS wsock32 ws2_32)
ENDIF(WIN32)

ADD_SUBDIRECTORY(src)
ADD_SUBDIRECTORY(test)

INCLUDE("CPackLists.txt")

enable_testing()
