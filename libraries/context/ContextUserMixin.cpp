#include "GEK\Context\ContextUserMixin.h"
#include "GEK\Utility\Trace.h"

namespace Gek
{
    ContextUserMixin::ContextUserMixin(void)
        : context(nullptr)
    {
    }

    ContextUserMixin::~ContextUserMixin(void)
    {
    }

    // User
    void ContextUserMixin::registerContext(Context *context)
    {
        this->context = context;
    }

    // Utility
    Context * ContextUserMixin::getContext(void)
    {
        GEK_REQUIRE(context);
        return context;
    }

    const Context * ContextUserMixin::getContext(void) const
    {
        GEK_REQUIRE(context);
        return context;
    }
}; // namespace Gek
