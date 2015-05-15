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
        LONG currentReferenceCount = InterlockedDecrement(&referenceCount);
        if (currentReferenceCount == 0)
        {
            delete this;
        }

        return currentReferenceCount;
    }

    STDMETHODIMP ContextUser::QueryInterface(REFIID interfaceType, LPVOID FAR *returnObject)
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
        else if (IsEqualIID(__uuidof(ContextUserInterface), interfaceType))
        {
            AddRef();
            (*returnObject) = dynamic_cast<ContextUserInterface *>(this);
            _ASSERTE(*returnObject);
            resultValue = S_OK;
        }

        return resultValue;
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
