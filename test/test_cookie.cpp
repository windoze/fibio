//
//  test_cookie.cpp
//  fibio
//
//  Created by Chen Xu on 14/10/24.
//  Copyright (c) 2014 0d0a.com. All rights reserved.
//

#include <iostream>
#include <fibio/http/common/common_types.hpp>

using namespace fibio::http::common;

void test() {
    {
        cookie c("lu=Rg3vHJZnehYLjVg7qi3bZjzg; Expires=Tue, 15-Jan-2013 21:47:38 GMT; Path=/; Domain=.example.com;");
        assert(!c.secure);
        assert(!c.http_only);
        assert(c.expired());
        // Not effective as this cookie is expired
        assert(!c.effective("http://www.example.com/"));
    }
    {
        cookie c("lu=Rg3vHJZnehYLjVg7qi3bZjzg; Expires=Sat, 15-Jan-2050 21:47:38 GMT; Path=/; Domain=.example.com;");
        assert(c.to_string()=="lu=Rg3vHJZnehYLjVg7qi3bZjzg; Domain=.example.com; Path=/; Expires=Sat, 15-Jan-2050 21:47:38 GMT");
        assert(!c.expired());
        assert(c.effective("http://example.com/"));
        assert(c.effective("http://www.example.com/"));
        assert(!c.effective("http://www.another-example.com/"));
    }
    {
        cookie c("lu=Rg3vHJZnehYLjVg7qi3bZjzg; Expires=Sat, 15-Jan-2050 21:47:38 GMT; Path=/test; Domain=.example.com;");
        assert(c.to_string()=="lu=Rg3vHJZnehYLjVg7qi3bZjzg; Domain=.example.com; Path=/test; Expires=Sat, 15-Jan-2050 21:47:38 GMT");
        assert(!c.expired());
        assert(c.effective("http://example.com/test"));
        assert(!c.effective("http://example.com/"));
        assert(c.effective("http://www.example.com/test/tes/te/t"));
        assert(c.effective("http://www.example.com/test/a/long/and/stupid/url?with=some&query=parameters#and_fragment"));
        assert(!c.effective("http://www.example.com/"));
        assert(!c.effective("http://www.another-example.com/"));
    }
    {
        cookie c("lu=Rg3vHJZnehYLjVg7qi3bZjzg; Expires=Sat, 15-Jan-2050 21:47:38 GMT; Domain=.example.com;");
        assert(c.to_string()=="lu=Rg3vHJZnehYLjVg7qi3bZjzg; Domain=.example.com; Expires=Sat, 15-Jan-2050 21:47:38 GMT");
        assert(!c.expired());
        assert(c.effective("http://example.com/"));
        assert(c.effective("http://example.com/test"));
        assert(c.effective("http://www.example.com/"));
        assert(c.effective("http://www.example.com/test/tes/te/t"));
        assert(c.effective("http://www.example.com/a/long/and/stupid/url?with=some&query=parameters#and_fragment"));
        assert(c.effective("http://someone.www.example.com/test/tes/te/t"));
        assert(!c.effective("http://www.another-example.com/"));
    }
    {
        cookie c("lu=Rg3vHJZnehYLjVg7qi3bZjzg; Expires=Sat, 15-Jan-2050 21:47:38 GMT; Domain=.example.com; Secure");
        assert(c.to_string()=="lu=Rg3vHJZnehYLjVg7qi3bZjzg; Domain=.example.com; Expires=Sat, 15-Jan-2050 21:47:38 GMT; Secure");
        assert(!c.expired());
        assert(c.secure);
        assert(!c.effective("http://example.com/"));
        assert(!c.effective("http://example.com/test"));
        assert(!c.effective("http://www.example.com/"));
        assert(!c.effective("http://www.example.com/test/tes/te/t"));
        assert(!c.effective("http://www.example.com/a/long/and/stupid/url?with=some&query=parameters#and_fragment"));
        assert(c.effective("https://example.com/"));
        assert(c.effective("https://example.com/test"));
        assert(c.effective("https://www.example.com/"));
        assert(c.effective("https://www.example.com/test/tes/te/t"));
        assert(c.effective("https://www.example.com/a/long/and/stupid/url?with=some&query=parameters#and_fragment"));
        assert(!c.effective("http://www.another-example.com/"));
    }
}

int main() {
    test();
    return 0;
}