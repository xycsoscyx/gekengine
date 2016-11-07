#include "GEK\Utility\JSON.hpp"
#include "GEK\Utility\FileSystem.hpp"

namespace Gek
{
    namespace JSON
    {
        Object load(const wchar_t *fileName)
        {
            String data;
            FileSystem::load(fileName, data);
            return Object::parse(data);
        }

        void save(const wchar_t *fileName, const Object &object)
        {
            std::wostringstream stream;;
            stream << jsoncons::pretty_print(object);
            FileSystem::save(fileName, String(stream.str().data()));
        }
    }; // namespace JSON
}; // namespace Gek
