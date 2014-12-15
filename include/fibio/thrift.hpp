//
//  TFibioTransport.h
//  fibio
//
//  Created by Chen Xu on 14/11/12.
//  Copyright (c) 2014 0d0a.com. All rights reserved.
//

#ifndef fibio_thrift_hpp
#define fibio_thrift_hpp

#include <boost/asio/read.hpp>
#include <boost/asio/write.hpp>
#include <fibio/iostream.hpp>
#include <thrift/transport/TVirtualTransport.h>
#include <thrift/transport/TServerTransport.h>
#include <thrift/server/TServer.h>

namespace apache { namespace thrift {
    namespace server { template<typename Stream, bool buffered> class TFibioServer; }
    namespace transport {
        namespace detail {
            template<bool buffered>
            struct io_ops;
            
            template<>
            struct io_ops<true> {
                template<typename Stream>
                static uint32_t read(fibio::stream::fiberized_iostream<Stream> &s, uint8_t* buf, uint32_t len) {
                    if(!s) {
                        throw TTransportException(TTransportException::NOT_OPEN,
                                                  "Cannot read.");
                    }
                    s.read((char *)buf, len);
                    if (s.gcount()<len) {
                        throw TTransportException(TTransportException::END_OF_FILE,
                                                  "No more data to read.");
                    }
                    return len;
                }
                template<typename Stream>
                static void write(fibio::stream::fiberized_iostream<Stream> &s, const uint8_t* buf, uint32_t len) {
                    if(!s) throw TTransportException(TTransportException::NOT_OPEN,
                                                     "Cannot write.");
                    s.write((const char *)buf, len);
                }
            };

            template<>
            struct io_ops<false> {
                template<typename Stream>
                static uint32_t read(fibio::stream::fiberized_iostream<Stream> &s, uint8_t* buf, uint32_t len) {
                    boost::system::error_code ec;
                    uint32_t ret=boost::asio::async_read(s.stream_descriptor(),
                                                         boost::asio::buffer(buf, len),
                                                         fibio::asio::yield[ec]);
                    if(ec) throw TTransportException(TTransportException::END_OF_FILE,
                                                     "No more data to read.");
                    return ret;
                }
                template<typename Stream>
                static void write(fibio::stream::fiberized_iostream<Stream> &s, const uint8_t* buf, uint32_t len) {
                    boost::system::error_code ec;
                    boost::asio::async_write(s.stream_descriptor(),
                                             boost::asio::buffer(buf, len),
                                             fibio::asio::yield[ec]);
                    if(ec) throw TTransportException(TTransportException::NOT_OPEN,
                                                     "Cannot write.");
                }
            };
        }
        template<typename Stream, bool buffered>
        class TFibioTransport : public TVirtualTransport<TFibioTransport<Stream, buffered>> {
        public:
            typedef fibio::stream::fiberized_iostream<Stream> fibio_stream;
            typedef fibio::stream::stream_traits<fibio_stream> traits_type;
            typedef typename traits_type::endpoint_type endpoint_type;
            typedef fibio::stream::listener<fibio_stream> listener_type;
            
            TFibioTransport(const std::string &access_point)
            : ep_(fibio::stream::detail::make_endpoint<endpoint_type>(access_point))
            , stream_obj_(new fibio_stream)
            , stream_(*stream_obj_)
            {}
        
            virtual bool isOpen() override { return stream_.is_open(); }
            
            virtual bool peek() override {
                if (buffered) {
                    return stream_.peek()!=fibio_stream::traits_type::eof();
                } else {
                    return true;
                }
            }
    
            virtual void open() override { stream_.connect(ep_); }
            
            virtual void close() override { stream_.close(); }

            virtual void flush() override { if(buffered) stream_.flush(); }

            uint32_t read(uint8_t* buf, uint32_t len)
            { return detail::io_ops<buffered>::read(stream_, buf, len); }
            
            void write(const uint8_t* buf, uint32_t len)
            { return detail::io_ops<buffered>::write(stream_, buf, len); }

        private:
            friend class server::TFibioServer<Stream, buffered>;
            
            TFibioTransport(fibio_stream &s)
            : stream_obj_()
            , stream_(s)
            {}
            
            endpoint_type ep_;
            std::unique_ptr<fibio_stream> stream_obj_;
            fibio_stream &stream_;
        };
        
        typedef TFibioTransport<boost::asio::ip::tcp::socket, true> TFibioTCPBufferedTransport;
        typedef TFibioTransport<boost::asio::local::stream_protocol::socket, true> TFibioLocalBufferedTransport;
        typedef TFibioTransport<boost::asio::ip::tcp::socket, false> TFibioTCPTransport;
        typedef TFibioTransport<boost::asio::local::stream_protocol::socket, false> TFibioLocalTransport;
    }   // End of namespace apache::thrift::transport
    
    namespace server {
        template<typename Stream, bool buffered>
        class TFibioServer : public TServer {
        public:
            typedef fibio::stream::fiberized_iostream<Stream> fibio_stream;
            typedef fibio::stream::stream_traits<fibio_stream> traits_type;
            typedef typename traits_type::endpoint_type endpoint_type;
            typedef fibio::stream::listener<fibio_stream> listener_type;
            typedef transport::TFibioTransport<Stream, buffered> transport_type;

            template<typename Processor>
            TFibioServer(const boost::shared_ptr<Processor>& processor,
                         const std::string &access_point,
                         const boost::shared_ptr<TProtocolFactory>& protocolFactory,
                         THRIFT_OVERLOAD_IF(Processor, TProcessor))
            : TServer(processor,
                      boost::shared_ptr<TServerTransport>(),    // Not used
                      boost::shared_ptr<TTransportFactory>(new TTransportFactory),
                      protocolFactory)
            , listener_(access_point)
            {}
            
            virtual void serve() override {
                if (eventHandler_) {
                    eventHandler_->preServe();
                }
                listener_.start([this](fibio_stream &s){
                    boost::shared_ptr<transport_type> client(new transport_type(s));
                    boost::shared_ptr<TProtocol> inputProtocol=inputProtocolFactory_->getProtocol(client);
                    boost::shared_ptr<TProtocol> outputProtocol=outputProtocolFactory_->getProtocol(client);
                    boost::shared_ptr<TProcessor> processor = getProcessor(inputProtocol,
                                                                           outputProtocol,
                                                                           client);
                    void* connectionContext = NULL;
                    if (eventHandler_) {
                        connectionContext = eventHandler_->createContext(inputProtocol, outputProtocol);
                    }
                    for (;;) {
                        try {
                            if (eventHandler_) {
                                eventHandler_->processContext(connectionContext, client);
                            }
                            if (!processor->process(inputProtocol,
                                                    outputProtocol,
                                                    connectionContext) ||
                                // Peek ahead, is the remote side closed?
                                !inputProtocol->getTransport()->peek()) {
                                break;
                            }
                        } catch (transport::TTransportException& ttx) {
                            break;
                        }
                    }
                    if (eventHandler_) {
                        eventHandler_->deleteContext(connectionContext, inputProtocol, outputProtocol);
                    }
                });
                listener_.join();
            }
            
            virtual void stop() override {
                listener_.stop();
            }
            
        private:
            listener_type listener_;
        };
        typedef TFibioServer<boost::asio::ip::tcp::socket, true> TFibioTCPBufferedServer;
        typedef TFibioServer<boost::asio::local::stream_protocol::socket, true> TFibioLocalBufferedServer;
        typedef TFibioServer<boost::asio::ip::tcp::socket, false> TFibioTCPServer;
        typedef TFibioServer<boost::asio::local::stream_protocol::socket, false> TFibioLocalServer;
    }   // End of namespace apache::thrift::server
}}   // End of namespace apache::thrift


#endif /* defined(fibio_thrift_hpp) */
