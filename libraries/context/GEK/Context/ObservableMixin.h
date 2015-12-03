#pragma once

#include "GEK\Context\Observable.h"
#include <concurrent_unordered_set.h>
#include <atlbase.h>
#include <functional>

namespace Gek
{
    class ObservableMixin : virtual public Observable
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
        concurrency::concurrent_unordered_set<Observer *> observerList;

    public:
        virtual ~ObservableMixin(void);

        void sendEvent(const BaseEvent &event) const;

        static HRESULT addObserver(IUnknown *observableUnknown, Observer *observer);
        static HRESULT removeObserver(IUnknown *observableUnknown, Observer *observer);

        // ObservableInterface
        STDMETHOD(addObserver)      (THIS_ Observer *observer);
        STDMETHOD(removeObserver)   (THIS_ Observer *observer);
    };
}; // namespace Gek
