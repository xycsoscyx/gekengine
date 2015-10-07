#pragma once

#include "GEK\Context\ObservableInterface.h"
#include <concurrent_unordered_set.h>
#include <atlbase.h>
#include <functional>

namespace Gek
{
    namespace Observable
    {
        class Mixin : virtual public Interface
        {
        public:
            template <typename RESULT>
            class VirtualEvent
            {
            public:
                virtual ~VirtualEvent(void)
                {
                }

                virtual RESULT operator () (Observer::Interface *observer) const = 0;
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

                virtual void operator () (Observer::Interface *observer) const
                {
                    CComQIPtr<INTERFACE> eventHandler(observer);
                    if (eventHandler)
                    {
                        onEvent(eventHandler);
                    }
                }
            };

        private:
            concurrency::concurrent_unordered_set<Observer::Interface *> observerList;

        public:
            virtual ~Mixin(void);

            void sendEvent(const VirtualEvent<void> &event) const;

            static HRESULT addObserver(IUnknown *observableUnknown, Observer::Interface *observer);
            static HRESULT removeObserver(IUnknown *observableUnknown, Observer::Interface *observer);

            // ObservableInterface
            STDMETHOD(addObserver)      (THIS_ Observer::Interface *observer);
            STDMETHOD(removeObserver)   (THIS_ Observer::Interface *observer);
        };
    }; // namespace Observable
}; // namespace Gek
