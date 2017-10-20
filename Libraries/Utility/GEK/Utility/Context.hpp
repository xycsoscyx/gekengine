/// @file
/// @author Todd Zupan <toddzupan@gmail.com>
/// @version $Revision$
/// @section LICENSE
/// https://en.wikipedia.org/wiki/MIT_License
/// @section DESCRIPTION
/// Last Changed: $Date$
#pragma once

#include "GEK/Utility/String.hpp"
#include "GEK/Utility/FileSystem.hpp"
#include "GEK/Utility/Profiler.hpp"
#include <functional>
#include <typeindex>
#include <iostream>
#include <memory>
#include <vector>

#define GEK_PREDECLARE(TYPE) struct TYPE; using TYPE##Ptr = std::unique_ptr<TYPE>;
#define GEK_INTERFACE(TYPE) struct TYPE; using TYPE##Ptr = std::unique_ptr<TYPE>; struct TYPE

namespace Gek
{
    GEK_PREDECLARE(ContextUser);

    GEK_INTERFACE(Context)
    {
        static ContextPtr Create(FileSystem::Path const &rootPath, const std::vector<FileSystem::Path> &searchPathList);

        virtual ~Context(void) = default;

        virtual FileSystem::Path const &getRootPath(void) const = 0;

        template <typename... PARAMETERS>
        FileSystem::Path getRootFileName(PARAMETERS&&... nameList)
        {
            return FileSystem::GetFileName(getRootPath(), nameList...);
        }

        virtual ContextUserPtr createBaseClass(std::string const &className, void *typelessArguments, std::vector<std::type_index> &argumentTypes) const = 0;

        template <typename TYPE, typename... PARAMETERS>
        std::unique_ptr<TYPE> createClass(std::string const &className, PARAMETERS... arguments) const
        {
            std::tuple<PARAMETERS...> packedArguments(arguments...);
            std::vector<std::type_index> argumentTypes = { typeid(PARAMETERS)... };
            ContextUserPtr baseClass = createBaseClass(className, static_cast<void *>(&packedArguments), argumentTypes);
            auto derivedClass = dynamic_cast<TYPE *>(baseClass.get());
            if (!derivedClass)
            {
                LockedWrite{ std::cerr } << String::Format("Unable to cast from base context user to requested class");
                return nullptr;
            }

            std::unique_ptr<TYPE> result(derivedClass);
            baseClass.release();
            return result;
        }

        virtual void listTypes(std::string const &typeName, std::function<void(std::string const &)> onType) const = 0;
    };
}; // namespace Gek
