#include "GEK\Context\BaseObservable.h"

namespace Gek
{
    BaseObservable::~BaseObservable(void)
    {
    }

    void BaseObservable::sendEvent(const BaseEvent<void> &event) const
    {
        for (auto &observer : observerList)
        {
            event(observer);
        }
    }

    HRESULT BaseObservable::addObserver(IUnknown *observableBase, ObserverInterface *observer)
    {
        HRESULT resultValue = E_FAIL;
        ObservableInterface *observable = dynamic_cast<ObservableInterface *>(observableBase);
        if (observable)
        {
            resultValue = observable->addObserver(observer);
        }

        return resultValue;
    }

    HRESULT BaseObservable::removeObserver(IUnknown *observableBase, ObserverInterface *observer)
    {
        HRESULT resultValue = E_FAIL;
        ObservableInterface *observable = dynamic_cast<ObservableInterface *>(observableBase);
        if (observable)
        {
            resultValue = observable->removeObserver(observer);
        }

        return resultValue;
    }

    // ObservableInterface
    STDMETHODIMP BaseObservable::addObserver(ObserverInterface *observer)
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

    STDMETHODIMP BaseObservable::removeObserver(ObserverInterface *observer)
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
