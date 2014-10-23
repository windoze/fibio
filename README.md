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
<pre><code>
#include &lt;fibio/fiberize.hpp&gt;
#include &lt;fibio/http_server.hpp&gt;

using namespace fibio::http;

bool handler(server::request &req,
             server::response &resp)
{
    resp.body_stream() &lt;&lt; "&lt;HTML&gt;&lt;BODY&gt;&lt;H1&gt;"
                       &lt;&lt; req.params["p"]
                       &lt;&lt; "&lt;/H1&gt;&lt;/BODY&gt;&lt;/HTML&gt;"
                       &lt;&lt; std::endl;
    return true;
}

int fibio::main(int argc, char *argv[]) {
    server svr(server::settings{
        route(path_("/*p") >> handler),
        23456,
    });
    svr.start();
    svr.join();
}
</code></pre>
