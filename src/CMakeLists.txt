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

IF(WITH_HTTP)
	SET(HTTP_HDR
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
		${CMAKE_SOURCE_DIR}/include/fibio/http/common/websocket.hpp
		${CMAKE_SOURCE_DIR}/include/fibio/http/server/request.hpp
		${CMAKE_SOURCE_DIR}/include/fibio/http/server/response.hpp
		${CMAKE_SOURCE_DIR}/include/fibio/http/server/restful.hpp
		${CMAKE_SOURCE_DIR}/include/fibio/http/server/routing.hpp
		${CMAKE_SOURCE_DIR}/include/fibio/http/server/server.hpp
		${CMAKE_SOURCE_DIR}/include/fibio/http/server/websocket_handler.hpp
		${CMAKE_SOURCE_DIR}/include/fibio/http/server/template.hpp
		${CMAKE_SOURCE_DIR}/include/fibio/http/server/templates/mustache.hpp
		${CMAKE_SOURCE_DIR}/include/fibio/http/server/templates/mustache_template.hpp
		${CMAKE_SOURCE_DIR}/include/fibio/http_client.hpp
		${CMAKE_SOURCE_DIR}/include/fibio/http_server.hpp)
	SET(HTTP_SRC
		http/chunked_stream.hpp
		http/client.cpp
		http/common.cpp
		http/http_parser_merged.h
		http/server.cpp)
	LIST(APPEND HDRS ${HTTP_HDR})
	LIST(APPEND SRCS ${HTTP_SRC})
ENDIF(WITH_HTTP)

IF(WITH_REDIS)
	SET(REDIS_HDR
		${CMAKE_SOURCE_DIR}/include/fibio/db/redis.hpp
		${CMAKE_SOURCE_DIR}/include/fibio/db/redis/client.hpp
		${CMAKE_SOURCE_DIR}/include/fibio/db/redis/redis_proto.hpp
	)
	SET(REDIS_SRC
		db/redis/client.cpp
		db/redis/redis_proto.cpp
	)
	LIST(APPEND HDRS ${REDIS_HDR})
	LIST(APPEND SRCS ${REDIS_SRC})
ENDIF(WITH_REDIS)

IF(WITH_MYSQL AND MYSQLCONNECTORCPP_FOUND)
	SET(MYSQL_HDR
		${CMAKE_SOURCE_DIR}/include/fibio/db/mysql.hpp
	)
	SET(MYSQL_SRC
		db/mysql/connection.cpp
		db/mysql/connection.hpp
		db/mysql/database_metadata.cpp
		db/mysql/database_metadata.hpp
		db/mysql/driver.cpp
		db/mysql/driver.hpp
		db/mysql/prepared_statement.cpp
		db/mysql/prepared_statement.hpp
		db/mysql/resultset.cpp
		db/mysql/resultset.hpp
		db/mysql/statement.cpp
		db/mysql/statement.hpp
	)
	LIST(APPEND HDRS ${MYSQL_HDR})
	LIST(APPEND SRCS ${MYSQL_SRC})
ENDIF(WITH_MYSQL AND MYSQLCONNECTORCPP_FOUND)

IF (WITH_CASSANDRA AND CASSANDRADRIVER_FOUND)
	SET(CASSANDRA_HDR ${CMAKE_SOURCE_DIR}/include/fibio/db/cassandra.hpp)
	SET(CASSANDRA_SRC db/cassandra/cassandra.cpp)
	LIST(APPEND HDRS ${CASSANDRA_HDR})
	LIST(APPEND SRCS ${CASSANDRA_SRC})
ENDIF (WITH_CASSANDRA AND CASSANDRADRIVER_FOUND)

ADD_LIBRARY(fibio ${HDRS} ${SRCS})
TARGET_LINK_LIBRARIES(${FIBIO_DEPS})

INSTALL(DIRECTORY ${CMAKE_SOURCE_DIR}/include/fibio DESTINATION include
	FILES_MATCHING PATTERN "[a-z]*.hpp")
INSTALL(TARGETS fibio EXPORT fibio-targets DESTINATION lib/fibio)
INSTALL(EXPORT fibio-targets DESTINATION lib/fibio)
INSTALL(FILES fibio-config.cmake DESTINATION lib/fibio)
