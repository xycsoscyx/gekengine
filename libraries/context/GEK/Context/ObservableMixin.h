#pragma once

#include "GEK\Context\Observable.h"
#include <unordered_set>
#include <atlbase.h>
#include <functional>

namespace Gek
{
    class ObservableMixin
        : virtual public Observable
    {
    public:
        struct BaseEvent
        {
            virtual void operator () (Observer *observer) const = 0;
        };

        template <typename INTERFACE>
        struct Event : public BaseEvent
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
                CComQIPtr<INTERFACE> eventHandler(observer);
                if (eventHandler)
                {
                    onEvent(eventHandler);
                }
            }
        };

    private:
        std::unordered_set<Observer *> observerList;

    public:
        virtual ~ObservableMixin(void);

        void sendEvent(const BaseEvent &event) const;

        static void addObserver(Observable *observable, Observer *observer);
        static void removeObserver(Observable *observable, Observer *observer);

        // ObservableInterface
        void addObserver(Observer *observer);
        void removeObserver(Observer *observer);
    };
}; // namespace Gek
