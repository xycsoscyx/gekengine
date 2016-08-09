#pragma once

#include "GEK\Context\Context.h"
#include <unordered_set>
#include <map>
#include <mutex>

namespace Gek
{
    template <typename LISTENER>
    class Broadcaster
    {
    private:
        std::mutex mutex;
        std::unordered_set<LISTENER *> listenerSet;

    public:
        virtual ~Broadcaster(void)
        {
            std::lock_guard<std::mutex> lock(mutex);
            GEK_REQUIRE(listenerSet.empty());
        }

        void addListener(LISTENER *listener)
        {
            std::lock_guard<std::mutex> lock(mutex);
            listenerSet.insert(listener);
        }

        void removeListener(LISTENER *listener)
        {
            std::lock_guard<std::mutex> lock(mutex);
            listenerSet.erase(listener);
        }

        template <typename... PARAMETERS, typename... ARGUMENTS>
        void sendEvent(void(LISTENER::*function)(PARAMETERS...), ARGUMENTS&&... arguments) const
        {
            for (auto &listenerSearch : listenerSet)
            {
                (listenerSearch->*function)(std::forward<ARGUMENTS>(arguments)...);
            }
        }
    };

    template <typename LISTENER>
    class OrderedBroadcaster
    {
    private:
        std::mutex mutex;
        std::multimap<uint32_t, LISTENER *> listenerMap;

    public:
        virtual ~OrderedBroadcaster(void)
        {
            std::lock_guard<std::mutex> lock(mutex);
            GEK_REQUIRE(listenerMap.empty());
        }

        template<typename... ORDER>
        void addListener(LISTENER *listener, ORDER... orderList)
        {
            std::lock_guard<std::mutex> lock(mutex);
            for (const auto &order : { orderList... })
            {
                listenerMap.insert(std::make_pair(order, listener));
            }
        }

        void removeListener(LISTENER *listener)
        {
            std::lock_guard<std::mutex> lock(mutex);
            for (auto listenerSearch = listenerMap.begin(); listenerSearch != listenerMap.end(); )
            {
                auto current = listenerSearch++;
                if (current->second == listener)
                {
                    listenerMap.erase(current);
                }
            }
        }

        template <typename... PARAMETERS, typename... ARGUMENTS>
        void sendEvent(void(LISTENER::*function)(uint32_t order, PARAMETERS...), ARGUMENTS&&... arguments) const
        {
            for (auto &listenerSearch : listenerMap)
            {
                (listenerSearch.second->*function)(listenerSearch.first, std::forward<ARGUMENTS>(arguments)...);
            }
        }
    };
}; // namespace Gek