#include "GEK/Utility/Exceptions.hpp"
#include "GEK/Utility/String.hpp"
#include "GEK/Utility/FileSystem.hpp"
#include "GEK/Utility/Context.hpp"
#include "GEK/Utility/ContextUser.hpp"
#include <unordered_map>
#include <Windows.h>

namespace Gek
{
    class ContextImplementation
        : public Context
    {
    private:
        FileSystem::Path rootPath;
        std::vector<HMODULE> moduleList;
        std::unordered_map<std::string, std::function<ContextUserPtr(Context *, void *, std::vector<std::type_index> &)>> classMap;
        std::unordered_multimap<std::string, std::string> typeMap;

    public:
        ContextImplementation(FileSystem::Path const &rootPath, std::vector<FileSystem::Path> searchPathList)
            : rootPath(rootPath)
        {
            SetCurrentDirectoryW(rootPath.native().c_str());
            
            searchPathList.push_back(rootPath);
            for (const auto &searchPath : searchPathList)
            {
                FileSystem::Find(searchPath, [&](FileSystem::Path const &filePath) -> bool
                {
					if (filePath.isFile() && String::GetLower(filePath.getExtension()) == ".dll"s)
					{
						HMODULE module = LoadLibrary(filePath.native().c_str());
						if (module)
						{
							InitializePlugin initializePlugin = (InitializePlugin)GetProcAddress(module, "initializePlugin");
							if (initializePlugin)
							{
								initializePlugin([this](std::string const &className, std::function<ContextUserPtr(Context *, void *, std::vector<std::type_index> &)> creator) -> void
								{
									if (classMap.count(className) == 0)
									{
										classMap[className] = creator;
									}
									else
									{
                                        throw DuplicateClass("Duplicate class found in plugin library");
									}
								}, [this](std::string const &typeName, std::string const &className) -> void
								{
									typeMap.insert(std::make_pair(typeName, className));
								});

								moduleList.push_back(module);
							}
							else
							{
								FreeLibrary(module);
							}
						}
						else
						{
							throw InvalidPlugin("Unable to load plugin");
						}
					}

                    return true;
                });
            }
        }

        ~ContextImplementation(void)
        {
            typeMap.clear();
            classMap.clear();
            for (const auto &module : moduleList)
            {
                FreeLibrary(module);
            }
        }

        // Context
        FileSystem::Path const &getRootPath(void) const
        {
            return rootPath;
        }

        ContextUserPtr createBaseClass(std::string const &className, void *typelessArguments, std::vector<std::type_index> &argumentTypes) const
        {
            auto classSearch = classMap.find(className);
            if (classSearch == std::end(classMap))
            {
                throw ClassNotFound("Unable to locate class in plugin library");
            }

            return (*classSearch).second((Context *)this, typelessArguments, argumentTypes);
        }

        void listTypes(std::string const &typeName, std::function<void(std::string const &)> onType) const
        {
            GEK_REQUIRE(onType);

            auto typeRange = typeMap.equal_range(typeName);
            for (auto typeSearch = typeRange.first; typeSearch != typeRange.second; ++typeSearch)
            {
                onType(typeSearch->second);
            }
        }
    };

    ContextPtr Context::Create(FileSystem::Path const &rootPath, const std::vector<FileSystem::Path> &searchPathList)
    {
        return std::make_unique<ContextImplementation>(rootPath, searchPathList);
    }
}; // namespace Gek
