#pragma once

#include <Windows.h>
#include <atlbase.h>
#include <atlstr.h>
#include <unordered_map>

namespace Gek
{
    interface Context;

    interface ContextUser
    {
        void registerContext(Context *context);
    };
}; // namespace Gek
