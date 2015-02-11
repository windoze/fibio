# Locate Fiberized.IO library
# This module defines
#  FIBIO_INCLUDE_DIR
#  FIBIO_LIBRARIES

find_package(Threads REQUIRED)
find_package(Boost 1.56 COMPONENTS system thread coroutine context iostreams REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(ZLIB REQUIRED)

get_filename_component(SELF_DIR "${CMAKE_CURRENT_LIST_FILE}" PATH)
include(${SELF_DIR}/fibio-targets.cmake)
get_filename_component(FIBIO_INCLUDE_DIRS "${SELF_DIR}/../../include" ABSOLUTE)
list(APPEND FIBIO_INCLUDE_DIRS ${Boost_INCLUDE_DIRS} ${OPENSSL_INCLUDE_DIRS} ${ZLIB_INCLUDE_DIRS})
list(APPEND FIBIO_LIBRARIES fibio ${Boost_LIBRARIES} ${OPENSSL_LIBRARIES} ${ZLIB_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})
