#pragma once

#include "GEK\Utility\Exceptions.h"
#include "GEK\Utility\String.h"
#include <functional>
#include <memory>
#include <vector>

#define GEK_PREDECLARE(TYPE) struct TYPE; using TYPE##Ptr = std::shared_ptr<TYPE>;
#define GEK_INTERFACE(TYPE) struct TYPE; using TYPE##Ptr = std::shared_ptr<TYPE>; struct TYPE

namespace Gek
{
    GEK_PREDECLARE(ContextUser);

    GEK_INTERFACE(Context)
    {
        GEK_START_EXCEPTIONS();
        GEK_ADD_EXCEPTION(DuplicateClass);
        GEK_ADD_EXCEPTION(InvalidPlugin);
        GEK_ADD_EXCEPTION(ClassNotFound);

        static ContextPtr create(const std::vector<String> &searchPathList);

        virtual ContextUserPtr createBaseClass(const wchar_t *className, void *parameters) const = 0;

        template <typename TYPE, typename... PARAMETERS>
        std::shared_ptr<TYPE> createClass(const wchar_t *className, PARAMETERS... arguments) const
        {
            std::tuple<PARAMETERS...> packedArguments(arguments...);
            ContextUserPtr baseClass = createBaseClass(className, static_cast<void *>(&packedArguments));
            return std::dynamic_pointer_cast<TYPE>(baseClass);
        }

        virtual void listTypes(const wchar_t *typeName, std::function<void(const wchar_t *)> onType) const = 0;
    };
}; // namespace Gek
