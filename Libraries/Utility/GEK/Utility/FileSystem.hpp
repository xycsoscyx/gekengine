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
		struct Path :
			public std::experimental::filesystem::path
		{
			Path(void);
            Path(std::string const &path);
            Path(std::wstring const &path);
            Path(std::experimental::filesystem::path const &path);
			Path(Path const &path);

            void operator = (std::string const &path);
            void operator = (std::wstring const &path);
			void operator = (Path const &path);

			void removeFileName(void);
			void removeExtension(void);

			void replaceFileName(std::string const &fileName);
			void replaceExtension(std::string const &extension);

			Path withExtension(std::string const &extension) const;
			Path withoutExtension() const;

			Path getParentPath(void) const;
			std::string getFileName(void) const;
			std::string getExtension(void) const;

			bool isFile(void) const;
			bool isDirectory(void) const;
			bool isNewerThan(Path const &path) const;
		};

		Path GetModuleFilePath(void);

		inline std::string CombineFileName(std::string const &name)
		{
			return (char(std::experimental::filesystem::path::preferred_separator) + name);
		}

		template <typename... PARAMETERS>
		std::string CombineFileName(std::string const &name, const PARAMETERS&... nameList)
		{
			return (CombineFileName(name) + CombineFileName(nameList...));
		}

		template <typename... PARAMETERS>
		Path GetFileName(std::string const &rootDirectory, const PARAMETERS&... nameList)
		{
			return (rootDirectory + CombineFileName(nameList...));
		}

		template <typename... PARAMETERS>
		Path GetFileName(Path const &rootDirectory, const PARAMETERS&... nameList)
		{
			return (rootDirectory.u8string() + CombineFileName(nameList...));
		}

		void MakeDirectoryChain(Path const &filePath);

		void Find(Path const &rootDirectory, std::function<bool(Path const &filePath)> onFileFound);

		template <typename CONTAINER>
		CONTAINER Load(Path const &filePath, CONTAINER const &defaultValue = CONTAINER(), std::uintmax_t limitReadSize = 0)
		{
			if (std::experimental::filesystem::is_regular_file(filePath))
			{
				CONTAINER buffer;
				auto size = std::experimental::filesystem::file_size(filePath);
				size = (limitReadSize == 0 ? size : std::min(size, limitReadSize));
				if (size > 0)
				{
					// Need to use fopen since STL methods break up the reads to multiple small calls
					FILE *file = _wfopen(filePath.native().c_str(), L"rb");
					if (file != nullptr)
					{
						buffer.resize(size);
						auto read = fread(static_cast<void *>(&buffer.front()), size, 1, file);
						fclose(file);

						if (read == 1)
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
			MakeDirectoryChain(filePath.getParentPath());

			FILE *file = _wfopen(filePath.native().c_str(), L"w+b");
			if (file != nullptr)
			{
				auto wrote = fwrite(buffer.data(), buffer.size(), 1, file);
				fclose(file);
			}
		}
	}; // namespace File
}; // namespace Gek
