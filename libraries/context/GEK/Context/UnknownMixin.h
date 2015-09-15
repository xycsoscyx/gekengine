#pragma once

#include "Common.h"
#include "UserInterface.h"

namespace Gek
{
    class UnknownMixin : virtual public IUnknown
    {
    private:
        ULONG referenceCount;

    public:
        UnknownMixin(void);
        virtual ~UnknownMixin(void);

        DECLARE_UNKNOWN(UnknownMixin);

        // UnknownMixin
        STDMETHOD_(IUnknown *, getUnknown)                  (THIS);
        STDMETHOD_(const IUnknown *, getUnknown)            (THIS) const;

        template <typename CLASS>
        CLASS *getClass(void)
        {
            return dynamic_cast<CLASS *>(getUnknown());
        }

        template <typename CLASS>
        const CLASS *getClass(void) const
        {
            return dynamic_cast<const CLASS *>(getUnknown());
        }
    };
};
