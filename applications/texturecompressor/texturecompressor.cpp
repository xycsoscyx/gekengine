#include "GEK\Math\Common.h"
#include "GEK\Math\Float3.h"
#include "GEK\Math\Float4x4.h"
#include "GEK\Shapes\AlignedBox.h"
#include "GEK\Utility\Trace.h"
#include "GEK\Utility\String.h"
#include "GEK\Utility\FileSystem.h"
#include <Shlwapi.h>
#include <DirectXTex.h>
#include <unordered_map>
#include <algorithm>
#include <vector>
#include <map>

using namespace Gek;

#include <conio.h>
int wmain(int argumentCount, const wchar_t *argumentList[], const wchar_t *environmentVariableList)
{
    printf("GEK Texture Compressor\r\n");

    try
    {
        FileSystem::Path fileNameInput;
        FileSystem::Path fileNameOutput;

        String format;
        bool overwrite = false;
        bool sRGBIn = false;
        bool sRGBOut = false;
        for (int argumentIndex = 1; argumentIndex < argumentCount; argumentIndex++)
        {
            String argument(argumentList[argumentIndex]);
            std::vector<String> arguments(argument.split(L':'));
            GEK_CHECK_CONDITION(arguments.empty(), Trace::Exception, "Invalid argument encountered: %v", argumentList[argumentIndex]);
            if (arguments[0].compareNoCase(L"-input") == 0 && ++argumentIndex < argumentCount)
            {
                fileNameInput = argumentList[argumentIndex];
            }
            else if (arguments[0].compareNoCase(L"-output") == 0 && ++argumentIndex < argumentCount)
            {
                fileNameOutput = argumentList[argumentIndex];
            }
            else if (arguments[0].compareNoCase(L"-format") == 0)
            {
                GEK_CHECK_CONDITION(arguments.size() != 2, Trace::Exception, "Invalid values specified for mode");
                format = arguments[1];
            }
            else if (arguments[0].compareNoCase(L"-overwrite") == 0)
            {
                overwrite = true;
            }
            else if (arguments[0].compareNoCase(L"-sRGB") == 0)
            {
                GEK_CHECK_CONDITION(arguments.size() != 2, Trace::Exception, "Invalid values specified for mode");
                if (arguments[1].compareNoCase(L"in") == 0)
                {
                    sRGBIn = true;
                }
                else if (arguments[1].compareNoCase(L"out") == 0)
                {
                    sRGBOut = true;
                }
                else if (arguments[1].compareNoCase(L"both") == 0)
                {
                    sRGBIn = true;
                    sRGBOut = true;
                }
            }
        }

        GEK_CHECK_CONDITION(format.empty(), Trace::Exception, "Compression format required");
        GEK_CHECK_CONDITION(!fileNameInput.isFile(), Trace::Exception, "Input file not found: %v", fileNameInput.string());
        GEK_CHECK_CONDITION(!overwrite && fileNameOutput.isFile(), Trace::Exception, "Output already exists (must specify overwrite): %v", fileNameOutput.string());

        DeleteFile(fileNameOutput);
        printf("Compressing: -> %S\r\n", fileNameInput.c_str());
        printf("             <- %S\r\n", fileNameOutput.c_str());
        printf("Format: %S\r\n", format.c_str());
        printf("In: %s, Out: %s\r\n", sRGBIn ? "sRGB" : "RGB", sRGBOut ? "sRGB" : "RGB");
        printf("Progress...");

        std::vector<uint8_t> fileData;
        FileSystem::load(fileNameInput, fileData);

        String extension(FileSystem::Path(fileNameInput).getExtension());
        std::function<HRESULT(uint8_t*, size_t, ::DirectX::ScratchImage &)> load;
        if (extension.compare(L".dds") == 0)
        {
            load = std::bind(::DirectX::LoadFromDDSMemory, std::placeholders::_1, std::placeholders::_2, 0, nullptr, std::placeholders::_3);
        }
        else if (extension.compare(L".tga") == 0)
        {
            load = std::bind(::DirectX::LoadFromTGAMemory, std::placeholders::_1, std::placeholders::_2, nullptr, std::placeholders::_3);
        }
        else if (extension.compare(L".png") == 0)
        {
            load = std::bind(::DirectX::LoadFromWICMemory, std::placeholders::_1, std::placeholders::_2, ::DirectX::WIC_CODEC_PNG, nullptr, std::placeholders::_3);
        }
        else if (extension.compare(L".bmp") == 0)
        {
            load = std::bind(::DirectX::LoadFromWICMemory, std::placeholders::_1, std::placeholders::_2, ::DirectX::WIC_CODEC_BMP, nullptr, std::placeholders::_3);
        }
        else if (extension.compare(L".jpg") == 0 ||
            extension.compare(L".jpeg") == 0)
        {
            load = std::bind(::DirectX::LoadFromWICMemory, std::placeholders::_1, std::placeholders::_2, ::DirectX::WIC_CODEC_JPEG, nullptr, std::placeholders::_3);
        }

        GEK_CHECK_CONDITION(!load, Trace::Exception, "Invalid file type: %v", extension);

        ::DirectX::ScratchImage image;
        HRESULT resultValue = load(fileData.data(), fileData.size(), image);
        GEK_CHECK_CONDITION(FAILED(resultValue), FileSystem::Exception, "Unable to load file: %v (%v)", resultValue, fileNameInput.string());
        printf(".loaded.");

        ::DirectX::ScratchImage mipMapChain;
        resultValue = ::DirectX::GenerateMipMaps(image.GetImages(), image.GetImageCount(), image.GetMetadata(), ::DirectX::TEX_FILTER_TRIANGLE, 0, mipMapChain);
        GEK_CHECK_CONDITION(FAILED(resultValue), FileSystem::Exception, "Unable to generate mipmap chain: %v", resultValue);
        image = std::move(mipMapChain);
        printf(".mipmapped.");

        DXGI_FORMAT outputFormat = DXGI_FORMAT_UNKNOWN;
        if (sRGBOut)
        {
            if (format.compareNoCase(L"BC1") == 0)
            {
                outputFormat = DXGI_FORMAT_BC1_UNORM_SRGB;
            }
            else if (format.compareNoCase(L"BC2") == 0)
            {
                outputFormat = DXGI_FORMAT_BC2_UNORM_SRGB;
            }
            else if (format.compareNoCase(L"BC3") == 0)
            {
                outputFormat = DXGI_FORMAT_BC3_UNORM_SRGB;
            }
            else if (format.compareNoCase(L"BC7") == 0)
            {
                outputFormat = DXGI_FORMAT_BC7_UNORM_SRGB;
            }
            else
            {
                GEK_THROW_EXCEPTION(Trace::Exception, "Invalid format specified: %v", format);
            }
        }
        else
        {
            if (format.compareNoCase(L"BC1") == 0)
            {
                outputFormat = DXGI_FORMAT_BC1_UNORM;
            }
            else if (format.compareNoCase(L"BC2") == 0)
            {
                outputFormat = DXGI_FORMAT_BC2_UNORM;
            }
            else if (format.compareNoCase(L"BC3") == 0)
            {
                outputFormat = DXGI_FORMAT_BC3_UNORM;
            }
            else if (format.compareNoCase(L"BC4") == 0)
            {
                outputFormat = DXGI_FORMAT_BC4_UNORM;
            }
            else if (format.compareNoCase(L"BC5") == 0)
            {
                outputFormat = DXGI_FORMAT_BC5_UNORM;
            }
            else if (format.compareNoCase(L"BC6") == 0)
            {
                outputFormat = DXGI_FORMAT_BC6H_UF16;
            }
            else if (format.compareNoCase(L"BC7") == 0)
            {
                outputFormat = DXGI_FORMAT_BC7_UNORM;
            }
            else
            {
                GEK_THROW_EXCEPTION(Trace::Exception, "Invalid format specified: %v", format);
            }
        }

        uint32_t flags = ::DirectX::TEX_COMPRESS_PARALLEL;
        if (sRGBIn)
        {
            flags |= ::DirectX::TEX_COMPRESS_SRGB_IN;
        }

        if (sRGBOut)
        {
            flags |= ::DirectX::TEX_COMPRESS_SRGB_OUT;
        }

        ::DirectX::ScratchImage output;
        resultValue = ::DirectX::Compress(image.GetImages(), image.GetImageCount(), image.GetMetadata(), outputFormat, flags, 0.5f, output);
        GEK_CHECK_CONDITION(FAILED(resultValue), FileSystem::Exception, "Unable to compress image: %v", resultValue);
        printf(".compressed.");

        resultValue = ::DirectX::SaveToDDSFile(output.GetImages(), output.GetImageCount(), output.GetMetadata(), ::DirectX::DDS_FLAGS_FORCE_DX10_EXT, fileNameOutput);
        GEK_CHECK_CONDITION(FAILED(resultValue), FileSystem::Exception, "Unable to compress image: %v", resultValue);
        printf(".done!\r\n");
    }
    catch (const Exception &exception)
    {
        printf("\r\n[error] Error (%d): %s", exception.at(), exception.what());
    }
    catch (...)
    {
        printf("\r\n[error] Unhandled Exception Occurred!");
    }

    printf("\r\n");
    return 0;
}