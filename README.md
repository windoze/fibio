Fiberized.IO
============

Fiberized.IO is a fast and simple framework without compromises.

* <B>Fast</B><BR/>Asynchronous I/O under the hood for maximum speed and throughtput.
* <B>Simple</B><BR/>Fiber based programming model for concise and intuitive development.
* <B>No compromises</B><BR/>Standard C++ thread and iostream compatible API, old-fashion programs just work more efficiently.

The echo server example
-----------------------
<pre><code>
#include &lt;iostream&gt;
#include &lt;fibio/fiber.hpp&gt;
#include &lt;fibio/stream/iostream.hpp&gt;

using namespace fibio;

int main_fiber(int argc, char *argv[]) {
    tcp_acceptor acc(atoi(argv[1]));
    while(1) {
        fiber([](tcp_stream s){
            s &lt;&lt; s.rdbuf();
        }, acc()).detach();
    }
    return 0;
}

int main(int argc, char *argv[]) {
    return fiberize(4, main_fiber, argc, argv);
}
</code></pre>