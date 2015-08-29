cmake_minimum_required(VERSION 2.8)

add_executable(websocket_echo websocket_echo.cpp)
TARGET_LINK_LIBRARIES(websocket_echo ${FIBIO_LIBS})

add_executable(echo_server echo_server.cpp)
TARGET_LINK_LIBRARIES(echo_server ${FIBIO_LIBS})

