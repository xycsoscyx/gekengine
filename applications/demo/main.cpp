#include <initguid.h>
#include <cguid.h>

#include "GEK\Utility\Display.h"
#include "GEK\Utility\FileSystem.h"
#include "GEK\Utility\String.h"
#include "GEK\Utility\XML.h"
#include "GEK\Context\Common.h"
#include "GEK\Context\Interface.h"
#include "GEK\Engine\CoreInterface.h"
#include "GEK\Math\Common.h"
#include "GEK\Math\Matrix4x4.h"
#include <CommCtrl.h>
#include <xmmintrin.h>
#include <array>
#include "resource.h"

#define GEKINLINE __forceinline

#define BUILD_BUG_ON(condition) ((void)sizeof(char[1 - 2*!!(condition)]))

namespace Gek
{
    namespace MathSIMD
    {
        namespace Types
        {
            typedef __m128 Vector;
            typedef std::array<Vector, 4> Matrix;
        };

        namespace Vector
        {
            GEKINLINE Types::Vector set(float value)
            {
                return _mm_set_ps1(value);
            }

            GEKINLINE Types::Vector set(float x, float y, float z, float w)
            {
                return _mm_setr_ps(x, y, z, w);
            }

            GEKINLINE Types::Vector set(const float(&data)[4])
            {
                return _mm_loadu_ps(data);
            }

            GEKINLINE Types::Vector add(Types::Vector left, Types::Vector right)
            {
                return _mm_add_ps(left, right);
            }

            GEKINLINE Types::Vector subtract(Types::Vector left, Types::Vector right)
            {
                return _mm_sub_ps(left, right);
            }

            GEKINLINE Types::Vector multiply(Types::Vector left, Types::Vector right)
            {
                return _mm_mul_ps(left, right);
            }

            GEKINLINE Types::Vector divide(Types::Vector left, Types::Vector right)
            {
                return _mm_div_ps(left, right);
            }

            GEKINLINE Types::Vector dot(Types::Vector left, Types::Vector right)
            {
                Types::Vector multiply = _mm_mul_ps(left, right); // lx*rx, ly*ry, lz*rz, lw*rw
                Types::Vector YXZW = _mm_shuffle_ps(multiply, multiply, _MM_SHUFFLE(2, 3, 0, 1)); // ry, rx, lw, lz
                Types::Vector add = _mm_add_ps(multiply, YXZW);
                Types::Vector ZWYX = _mm_shuffle_ps(add, add, _MM_SHUFFLE(0, 1, 2, 3)); // rz, rw, ly, lx
                return _mm_add_ps(ZWYX, add);
            }

            GEKINLINE Types::Vector sqrt(Types::Vector value)
            {
                return _mm_sqrt_ps(value);
            }

            GEKINLINE Types::Vector length(Types::Vector vector)
            {
                return sqrt(dot(vector, vector));
            }

            GEKINLINE Types::Vector normalize(Types::Vector value)
            {
                return divide(value, length(value));
            }

            GEKINLINE Types::Vector equal(Types::Vector left, Types::Vector right)
            {
                return _mm_cmpeq_ps(left, right);
            }

            GEKINLINE Types::Vector notEqual(Types::Vector left, Types::Vector right)
            {
                return _mm_cmpneq_ps(left, right);
            }

            GEKINLINE Types::Vector less(Types::Vector left, Types::Vector right)
            {
                return _mm_cmplt_ps(left, right);
            }

            GEKINLINE Types::Vector lessEqual(Types::Vector left, Types::Vector right)
            {
                return _mm_cmple_ps(left, right);
            }

            GEKINLINE Types::Vector greater(Types::Vector left, Types::Vector right)
            {
                return _mm_cmpgt_ps(left, right);
            }

            GEKINLINE Types::Vector greaterEqual(Types::Vector left, Types::Vector right)
            {
                return _mm_cmpge_ps(left, right);
            }
        };

        namespace Vector3
        {
            GEKINLINE Types::Vector dot(Types::Vector left, Types::Vector right)
            {
                Types::Vector multiply = _mm_mul_ps(left, right); // lx*rx, ly*ry, lz*rz, lw*rw
                Types::Vector Y = _mm_shuffle_ps(multiply, multiply, _MM_SHUFFLE(0, 0, 0, 1)); // get lyry in a
                Types::Vector Z = _mm_shuffle_ps(multiply, multiply, _MM_SHUFFLE(0, 0, 0, 2)); // get lzrz in a
                Types::Vector addYZ = _mm_add_ps(Y, Z); // lyry+lzrz
                Types::Vector addXYZ = _mm_add_ps(multiply, addYZ); // xx+(lyry+lzrz)
                return _mm_shuffle_ps(addXYZ, addXYZ, _MM_SHUFFLE(0, 0, 0, 0)); // shuffle (lxrx+lyry+lzrz) to all four
            }

            GEKINLINE Types::Vector cross(Types::Vector left, Types::Vector right)
            {
                Types::Vector shuffle0 = _mm_shuffle_ps(left, left, _MM_SHUFFLE(0, 0, 2, 1));
                Types::Vector shuffle1 = _mm_shuffle_ps(right, right, _MM_SHUFFLE(0, 1, 0, 2));
                Types::Vector multiply0 = _mm_mul_ps(shuffle0, shuffle1); // (y1*z2), (z1*x2), (x1*y2), (x1*x2)
                shuffle0 = _mm_shuffle_ps(left, left, _MM_SHUFFLE(0, 1, 0, 2));
                shuffle1 = _mm_shuffle_ps(right, right, _MM_SHUFFLE(0, 0, 2, 1));
                Types::Vector multiply1 = _mm_mul_ps(shuffle0, shuffle1); // (z1*y2), (x1*z2), (y1*x2), (x1*x2)
                return _mm_sub_ps(multiply0, multiply1);
            }

            GEKINLINE Types::Vector length(Types::Vector vector)
            {
                return Vector::sqrt(dot(vector, vector));
            }

            GEKINLINE Types::Vector normalize(Types::Vector value)
            {
                return Vector::divide(value, length(value));
            }
        };

        namespace Quaternion
        {
            GEKINLINE Types::Vector set(const Types::Matrix &value)
            {
            }

            GEKINLINE Types::Vector multiply(Types::Vector left, Types::Vector right)
            {
            }
        };

        namespace Matrix
        {
            GEKINLINE Types::Matrix set(const float(&data)[16])
            {
                return{ _mm_loadu_ps(data), _mm_loadu_ps(data + 4), _mm_loadu_ps(data + 8), _mm_loadu_ps(data + 12) };
            }

            GEKINLINE Types::Matrix set(const Types::Vector(&data)[4])
            {
                return{ data[0], data[1], data[2], data[3] };
            }

            GEKINLINE Types::Matrix setEuler(float x, float y, float z)
            {
                float cosX(std::cos(x));
                float sinX(std::sin(x));
                float cosY(std::cos(y));
                float sinY(std::sin(y));
                float cosZ(std::cos(z));
                float sinZ(std::sin(z));
                float cosXsinY(cosX * sinY);
                float sinXsinY(sinX * sinY);

                return set({ ( cosY * cosZ), ( sinXsinY * cosZ + cosX * sinZ), (-cosXsinY * cosZ + sinX * sinZ), 0.0f,
                             (-cosY * sinZ), (-sinXsinY * sinZ + cosX * cosZ), ( cosXsinY * sinZ + sinX * cosZ), 0.0f,
                               sinY, (-sinX * cosY), (cosX * cosY), 0.0f,
                               0.0f, 0.0f, 0.0f, 1.0f, });
            }

            GEKINLINE Types::Matrix setQuaternion(Types::Vector rotation)
            {
                float xy(rotation.m128_f32[0] * rotation.m128_f32[1]);
                float zw(rotation.m128_f32[2] * rotation.m128_f32[3]);
                float xz(rotation.m128_f32[0] * rotation.m128_f32[2]);
                float yw(rotation.m128_f32[1] * rotation.m128_f32[3]);
                float yz(rotation.m128_f32[1] * rotation.m128_f32[2]);
                float xw(rotation.m128_f32[0] * rotation.m128_f32[3]);
                Types::Vector square = Vector::multiply(rotation, rotation);
                float determinant(1.0f / (square.m128_f32[0] + square.m128_f32[1] + square.m128_f32[2] + square.m128_f32[3]));

                return set({ ((square.m128_f32[0] - square.m128_f32[1] - square.m128_f32[2] + square.m128_f32[3]) * determinant), (2.0f * (xy + zw) * determinant), (2.0f * (xz - yw) * determinant), 0.0f,
                              (2.0f * (xy - zw) * determinant), ((-square.m128_f32[0] + square.m128_f32[1] - square.m128_f32[2] + square.m128_f32[3]) * determinant), (2.0f * (yz + xw) * determinant), 0.0f,
                              (2.0f * (xz + yw) * determinant), (2.0f * (yz - xw) * determinant), ((-square.m128_f32[0] - square.m128_f32[1] + square.m128_f32[2] + square.m128_f32[3]) * determinant), 0.0f,
                               0.0f, 0.0f, 0.0f, 1.0f, });
            }

            GEKINLINE Types::Matrix createPerspective(float fieldOfView, float aspectRatio, float nearDepth, float farDepth)
            {
                float x(1.0f / std::tan(fieldOfView * 0.5f));
                float y(x * aspectRatio);
                float distance(farDepth - nearDepth);

                return set({ x, 0.0f, 0.0f, 0.0f,
                             0.0f, y, 0.0f, 0.0f,
                             0.0f, 0.0f, ((farDepth + nearDepth) / distance), 1.0f,
                             0.0f, 0.0f, -((2.0f * farDepth * nearDepth) / distance), 0.0f, });
            }

            GEKINLINE Types::Matrix createOrthographic(float left, float top, float right, float bottom, float nearDepth, float farDepth)
            {
                return set({ (2.0f / (right - left)), 0.0f, 0.0f, 0.0f,
                              0.0f, (2.0f / (top - bottom)), 0.0f, 0.0f,
                              0.0f, 0.0f, (-2.0f / (farDepth - nearDepth)), 0.0f,
                    -((right + left) / (right - left)), -((top + bottom) / (top - bottom)), -((farDepth + nearDepth) / (farDepth - nearDepth)), 1.0f, });
            }

            GEKINLINE Types::Matrix inverse(const Types::Matrix &matrix)
            {
            }

            GEKINLINE Types::Matrix transpose(const Types::Matrix &matrix)
            {
                Types::Matrix unpacked =
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

            GEKINLINE Types::Vector multiply(const Types::Matrix &matrix, Types::Vector vector)
            {
                Types::Matrix shuffled =
                {
                    _mm_shuffle_ps(vector, vector, _MM_SHUFFLE(0, 0, 0, 0)),
                    _mm_shuffle_ps(vector, vector, _MM_SHUFFLE(1, 1, 1, 1)),
                    _mm_shuffle_ps(vector, vector, _MM_SHUFFLE(2, 2, 2, 2)),
                    _mm_shuffle_ps(vector, vector, _MM_SHUFFLE(3, 3, 3, 3)),
                };

                return _mm_add_ps(
                    _mm_add_ps(
                        _mm_mul_ps(shuffled[0], matrix[0]),
                        _mm_mul_ps(shuffled[1], matrix[1])),
                    _mm_add_ps(
                        _mm_mul_ps(shuffled[2], matrix[2]),
                        _mm_mul_ps(shuffled[3], matrix[3])));
            }

            GEKINLINE Types::Matrix multiply(const Types::Matrix &left, const Types::Matrix &right)
            {
                Types::Matrix result;
                for (UINT32 rowIndex = 0; rowIndex < 4; rowIndex++)
                {
                    Types::Matrix shuffled =
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
        };

        void test(void)
        {
            BUILD_BUG_ON(sizeof(Types::Vector) != 16);
            BUILD_BUG_ON(sizeof(Types::Matrix) != 64);

            Types::Vector vector = Vector::set({ 1.0f, 2.0f, 4.0f, 4.0f });

            Types::Vector xAxis = Vector::set(1.0f, 0.0f, 0.0f, 0.0f);
            Types::Vector yAxis = Vector::set(0.0f, 1.0f, 0.0f, 0.0f);
            Types::Vector zAxis = Vector3::cross(xAxis, yAxis);

            Types::Vector vectorA = Vector::set(1.0f, 2.0f, 3.0f, 4.0f);
            Types::Vector vectorB = Vector::set(5.0f, 6.0f, 7.0f, 8.0f);
            Types::Vector cross3Result = Vector3::cross(vectorA, vectorB);
            Types::Vector normalA = Vector::normalize(vectorA);
            Types::Vector normalA3 = Vector3::normalize(vectorA);

            Types::Vector lengthResult = Vector::length(vectorA);
            Types::Vector length3Result = Vector3::length(vectorA);
            Types::Vector dotResult = Vector::dot(vectorA, vectorB);
            Types::Vector dot3Result = Vector3::dot(vectorA, vectorB);
            Types::Vector addResult = Vector::add(vectorA, vectorB);
            Types::Vector subtractResult = Vector::subtract(vectorA, vectorB);
            Types::Vector multiplyResult = Vector::multiply(vectorA, vectorB);
            Types::Vector divideResult = Vector::divide(vectorA, vectorB);

            static const float identityData[16] =
            {
                1.0f, 0.0f, 0.0f, 0.0f,
                0.0f, 1.0f, 0.0f, 0.0f,
                0.0f, 0.0f, 1.0f, 0.0f,
                0.0f, 0.0f, 0.0f, 1.0f
            };

            Types::Matrix identity = Matrix::set(identityData);

            static const float scaleData[16] = 
            {
                1.0f, 0.0f, 0.0f, 0.0f,
                0.0f, 2.0f, 0.0f, 0.0f,
                0.0f, 0.0f, 3.0f, 0.0f,
                4.0f, 5.0f, 6.0f, 1.0f,
            };

            Types::Matrix scale = Matrix::set(scaleData);

            Types::Matrix multiplyResult1 = Matrix::multiply(identity, scale);
            Types::Matrix multiplyResult2 = Matrix::multiply(scale, identity);
            Types::Matrix multiplyResult3 = Matrix::multiply(scale, scale);
            
            static const float fullData[16] =
            {
                 1.0f,  2.0f,  3.0f,  4.0f,
                 5.0f,  6.0f,  7.0f,  8.0f,
                 9.0f, 10.0f, 11.0f, 12.0f,
                13.0f, 14.0f, 15.0f, 16.0f,
            };

            Types::Matrix full = Matrix::set(fullData);
            Types::Matrix transposeResult = Matrix::transpose(full);
            Types::Matrix multiplyResult4 = Matrix::multiply(full, transposeResult);
            
            Types::Vector base = Vector::set(1.0f, 1.0f, 1.0f, 1.0f);
            Types::Vector base3 = Vector::set(1.0f, 1.0f, 1.0f, 0.0f);
            Types::Vector multiplyBase = Matrix::multiply(scale, base);
            Types::Vector multiplyBase3 = Matrix::multiply(scale, base3);

            Types::Matrix eulerX = Matrix::setEuler(Gek::Math::convertDegreesToRadians(90.0f), 0.0f, 0.0f);
            Types::Matrix eulerY = Matrix::setEuler(0.0f, Gek::Math::convertDegreesToRadians(90.0f), 0.0f);
            Types::Matrix eulerZ = Matrix::setEuler(0.0f, 0.0f, Gek::Math::convertDegreesToRadians(90.0f));
            Types::Vector quaternion = Vector::set(0.0f, 0.0f, 0.0f, 1.0f);
            Types::Matrix matrix = Matrix::setQuaternion(quaternion);

            Types::Vector xxMultiplyResult = Matrix::multiply(eulerX, xAxis);
            Types::Vector xyMultiplyResult = Matrix::multiply(eulerX, yAxis);
            Types::Vector xzMultiplyResult = Matrix::multiply(eulerX, zAxis);
            Types::Vector yxMultiplyResult = Matrix::multiply(eulerY, xAxis);
            Types::Vector yyMultiplyResult = Matrix::multiply(eulerY, yAxis);
            Types::Vector yzMultiplyResult = Matrix::multiply(eulerY, zAxis);
            Types::Vector zxMultiplyResult = Matrix::multiply(eulerZ, xAxis);
            Types::Vector zyMultiplyResult = Matrix::multiply(eulerZ, yAxis);
            Types::Vector zzMultiplyResult = Matrix::multiply(eulerZ, zAxis);
        }
    };
};

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
    Gek::MathSIMD::test();

    return 0;

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