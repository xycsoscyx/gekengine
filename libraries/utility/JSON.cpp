#include "GEK\Utility\JSON.hpp"

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
    }; // namespace JSON
}; // namespace Gek
