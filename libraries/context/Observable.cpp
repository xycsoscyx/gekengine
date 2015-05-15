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
        HRESULT returnValue = S_OK;
        for (auto &observer : observerList)
        {
            returnValue = event(observer);
            if (FAILED(returnValue))
            {
                break;
            }
        }

        return returnValue;
    }

    HRESULT Observable::addObserver(IUnknown *observableBase, ObserverInterface *observer)
    {
        HRESULT returnValue = E_FAIL;
        ObservableInterface *observable = dynamic_cast<ObservableInterface *>(observableBase);
        if (observable)
        {
            returnValue = observable->addObserver(observer);
        }

        return returnValue;
    }

    HRESULT Observable::removeObserver(IUnknown *observableBase, ObserverInterface *observer)
    {
        HRESULT returnValue = E_FAIL;
        ObservableInterface *observable = dynamic_cast<ObservableInterface *>(observableBase);
        if (observable)
        {
            returnValue = observable->removeObserver(observer);
        }

        return returnValue;
    }

    // ObservableInterface
    STDMETHODIMP Observable::addObserver(ObserverInterface *observer)
    {
        HRESULT returnValue = E_FAIL;
        auto observerIterator = observerList.find(observer);
        if (observerIterator == observerList.end())
        {
            observerList.insert(observer);
            returnValue = S_OK;
        }

        return returnValue;
    }

    STDMETHODIMP Observable::removeObserver(ObserverInterface *observer)
    {
        HRESULT returnValue = E_FAIL;
        auto observerIterator = observerList.find(observer);
        if (observerIterator != observerList.end())
        {
            observerList.unsafe_erase(observerIterator);
            returnValue = S_OK;
        }

        return returnValue;
    }
}; // namespace Gek
