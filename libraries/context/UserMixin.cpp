#include "GEK\Context\UserMixin.h"

namespace Gek
{
    namespace Context
    {
        namespace User
        {
            Mixin::Mixin(void)
                : context(nullptr)
            {
            }

            Mixin::~Mixin(void)
            {
            }

            // IUnknown
            BEGIN_INTERFACE_LIST(Mixin)
                INTERFACE_LIST_ENTRY_COM(Context::User::Interface)
            END_INTERFACE_LIST_UNKNOWN

            // User::Interface
            STDMETHODIMP_(void) Mixin::registerContext(Context::Interface *context)
            {
                this->context = context;
            }

            // Utility
            Context::Interface * Mixin::getContext(void)
            {
                REQUIRE_RETURN(context, nullptr);
                return context;
            }

            const Context::Interface * Mixin::getContext(void) const
            {
                REQUIRE_RETURN(context, nullptr);
                return context;
            }
        }; // namespace User
    }; // namespace Context
}; // namespace Gek
