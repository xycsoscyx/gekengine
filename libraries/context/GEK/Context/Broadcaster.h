#pragma once

#include "GEK\Context\Context.h"
#include <concurrent_unordered_set.h>
#include <iostream>

namespace Gek
{
    template <typename LISTENER>
    class Broadcaster
    {
    private:
        concurrency::concurrent_unordered_set<LISTENER *> listenerSet;

    public:
        virtual ~Broadcaster(void)
        {
            GEK_REQUIRE(listenerSet.empty());
        }

        void addListener(LISTENER *listener)
        {
            listenerSet.insert(listener);
        }

        void removeListener(LISTENER *listener)
        {
            listenerSet.unsafe_erase(listener);
        }

        template <typename... PARAMETERS, typename... ARGUMENTS>
        void sendEvent(void(LISTENER::*function)(PARAMETERS...), ARGUMENTS&&... arguments) const
        {
            for (auto &listener : listenerSet)
            {
                (listener->*function)(std::forward<ARGUMENTS>(arguments)...);
            }
        }
    };
}; // namespace Gek