//
//  test_http_client.cpp
//  fibio
//
//  Created by Chen Xu on 14-3-12.
//  Copyright (c) 2014 0d0a.com. All rights reserved.
//

#include <iostream>
#include <vector>
#include <chrono>
#include <sstream>
#include <boost/asio/basic_waitable_timer.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <fibio/fiber.hpp>
#include <fibio/fiberize.hpp>
#include <fibio/http_client.hpp>
#include <fibio/http_server.hpp>

using namespace fibio;
using namespace fibio::http;

bool handler(server::request& req, server::response& resp)
{
    resp.headers.insert({"Header1", "Value1"});
    // Write all headers back in a table
    resp.content_type("text/html") << "<HTML><HEAD><TITLE>Test</TITLE></HEAD><BODY>\n";
    resp << "<H1>Request Info</H1>\n";
    resp << "<TABLE>\n";
    resp << "<TR><TD>URL</TD><TD>" << req.url << "</TD></TR>\n";
    resp << "<TR><TD>Schema</TD><TD>" << req.parsed_url.schema << "</TD></TR>\n";
    resp << "<TR><TD>Port</TD><TD>" << req.parsed_url.port << "</TD></TR>\n";
    resp << "<TR><TD>Path</TD><TD>" << req.parsed_url.path << "</TD></TR>\n";
    resp << "<TR><TD>Query</TD><TD>" << req.parsed_url.query << "</TD></TR>\n";
    resp << "<TR><TD>User Info</TD><TD>" << req.parsed_url.userinfo << "</TD></TR>\n";
    resp << "</TABLE>\n";
    resp << "<H1>Headers</H1>\n";
    resp << "<TABLE>\n";
    for (auto& p : req.headers) {
        resp << "<TR><TD>" << p.first << "</TD><TD>" << p.second << "</TD></TR>\n";
    }
    resp << "</TABLE>\n";
    resp << "<H1>Parameters</H1>\n";
    resp << "<TABLE>\n";

    for (auto& p : req.params) {
        resp << "<TR><TD>" << p.first << "</TD><TD>" << p.second << "</TD></TR>\n";
    }
    resp << "</TABLE>\n";
    resp << "<H1>Query</H1>\n";
    resp << "<TABLE>\n";
    for (auto& p : req.parsed_url.query_params) {
        resp << "<TR><TD>" << p.first << "</TD><TD>" << p.second << "</TD></TR>\n";
    }
    resp << "</TABLE>\n";
    if (req.has_body()) {
        resp.body_stream() << "<H1>Request body</H1>\n<DIV><PRE>";
        resp.body_stream() << req.body_stream().rdbuf();
        resp.body_stream() << "</PRE></DIV>" << std::endl;
    }
    resp << "</BODY></HTML>\n";
    return true;
}

json::wvalue test_model(std::string id)
{
    json::wvalue v;
    v["name"] = id;
    v["value"] = 10000;
    v["taxed_value"] = 10000 - (10000 * 0.4);
    v["in_ca"] = true;
    ;
    return v;
}

const char s[] = "Hello {{name}}\nYou have just won {{value}} dollars!\n{{#in_ca}}\nWell, "
                 "{{taxed_value}} dollars, after taxes.\n{{/in_ca}}";

int add(int x, int y)
{
    return x + y;
}

struct sub
{
    int operator()(int x, int y) const { return x - y; }
};

struct val
{
    int n;
};

// boost::lexical_cast support
std::istream& operator>>(std::istream& is, val& v)
{
    is >> v.n;
    return is;
}

bool gen_handler(server::request& req, server::response& resp, int id)
{
    resp.body_stream() << "<HTML><BODY><H1>id=" << id << "</H1></BODY></HTML>";
    resp.content_type("text/html");
    return true;
}

int rw = 100;
const int ro = 200;
std::vector<int> rwv{2, 4, 6};
const std::vector<int> rov{3, 5, 7};
namespace std {
// HACK: this operator has to be defined in std to follow the dependent name lookup rule
std::ostream& operator<<(std::ostream& os, const std::vector<int>& v)
{
    for (auto e : v) os << e << "\n";
    return os;
}
}

bool chunked_handler(server::request& req, server::response& resp, int n)
{
    std::stringstream ss;
    if (req.has_body()) {
        ss << req.body_stream().rdbuf();
    }
    return resp.write_chunked([&](std::ostream& os) {
        for (int i = 0; i < n; i++) {
            if (ss.str().empty()) {
                os << "Hello, client\n";
            } else {
                os << ss.str();
            }
            // Each flush makes a new chunk
            // Also auto-flush caused by full buffer makes a new chunk
            os.flush();
        }
        return true;
    });
}

bool chk(const std::string& user, const std::string& pass)
{
    return user == "alibaba" && pass == "sesame";
}

auto r
    = route((path_("/") || path_("/index.html") || path_("/index.htm")) >> handler,
            get_("/test1/:id/test2") >> handler,
            post_("/test2/*p") >> handler, // basic handler type
            (get_("/prize_with_auth/:id") && basic_auth("Prize", chk))
                >> mustache_(s, test_model), // mustache template
            get_("/prize/:id") >> mustache_(s, test_model), // mustache template
            get_("/add/:x/:y") >> add, // plain function
            get_("/sub/:x/:y") >> sub(), // functor
            get_("/mul/:x/:y") >> [](int x, int y) { return x * y; }, // lambda
            get_("/mul2/:x/:y") >>
                [](val x, val y) { return x.n * y.n; }, // custom type via boost::lexical_cast
            (path_("/test3/*p") && url_(iends_with{".html"})) >> handler,
            path_("/test3/*") >> stock_handler(http_status_code::FORBIDDEN),
            path_("/test4") >> []() { return 100; }, // no arguments
            path_("/test5/:id") >> gen_handler, // handler with req/resp and additional parameters
            resource("/test6rw", rw), // singleton resource
            resource("/test6ro", ro), // read-only singletin resource
            resources("/test7rw", rwv), // resource collection
            resources("/test7ro", rov), // read-only resource collection
            path_("/chunked/:n") >> chunked_handler, // chunked text
            not_(method_is(http_method::GET)) >> stock_handler(http_status_code::BAD_REQUEST));

void the_client()
{
    client c;
    if (c.connect("127.0.0.1", 23456)) {
        assert(false);
    }

    client::request req;
    client::response resp;
    bool ret;

    // std::cout << "GET /" << std::endl;
    ret = c.send_request(make_request(req, "/"), resp);
    assert(ret);
    assert(resp.status_code == http_status_code::OK);
    assert(resp.status_message == "OK");
    assert(resp.version == http_version::HTTP_1_1);

    // std::cout << "GET /index.html" << std::endl;
    ret = c.send_request(make_request(req, "/index.html"), resp);
    assert(ret);
    assert(resp.status_code == http_status_code::OK);

    // std::cout << "GET /index.htm" << std::endl;
    ret = c.send_request(make_request(req, "/index.htm"), resp);
    assert(ret);
    assert(resp.status_code == http_status_code::OK);

    // std::cout << "GET /index.php" << std::endl;
    ret = c.send_request(make_request(req, "/index.php"), resp);
    assert(ret);
    assert(resp.status_code == http_status_code::NOT_FOUND);

    // std::cout << "GET /test1/123/test2" << std::endl;
    ret = c.send_request(make_request(req, "/test1/123/test2"), resp);
    assert(ret);
    assert(resp.status_code == http_status_code::OK);

    // std::cout << "GET /test1/123" << std::endl;
    ret = c.send_request(make_request(req, "/test1/123"), resp);
    assert(ret);
    assert(resp.status_code == http_status_code::NOT_FOUND);

    // std::cout << "POST /test1/123/test2" << std::endl;
    ret = c.send_request(make_request(req, "/test1/123/test2", "this is request body"), resp);
    assert(ret);
    assert(resp.status_code == http_status_code::BAD_REQUEST);

    // std::cout << "POST /test2/123" << std::endl;
    ret = c.send_request(make_request(req, "/test2/123", "this is request body"), resp);
    assert(ret);
    assert(resp.status_code == http_status_code::OK);

    // std::cout << "POST /test2/123/abc/xyz" << std::endl;
    ret = c.send_request(make_request(req, "/test2/123/abc/xyz", "this is request body"), resp);
    assert(ret);
    assert(resp.status_code == http_status_code::OK);

    // std::cout << "GET /test2/123" << std::endl;
    ret = c.send_request(make_request(req, "/test2/123"), resp);
    assert(ret);
    assert(resp.status_code == http_status_code::NOT_FOUND);

    // std::cout << "GET /test3/with/a/long/and/stupid/url" << std::endl;
    ret = c.send_request(make_request(req, "/test3/with/a/long/and/stupid/url"), resp);
    assert(ret);
    assert(resp.status_code == http_status_code::FORBIDDEN);

    // std::cout << "GET /test3/with/a/long/and/stupid/url" << std::endl;
    ret = c.send_request(make_request(req, "/test3/with/a/long/and/stupid/url.html"), resp);
    assert(ret);
    assert(resp.status_code == http_status_code::OK);

    {
        // std::cout << "GET /add/42/24" << std::endl;
        ret = c.send_request(make_request(req, "/add/42/24"), resp);
        assert(ret);
        assert(resp.status_code == http_status_code::OK);
        int r;
        resp.body_stream() >> r;
        assert(r == 66);
    }
    {
        // std::cout << "GET /sub/42/24" << std::endl;
        ret = c.send_request(make_request(req, "/sub/42/24"), resp);
        assert(ret);
        assert(resp.status_code == http_status_code::OK);
        int r;
        resp.body_stream() >> r;
        assert(r == 18);
    }
    {
        // std::cout << "GET /mul/42/24" << std::endl;
        ret = c.send_request(make_request(req, "/mul/42/24"), resp);
        assert(ret);
        assert(resp.status_code == http_status_code::OK);
        int r;
        resp.body_stream() >> r;
        assert(r == 1008);
    }
    {
        // std::cout << "GET /mul2/42/24" << std::endl;
        ret = c.send_request(make_request(req, "/mul2/42/24"), resp);
        assert(ret);
        assert(resp.status_code == http_status_code::OK);
        int r;
        resp.body_stream() >> r;
        assert(r == 1008);
    }
    {
        // std::cout << "GET /test4" << std::endl;
        ret = c.send_request(make_request(req, "/test4"), resp);
        assert(ret);
        assert(resp.status_code == http_status_code::OK);
        int r;
        resp.body_stream() >> r;
        assert(r == 100);
    }
    {
        // std::cout << "GET /test5/123" << std::endl;
        ret = c.send_request(make_request(req, "/test5/123"), resp);
        assert(ret);
        assert(resp.status_code == http_status_code::OK);
        std::string r;
        resp.body_stream() >> r;
        assert(r == "<HTML><BODY><H1>id=123</H1></BODY></HTML>");
    }
    {
        // std::cout << "GET /test6rw" << std::endl;
        int r;
        ret = c.send_request(make_request(req, "/test6rw"), resp);
        assert(ret);
        assert(resp.status_code == http_status_code::OK);
        resp.body_stream() >> r;
        assert(r == 100 || r == 200);
        // std::cout << "PUT /test6rw" << std::endl;
        req.clear();
        req.url("/test6rw")
            .method(http_method::PUT)
            .version(http_version::HTTP_1_1)
            .keep_alive(true)
            .body(200);
        ret = c.send_request(req, resp);
        assert(ret);
        assert(resp.status_code == http_status_code::OK);
        // std::cout << "GET /test6rw" << std::endl;
        ret = c.send_request(make_request(req, "/test6rw"), resp);
        assert(ret);
        assert(resp.status_code == http_status_code::OK);
        resp.body_stream() >> r;
        assert(r == 200);
    }
    {
        // std::cout << "GET /test6ro" << std::endl;
        int r;
        ret = c.send_request(make_request(req, "/test6ro"), resp);
        assert(ret);
        assert(resp.status_code == http_status_code::OK);
        resp.body_stream() >> r;
        assert(r == 200);
        // std::cout << "PUT /test6ro" << std::endl;
        req.clear();
        req.url("/test6ro")
            .method(http_method::PUT)
            .version(http_version::HTTP_1_1)
            .keep_alive(true)
            .body(300);
        ret = c.send_request(req, resp);
        assert(ret);
        assert(resp.status_code != http_status_code::OK);
    }
    {
        ret = c.send_chunked_request(make_request(req, "/chunked/42"),
                                     [](std::ostream& os) {
                                         os << "Hello, ";
                                         os.flush();
                                         os << "server" << std::endl;
                                         return true;
                                     },
                                     resp);
        assert(ret);
        assert(resp.status_code == http_status_code::OK);
        std::stringstream ss;
        ss << resp.body_stream().rdbuf();
        // Response body is "Hello, server\n"*42
        assert(ss.str().size() == 14 * 42);
    }
    {
        make_request(req, "/prize_with_auth/John%20Doe");
        req.basic_auth("alibaba", "sesame");
        ret = c.send_request(req, resp);
        assert(ret);
        assert(resp.status_code == http_status_code::OK);
        const std::string text(
            "Hello John Doe\nYou have just won 10000 dollars!\nWell, 6000 dollars, after taxes.\n");
        assert(resp.status_code == http_status_code::OK);
        assert(resp.content_length == text.length());
        std::stringstream ss;
        ss << resp.body_stream().rdbuf();
        ss.flush();
        assert(ss.str() == text);
    }
}

void the_url_client()
{
    url_client c;
    client::response& resp = c.request("http://127.0.0.1:23456/");
    assert(resp.status_code == http_status_code::OK);
    c.request("http://127.0.0.1:23456/index.html");
    assert(resp.status_code == http_status_code::OK);
    c.request("http://127.0.0.1:23456/index.htm");
    assert(resp.status_code == http_status_code::OK);
    c.request("http://127.0.0.1:23456/index.php");
    assert(resp.status_code == http_status_code::NOT_FOUND);
    c.request("http://127.0.0.1:23456/test1/123/test2");
    assert(resp.status_code == http_status_code::OK);
    c.request("http://127.0.0.1:23456/test1/123");
    assert(resp.status_code == http_status_code::NOT_FOUND);
    c.request("http://127.0.0.1:23456/test1/123/test2", "this is request body");
    assert(resp.status_code == http_status_code::BAD_REQUEST);
    c.request("http://127.0.0.1:23456/test2/123", "this is request body");
    assert(resp.status_code == http_status_code::OK);
    c.request("http://127.0.0.1:23456/test2/123/abc/xyz", "this is request body");
    assert(resp.status_code == http_status_code::OK);
    c.request("http://127.0.0.1:23456/test2/123");
    assert(resp.status_code == http_status_code::NOT_FOUND);
    c.request("http://127.0.0.1:23456/test3/with/a/long/and/stupid/url");
    assert(resp.status_code == http_status_code::FORBIDDEN);
    c.request("http://127.0.0.1:23456/test3/with/a/long/and/stupid/url.html");
    assert(resp.status_code == http_status_code::OK);
    c.request("http://127.0.0.1:23456/prize/John%20Doe");
    const std::string text(
        "Hello John Doe\nYou have just won 10000 dollars!\nWell, 6000 dollars, after taxes.\n");
    assert(resp.status_code == http_status_code::OK);
    assert(resp.content_length == text.length());
    std::stringstream ss;
    ss << resp.body_stream().rdbuf();
    ss.flush();
    assert(ss.str() == text);
    c.request("http://127.0.0.1:23456/mul/42/24");
    assert(resp.status_code == http_status_code::OK);
    int r;
    resp.body_stream() >> r;
    assert(r == 1008);
    c.request("http://127.0.0.1:23456/test4");
    assert(resp.status_code == http_status_code::OK);
    resp.body_stream() >> r;
    assert(r == 100);
    {
        c.request("http://127.0.0.1:23456/chunked/42");
        assert(resp.status_code == http_status_code::OK);
        std::stringstream ss;
        ss << resp.body_stream().rdbuf();
        // Server response "Hello, client\n" 42 times
        assert(ss.str().size() == 14 * 42);
    }
    {
        client::response& resp = c.chunked_request("http://127.0.0.1:23456/chunked/42",
                                                   [](std::ostream& os) {
                                                       os << "Hello, ";
                                                       os.flush();
                                                       os << "server" << std::endl;
                                                       return true;
                                                   });
        assert(resp.status_code == http_status_code::OK);
        std::stringstream ss;
        ss << resp.body_stream().rdbuf();
        // Server response "Hello, server\n" 42 times
        assert(ss.str().size() == 14 * 42);
    }
}

void test_http_server()
{
    server svr;
    svr.address("127.0.0.1").port(23456).handler(r).start();
    this_fiber::sleep_for(std::chrono::milliseconds(500));
    {
        // Create some clients, do some requests
        fiber_group fibers;
        size_t n = 1;
        for (int i = 0; i < n; i++) {
            fibers.create_fiber(the_client);
            fibers.create_fiber(the_url_client);
        }
        fibers.join_all();
    }
    svr.stop();
    svr.join();
}

void the_ssl_client()
{
    client c;
    ssl::context ctx(ssl::context::tlsv1_client);
    if (c.connect(ctx, "127.0.0.1", 23457)) {
        assert(false);
    }

    client::request req;
    client::response resp;
    bool ret;

    // std::cout << "GET /" << std::endl;
    ret = c.send_request(make_request(req, "/"), resp);
    assert(ret);
    assert(resp.status_code == http_status_code::OK);
    assert(resp.status_message == "OK");
    assert(resp.version == http_version::HTTP_1_1);

    // std::cout << "GET /index.html" << std::endl;
    ret = c.send_request(make_request(req, "/index.html"), resp);
    assert(ret);
    assert(resp.status_code == http_status_code::OK);

    // std::cout << "GET /index.htm" << std::endl;
    ret = c.send_request(make_request(req, "/index.htm"), resp);
    assert(ret);
    assert(resp.status_code == http_status_code::OK);

    // std::cout << "GET /index.php" << std::endl;
    ret = c.send_request(make_request(req, "/index.php"), resp);
    assert(ret);
    assert(resp.status_code == http_status_code::NOT_FOUND);

    // std::cout << "GET /test1/123/test2" << std::endl;
    ret = c.send_request(make_request(req, "/test1/123/test2"), resp);
    assert(ret);
    assert(resp.status_code == http_status_code::OK);

    // std::cout << "GET /test1/123" << std::endl;
    ret = c.send_request(make_request(req, "/test1/123"), resp);
    assert(ret);
    assert(resp.status_code == http_status_code::NOT_FOUND);

    // std::cout << "POST /test1/123/test2" << std::endl;
    ret = c.send_request(make_request(req, "/test1/123/test2", "this is request body"), resp);
    assert(ret);
    assert(resp.status_code == http_status_code::BAD_REQUEST);

    // std::cout << "POST /test2/123" << std::endl;
    ret = c.send_request(make_request(req, "/test2/123", "this is request body"), resp);
    assert(ret);
    assert(resp.status_code == http_status_code::OK);

    // std::cout << "POST /test2/123/abc/xyz" << std::endl;
    ret = c.send_request(make_request(req, "/test2/123/abc/xyz", "this is request body"), resp);
    assert(ret);
    assert(resp.status_code == http_status_code::OK);

    // std::cout << "GET /test2/123" << std::endl;
    ret = c.send_request(make_request(req, "/test2/123"), resp);
    assert(ret);
    assert(resp.status_code == http_status_code::NOT_FOUND);

    // std::cout << "GET /test3/with/a/long/and/stupid/url" << std::endl;
    ret = c.send_request(make_request(req, "/test3/with/a/long/and/stupid/url"), resp);
    assert(ret);
    assert(resp.status_code == http_status_code::FORBIDDEN);

    // std::cout << "GET /test3/with/a/long/and/stupid/url" << std::endl;
    ret = c.send_request(make_request(req, "/test3/with/a/long/and/stupid/url.html"), resp);
    assert(ret);
    assert(resp.status_code == http_status_code::OK);

    {
        // std::cout << "GET /add/42/24" << std::endl;
        ret = c.send_request(make_request(req, "/add/42/24"), resp);
        assert(ret);
        assert(resp.status_code == http_status_code::OK);
        int r;
        resp.body_stream() >> r;
        assert(r == 66);
    }
    {
        // std::cout << "GET /sub/42/24" << std::endl;
        ret = c.send_request(make_request(req, "/sub/42/24"), resp);
        assert(ret);
        assert(resp.status_code == http_status_code::OK);
        int r;
        resp.body_stream() >> r;
        assert(r == 18);
    }
    {
        // std::cout << "GET /mul/42/24" << std::endl;
        ret = c.send_request(make_request(req, "/mul/42/24"), resp);
        assert(ret);
        assert(resp.status_code == http_status_code::OK);
        int r;
        resp.body_stream() >> r;
        assert(r == 1008);
    }
    {
        // std::cout << "GET /mul2/42/24" << std::endl;
        ret = c.send_request(make_request(req, "/mul2/42/24"), resp);
        assert(ret);
        assert(resp.status_code == http_status_code::OK);
        int r;
        resp.body_stream() >> r;
        assert(r == 1008);
    }
    {
        ret = c.send_chunked_request(make_request(req, "/chunked/42"),
                                     [](std::ostream& os) {
                                         os << "Hello, ";
                                         os.flush();
                                         os << "server" << std::endl;
                                         return true;
                                     },
                                     resp);
        assert(ret);
        assert(resp.status_code == http_status_code::OK);
        std::stringstream ss;
        ss << resp.body_stream().rdbuf();
        // Response body is "Hello, server\n"*42
        assert(ss.str().size() == 14 * 42);
    }
    {
        make_request(req, "/prize_with_auth/John%20Doe");
        req.basic_auth("alibaba", "sesame");
        ret = c.send_request(req, resp);
        assert(ret);
        assert(resp.status_code == http_status_code::OK);
        const std::string text(
            "Hello John Doe\nYou have just won 10000 dollars!\nWell, 6000 dollars, after taxes.\n");
        assert(resp.status_code == http_status_code::OK);
        assert(resp.content_length == text.length());
        std::stringstream ss;
        ss << resp.body_stream().rdbuf();
        ss.flush();
        assert(ss.str() == text);
    }
}

void the_ssl_url_client()
{
    url_client c;
    client::response& resp = c.request("https://127.0.0.1:23457/");
    assert(resp.status_code == http_status_code::OK);
    c.request("https://127.0.0.1:23457/index.html");
    assert(resp.status_code == http_status_code::OK);
    c.request("https://127.0.0.1:23457/index.htm");
    assert(resp.status_code == http_status_code::OK);
    c.request("https://127.0.0.1:23457/index.php");
    assert(resp.status_code == http_status_code::NOT_FOUND);
    c.request("https://127.0.0.1:23457/test1/123/test2");
    assert(resp.status_code == http_status_code::OK);
    c.request("https://127.0.0.1:23457/test1/123");
    assert(resp.status_code == http_status_code::NOT_FOUND);
    c.request("https://127.0.0.1:23457/test1/123/test2", "this is request body");
    assert(resp.status_code == http_status_code::BAD_REQUEST);
    c.request("https://127.0.0.1:23457/test2/123", "this is request body");
    assert(resp.status_code == http_status_code::OK);
    c.request("https://127.0.0.1:23457/test2/123/abc/xyz", "this is request body");
    assert(resp.status_code == http_status_code::OK);
    c.request("https://127.0.0.1:23457/test2/123");
    assert(resp.status_code == http_status_code::NOT_FOUND);
    c.request("https://127.0.0.1:23457/test3/with/a/long/and/stupid/url");
    assert(resp.status_code == http_status_code::FORBIDDEN);
    c.request("https://127.0.0.1:23457/test3/with/a/long/and/stupid/url.html");
    assert(resp.status_code == http_status_code::OK);
    c.request("https://127.0.0.1:23457/mul/42/24");
    assert(resp.status_code == http_status_code::OK);
    int r;
    resp.body_stream() >> r;
    assert(r == 1008);
    {
        c.request("https://127.0.0.1:23457/chunked/42");
        assert(resp.status_code == http_status_code::OK);
        std::stringstream ss;
        ss << resp.body_stream().rdbuf();
        // Server response "Hello, client\n" 42 times
        assert(ss.str().size() == 14 * 42);
    }
    {
        c.chunked_request("https://127.0.0.1:23457/chunked/42",
                          [](std::ostream& os) {
                              os << "Hello, ";
                              os.flush();
                              os << "server" << std::endl;
                              return true;
                          });
        assert(resp.status_code == http_status_code::OK);
        std::stringstream ss;
        ss << resp.body_stream().rdbuf();
        // Server response "Hello, client\n" 42 times
        assert(ss.str().size() == 14 * 42);
    }
}

void test_https_server()
{
    boost::system::error_code ec;
    ssl::context ctx(ssl::context::tlsv1_server);
    ctx.set_options(ssl::context::default_workarounds | ssl::context::no_sslv2
                    | ssl::context::single_dh_use);
    ctx.set_password_callback(
        [](std::size_t, ssl::context::password_purpose) -> std::string { return "test"; });
    ctx.use_certificate_chain_file("server.pem", ec);
    assert(!ec);
    ctx.use_private_key_file("server.pem", ssl::context::pem, ec);
    assert(!ec);
    ctx.use_tmp_dh_file("dh2048.pem", ec);
    assert(!ec);

    server svr;
    svr.address("127.0.0.1").port(23457).ssl(ctx).handler(r).start();
    this_fiber::sleep_for(std::chrono::milliseconds(500));
    {
        // Create some clients, do some requests
        fiber_group fibers;
        size_t n = 10;
        for (int i = 0; i < n; i++) {
            fibers.create_fiber(the_ssl_client);
            fibers.create_fiber(the_ssl_url_client);
        }
        fibers.join_all();
    }
    svr.stop();
    svr.join();
}

int fibio::main(int argc, char* argv[])
{
    this_fiber::get_scheduler().add_worker_thread(3);
    fiber_group fibers;
    fibers.create_fiber(test_http_server);
    fibers.create_fiber(test_https_server);
    fibers.join_all();
    std::cout << "main_fiber exiting" << std::endl;
    return 0;
}
