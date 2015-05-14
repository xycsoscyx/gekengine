#pragma once

#include <windows.h>
#include <atlbase.h>
#include <atlstr.h>
#include <functional>

#pragma warning(disable:4251)
#pragma warning(disable:4996)

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
