#pragma once

#include "Common.h"
#include "UserInterface.h"

namespace Gek
{
    class BaseUnknown : public IUnknown
    {
    private:
        ULONG referenceCount;

    public:
        BaseUnknown(void);
        virtual ~BaseUnknown(void);

        DECLARE_UNKNOWN(BaseUnknown);

        // BaseUnknown
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
