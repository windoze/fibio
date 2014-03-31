TODO List for Fiberized.IO
==========================

BUGS
----
* <del>scheduler::add_thread caused problem, disabled(FIXED)</del>

Core
----

* Add signal handler to scheduler to handle Ctrl-C/Ctrl-D/...
* Refine `FIBERIZED_MAIN` macro if we have to determin thread number before starting scheduler
    * make this work under Windows
* Make `concurrent_queue` fully work between `fiber` and `not-a-fiber`
    * <del>c_q<fibers::mutex, fiber::c_v> can transfer data from outside to fiber, as long as there is no size limit(push won't block)</del>
    * <del>c_q<std::mutex, std::c_v> can transfer data from a fiber to outside, as long as there is no size limit(push won't block)</del>
    * Extra work is still needed to make both directions work with size_limit set
* Find a way to get stack track for uncaught exception in fiber
* Find a way to properly implement timeout for async ops
* <del>Future support(DONE)</del>
* Logging
    * Async log (high throughput/low reliability)
    * Sync log (low throughput/high reliability)
    * Boost.Log integration (?)
    * Log4CXX/Log4CPP/Log4CPlus (?)
* async/await support (?), this is little hard as creating coroutine inside a fiber may interfere with fiber scheduling, need to find a clean solution to support this
* <del>Make sure `fibio::condition_variable` and `std::condition_variable` can be used to communicate between `fiber` and `not-a-fiber`</del>
    * <del>Make sure `not-a-fiber` can notify `fiber` via `fibio::condition_variable`</del>
    * <del>Make sure `fiber` can notify `not-a-fiber` via `std::condition_variable`</del>
* Make sure `future` can work between `fiber` and `not-a-fiber`
* <del>Shared mutex(DONE)</del>
* Windows support
    * Windows handle stream(should work with some typedefs)
    * Windows std stream guard(should work by replacing fd 0,1,2 with handle STDIN,STDOUT,STDERR)
    * Windows Service control

Protocol
--------

* Test drive, start with a simple Redis client
* HTTP client
    * Cookie
    * Chunked response
* HTTP server framework
    * Chunked resquest (File upload, etc.)
    * Session store
    * WebSocket
    * RESTful service
* HTTP request router for HTTP server
* Connection pool
* HTTPS support
* <del>SSL support(DONE)</del>

Utilities
---------

* Serialization
    * JSON
    * XML
    * BSON
* Stream with compression
    * gzip
        * Client can send compressed request
        * <del>Client can receive compressed response (DONE)</del>
        * Server can receive compressed request
        * Server can send compressed response
    * deflate
* Database driver
    * Redis driver
    * MongoDB driver(?)
    * MySQL driver(?)
    * Cassandra driver(?)
* RPC framework
    * Thrift
    * Protocol buffer based
* Template engine for HTTP server
* Fiber-local-allocator(?)
* fiber-local allocated containers(?)

Scriptable
----------

* All script engines seem to have a GIL, not sure how will this impact performance?
* [Lua](http://www.lua.org)/[LuaJIT](http://luajit.org) binding via [LuaWrapper](https://github.com/Tomaka17/luawrapper)
* [ChaiScript](https://github.com/ChaiScript/ChaiScript) binding
* [V8](https://code.google.com/p/v8/) binding(?)
