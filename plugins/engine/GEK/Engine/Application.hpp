#pragma once

#include "GEK\Engine\Core.hpp"
#include <Windows.h>

namespace Gek
{
    GEK_INTERFACE(Application)
    {
        virtual LRESULT windowEvent(uint32_t message, WPARAM wParam, LPARAM lParam) = 0;

        virtual bool update(void) = 0;
    };
}; // namespace Gek
