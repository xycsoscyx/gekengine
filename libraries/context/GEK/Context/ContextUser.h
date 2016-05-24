#pragma once

#include "GEK\Context\Context.h"
#include <unordered_map>

namespace Gek
{
    GEK_PREDECLARE(Context);

    GEK_INTERFACE(ContextUser)
    {
    private:
        Context *context;

    public:
        ContextUser(Context *context)
            : context(context)
        {
        }

        virtual ~ContextUser(void) = default;

        Context * const getContext(void) const
        {
            return context;
        }
    };
}; // namespace Gek
