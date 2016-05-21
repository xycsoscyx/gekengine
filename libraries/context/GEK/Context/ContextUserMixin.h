#pragma once

#include "COM.h"
#include "ContextUser.h"
#include "Context.h"

namespace Gek
{
    class ContextUserMixin
        : virtual public ContextUser
    {
    private:
        Context *context;

    public:
        ContextUserMixin(void);
        virtual ~ContextUserMixin(void);

        // Interface
        void registerContext(Context *context);

        // Utilities
        Context * getContext(void);
        const Context * getContext(void) const;
    };
}; // namespace GEK
