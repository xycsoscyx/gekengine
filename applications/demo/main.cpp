#include <initguid.h>
#include <cguid.h>

#include "GEK\Utility\Display.h"
#include "GEK\Utility\FileSystem.h"
#include "GEK\Utility\String.h"
#include "GEK\Utility\XML.h"
#include "GEK\Context\Common.h"
#include "GEK\Context\Interface.h"
#include "GEK\Engine\CoreInterface.h"
#include "GEK\Math\Matrix4x4.h"
#include <CommCtrl.h>
#include <xmmintrin.h>
#include <array>
#include "resource.h"

#define GEKINLINE __forceinline

#define BUILD_BUG_ON(condition) ((void)sizeof(char[1 - 2*!!(condition)]))

namespace Gek
{
    namespace FastMath
    {
        typedef __m128 Vector;

        typedef std::array<Vector, 4> Matrix;

        struct Float2
        {
            union
            {
                struct { float x, y; };
                struct { float u, v; };
                struct { float data[2]; };
            };

            Float2(void)
                : data{ 0.0f, 0.0f }
            {
            }

            void operator = (Vector vector)
            {
                x = vector.m128_f32[0];
                y = vector.m128_f32[1];
            }
        };

        struct Float3
        {
            union
            {
                struct { float x, y, z; };
                struct { float r, g, b; };
                struct { float data[3]; };
            };

            Float3(void)
                : data{ 0.0f, 0.0f, 0.0f }
            {
            }

            void operator = (Vector vector)
            {
                x = vector.m128_f32[0];
                y = vector.m128_f32[1];
                z = vector.m128_f32[2];
            }
        };

        struct __declspec(align(16)) Float4
        {
            union
            {
                struct { float x, y, w, z; };
                struct { float r, g, b, a; };
                struct { Float3 normal; float distance; };
                struct { float data[4]; };
                struct { Vector vector; };
            };

            Float4(void)
                : data{ 0.0f, 0.0f, 0.0f, 0.0f }
            {
            }

            Float4(Vector vector)
                : vector(vector)
            {
            }

            void operator = (Vector vector)
            {
                this->vector = vector;
            }
        };

        struct Float4x4
        {
            union
            {
                struct { Float4 rows[4]; };
                struct { float data[16]; };
                struct { Matrix matrix; };
            };

            Float4x4(void)
            {
            }

            Float4x4(const Matrix &value)
                : matrix{ value[0], value[1], value[2], value[3] }
            {
            }

            void operator = (const Matrix &value)
            {
                matrix[0] = value[0];
                matrix[1] = value[1];
                matrix[2] = value[2];
                matrix[3] = value[3];
            }
        };

        GEKINLINE Vector setVector(float value)
        {
            return _mm_set_ps1(value);
        }

        GEKINLINE Vector setVector(float x, float y, float z, float w)
        {
            return _mm_setr_ps(x, y, z, w);
        }

        GEKINLINE Vector setVector(const float (&data) [4])
        {
            return _mm_loadu_ps(data);
        }

        GEKINLINE Vector setVector(const Float4 &value)
        {
            return _mm_setr_ps(value.x, value.y, value.z, value.w);
        }

        GEKINLINE Matrix setMatrix(const Float4x4 &value)
        {
            return{ setVector(value.rows[0]), setVector(value.rows[1]), setVector(value.rows[2]), setVector(value.rows[3]) };
        }

        GEKINLINE Matrix setMatrix(const Float4 data[4])
        {
            return{ setVector(data[0]), setVector(data[1]), setVector(data[2]), setVector(data[3]) };
        }

        GEKINLINE Matrix setMatrix(const float (&data) [16])
        {
            return{ _mm_loadu_ps(data), _mm_loadu_ps(data + 4), _mm_loadu_ps(data + 8), _mm_loadu_ps(data + 12) };
        }

        GEKINLINE Vector sqrt(Vector value)
        {
            return _mm_sqrt_ps(value);
        }

        GEKINLINE Matrix transpose(const Matrix &matrix)
        {
            Matrix unpacked =
            {
                _mm_unpacklo_ps(matrix[0], matrix[2]),
                _mm_unpackhi_ps(matrix[0], matrix[2]),
                _mm_unpacklo_ps(matrix[1], matrix[3]),
                _mm_unpackhi_ps(matrix[1], matrix[3]),
            };

            return
            {
                _mm_unpacklo_ps(unpacked[0], unpacked[2]),
                _mm_unpackhi_ps(unpacked[0], unpacked[2]),
                _mm_unpacklo_ps(unpacked[1], unpacked[3]),
                _mm_unpackhi_ps(unpacked[1], unpacked[3]),
            };
        }

        GEKINLINE Vector add(Vector left, Vector right)
        {
            return _mm_add_ps(left, right);
        }

        GEKINLINE Vector subtract(Vector left, Vector right)
        {
            return _mm_sub_ps(left, right);
        }

        GEKINLINE Vector multiply(Vector left, Vector right)
        {
            return _mm_mul_ps(left, right);
        }
        /*
        GEKINLINE Vector multiply(const Matrix &left, Vector right)
        {
        }
        */
        GEKINLINE Matrix multiply(const Matrix &left, const Matrix &right)
        {
            Matrix result;
            for (UINT32 rowIndex = 0; rowIndex < 4; rowIndex++)
            {
                Matrix shuffled =
                {
                    _mm_shuffle_ps(left[rowIndex], left[rowIndex], _MM_SHUFFLE(0, 0, 0, 0)),
                    _mm_shuffle_ps(left[rowIndex], left[rowIndex], _MM_SHUFFLE(1, 1, 1, 1)),
                    _mm_shuffle_ps(left[rowIndex], left[rowIndex], _MM_SHUFFLE(2, 2, 2, 2)),
                    _mm_shuffle_ps(left[rowIndex], left[rowIndex], _MM_SHUFFLE(3, 3, 3, 3)),
                };

                result[rowIndex] = _mm_add_ps(
                    _mm_add_ps(
                        _mm_mul_ps(shuffled[0], right[0]),
                        _mm_mul_ps(shuffled[1], right[1])),
                    _mm_add_ps(
                        _mm_mul_ps(shuffled[2], right[2]),
                        _mm_mul_ps(shuffled[3], right[3])));
            }

            return result;
        }

        GEKINLINE Vector divide(Vector left, Vector right)
        {
            return _mm_div_ps(left, right);
        }

        GEKINLINE Vector dot3(Vector left, Vector right)
        {
            Vector multiply = _mm_mul_ps(left, right); // lx*rx, ly*ry, lz*rz, lw*rw
            Vector Y = _mm_shuffle_ps(multiply, multiply, _MM_SHUFFLE(0, 0, 0, 1)); // get lyry in a
            Vector Z = _mm_shuffle_ps(multiply, multiply, _MM_SHUFFLE(0, 0, 0, 2)); // get lzrz in a
            Vector addYZ = _mm_add_ps(Y, Z); // lyry+lzrz
            Vector addXYZ = _mm_add_ps(multiply, addYZ); // xx+(lyry+lzrz)
            return _mm_shuffle_ps(addXYZ, addXYZ, _MM_SHUFFLE(0, 0, 0, 0)); // shuffle (lxrx+lyry+lzrz) to all four
        }

        GEKINLINE Vector dot(Vector left, Vector right)
        {
            Vector multiply = _mm_mul_ps(left, right); // lx*rx, ly*ry, lz*rz, lw*rw
            Vector YXZW = _mm_shuffle_ps(multiply, multiply, _MM_SHUFFLE(2, 3, 0, 1)); // ry, rx, lw, lz
            Vector add = _mm_add_ps(multiply, YXZW);
            Vector ZWYX = _mm_shuffle_ps(add, add, _MM_SHUFFLE(0, 1, 2, 3)); // rz, rw, ly, lx
            return _mm_add_ps(ZWYX, add);
        }

        GEKINLINE Vector cross3(Vector left, Vector right)
        {
            Vector shuffle0 = _mm_shuffle_ps(left, left, _MM_SHUFFLE(0, 0, 2, 1));
            Vector shuffle1 = _mm_shuffle_ps(right, right, _MM_SHUFFLE(0, 1, 0, 2));
            Vector multiply0 = _mm_mul_ps(shuffle0, shuffle1); // (y1*z2), (z1*x2), (x1*y2), (x1*x2)
            shuffle0 = _mm_shuffle_ps(left, left, _MM_SHUFFLE(0, 1, 0, 2));
            shuffle1 = _mm_shuffle_ps(right, right, _MM_SHUFFLE(0, 0, 2, 1));
            Vector multiply1 = _mm_mul_ps(shuffle0, shuffle1); // (z1*y2), (x1*z2), (y1*x2), (x1*x2)
            return _mm_sub_ps(multiply0, multiply1);
        }

        GEKINLINE Vector length3(Vector vector)
        {
            return sqrt(dot3(vector, vector));
        }

        GEKINLINE Vector length(Vector vector)
        {
            return sqrt(dot(vector, vector));
        }

        void TestFastMath(void)
        {
            BUILD_BUG_ON(sizeof(Vector) != 16);
            BUILD_BUG_ON(sizeof(Matrix) != 64);

            Vector vector = setVector({ 1.0f, 2.0f, 4.0f, 4.0f });

            Vector xAxis = setVector(1.0f, 0.0f, 0.0f, 0.0f);
            Vector yAxis = setVector(0.0f, 1.0f, 0.0f, 0.0f);
            Vector zAxis = cross3(xAxis, yAxis);

            Vector vectorA = setVector(1.0f, 2.0f, 3.0f, 4.0f);
            Vector vectorB = setVector(5.0f, 6.0f, 7.0f, 8.0f);
            Vector cross3Result = cross3(vectorA, vectorB);

            Vector length3Result = length3(vectorA);
            Vector lengthResult = length(vectorA);
            Vector dot3Result = dot3(vectorA, vectorB);
            Vector dotResult = dot(vectorA, vectorB);
            Vector addResult = add(vectorA, vectorB);
            Vector subtractResult = subtract(vectorA, vectorB);
            Vector multiplyResult = multiply(vectorA, vectorB);
            Vector divideResult = divide(vectorA, vectorB);

            static const float identityData[16] =
            {
                1.0f, 0.0f, 0.0f, 0.0f,
                0.0f, 1.0f, 0.0f, 0.0f,
                0.0f, 0.0f, 1.0f, 0.0f,
                0.0f, 0.0f, 0.0f, 1.0f
            };

            Matrix identity = setMatrix(identityData);

            static const float scaleData[16] = 
            {
                1.0f, 0.0f, 0.0f, 0.0f,
                0.0f, 2.0f, 0.0f, 0.0f,
                0.0f, 0.0f, 3.0f, 0.0f,
                4.0f, 5.0f, 6.0f, 1.0f,
            };

            Matrix scale = setMatrix(scaleData);

            Matrix multiplyResult1 = multiply(identity, scale);
            Matrix multiplyResult2 = multiply(scale, identity);
            Matrix multiplyResult3 = multiply(scale, scale);
            
            static const float fullData[16] =
            {
                 1.0f,  2.0f,  3.0f,  4.0f,
                 5.0f,  6.0f,  7.0f,  8.0f,
                 9.0f, 10.0f, 11.0f, 12.0f,
                13.0f, 14.0f, 15.0f, 16.0f,
            };

            Matrix full = setMatrix(fullData);
            Matrix transposeResult = transpose(full);
            Matrix multiplyResult4 = multiply(full, transposeResult);
        }
    };
};

GEKINLINE Gek::FastMath::Vector operator + (Gek::FastMath::Vector left, Gek::FastMath::Vector right)
{
    return _mm_add_ps(left, right);
}

GEKINLINE Gek::FastMath::Vector operator - (Gek::FastMath::Vector left, Gek::FastMath::Vector right)
{
    return _mm_sub_ps(left, right);
}

GEKINLINE Gek::FastMath::Vector operator * (Gek::FastMath::Vector left, Gek::FastMath::Vector right)
{
    return _mm_mul_ps(left, right);
}

GEKINLINE Gek::FastMath::Vector operator / (Gek::FastMath::Vector left, Gek::FastMath::Vector right)
{
    return _mm_div_ps(left, right);
}

GEKINLINE Gek::FastMath::Matrix operator * (const Gek::FastMath::Matrix &left, const Gek::FastMath::Matrix &right)
{
    Gek::FastMath::Matrix result;
    for (UINT32 rowIndex = 0; rowIndex < 4; rowIndex++)
    {
        Gek::FastMath::Matrix shuffled =
        {
            _mm_shuffle_ps(left[rowIndex], left[rowIndex], _MM_SHUFFLE(0, 0, 0, 0)),
            _mm_shuffle_ps(left[rowIndex], left[rowIndex], _MM_SHUFFLE(1, 1, 1, 1)),
            _mm_shuffle_ps(left[rowIndex], left[rowIndex], _MM_SHUFFLE(2, 2, 2, 2)),
            _mm_shuffle_ps(left[rowIndex], left[rowIndex], _MM_SHUFFLE(3, 3, 3, 3)),
        };

        result[rowIndex] = _mm_add_ps(
            _mm_add_ps(
                _mm_mul_ps(shuffled[0], right[0]),
                _mm_mul_ps(shuffled[1], right[1])),
            _mm_add_ps(
                _mm_mul_ps(shuffled[2], right[2]),
                _mm_mul_ps(shuffled[3], right[3])));
    }

    return result;
}

INT_PTR CALLBACK DialogProc(HWND dialog, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_CLOSE:
        EndDialog(dialog, IDCANCEL);
        return TRUE;

    case WM_INITDIALOG:
        {
            UINT32 width = 800;
            UINT32 height = 600;
            bool windowed = true;

            Gek::Xml::Document xmlDocument;
            if (SUCCEEDED(xmlDocument.load(L"%root%\\config.xml")))
            {
                Gek::Xml::Node xmlConfigNode = xmlDocument.getRoot();
                if (xmlConfigNode && xmlConfigNode.getType().CompareNoCase(L"config") == 0 && xmlConfigNode.hasChildElement(L"display"))
                {
                    Gek::Xml::Node xmlDisplayNode = xmlConfigNode.firstChildElement(L"display");
                    if (xmlDisplayNode)
                    {
                        if (xmlDisplayNode.hasAttribute(L"width"))
                        {
                            width = Gek::String::getUINT32(xmlDisplayNode.getAttribute(L"width"));
                        }
                        
                        if (xmlDisplayNode.hasAttribute(L"height"))
                        {
                            height = Gek::String::getUINT32(xmlDisplayNode.getAttribute(L"height"));
                        }
                        
                        if (xmlDisplayNode.hasAttribute(L"windowed"))
                        {
                            windowed = Gek::String::getBoolean(xmlDisplayNode.getAttribute(L"windowed"));
                        }
                    }
                }
            }

            UINT32 selectIndex = 0;
            SendDlgItemMessage(dialog, IDC_MODES, CB_RESETCONTENT, 0, 0);
            std::vector<Gek::Display::Mode> modeList = Gek::Display::getModes()[32];
            for(auto &mode : modeList)
            {
                CStringW aspectRatio(L"");
                switch (mode.aspectRatio)
                {
                case Gek::Display::AspectRatio::_4x3:
                    aspectRatio = L", (4x3)";
                    break;

                case Gek::Display::AspectRatio::_16x9:
                    aspectRatio = L", (16x9)";
                    break;

                case Gek::Display::AspectRatio::_16x10:
                    aspectRatio = L", (16x10)";
                    break;
                };

                CStringW modeString;
                modeString.Format(L"%dx%d%s", mode.width, mode.height, aspectRatio.GetString());
                int modeIndex = SendDlgItemMessage(dialog, IDC_MODES, CB_ADDSTRING, 0, (WPARAM)modeString.GetString());
                if (mode.width == width && mode.height == height)
                {
                    selectIndex = modeIndex;
                }
            }

            SendDlgItemMessage(dialog, IDC_FULLSCREEN, BM_SETCHECK, windowed ? BST_UNCHECKED : BST_CHECKED, 0);

            SendDlgItemMessage(dialog, IDC_MODES, CB_SETMINVISIBLE, 5, 0);
            SendDlgItemMessage(dialog, IDC_MODES, CB_SETEXTENDEDUI, TRUE, 0);
            SendDlgItemMessage(dialog, IDC_MODES, CB_SETCURSEL, selectIndex, 0);
            return TRUE;
        }

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDOK:
            {
                std::vector<Gek::Display::Mode> modeList = Gek::Display::getModes()[32];
                UINT32 selectIndex = SendDlgItemMessage(dialog, IDC_MODES, CB_GETCURSEL, 0, 0);
                auto &mode = modeList[selectIndex];

                Gek::Xml::Document xmlDocument;
                xmlDocument.load(L"%root%\\config.xml");
                Gek::Xml::Node xmlConfigNode = xmlDocument.getRoot();
                if (!xmlConfigNode || xmlConfigNode.getType().CompareNoCase(L"config") != 0)
                {
                    xmlDocument.create(L"config");
                    xmlConfigNode = xmlDocument.getRoot();
                }

                Gek::Xml::Node xmlDisplayNode = xmlConfigNode.firstChildElement(L"display");
                if (!xmlDisplayNode)
                {
                    xmlDisplayNode = xmlConfigNode.createChildElement(L"display");
                }

                xmlDisplayNode.setAttribute(L"width", L"%d", mode.width);
                xmlDisplayNode.setAttribute(L"height", L"%d", mode.height);
                xmlDisplayNode.setAttribute(L"windowed", L"%s", (SendDlgItemMessage(dialog, IDC_FULLSCREEN, BM_GETCHECK, 0, 0) == BST_UNCHECKED ? L"true" : L"false"));
                xmlDocument.save(L"%root%\\config.xml");

                EndDialog(dialog, IDOK);
                return TRUE;
            }

        case IDCANCEL:
            EndDialog(dialog, IDCANCEL);
            return TRUE;
        };

        return TRUE;
    };

    return FALSE;
}

LRESULT CALLBACK WindowProc(HWND window, UINT32 message, WPARAM wParam, LPARAM lParam)
{
    LRESULT resultValue = 0;
    Gek::Engine::Core::Interface *engineCore = (Gek::Engine::Core::Interface *)GetWindowLongPtr(window, GWLP_USERDATA);
    switch (message)
    {
    case WM_CLOSE:
    case WM_DESTROY:
        PostQuitMessage(0);
        break;

    default:
        if (engineCore)
        {
            resultValue = engineCore->windowEvent(message, wParam, lParam);
        }

        break;
    };

    if (resultValue == 0)
    {
        resultValue = DefWindowProc(window, message, wParam, lParam);
    }

    return resultValue;
}

int CALLBACK wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR strCommandLine, _In_ int nCmdShow)
{
    Gek::FastMath::TestFastMath();
    return 0;

/*
    OutputDebugString(Gek::String::format(L"Float2(%d)\r\n", sizeof(Gek::FastMath::Float2)));
    OutputDebugString(Gek::String::format(L"Float3(%d)\r\n", sizeof(Gek::FastMath::Float3)));
    OutputDebugString(Gek::String::format(L"Float4(%d)\r\n", sizeof(Gek::FastMath::Float4)));
    OutputDebugString(Gek::String::format(L"Float4x4(%d)\r\n", sizeof(Gek::FastMath::Float4x4)));
    OutputDebugString(Gek::String::format(L"Vector(%d)\r\n", sizeof(Gek::FastMath::Vector)));
    OutputDebugString(Gek::String::format(L"Matrix(%d)\r\n", sizeof(Gek::FastMath::Matrix)));

    const UINT32 cyclesCount = 1000000;

    static const float identity[16] =
    {
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    };

    auto start = GetTickCount();
    for (UINT32 cycle = 0; cycle < cyclesCount; cycle++)
    {
        Gek::Math::Float4 value(1.0f, 2.0f, 3.0f, 4.0f);
        value = value + value;
        value = value - value;
        value = value * value;
        value = value / value;
        auto dot = value.dot(value);
        auto dot3 = Gek::Math::Float3(value).dot(value);
        auto cross3 = Gek::Math::Float3(value).cross(value);
        Gek::Math::Float4x4 matrix(identity);
        auto transpose = matrix.getTranspose();
        auto multiply = matrix * transpose;
    }

    OutputDebugString(Gek::String::format(L"Base: %d\r\n", GetTickCount() - start));
    start = GetTickCount();
    for (UINT32 cycle = 0; cycle < cyclesCount; cycle++)
    {
        Gek::FastMath::Vector value = Gek::FastMath::setVector(1.0f, 2.0f, 3.0f, 4.0f);
        value = value + value;
        value = value - value;
        value = value * value;
        value = value / value;
        auto dot = Gek::FastMath::dot(value, value);
        auto dot3 = Gek::FastMath::dot3(value, value);
        auto cross3 = Gek::FastMath::cross3(value, value);
        Gek::FastMath::Matrix matrix = Gek::FastMath::setMatrix(identity);
        auto transpose = Gek::FastMath::transpose(matrix);
        auto multiply = matrix * transpose;
    }

    OutputDebugString(Gek::String::format(L"Operator: %d\r\n", GetTickCount() - start));
    start = GetTickCount();
    for (UINT32 cycle = 0; cycle < cyclesCount; cycle++)
    {
        Gek::FastMath::Vector value = Gek::FastMath::setVector(1.0f, 2.0f, 3.0f, 4.0f);
        value = Gek::FastMath::add(value, value);
        value = Gek::FastMath::subtract(value, value);
        value = Gek::FastMath::multiply(value, value);
        value = Gek::FastMath::divide(value, value);
        auto dot = Gek::FastMath::dot(value, value);
        auto dot3 = Gek::FastMath::dot3(value, value);
        auto cross3 = Gek::FastMath::cross3(value, value);
        Gek::FastMath::Matrix matrix = Gek::FastMath::setMatrix(identity);
        auto transpose = Gek::FastMath::transpose(matrix);
        auto multiply = Gek::FastMath::multiply(matrix, transpose);
    }

    OutputDebugString(Gek::String::format(L"Fast: %d\r\n", GetTickCount() - start));
    start = GetTickCount();
    return 0;
*/
    if (DialogBox(hInstance, MAKEINTRESOURCE(IDD_SETTINGS), nullptr, DialogProc) == IDOK)
    {
        CComPtr<Gek::Context::Interface> context;
        Gek::Context::create(&context);
        if (context)
        {
#ifdef _DEBUG
            SetCurrentDirectory(Gek::FileSystem::expandPath(L"%root%\\Debug"));
            context->addSearchPath(L"%root%\\Debug\\Plugins");
#else
            SetCurrentDirectory(GEKParseFileName(L"%root%\\Release"));
            context->AddSearchPath(GEKParseFileName(L"%root%\\Release\\Plugins"));
#endif

            context->initialize();
            CComPtr<Gek::Engine::Core::Interface> engineCore;
            context->createInstance(CLSID_IID_PPV_ARGS(Gek::Engine::Core::Class, &engineCore));
            if (engineCore)
            {
                WNDCLASS kClass;
                kClass.style = 0;
                kClass.lpfnWndProc = WindowProc;
                kClass.cbClsExtra = 0;
                kClass.cbWndExtra = 0;
                kClass.hInstance = GetModuleHandle(nullptr);
                kClass.hIcon = LoadIcon(GetModuleHandle(nullptr), MAKEINTRESOURCE(103));
                kClass.hCursor = nullptr;
                kClass.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
                kClass.lpszMenuName = nullptr;
                kClass.lpszClassName = L"GEKvX_Engine_314159";
                if (RegisterClass(&kClass))
                {
                    UINT32 width = 800;
                    UINT32 height = 600;
                    bool windowed = true;

                    Gek::Xml::Document xmlDocument;
                    if (SUCCEEDED(xmlDocument.load(L"%root%\\config.xml")))
                    {
                        Gek::Xml::Node xmlConfigNode = xmlDocument.getRoot();
                        if (xmlConfigNode && xmlConfigNode.getType().CompareNoCase(L"config") == 0 && xmlConfigNode.hasChildElement(L"display"))
                        {
                            Gek::Xml::Node xmlDisplayNode = xmlConfigNode.firstChildElement(L"display");
                            if (xmlDisplayNode)
                            {
                                if (xmlDisplayNode.hasAttribute(L"width"))
                                {
                                    width = Gek::String::getUINT32(xmlDisplayNode.getAttribute(L"width"));
                                }

                                if (xmlDisplayNode.hasAttribute(L"height"))
                                {
                                    height = Gek::String::getUINT32(xmlDisplayNode.getAttribute(L"height"));
                                }

                                if (xmlDisplayNode.hasAttribute(L"windowed"))
                                {
                                    windowed = Gek::String::getBoolean(xmlDisplayNode.getAttribute(L"windowed"));
                                }
                            }
                        }
                    }

                    RECT clientRect;
                    clientRect.left = 0;
                    clientRect.top = 0;
                    clientRect.right = width;
                    clientRect.bottom = height;
                    AdjustWindowRect(&clientRect, WS_OVERLAPPEDWINDOW, false);
                    int windowWidth = (clientRect.right - clientRect.left);
                    int windowHeight = (clientRect.bottom - clientRect.top);
                    int centerPositionX = (windowed ? (GetSystemMetrics(SM_CXFULLSCREEN) / 2) - ((clientRect.right - clientRect.left) / 2) : 0);
                    int centerPositionY = (windowed ? (GetSystemMetrics(SM_CYFULLSCREEN) / 2) - ((clientRect.bottom - clientRect.top) / 2) : 0);
                    HWND window = CreateWindow(L"GEKvX_Engine_314159", L"GEKvX Engine", WS_SYSMENU | WS_BORDER | WS_MINIMIZEBOX, centerPositionX, centerPositionY, windowWidth, windowHeight, 0, nullptr, GetModuleHandle(nullptr), 0);
                    if (window)
                    {
                        if (SUCCEEDED(engineCore->initialize(window)))
                        {
                            SetWindowLongPtr(window, GWLP_USERDATA, LONG((Gek::Engine::Core::Interface *)engineCore));
                            ShowWindow(window, SW_SHOW);
                            UpdateWindow(window);

                            context->logMessage(__FILE__, __LINE__, L"[entering] Game Loop");
                            context->logEnterScope();

                            MSG message = { 0 };
                            while (message.message != WM_QUIT)
                            {
                                while (PeekMessage(&message, nullptr, 0U, 0U, PM_REMOVE))
                                {
                                    TranslateMessage(&message);
                                    DispatchMessage(&message);
                                };

                                if (!engineCore->update())
                                {
                                    break;
                                }
                            };

                            context->logExitScope();
                            context->logMessage(__FILE__, __LINE__, L"[entering] Game Loop");

                            SetWindowLongPtr(window, GWLP_USERDATA, 0);
                            engineCore.Release();
                            DestroyWindow(window);
                        }
                    }
                    else
                    {
                        context->logMessage(__FILE__, __LINE__, L"Unable to create window: %d", GetLastError());
                    }
                }
                else
                {
                    context->logMessage(__FILE__, __LINE__, L"Unable to register window class: %d", GetLastError());
                }
            }
        }

        return 0;
    }

    return -1;
}