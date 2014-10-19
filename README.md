[![Build Status](https://travis-ci.org/windoze/fibio.svg?branch=http)](https://travis-ci.org/windoze/fibio)

Fiberized.IO
============

Fiberized.IO is a fast and simple networking framework without compromises.

* <B>Fast</B><BR/>Asynchronous I/O under the hood for maximum speed and throughtput.
* <B>Simple</B><BR/>Fiber based programming model for concise and intuitive development.
* <B>No compromises</B><BR/>Standard C++ thread and iostream compatible API, old-fashion programs just work more efficiently.

Read the [Wiki](https://github.com/windoze/fibio/wiki) for manuals and references

The HTTP server example
-----------------------
<pre><code>
#include &lt;fibio/fiberize.hpp&gt;
#include &lt;fibio/http_server.hpp&gt;

using namespace fibio::http;

bool handler(server::request &req, server::response &resp, server::connection &conn) {
    resp.body_stream() << "<HTML><BODY><H1>" << req.params["p"] << "</H1></BODY></HTML>";
    return true;
}

int fibio::main(int argc, char *argv[]) {
    server svr(server::settings{
        route({
            {path_matches("/*p"), handler},
        }),
        "0.0.0.0",
        23456,
    });
    svr.start();
    svr.join();
}
</code></pre>
