//
//  fiberize.hpp
//  fibio
//
//  Created by Chen Xu on 14-3-8.
//  Copyright (c) 2014 0d0a.com. All rights reserved.
//

#ifndef fibio_fiberize_hpp
#define fibio_fiberize_hpp

#include <fibio/stream/iostream.hpp>

namespace fibio { namespace fibers {
    struct fiberized_std_stream_guard {
        typedef stream::streambuf<io::posix::stream_descriptor> sbuf_t;
        typedef sbuf_t *sbuf_ptr_t;
        
        fiberized_std_stream_guard();
        ~fiberized_std_stream_guard();
        
        std::streambuf *old_cin_buf_;
        std::streambuf *old_cout_buf_;
        std::streambuf *old_cerr_buf_;
        sbuf_ptr_t cin_buf_;
        sbuf_ptr_t cout_buf_;
        sbuf_ptr_t cerr_buf_;
    };
    
    int fiberize(size_t nthr, std::function<int(int, char*[])> &&entry, int argc, char *argv[]);
}}  // End of namespace fibio::fibers

#define FIBERIZED(nthr, f) \
int main(int argc, char* argv[]) \
{ int fibio_FABERIZED_##f(int, char *[]); return fibio::fibers::fiberize(nthr, &fibio_FABERIZED_##f, argc, argv); } \
int fibio_FABERIZED_##f

#define FIBERIZED_MAIN FIBERIZED(3, main)

#endif
