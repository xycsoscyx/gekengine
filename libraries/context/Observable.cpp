#include "GEK\Context\Observable.h"

namespace Gek
{
    Observable::~Observable(void)
    {
    }

    void Observable::sendEvent(const BaseEvent<void> &event)
    {
        for (auto &observer : observerList)
        {
            event(observer);
        }
    }

    HRESULT Observable::checkEvent(const BaseEvent<HRESULT> &event)
    {
        HRESULT resultValue = S_OK;
        for (auto &observer : observerList)
        {
            resultValue = event(observer);
            if (FAILED(resultValue))
            {
                break;
            }
        }

        return resultValue;
    }

    HRESULT Observable::addObserver(IUnknown *observableBase, ObserverInterface *observer)
    {
        HRESULT resultValue = E_FAIL;
        ObservableInterface *observable = dynamic_cast<ObservableInterface *>(observableBase);
        if (observable)
        {
            resultValue = observable->addObserver(observer);
        }

        return resultValue;
    }

    HRESULT Observable::removeObserver(IUnknown *observableBase, ObserverInterface *observer)
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
    STDMETHODIMP Observable::addObserver(ObserverInterface *observer)
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

    STDMETHODIMP Observable::removeObserver(ObserverInterface *observer)
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
