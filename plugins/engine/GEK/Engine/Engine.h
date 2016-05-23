#pragma once

#include <Windows.h>

namespace Gek
{
    interface Engine
    {
        void initialize(HWND window);

        LRESULT windowEvent(UINT32 message, WPARAM wParam, LPARAM lParam);

        bool update(void);
    };

    DECLARE_INTERFACE_IID(EngineRegistration, "D0361A83-6C91-440B-A383-155D9A2D2FC8");
}; // namespace Gek
