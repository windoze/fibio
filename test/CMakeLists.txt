cmake_minimum_required(VERSION 2.8)

add_executable(test_mutex test_mutex.cpp)
TARGET_LINK_LIBRARIES(test_mutex fibio ${Boost_LIBRARIES})

add_executable(test_cv test_cv.cpp)
TARGET_LINK_LIBRARIES(test_cv fibio ${Boost_LIBRARIES})

add_executable(test_cq test_cq.cpp)
TARGET_LINK_LIBRARIES(test_cq fibio ${Boost_LIBRARIES})

add_executable(test_tcp_stream test_tcp_stream.cpp)
TARGET_LINK_LIBRARIES(test_tcp_stream fibio ${Boost_LIBRARIES})

add_executable(test_http_client test_http_client.cpp)
TARGET_LINK_LIBRARIES(test_http_client fibio ${Boost_LIBRARIES})
