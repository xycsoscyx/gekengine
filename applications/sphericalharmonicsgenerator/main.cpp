#include "GEK\Utility\String.h"
#include "GEK\Utility\FileSystem.h"
#include "GEK\Math\Common.h"
#include "GEK\Math\Float4x4.h"
#include <DirectXTex.h>
#include <atlbase.h>
#include <atlpath.h>
#include <array>
#include <ppl.h>

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
    static const float PI = 3.1415927f;
    static const float CosineA0 = 1.0f;
    static const float CosineA1 = 2.0f / 3.0f;
    static const float CosineA2 = 0.25f;

    template <typename TYPE, int SIZE>
    class SH
    {
    public:
        std::array<TYPE, SIZE> coefficients;

    public:
        SH(void)
        {
            for (int index = 0; index < SIZE; ++index)
            {
                coefficients[index] = TYPE(0.0f);
            }
        }

        const TYPE &operator[] (int index) const
        {
            return coefficients[index];
        }

        TYPE &operator[] (int index)
        {
            return coefficients[index];
        }

        SH &operator = (const SH &other)
        {
            for (int index = 0; index < SIZE; ++index)
            {
                coefficients[index] = other.coefficients[index];
            }

            return (*this);
        }

        SH operator + (const SH &other)
        {
            SH result;
            for (int index = 0; index < SIZE; ++index)
            {
                result[index] = coefficients[index] + other.coefficients[index];
            }

            return result;
        }

        SH operator - (const SH &other)
        {
            SH result;
            for (int index = 0; index < SIZE; ++index)
            {
                result[index] = coefficients[index] - other.coefficients[index];
            }

            return result;
        }

        SH operator * (const SH &other)
        {
            SH result;
            for (int index = 0; index < SIZE; ++index)
            {
                result[index] = coefficients[index] * other.coefficients[index];
            }

            return result;
        }

        SH operator * (const TYPE &scale)
        {
            SH result;
            for (int index = 0; index < SIZE; ++index)
            {
                result[index] = coefficients[index] * scale;
            }

            return result;
        }

        SH operator / (const TYPE &scale)
        {
            SH result;
            for (int index = 0; index < SIZE; ++index)
            {
                result[index] = coefficients[index] / scale;
            }

            return result;
        }

        SH &operator += (const SH &other)
        {
            for (int index = 0; index < SIZE; ++index)
            {
                coefficients[index] += other.coefficients[index];
            }

            return (*this);
        }

        SH &operator -= (const SH &other)
        {
            for (int index = 0; index < SIZE; ++index)
            {
                coefficients[index] -= other.coefficients[index];
            }

            return (*this);
        }

        SH &operator *= (const SH &other)
        {
            for (int index = 0; index < SIZE; ++index)
            {
                coefficients[index] *= other.coefficients[index];
            }

            return (*this);
        }

        SH &operator *= (const TYPE &scale)
        {
            for (int index = 0; index < SIZE; ++index)
            {
                coefficients[index] *= scale;
            }

            return (*this);
        }

        SH &operator /= (const TYPE &scale)
        {
            for (int index = 0; index < SIZE; ++index)
            {
                coefficients[index] /= scale;
            }

            return (*this);
        }
    };

    typedef SH<float, 9> SH9;
    typedef SH<Math::Float4, 9> SH9Color;

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

            if (SUCCEEDED(resultValue) && image.GetMetadata().format != DXGI_FORMAT_R32G32B32A32_FLOAT)
            {
                ::DirectX::ScratchImage rgbImage;
                resultValue = ::DirectX::Convert(image.GetImages(), image.GetImageCount(), image.GetMetadata(), DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0.0f, rgbImage);
                if (SUCCEEDED(resultValue))
                {
                    image = std::move(rgbImage);
                }
            }

            if (SUCCEEDED(resultValue) && !image.GetMetadata().IsCubemap())
            {
                ::DirectX::ScratchImage resized;
                resultValue = ::DirectX::Resize(image.GetImages()[0], 256, 256, 0, resized);
                if (SUCCEEDED(resultValue))
                {
                    image = std::move(resized);
                }
            }
        }

        return resultValue;
    }

    HRESULT loadCubeMap(LPCWSTR fileName, ::DirectX::ScratchImage &image)
    {
        HRESULT resultValue = loadTexture(fileName, image);
        if (SUCCEEDED(resultValue) && !image.GetMetadata().IsCubemap())
        {
            resultValue = E_INVALIDARG;
        }

        return resultValue;
    }

    HRESULT loadIntoCubeMap(LPCWSTR directory, ::DirectX::ScratchImage &image)
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

    SH9 ProjectOntoSH9(const Math::Float3& dir)
    {
        SH9 sh;

        // Band 0
        sh[0] = 0.282095f;

        // Band 1
        sh[1] = 0.488603f * dir.y;
        sh[2] = 0.488603f * dir.z;
        sh[3] = 0.488603f * dir.x;

        // Band 2
        sh[4] = 1.092548f * dir.x * dir.y;
        sh[5] = 1.092548f * dir.y * dir.z;
        sh[6] = 0.315392f * (3.0f * dir.z * dir.z - 1.0f);
        sh[7] = 1.092548f * dir.x * dir.z;
        sh[8] = 0.546274f * (dir.x * dir.x - dir.y * dir.y);

        return sh;
    }

    SH9Color ProjectOntoSH9Color(const Math::Float3& dir, const Math::Float4& color)
    {
        SH9 sh = ProjectOntoSH9(dir);

        SH9Color shColor;
        for (int i = 0; i < 9; ++i)
        {
            shColor[i] = color * sh[i];
        }

        return shColor;
    }

    Math::Float4 EvalSH9Cosine(const Math::Float3& dir, const SH9Color& sh)
    {
        SH9 dirSH = ProjectOntoSH9(dir);
        dirSH[0] *= CosineA0;
        dirSH[1] *= CosineA1;
        dirSH[2] *= CosineA1;
        dirSH[3] *= CosineA1;
        dirSH[4] *= CosineA2;
        dirSH[5] *= CosineA2;
        dirSH[6] *= CosineA2;
        dirSH[7] *= CosineA2;
        dirSH[8] *= CosineA2;

        Math::Float4 result(0.0f);
        for (int i = 0; i < 9; ++i)
        {
            result += sh[i] * dirSH[i];
        }

        return result;
    }

    // Utility function to vmap a XY + Side coordinate to a direction vector
    Math::Float3 MapXYSToDirection(int face, float u, float v)
    {
        v *= -1.0f;
        Math::Float3 dir;
        switch (face)
        {
        case 0: // +x
            dir.set(1.0f, v, -u);
            break;

        case 1: // -x
            dir.set(-1.0f, v, u);
            break;

        case 2: // +y
            dir.set(u, 1.0f, -v);
            break;

        case 3: // -y
            dir.set(u, -1.0f, v);
            break;

        case 4: // +z
            dir.set(u, v, 1.0f);
            break;

        case 5: // -z
            dir.set(-u, v, -1.0f);
            break;
        };

        return dir.getNormal();
    }

    SH9Color ProjectCubeMapToSH(const ::DirectX::ScratchImage &image)
    {
        int imageSize = image.GetMetadata().width;
        float imageSizeFloat = float(imageSize);

        SH9Color result;
        float weightSum = 0.0f;
        for (int face = 0; face < 6; ++face)
        {
            ::DirectX::Image imageFace(image.GetImages()[face]);
            for (int y = 0; y < imageSize; ++y)
            {
                float v = (((y + 0.5f) / imageSizeFloat) * 2.0f) - 1.0f;

                float *pixels = (float *)&imageFace.pixels[y * imageFace.rowPitch];
                for (int x = 0; x < imageSize; ++x)
                {
                    float u = (((x + 0.5f) / imageSizeFloat) * 2.0f) - 1.0f;

                    Math::Float4 sample(&pixels[x * 4]);

                    const float temp = 1.0f + u * u + v * v;
                    const float weight = 4.0f / (std::sqrt(temp) * temp);

                    Math::Float3 dir = MapXYSToDirection(face, u, v);
                    result += ProjectOntoSH9Color(dir, sample) * weight;
                    weightSum += weight;
                }
            }
        }

        result *= (4.0f * 3.14159f) / weightSum;
        return result;
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
        HRESULT resultValue = Gek::loadCubeMap(fileNameInput, image);
        if (FAILED(resultValue))
        {
            resultValue = Gek::loadIntoCubeMap(fileNameInput, image);
            if (SUCCEEDED(resultValue))
            {
                ::DirectX::SaveToDDSFile(image.GetImages(), image.GetImageCount(), image.GetMetadata(), 0, fileNameInput + L"\\cubemap.dds");
            }
        }

        if (SUCCEEDED(resultValue))
        {
            Gek::SH9Color sphericalHarmonics = Gek::ProjectCubeMapToSH(image);

            CStringA output;
            for (int i = 0; i < 9; i++)
            {
                output.AppendFormat("    radiance.coefficients[%d] = float3(%f, %f, %f);\r\n", i, sphericalHarmonics[i].x, sphericalHarmonics[i].y, sphericalHarmonics[i].z);
            }

            Gek::FileSystem::save(L"..//data//programs//Standard//radiance.h", output);
            printf(output);
        }
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