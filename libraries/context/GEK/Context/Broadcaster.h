#pragma once

#include "GEK\Context\Context.h"
#include <unordered_set>

namespace Gek
{
    GEK_PREDECLARE(Listener);

    template <typename LISTENER>
    class Broadcaster
    {
    private:
        std::unordered_set<Listener *> listenerSet;

    public:
        virtual ~Broadcaster(void)
        {
        }

        void addListener(LISTENER *listener)
        {
            auto listenerSearch = listenerSet.find(listener);
            if (listenerSearch == listenerSet.end())
            {
                listenerSet.insert(listener);
            }
        }

        void removeListener(LISTENER *listener)
        {
            auto listenerSearch = listenerSet.find(listener);
            if (listenerSearch != listenerSet.end())
            {
                listenerSet.erase(listenerSearch);
            }
        }

        template <typename... ARGUMENTS>
        void sendEvent(void(LISTENER::*function)(ARGUMENTS...), ARGUMENTS... arguments) const
        {
            for (auto &listener : listenerSet)
            {
                (listener->*function)(arguments...);
            }
        }
    };
}; // namespace Gek
