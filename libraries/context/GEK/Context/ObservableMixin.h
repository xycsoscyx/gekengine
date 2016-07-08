#pragma once

#include "GEK\Context\Observable.h"
#include <unordered_set>
#include <atlbase.h>
#include <functional>

namespace Gek
{
    template <class OBSERVER>
    class ObservableMixin
        : virtual public Observable
    {
    public:
        struct Event
        {
        private:
            const std::function<void(OBSERVER *)> &onEvent;

        public:
            Event(const std::function<void(OBSERVER *)> &onEvent) :
                onEvent(onEvent)
            {
            }

            void operator () (OBSERVER *observer) const
            {
                onEvent(observer);
            }
        };

    private:
        std::unordered_set<OBSERVER *> observerList;

    public:
        virtual ~ObservableMixin(void)
        {
        }

        // ObservableMixin
        void sendEvent(const Event &event) const
        {
            for (auto &observer : observerList)
            {
                event(observer);
            }
        }

        // Observable
        void addObserver(Observer *observer)
        {
            auto observerSearch = observerList.find(dynamic_cast<OBSERVER *>(observer));
            if (observerSearch == observerList.end())
            {
                observerList.insert(dynamic_cast<OBSERVER *>(observer));
            }
        }

        void removeObserver(Observer *observer)
        {
            auto observerSearch = observerList.find(dynamic_cast<OBSERVER *>(observer));
            if (observerSearch != observerList.end())
            {
                observerList.erase(observerSearch);
            }
        }
    };
}; // namespace Gek
