#pragma once

#include "Common.h"
#include "UnknownMixin.h"
#include "UserInterface.h"

namespace Gek
{
    namespace Context
    {
        class UserMixin : virtual public UnknownMixin
                       , virtual public UserInterface
        {
        private:
            Interface *context;

        public:
            UserMixin(void);
            virtual ~UserMixin(void);

            DECLARE_UNKNOWN(UserMixin);

            // UserInterface
            STDMETHOD_(void, registerContext)                   (THIS_ Interface *context);
            STDMETHOD_(Interface *, getContext)                 (THIS);
            STDMETHOD_(const Interface *, getContext)           (THIS) const;
        };
    }; // namespace Context
};
