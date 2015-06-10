#pragma once

#include <atlbase.h>
#include <atlstr.h>
#include <functional>

namespace Gek
{
    typedef UINT32 Handle;
    const Handle InvalidHandle = 0;

    template <typename TYPE>
    struct Rectangle
    {
        TYPE left;
        TYPE top;
        TYPE right;
        TYPE bottom;
    };
}; // namespace Gek
