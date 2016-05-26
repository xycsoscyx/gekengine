#pragma once

#include "GEK\Context\Context.h"

namespace Gek
{
    GEK_INTERFACE(Engine)
    {
        virtual LRESULT windowEvent(UINT32 message, WPARAM wParam, LPARAM lParam) = 0;

        virtual bool update(void) = 0;
    };
}; // namespace Gek
