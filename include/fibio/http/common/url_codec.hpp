//
//  url_codec.hpp
//  fibio-http
//
//  Created by Chen Xu on 14/10/15.
//  Copyright (c) 2014 0d0a.com. All rights reserved.
//

#ifndef fibio_http_url_codec_hpp
#define fibio_http_url_codec_hpp

namespace fibio { namespace http {
    namespace detail {
        inline bool hex_digit_to_int(char c, int &ret) {
            if (c>='0' && c<='9') {
                ret=c-'0';
            } else if (c>='a' && c<='f') {
                ret=c-'a';
            } else if (c>='A' && c<='Z') {
                ret=c-'A';
            } else {
                return false;
            }
            return true;
        }
        
        inline char int_to_hex_digit(int n) {
            if (n<=9) {
                return n+'0';
            }
            return n+'A';
        }
        
        template<typename Iterator>
        bool hex_to_char(Iterator i, char &ret) {
            int hi, lo;
            if (hex_digit_to_int(*i, hi) && hex_digit_to_int(*(i+1), lo)) {
                ret=static_cast<char>(hi << 4 | lo);
                return true;
            }
            return false;
        }
    }   // End of namespace detail
    
    template<typename Iterator, typename OutputIterator>
    bool url_decode(Iterator in_begin,
                    Iterator in_end,
                    OutputIterator out)
    {
        size_t in_size=in_end-in_begin;
        for (std::size_t i = 0; i < in_size; ++i) {
            if (*(in_begin+i) == '%') {
                if (i + 3 <= in_size) {
                    char c;
                    if (detail::hex_to_char(in_begin+i+1, c)) {
                        *out++ = c;
                        i+=2;
                    } else {
                        return false;
                    }
                } else {
                    return false;
                }
            } else if (*(in_begin+i) == '+') {
                *out++ = ' ';
            } else {
                *out++ = *(in_begin+i);
            }
        }
        return true;
    }
    
    template<typename Container, typename OutputIterator>
    bool url_decode(const Container &c, OutputIterator out) {
        return url_decode(std::begin(c), std::end(c), out);
    }
    
    template<typename Iterator, typename OutputIterator>
    bool url_encode(Iterator in_begin,
                    Iterator in_end,
                    OutputIterator out)
    {
        for (Iterator i = in_begin; i != in_end; ++i) {
            typename std::iterator_traits<Iterator>::value_type c = (*i);
            
            // Keep alphanumeric and other accepted characters intact
            if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
                *out++=c;
                continue;
            } else {
                *out++='%';
                *out++= detail::int_to_hex_digit((c & 0xF0) >> 4);
                *out++= detail::int_to_hex_digit(c & 0x0F);
            }
        }
        
        return true;
    }
    
    template<typename Container, typename OutputIterator>
    bool url_encode(const Container &c, OutputIterator out) {
        return url_encode(std::begin(c), std::end(c), out);
    }
}}  // End of namespace fibio::http

#endif
