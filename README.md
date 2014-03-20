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

void servant(tcp_stream s) {
    while(!s.eof()) {
        std::string line;
        std::getline(s, line);
        s &lt;&lt; line &lt;&lt; std::endl;
    }
}

int main_fiber(int argc, char *argv[]) {
    try {
        auto acc=io::listen(atoi(argv[1]));
        while(1) {
            tcp_stream stream(io::accept(acc));
            fiber(servant, std::move(stream)).detach();
        }
    } catch (std::system_error &e) {
        std::cerr &lt;&lt; e.what() &lt;&lt; std::endl;
        return 1;
    }
    return 0;
}

int main(int argc, char *argv[]) {
    return fibio::fibers::fiberize(4, main_fiber, argc, argv);
}
</code></pre>