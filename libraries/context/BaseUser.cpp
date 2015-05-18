#include "GEK\Context\BaseUser.h"

namespace Gek
{
    namespace Context
    {
        BaseUser::BaseUser(void)
            : context(nullptr)
        {
        }

        BaseUser::~BaseUser(void)
        {
        }

        // IUnknown
        BEGIN_INTERFACE_LIST(BaseUser)
            INTERFACE_LIST_ENTRY_COM(UserInterface)
        END_INTERFACE_LIST_UNKNOWN

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
    }; // namespace Context
}; // namespace Gek
