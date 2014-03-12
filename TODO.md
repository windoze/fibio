TODO List for Fiberized.IO
==========================

BUGS
----
* scheduler::add_thread caused problem, disabled

Core
----

* Add signal handler to scheduler to handle Ctrl-C/Ctrl-D/...
* Refine `FIBERIZED_MAIN` macro if we have to determin thread number before starting scheduler
* Complete `fibio::io::{udp, local_stream, local_datagram}`
* Make `concurrent_queue` work between `fiber` and `not-a-fiber`
    * c_q<fibers::mutex, fiber::c_v> can transfer data from outside to fiber, as long as there is no size limit(push won't block)
    * c_q<std::mutex, std::c_v> can transfer data from a fiber to outside, as long as there is no size limit(push won't block)
    * Extra work is still needed to make both directions work with size_limit set
* Shared mutex(RWLock)
* Barrier
* Future support
* Fiber constructor with variadic arguments
* Logging

Protocol
--------

* Test drive, start with a simple Redis client
* HTTP client
    * Check for EOF
* HTTP server framework
    * Session store
    * WebSocket
    * RESTful service
* HTTP request router for HTTP server
* Connection pool
* SSL/HTTPS support

Utilities
---------

* Serialization
    * JSON
    * XML
    * BSON
* Stream with compression
    * zlib
    * bzip2
    * snappy
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

* [Lua](http://www.lua.org)/[LuaJIT](http://luajit.org) binding via [LuaWrapper](https://github.com/Tomaka17/luawrapper)
* [ChaiScript](https://github.com/ChaiScript/ChaiScript) binding
* [V8](https://code.google.com/p/v8/) binding(?)
