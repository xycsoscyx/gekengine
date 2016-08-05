#include "GEK\Utility\Exceptions.h"
#include "GEK\Utility\String.h"
#include "GEK\Utility\FileSystem.h"
#include "GEK\Context\Context.h"
#include "GEK\Context\ContextUser.h"
#include <Windows.h>
#include <unordered_map>
#include <list>

namespace Gek
{
    class ContextImplementation
        : public Context
    {
    private:
        std::list<HMODULE> moduleList;
        std::unordered_map<String, std::function<ContextUserPtr(Context *, void *, std::vector<std::type_index> &)>> classMap;
        std::unordered_multimap<String, String> typeMap;

    public:
        ContextImplementation(std::vector<String> searchPathList)
        {
            searchPathList.push_back(L"$root");
            for (auto &searchPath : searchPathList)
            {
                FileSystem::find(searchPath, L"*.dll", false, [&](const wchar_t *fileName) -> bool
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
        ContextUserPtr createBaseClass(const wchar_t *className, void *typelessArguments, std::vector<std::type_index> &argumentTypes) const
        {
            auto classSearch = classMap.find(className);
            if (classSearch == classMap.end())
            {
                throw ClassNotFound();
            }

            return (*classSearch).second((Context *)this, typelessArguments, argumentTypes);
        }

        void listTypes(const wchar_t *typeName, std::function<void(const wchar_t *)> onType) const
        {
            GEK_REQUIRE(typeName);
            GEK_REQUIRE(onType);

            auto typeRange = typeMap.equal_range(typeName);
            for (auto typeSearch = typeRange.first; typeSearch != typeRange.second; ++typeSearch)
            {
                onType((*typeSearch).second);
            }
        }
    };

    ContextPtr Context::create(const std::vector<String> &searchPathList)
    {
        return std::make_shared<ContextImplementation>(searchPathList);
    }
};
