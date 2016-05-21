#pragma once

#include <Windows.h>

namespace Gek
{
    interface Observer;

    interface Observable
    {
        void addObserver(Observer *observer);
        void removeObserver(Observer *observer);
    };
}; // namespace Gek
