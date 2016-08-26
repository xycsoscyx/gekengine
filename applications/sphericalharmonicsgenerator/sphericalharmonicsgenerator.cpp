#include "GEK\Utility\Exceptions.h"
#include "GEK\Utility\String.h"
#include "GEK\Utility\FileSystem.h"
#include "GEK\Math\Common.h"
#include "GEK\Math\Float4x4.h"
#include <DirectXTex.h>
#include <array>
#include <ppl.h>

using namespace Gek;

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

::DirectX::ScratchImage loadTexture(const wchar_t *fileName)
{
    std::vector<uint8_t> buffer;
    FileSystem::load(fileName, buffer);

	String extension(FileSystem::getExtension(fileName));
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
    HRESULT resultValue = load(buffer.data(), buffer.size(), image);
    if (FAILED(resultValue))
    {
        throw std::exception("Unable to load input file");
    }

    if (::DirectX::IsCompressed(image.GetMetadata().format))
    {
        ::DirectX::ScratchImage decompressedImage;
        resultValue = ::DirectX::Decompress(image.GetImages(), image.GetImageCount(), image.GetMetadata(), DXGI_FORMAT_R8G8B8A8_UNORM, decompressedImage);
        if (FAILED(resultValue))
        {
            throw std::exception("Unable to decompress image to raw RGBA");
        }

        image = std::move(decompressedImage);
    }

    if (SUCCEEDED(resultValue) && image.GetMetadata().format != DXGI_FORMAT_R32G32B32A32_FLOAT)
    {
        ::DirectX::ScratchImage rgbImage;
        resultValue = ::DirectX::Convert(image.GetImages(), image.GetImageCount(), image.GetMetadata(), DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0.0f, rgbImage);
        if (FAILED(resultValue))
        {
            throw std::exception("Unable to convert image to float");
        }

        image = std::move(rgbImage);
    }

    if (SUCCEEDED(resultValue) && !image.GetMetadata().IsCubemap())
    {
        ::DirectX::ScratchImage resized;
        resultValue = ::DirectX::Resize(image.GetImages()[0], 256, 256, 0, resized);
        if (FAILED(resultValue))
        {
            throw std::exception("Unable to resize image to 256x256");
        }

        image = std::move(resized);
    }

    return image;
}

::DirectX::ScratchImage loadIntoCubeMap(const wchar_t *directory)
{
	static const wchar_t *faceList[] =
	{
		L"posx",
		L"negx",
		L"posy",
		L"negy",
		L"posz",
		L"negz",
	};
	
	static const wchar_t *formatList[] =
	{
		L"",
		L".dds",
		L".tga",
		L".png",
		L".jpg",
		L".bmp",
	};

	::DirectX::ScratchImage cubeMapList[6];
	for (auto &face : { 0, 1, 2, 3, 4, 5 })
    {
		for (auto &format : formatList)
		{
            String fileName(FileSystem::getFileName(directory, faceList[face]).append(format));
            if (FileSystem::isFile(fileName))
            {
                cubeMapList[face] = loadTexture(FileSystem::getFileName(directory, faceList[face]).append(format));
                break;
            }
		}

		if (cubeMapList[face].GetImage(0, 0, 0) == nullptr)
		{
			throw std::exception("Unable to load cubemap face");
		}
    }

    ::DirectX::Image imageList[6] =
    {
        *cubeMapList[0].GetImage(0, 0, 0),
        *cubeMapList[1].GetImage(0, 0, 0),
        *cubeMapList[2].GetImage(0, 0, 0),
        *cubeMapList[3].GetImage(0, 0, 0),
        *cubeMapList[4].GetImage(0, 0, 0),
        *cubeMapList[5].GetImage(0, 0, 0),
    };

    ::DirectX::ScratchImage image;
    HRESULT resultValue = image.InitializeCubeFromImages(imageList, 6, 0);
    if (FAILED(resultValue))
    {
        throw std::exception("Unable to initialize cubemap from face list");
    }

    return image;
}

SH9 ProjectOntoSH9(const Math::Float3& direction)
{
    SH9 sh;

    // Band 0
    sh[0] = 0.282095f;

    // Band 1
    sh[1] = 0.488603f * direction.y;
    sh[2] = 0.488603f * direction.z;
    sh[3] = 0.488603f * direction.x;

    // Band 2
    sh[4] = 1.092548f * direction.x * direction.y;
    sh[5] = 1.092548f * direction.y * direction.z;
    sh[6] = 0.315392f * (3.0f * direction.z * direction.z - 1.0f);
    sh[7] = 1.092548f * direction.x * direction.z;
    sh[8] = 0.546274f * (direction.x * direction.x - direction.y * direction.y);

    return sh;
}

SH9Color ProjectOntoSH9Color(const Math::Float3& direction, const Math::Float4& color)
{
    SH9 sh = ProjectOntoSH9(direction);

    SH9Color shColor;
    for (int i = 0; i < 9; ++i)
    {
        shColor[i] = color * sh[i];
    }

    return shColor;
}

Math::Float4 EvalSH9Cosine(const Math::Float3& direction, const SH9Color& sh)
{
    SH9 dirSH = ProjectOntoSH9(direction);
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
    Math::Float3 direction;
    switch (face)
    {
    case 0: // +x
        direction.set(1.0f, v, -u);
        break;

    case 1: // -x
        direction.set(-1.0f, v, u);
        break;

    case 2: // +y
        direction.set(u, 1.0f, -v);
        break;

    case 3: // -y
        direction.set(u, -1.0f, v);
        break;

    case 4: // +z
        direction.set(u, v, 1.0f);
        break;

    case 5: // -z
        direction.set(-u, v, -1.0f);
        break;
    };

    return direction.getNormal();
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

                Math::Float3 direction(MapXYSToDirection(face, u, v));
                result += ProjectOntoSH9Color(direction, sample) * weight;
                weightSum += weight;
            }
        }
    }

    result *= (4.0f * 3.14159f) / weightSum;
    return result;
}

int wmain(int argumentCount, const wchar_t *argumentList[], const wchar_t *environmentVariableList)
{
    printf("GEK Spherical Harmonics Generator\r\n");

    CoInitialize(nullptr);
    try
    {
        String fileNameInput;
        String fileNameOutput;
        for (int argumentIndex = 1; argumentIndex < argumentCount; argumentIndex++)
        {
            String argument(argumentList[argumentIndex]);
            if (argument.compareNoCase(L"-input") == 0 && ++argumentIndex < argumentCount)
            {
                fileNameInput = argumentList[argumentIndex];
            }
            else if (argument.compareNoCase(L"-output") == 0 && ++argumentIndex < argumentCount)
            {
                fileNameOutput = argumentList[argumentIndex];
            }
        }

        ::DirectX::ScratchImage image;
        try
        {
            image = loadTexture(fileNameInput);
            if (!image.GetMetadata().IsCubemap())
            {
                throw std::exception("Specified input image is not a cubemap");
            }
        }
        catch (const std::exception &)
        {
            image = loadIntoCubeMap(fileNameInput);
        };

        ::DirectX::SaveToDDSFile(image.GetImages(), image.GetImageCount(), image.GetMetadata(), 0, String(L"%v\\cubemap.dds", fileNameInput));

        SH9Color sphericalHarmonics = ProjectCubeMapToSH(image);

        StringUTF8 output;
        for (int i = 0; i < 9; i++)
        {
            output.append("    radiance.coefficients[%] = float3(%v, %v, %v);\r\n", i, sphericalHarmonics[i].x, sphericalHarmonics[i].y, sphericalHarmonics[i].z);
        }

        FileSystem::save(L"..//data//programs//Standard//radiance.h", output);
        printf("%s", output.c_str());
    }
    catch (const std::exception &exception)
    {
        printf("[error] Exception occurred: %s", exception.what());
    }
    catch (...)
    {
        printf("[error] Unhandled Exception Occurred!");
    }

    printf("\r\n");
    CoUninitialize();
    return 0;
}