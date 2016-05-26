#pragma once

#include "GEK\Context\Observable.h"
#include <unordered_set>
#include <atlbase.h>
#include <functional>

namespace Gek
{
    template <typename TYPE>
    class ObservableMixin
        : public Observable
    {
    public:
        struct BaseEvent
        {
            virtual void operator () (Observer *observer) const = 0;
        };

        template <typename INTERFACE>
        struct Event
            : public BaseEvent
        {
        private:
            const std::function<void(INTERFACE *)> &onEvent;

        public:
            Event(const std::function<void(INTERFACE *)> &onEvent) :
                onEvent(onEvent)
            {
            }

            void operator () (Observer *observer) const
            {
                onEvent(reinterpret_cast<INTERFACE *>(observer));
            }
        };

    private:
        std::unordered_set<Observer *> observerList;

    public:
        virtual ~ObservableMixin(void)
        {
        }

        void sendEvent(const BaseEvent &event) const
        {
            for (auto &observer : observerList)
            {
                event(observer);
            }
        }

        // Observable
        void addObserver(Observer *observer)
        {
            auto observerIterator = observerList.find(observer);
            if (observerIterator == observerList.end())
            {
                observerList.insert(observer);
            }
        }

        void removeObserver(Observer *observer)
        {
            auto observerIterator = observerList.find(observer);
            if (observerIterator != observerList.end())
            {
                observerList.erase(observerIterator);
            }
        }
    };
}; // namespace Gek
