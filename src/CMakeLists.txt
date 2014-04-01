cmake_minimum_required(VERSION 2.8)

file(GLOB FIBER_SRC "fiber/*.[ch]pp")
add_library(fibio ${FIBER_SRC})
target_link_libraries(fibio ${Boost_LIBRARIES})
