//
//  websocket_echo.cpp
//  fibio
//
//  Created by Chen Xu on 14-3-12.
//  Copyright (c) 2014 0d0a.com. All rights reserved.
//

#include <boost/asio/signal_set.hpp>
#include <fibio/fiber.hpp>
#include <fibio/fiberize.hpp>
#include <fibio/http_server.hpp>

using namespace fibio;
using namespace fibio::http;

/**
 * JavaScript WebSocket client demo from http://cjihrig.com/blog/creating-your-own-websocket-echo-client/
 */
const std::string html
(
 "<!DOCTYPE html>\n"
 "<html lang=\"en\">\n"
 "<head>\n"
 "  <title>WebSocket Echo Client</title>\n"
 "  <meta charset=\"UTF-8\" />\n"
 "  <script>\n"
 "    \"use strict\";\n"
 "    // Initialize everything when the window finishes loading\n"
 "    window.addEventListener(\"load\", function(event) {\n"
 "      var status = document.getElementById(\"status\");\n"
 "      var url = document.getElementById(\"url\");\n"
 "      var open = document.getElementById(\"open\");\n"
 "      var close = document.getElementById(\"close\");\n"
 "      var send = document.getElementById(\"send\");\n"
 "      var text = document.getElementById(\"text\");\n"
 "      var message = document.getElementById(\"message\");\n"
 "      var socket;\n"
 "\n"
 "      status.textContent = \"Not Connected\";\n"
 "      url.value = \"ws://localhost:23456/echo\";\n"
 "      close.disabled = true;\n"
 "      send.disabled = true;\n"
 "\n"
 "      // Create a new connection when the Connect button is clicked\n"
 "      open.addEventListener(\"click\", function(event) {\n"
 "        open.disabled = true;\n"
 "        socket = new WebSocket(url.value, \"echo-protocol\");\n"
 "\n"
 "        socket.addEventListener(\"open\", function(event) {\n"
 "          close.disabled = false;\n"
 "          send.disabled = false;\n"
 "          status.textContent = \"Connected\";\n"
 "        });\n"
 "\n"
 "        // Display messages received from the server\n"
 "        socket.addEventListener(\"message\", function(event) {\n"
 "          message.textContent = \"Server Says: \" + event.data;\n"
 "        });\n"
 "\n"
 "        // Display any errors that occur\n"
 "        socket.addEventListener(\"error\", function(event) {\n"
 "          message.textContent = \"Error: \" + event;\n"
 "        });\n"
 "\n"
 "        socket.addEventListener(\"close\", function(event) {\n"
 "          open.disabled = false;\n"
 "          status.textContent = \"Not Connected\";\n"
 "        });\n"
 "      });\n"
 "\n"
 "      // Close the connection when the Disconnect button is clicked\n"
 "      close.addEventListener(\"click\", function(event) {\n"
 "        close.disabled = true;\n"
 "        send.disabled = true;\n"
 "        message.textContent = \"\";\n"
 "        socket.close();\n"
 "      });\n"
 "\n"
 "      // Send text to the server when the Send button is clicked\n"
 "      send.addEventListener(\"click\", function(event) {\n"
 "        socket.send(text.value);\n"
 "        text.value = \"\";\n"
 "      });\n"
 "    });\n"
 "  </script>\n"
 "</head>\n"
 "<body>\n"
 "  Status: <span id=\"status\"></span><br />\n"
 "  URL: <input id=\"url\" /><br />\n"
 "  <input id=\"open\" type=\"button\" value=\"Connect\" />&nbsp;\n"
 "  <input id=\"close\" type=\"button\" value=\"Disconnect\" /><br />\n"
 "  <input id=\"send\" type=\"button\" value=\"Send\" />&nbsp;\n"
 "  <input id=\"text\" /><br />\n"
 "  <span id=\"message\"></span>\n"
 "</body>\n"
 "</html>\n"
);

bool html_handler(server::request &req, server::response &resp) {
    resp.status_code(http_status_code::OK);
    resp.body_stream() << html;
    return true;
}

void echo_handler(websocket::connection &conn) {
    std::string text;
    while (conn.recv_msg(websocket::OPCODE::TEXT, std::back_insert_iterator<std::string>(text))) {
        conn.send_text(text);
        text.clear();
    }
}

void test_http_server() {
    server svr;
    fiber signal_handler([&svr](){
        boost::asio::signal_set(asio::get_io_service(),
                                SIGINT,
                                SIGTERM)
        .async_wait(asio::yield);
        svr.stop();
    });
    svr.address("127.0.0.1")
    .port(23456)
    .handler(
             route(
                   path_("/") >> html_handler,
                   path_("/echo") >> websocket::handler("echo-protocol", echo_handler)
                   )
             )
    .start();
    svr.join();
    signal_handler.join();
}

int fibio::main(int argc, char *argv[]) {
    std::cout << "Open http://127.0.0.1:23456 with browser to test" << std::endl;
    this_fiber::get_scheduler().add_worker_thread(3);
    test_http_server();
    std::cout << "main_fiber exiting" << std::endl;
    return 0;
}
