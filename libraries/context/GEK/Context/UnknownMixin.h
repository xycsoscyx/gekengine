#pragma once

#include "Common.h"
#include "UserInterface.h"

namespace Gek
{
    namespace Unknown
    {
        class Mixin : virtual public IUnknown
        {
        private:
            ULONG referenceCount;

        public:
            Mixin(void);
            virtual ~Mixin(void);

            DECLARE_UNKNOWN(Mixin);

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
    }; // namespace Unknown
};
