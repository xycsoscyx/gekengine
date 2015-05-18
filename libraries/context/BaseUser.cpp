#include "GEK\Context\BaseUser.h"

namespace Gek
{
    namespace Context
    {
        BaseUser::BaseUser(void)
            : referenceCount(0)
            , context(nullptr)
        {
        }

        BaseUser::~BaseUser(void)
        {
        }

        // IUnknown
        STDMETHODIMP_(ULONG) BaseUser::AddRef(void)
        {
            return InterlockedIncrement(&referenceCount);
        }

        STDMETHODIMP_(ULONG) BaseUser::Release(void)
        {
            LONG currentReferenceCount = InterlockedDecrement(&referenceCount);
            if (currentReferenceCount == 0)
            {
                delete this;
            }

            return currentReferenceCount;
        }

        STDMETHODIMP BaseUser::QueryInterface(REFIID interfaceType, LPVOID FAR *returnObject)
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
            else if (IsEqualIID(__uuidof(UserInterface), interfaceType))
            {
                AddRef();
                (*returnObject) = dynamic_cast<UserInterface *>(this);
                _ASSERTE(*returnObject);
                resultValue = S_OK;
            }

            return resultValue;
        }

        // UserInterface
        STDMETHODIMP_(void) BaseUser::registerContext(Interface *context)
        {
            this->context = context;
        }

        STDMETHODIMP_(Interface *) BaseUser::getContext(void)
        {
            REQUIRE_RETURN(context, nullptr);
            return context;
        }

        STDMETHODIMP_(const Interface *) BaseUser::getContext(void) const
        {
            REQUIRE_RETURN(context, nullptr);
            return context;
        }

        STDMETHODIMP_(IUnknown *) BaseUser::getUnknown(void)
        {
            return dynamic_cast<IUnknown *>(this);
        }

        STDMETHODIMP_(const IUnknown *) BaseUser::getUnknown(void) const
        {
            return dynamic_cast<const IUnknown *>(this);
        }
    }; // namespace Context
}; // namespace Gek
