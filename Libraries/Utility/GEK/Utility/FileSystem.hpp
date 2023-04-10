/// @file
/// @author Todd Zupan <toddzupan@gmail.com>
/// @version $Revision$
/// @section LICENSE
/// https://en.wikipedia.org/wiki/MIT_License
/// @section DESCRIPTION
/// Last Changed: $Date$
#pragma once

#include "GEK/Utility/String.hpp"
#include <filesystem>
#include <functional>
#include <algorithm>
#include <vector>

namespace Gek
{
	namespace FileSystem
	{
		struct Path
		{
		public:
            std::filesystem::path data;

        public:
			Path(void);
			Path(std::filesystem::path const& path);
			Path(std::string_view path);
            Path(std::string const &path);
			Path(Path const &path);
			Path(const char *path);

            void operator = (std::string_view path);
            void operator = (std::string const &path);
			void operator = (Path const &path);

			bool operator == (Path const& path);
			bool operator != (Path const& path);

            Path &removeFileName(void);
            Path &removeExtension(void);

			Path &replaceFileName(std::string_view fileName);
            Path &replaceExtension(std::string_view extension);

			Path withExtension(std::string_view extension) const;
			Path withoutExtension() const;

			Path getRootPath(void) const;
			Path getParentPath(void) const;
			std::string getFileName(void) const;
			std::string getExtension(void) const;
            std::string getString(void) const;
            std::wstring getWideString(void) const;

            void rename(Path const &name) const;
            bool isNewerThan(Path const &path) const;

			bool isFile(void) const;
            size_t getFileSize(void) const;

			bool isDirectory(void) const;
            void createChain(void) const;
            void findFiles(std::function<bool(Path const &filePath)> onFileFound, bool recursive = true) const;

			FileSystem::Path lexicallyRelative(FileSystem::Path const& root) const;
        };

		Path GetModuleFilePath(void);
		Path GetCanonicalPath(Path const& path);

        template <typename CONTAINER>
		CONTAINER Load(Path const &filePath, CONTAINER const &defaultValue = CONTAINER(), std::uintmax_t limitReadSize = 0)
		{
			if (filePath.isFile())
			{
				CONTAINER buffer;
                std::uintmax_t fileSize = filePath.getFileSize();
                auto size = (limitReadSize == 0 ? fileSize : std::min(fileSize, limitReadSize));
				if (size > 0)
				{
					// Need to use fopen since STL methods break up the reads to multiple small calls
                    FILE *file = nullptr;
                    _wfopen_s(&file, filePath.getWideString().data(), L"rb");
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
            _wfopen_s(&file, filePath.getWideString().data(), L"wb");
            if (file != nullptr)
			{
				fwrite(buffer.data(), buffer.size(), 1, file);
				fclose(file);
			}
		}

		template <class ... STRINGS>
		Path CreatePath(STRINGS && ... names)
		{
			Path path;
			([&] {
				path = path / Path(names);
			} (), ...);
			return path;
		}

		Path operator / (Path const& leftPath, std::string_view rightPath);
		Path operator / (Path const& leftPath, std::string const& rightPath);
		Path operator / (Path const& leftPath, Path const& rightPath);
		Path operator / (Path const& leftPath, const char *rightPath);
	}; // namespace FileSystem
}; // namespace Gek

namespace std
{
    inline std::string to_string(Gek::FileSystem::Path const &path)
    {
        return path.getString();
    }
};