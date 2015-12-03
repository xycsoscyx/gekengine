#pragma once

#include "Common.h"
#include "UnknownMixin.h"
#include "ContextUser.h"

namespace Gek
{
    class ContextUserMixin : virtual public UnknownMixin
        , virtual public ContextUser
    {
    private:
        Context *context;

    public:
        ContextUserMixin(void);
        virtual ~ContextUserMixin(void);

        DECLARE_UNKNOWN(ContextUserMixin);

        // Interface
        STDMETHOD_(void, registerContext)                   (THIS_ Context *context);

        // Utilities
        Context * getContext(void);
        const Context * getContext(void) const;
    };
}; // namespace GEK
