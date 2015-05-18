#pragma once

#include "Common.h"
#include "UserInterface.h"

namespace Gek
{
    namespace Context
    {
        class BaseUser : public UserInterface
        {
        private:
            ULONG referenceCount;
            Interface *context;

        public:
            BaseUser(void);
            virtual ~BaseUser(void);

            // IUnknown
            STDMETHOD_(ULONG, AddRef)                           (THIS);
            STDMETHOD_(ULONG, Release)                          (THIS);
            STDMETHOD(QueryInterface)                           (THIS_ REFIID interfaceType, LPVOID FAR *returnObject);

            // UserInterface
            STDMETHOD_(void, registerContext)                   (THIS_ Interface *context);
            STDMETHOD_(Interface *, getContext)                 (THIS);
            STDMETHOD_(const Interface *, getContext)           (THIS) const;
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
    }; // namespace Context
};
