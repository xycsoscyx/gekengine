#include "GEK\Utility\String.h"
#include "GEK\Utility\FileSystem.h"
#include "GEK\Context\Common.h"
#include <DirectXTex.h>
#include <atlbase.h>
#include <atlpath.h>

#ifdef _DEBUG
#pragma comment(lib, "..\\..\\external\\DirectXTex\\Bin\\Desktop_2013\\Win32\\Debug\\DirectXTex.lib")
#else
#pragma comment(lib, "..\\..\\external\\DirectXTex\\Bin\\Desktop_2013\\Win32\\Release\\DirectXTex.lib")
#endif

class GeneratorException
{
public:
    CStringW message;
    int line;

public:
    GeneratorException(int line, LPCWSTR format, ...)
        : line(line)
    {
        va_list variableList;
        va_start(variableList, format);
        message.FormatV(format, variableList);
        va_end(variableList);
    }
};

namespace Gek
{
    static const float CosineA0 = 1.0f;
    static const float CosineA1 = 2.0f / 3.0f;
    static const float CosineA2 = 0.25f;

    class SH9
    {
    public:
        float coefficients[9];

    public:
        SH9(void)
            : coefficients{ 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f }
        {
        }
    };

    class SH9Color
    {
    public:
        Math::Float3 coefficients[9];

    public:
        SH9Color operator * (float scalar)
        {
            SH9Color result;
            for (int i = 0; i < 9; ++i)
            {
                result.coefficients[i] = coefficients[i] * scalar;
            }

            return result;
        }

        void operator += (const SH9Color &sh)
        {
            for (int i = 0; i < 9; ++i)
            {
                coefficients[i] += sh.coefficients[i];
            }
        }

        void operator *= (float scalar)
        {
            for (int i = 0; i < 9; ++i)
            {
                coefficients[i] *= scalar;
            }
        }
    };

    SH9 ProjectOntoSH9(const Math::Float3& direction)
    {
        SH9 sh;

        // Band 0
        sh.coefficients[0] = 0.282095f;

        // Band 1
        sh.coefficients[1] = 0.488603f * direction.y;
        sh.coefficients[2] = 0.488603f * direction.z;
        sh.coefficients[3] = 0.488603f * direction.x;

        // Band 2
        sh.coefficients[4] = 1.092548f * direction.x * direction.y;
        sh.coefficients[5] = 1.092548f * direction.y * direction.z;
        sh.coefficients[6] = 0.315392f * (3.0f * direction.z * direction.z - 1.0f);
        sh.coefficients[7] = 1.092548f * direction.x * direction.z;
        sh.coefficients[8] = 0.546274f * (direction.x * direction.x - direction.y * direction.y);

        return sh;
    }

    SH9Color ProjectOntoSH9Color(const Math::Float3& direction, const Math::Float3& color)
    {
        SH9 sh = ProjectOntoSH9(direction);

        SH9Color shColor;
        for (int i = 0; i < 9; ++i)
        {
            shColor.coefficients[i] = color * sh.coefficients[i];
        }

        return shColor;
    }

    Math::Float3 EvalSH9Cosine(const Math::Float3& direction, const SH9Color& sh)
    {
        SH9 dirSH = ProjectOntoSH9(direction);
        dirSH.coefficients[0] *= CosineA0;
        dirSH.coefficients[1] *= CosineA1;
        dirSH.coefficients[2] *= CosineA1;
        dirSH.coefficients[3] *= CosineA1;
        dirSH.coefficients[4] *= CosineA2;
        dirSH.coefficients[5] *= CosineA2;
        dirSH.coefficients[6] *= CosineA2;
        dirSH.coefficients[7] *= CosineA2;
        dirSH.coefficients[8] *= CosineA2;

        Math::Float3 result;
        for (int i = 0; i < 9; ++i)
        {
            result += sh.coefficients[i] * dirSH.coefficients[i];
        }

        return result;
    }

    Math::Float3 MapXYSToDirection(float u, float v, int face)
    {
        v *= -1.0;
        Math::Float3 direction;
        switch (face)
        {
        case 0: // +x
            direction = Math::Float3(1.0, v, -u);
            break;

        case 1: // -x
            direction = Math::Float3(-1.0, v, u);
            break;

        case 2: // +y
            direction = Math::Float3(u, 1.0, -v);
            break;

        case 3: // -y
            direction = Math::Float3(u, -1.0, v);
            break;

        case 4: // +z
            direction = Math::Float3(u, v, 1.0);
            break;

        case 5: // -z
            direction = Math::Float3(-u, v, -1.0);
            break;
        };

        return direction.getNormal();
    }

    SH9Color ProjectCubemapToSH(::DirectX::ScratchImage &image)
    {
        const int width = image.GetMetadata().width;
        const int height = image.GetMetadata().height;

        SH9Color result;
        float weightSum = 0.0f;
        for (int face = 0; face < 6; ++face)
        {
            printf("Face (%d): .", face);
            ::DirectX::Image imageFace = image.GetImages()[face];
            for (int y = 0; y < height; ++y)
            {
                for (int x = 0; x < width; ++x)
                {
                    Math::Float3 texel;
                    int offset = ((imageFace.rowPitch * y) + (x * 4));
                    texel.x = float(imageFace.pixels[offset + 0]) / 255.0f;
                    texel.y = float(imageFace.pixels[offset + 1]) / 255.0f;
                    texel.z = float(imageFace.pixels[offset + 2]) / 255.0f;

                    float u = (x + 0.5f) / width;
                    float v = (y + 0.5f) / height;

                    // Account for cubemap texel distribution
                    u = u * 2.0f - 1.0f;
                    v = v * 2.0f - 1.0f;
                    const float temp = 1.0f + u * u + v * v;
                    const float weight = 4.0f / (std::sqrt(temp) * temp);

                    Math::Float3 direction = MapXYSToDirection(u, v, face);
                    result += ProjectOntoSH9Color(direction, texel) * weight;
                    weightSum += weight;
                }

                printf(".");
            }

            printf("\r\n");
        }

        result *= (4.0f * 3.14159f) / weightSum;
        return result;
    }

    HRESULT loadTexture(LPCWSTR fileName, ::DirectX::ScratchImage &image)
    {
        std::vector<UINT8> fileData;
        HRESULT resultValue = FileSystem::load(fileName, fileData);
        if (SUCCEEDED(resultValue))
        {

            CPathW filePath(fileName);
            CStringW extension(filePath.GetExtension());
            if (extension.CompareNoCase(L".dds") == 0)
            {
                resultValue = ::DirectX::LoadFromDDSMemory(fileData.data(), fileData.size(), 0, nullptr, image);
            }
            else if (extension.CompareNoCase(L".tga") == 0)
            {
                resultValue = ::DirectX::LoadFromTGAMemory(fileData.data(), fileData.size(), nullptr, image);
            }
            else if (extension.CompareNoCase(L".png") == 0)
            {
                resultValue = ::DirectX::LoadFromWICMemory(fileData.data(), fileData.size(), ::DirectX::WIC_CODEC_PNG, nullptr, image);
            }
            else if (extension.CompareNoCase(L".bmp") == 0)
            {
                resultValue = ::DirectX::LoadFromWICMemory(fileData.data(), fileData.size(), ::DirectX::WIC_CODEC_BMP, nullptr, image);
            }
            else if (extension.CompareNoCase(L".jpg") == 0 ||
                extension.CompareNoCase(L".jpeg") == 0)
            {
                resultValue = ::DirectX::LoadFromWICMemory(fileData.data(), fileData.size(), ::DirectX::WIC_CODEC_JPEG, nullptr, image);
            }

            if (SUCCEEDED(resultValue) && ::DirectX::IsCompressed(image.GetMetadata().format))
            {
                ::DirectX::ScratchImage decompressedImage;
                resultValue = ::DirectX::Decompress(image.GetImages(), image.GetImageCount(), image.GetMetadata(), DXGI_FORMAT_R8G8B8A8_UNORM, decompressedImage);
                if (SUCCEEDED(resultValue))
                {
                    image = std::move(decompressedImage);
                }
            }

            if (SUCCEEDED(resultValue) && image.GetMetadata().format != DXGI_FORMAT_R8G8B8A8_UNORM)
            {
                ::DirectX::ScratchImage rgbImage;
                resultValue = ::DirectX::Convert(image.GetImages(), image.GetImageCount(), image.GetMetadata(), DXGI_FORMAT_R8G8B8A8_UNORM, 0, 0.0f, rgbImage);
                if (SUCCEEDED(resultValue))
                {
                    image = std::move(rgbImage);
                }
            }
        }

        if (SUCCEEDED(resultValue) && !image.GetMetadata().IsCubemap())
        {
            resultValue = E_INVALIDARG;
        }

        return resultValue;
    }

    HRESULT loadCubeMap(LPCWSTR directory, ::DirectX::ScratchImage &image)
    {
        HRESULT resultValue = E_FAIL;
        ::DirectX::ScratchImage cubeMapList[6];
        for (UINT32 face = 0; face < 6; face++)
        {
            static const LPCWSTR directions[] =
            {
                L"posx",
                L"negx",
                L"posy",
                L"negy",
                L"posz",
                L"negz",
            };

            FileSystem::find(directory, String::format(L"%s.*", directions[face]), false, [&](LPCWSTR fileName) -> HRESULT
            {
                resultValue = loadTexture(fileName, cubeMapList[face]);
                if (SUCCEEDED(resultValue))
                {
                    // break when found
                    return E_FAIL;
                }

                return S_OK;
            });

            if (FAILED(resultValue))
            {
                break;
            }
        }

        if (SUCCEEDED(resultValue))
        {
            ::DirectX::Image imageList[6] =
            {
                *cubeMapList[0].GetImage(0, 0, 0),
                *cubeMapList[1].GetImage(0, 0, 0),
                *cubeMapList[2].GetImage(0, 0, 0),
                *cubeMapList[3].GetImage(0, 0, 0),
                *cubeMapList[4].GetImage(0, 0, 0),
                *cubeMapList[5].GetImage(0, 0, 0),
            };

            resultValue = image.InitializeCubeFromImages(imageList, 6, 0);
        }

        return resultValue;
    }
};

int wmain(int argumentCount, wchar_t *argumentList[], wchar_t *environmentVariableList)
{
    printf("GEK Spherical Harmonics Generator\r\n");

    CStringW fileNameInput;
    CStringW fileNameOutput;
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
    }

    CoInitialize(nullptr);
    try
    {
#ifdef _DEBUG
        SetCurrentDirectory(Gek::FileSystem::expandPath(L"%root%\\Debug"));
#else
        SetCurrentDirectory(Gek::FileSystem::expandPath(L"%root%\\Release"));
#endif
        ::DirectX::ScratchImage image;
        HRESULT resultValue = Gek::loadTexture(fileNameInput, image);
        if (FAILED(resultValue))
        {
            resultValue = Gek::loadCubeMap(fileNameInput, image);
        }

        Gek::SH9Color sh = Gek::ProjectCubemapToSH(image);

        CStringA output;
        for (int i = 0; i < 9; i++)
        {
            output.AppendFormat("    sh.coefficients[%d] = float3(%f, %f, %f);\r\n", i, sh.coefficients[i].x, sh.coefficients[i].y, sh.coefficients[i].z);
        }

        printf(output);
    }
    catch (GeneratorException exception)
    {
        printf("[error] Error (%d): %S", exception.line, exception.message.GetString());
    }
    catch (...)
    {
        printf("[error] Unhandled Exception Occurred!");
    }

    printf("\r\n");
    CoUninitialize();
    return 0;
}