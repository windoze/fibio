TODO List for Fiberized.IO
==========================

BUGS
----
* <del>scheduler::add_thread caused problem, disabled(FIXED)</del>

Core
----

* <del>Add signal handler to scheduler to handle Ctrl-C/Ctrl-D/...</del>
    * No need, we can use `asio::use_future` to get a future and wait it with timeout, see echo_server example
* Make `concurrent_queue` fully work between `fiber` and `not-a-fiber`
    * <del>c_q<fibers::mutex, fiber::c_v> can transfer data from outside to fiber, as long as there is no size limit(push won't block)</del>
    * <del>c_q<std::mutex, std::c_v> can transfer data from a fiber to outside, as long as there is no size limit(push won't block)</del>
    * Extra work is still needed to make both directions work with size_limit set
* Find a way to get stack track for uncaught exception in fiber
* <del>Find a way to properly implement timeout for async ops</del>
    * `asio::use_future` can be waited with timeout
* <del>Future support(DONE)</del>
* Logging
    * Async log (high throughput/low reliability)
    * Sync log (low throughput/high reliability)
    * Boost.Log integration (?)
    * Log4CXX/Log4CPP/Log4CPlus (?)
* async/await support (?), this is little hard as creating coroutine inside a fiber may interfere with fiber scheduling, need to find a clean solution to support this
* <del>Make sure `fibio::condition_variable` and `std::condition_variable` can be used to communicate between `fiber` and `not-a-fiber`</del>
    * <del>Make sure `not-a-fiber` can notify `fiber` via `fibio::condition_variable`</del>(Only bare-notify works, as mutex only works inside of fibers, should not be big problem as fibio::condition_variable doesn't spuriously wake up waiters)
    * <del>Make sure `fiber` can notify `not-a-fiber` via `std::condition_variable`</del>
* Make `future` to work between `fiber` and `not-a-fiber`, for now it cannot as set_value/exception needs to lock mutex
* <del>Shared mutex(DONE)</del>
* Windows support, will start as soon as I have access to a Windows machine with development tool installed :-(
    * Fiberized main function, WinMain and ServiceMain, ANSI and Unicode version
    * Windows handle stream(should work with some typedefs)
    * Async file read/write(should work with some typedefs)
    * Windows std stream guard(should work with minor efforts)
    * Windows Service control


TODO List for auxiliary libraries
=================================

Protocol
--------

* HTTP support moved to a separated library at [fibio-http](https://github.com/windoze/fibio-http)
* SMTP client to send mail

Utilities
---------

* Serialization
    * JSON
    * XML
    * BSON
* Database driver
    * Redis driver
    * MongoDB driver(?)
    * MySQL driver(?)
    * Cassandra driver(?)
* RPC framework
    * Thrift
    * Protocol buffer based
* Fiber-local-allocator(?)
* fiber-local allocated containers(?)

Scriptable
----------

* All script engines seem to have a GIL, not sure how will this impact performance?
* [Lua](http://www.lua.org)/[LuaJIT](http://luajit.org) binding via [LuaWrapper](https://github.com/Tomaka17/luawrapper)
* [ChaiScript](https://github.com/ChaiScript/ChaiScript) binding
* [V8](https://code.google.com/p/v8/) binding(?)
