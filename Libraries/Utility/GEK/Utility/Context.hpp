/// @file
/// @author Todd Zupan <toddzupan@gmail.com>
/// @version $Revision$
/// @section LICENSE
/// https://en.wikipedia.org/wiki/MIT_License
/// @section DESCRIPTION
/// Last Changed: $Date$
#pragma once

#include "GEK/Utility/Exceptions.hpp"
#include "GEK/Utility/String.hpp"
#include "GEK/Utility/FileSystem.hpp"
#include <functional>
#include <typeindex>
#include <memory>
#include <vector>

#define GEK_PREDECLARE(TYPE) struct TYPE; using TYPE##Ptr = std::shared_ptr<TYPE>;
#define GEK_INTERFACE(TYPE) struct TYPE; using TYPE##Ptr = std::shared_ptr<TYPE>; struct TYPE

namespace Gek
{
    GEK_PREDECLARE(ContextUser);

    GEK_INTERFACE(Context)
    {
        GEK_ADD_EXCEPTION(DuplicateClass);
        GEK_ADD_EXCEPTION(InvalidPlugin);
        GEK_ADD_EXCEPTION(ClassNotFound);

        static ContextPtr Create(const wchar_t *rootPath, const std::vector<String> &searchPathList);

        virtual const wchar_t *getRootPath(void) const = 0;

        virtual ContextUserPtr createBaseClass(const wchar_t *className, void *typelessArguments, std::vector<std::type_index> &argumentTypes) const = 0;

        template <typename TYPE, typename... PARAMETERS>
        std::shared_ptr<TYPE> createClass(const wchar_t *className, PARAMETERS... arguments) const
        {
            std::tuple<PARAMETERS...> packedArguments(arguments...);
            std::vector<std::type_index> argumentTypes = { typeid(PARAMETERS)... };
            ContextUserPtr baseClass = createBaseClass(className, static_cast<void *>(&packedArguments), argumentTypes);
            return std::dynamic_pointer_cast<TYPE>(baseClass);
        }

        virtual void listTypes(const wchar_t *typeName, std::function<void(const wchar_t *)> onType) const = 0;

        template <typename... PARAMETERS>
        String getFileName(PARAMETERS... nameList)
        {
            return FileSystem::GetFileName(getRootPath(), { nameList... });
        }

    };
}; // namespace Gek
