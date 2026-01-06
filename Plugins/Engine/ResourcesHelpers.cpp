#include "GEK/Engine/ResourcesHelpers.hpp"
#include "GEK/Utility/FileSystem.hpp"

namespace Gek
{
    namespace Implementation
    {
        void SaveCompiledSidecar(const FileSystem::Path &compiledPath, std::string const &compiledData)
        {
            FileSystem::Save(compiledPath, compiledData);

            if (compiledData.size() >= 4)
            {
                const uint8_t* bytes = reinterpret_cast<const uint8_t*>(compiledData.data());
                // SPIR-V magic number 0x07230203 (little-endian bytes: 0x03,0x02,0x23,0x07)
                if (bytes[0] == 0x03 && bytes[1] == 0x02 && bytes[2] == 0x23 && bytes[3] == 0x07)
                {
                    auto spvCachePath = compiledPath.replaceExtension(".spv");
                    FileSystem::Save(spvCachePath, compiledData);
                }
            }
        }
    }
}
