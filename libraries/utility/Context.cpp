#include "GEK\Utility\Exceptions.hpp"
#include "GEK\Utility\String.hpp"
#include "GEK\Utility\FileSystem.hpp"
#include "GEK\Utility\Context.hpp"
#include "GEK\Utility\ContextUser.hpp"
#include <Windows.h>
#include <unordered_map>
#include <list>

namespace Gek
{
    class ContextImplementation
        : public Context
    {
    private:
        String rootPath;
        std::list<HMODULE> moduleList;
        std::unordered_map<String, std::function<ContextUserPtr(Context *, void *, std::vector<std::type_index> &)>> classMap;
        std::unordered_multimap<String, String> typeMap;

    public:
        ContextImplementation(const wchar_t *rootPath, std::vector<String> searchPathList)
            : rootPath(rootPath)
        {
			searchPathList.push_back(rootPath);
            for (auto &searchPath : searchPathList)
            {
                FileSystem::find(searchPath, [&](const wchar_t *fileName) -> bool
                {
					if (FileSystem::isFile(fileName) && FileSystem::getExtension(fileName).compareNoCase(L".dll") == 0)
					{
						HMODULE module = LoadLibrary(fileName);
						if (module)
						{
							InitializePlugin initializePlugin = (InitializePlugin)GetProcAddress(module, "initializePlugin");
							if (initializePlugin)
							{
								initializePlugin([this](const wchar_t *className, std::function<ContextUserPtr(Context *, void *, std::vector<std::type_index> &)> creator) -> void
								{
									if (classMap.count(className) == 0)
									{
										classMap[className] = creator;
									}
									else
									{
										throw DuplicateClass();
									}
								}, [this](const wchar_t *typeName, const wchar_t *className) -> void
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
							throw InvalidPlugin();
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
            for (auto &module : moduleList)
            {
                FreeLibrary(module);
            }
        }

        // Context
        const wchar_t *getRootPath(void) const
        {
            return rootPath.data();
        }

        ContextUserPtr createBaseClass(const wchar_t *className, void *typelessArguments, std::vector<std::type_index> &argumentTypes) const
        {
            auto classSearch = classMap.find(className);
            if (classSearch == std::end(classMap))
            {
                throw ClassNotFound();
            }

            return (*classSearch).second((Context *)this, typelessArguments, argumentTypes);
        }

        void listTypes(const wchar_t *typeName, std::function<void(const wchar_t *)> onType) const
        {
            GEK_REQUIRE(onType);

            auto typeRange = typeMap.equal_range(typeName);
            for (auto typeSearch = typeRange.first; typeSearch != typeRange.second; ++typeSearch)
            {
                onType(typeSearch->second);
            }
        }
    };

    ContextPtr Context::create(const wchar_t *rootPath, const std::vector<String> &searchPathList)
    {
        return std::make_shared<ContextImplementation>(rootPath, searchPathList);
    }
};
