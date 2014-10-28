cmake_minimum_required(VERSION 2.8)

file(GLOB_RECURSE FIBER_HDR "../include/fibio/*.hpp")
file(GLOB FIBER_SRC "fiber/*.[ch]pp")
file(GLOB HTTP_SRC "http/*.[ch]pp")
file(GLOB HTTP_PARSER_SRC "http-parser/http_parser.[ch]")
file(GLOB REDIS_SRC "redis/*.[ch]pp")
add_library(fibio ${FIBER_HDR} ${FIBER_SRC} ${HTTP_SRC} ${HTTP_PARSER_SRC} ${REDIS_SRC})
target_link_libraries(fibio ${Boost_LIBRARIES})

install (DIRECTORY "${CMAKE_SOURCE_DIR}/include/fibio"
         DESTINATION include
         PATTERN ".*" EXCLUDE)
install (TARGETS fibio EXPORT fibio-targets DESTINATION lib/fibio)
install (EXPORT fibio-targets DESTINATION lib/fibio)
install (FILES fibio-config.cmake DESTINATION lib/fibio)
