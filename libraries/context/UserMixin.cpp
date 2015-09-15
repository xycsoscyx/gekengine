#include "GEK\Context\UserMixin.h"

namespace Gek
{
    namespace Context
    {
        UserMixin::UserMixin(void)
            : context(nullptr)
        {
        }

        UserMixin::~UserMixin(void)
        {
        }

        // IUnknown
        BEGIN_INTERFACE_LIST(UserMixin)
            INTERFACE_LIST_ENTRY_COM(UserInterface)
        END_INTERFACE_LIST_UNKNOWN

        // UserInterface
        STDMETHODIMP_(void) UserMixin::registerContext(Interface *context)
        {
            this->context = context;
        }

        STDMETHODIMP_(Interface *) UserMixin::getContext(void)
        {
            REQUIRE_RETURN(context, nullptr);
            return context;
        }

        STDMETHODIMP_(const Interface *) UserMixin::getContext(void) const
        {
            REQUIRE_RETURN(context, nullptr);
            return context;
        }
    }; // namespace Context
}; // namespace Gek
