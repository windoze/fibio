CMAKE_MINIMUM_REQUIRED(VERSION 2.8)

ADD_EXECUTABLE(websocket_echo websocket_echo.cpp)
TARGET_LINK_LIBRARIES(websocket_echo ${FIBIO_LIBS})

ADD_EXECUTABLE(echo_server echo_server.cpp)
TARGET_LINK_LIBRARIES(echo_server ${FIBIO_LIBS})

