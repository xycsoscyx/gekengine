#include "GEK/Utility/JSON.hpp"
#include "GEK/Utility/FileSystem.hpp"

namespace Gek
{
    namespace JSON
    {
		const Object EmptyObject = Object();

		Object Load(FileSystem::Path const &filePath, const Object &defaultValue)
		{
			WString data(FileSystem::Load(filePath, CString::Empty));
			std::wistringstream dataStream(data);
			jsoncons::json_decoder<jsoncons::wjson> decoder;
			jsoncons::wjson_reader reader(dataStream, decoder);

			std::error_code errorCode;
			reader.read(errorCode);

			if (errorCode)
			{
				return defaultValue;
				std::cerr << errorCode.message() << " at line " << reader.line_number() << " and column " << reader.column_number() << std::endl;
			}
			else
			{
				return decoder.get_result();
			}
		}

        void Save(FileSystem::Path const &filePath, Object const &object)
        {
            std::wostringstream stream;;
            stream << jsoncons::pretty_print(object);
            FileSystem::Save(filePath, CString(stream.str().data()));
        }
    }; // namespace JSON
}; // namespace Gek
