#include "GEK/Utility/JSON.hpp"
#include "GEK/Utility/FileSystem.hpp"

namespace Gek
{
    namespace JSON
    {
        Object Load(const FileSystem::Path &filePath)
        {
            String data;
            FileSystem::Load(filePath, data);
            return Object::parse(data);
        }

        void Save(const FileSystem::Path &filePath, const Object &object)
        {
            std::wostringstream stream;;
            stream << jsoncons::pretty_print(object);
            FileSystem::Save(filePath, String(stream.str().data()));
        }
    }; // namespace JSON
}; // namespace Gek
