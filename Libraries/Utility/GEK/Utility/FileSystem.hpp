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
#include <iostream>
#include <fstream>
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

            void rename(Path const &name) const;
            bool isNewerThan(Path const &path) const;

			bool isFile(void) const;
            size_t getFileSize(void) const;

			bool isDirectory(void) const;
            void createChain(void) const;
			void setWorkingDirectory(void) const;
            void findFiles(std::function<bool(Path const &filePath)> onFileFound, bool recursive = true) const;

			FileSystem::Path lexicallyRelative(FileSystem::Path const& root) const;
        };

		Path GetModuleFilePath(void);
		Path GetCanonicalPath(Path const& path);

		std::string Read(Path const& filePath);
		std::vector<uint8_t> Load(Path const& filePath, std::uintmax_t limitReadSize = 0);

		template <typename DATA>
		void Write(std::ofstream& file, DATA *data, uint32_t size)
		{
			file.write(reinterpret_cast<const char*>(data), sizeof(DATA) * size);
		}

		template <typename CONTAINER>
		void Save(Path const &filePath, CONTAINER const &buffer)
		{
            filePath.getParentPath().createChain();
            
            std::ofstream file;
            file.open(filePath.getString().data(), std::ios::out | std::ios::binary);
            if (file.is_open())
            {
                file.write(reinterpret_cast<const char*>(buffer.data()), buffer.size());
                file.close();
            }
		}

        Path operator / (Path const& leftPath, std::string_view rightPath);
        Path operator / (Path const& leftPath, std::string const& rightPath);
        Path operator / (Path const& leftPath, Path const& rightPath);
        Path operator / (Path const& leftPath, const char *rightPath);

		template <class ... STRINGS>
		Path CreatePath(STRINGS && ... names)
		{
			Path path;
			([&] {
                path = path / Path(names);
			} (), ...);
			return path;
		}
	}; // namespace FileSystem
}; // namespace Gek

namespace std
{
    inline std::string to_string(Gek::FileSystem::Path const &path)
    {
        return path.getString();
    }
};