//
//  test_tcp_stream.cpp
//  fibio
//
//  Created by Chen Xu on 14-3-12.
//  Copyright (c) 2014 0d0a.com. All rights reserved.
//

#include <iostream>
#include <vector>
#include <chrono>
#include <boost/random.hpp>
#include <boost/lexical_cast.hpp>
#include <fibio/fiber.hpp>
#include <fibio/asio.hpp>
#include <fibio/iostream.hpp>
#include <fibio/stream/ssl.hpp>
#include <fibio/fiberize.hpp>

using namespace fibio;

// Certificates are copied from ASIO SSL example
void ssl_child() {
    this_fiber::sleep_for(std::chrono::seconds(1));
    ssl::context ctx(ssl::context::sslv23);
    boost::system::error_code ec;
    ctx.load_verify_file("ca.pem", ec);
    assert(!ec);
    ssl::tcp_stream str(ctx);
    str.stream_descriptor().set_verify_callback([](bool preverified, ssl::verify_context&ctx){
        char subject_name[256];
        X509* cert = X509_STORE_CTX_get_current_cert(ctx.native_handle());
        X509_NAME_oneline(X509_get_subject_name(cert), subject_name, 256);
        std::cout << "Verifying " << subject_name << "\n";
        return preverified;
    });
    ec=str.connect("127.0.0.1", "23457");
    assert(!ec);
    str << "hello" << std::endl;
    for(int i=0; i<100; i++) {
        // Receive a random number from server and send it back
        std::string line;
        std::getline(str, line);
        int n=boost::lexical_cast<int>(line);
        str << n << std::endl;
    }
    str.close();
}

/**
 * Copy boost/libs/asio/example/cpp03/ssl/{ca,dh512,server}.pem to /tmp before running
 */
void ssl_parent() {
    fiber f(ssl_child);
    boost::random::mt19937 rng;
    boost::random::uniform_int_distribution<> rand(1,1000);

    boost::system::error_code ec;
    
    ssl::context ctx(ssl::context::sslv23);
    ctx.set_options(ssl::context::default_workarounds
                    | ssl::context::no_sslv2
                    | ssl::context::single_dh_use);
    ctx.set_password_callback([](std::size_t, ssl::context::password_purpose)->std::string{ return "test"; });
    ctx.use_certificate_chain_file("server.pem", ec);
    assert(!ec);
    ctx.use_private_key_file("server.pem", ssl::context::pem, ec);
    assert(!ec);
    ctx.use_tmp_dh_file("dh512.pem", ec);
    assert(!ec);

    ssl::tcp_stream str(ctx);
    
    ssl::tcp_stream_acceptor acc("127.0.0.1", 23457);
    acc.accept(str, ec);
    assert(!ec);
    std::string line;
    std::getline(str, line);
    assert(line=="hello");
    for (int i=0; i<100; i++) {
        // Ping client with random number
        int n=rand(rng);
        str << n << std::endl;
        std::getline(str, line);
        int r=boost::lexical_cast<int>(line);
        assert(n==r);
    }
    str.close();
    acc.close();
    
    f.join();
}

int fibio::main(int argc, char *argv[]) {
    fiber_group fibers;
    fibers.create_fiber(ssl_parent);
    fibers.join_all();
    std::cout << "main_fiber exiting" << std::endl;
    return 0;
}
