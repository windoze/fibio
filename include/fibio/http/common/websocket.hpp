//
//  websocket.hpp
//  fibio-http
//
//  Created by Chen Xu on 15/08/28.
//  Copyright (c) 2015 0d0a.com. All rights reserved.
//

#ifndef fibio_http_common_websocket_hpp
#define fibio_http_common_websocket_hpp

#include <array>
#include <cstdint>
#include <iostream>

namespace fibio { namespace http { namespace websocket {
    
    constexpr size_t DEFAULT_MAX_PAYLOAD_LEN=4096;
    constexpr size_t DEFAULT_MAX_MESSAGE_SIZE=65536;
    
    enum class OPCODE : uint8_t {
        CONTINUATION = 0,
        TEXT = 1,
        BINARY = 2,
        CLOSE = 8,
        PING = 9,
        PONG = 10,
        INVALID = 255,
    };
    
    typedef std::array<uint8_t, 4> mask_type;
    
    struct frame_header {
        bool fin() const { return (data_[0] & 0x80)!=0; }
        void fin(bool v) { if(v) { data_[0] |= 0x80; } else { data_[0] &= 0x7F; } }
        
        bool rsv1() const { return (data_[0] & 0x40)!=0; }
        void rsv1(bool v) { if(v) { data_[0] |= 0x40; } else { data_[0] &= 0xBF; } }
        
        bool rsv2() const { return (data_[0] & 0x20)!=0; }
        void rsv2(bool v) { if(v) { data_[0] |= 0x20; } else { data_[0] &= 0xDF; } }
        
        bool rsv3() const { return (data_[0] & 0x10)!=0; }
        void rsv3(bool v) { if(v) { data_[0] |= 0x10; } else { data_[0] &= 0xEF; } }
        
        OPCODE opcode() const { return OPCODE(data_[0] & 0x0F); }
        void opcode(OPCODE v) { data_[0] &= 0xF0; data_[0] |= uint8_t(v)&0x0F; }
        
        bool mask() const { return (data_[1] & 0x80)!=0; }
        void mask(bool v) { if(v) { data_[1] |= 0x80; } else { data_[1] &= 0x7F; } }
        
        uint8_t payload_len_7() const { return data_[1] & 0x7F; }
        void payload_len_7(uint8_t v) { data_[1] &= 0x80; data_[1] |= v&0x7F; }
        
        uint16_t ext_payload_len_16() const { return data_[2]<<8 | data_[3]; }
        void ext_payload_len_16(uint16_t v) { data_[2] = v>>8; data_[3] = v&0xFF; }
        
        uint64_t ext_payload_len_64() const {
            uint64_t ret = uint64_t(data_[2]) << 56
                | uint64_t(data_[3]) << 48
                | uint64_t(data_[4]) << 40
                | uint64_t(data_[5]) << 32
                | uint64_t(data_[6]) << 24
                | uint64_t(data_[7]) << 16
                | uint64_t(data_[8]) << 8
                | uint64_t(data_[9]);
            return ret;
        }
        void ext_payload_len_64(uint64_t v) {
            data_[2] = v>>56 & 0xFF;
            data_[3] = v>>48 & 0xFF;
            data_[4] = v>>40 & 0xFF;
            data_[5] = v>>32 & 0xFF;
            data_[6] = v>>24 & 0xFF;
            data_[7] = v>>16 & 0xFF;
            data_[8] = v>>8 & 0xFF;
            data_[9] = v & 0xFF;
        }
        
        uint64_t payload_len() const {
            uint64_t ret=payload_len_7();
            if(ret==126) return ext_payload_len_16();
            if(ret==127) return ext_payload_len_64();
            return ret;
        }
        void internal_set_payload_len(uint64_t v) {
            if(v>0xFFFF) {
                // 8 bytes
                payload_len_7(127);
                ext_payload_len_64(v);
            } else if(v>=126) {
                // 2 bytes
                payload_len_7(126);
                ext_payload_len_16(v);
            } else {
                payload_len_7(v);
            }
        }
        void payload_len(uint64_t v) {
            if(mask()) {
                // Preserve masking key
                mask_type m=masking_key();
                internal_set_payload_len(v);
                masking_key(m);
            } else {
                internal_set_payload_len(v);
            }
        }
        
        uint8_t payload_len_end_idx() const {
            uint8_t pl=payload_len_7();
            if(pl<126) return 2;
            if(pl<127) return 4;
            return 10;
        }
        
        mask_type masking_key() const {
            uint8_t idx=payload_len_end_idx();
            return mask_type{{
                data_[idx],
                data_[idx+1],
                data_[idx+2],
                data_[idx+3]
            }};
        }
        
        void masking_key(const mask_type &v) {
            mask(true);
            uint8_t idx=payload_len_end_idx();
            data_[idx]  =v[0];
            data_[idx+1]=v[1];
            data_[idx+2]=v[2];
            data_[idx+3]=v[3];
        }
        
        size_t size() const {
            return payload_len_end_idx() + (mask()?4:0);
        }
        
        void clear() { memset(data_, 0, sizeof(data_)); }
        
        uint8_t data_[14];
    };
    
    inline std::ostream& operator<<(std::ostream &os, const frame_header &hdr) {
        os.write((char *)(hdr.data_), hdr.size());
        return os;
    }
    
    inline std::istream& operator>>(std::istream &is, frame_header &hdr) {
        is.read((char *)(hdr.data_), 2);
        std::streamsize sz=hdr.payload_len_end_idx()-2;
        if (hdr.mask()) { sz+=4; }
        if (sz>0) {
            is.read((char *)(hdr.data_)+2, sz);
        }
        return is;
    }
    
    struct connection {
        connection(std::istream &is,std::ostream &os)
        : is_(is)
        , os_(os)
        , max_payload_len_(DEFAULT_MAX_PAYLOAD_LEN)
        , max_message_size_(DEFAULT_MAX_MESSAGE_SIZE)
        , masked_(false)
        {}
        
        connection(std::istream &is,std::ostream &os, const mask_type &masking_key)
        : is_(is)
        , os_(os)
        , max_payload_len_(DEFAULT_MAX_PAYLOAD_LEN)
        , max_message_size_(DEFAULT_MAX_MESSAGE_SIZE)
        , masked_(true)
        , masking_key_(masking_key)
        {}
        
        size_t max_payload_len() const { return max_payload_len_; }
        void max_payload_len(size_t v) { max_payload_len_=v; }
        size_t max_message_size() const { return max_message_size_; }
        void max_message_size(size_t v) { max_message_size_=v; }
        bool masked() const { return masked_; }
        void masking_key(const mask_type &mask) { masked_=true; masking_key_=mask; }
        void masking_key() { masked_=false; }
        
        template<typename RandomIterator>
        void send_one_frame(OPCODE op, RandomIterator begin, RandomIterator end, bool last=true) {
            static_assert(sizeof(typename std::iterator_traits<RandomIterator>::value_type)==1,
                          "Byte buffer required");
            static_assert(std::is_same<
                              typename std::iterator_traits<RandomIterator>::iterator_category,
                              std::random_access_iterator_tag
                          >::value,
                          "Random iterator required");
            frame_header hdr;
            hdr.clear();
            hdr.fin(last);
            hdr.opcode(op);
            size_t sz=end-begin;
            hdr.payload_len(sz);
            if(masked_) hdr.masking_key(masking_key_);
            os_ << hdr;
            for (size_t i=0; i<sz; ++i) {
                uint8_t d=*(begin+i);
                if (masked_) {
                    d ^= masking_key_[i%4];
                }
                os_.write((char *)(&d), 1);
            }
            os_.flush();
        }
        
        template<typename RandomIterator>
        void send(OPCODE op, RandomIterator begin, RandomIterator end) {
            bool first=true;
            while(begin<end) {
                if(!first) {
                    op=OPCODE::CONTINUATION;
                }
                RandomIterator frame_end=begin+max_payload_len_;
                if(frame_end>end) {
                    frame_end=end;
                }
                send_one_frame(op, begin, frame_end, frame_end==end);
                begin=frame_end;
                first=false;
            }
        }

        template<typename Container>
        void send(OPCODE op, const Container &c) {
            send(op, std::begin(c), std::end(c));
        }
        
        template<typename RandomIterator>
        void send_binary(RandomIterator begin, RandomIterator end) {
            send(OPCODE::BINARY, begin, end);
        }

        template<typename Container>
        void send_binary(const Container &c) {
            send_binary(std::begin(c), std::end(c));
        }

        template<typename RandomIterator>
        void send_text(RandomIterator begin, RandomIterator end) {
            send(OPCODE::TEXT, begin, end);
        }
        
        template<typename Container>
        void send_text(const Container &c) {
            send_text(std::begin(c), std::end(c));
        }
        
        template<typename RandomIterator>
        void ping(RandomIterator begin, RandomIterator end) {
            send(OPCODE::PING, begin, end);
        }
        
        template<typename Container>
        void ping(const Container &c) {
            ping(std::begin(c), std::end(c));
        }

        template<typename RandomIterator>
        void pong(RandomIterator begin, RandomIterator end) {
            send(OPCODE::PONG, begin, end);
        }
        
        template<typename Container>
        void pong(const Container &c) {
            pong(std::begin(c), std::end(c));
        }

        template<typename RandomIterator>
        void close(RandomIterator begin, RandomIterator end) {
            send(OPCODE::CLOSE, begin, end);
        }
        
        template<typename Container>
        void close(const Container &c) {
            close(std::begin(c), std::end(c));
        }

        void close() {
            close(std::string());
        }
        
        template<typename OutputIterator>
        std::pair<uint8_t, size_t> recv_one_frame(OutputIterator it) {
            //static_assert(sizeof(typename std::iterator_traits<OutputIterator>::value_type)==1, "Byte buffer required");
            frame_header hdr;
            hdr.clear();
            is_ >> hdr;
            size_t sz=hdr.payload_len();
            if(sz>max_payload_len_) {
                return {0xFF, 0};
            }
            mask_type m;
            if(hdr.mask()) { m=hdr.masking_key(); }
            for(size_t i=0; i<sz; ++i) {
                char d;
                is_.read(&d, 1);
                if(hdr.mask()) {
                    d ^= m[i%4];
                }
                *it++=d;
            }
            return {hdr.data_[0], sz};
        }
        
        template<typename OutputIterator>
        std::pair<uint8_t, size_t> recv_one_frame(OPCODE op, OutputIterator it) {
            //static_assert(sizeof(typename std::iterator_traits<OutputIterator>::value_type)==1, "Byte buffer required");
            frame_header hdr;
            hdr.clear();
            is_ >> hdr;
            if(hdr.opcode()!=op && hdr.opcode()!=OPCODE::CONTINUATION) {
                return {0xFF, 0};
            }
            size_t sz=hdr.payload_len();
            if(sz>max_payload_len_) {
                return {0xFF, 0};
            }
            mask_type m;
            if(hdr.mask()) { m=hdr.masking_key(); }
            for(size_t i=0; i<sz; ++i) {
                char d;
                is_.read(&d, 1);
                if(hdr.mask()) {
                    d ^= m[i%4];
                }
                *it++=d;
            }
            return {hdr.data_[0], sz};
        }
        
        template<typename OutputIterator>
        OPCODE recv_msg(OutputIterator it) {
            size_t msg_sz=0;
            std::pair<uint8_t, size_t> ret;
            bool first=true;
            OPCODE op=OPCODE::INVALID;
            do {
                ret=recv_one_frame(it);
                op=OPCODE(ret.first & 0x0F);
                msg_sz+=ret.second;
                if(msg_sz>max_message_size_) return OPCODE::INVALID;
                first=false;
            } while((ret.first&0x80)==0);
            return op;
        }
        
        template<typename OutputIterator>
        bool recv_msg(OPCODE op, OutputIterator it) {
            size_t msg_sz=0;
            std::pair<uint8_t, size_t> ret;
            bool first=true;
            do {
                ret=recv_one_frame(op, it);
                if(ret.first==0xFF) return false;
                msg_sz+=ret.second;
                if(msg_sz>max_message_size_) return false;
                first=false;
            } while((ret.first&0x80)==0);
            return true;
        }
        
        std::istream &is_;
        std::ostream &os_;
        size_t max_payload_len_;
        size_t max_message_size_;
        bool masked_;
        mask_type masking_key_;
    };

    typedef std::function<void(websocket::connection &)> connection_handler;
}}} // End of namespace fibio::http::websocket

#endif  // !defined(fibio_http_common_websocket_hpp)
