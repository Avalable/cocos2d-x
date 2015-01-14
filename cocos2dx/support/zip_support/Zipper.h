// Copyright 2007 Timo Bingmann <tb@panthema.net>
// Distributed under the Boost Software License, Version 1.0.
// (See http://www.boost.org/LICENSE_1_0.txt)

#include <string>
#include <stdexcept>
#include <iostream>
#include <iomanip>
#include <sstream>

#include <zlib.h>

namespace cocos2d
{
   class Zipper
    {
    public:
        static std::string compress_string(const std::string &str, int compressionlevel = Z_BEST_COMPRESSION);
        static std::string decompress_string(const std::string &str);
    };
}
