#include "GEK\Context\ObservableMixin.h"
#include "GEK\Utility\Trace.h"

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

    // Interface
    void ObservableMixin::addObserver(Observer *observer)
    {
        auto observerIterator = observerList.find(observer);
        if (observerIterator == observerList.end())
        {
            observerList.insert(observer);
        }
    }

    void ObservableMixin::removeObserver(Observer *observer)
    {
        auto observerIterator = observerList.find(observer);
        if (observerIterator != observerList.end())
        {
            observerList.erase(observerIterator);
        }
    }
}; // namespace Gek
