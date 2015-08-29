cmake_minimum_required(VERSION 2.8)

enable_testing()

add_executable(test_fibers test_fibers.cpp)
TARGET_LINK_LIBRARIES(test_fibers ${FIBIO_LIBS})

add_executable(test_fss test_fss.cpp)
TARGET_LINK_LIBRARIES(test_fss ${FIBIO_LIBS})

add_executable(test_mutex test_mutex.cpp)
TARGET_LINK_LIBRARIES(test_mutex ${FIBIO_LIBS})

add_executable(test_cv test_cv.cpp)
TARGET_LINK_LIBRARIES(test_cv ${FIBIO_LIBS})

add_executable(test_cq test_cq.cpp)
TARGET_LINK_LIBRARIES(test_cq ${FIBIO_LIBS})

add_executable(test_future test_future.cpp)
TARGET_LINK_LIBRARIES(test_future ${FIBIO_LIBS})

add_executable(test_asio test_asio.cpp)
TARGET_LINK_LIBRARIES(test_asio ${FIBIO_LIBS})

add_executable(test_tcp_stream test_tcp_stream.cpp)
TARGET_LINK_LIBRARIES(test_tcp_stream ${FIBIO_LIBS})

add_executable(test_http_client test_http_client.cpp)
TARGET_LINK_LIBRARIES(test_http_client ${FIBIO_LIBS})

add_executable(test_http_server test_http_server.cpp)
TARGET_LINK_LIBRARIES(test_http_server ${FIBIO_LIBS})

add_executable(test_cookie test_cookie.cpp)
TARGET_LINK_LIBRARIES(test_cookie ${FIBIO_LIBS})

add_executable(test_redis_proto test_redis_proto.cpp)
TARGET_LINK_LIBRARIES(test_redis_proto ${FIBIO_LIBS})

add_executable(test_redis_client test_redis_client.cpp)
TARGET_LINK_LIBRARIES(test_redis_client ${FIBIO_LIBS})

add_test(fibers test_fibers)
add_test(fss test_fss)
add_test(mutex test_mutex)
add_test(condition_variable test_cv)
add_test(concurrent_queue test_cq)
add_test(future test_future)
add_test(ASIO test_asio)
add_test(TCP_stream test_tcp_stream)
add_test(http_client test_http_client)
add_test(http_server test_http_server)
add_test(cookie test_cookie)
add_test(redis_proto test_redis_proto)
add_test(redis_client test_redis_client)

IF(OPENSSL_FOUND)
file(COPY "ca.pem" "dh2048.pem" "server.pem" DESTINATION ${CMAKE_BINARY_DIR}/test)
add_executable(test_ssl_stream test_ssl_stream.cpp)
TARGET_LINK_LIBRARIES(test_ssl_stream ${FIBIO_LIBS})
add_test(SSL_stream test_ssl_stream)
ENDIF(OPENSSL_FOUND)

set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/test)
find_package(Thrift)
if(THRIFT_FOUND)
execute_process(COMMAND ${THRIFT_COMPILER} --gen cpp -out ${CMAKE_BINARY_DIR}/test ${CMAKE_SOURCE_DIR}/test/t.thrift)
INCLUDE_DIRECTORIES(AFTER ${CMAKE_BINARY_DIR}/test)
add_executable(test_thrift
    test_thrift.cpp
    ${CMAKE_BINARY_DIR}/test/Something.cpp
    ${CMAKE_BINARY_DIR}/test/Something.h
    ${CMAKE_BINARY_DIR}/test/t_constants.cpp
    ${CMAKE_BINARY_DIR}/test/t_constants.h
    ${CMAKE_BINARY_DIR}/test/t_types.cpp
    ${CMAKE_BINARY_DIR}/test/t_types.h
)
TARGET_LINK_LIBRARIES(test_thrift ${FIBIO_LIBS} ${THRIFT_LIBS})
add_test(thrift test_thrift)
endif(THRIFT_FOUND)

