#include "GEK/Utility/JSON.hpp"
#include "GEK/Utility/FileSystem.hpp"

namespace Gek
{
    namespace JSON
    {
        Object Load(const wchar_t *fileName)
        {
            String data;
            FileSystem::Load(fileName, data);
            return Object::parse(data);
        }

        void Save(const wchar_t *fileName, const Object &object)
        {
            std::wostringstream stream;;
            stream << jsoncons::pretty_print(object);
            FileSystem::Save(fileName, String(stream.str().data()));
        }
    }; // namespace JSON
}; // namespace Gek
