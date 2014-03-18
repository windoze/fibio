cmake_minimum_required(VERSION 2.8)

add_executable(test_mutex test_mutex.cpp)
TARGET_LINK_LIBRARIES(test_mutex ${FIBIO_LIBS})

add_executable(test_cv test_cv.cpp)
TARGET_LINK_LIBRARIES(test_cv ${FIBIO_LIBS})

add_executable(test_cq test_cq.cpp)
TARGET_LINK_LIBRARIES(test_cq ${FIBIO_LIBS})

add_executable(test_tcp_stream test_tcp_stream.cpp)
TARGET_LINK_LIBRARIES(test_tcp_stream ${FIBIO_LIBS})

add_executable(test_http_client test_http_client.cpp)
TARGET_LINK_LIBRARIES(test_http_client ${FIBIO_LIBS} ${ZLIB_LIBRARIES})

add_executable(test_http_server test_http_server.cpp)
TARGET_LINK_LIBRARIES(test_http_server ${FIBIO_LIBS} ${ZLIB_LIBRARIES})
