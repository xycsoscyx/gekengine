#pragma once

#include "GEK\Context\ObservableInterface.h"
#include <concurrent_unordered_set.h>
#include <atlbase.h>
#include <functional>

namespace Gek
{
    class BaseObservable : public ObservableInterface
    {
    public:
        template <typename RESULT>
        class BaseEvent
        {
        public:
            virtual ~BaseEvent(void)
            {
            }

            virtual RESULT operator () (ObserverInterface *observer) const = 0;
        };

        template <typename INTERFACE>
        class Event : public BaseEvent<void>
        {
        private:
            std::function<void(INTERFACE *)> onEvent;

        public:
            Event(std::function<void(INTERFACE *)> const &onEvent) :
                onEvent(onEvent)
            {
            }

            virtual void operator () (ObserverInterface *observer) const
            {
                CComQIPtr<INTERFACE> eventHandler(observer);
                if (eventHandler)
                {
                    onEvent(eventHandler);
                }
            }
        };

        template <typename INTERFACE>
        class Check : public BaseEvent<HRESULT>
        {
        private:
            std::function<HRESULT(INTERFACE *)> onEvent;

        public:
            Check(std::function<HRESULT(INTERFACE *)> const &onEvent) :
                onEvent(onEvent)
            {
            }

            virtual HRESULT operator () (ObserverInterface *observer) const
            {
                CComQIPtr<INTERFACE> eventHandler(observer);
                if (eventHandler)
                {
                    return onEvent(eventHandler);
                }

                return E_FAIL;
            }
        };

    private:
        concurrency::concurrent_unordered_set<ObserverInterface *> observerList;

    public:
        virtual ~BaseObservable(void);

        void sendEvent(const BaseEvent<void> &event);
        HRESULT checkEvent(const BaseEvent<HRESULT> &event);

        static HRESULT addObserver(IUnknown *observableBase, ObserverInterface *observer);
        static HRESULT removeObserver(IUnknown *observableBase, ObserverInterface *observer);

        // ObservableInterface
        STDMETHOD(addObserver)      (THIS_ ObserverInterface *observer);
        STDMETHOD(removeObserver)   (THIS_ ObserverInterface *observer);
    };
}; // namespace Gek
