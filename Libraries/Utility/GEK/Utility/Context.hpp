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
        static ContextPtr Create(const std::vector<FileSystem::Path> &pluginSearchList);

        virtual ~Context(void) = default;

		virtual void setCachePath(FileSystem::Path const &path) = 0;
		virtual FileSystem::Path getCachePath(FileSystem::Path const &path) = 0;

        virtual void addDataPath(FileSystem::Path const &path) = 0;
        virtual FileSystem::Path findDataPath(FileSystem::Path const &path) const = 0;
		virtual void findDataFiles(FileSystem::Path const &path, std::function<bool(FileSystem::Path const &filePath)> onFileFound) const = 0;

        virtual ContextUserPtr createBaseClass(std::string_view className, void *typelessArguments, std::vector<std::type_index> &argumentTypes) const = 0;

        template <typename TYPE, typename... PARAMETERS>
        std::unique_ptr<TYPE> createClass(std::string_view className, PARAMETERS... arguments) const
        {
            std::tuple<PARAMETERS...> packedArguments(arguments...);
            std::vector<std::type_index> argumentTypes = { typeid(PARAMETERS)... };
            ContextUserPtr baseClass = createBaseClass(className, static_cast<void *>(&packedArguments), argumentTypes);
            if (!baseClass)
            {
                return nullptr;
            }

            auto derivedClass = dynamic_cast<TYPE *>(baseClass.get());
            if (!derivedClass)
            {
                LockedWrite{ std::cerr } << "Unable to cast " << className << " from base context user to requested class of " << typeid(TYPE).name();
                return nullptr;
            }

            std::unique_ptr<TYPE> result(derivedClass);
            baseClass.release();
            return result;
        }

        virtual void listTypes(std::string_view typeName, std::function<void(std::string_view )> onType) const = 0;
    };
}; // namespace Gek
