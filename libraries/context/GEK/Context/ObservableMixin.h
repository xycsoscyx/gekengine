#pragma once

#include "GEK\Context\ObservableInterface.h"
#include <concurrent_unordered_set.h>
#include <atlbase.h>
#include <functional>

namespace Gek
{
    class ObservableMixin : virtual public ObservableInterface
    {
    public:
        template <typename RESULT>
        class VirtualEvent
        {
        public:
            virtual ~VirtualEvent(void)
            {
            }

            virtual RESULT operator () (ObserverInterface *observer) const = 0;
        };

        template <typename INTERFACE>
        class Event : public VirtualEvent<void>
        {
        private:
            const std::function<void(INTERFACE *)> &onEvent;

        public:
            Event(const std::function<void(INTERFACE *)> &onEvent) :
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

    private:
        concurrency::concurrent_unordered_set<ObserverInterface *> observerList;

    public:
        virtual ~ObservableMixin(void);

        void sendEvent(const VirtualEvent<void> &event) const;

        static HRESULT addObserver(IUnknown *observableUnknown, ObserverInterface *observer);
        static HRESULT removeObserver(IUnknown *observableUnknown, ObserverInterface *observer);

        // ObservableInterface
        STDMETHOD(addObserver)      (THIS_ ObserverInterface *observer);
        STDMETHOD(removeObserver)   (THIS_ ObserverInterface *observer);
    };
}; // namespace Gek
