#include "GEK\Math\Common.h"
#include "GEK\Math\Float3.h"
#include "GEK\Math\Float4x4.h"
#include "GEK\Shapes\AlignedBox.h"
#include "GEK\Utility\Exceptions.h"
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
        String fileNameInput;
        String fileNameOutput;

        String format;
        bool overwrite = false;
        bool sRGBIn = false;
        bool sRGBOut = false;
        for (int argumentIndex = 1; argumentIndex < argumentCount; argumentIndex++)
        {
            String argument(argumentList[argumentIndex]);
            std::vector<String> arguments(argument.split(L':'));
            if (arguments.empty())
            {
                throw std::exception("No arguments specified for command line parameter");
            }

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
                throw std::exception("Missing parameters for mode");
                format = arguments[1];
            }
            else if (arguments[0].compareNoCase(L"-overwrite") == 0)
            {
                overwrite = true;
            }
            else if (arguments[0].compareNoCase(L"-sRGB") == 0)
            {
                throw std::exception("Missing parameters for sRGB");
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

        if (format.empty())
        {
            throw std::exception("Compression format required");
        }

        if (!FileSystem::isFile(fileNameInput))
        {
            throw std::exception("Input file not found");
        }

        if (!overwrite && FileSystem::isFile(fileNameOutput))
        {
            throw std::exception("Output already exists (must specify overwrite)");
        }

        DeleteFile(fileNameOutput.c_str());
        printf("Compressing: -> %S\r\n", fileNameInput.c_str());
        printf("             <- %S\r\n", fileNameOutput.c_str());
        printf("Format: %S\r\n", format.c_str());
        printf("In: %s, Out: %s\r\n", sRGBIn ? "sRGB" : "RGB", sRGBOut ? "sRGB" : "RGB");
        printf("Progress...");

        std::vector<uint8_t> fileData;
        FileSystem::load(fileNameInput, fileData);

        String extension(FileSystem::getExtension(fileNameInput));
        std::function<HRESULT(uint8_t*, size_t, ::DirectX::ScratchImage &)> load;
        if (extension.compareNoCase(L".dds") == 0)
        {
            load = std::bind(::DirectX::LoadFromDDSMemory, std::placeholders::_1, std::placeholders::_2, 0, nullptr, std::placeholders::_3);
        }
        else if (extension.compareNoCase(L".tga") == 0)
        {
            load = std::bind(::DirectX::LoadFromTGAMemory, std::placeholders::_1, std::placeholders::_2, nullptr, std::placeholders::_3);
        }
        else if (extension.compareNoCase(L".png") == 0)
        {
            load = std::bind(::DirectX::LoadFromWICMemory, std::placeholders::_1, std::placeholders::_2, ::DirectX::WIC_CODEC_PNG, nullptr, std::placeholders::_3);
        }
        else if (extension.compareNoCase(L".bmp") == 0)
        {
            load = std::bind(::DirectX::LoadFromWICMemory, std::placeholders::_1, std::placeholders::_2, ::DirectX::WIC_CODEC_BMP, nullptr, std::placeholders::_3);
        }
        else if (extension.compareNoCase(L".jpg") == 0 ||
            extension.compareNoCase(L".jpeg") == 0)
        {
            load = std::bind(::DirectX::LoadFromWICMemory, std::placeholders::_1, std::placeholders::_2, ::DirectX::WIC_CODEC_JPEG, nullptr, std::placeholders::_3);
        }

        if (!load)
        {
            throw std::exception("Unknown file tyupe listed for input");
        }

        ::DirectX::ScratchImage image;
        HRESULT resultValue = load(fileData.data(), fileData.size(), image);
        if (FAILED(resultValue))
        {
            throw std::exception("Unable to load input file");
        }

        printf(".loaded.");

        ::DirectX::ScratchImage mipMapChain;
        resultValue = ::DirectX::GenerateMipMaps(image.GetImages(), image.GetImageCount(), image.GetMetadata(), ::DirectX::TEX_FILTER_TRIANGLE, 0, mipMapChain);
        if (FAILED(resultValue))
        {
            throw std::exception("Unabole to create mipmap chain");
        }

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
                throw std::exception("Invalid conversion format specified");
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
                throw std::exception("Invalid conversion format specified");
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
        if (FAILED(resultValue))
        {
            throw std::exception("Unable to compress image");
        }

        printf(".compressed.");

        resultValue = ::DirectX::SaveToDDSFile(output.GetImages(), output.GetImageCount(), output.GetMetadata(), ::DirectX::DDS_FLAGS_FORCE_DX10_EXT, fileNameOutput.c_str());
        if (FAILED(resultValue))
        {
            throw std::exception("Unable to save image");
        }

        printf(".done!\r\n");
    }
    catch (const std::exception &exception)
    {
        printf("[error] Exception occurred: %s", exception.what());
    }
    catch (...)
    {
        printf("\r\n[error] Unhandled Exception Occurred!");
    }

    printf("\r\n");
    return 0;
}