#include "GEK\Context\ObservableMixin.h"

namespace Gek
{
    namespace Observable
    {
        Mixin::~Mixin(void)
        {
        }

        void Mixin::sendEvent(const BaseEvent &event) const
        {
            for (auto &observer : observerList)
            {
                event(observer);
            }
        }

        HRESULT Mixin::addObserver(IUnknown *observableUnknown, Observer::Interface *observer)
        {
            HRESULT resultValue = E_FAIL;
            Interface *observable = dynamic_cast<Interface *>(observableUnknown);
            if (observable)
            {
                resultValue = observable->addObserver(observer);
            }

            return resultValue;
        }

        HRESULT Mixin::removeObserver(IUnknown *observableUnknown, Observer::Interface *observer)
        {
            HRESULT resultValue = E_FAIL;
            Interface *observable = dynamic_cast<Interface *>(observableUnknown);
            if (observable)
            {
                resultValue = observable->removeObserver(observer);
            }

            return resultValue;
        }

        // Interface
        STDMETHODIMP Mixin::addObserver(Observer::Interface *observer)
        {
            HRESULT resultValue = E_FAIL;
            auto observerIterator = observerList.find(observer);
            if (observerIterator == observerList.end())
            {
                observerList.insert(observer);
                resultValue = S_OK;
            }

            return resultValue;
        }

        STDMETHODIMP Mixin::removeObserver(Observer::Interface *observer)
        {
            HRESULT resultValue = E_FAIL;
            auto observerIterator = observerList.find(observer);
            if (observerIterator != observerList.end())
            {
                observerList.unsafe_erase(observerIterator);
                resultValue = S_OK;
            }

            return resultValue;
        }
    }; // namespace Observable
}; // namespace Gek
