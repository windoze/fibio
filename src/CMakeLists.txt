cmake_minimum_required(VERSION 2.8)

set(HDRS)
set(SRCS)

set(FIBER_HDR
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
	${CMAKE_SOURCE_DIR}/include/fibio/stream/iostream.hpp
	${CMAKE_SOURCE_DIR}/include/fibio/stream/ssl.hpp
	${CMAKE_SOURCE_DIR}/include/fibio/stream/streambuf.hpp
	${CMAKE_SOURCE_DIR}/include/fibio/thrift.hpp
	${CMAKE_SOURCE_DIR}/include/fibio/utility.hpp)
set(FIBER_SRC
	fiber/condition.cpp
	fiber/fiber_object.cpp
	fiber/fiber_object.hpp
	fiber/future.cpp
	fiber/mutex.cpp
	fiber/scheduler_object.cpp
	fiber/scheduler_object.hpp)
if((CMAKE_BUILD_TYPE MATCHES Debug) OR (NOT CMAKE_BUILD_TYPE))
	list(APPEND SRCS fiber/valgrind.h)
endif((CMAKE_BUILD_TYPE MATCHES Debug) OR (NOT CMAKE_BUILD_TYPE))
list(APPEND HDRS ${FIBER_HDR})
list(APPEND SRCS ${FIBER_SRC})

if(WITH_HTTP)
	set(HTTP_HDR
		${CMAKE_SOURCE_DIR}/include/fibio/http/client/client.hpp
		${CMAKE_SOURCE_DIR}/include/fibio/http/client/request.hpp
		${CMAKE_SOURCE_DIR}/include/fibio/http/client/response.hpp
		${CMAKE_SOURCE_DIR}/include/fibio/http/common/common_types.hpp
		${CMAKE_SOURCE_DIR}/include/fibio/http/common/content_type.hpp
		${CMAKE_SOURCE_DIR}/include/fibio/http/common/cookie.hpp
		${CMAKE_SOURCE_DIR}/include/fibio/http/common/json.hpp
		${CMAKE_SOURCE_DIR}/include/fibio/http/common/request.hpp
		${CMAKE_SOURCE_DIR}/include/fibio/http/common/response.hpp
		${CMAKE_SOURCE_DIR}/include/fibio/http/common/string_pred.hpp
		${CMAKE_SOURCE_DIR}/include/fibio/http/common/url_codec.hpp
		${CMAKE_SOURCE_DIR}/include/fibio/http/common/url_parser.hpp
		${CMAKE_SOURCE_DIR}/include/fibio/http/server/request.hpp
		${CMAKE_SOURCE_DIR}/include/fibio/http/server/response.hpp
		${CMAKE_SOURCE_DIR}/include/fibio/http/server/restful.hpp
		${CMAKE_SOURCE_DIR}/include/fibio/http/server/routing.hpp
		${CMAKE_SOURCE_DIR}/include/fibio/http/server/server.hpp
		${CMAKE_SOURCE_DIR}/include/fibio/http/server/template.hpp
		${CMAKE_SOURCE_DIR}/include/fibio/http/server/templates/mustache.hpp
		${CMAKE_SOURCE_DIR}/include/fibio/http/server/templates/mustache_template.hpp
		${CMAKE_SOURCE_DIR}/include/fibio/http_client.hpp
		${CMAKE_SOURCE_DIR}/include/fibio/http_server.hpp)
	set(HTTP_SRC
		http/client.cpp
		http/common.cpp
		http/http_parser_merged.h
		http/server.cpp)
	list(APPEND HDRS ${HTTP_HDR})
	list(APPEND SRCS ${HTTP_SRC})
endif(WITH_HTTP)	

if(WITH_REDIS)
	set(REDIS_HDR
		${CMAKE_SOURCE_DIR}/include/fibio/redis/client.hpp
		${CMAKE_SOURCE_DIR}/include/fibio/redis/redis_proto.hpp
	)
	set(REDIS_SRC
		redis/client.cpp
		redis/redis_proto.cpp
	)
	list(APPEND HDRS ${REDIS_HDR})
	list(APPEND SRCS ${REDIS_SRC})
endif(WITH_REDIS)

add_library(fibio ${HDRS} ${SRCS})
target_link_libraries(${FIBIO_DEPS})

INSTALL(FILES ${FIBER_HDR} DESTINATION include)
INSTALL (TARGETS fibio EXPORT fibio-targets DESTINATION lib/fibio)
INSTALL(EXPORT fibio-targets DESTINATION lib/fibio)
INSTALL(FILES fibio-config.cmake DESTINATION lib/fibio)
