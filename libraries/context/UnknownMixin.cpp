#include "GEK\Context\UnknownMixin.h"

namespace Gek
{
    namespace Unknown
    {
        Mixin::Mixin(void)
            : referenceCount(0)
        {
        }

        Mixin::~Mixin(void)
        {
        }

        // IUnknown
        STDMETHODIMP_(ULONG) Mixin::AddRef(void)
        {
            return InterlockedIncrement(&referenceCount);
        }

        STDMETHODIMP_(ULONG) Mixin::Release(void)
        {
            LONG currentReferenceCount = InterlockedDecrement(&referenceCount);
            if (currentReferenceCount == 0)
            {
                delete this;
            }

            return currentReferenceCount;
        }

        STDMETHODIMP Mixin::QueryInterface(REFIID interfaceType, LPVOID FAR *returnObject)
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
        IUnknown * Mixin::getUnknown(void)
        {
            return dynamic_cast<IUnknown *>(this);
        }

        const IUnknown * Mixin::getUnknown(void) const
        {
            return dynamic_cast<const IUnknown *>(this);
        }
    }; // namespace Unknown
}; // namespace Gek
