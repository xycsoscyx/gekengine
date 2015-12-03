#include "GEK\Context\ObservableMixin.h"

namespace Gek
{
    ObservableMixin::~ObservableMixin(void)
    {
    }

    void ObservableMixin::sendEvent(const BaseEvent &event) const
    {
        for (auto &observer : observerList)
        {
            event(observer);
        }
    }

    HRESULT ObservableMixin::addObserver(IUnknown *observableUnknown, Observer *observer)
    {
        HRESULT resultValue = E_FAIL;
        Observable *observable = dynamic_cast<Observable *>(observableUnknown);
        if (observable)
        {
            resultValue = observable->addObserver(observer);
        }

        return resultValue;
    }

    HRESULT ObservableMixin::removeObserver(IUnknown *observableUnknown, Observer *observer)
    {
        HRESULT resultValue = E_FAIL;
        Observable *observable = dynamic_cast<Observable *>(observableUnknown);
        if (observable)
        {
            resultValue = observable->removeObserver(observer);
        }

        return resultValue;
    }

    // Interface
    STDMETHODIMP ObservableMixin::addObserver(Observer *observer)
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

    STDMETHODIMP ObservableMixin::removeObserver(Observer *observer)
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
