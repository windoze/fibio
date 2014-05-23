cmake_minimum_required(VERSION 2.8)
project(fibio)

INCLUDE(InstallRequiredSystemLibraries)
SET(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Fiberized.IO")
SET(CPACK_PACKAGE_VENDOR "0d0a.com")
SET(CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_SOURCE_DIR}/README.md")
SET(CPACK_PACKAGE_VERSION "0.1.0")
SET(CPACK_PACKAGE_VERSION_MAJOR "0")
SET(CPACK_PACKAGE_VERSION_MINOR "1")
SET(CPACK_PACKAGE_VERSION_PATCH "0")
SET(CPACK_PACKAGE_CONTACT "Windoze <windoze@0d0a.com>")
SET(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/LICENSE")
SET(CPACK_DEBIAN_PACKAGE_NAME "libfibio-dev")
SET(CPACK_DEBIAN_PACKAGE_MAINTAINER "windoze@0d0a.com")
SET(CPACK_DEBIAN_PACKAGE_HOMEPAGE "https://github.com/windoze/fibio")
SET(CPACK_DEBIAN_PACKAGE_DEPENDS "libboost-coroutine1.55-dev, libboost-context1.55-dev, libboost-system1.55-dev")
INCLUDE(CPack)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -ftemplate-depth=256")
if(APPLE)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-deprecated-declarations -Wno-deprecated-register")
    set(CMAKE_LINK_FLAGS "${CMAKE_LINK_FLAGS} -stdlib=libc++")
endif(APPLE)

find_package(Threads REQUIRED)
find_package(Boost 1.55.0 COMPONENTS system coroutine context REQUIRED)

INCLUDE_DIRECTORIES(
    ${Boost_INCLUDE_DIRS}
    ${CMAKE_SOURCE_DIR}/include
)

set(FIBIO_LIBS)
list(APPEND FIBIO_LIBS fibio ${Boost_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})

ADD_SUBDIRECTORY(src)
ADD_SUBDIRECTORY(test)
