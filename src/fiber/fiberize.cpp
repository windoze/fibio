//
//  fiberize.cpp
//  fibio
//
//  Created by Chen Xu on 14-3-10.
//  Copyright (c) 2014 0d0a.com. All rights reserved.
//

#include <fibio/fibers/fiberize.hpp>
//#include "std_stream_guard.hpp"

namespace fibio { namespace fibers { namespace detail {
    fiberized_std_stream_guard::fiberized_std_stream_guard(asio::io_service &iosvc)
    : old_cin_buf_(0)
    , old_cout_buf_(0)
    , old_cerr_buf_(0)
    , cin_buf_(new sbuf_t())
    , cout_buf_(new sbuf_t())
    , cerr_buf_(new sbuf_t())
    {
        old_cin_buf_=std::cin.rdbuf(cin_buf_);
        old_cout_buf_=std::cout.rdbuf(cout_buf_);
        old_cerr_buf_=std::cerr.rdbuf(cerr_buf_);
        cin_buf_->assign(0);
        cout_buf_->assign(1);
        cerr_buf_->assign(2);
        // Set cerr to unbuffered
        std::cerr.rdbuf()->pubsetbuf(0, 0);
    }
    
    fiberized_std_stream_guard::~fiberized_std_stream_guard() {
        std::cout.flush();
        std::cerr.flush();
        cin_buf_->release();
        cout_buf_->release();
        cerr_buf_->release();
        std::cin.rdbuf(old_cin_buf_);
        std::cout.rdbuf(old_cout_buf_);
        std::cerr.rdbuf(old_cerr_buf_);
        delete cin_buf_;
        delete cout_buf_;
        delete cerr_buf_;
    }
    
    void fiberized_std_stream_guard::open() {
        if (cin_buf_->is_open()) {
            return;
        }
        cin_buf_->assign(0);
        cout_buf_->assign(1);
        cerr_buf_->assign(2);
        // Set cerr to unbuffered
        std::cerr.rdbuf()->pubsetbuf(0, 0);
    }
}}}   // End of namespace fibio::fibers::detail
