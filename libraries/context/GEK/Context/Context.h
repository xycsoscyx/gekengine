#pragma once

#include "GEK\Context\Observer.h"
#include <functional>
#include <typeindex>

namespace Gek
{
    interface Context
    {
        static std::shared_ptr<Context> create(void);
    
        virtual void addSearchPath(LPCWSTR fileName);
        virtual void initialize(void);

        virtual std::shared_ptr<ContextUser> createInstance(REFCLSID className);
        virtual void createEachType(REFCLSID typeName, std::function<void(REFCLSID, std::shared_ptr<ContextUser>)> onCreateInstance);
    };
}; // namespace Gek
