#include "GEK\Context\UnknownMixin.h"

namespace Gek
{
    UnknownMixin::UnknownMixin(void)
        : referenceCount(0)
    {
    }

    UnknownMixin::~UnknownMixin(void)
    {
    }

    // IUnknown
    STDMETHODIMP_(ULONG) UnknownMixin::AddRef(void)
    {
        return InterlockedIncrement(&referenceCount);
    }

    STDMETHODIMP_(ULONG) UnknownMixin::Release(void)
    {
        LONG currentReferenceCount = InterlockedDecrement(&referenceCount);
        if (currentReferenceCount == 0)
        {
            delete this;
        }

        return currentReferenceCount;
    }

    STDMETHODIMP UnknownMixin::QueryInterface(REFIID interfaceType, LPVOID FAR *returnObject)
    {
        REQUIRE_RETURN(returnObject, E_INVALIDARG);

        HRESULT resultValue = E_INVALIDARG;
        if (IsEqualIID(IID_IUnknown, interfaceType))
        {
            AddRef();
            (*returnObject) = dynamic_cast<IUnknown *>(this);
            _ASSERTE(*returnObject);
            resultValue = S_OK;
        }

        return resultValue;
    }

    // Utility
    IUnknown * UnknownMixin::getUnknown(void)
    {
        return dynamic_cast<IUnknown *>(this);
    }

    const IUnknown * UnknownMixin::getUnknown(void) const
    {
        return dynamic_cast<const IUnknown *>(this);
    }
}; // namespace Gek
