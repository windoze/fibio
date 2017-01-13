CMAKE_MINIMUM_REQUIRED(VERSION 2.8)

SET(HDRS)
SET(SRCS)

SET(FIBER_HDR
	${CMAKE_SOURCE_DIR}/include/fibio/asio.hpp
	${CMAKE_SOURCE_DIR}/include/fibio/concurrent/concurrent_queue.hpp
	${CMAKE_SOURCE_DIR}/include/fibio/fiber.hpp
	${CMAKE_SOURCE_DIR}/include/fibio/fiberize.hpp
	${CMAKE_SOURCE_DIR}/include/fibio/fibers/asio/detail/use_future.hpp
	${CMAKE_SOURCE_DIR}/include/fibio/fibers/asio/detail/yield.hpp
	${CMAKE_SOURCE_DIR}/include/fibio/fibers/asio/use_future.hpp
	${CMAKE_SOURCE_DIR}/include/fibio/fibers/asio/yield.hpp
	${CMAKE_SOURCE_DIR}/include/fibio/fibers/barrier.hpp
	${CMAKE_SOURCE_DIR}/include/fibio/fibers/condition_variable.hpp
	${CMAKE_SOURCE_DIR}/include/fibio/fibers/detail/fiber_base.hpp
	${CMAKE_SOURCE_DIR}/include/fibio/fibers/detail/fiber_data.hpp
	${CMAKE_SOURCE_DIR}/include/fibio/fibers/detail/forward.hpp
	${CMAKE_SOURCE_DIR}/include/fibio/fibers/detail/spinlock.hpp
	${CMAKE_SOURCE_DIR}/include/fibio/fibers/exceptions.hpp
	${CMAKE_SOURCE_DIR}/include/fibio/fibers/fiber.hpp
	${CMAKE_SOURCE_DIR}/include/fibio/fibers/fiber_group.hpp
	${CMAKE_SOURCE_DIR}/include/fibio/fibers/fss.hpp
	${CMAKE_SOURCE_DIR}/include/fibio/fibers/future/async.hpp
	${CMAKE_SOURCE_DIR}/include/fibio/fibers/future/detail/shared_state.hpp
	${CMAKE_SOURCE_DIR}/include/fibio/fibers/future/detail/shared_state_object.hpp
	${CMAKE_SOURCE_DIR}/include/fibio/fibers/future/detail/task_base.hpp
	${CMAKE_SOURCE_DIR}/include/fibio/fibers/future/detail/task_object.hpp
	${CMAKE_SOURCE_DIR}/include/fibio/fibers/future/future.hpp
	${CMAKE_SOURCE_DIR}/include/fibio/fibers/future/future_status.hpp
	${CMAKE_SOURCE_DIR}/include/fibio/fibers/future/packaged_task.hpp
	${CMAKE_SOURCE_DIR}/include/fibio/fibers/future/promise.hpp
	${CMAKE_SOURCE_DIR}/include/fibio/fibers/mutex.hpp
	${CMAKE_SOURCE_DIR}/include/fibio/fibers/shared_mutex.hpp
	${CMAKE_SOURCE_DIR}/include/fibio/future.hpp
	${CMAKE_SOURCE_DIR}/include/fibio/iostream.hpp
	${CMAKE_SOURCE_DIR}/include/fibio/stream/fstream.hpp
	${CMAKE_SOURCE_DIR}/include/fibio/stream/iostream.hpp
	${CMAKE_SOURCE_DIR}/include/fibio/stream/ssl.hpp
	${CMAKE_SOURCE_DIR}/include/fibio/stream/streambuf.hpp
	${CMAKE_SOURCE_DIR}/include/fibio/thrift.hpp
	${CMAKE_SOURCE_DIR}/include/fibio/utility.hpp)
SET(FIBER_SRC
	fiber/condition.cpp
	fiber/fiber_object.cpp
	fiber/fiber_object.hpp
	fiber/future.cpp
	fiber/mutex.cpp
	fiber/scheduler_object.cpp
	fiber/scheduler_object.hpp
	fiber/stream.cpp)
IF((CMAKE_BUILD_TYPE MATCHES Debug) OR (NOT CMAKE_BUILD_TYPE))
	LIST(APPEND SRCS fiber/valgrind/valgrind.h)
ENDIF((CMAKE_BUILD_TYPE MATCHES Debug) OR (NOT CMAKE_BUILD_TYPE))
LIST(APPEND HDRS ${FIBER_HDR})
LIST(APPEND SRCS ${FIBER_SRC})

INCLUDE_DIRECTORIES(AFTER ${CMAKE_SOURCE_DIR}/src/fiber)

IF(BUILD_SHARED_LIBS)
	ADD_LIBRARY(fibio SHARED ${HDRS} ${SRCS})
ELSE(BUILD_SHARED_LIBS)
	ADD_LIBRARY(fibio STATIC ${HDRS} ${SRCS})
ENDIF(BUILD_SHARED_LIBS)
TARGET_LINK_LIBRARIES(${FIBIO_DEPS})

