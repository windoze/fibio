[![Build Status](https://travis-ci.org/windoze/fibio.svg?branch=master)](https://travis-ci.org/windoze/fibio)

Fiberized.IO
============

Fiberized.IO is a fast and simple networking framework without compromises.

* <B>Fast</B><BR/>Asynchronous I/O under the hood for maximum speed and throughtput.
* <B>Simple</B><BR/>Fiber based programming model for concise and intuitive development.
* <B>No compromises</B><BR/>Standard C++ thread and iostream compatible API, old-fashion programs just work more efficiently.

Read the [Wiki](https://github.com/windoze/fibio/wiki) for manuals and references

The echo server example
-----------------------
```
#include <fibio/fiberize.hpp>
#include <fibio/iostream.hpp>
 
using namespace fibio;
 
int fibio::main(int argc, char *argv[]) {
    return tcp_listener(7)([](tcp_stream &s){
        s << s.rdbuf();
    }).value();
}
```


The HTTP server example
-----------------------
```
#include <fibio/fiberize.hpp>
#include <fibio/http_server.hpp>

using namespace fibio::http;
 
int fibio::main(int argc, char *argv[]) {
    return server(23456).handler(
        route(
            path_("/add/:x/:y")>>[](double x, double y){return x+y;}
        )
    ).run().value();
}
```
