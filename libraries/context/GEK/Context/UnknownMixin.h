#pragma once

#include "COM.h"

namespace Gek
{
    class UnknownMixin
        : virtual public IUnknown
    {
    private:
        ULONG referenceCount;

    public:
        UnknownMixin(void);
        virtual ~UnknownMixin(void);

        DECLARE_UNKNOWN(UnknownMixin);

        // Utilities
        IUnknown * getUnknown(void);
        const IUnknown * getUnknown(void) const;

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
