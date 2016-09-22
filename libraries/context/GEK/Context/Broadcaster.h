#pragma once

#include "GEK\Context\Context.h"
#include <unordered_set>
#include <mutex>
#include <map>

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
        void sendShout(void(LISTENER::*function)(PARAMETERS...), ARGUMENTS&&... arguments) const
        {
            for (auto &listenerSearch : listenerSet)
            {
                (listenerSearch->*function)(std::forward<ARGUMENTS>(arguments)...);
            }
        }
    };

    template <typename STEP>
    class Sequencer
    {
    private:
        std::mutex mutex;
        std::multimap<uint32_t, STEP *> stepMap;

    public:
        virtual ~Sequencer(void)
        {
            std::lock_guard<std::mutex> lock(mutex);
            GEK_REQUIRE(stepMap.empty());
        }

        template<typename... ORDER>
        void addStep(STEP *step, ORDER... orderList)
        {
            std::lock_guard<std::mutex> lock(mutex);
            for (const auto &order : { orderList... })
            {
                stepMap.insert(std::make_pair(order, step));
            }
        }

        void removeStep(STEP *step)
        {
            std::lock_guard<std::mutex> lock(mutex);
            for (auto stepSearch = stepMap.begin(); stepSearch != stepMap.end(); )
            {
                auto currentStep = stepSearch++;
                if (currentStep->second == step)
                {
                    stepMap.erase(currentStep);
                }
            }
        }

        template <typename... PARAMETERS, typename... ARGUMENTS>
        void sendUpdate(void(STEP::*function)(uint32_t order, PARAMETERS...), ARGUMENTS&&... arguments) const
        {
            for (auto &stepSearch : stepMap)
            {
                (stepSearch.second->*function)(stepSearch.first, std::forward<ARGUMENTS>(arguments)...);
            }
        }
    };
}; // namespace Gek