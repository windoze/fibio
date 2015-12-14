# - Try to find Cassandra C/C++ Driver
# Once done, this will define
#
#  CASSANDRADRIVER_FOUND - system has Mysql-Connector-C++ installed
#  CASSANDRADRIVER_INCLUDE_DIRS - the Mysql-Connector-C++ include directories
#  CASSANDRADRIVER_LIBRARIES - link these to use Mysql-Connector-C++
#
# The user may wish to set, in the CMake GUI or otherwise, this variable:
#  CASSANDRADRIVER_ROOT_DIR - path to start searching for the module

SET(CASSANDRADRIVER_ROOT_DIR
        "${CASSANDRADRIVER_ROOT_DIR}"
        CACHE
        PATH
        "Where to start looking for this component.")

IF (WIN32)
    FIND_PATH(CASSANDRADRIVER_INCLUDE_DIR
            NAMES
            cassandra.h
            PATHS
            "C:\\Program Files"
            HINTS
            ${CASSANDRADRIVER_ROOT_DIR}
            PATH_SUFFIXES
            include)

    FIND_LIBRARY(CASSANDRADRIVER_LIBRARY
            NAMES
            cassandra
            cassandra_static
            HINTS
            ${CASSANDRADRIVER_ROOT_DIR}
            PATH_SUFFIXES
            lib)

ELSE ()
    FIND_PATH(CASSANDRADRIVER_INCLUDE_DIR
            cassandra.h
            HINTS
            ${CASSANDRADRIVER_ROOT_DIR}
            PATH_SUFFIXES
            include)

    FIND_LIBRARY(CASSANDRADRIVER_LIBRARY
            NAMES
            cassandra
            cassandra_static
            HINTS
            ${CASSANDRADRIVER_ROOT_DIR}
            PATH_SUFFIXES
            lib64
            lib)
ENDIF ()

MARK_AS_ADVANCED(CASSANDRADRIVER_INCLUDE_DIR
        CASSANDRADRIVER_LIBRARY)

INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(CASSANDRADRIVER
        DEFAULT_MSG
        CASSANDRADRIVER_INCLUDE_DIR
        CASSANDRADRIVER_LIBRARY)

IF (CASSANDRADRIVER_FOUND)
    SET(CASSANDRADRIVER_INCLUDE_DIRS
            "${CASSANDRADRIVER_INCLUDE_DIR}")
    # Add any dependencies here
    SET(CASSANDRADRIVER_LIBRARIES
            "${CASSANDRADRIVER_LIBRARY}")
    # Add any dependencies here
    MARK_AS_ADVANCED(CASSANDRADRIVER_ROOT_DIR)
ENDIF (CASSANDRADRIVER_FOUND)
