#include "GEK\Context\ContextUser.h"

namespace Gek
{
    ContextUser::ContextUser(void)
        : referenceCount(0)
        , context(nullptr)
    {
    }

    ContextUser::~ContextUser(void)
    {
    }

    // IUnknown
    STDMETHODIMP_(ULONG) ContextUser::AddRef(void)
    {
        return InterlockedIncrement(&referenceCount);
    }

    STDMETHODIMP_(ULONG) ContextUser::Release(void)
    {
        LONG nRefCount = InterlockedDecrement(&referenceCount);
        if (nRefCount == 0)
        {
            delete this;
        }

        return nRefCount;
    }

    STDMETHODIMP ContextUser::QueryInterface(REFIID interfaceID, LPVOID FAR *object)
    {
        REQUIRE_RETURN(object, E_INVALIDARG);

        HRESULT returnValue = E_INVALIDARG;
        if (IsEqualIID(IID_IUnknown, interfaceID))
        {
            AddRef();
            (*object) = dynamic_cast<IUnknown *>(this);
            _ASSERTE(*object);
            returnValue = S_OK;
        }
        else if (IsEqualIID(__uuidof(ContextUserInterface), interfaceID))
        {
            AddRef();
            (*object) = dynamic_cast<ContextUserInterface *>(this);
            _ASSERTE(*object);
            returnValue = S_OK;
        }

        return returnValue;
    }

    // ContextUserInterface
    STDMETHODIMP_(void) ContextUser::registerContext(ContextInterface *context)
    {
        this->context = context;
    }

    STDMETHODIMP_(ContextInterface *) ContextUser::getContext(void)
    {
        REQUIRE_RETURN(context, nullptr);
        return context;
    }

    STDMETHODIMP_(const ContextInterface *) ContextUser::getContext(void) const
    {
        REQUIRE_RETURN(context, nullptr);
        return context;
    }

    STDMETHODIMP_(IUnknown *) ContextUser::getUnknown(void)
    {
        return dynamic_cast<IUnknown *>(this);
    }

    STDMETHODIMP_(const IUnknown *) ContextUser::getUnknown(void) const
    {
        return dynamic_cast<const IUnknown *>(this);
    }
}; // namespace Gek
