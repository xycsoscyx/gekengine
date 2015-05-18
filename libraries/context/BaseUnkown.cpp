#include "GEK\Context\BaseUnknown.h"

namespace Gek
{
    BaseUnknown::BaseUnknown(void)
        : referenceCount(0)
    {
    }

    BaseUnknown::~BaseUnknown(void)
    {
    }

    // IUnknown
    STDMETHODIMP_(ULONG) BaseUnknown::AddRef(void)
    {
        return InterlockedIncrement(&referenceCount);
    }

    STDMETHODIMP_(ULONG) BaseUnknown::Release(void)
    {
        LONG currentReferenceCount = InterlockedDecrement(&referenceCount);
        if (currentReferenceCount == 0)
        {
            delete this;
        }

        return currentReferenceCount;
    }

    STDMETHODIMP BaseUnknown::QueryInterface(REFIID interfaceType, LPVOID FAR *returnObject)
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

    // BaseUnknown
    STDMETHODIMP_(IUnknown *) BaseUnknown::getUnknown(void)
    {
        return dynamic_cast<IUnknown *>(this);
    }

    STDMETHODIMP_(const IUnknown *) BaseUnknown::getUnknown(void) const
    {
        return dynamic_cast<const IUnknown *>(this);
    }
}; // namespace Gek
