#include "GEK\Context\ObservableMixin.h"

namespace Gek
{
    ObservableMixin::~ObservableMixin(void)
    {
    }

    void ObservableMixin::sendEvent(const VirtualEvent<void> &event) const
    {
        for (auto &observer : observerList)
        {
            event(observer);
        }
    }

    HRESULT ObservableMixin::addObserver(IUnknown *observableUnknown, ObserverInterface *observer)
    {
        HRESULT resultValue = E_FAIL;
        ObservableInterface *observable = dynamic_cast<ObservableInterface *>(observableUnknown);
        if (observable)
        {
            resultValue = observable->addObserver(observer);
        }

        return resultValue;
    }

    HRESULT ObservableMixin::removeObserver(IUnknown *observableUnknown, ObserverInterface *observer)
    {
        HRESULT resultValue = E_FAIL;
        ObservableInterface *observable = dynamic_cast<ObservableInterface *>(observableUnknown);
        if (observable)
        {
            resultValue = observable->removeObserver(observer);
        }

        return resultValue;
    }

    // ObservableInterface
    STDMETHODIMP ObservableMixin::addObserver(ObserverInterface *observer)
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

    STDMETHODIMP ObservableMixin::removeObserver(ObserverInterface *observer)
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
}; // namespace Gek
