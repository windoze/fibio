cmake_minimum_required(VERSION 2.8)

enable_testing()

add_executable(test_fibers test_fibers.cpp)
TARGET_LINK_LIBRARIES(test_fibers ${FIBIO_LIBS})

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

find_package(OpenSSL)
add_executable(test_tcp_stream test_tcp_stream.cpp)
TARGET_LINK_LIBRARIES(test_tcp_stream ${FIBIO_LIBS} ${OPENSSL_LIBRARIES})

add_executable(echo_server echo_server.cpp)
TARGET_LINK_LIBRARIES(echo_server ${FIBIO_LIBS})

add_test(fibers test_fibers)
add_test(mutex test_mutex)
add_test(condition_variable test_cv)
add_test(concurrent_queue test_cq)
add_test(future test_future)
add_test(ASIO test_asio)

