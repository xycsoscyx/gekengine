#include "GEK/Utility/JSON.hpp"
#include "GEK/Utility/FileSystem.hpp"
#include "GEK/Utility/Context.hpp"

namespace Gek
{
    namespace JSON
    {
		const Object EmptyObject = Object();

		Object Load(FileSystem::Path const &filePath, const Object &defaultValue)
		{
			std::string data(FileSystem::Load(filePath, String::Empty));
			std::istringstream dataStream(data);
			jsoncons::json_decoder<jsoncons::json> decoder;
			jsoncons::json_reader reader(dataStream, decoder);

			std::error_code errorCode;
			reader.read(errorCode);

			if (errorCode)
			{
				return defaultValue;
                AtomicWriter(std::cerr) << errorCode.message() << " at line " << reader.line_number() << " and column " << reader.column_number() << std::endl;
			}
			else
			{
				return decoder.get_result();
			}
		}

        void Save(FileSystem::Path const &filePath, Object const &object)
        {
            std::ostringstream stream;
            stream << jsoncons::pretty_print(object);
            FileSystem::Save(filePath, stream.str());
        }
    }; // namespace JSON
}; // namespace Gek
