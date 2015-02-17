//
//  restful.hpp
//  fibio
//
//  Created by Chen Xu on 14/12/11.
//  Copyright (c) 2014 0d0a.com. All rights reserved.
//

#ifndef fibio_http_server_restful_hpp
#define fibio_http_server_restful_hpp

#include <fibio/fibers/shared_mutex.hpp>
#include <fibio/http/server/routing.hpp>

namespace fibio { namespace http {
    namespace traits {
        namespace detail {
            struct null_mutex {
                void lock() {}
                void unlock() {}
                void lock_shared() {}
                void unlock_shared() {}
            };
            template<bool l>
            struct lock_type;
            
            // For non-lock-free types
            template<>
            struct lock_type<false> {
                typedef fibio::shared_timed_mutex type;
            };
            
            // For lock-free types
            template<>
            struct lock_type<true> {
                typedef null_mutex type;
            };
        }
        
        template<typename T>
        struct is_lock_free {
            static constexpr bool value=false;
        };
        
        template<typename T>
        struct is_lock_free<const T> {
            static constexpr bool value=true;
        };
        
        template<typename T>
        struct resource_lock {
            typedef typename detail::lock_type<is_lock_free<T>::value>::type type;
        };
        
        // map and unordered_map
        template<typename Collection>
        struct collection_traits {
            typedef typename Collection::key_type key_type;
            typedef typename Collection::mapped_type value_type;
            static bool index(std::ostream &s, const Collection &c) {
                return false;
            }
            static bool get(std::ostream &s, const Collection &c, const key_type &k) {
                try {
                    s << c.at(k);
                    return true;
                } catch(std::out_of_range &) {
                    return false;
                }
            }
            // Same behavior as map::insert_or_assign
            static std::pair<key_type, bool> put(std::istream &s, Collection &c, const key_type &k) {
                try {
                    // Update
                    s >> c.at(k);
                    return {k, false};
                } catch(std::out_of_range &) {
                    // Insert
                    value_type v;
                    s >> v;
                    c.insert({k, std::move(v)});
                    return {k, true};
                }
            }
            static bool delete_(Collection &c, const key_type &k) {
                return c.erase(k)>0;
            }
        };
        
        // vector
        template<typename T>
        struct collection_traits<std::vector<T>> {
            typedef std::vector<T> Collection;
            typedef typename Collection::size_type key_type;
            typedef typename Collection::value_type value_type;
            static bool index(std::ostream &s, const Collection &c) {
                return false;
            }
            static bool get(std::ostream &s, const Collection &c, const key_type &k) {
                if(k>=c.size()) { return false; }
                s << c[k];
                return true;
            }
            static std::pair<key_type, bool> put(std::istream &s, Collection &c, const key_type &k) {
                if(k>=c.size()) {
                    // Insert
                    value_type v;
                    s >> v;
                    c.push_back(std::move(v));
                    return {c.size()-1, true};
                } else {
                    s >> c[k];
                    return {k, false};
                }
            }
            static bool delete_(Collection &c, const key_type &k) {
                if(k>=c.size()) return false;
                c.erase(std::begin(c)+k);
                return true;
            }
        };
        
        template<typename Collection>
        struct collection_traits<const Collection> {
            typedef collection_traits<Collection> base_traits_type;
            typedef typename base_traits_type::key_type key_type;
            typedef typename base_traits_type::value_type value_type;
            static bool index(std::ostream &s, const Collection &c) {
                return base_traits_type::index(s, c);
            }
            static bool get(std::ostream &s, const Collection &c, const key_type &k) {
                return base_traits_type::get(s, c, k);
            }
        };
    }   // End of namespace traits
    
    // Singleton resource
    template<typename T>
    struct resource_controller : std::enable_shared_from_this<resource_controller<T>> {
        typedef std::enable_shared_from_this<resource_controller<T>> base_type;
        resource_controller(const std::string &prefix, T &t) : prefix_(prefix), t_(t) {}
        static constexpr bool is_lock_free=traits::is_lock_free<T>::value;
        
        operator routing_table() {
            auto p=base_type::shared_from_this();
            return routing_table::make(
                                       get_(prefix_) >> [p](){
                                           fibio::shared_lock<typename traits::resource_lock<T>::type> lk(p->mtx_);
                                           return p->t_;
                                       },
                                       put_(prefix_) >> [p](server::request &req, server::response &){
                                           fibio::unique_lock<typename traits::resource_lock<T>::type> lk(p->mtx_);
                                           req >> (p->t_);
                                           return true;
                                       });
        }
        
        typename traits::resource_lock<T>::type mtx_;
        const std::string prefix_;
        T &t_;
    };
    
    // Singleton read only resource
    template<typename T>
    struct const_resource_controller : std::enable_shared_from_this<const_resource_controller<T>> {
        typedef std::enable_shared_from_this<const_resource_controller<T>> base_type;
        const_resource_controller(const std::string &prefix, const T &t) : prefix_(prefix), t_(t) {}
        
        operator routing_table() const {
            auto p=base_type::shared_from_this();
            return routing_table::make(get_(prefix_) >> [p](){
                fibio::shared_lock<typename traits::resource_lock<const T>::type> lk(p->mtx_);
                return p->t_;
            });
        }
        
        mutable typename traits::resource_lock<const T>::type mtx_;
        const std::string prefix_;
        const T &t_;
    };
    
    template<typename Collection>
    struct resource_key_generator;
    
    struct null_key_generator{};
    
    // Key generator for vector, key is not used
    template<typename T>
    struct resource_key_generator<std::vector<T>> {
        typedef std::vector<T> Collection;
        typedef typename traits::collection_traits<Collection> traits_type;
        typedef typename traits_type::key_type key_type;
        constexpr key_type operator()(Collection &c) const {
            return 0;
        }
    };
    
    // Key generator for list, key is not used
    template<typename T>
    struct resource_key_generator<std::list<T>> {
        typedef std::list<T> Collection;
        typedef typename traits::collection_traits<Collection> traits_type;
        typedef typename traits_type::key_type key_type;
        constexpr key_type operator()(Collection &c) const {
            return 0;
        }
    };
    
    // Collection resource with key generator, key generator will generate a new key for posted new item.
    template<typename Collection, typename KeyGen>
    struct resources_controller : std::enable_shared_from_this<resources_controller<Collection, KeyGen>> {
        typedef std::enable_shared_from_this<resources_controller<Collection, KeyGen>> base_type;
        typedef typename traits::collection_traits<Collection> traits_type;
        typedef typename traits_type::key_type key_type;
        typedef typename traits_type::value_type value_type;
        
        resources_controller(const std::string &prefix, Collection &c, KeyGen kg=KeyGen())
        : prefix_(prefix)
        , item_(prefix+"/:key")
        , c_(c)
        , kg_(kg)
        {}
        
        operator routing_table() {
            auto p=base_type::shared_from_this();
            return routing_table::make(
                                       // Index
                                       get_(prefix_) >> [p](server::request &, server::response &resp) {
                                           try {
                                               fibio::shared_lock<typename traits::resource_lock<Collection>::type> lk(p->mtx_);
                                               if(!traits_type::index(resp.body_stream(), p->c_)) {
                                                   throw(server_error(http_status_code::FORBIDDEN));
                                               }
                                               return true;
                                           } catch(server_error &e) {
                                               throw;
                                           } catch(...) {
                                               throw(server_error(http_status_code::BAD_REQUEST));
                                           }
                                       },
                                       // Create item with a auto generated key
                                       post_(prefix_) >>[p](server::request &req, server::response &resp, key_type &&k) {
                                           try {
                                               if (!req.has_body()) {
                                                   throw(server_error(http_status_code::BAD_REQUEST));
                                               }
                                               fibio::unique_lock<typename traits::resource_lock<Collection>::type> lk(p->mtx_);
                                               auto ret=traits_type::put(req.body_stream(), p->c_, p->kg_(p->c_));
                                               if(ret.second) {
                                                   // Created
                                                   resp.status_code(http_status_code::CREATED);
                                                   resp.header("Location", p->prefix_+"/"+boost::lexical_cast<std::string>(ret.first));
                                               } else {
                                                   // Updated, shouldn't happen with a correct keygen
                                               }
                                               return true;
                                           } catch(server_error &e) {
                                               throw;
                                           } catch(...) {
                                               throw(server_error(http_status_code::BAD_REQUEST));
                                           }
                                       },
                                       // Read item
                                       get_(item_) >> [p](server::request &req, server::response &resp, key_type &&k) {
                                           try {
                                               fibio::shared_lock<typename traits::resource_lock<Collection>::type> lk(p->mtx_);
                                               if(!traits_type::get(resp.body_stream(), p->c_, k)) {
                                                   throw(server_error(http_status_code::NOT_FOUND));
                                               }
                                               return true;
                                           } catch(server_error &e) {
                                               throw;
                                           } catch(...) {
                                               throw(server_error(http_status_code::BAD_REQUEST));
                                           }
                                       },
                                       // Update/create item with supplied key
                                       put_(item_) >> [p](server::request &req, server::response &resp, key_type &&k) {
                                           try {
                                               if (!req.has_body()) {
                                                   throw(server_error(http_status_code::BAD_REQUEST));
                                               }
                                               fibio::unique_lock<typename traits::resource_lock<Collection>::type> lk(p->mtx_);
                                               auto ret=traits_type::put(req.body_stream(), p->c_, k);
                                               if(ret.second) {
                                                   // Created
                                                   resp.status_code(http_status_code::CREATED);
                                                   resp.header("Location", p->prefix_+"/"+boost::lexical_cast<std::string>(ret.first));
                                               } else {
                                                   // Updated
                                               }
                                               return true;
                                           } catch(server_error &e) {
                                               throw;
                                           } catch(...) {
                                               throw(server_error(http_status_code::BAD_REQUEST));
                                           }
                                       },
                                       // Delete item
                                       delete_(item_) >> [p](server::request &, server::response &, key_type &&k) {
                                           try {
                                               fibio::unique_lock<typename traits::resource_lock<Collection>::type> lk(p->mtx_);
                                               if(!traits_type::delete_(p->c_, k)) {
                                                   throw(server_error(http_status_code::NOT_FOUND));
                                               }
                                               return true;
                                           } catch(server_error &e) {
                                               throw;
                                           } catch(...) {
                                               throw(server_error(http_status_code::BAD_REQUEST));
                                           }
                                       });
        }
        
        typename traits::resource_lock<Collection>::type mtx_;
        const std::string prefix_;
        const std::string item_;
        Collection &c_;
        KeyGen kg_;
    };
    
    // Collection resource without key generator, only way to create/update item is to PUT with key.
    template<typename Collection>
    struct resources_controller<Collection, null_key_generator>
    : std::enable_shared_from_this<resources_controller<Collection, null_key_generator>>
    {
        typedef std::enable_shared_from_this<resources_controller<Collection, null_key_generator>> base_type;
        typedef typename traits::collection_traits<Collection> traits_type;
        typedef typename traits_type::key_type key_type;
        typedef typename traits_type::value_type value_type;
        
        resources_controller(const std::string &prefix, Collection &c)
        : prefix_(prefix)
        , item_(prefix+"/:key")
        , c_(c)
        {}
        
        operator routing_table() {
            auto p=base_type::shared_from_this();
            return routing_table::make(
                                       // Index
                                       get_(prefix_) >> [p](server::request &, server::response &resp) {
                                           try {
                                               fibio::shared_lock<typename traits::resource_lock<Collection>::type> lk(p->mtx_);
                                               if(!traits_type::index(resp.body_stream(), p->c_)) {
                                                   throw(server_error(http_status_code::FORBIDDEN));
                                               }
                                               return true;
                                           } catch(server_error &e) {
                                               throw;
                                           } catch(...) {
                                               throw(server_error(http_status_code::BAD_REQUEST));
                                           }
                                       },
                                       // Read item
                                       get_(item_) >> [p](server::request &req, server::response &resp, key_type &&k) {
                                           try {
                                               fibio::shared_lock<typename traits::resource_lock<Collection>::type> lk(p->mtx_);
                                               if(!traits_type::get(resp.body_stream(), p->c_, k)) {
                                                   throw(server_error(http_status_code::NOT_FOUND));
                                               }
                                               return true;
                                           } catch(server_error &e) {
                                               throw;
                                           } catch(...) {
                                               throw(server_error(http_status_code::BAD_REQUEST));
                                           }
                                       },
                                       // Update/create item with supplied key
                                       put_(item_) >> [p](server::request &req, server::response &resp, key_type &&k) {
                                           try {
                                               if (!req.has_body()) {
                                                   throw(server_error(http_status_code::BAD_REQUEST));
                                               }
                                               fibio::unique_lock<typename traits::resource_lock<Collection>::type> lk(p->mtx_);
                                               auto ret=traits_type::put(req.body_stream(), p->c_, k);
                                               if(ret.second) {
                                                   // Created
                                                   resp.status_code(http_status_code::CREATED);
                                                   resp.header("Location", p->prefix_+"/"+boost::lexical_cast<std::string>(ret.first));
                                               } else {
                                                   // Updated
                                               }
                                               return true;
                                           } catch(server_error &e) {
                                               throw;
                                           } catch(...) {
                                               throw(server_error(http_status_code::BAD_REQUEST));
                                           }
                                       },
                                       // Delete item
                                       delete_(item_) >> [p](server::request &, server::response &, key_type &&k) {
                                           try {
                                               fibio::unique_lock<typename traits::resource_lock<Collection>::type> lk(p->mtx_);
                                               if(!traits_type::delete_(p->c_, k)) {
                                                   throw(server_error(http_status_code::NOT_FOUND));
                                               }
                                               return true;
                                           } catch(server_error &e) {
                                               throw;
                                           } catch(...) {
                                               throw(server_error(http_status_code::BAD_REQUEST));
                                           }
                                       });
        }
        
        typename traits::resource_lock<Collection>::type mtx_;
        const std::string prefix_;
        const std::string item_;
        Collection &c_;
    };
    
    // Read-only collection resource
    template<typename Collection>
    struct const_resources_controller : std::enable_shared_from_this<const_resources_controller<Collection>> {
        typedef std::enable_shared_from_this<const_resources_controller<Collection>> base_type;
        typedef typename traits::collection_traits<const Collection> traits_type;
        typedef typename traits_type::key_type key_type;
        typedef typename traits_type::value_type value_type;
        
        const_resources_controller(const std::string &prefix, const Collection &c)
        : prefix_(prefix)
        , item_(prefix+"/:key")
        , c_(c)
        {}
        
        operator routing_table() const {
            auto p=base_type::shared_from_this();
            return routing_table::make(
                                       // Index
                                       get_(prefix_) >> [p](server::request &, server::response &resp) {
                                           try {
                                               fibio::shared_lock<typename traits::resource_lock<const Collection>::type> lk(p->mtx_);
                                               if(!traits_type::index(resp.body_stream(), p->c_)) {
                                                   throw(server_error(http_status_code::FORBIDDEN));
                                               }
                                               return true;
                                           } catch(server_error &e) {
                                               throw;
                                           } catch(...) {
                                               throw(server_error(http_status_code::BAD_REQUEST));
                                           }
                                       },
                                       // Read item
                                       get_(item_) >> [p](server::request &req, server::response &resp, key_type &&k) {
                                           try {
                                               fibio::shared_lock<typename traits::resource_lock<const Collection>::type> lk(p->mtx_);
                                               if(!traits_type::get(resp.body_stream(), p->c_, k)) {
                                                   throw(server_error(http_status_code::NOT_FOUND));
                                               }
                                               return true;
                                           } catch(server_error &e) {
                                               throw;
                                           } catch(...) {
                                               throw(server_error(http_status_code::BAD_REQUEST));
                                           }
                                       });
        }
        
        mutable typename traits::resource_lock<const Collection>::type mtx_;
        const std::string prefix_;
        const std::string item_;
        const Collection &c_;
    };

    /// Singleton resource
    template<typename T>
    routing_table resource(const std::string &prefix, T &t) {
        static_assert(std::is_convertible<decltype(std::declval<std::istream&>() >> t), std::istream&>::value,
                      "Type must be able to read from std::istream");
        static_assert(std::is_convertible<decltype(std::declval<std::ostream&>() << t), std::ostream&>::value,
                      "Type must be able to write to std::ostream");
        return std::make_shared<resource_controller<T>>(prefix, t)->operator routing_table();
    }
    
    /// Singleton read only resource
    template<typename T>
    routing_table resource(const std::string &prefix, const T &t) {
        static_assert(std::is_convertible<decltype(std::declval<std::ostream&>() << t), std::ostream&>::value,
                      "Type must be able to write to std::ostream");
        return std::make_shared<const_resource_controller<T>>(prefix, t)->operator routing_table();
    }
    
    /// Resource collection without key generator, can only use PUT to create or update item with supplied key
    template<typename Collection>
    routing_table resources(const std::string &prefix, Collection &c) {
        typedef traits::collection_traits<Collection> traits_type;
        typedef typename traits_type::value_type element;
        static_assert(std::is_convertible<decltype(std::declval<std::istream&>()
                                                   >> std::declval<element&>()), std::istream&>::value,
                      "Element must be able to read from std::istream");
        static_assert(std::is_convertible<decltype(std::declval<std::ostream&>()
                                                   << std::declval<const element&>()), std::ostream&>::value,
                      "Element must be able to write to std::ostream");
        return std::make_shared<resources_controller<Collection, null_key_generator>>(prefix, c)->operator routing_table();
    }
    
    /// Resource collection with key generator, key will be generated when using POST to create new item
    /// Following function will be called to generate a new key
    /// key_type kg(Collection &)
    template<typename Collection, typename KeyGen>
    routing_table resources(const std::string &prefix, Collection &c, KeyGen kg) {
        typedef traits::collection_traits<Collection> traits_type;
        typedef typename traits_type::value_type element;
        static_assert(std::is_convertible<decltype(std::declval<std::istream&>()
                                                   >> std::declval<element&>()), std::istream&>::value,
                      "Element must be able to read from std::istream");
        static_assert(std::is_convertible<decltype(std::declval<std::ostream&>()
                                                   << std::declval<const element&>()), std::ostream&>::value,
                      "Element must be able to write to std::ostream");
        static_assert(std::is_convertible<decltype(kg(c)), typename resources_controller<Collection, KeyGen>::key_type>::value,
                      "KeyGen(c) must return key_type");
        return std::make_shared<resources_controller<Collection, KeyGen>>(prefix, c, kg)->operator routing_table();
    }
    
    /// Read only resource collection
    template<typename Collection>
    routing_table resources(const std::string &prefix, const Collection &c) {
        typedef traits::collection_traits<Collection> traits_type;
        typedef typename traits_type::value_type element;
        static_assert(std::is_convertible<decltype(std::declval<std::ostream&>()
                                                   << std::declval<const element&>()), std::ostream&>::value,
                      "Element must be able to write to std::ostream");
        return std::make_shared<const_resources_controller<Collection>>(prefix, c)->operator routing_table();
    }
}}  // End of namespace fibio::http

#endif
