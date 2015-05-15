#pragma once

#include "Common.h"
#include "ContextUserInterface.h"
#include "ContextInterface.h"

namespace Gek
{
    class ContextUser : public ContextUserInterface
    {
    private:
        ULONG referenceCount;
        ContextInterface *context;

    public:
        ContextUser(void);
        virtual ~ContextUser(void);

        // IUnknown
        STDMETHOD_(ULONG, AddRef)                           (THIS);
        STDMETHOD_(ULONG, Release)                          (THIS);
        STDMETHOD(QueryInterface)                           (THIS_ REFIID interfaceType, LPVOID FAR *returnObject);

        // ContextUserInterface
        STDMETHOD_(void, registerContext)                   (THIS_ ContextInterface *context);
        STDMETHOD_(ContextInterface *, getContext)          (THIS);
        STDMETHOD_(const ContextInterface *, getContext)    (THIS) const;
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
