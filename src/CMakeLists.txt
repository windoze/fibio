cmake_minimum_required(VERSION 2.8)

file(GLOB_RECURSE FIBER_HDR "../include/fibio/*.hpp")
file(GLOB FIBER_SRC "fiber/*.[ch]pp")
add_library(fibio ${FIBER_HDR} ${FIBER_SRC})
target_link_libraries(fibio ${Boost_LIBRARIES})

install (TARGETS fibio DESTINATION lib)
install (DIRECTORY "${CMAKE_SOURCE_DIR}/include/fibio"
         DESTINATION include
         PATTERN ".*" EXCLUDE)
