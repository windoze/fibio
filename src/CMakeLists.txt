cmake_minimum_required(VERSION 2.8)

file(GLOB FIBER_SRC "fiber/*.[ch]pp")
add_library(fibio ${FIBER_SRC})
target_link_libraries(fibio ${Boost_LIBRARIES})

find_package(ZLIB REQUIRED)
file(GLOB HTTP_SRC "http/*.[ch]pp")
add_library(fibio_http  ${HTTP_SRC} "../http-parser/http_parser.c")
target_link_libraries(fibio_http ${Boost_LIBRARIES} fibio)