#include "GEK/Utility/String.hpp"
#include "GEK/Utility/FileSystem.hpp"
#include "GEK/Utility/Context.hpp"
#include "GEK/Utility/ContextUser.hpp"
#include <unordered_map>
#include <Windows.h>
#include <set>

namespace Gek
{
    class ContextImplementation
        : public Context
    {
    private:
        std::vector<HMODULE> moduleList;
        std::unordered_map<std::string_view, std::function<ContextUserPtr(Context *, void *, std::vector<Hash> &)>> classMap;
        std::unordered_multimap<std::string_view, std::string_view> typeMap;
        std::set<std::string> dataPathList;
		FileSystem::Path cachePath;

	public:
        ContextImplementation(void)
        {
        }

        ContextImplementation(std::vector<FileSystem::Path> const &pluginSearchList)
        {
			for (auto const &searchPath : pluginSearchList)
            {
                searchPath.findFiles([&](FileSystem::Path const &filePath) -> bool
                {
					if (filePath.isFile() && String::GetLower(filePath.getExtension()) == ".dll")
					{
                        HMODULE module = LoadLibrary(filePath.getWideString().data());
						if (module)
						{
							InitializePlugin initializePlugin = (InitializePlugin)GetProcAddress(module, "initializePlugin");
							if (initializePlugin)
							{
								LockedWrite{ std::cout } << "Initializing Plugin: " << filePath.getFileName();
								initializePlugin([this, filePath = filePath.getString()](std::string_view className, std::function<ContextUserPtr(Context *, void *, std::vector<Hash> &)> creator) -> void
								{
									if (classMap.count(className) == 0)
									{
										classMap[className] = creator;
										LockedWrite{ std::cout } << "- Adding " << className << " to context registry";
									}
									else
									{
                                        LockedWrite{ std::cerr } << "- Skipping duplicate class from plugin: " << className << ", from: " << filePath;
									}
								}, [this](std::string_view typeName, std::string_view className) -> void
								{
									typeMap.insert(std::make_pair(typeName, className));
									LockedWrite{ std::cout } << "- - Adding " << typeName << " to " << className;
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
                            LockedWrite{ std::cerr } << "! Unable to load plugin: " << filePath.getString();
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
            for (auto const &module : moduleList)
            {
                FreeLibrary(module);
            }
        }

        // Context
		void setCachePath(FileSystem::Path const &path)
		{
			cachePath = path;
		}

		FileSystem::Path getCachePath(FileSystem::Path const &path)
		{
			return cachePath / path;
		}

		void addDataPath(FileSystem::Path const &path)
        {
            dataPathList.insert(path.getString());
        }

        FileSystem::Path findDataPath(FileSystem::Path const &path, bool includeCache) const
        {
			if (includeCache)
			{
				auto fullPath = cachePath / path;
				if (fullPath.isFile() || fullPath.isDirectory())
				{
					return fullPath;
				}
			}

            for (auto &dataPath : dataPathList)
            {
                auto fullPath = dataPath / path;
                if (fullPath.isFile() || fullPath.isDirectory())
                {
                    return fullPath;
                }
            }

            return FileSystem::Path();
        }

		void findDataFiles(FileSystem::Path const &path, std::function<bool(FileSystem::Path const &filePath)> onFileFound, bool includeCache, bool recursive) const
		{
			if (includeCache)
			{
				auto fullPath = cachePath / path;
				if (fullPath.isDirectory())
				{
                    fullPath.findFiles(onFileFound, recursive);
				}
			}

			for (auto &dataPath : dataPathList)
			{
				auto fullPath = dataPath / path;
				if (fullPath.isDirectory())
				{
                    fullPath.findFiles(onFileFound, recursive);
				}
			}
		}

        ContextUserPtr createBaseClass(std::string_view className, void *typelessArguments, std::vector<Hash> &argumentTypes) const
        {
            auto classSearch = classMap.find(className);
            if (classSearch == std::end(classMap))
            {
                LockedWrite{ std::cerr } << "Requested class doesn't exist: " << className;
                return nullptr;
            }

            try
            {
                return (*classSearch).second((Context *)this, typelessArguments, argumentTypes);
            }
            catch (std::exception const &exception)
            {
                LockedWrite{ std::cerr } << "Exception raised trying to create " << exception.what() << ": " << className;
                return nullptr;
            }
            catch (std::string_view error)
            {
                LockedWrite{ std::cerr } << "Error raised trying to create " << error << ": " << className;
                return nullptr;
            }
            catch (...)
            {
                LockedWrite{ std::cerr } << "Unknown exception occurred trying to create " << className;
                return nullptr;
            };
        }

        void listTypes(std::string_view typeName, std::function<void(std::string_view )> onType) const
        {
            assert(onType);

            auto typeRange = typeMap.equal_range(typeName);
            for (auto typeSearch = typeRange.first; typeSearch != typeRange.second; ++typeSearch)
            {
                onType(typeSearch->second);
            }
        }
    };

    ContextPtr Context::Create(std::vector<FileSystem::Path> const *pluginSearchList)
    {
        if (pluginSearchList)
        {
            return std::make_unique<ContextImplementation>(*pluginSearchList);
        }
        else
        {
            return std::make_unique<ContextImplementation>();
        }
    }
}; // namespace Gek
