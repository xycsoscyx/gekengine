#pragma once

#include "Common.h"
#include "UnknownMixin.h"
#include "UserInterface.h"

namespace Gek
{
    namespace Context
    {
        namespace User
        {
            class Mixin : virtual public Unknown::Mixin
                , virtual public Interface
            {
            private:
                Context::Interface *context;

            public:
                Mixin(void);
                virtual ~Mixin(void);

                DECLARE_UNKNOWN(Mixin);

                // Interface
                STDMETHOD_(void, registerContext)                   (THIS_ Context::Interface *context);

                // Utilities
                Context::Interface * getContext(void);
                const Context::Interface * getContext(void) const;
            };
        }; // namespace User
    }; // namespace Context
};
