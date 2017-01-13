CMAKE_MINIMUM_REQUIRED(VERSION 2.8)

ENABLE_TESTING()

ADD_EXECUTABLE(test_fibers test_fibers.cpp)
TARGET_LINK_LIBRARIES(test_fibers ${FIBIO_LIBS})

ADD_EXECUTABLE(test_fss test_fss.cpp)
TARGET_LINK_LIBRARIES(test_fss ${FIBIO_LIBS})

ADD_EXECUTABLE(test_mutex test_mutex.cpp)
TARGET_LINK_LIBRARIES(test_mutex ${FIBIO_LIBS})

ADD_EXECUTABLE(test_cv test_cv.cpp)
TARGET_LINK_LIBRARIES(test_cv ${FIBIO_LIBS})

ADD_EXECUTABLE(test_cq test_cq.cpp)
TARGET_LINK_LIBRARIES(test_cq ${FIBIO_LIBS})

ADD_EXECUTABLE(test_future test_future.cpp)
TARGET_LINK_LIBRARIES(test_future ${FIBIO_LIBS})

ADD_EXECUTABLE(test_asio test_asio.cpp)
TARGET_LINK_LIBRARIES(test_asio ${FIBIO_LIBS})

ADD_EXECUTABLE(test_fstream test_fstream.cpp)
TARGET_LINK_LIBRARIES(test_fstream ${FIBIO_LIBS})

ADD_EXECUTABLE(test_tcp_stream test_tcp_stream.cpp)
TARGET_LINK_LIBRARIES(test_tcp_stream ${FIBIO_LIBS})

ADD_EXECUTABLE(test_http_client test_http_client.cpp)
TARGET_LINK_LIBRARIES(test_http_client ${FIBIO_LIBS})

ADD_EXECUTABLE(test_http_server test_http_server.cpp)
TARGET_LINK_LIBRARIES(test_http_server ${FIBIO_LIBS})

ADD_EXECUTABLE(test_cookie test_cookie.cpp)
TARGET_LINK_LIBRARIES(test_cookie ${FIBIO_LIBS})

ADD_EXECUTABLE(test_redis_proto test_redis_proto.cpp)
TARGET_LINK_LIBRARIES(test_redis_proto ${FIBIO_LIBS})

ADD_EXECUTABLE(test_redis_client test_redis_client.cpp)
TARGET_LINK_LIBRARIES(test_redis_client ${FIBIO_LIBS})

ADD_TEST(fibers test_fibers)
ADD_TEST(fss test_fss)
ADD_TEST(mutex test_mutex)
ADD_TEST(condition_variable test_cv)
ADD_TEST(concurrent_queue test_cq)
ADD_TEST(future test_future)
ADD_TEST(ASIO test_asio)
ADD_TEST(fstream test_fstream)
ADD_TEST(TCP_stream test_tcp_stream)
ADD_TEST(http_client test_http_client)
ADD_TEST(http_server test_http_server)
ADD_TEST(cookie test_cookie)
ADD_TEST(redis_proto test_redis_proto)
ADD_TEST(redis_client test_redis_client)

IF (OPENSSL_FOUND)
    FILE(COPY "ca.pem" "dh2048.pem" "server.pem" DESTINATION ${CMAKE_BINARY_DIR}/test)
    ADD_EXECUTABLE(test_ssl_stream test_ssl_stream.cpp)
    TARGET_LINK_LIBRARIES(test_ssl_stream ${FIBIO_LIBS})
    ADD_TEST(SSL_stream test_ssl_stream)
ENDIF (OPENSSL_FOUND)

IF (WITH_THRIFT)
    SET(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/test)
    FIND_PACKAGE(Thrift)
    IF (THRIFT_FOUND)
        EXECUTE_PROCESS(COMMAND ${THRIFT_COMPILER} --gen cpp -out ${CMAKE_BINARY_DIR}/test ${CMAKE_SOURCE_DIR}/test/t.thrift)
        INCLUDE_DIRECTORIES(AFTER ${CMAKE_BINARY_DIR}/test)
        ADD_EXECUTABLE(test_thrift
                test_thrift.cpp
                ${CMAKE_BINARY_DIR}/test/Something.cpp
                ${CMAKE_BINARY_DIR}/test/Something.h
                ${CMAKE_BINARY_DIR}/test/t_constants.cpp
                ${CMAKE_BINARY_DIR}/test/t_constants.h
                ${CMAKE_BINARY_DIR}/test/t_types.cpp
                ${CMAKE_BINARY_DIR}/test/t_types.h
                )
        TARGET_LINK_LIBRARIES(test_thrift ${FIBIO_LIBS} ${THRIFT_LIBS})
        ADD_TEST(thrift test_thrift)
    ENDIF (THRIFT_FOUND)
ENDIF (WITH_THRIFT)

IF (WITH_MYSQL AND MYSQLCONNECTORCPP_FOUND)
    EXECUTE_PROCESS(COMMAND sh -c "mysql -u root < ${CMAKE_SOURCE_DIR}/test/prepare_test_mysql.sql")
    ADD_EXECUTABLE(test_mysql_client test_mysql_client.cpp)
    TARGET_LINK_LIBRARIES(test_mysql_client ${FIBIO_LIBS})
    ADD_TEST(mysql_client test_mysql_client)
ENDIF (WITH_MYSQL AND MYSQLCONNECTORCPP_FOUND)

IF (WITH_CASSANDRA AND CASSANDRADRIVER_FOUND)
    #EXECUTE_PROCESS(COMMAND sh -c "mysql -u root < ${CMAKE_SOURCE_DIR}/test/prepare_test_mysql.sql")
    ADD_EXECUTABLE(test_cassandra test_cassandra.cpp)
    TARGET_LINK_LIBRARIES(test_cassandra ${FIBIO_LIBS})
    ADD_TEST(cassandra test_cassandra)
ENDIF (WITH_CASSANDRA AND CASSANDRADRIVER_FOUND)

