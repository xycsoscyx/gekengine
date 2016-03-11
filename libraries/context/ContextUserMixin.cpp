#include "GEK\Context\ContextUserMixin.h"

namespace Gek
{
    ContextUserMixin::ContextUserMixin(void)
        : context(nullptr)
    {
    }

    ContextUserMixin::~ContextUserMixin(void)
    {
    }

    // IUnknown
    BEGIN_INTERFACE_LIST(ContextUserMixin)
        INTERFACE_LIST_ENTRY_COM(ContextUser)
    END_INTERFACE_LIST_UNKNOWN

    // User
    STDMETHODIMP_(void) ContextUserMixin::registerContext(Context *context)
    {
        this->context = context;
    }

    // Utility
    Context * ContextUserMixin::getContext(void)
    {
        GEK_REQUIRE_RETURN(context, nullptr);
        return context;
    }

    const Context * ContextUserMixin::getContext(void) const
    {
        GEK_REQUIRE_RETURN(context, nullptr);
        return context;
    }
}; // namespace Gek
