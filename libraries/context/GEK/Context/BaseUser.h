#pragma once

#include "Common.h"
#include "BaseUnknown.h"
#include "UserInterface.h"

namespace Gek
{
    namespace Context
    {
        class BaseUser : public BaseUnknown
                       , virtual public UserInterface
        {
        private:
            Interface *context;

        public:
            BaseUser(void);
            virtual ~BaseUser(void);

            DECLARE_UNKNOWN(BaseUser);

            // UserInterface
            STDMETHOD_(void, registerContext)                   (THIS_ Interface *context);
            STDMETHOD_(Interface *, getContext)                 (THIS);
            STDMETHOD_(const Interface *, getContext)           (THIS) const;
        };
    }; // namespace Context
};
