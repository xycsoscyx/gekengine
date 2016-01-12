#include "GEK\Utility\String.h"
#include "GEK\Utility\FileSystem.h"
#include "GEK\Context\Common.h"
#include "GEK\Math\Matrix4x4.h"
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
    static const float PI = 3.1415927f;
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

        const float &operator[] (int index) const
        {
            return coefficients[index];
        }

        float &operator[] (int index)
        {
            return coefficients[index];
        }
    };

    class SH9Color
    {
    public:
        Math::Float4 coefficients[9];

    public:
        const Math::Float4 &operator[] (int index) const
        {
            return coefficients[index];
        }

        Math::Float4 &operator[] (int index)
        {
            return coefficients[index];
        }

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

    static const Math::Float3 nx(-1.0, 0.0, 0.0);
    static const Math::Float3 px(1.0, 0.0, 0.0);
    static const Math::Float3 ny(0.0, -1.0, 0.0);
    static const Math::Float3 py(0.0, 1.0, 0.0);
    static const Math::Float3 nz(0.0, 0.0, -1.0);
    static const Math::Float3 pz(0.0, 0.0, 1.0);

    static const Math::Float3 cubemap_axis[6][3] =
    {
        { pz, py, nx },
        { nz, py, px },
        { nx, nz, ny },
        { nx, pz, py },
        { nx, py, nz },
        { px, py, pz },
    };

    static Math::Float3 getVector(int F, float x, float y, float z)
    {
        Math::Float3 X = cubemap_axis[F][0];
        Math::Float3 Y = cubemap_axis[F][1];
        Math::Float3 Z = cubemap_axis[F][2];

        Math::Float3 w;
        w[0] = X[0] * x + Y[0] * y + Z[0] * z;
        w[1] = X[1] * x + Y[1] * y + Z[1] * z;
        w[2] = X[2] * x + Y[2] * y + Z[2] * z;
        return w.getNormal();
    }

    float solid_angle(const Math::Float3 &a, const Math::Float3 &b, const Math::Float3 &c)
    {
        float n = fabs(a[0] * (b[1] * c[2] - b[2] * c[1]) +
                       a[1] * (b[2] * c[0] - b[0] * c[2]) +
                       a[2] * (b[0] * c[1] - b[1] * c[0]));

        float d = 1.0f + a[0] * b[0] + a[1] * b[1] + a[2] * b[2]
                       + a[0] * c[0] + a[1] * c[1] + a[2] * c[2]
                       + b[0] * c[0] + b[1] * c[1] + b[2] * c[2];

        return 2.0f * std::atan2(n, d);
    }

    SH9Color sph_grace_cathedral(void)
    {
        SH9Color sh;
        sh.coefficients[0] = Math::Float3(0.79f, 0.44f, 0.54f).w(0.0f);
        sh.coefficients[1] = Math::Float3(0.39f, 0.35f, 0.60f).w(0.0f);
        sh.coefficients[2] = Math::Float3(-0.34f, -0.18f, -0.27f).w(0.0f);
        sh.coefficients[3] = Math::Float3(-0.29f, -0.06f, 0.01f).w(0.0f);
        sh.coefficients[4] = Math::Float3(-0.11f, -0.05f, -0.12f).w(0.0f);
        sh.coefficients[5] = Math::Float3(-0.26f, -0.22f, -0.47f).w(0.0f);
        sh.coefficients[6] = Math::Float3(-0.16f, -0.09f, -0.15f).w(0.0f);
        sh.coefficients[7] = Math::Float3(0.56f, 0.21f, 0.14f).w(0.0f);
        sh.coefficients[8] = Math::Float3(0.21f, -0.05f, -0.30f).w(0.0f);
        return sh;
    }

    SH9Color sph_eucalyptus_grove(void)
    {
        SH9Color sh;
        sh.coefficients[0] = Math::Float3(0.38f, 0.43f, 0.45f).w(0.0f);
        sh.coefficients[1] = Math::Float3(0.29f, 0.36f, 0.41f).w(0.0f);
        sh.coefficients[2] = Math::Float3(0.04f, 0.03f, 0.01f).w(0.0f);
        sh.coefficients[3] = Math::Float3(-0.10f, -0.10f, -0.09f).w(0.0f);
        sh.coefficients[4] = Math::Float3(-0.06f, -0.06f, -0.04f).w(0.0f);
        sh.coefficients[5] = Math::Float3(0.01f, -0.01f, -0.05f).w(0.0f);
        sh.coefficients[6] = Math::Float3(-0.09f, -0.13f, -0.15f).w(0.0f);
        sh.coefficients[7] = Math::Float3(-0.06f, -0.05f, -0.04f).w(0.0f);
        sh.coefficients[8] = Math::Float3(0.02f, -0.00f, -0.05f).w(0.0f);
        return sh;
    }

    SH9Color sph_st_peters_basilica(void)
    {
        SH9Color sh;
        sh.coefficients[0] = Math::Float3(0.36f, 0.26f, 0.23f).w(0.0f);
        sh.coefficients[1] = Math::Float3(0.18f, 0.14f, 0.13f).w(0.0f);
        sh.coefficients[2] = Math::Float3(-0.02f, -0.01f, -0.00f).w(0.0f);
        sh.coefficients[3] = Math::Float3(0.03f, 0.02f, 0.01f).w(0.0f);
        sh.coefficients[4] = Math::Float3(0.02f, 0.01f, 0.00f).w(0.0f);
        sh.coefficients[5] = Math::Float3(-0.05f, -0.03f, -0.01f).w(0.0f);
        sh.coefficients[6] = Math::Float3(-0.09f, -0.08f, -0.07f).w(0.0f);
        sh.coefficients[7] = Math::Float3(0.01f, 0.00f, 0.00f).w(0.0f);
        sh.coefficients[8] = Math::Float3(-0.08f, -0.06f, 0.00f).w(0.0f);
        return sh;
    }

    /*----------------------------------------------------------------------------*/

    float calc_domega(const Math::Float3 &v00, const Math::Float3 &v01, const Math::Float3 &v10, const Math::Float3 &v11)
    {
        return (solid_angle(v00, v11, v01) +
                solid_angle(v11, v00, v10)) / (4 * PI);
    }

    SH9 calc_Y(const Math::Float3 &dir)
    {
        SH9 sh;
        sh[0] = 0.282095f;
        sh[1] = 0.488603f * dir.y;
        sh[2] = 0.488603f * dir.z;
        sh[3] = 0.488603f * dir.x;
        sh[4] = 1.092548f * dir.x * dir.y;
        sh[5] = 1.092548f * dir.y * dir.z;
        sh[6] = 0.315392f * (3.0f * dir.z * dir.z - 1.0f);
        sh[7] = 1.092548f * dir.x * dir.z;
        sh[8] = 0.546274f * (dir.x * dir.x - dir.y * dir.y);
        return sh;
    }

    SH9Color env_to_sph_face(const ::DirectX::Image &image, int F)
    {
        SH9Color L;
        int N = image.width;
        for (int ypos = 0; ypos < N; ++ypos)
        {
            for (int xpos = 0; xpos < N; ++xpos)
            {
                /* Compute the direction vector to this pixel. */
                float y0 = (2.0f * (ypos) - N) / N;
                float y  = (2.0f * (ypos + 0.5f) - N) / N;
                float y1 = (2.0f * (ypos + 1.0f) - N) / N;
                float x0 = (2.0f * (xpos) - N) / N;
                float x  = (2.0f * (xpos + 0.5f) - N) / N;
                float x1 = (2.0f * (xpos + 1.0f) - N) / N;

                Math::Float3 v00 = getVector(F, x0, y0, 1.0f);
                Math::Float3 v01 = getVector(F, x0, y1, 1.0f);
                Math::Float3 v10 = getVector(F, x1, y0, 1.0f);
                Math::Float3 v11 = getVector(F, x1, y1, 1.0f);
                Math::Float3 v = getVector(F, x, y, 1.0f);

                float dd = calc_domega(v00, v01, v10, v11);

                SH9 Y = calc_Y(v);

                Math::Float3 p;
                p.x = float(image.pixels[(ypos * image.rowPitch) + (xpos * 4) + 0]) / 255.0f;
                p.y = float(image.pixels[(ypos * image.rowPitch) + (xpos * 4) + 1]) / 255.0f;
                p.z = float(image.pixels[(ypos * image.rowPitch) + (xpos * 4) + 2]) / 255.0f;
                for (int k = 0; k < 3; ++k)
                {
                    L[0][k] += Y[0] * p[k] * dd;
                    L[1][k] += Y[1] * p[k] * dd;
                    L[2][k] += Y[2] * p[k] * dd;
                    L[3][k] += Y[3] * p[k] * dd;
                    L[4][k] += Y[4] * p[k] * dd;
                    L[5][k] += Y[5] * p[k] * dd;
                    L[6][k] += Y[6] * p[k] * dd;
                    L[7][k] += Y[7] * p[k] * dd;
                    L[8][k] += Y[8] * p[k] * dd;
                }
            }
        }

        return L;
    }

    SH9Color env_to_sph(const ::DirectX::ScratchImage &image)
    {
        SH9Color L;
        for (int F = 0; F < 6; ++F)
        {
            L += env_to_sph_face(image.GetImages()[F], F);
        }

        return L;
    }

    Math::Float4x4 irrmatrix(float Lzz, float Lpn, float Lpz,
                             float Lpp, float Lqm, float Lqn,
                             float Lqz, float Lqp, float Lqq)
    {
        const float c1 = 0.429043f;
        const float c2 = 0.511664f;
        const float c3 = 0.743125f;
        const float c4 = 0.886227f;
        const float c5 = 0.247708f;

        return Math::Float4x4(
        {
            c1 * Lqq, c1 * Lqm, c1 * Lqp, c2 * Lpp,
            c1 * Lqm,-c1 * Lqq, c1 * Lqn, c2 * Lpn,
            c1 * Lqp, c1 * Lqn, c3 * Lqz, c2 * Lpz,
            c2 * Lpp, c2 * Lpn, c2 * Lpz, c4 * Lzz - c5 * Lqz,
        });
    }

    void sph_to_irr_face(const ::DirectX::Image &image, const Math::Float4x4 &M, int F, int k)
    {
        int N = image.width;

        /* Iterate over all pixels of the current cube face. */
        for (int ypos = 0; ypos < N; ++ypos)
        {
            for (int xpos = 0; xpos < N; ++xpos)
            {
                /* Compute the direction vector of the current pixel. */
                float y = (2.0f * (ypos + 0.5f) - float(N)) / float(N);
                float x = (2.0f * (xpos + 0.5f) - float(N)) / float(N);

                Math::Float3 v = getVector(F, x, y, 1.0f);

                /* Compute the irradiance. */
                Math::Float3 w = (M * v.w(1.0f)).xyz;
                image.pixels[(ypos * image.rowPitch) + (xpos * 4) + k] = unsigned char(v.dot(w) * 255.0f);
            }
        }
    }

    SH9Color sph_to_irr(const SH9Color &L, int N)
    {
        ::DirectX::ScratchImage image;
        image.InitializeCube(DXGI_FORMAT_R8G8B8A8_UNORM, N, N, 1, 0);

        /* Iterate over all cube faces. */
        for (int F = 0; F < 6; ++F)
        {
            /* Iterate over all channels. */
            for (int k = 0; k < 3; ++k)
            {
                /* Generate the irradiance matrix for the current channel. */
                Math::Float4x4 M = irrmatrix(L[0][k], L[1][k], L[2][k],
                                             L[3][k], L[4][k], L[5][k],
                                             L[6][k], L[7][k], L[8][k]);

                /* Render the current cube face using the current irradiance. */
                sph_to_irr_face(image.GetImages()[F], M, F, k);
            }
        }

        ::DirectX::SaveToDDSFile(image.GetImages(), image.GetImageCount(), image.GetMetadata(), 0, L".//sh.dds");
        return env_to_sph(image);
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

        if (SUCCEEDED(resultValue))
        {
            Gek::SH9Color sh = Gek::env_to_sph(image);

            Gek::SH9Color ish = Gek::sph_to_irr(sh, 32);

            CStringA output;
            for (int i = 0; i < 9; i++)
            {
                output.AppendFormat("    sh.coefficients[%d] = float3(%f, %f, %f);\r\n", i, ish[i].x, ish[i].y, ish[i].z);
            }

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