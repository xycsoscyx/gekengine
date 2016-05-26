#include "GEK\Math\Common.h"
#include "GEK\Math\Float3.h"
#include "GEK\Math\Float4x4.h"
#include "GEK\Shapes\AlignedBox.h"
#include "GEK\Utility\String.h"
#include <DirectXTex.h>
#include <atlpath.h>
#include <algorithm>
#include <vector>
#include <unordered_map>
#include <map>

class CompressorException
{
public:
    CStringW message;
    int line;

public:
    CompressorException(int line, const wchar_t *formatting, ...)
        : line(line)
    {
        va_list variableList;
        va_start(variableList, formatting);
        message.FormatV(formatting, variableList);
        va_end(variableList);
    }
};

#include <conio.h>
int wmain(int argumentCount, wchar_t *argumentList[], wchar_t *environmentVariableList)
{
    printf("GEK Texture Compressor\r\n");

    CStringW fileNameInput;
    CStringW fileNameOutput;

    CStringW format;
    bool overwrite = false;
    bool sRGBIn = false;
    bool sRGBOut = false;
    for (int argumentIndex = 1; argumentIndex < argumentCount; argumentIndex++)
    {
        CStringW argument(argumentList[argumentIndex]);

        int position = 0;
        CStringW operation(argument.Tokenize(L":", position));
        if (operation.CompareNoCase(L"-input") == 0 && ++argumentIndex < argumentCount)
        {
            fileNameInput = argumentList[argumentIndex];
        }
        else if (operation.CompareNoCase(L"-output") == 0 && ++argumentIndex < argumentCount)
        {
            fileNameOutput = argumentList[argumentIndex];
        }
        else if (operation.CompareNoCase(L"-format") == 0)
        {
            format = argument.Tokenize(L":", position);
        }
        else if (operation.CompareNoCase(L"-overwrite") == 0)
        {
            overwrite = true;
        }
        else if (operation.CompareNoCase(L"-sRGB") == 0)
        {
            CStringW parameter(argument.Tokenize(L":", position));
            if (parameter.CompareNoCase(L"in") == 0)
            {
                sRGBIn = true;
            }
            else if (parameter.CompareNoCase(L"out") == 0)
            {
                sRGBOut = true;
            }
            else if (parameter.CompareNoCase(L"both") == 0)
            {
                sRGBIn = true;
                sRGBOut = true;
            }
        }
    }

    if (format.IsEmpty())
    {
        printf("[error] Compression format required\r\n");
        return -1;
    }
    else if (!PathFileExists(fileNameInput))
    {
        printf("[error] Input file does not exist: %\r\n", fileNameInput.GetString());
        return -2;
    }
    else if (!overwrite && PathFileExists(fileNameOutput))
    {
        printf("[error] Output file already exists: %\r\n", fileNameOutput.GetString());
        return -3;
    }

    DeleteFile(fileNameOutput);
    printf("Compressing: -> %\r\n", fileNameInput.GetString());
    printf("             <- %\r\n", fileNameOutput.GetString());
    printf("Format: %\r\n", format.GetString());
    printf("In: %, Out: %\r\n", sRGBIn ? "sRGB" : "RGB", sRGBOut ? "sRGB" : "RGB");

    try
    {
        printf("Progress...");
        HRESULT resultValue = E_FAIL;

        ::DirectX::ScratchImage inputImage;
        ::DirectX::TexMetadata textureMetaData;
        if (FAILED(resultValue = ::DirectX::LoadFromDDSFile(fileNameInput, 0, &textureMetaData, inputImage)))
        {
            if (FAILED(resultValue = ::DirectX::LoadFromTGAFile(fileNameInput, &textureMetaData, inputImage)))
            {
                static const DWORD formatList[] =
                {
                    ::DirectX::WIC_CODEC_PNG,              // Portable Network Graphics (.png)
                    ::DirectX::WIC_CODEC_BMP,              // Windows Bitmap (.bmp)
                    ::DirectX::WIC_CODEC_JPEG,             // Joint Photographic Experts Group (.jpg, .jpeg)
                };

                for (UINT32 format = 0; format < _ARRAYSIZE(formatList); format++)
                {
                    if (SUCCEEDED(resultValue = ::DirectX::LoadFromWICFile(fileNameInput, formatList[format], &textureMetaData, inputImage)))
                    {
                        break;
                    }
                }
            }

            if (FAILED(resultValue))
            {
                throw CompressorException(__LINE__, L"Unable to load input file");
            }

            ::DirectX::ScratchImage mipMapChain;
            if (SUCCEEDED(resultValue = ::DirectX::GenerateMipMaps(inputImage.GetImages(), inputImage.GetImageCount(), inputImage.GetMetadata(), ::DirectX::TEX_FILTER_TRIANGLE, 0, mipMapChain)))
            {
                inputImage = std::move(mipMapChain);
                printf(".mipmapped.");
            }
            else
            {
                throw CompressorException(__LINE__, L"Unable to generate mipmaps");
            }
        }

        printf(".loaded.");

        DXGI_FORMAT outputFormat = DXGI_FORMAT_UNKNOWN;
        if (sRGBOut)
        {
            if (format.CompareNoCase(L"BC1") == 0)
            {
                outputFormat = DXGI_FORMAT_BC1_UNORM_SRGB;
            }
            else if (format.CompareNoCase(L"BC2") == 0)
            {
                outputFormat = DXGI_FORMAT_BC2_UNORM_SRGB;
            }
            else if (format.CompareNoCase(L"BC3") == 0)
            {
                outputFormat = DXGI_FORMAT_BC3_UNORM_SRGB;
            }
            else if (format.CompareNoCase(L"BC7") == 0)
            {
                outputFormat = DXGI_FORMAT_BC7_UNORM_SRGB;
            }
        }
        else
        {
            if (format.CompareNoCase(L"BC1") == 0)
            {
                outputFormat = DXGI_FORMAT_BC1_UNORM;
            }
            else if (format.CompareNoCase(L"BC2") == 0)
            {
                outputFormat = DXGI_FORMAT_BC2_UNORM;
            }
            else if (format.CompareNoCase(L"BC3") == 0)
            {
                outputFormat = DXGI_FORMAT_BC3_UNORM;
            }
            else if (format.CompareNoCase(L"BC4") == 0)
            {
                outputFormat = DXGI_FORMAT_BC4_UNORM;
            }
            else if (format.CompareNoCase(L"BC5") == 0)
            {
                outputFormat = DXGI_FORMAT_BC5_UNORM;
            }
            else if (format.CompareNoCase(L"BC6") == 0)
            {
                outputFormat = DXGI_FORMAT_BC6H_UF16;
            }
            else if (format.CompareNoCase(L"BC7") == 0)
            {
                outputFormat = DXGI_FORMAT_BC7_UNORM;
            }
        }

        if (outputFormat == DXGI_FORMAT_UNKNOWN)
        {
            throw CompressorException(__LINE__, L"Unknown format specified: %", format.GetString());
        }

        UINT32 flags = ::DirectX::TEX_COMPRESS_PARALLEL;
        if (sRGBIn)
        {
            flags |= ::DirectX::TEX_COMPRESS_SRGB_IN;
        }

        if (sRGBOut)
        {
            flags |= ::DirectX::TEX_COMPRESS_SRGB_OUT;
        }

        ::DirectX::ScratchImage outputImage;
        if (FAILED(::DirectX::Compress(inputImage.GetImages(), inputImage.GetImageCount(), inputImage.GetMetadata(), outputFormat, flags, 0.5f, outputImage)))
        {
            throw CompressorException(__LINE__, L"Unable to compress to format: %", format.GetString());
        }

        printf(".compressed.");
        if (FAILED(::DirectX::SaveToDDSFile(outputImage.GetImages(), outputImage.GetImageCount(), outputImage.GetMetadata(), ::DirectX::DDS_FLAGS_FORCE_DX10_EXT, fileNameOutput)))
        {
            throw CompressorException(__LINE__, L"Unable to save to output file");
        }

        printf(".done!\r\n");
    }
    catch (CompressorException exception)
    {
        printf("\r\n[error] Error (%): %", exception.line, exception.message.GetString());
    }
    catch (...)
    {
        printf("\r\n[error] Unhandled Exception Occurred!");
    }

    printf("\r\n");
    return 0;
}