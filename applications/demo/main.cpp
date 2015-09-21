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
#include "resource.h"

namespace Gek
{
    namespace FastMath
    {
        typedef __m128 Vector;

        struct __declspec(align(16)) Matrix
        {
            Vector rows[4];
        };

        struct Float2
        {
            union
            {
                struct { float x, y; };
                struct { float u, v; };
                struct { float data[2]; };
            };

            Float2(void)
                : x(0.0f)
                , y(0.0f)
            {
            }

            void operator = (Vector vector)
            {
                float vectorData[4];
                _mm_store_ps(vectorData, vector);
                x = vectorData[0];
                y = vectorData[1];
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
                : x(0.0f)
                , y(0.0f)
                , z(0.0f)
            {
            }

            void operator = (Vector vector)
            {
                float vectorData[4];
                _mm_store_ps(vectorData, vector);
                x = vectorData[0];
                y = vectorData[1];
                z = vectorData[2];
            }
        };

        struct Float4
        {
            union
            {
                struct { float x, y, w, z; };
                struct { float r, g, b, a; };
                struct { float nx, ny, nz, d; };
                struct { float data[4]; };
            };

            Float4(void)
                : x(0.0f)
                , y(0.0f)
                , z(0.0f)
                , w(0.0f)
            {
            }

            Float4(Vector vector)
            {
                _mm_store_ps(data, vector);
            }

            void operator = (Vector vector)
            {
                _mm_store_ps(data, vector);
            }
        };

        struct Float4x4
        {
            union
            {
                struct { Float4 rows[4]; };
                struct { float data[16]; };
            };

            Float4x4(void)
            {
            }

            Float4x4(const Matrix &value)
                : rows{ Float4(value.rows[0]), Float4(value.rows[1]), Float4(value.rows[2]), Float4(value.rows[3]) }
            {
            }

            void operator = (const Matrix &value)
            {
                rows[0] = value.rows[0]; // converts __m128 to float4
                rows[1] = value.rows[1]; // converts __m128 to float4
                rows[2] = value.rows[2]; // converts __m128 to float4
                rows[3] = value.rows[3]; // converts __m128 to float4
            }
        };

        Vector setVector(float value)
        {
            return _mm_set_ps1(value);
        }

        Vector setVector(const Float2 &value)
        {
            return _mm_set_ps(value.x, value.y, 0.0f, 0.0f);
        }

        Vector setVector(const Float3 &value)
        {
            return _mm_set_ps(value.x, value.y, value.y, 0.0f);
        }

        Vector setVector(const Float4 &value)
        {
            return _mm_loadu_ps(value.data);
        }

        Vector setVector(float x, float y, float z, float w)
        {
            return _mm_set_ps(x, y, z, w);
        }

        Vector setVector(const float (&data) [4])
        {
            return _mm_loadu_ps(data);
        }

        Matrix setMatrix(const Float4x4 &value)
        {
            return{ setVector(value.rows[0]), setVector(value.rows[1]), setVector(value.rows[2]), setVector(value.rows[3]) };
        }

        Matrix setMatrix(const Float4 data[4])
        {
            return{ setVector(data[0]), setVector(data[1]), setVector(data[2]), setVector(data[3]) };
        }

        Matrix setMatrix(const float (&data) [16])
        {
            return{ _mm_loadu_ps(data), _mm_loadu_ps(data + 4), _mm_loadu_ps(data + 8), _mm_loadu_ps(data + 12) };
        }

        Vector squareRoot(Vector value)
        {
            return _mm_sqrt_ps(value);
        }

        Matrix transpose(const Matrix &matrix)
        {
            Vector rows[4] =
            {
                _mm_unpacklo_ps(matrix.rows[0], matrix.rows[1]),
                _mm_unpacklo_ps(matrix.rows[2], matrix.rows[3]),
                _mm_unpackhi_ps(matrix.rows[0], matrix.rows[1]),
                _mm_unpackhi_ps(matrix.rows[2], matrix.rows[3]),
            };

            return
            {
                _mm_unpacklo_ps(rows[0], rows[1]),
                _mm_unpackhi_ps(rows[0], rows[1]),
                _mm_unpacklo_ps(rows[2], rows[3]),
                _mm_unpackhi_ps(rows[2], rows[3]),
            };
        }

        float getX(Vector value)
        {
            Float4 output;
            _mm_store_ps(output.data, value);
            return output.x;
        }

        float getY(Vector value)
        {
            Float4 output;
            _mm_store_ps(output.data, value);
            return output.y;
        }

        float getZ(Vector value)
        {
            Float4 output;
            _mm_store_ps(output.data, value);
            return output.z;
        }

        float getW(Vector value)
        {
            Float4 output;
            _mm_store_ps(output.data, value);
            return output.w;
        }

        Vector add(Vector left, Vector right)
        {
            return _mm_add_ps(left, right);
        }

        Vector subtract(Vector left, Vector right)
        {
            return _mm_sub_ps(left, right);
        }

        Vector multiply(Vector left, Vector right)
        {
            return _mm_mul_ps(left, right);
        }
        /*
        Vector multiply(const Matrix &left, Vector right)
        {
        }
        */
        Matrix multiply(const Matrix &left, const Matrix &right)
        {
            Matrix result;
            for (UINT32 rowIndex = 0; rowIndex < 4; rowIndex++)
            {
                __m128 xxxx = _mm_shuffle_ps(left.rows[rowIndex], left.rows[rowIndex], _MM_SHUFFLE(0, 0, 0, 0));
                __m128 yyyy = _mm_shuffle_ps(left.rows[rowIndex], left.rows[rowIndex], _MM_SHUFFLE(1, 1, 1, 1));
                __m128 zzzz = _mm_shuffle_ps(left.rows[rowIndex], left.rows[rowIndex], _MM_SHUFFLE(2, 2, 2, 2));
                __m128 wwww = _mm_shuffle_ps(left.rows[rowIndex], left.rows[rowIndex], _MM_SHUFFLE(3, 3, 3, 3));
                result.rows[rowIndex] = _mm_add_ps(
                    _mm_add_ps(
                        _mm_mul_ps(xxxx, right.rows[0]),
                        _mm_mul_ps(yyyy, right.rows[1])),
                    _mm_add_ps(
                        _mm_mul_ps(zzzz, right.rows[2]),
                        _mm_mul_ps(wwww, right.rows[3])));
            }

            return result;
        }

        Vector divide(Vector left, Vector right)
        {
            return _mm_div_ps(left, right);
        }

        Vector dot(Vector left, Vector right)
        {
            Vector multiply = _mm_mul_ps(left, right);
            Vector shuffle0 = _mm_shuffle_ps(multiply, multiply, _MM_SHUFFLE(1, 0, 3, 2));
            Vector add = _mm_add_ps(multiply, shuffle0);
            Vector shuffle2 = _mm_shuffle_ps(add, add, _MM_SHUFFLE(2, 3, 0, 1));
            return _mm_add_ps(shuffle2, add);
        }

        Vector dot3(Vector left, Vector right)
        {
            Vector multiply = _mm_mul_ps(left, right);
            Vector shuffle0 = _mm_shuffle_ps(multiply, multiply, _MM_SHUFFLE(1, 0, 0, 0));
            Vector shuffle2 = _mm_shuffle_ps(multiply, multiply, _MM_SHUFFLE(2, 0, 0, 0));
            Vector add = _mm_add_ps(multiply, _mm_add_ps(shuffle0, shuffle2));
            return _mm_shuffle_ps(add, add, _MM_SHUFFLE(0, 0, 0, 0));
        }

        Vector cross3(Vector left, Vector right)
        {
            const uint32_t maskYZX = _MM_SHUFFLE(1, 2, 0, 0);
            const uint32_t maskZXY = _MM_SHUFFLE(2, 0, 1, 0);
            Vector shuffle0 = _mm_shuffle_ps(left, left, maskYZX);
            Vector shuffle1 = _mm_shuffle_ps(right, right, maskZXY);
            Vector multiply0 = _mm_mul_ps(shuffle0, shuffle1); // (y1*z2), (z1*x2), (x1*y2), (x1*x2)
            shuffle0 = _mm_shuffle_ps(left, left, maskZXY);
            shuffle1 = _mm_shuffle_ps(right, right, maskYZX);
            Vector multiple1 = _mm_mul_ps(shuffle0, shuffle1); // (z1*y2), (x1*z2), (y1*x2), (x1*x2)
            return _mm_sub_ps(multiply0, multiple1);
        }
    };
};

Gek::FastMath::Vector operator + (Gek::FastMath::Vector left, Gek::FastMath::Vector right)
{
    return _mm_add_ps(left, right);
}

Gek::FastMath::Vector operator - (Gek::FastMath::Vector left, Gek::FastMath::Vector right)
{
    return _mm_sub_ps(left, right);
}

Gek::FastMath::Vector operator * (Gek::FastMath::Vector left, Gek::FastMath::Vector right)
{
    return _mm_mul_ps(left, right);
}

Gek::FastMath::Vector operator / (Gek::FastMath::Vector left, Gek::FastMath::Vector right)
{
    return _mm_div_ps(left, right);
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
    const UINT32 trialsCount = 100;
    const UINT32 cyclesCount = 100000;

    static const float identity[16] =
    { 
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    };

    UINT32 start;
    std::vector<UINT32> times[3];
    for (UINT32 average = 0; average < trialsCount; average++)
    {
        start = GetTickCount();
        for (UINT32 cycle = 0; cycle < cyclesCount; cycle++)
        {
            /*
            Gek::Math::Float4 value(1.0f, 2.0f, 3.0f, 4.0f);
            value = value + value;
            value = value - value;
            value = value * value;
            value = value / value;
            auto dot = value.dot(value);
            auto dot3 = Gek::Math::Float3(value).dot(value);
            auto cross3 = Gek::Math::Float3(value).cross(value);
            */
            Gek::Math::Float4x4 matrix(identity);
            auto transpose = matrix.getTranspose();
        }

        times[0].push_back(GetTickCount() - start);

        start = GetTickCount();
        for (UINT32 cycle = 0; cycle < cyclesCount; cycle++)
        {
            /*
            Gek::FastMath::Vector value = Gek::FastMath::setVector(1.0f, 2.0f, 3.0f, 4.0f);
            value = Gek::FastMath::add(value, value);
            value = Gek::FastMath::subtract(value, value);
            value = Gek::FastMath::multiply(value, value);
            value = Gek::FastMath::divide(value, value);
            auto dot = Gek::FastMath::dot(value, value);
            auto dot3 = Gek::FastMath::dot3(value, value);
            auto cross3 = Gek::FastMath::cross3(value, value);
            */
            Gek::FastMath::Matrix matrix = Gek::FastMath::setMatrix(identity);
            auto transpose = Gek::FastMath::transpose(matrix);
        }

        times[1].push_back(GetTickCount() - start);

        start = GetTickCount();
        for (UINT32 cycle = 0; cycle < cyclesCount; cycle++)
        {
            /*
            Gek::FastMath::Vector value = Gek::FastMath::setVector(1.0f, 2.0f, 3.0f, 4.0f);
            value = value + value;
            value = value - value;
            value = value * value;
            value = value / value;
            auto dot = Gek::FastMath::dot(value, value);
            auto dot3 = Gek::FastMath::dot3(value, value);
            auto cross3 = Gek::FastMath::cross3(value, value);
            */
            Gek::FastMath::Matrix matrix = Gek::FastMath::setMatrix(identity);
            auto transpose = Gek::FastMath::transpose(matrix);
        }

        times[2].push_back(GetTickCount() - start);
    }

    for (UINT32 type = 0; type < 3; type++)
    {
        UINT64 sum = 0;
        for (auto time : times[type])
        {
            sum += time;
        }

        UINT32 average = (sum / times[type].size());

        CStringA message;
        message.AppendFormat("Average(%d)\r\n", average);
        OutputDebugStringA(message);
    }

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