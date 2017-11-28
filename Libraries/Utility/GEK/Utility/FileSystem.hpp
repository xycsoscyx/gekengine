/// @file
/// @author Todd Zupan <toddzupan@gmail.com>
/// @version $Revision$
/// @section LICENSE
/// https://en.wikipedia.org/wiki/MIT_License
/// @section DESCRIPTION
/// Last Changed: $Date$
#pragma once

#include "GEK/Utility/String.hpp"
#include <experimental\filesystem>
#include <functional>
#include <algorithm>
#include <vector>

namespace Gek
{
	namespace FileSystem
	{
		struct Path
		{
        private:
            std::experimental::filesystem::path data;

        private:
            Path(std::experimental::filesystem::path const &path);

        public:
			Path(void);
            Path(std::string_view path);
            Path(std::string const &path);
			Path(Path const &path);

            void operator = (std::string_view path);
            void operator = (std::string const &path);
			void operator = (Path const &path);

            Path &removeFileName(void);
            Path &removeExtension(void);

			Path &replaceFileName(std::string_view fileName);
            Path &replaceExtension(std::string_view extension);

			Path withExtension(std::string_view extension) const;
			Path withoutExtension() const;

			Path getParentPath(void) const;
			std::string getFileName(void) const;
			std::string getExtension(void) const;
            std::string getString(void) const;
            std::wstring getWindowsString(void) const;

            void rename(Path const &name) const;
            bool isNewerThan(Path const &path) const;

			bool isFile(void) const;
            size_t getFileSize(void) const;

			bool isDirectory(void) const;
            void createChain(void) const;
            void findFiles(std::function<bool(Path const &filePath)> onFileFound) const;
        };

        struct File
            : public Path
        {
        };

		Path GetModuleFilePath(void);

		inline std::string CombineFileName(std::string_view name)
		{
            return String::Format("%v%v", std::experimental::filesystem::path::preferred_separator, name);
		}

		template <typename... PARAMETERS>
		std::string CombineFileName(std::string_view name, const PARAMETERS&... nameList)
		{
			return (CombineFileName(name) + CombineFileName(nameList...));
		}

		template <typename... PARAMETERS>
		Path GetFileName(Path const &rootDirectory, const PARAMETERS&... nameList)
		{
			return (rootDirectory.getString() + CombineFileName(nameList...));
		}

		template <typename CONTAINER>
		CONTAINER Load(Path const &filePath, CONTAINER const &defaultValue = CONTAINER(), std::uintmax_t limitReadSize = 0)
		{
			if (filePath.isFile())
			{
				CONTAINER buffer;
                auto fileSize = filePath.getFileSize();
                auto size = (limitReadSize == 0 ? fileSize : std::min(fileSize, limitReadSize));
				if (size > 0)
				{
					// Need to use fopen since STL methods break up the reads to multiple small calls
                    FILE *file = nullptr;
                    _wfopen_s(&file, filePath.getWindowsString().c_str(), L"rb");
					if (file != nullptr)
					{
						buffer.resize(size);
						auto numberOfSegmentsRead = fread(static_cast<void *>(&buffer.front()), size, 1, file);
						fclose(file);

						if (numberOfSegmentsRead == 1)
						{
							return buffer;
						}
					}
				}
			}

			return defaultValue;
		}

		template <typename CONTAINER>
		void Save(Path const &filePath, CONTAINER const &buffer)
		{
            filePath.getParentPath().createChain();
            
            FILE *file = nullptr;
            _wfopen_s(&file, filePath.getWindowsString().c_str(), L"wb");
            if (file != nullptr)
			{
				auto numberOfSegmentsWritten = fwrite(buffer.data(), buffer.size(), 1, file);
				fclose(file);
			}
		}
	}; // namespace File
}; // namespace Gek

namespace std
{
    inline std::string to_string(Gek::FileSystem::Path const &path)
    {
        return path.getString();
    }
};