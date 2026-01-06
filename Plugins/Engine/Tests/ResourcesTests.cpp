#include <gtest/gtest.h>
#include "GEK/Engine/ResourcesHelpers.hpp"
#include "GEK/Utility/FileSystem.hpp"
#include <filesystem>

using namespace Gek::Implementation;
using namespace Gek;

TEST(ResourcesHelpers, SavesSpvSidecarForSpirv)
{
    auto tmp = std::filesystem::temp_directory_path() / std::format("gek_test_resources_{}", std::chrono::high_resolution_clock::now().time_since_epoch().count());
    std::filesystem::create_directories(tmp);

    FileSystem::Path compiledPath(tmp.string());
    compiledPath = compiledPath / FileSystem::CreatePath("test_compiled.bin");

    // SPIR-V magic bytes + some data
    std::string spvData;
    spvData.push_back(char(0x03));
    spvData.push_back(char(0x02));
    spvData.push_back(char(0x23));
    spvData.push_back(char(0x07));
    spvData.push_back(char(0xAA));

    SaveCompiledSidecar(compiledPath, spvData);

    // compiled file exists
    EXPECT_TRUE(compiledPath.isFile());
    auto loaded = FileSystem::Read(compiledPath);
    EXPECT_EQ(loaded, spvData);

    auto spvPath = compiledPath.replaceExtension(".spv");
    EXPECT_TRUE(spvPath.isFile());
    auto loadedSpv = FileSystem::Read(spvPath);
    EXPECT_EQ(loadedSpv, spvData);

    std::filesystem::remove_all(tmp);
}

TEST(ResourcesHelpers, DoesNotSaveSpvSidecarForNonSpirv)
{
    auto tmp = std::filesystem::temp_directory_path() / std::format("gek_test_resources_{}", std::chrono::high_resolution_clock::now().time_since_epoch().count());
    std::filesystem::create_directories(tmp);

    FileSystem::Path compiledPath(tmp.string());
    compiledPath = compiledPath / FileSystem::CreatePath("test_compiled2.bin");

    // non-SPIRV data
    std::string data = "\x00\x01\x02\x03\x04";

    SaveCompiledSidecar(compiledPath, data);

    // compiled file exists
    EXPECT_TRUE(compiledPath.isFile());
    auto loaded = FileSystem::Read(compiledPath);
    EXPECT_EQ(loaded, data);

    auto spvPath = compiledPath.replaceExtension(".spv");
    EXPECT_FALSE(spvPath.isFile());

    std::filesystem::remove_all(tmp);
}
