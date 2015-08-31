//
//  chunked_stream.hpp
//  fibio
//
//  Created by Chen Xu on 15/08/30.
//  Copyright (c) 2015 0d0a.com. All rights reserved.
//

#ifndef fibio_http_common_chunked_stream_hpp
#define fibio_http_common_chunked_stream_hpp

#include <iostream>
#include <vector>

namespace fibio { namespace http { namespace common {
    class chunked_istream;
    class chunked_ostream;
    class chunked_iostream;
    
    namespace detail {
        inline bool is_hex(int c) {
            return (c>='0' && c<='9') || (c>='a' && c<='f') || (c>='A' && c<='F');
        }
        
        inline int hex_to_int(int c) {
            if(c>='0' && c<='9') return c-'0';
            if(c>='A' && c<='F') return c-'A'+10;
            return c-'a'+10;
        }
        
        class chunked_istreambuf : public std::basic_streambuf<char> {
        public:
            chunked_istreambuf()
            { init_buffers(); }
        protected:
            virtual pos_type seekoff(off_type off,
                                     std::ios_base::seekdir dir,
                                     std::ios_base::openmode which) override
            {
                // Only seeking in input buffer from current pos is allowed
                if (which!=std::ios_base::in || dir!=std::ios_base::cur) return pos_type(off_type(-1));
                // Do nothing when off=0
                if (off==0) return pos_type(off_type(0));
                char_type* newg=gptr()+off;
                // Cannot seek back into put back area
                if (newg <eback()+putback_max) return pos_type(off_type(-1));
                // Cannot seek beyond end of get area
                if (newg >=egptr()) return pos_type(off_type(-1));
                setg(eback(), newg, egptr());
                return pos_type(off_type(0));
            }
            
            virtual std::streamsize showmanyc() override {
                if ( gptr() == egptr() ) {
                    underflow();
                }
                return egptr()-gptr();
            }
            
            virtual int_type underflow() override {
                if (gptr() == egptr()) {
                    if (current_chunk_>0) {
                        raw_stream_->peek();
                        size_t bytes_transferred=raw_stream_->readsome(&get_buffer_[0]+ putback_max,
                                                                       std::min(current_chunk_,
                                                                                std::streamsize(buffer_size-putback_max)));
                        if (bytes_transferred==0) {
                            return traits_type::eof();
                        }
                        current_chunk_-=bytes_transferred;
                        if (current_chunk_==0) {
                            // Current chunk data all consumed
                            // Consume following "\r\n"
                            if (getchar()!='\r') return traits_type::eof();
                            if (getchar()!='\n') return traits_type::eof();
                        }
                        setg(&get_buffer_[0],
                             &get_buffer_[0] + putback_max,
                             &get_buffer_[0] + putback_max + bytes_transferred);
                        return traits_type::to_int_type(*gptr());
                    } else {
                        // Start a new chunk
                        current_chunk_=read_chunk_size();
                        if (current_chunk_==0) {
                            // This is the last empty chunk
                            return traits_type::eof();
                        }
                        // Read again
                        return underflow();
                    }
                } else {
                    return traits_type::eof();
                }
            }
            
        private:
            void init_buffers() {
                get_buffer_.resize(buffer_size+putback_max);
                setg(&get_buffer_[0],
                     &get_buffer_[0] + putback_max,
                     &get_buffer_[0] + putback_max);
            }
            
            int getchar() {
                char c;
                raw_stream_->get(c);
                if (raw_stream_->eof()) {
                    return EOF;
                }
                return c;
            }
            
            std::streamsize read_chunk_size() {
                int ret=0;
                int c;
                std::string sz;
                while((c = getchar()) != EOF
                      && detail::is_hex(c))
                {
                    ret=ret*16+detail::hex_to_int(c);
                }
                // Make sure it's "\r\n"
                if (c=='\r') {
                    c=getchar();
                    assert(c=='\n');
                }
                return ret;
            }
            
            enum { putback_max = 8 };
            enum { buffer_size = 4096 };
            std::vector<char> get_buffer_;
            std::istream *raw_stream_=nullptr;
            std::streamsize current_chunk_=0;
            friend class fibio::http::common::chunked_istream;
        };
        
        class chunked_ostreambuf : public std::basic_streambuf<char> {
        public:
            chunked_ostreambuf()
            { init_buffers(); }
            
            virtual ~chunked_ostreambuf() {
                // Write data remain in put buffer
                if (pptr() != pbase())
                    overflow(traits_type::eof());
                // Ensure an empty chunk is written
                overflow(traits_type::eof());
            }
        protected:
            int_type overflow(int_type c) override {
                char *ptr=pbase();
                size_t size=pptr() - pbase();
                
                // Chunk header
                write_chunk_size(size);
                // Chunk data
                if (size>0) {
                    raw_stream_->write(ptr, size);
                }
                // End of chunk
                raw_stream_->write("\r\n", 2);
                raw_stream_->flush();
                setp(&put_buffer_[0], &put_buffer_[0] + put_buffer_.size());
                
                // If the new character is eof then our work here is done.
                if (traits_type::eq_int_type(c, traits_type::eof()))
                    return traits_type::not_eof(c);
                
                // Add the new character to the output buffer.
                *pptr() = traits_type::to_char_type(c);
                pbump(1);
                return c;
            }
            
            virtual int sync() override
            {
                if(pptr() - pbase()>0)
                    return overflow(traits_type::eof());
                return traits_type::not_eof(traits_type::eof());
            }
            
        private:
            void init_buffers() {
                put_buffer_.resize(buffer_size);
                setp(&put_buffer_[0], &put_buffer_[0] + put_buffer_.size());
            }
            
            void write_chunk_size(std::streamsize n) {
                std::stringstream ss;
                ss << std::hex << n << "\r\n";
                ss.flush();
                raw_stream_->write(ss.str().c_str(), ss.str().size());
            }
            
            enum { buffer_size = 4096 };
            std::vector<char> put_buffer_;
            std::ostream *raw_stream_=nullptr;
            friend class fibio::http::common::chunked_ostream;
        };
        
        
        class chunked_streambuf : public std::basic_streambuf<char> {
        public:
            chunked_streambuf()
            { init_buffers(); }
            
            virtual ~chunked_streambuf() {
                // Write data remain in put buffer
                if (pptr() != pbase())
                    overflow(traits_type::eof());
                // Ensure an empty chunk is written
                overflow(traits_type::eof());
            }
            
        protected:
            virtual pos_type seekoff(off_type off,
                                     std::ios_base::seekdir dir,
                                     std::ios_base::openmode which) override
            {
                // Only seeking in input buffer from current pos is allowed
                if (which!=std::ios_base::in || dir!=std::ios_base::cur) return pos_type(off_type(-1));
                // Do nothing when off=0
                if (off==0) return pos_type(off_type(0));
                char_type* newg=gptr()+off;
                // Cannot seek back into put back area
                if (newg <eback()+putback_max) return pos_type(off_type(-1));
                // Cannot seek beyond end of get area
                if (newg >=egptr()) return pos_type(off_type(-1));
                setg(eback(), newg, egptr());
                return pos_type(off_type(0));
            }
            
            virtual std::streamsize showmanyc() override {
                if ( gptr() == egptr() ) {
                    underflow();
                }
                return egptr()-gptr();
            }
            
            virtual int_type underflow() override {
                if (gptr() == egptr()) {
                    if (current_chunk_>0) {
                        raw_stream_->peek();
                        size_t bytes_transferred=raw_stream_->readsome(&get_buffer_[0]+ putback_max,
                                                                       std::min(current_chunk_,
                                                                                std::streamsize(buffer_size-putback_max)));
                        if (bytes_transferred==0) {
                            return traits_type::eof();
                        }
                        current_chunk_-=bytes_transferred;
                        setg(&get_buffer_[0],
                             &get_buffer_[0] + putback_max,
                             &get_buffer_[0] + putback_max + bytes_transferred);
                        if (current_chunk_==0) {
                            // Current chunk data all consumed
                            // Consume following "\r\n"
                            if (getchar()!='\r') return traits_type::eof();
                            if (getchar()!='\n') return traits_type::eof();
                        }
                        return traits_type::to_int_type(*gptr());
                    } else {
                        // Start a new chunk
                        current_chunk_=read_chunk_size();
                        if (current_chunk_==0) {
                            // This is the last empty chunk
                            return traits_type::eof();
                        }
                        // Read again
                        return underflow();
                    }
                } else {
                    return traits_type::eof();
                }
            }
            
            int_type overflow(int_type c) override {
                char *ptr=pbase();
                size_t size=pptr() - pbase();
                
                // Chunk header
                write_chunk_size(size);
                // Chunk data
                if (size>0) {
                    raw_stream_->write(ptr, size);
                }
                // End of chunk
                raw_stream_->write("\r\n", 2);
                raw_stream_->flush();
                setp(&put_buffer_[0], &put_buffer_[0] + put_buffer_.size());
                
                // If the new character is eof then our work here is done.
                if (traits_type::eq_int_type(c, traits_type::eof()))
                    return traits_type::not_eof(c);
                
                // Add the new character to the output buffer.
                *pptr() = traits_type::to_char_type(c);
                pbump(1);
                return c;
            }
            
            virtual int sync() override
            {
                if(pptr() - pbase()>0)
                    return overflow(traits_type::eof());
                return traits_type::not_eof(traits_type::eof());
            }
            
        private:
            void init_buffers() {
                get_buffer_.resize(buffer_size+putback_max);
                setg(&get_buffer_[0],
                     &get_buffer_[0] + putback_max,
                     &get_buffer_[0] + putback_max);
                put_buffer_.resize(buffer_size);
                setp(&put_buffer_[0], &put_buffer_[0] + put_buffer_.size());
            }
            
            int getchar() {
                char c;
                raw_stream_->get(c);
                if (raw_stream_->eof()) {
                    return EOF;
                }
                return c;
            }
            
            std::streamsize read_chunk_size() {
                int ret=0;
                int c;
                std::string sz;
                while((c = getchar()) != EOF
                      && detail::is_hex(c))
                {
                    ret=ret*16+detail::hex_to_int(c);
                }
                // Make sure it's "\r\n"
                if (c=='\r') {
                    c=getchar();
                    assert(c=='\n');
                }
                return ret;
            }
            
            void write_chunk_size(std::streamsize n) {
                std::stringstream ss;
                ss << std::hex << n << "\r\n";
                ss.flush();
                raw_stream_->write(ss.str().c_str(), ss.str().size());
            }
            
            enum { putback_max = 8 };
            enum { buffer_size = 4096 };
            std::vector<char> get_buffer_;
            std::vector<char> put_buffer_;
            std::iostream *raw_stream_=nullptr;
            std::streamsize current_chunk_=0;
            friend class fibio::http::common::chunked_iostream;
        };
    }
    
    class chunked_istream : public std::istream {
    public:
        chunked_istream(std::istream *s)
        : std::ios(0), std::istream(&sb_)
        { sb_.raw_stream_=s; }
    private:
        detail::chunked_istreambuf sb_;
    };
    
    class chunked_ostream : public std::ostream {
    public:
        chunked_ostream(std::ostream *s)
        : std::ios(0), std::ostream(&sb_)
        { sb_.raw_stream_=s; }
    private:
        detail::chunked_ostreambuf sb_;
    };
    
    class chunked_iostream : public std::iostream {
    public:
        chunked_iostream(std::iostream *s)
        : std::ios(0), std::iostream(&sb_)
        { sb_.raw_stream_=s; }
    private:
        detail::chunked_streambuf sb_;
    };
}}} // End of namespace fibio::http::common

#endif //fibio_http_common_chunked_stream_hpp
