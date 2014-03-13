//
//  std_stream_guard.hpp
//  fibio
//
//  Created by Chen Xu on 14-3-10.
//  Copyright (c) 2014 0d0a.com. All rights reserved.
//

#ifndef fiberized_io_std_stream_guard_hpp
#define fiberized_io_std_stream_guard_hpp

#include <fibio/stream/iostream.hpp>

namespace fibio { namespace fibers { namespace detail {
    struct fiberized_std_stream_guard {
        typedef stream::streambuf<io::posix::stream_descriptor> sbuf_t;
        typedef sbuf_t *sbuf_ptr_t;
        
        fiberized_std_stream_guard(asio::io_service &iosvc);
        ~fiberized_std_stream_guard();
        
        void open();
        
        std::streambuf *old_cin_buf_;
        std::streambuf *old_cout_buf_;
        std::streambuf *old_cerr_buf_;
        sbuf_ptr_t cin_buf_;
        sbuf_ptr_t cout_buf_;
        sbuf_ptr_t cerr_buf_;
    };
    
    typedef std::shared_ptr<fiberized_std_stream_guard> fiberized_std_stream_guard_ptr_t;
}}} // End of namespace fibio::fibers::detail

#endif
