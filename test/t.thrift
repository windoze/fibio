#!/usr/local/bin/thrift --gen cpp

namespace cpp Test

service Something {
  i32 mul10(1:i32 n)
}
