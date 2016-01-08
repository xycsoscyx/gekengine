#include "GEK\Utility\String.h"
#include "GEK\Utility\FileSystem.h"
#include "GEK\Context\Common.h"
#include "GEK\System\VideoSystem.h"
#include "resource.h"

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

LRESULT CALLBACK WindowProc(HWND window, UINT32 message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_CLOSE:
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    };

    return DefWindowProc(window, message, wParam, lParam);
}

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
        CComPtr<Gek::Context> context;
        HRESULT resultValue = Gek::Context::create(&context);
        if (context)
        {
#ifdef _DEBUG
            SetCurrentDirectory(Gek::FileSystem::expandPath(L"%root%\\Debug"));
            context->addSearchPath(L"%root%\\Debug\\Plugins");
#else
            SetCurrentDirectory(Gek::FileSystem::expandPath(L"%root%\\Release"));
            context->addSearchPath(L"%root%\\Release\\Plugins");
#endif

            context->initialize();
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
            kClass.lpszClassName = L"GEKvX_Engine_SHGen";
            if (RegisterClass(&kClass))
            {
                HWND window = CreateWindow(L"GEKvX_Engine_SHGen", L"GEKvX Engine - Spherical Harmonics Generator", WS_POPUP, 0, 0, 1, 1, 0, nullptr, GetModuleHandle(nullptr), 0);
                if (window)
                {
                    CComPtr<Gek::VideoSystem> video;
                    resultValue = context->createInstance(CLSID_IID_PPV_ARGS(Gek::VideoSystemRegistration, &video));
                    if (video)
                    {
                        resultValue = video->initialize(window, false);
                        if(SUCCEEDED(resultValue))
                        {
                            HRSRC computeScriptResource = ::FindResource(nullptr, MAKEINTRESOURCE(IDR_COMPUTE_SCRIPT), RT_RCDATA);
                            if (computeScriptResource)
                            {
                                HGLOBAL computeScriptHandle = ::LoadResource(nullptr, computeScriptResource);
                                if (computeScriptHandle)
                                {
                                    UINT32 computeScriptSize = SizeofResource(nullptr, computeScriptResource);
                                    CStringA computeScript((LPCSTR)::LockResource(computeScriptHandle), computeScriptSize);

                                    CComPtr<IUnknown> computeProgram;
                                    resultValue = video->compileComputeProgram(&computeProgram, computeScript, "mainComputeProgram");
                                    if (computeProgram)
                                    {
                                        CComPtr<Gek::VideoTexture> environmentMap;
                                        resultValue = video->loadTexture(&environmentMap, fileNameInput, 0);
                                        if (!environmentMap)
                                        {
                                            CStringW fileNameStrings[6] = 
                                            {
                                                Gek::String::format(L"%s\\posx.jpg", fileNameInput.GetString()),
                                                Gek::String::format(L"%s\\negx.jpg", fileNameInput.GetString()),
                                                Gek::String::format(L"%s\\posy.jpg", fileNameInput.GetString()),
                                                Gek::String::format(L"%s\\negy.jpg", fileNameInput.GetString()),
                                                Gek::String::format(L"%s\\posz.jpg", fileNameInput.GetString()),
                                                Gek::String::format(L"%s\\negz.jpg", fileNameInput.GetString()),
                                            };

                                            LPCWSTR fileNamePointers[6] = 
                                            {
                                                fileNameStrings[0].GetString(),
                                                fileNameStrings[1].GetString(),
                                                fileNameStrings[2].GetString(),
                                                fileNameStrings[3].GetString(),
                                                fileNameStrings[4].GetString(),
                                                fileNameStrings[5].GetString(),
                                            };

                                            resultValue = video->loadCubeMap(&environmentMap, fileNamePointers, 0);
                                        }

                                        if (environmentMap)
                                        {
                                            CComPtr<Gek::VideoBuffer> coefficientBuffer;
                                            resultValue = video->createBuffer(&coefficientBuffer, Gek::Video::Format::Float4, 9, Gek::Video::BufferType::Raw, Gek::Video::BufferFlags::UnorderedAccess);
                                            if (coefficientBuffer)
                                            {
                                                CComPtr<Gek::VideoBuffer> coefficientCopy;
                                                resultValue = video->createBuffer(&coefficientCopy, Gek::Video::Format::Float4, 9, Gek::Video::BufferType::Raw, Gek::Video::BufferFlags::Readable);
                                                if (coefficientCopy)
                                                {
                                                    video->present(false);
                                                    CComPtr<IUnknown> samplerStates;
                                                    video->createSamplerStates(&samplerStates, Gek::Video::SamplerStates());
                                                    video->getDefaultContext()->computePipeline()->setSamplerStates(samplerStates, 0);
                                                    video->getDefaultContext()->computePipeline()->setResource(environmentMap, 0);
                                                    video->getDefaultContext()->computePipeline()->setUnorderedAccess(coefficientBuffer, 0);
                                                    video->getDefaultContext()->computePipeline()->setProgram(computeProgram);
                                                    video->getDefaultContext()->dispatch(1, 1, 1);
                                                    //video->present(true);

                                                    video->copyBuffer(coefficientCopy, coefficientBuffer);

                                                    Gek::Math::Float4 *coefficientData = nullptr;
                                                    resultValue = video->mapBuffer(coefficientCopy, (LPVOID *)&coefficientData, Gek::Video::Map::Read);
                                                    if (SUCCEEDED(resultValue))
                                                    {
                                                        CStringA output;
                                                        output += "float3 coefficients[9] = \r\n";
                                                        output += "{\r\n";
                                                        for (UINT32 order = 0; order < 9; order++)
                                                        {
                                                            output.AppendFormat("    float3(%f, %f, %f),\r\n", coefficientData[order].x, coefficientData[order].y, coefficientData[order].z);
                                                        }

                                                        output += "};\r\n";
                                                        video->unmapBuffer(coefficientBuffer);
                                                    }
                                                    else
                                                    {
                                                        throw GeneratorException(__LINE__, L"Unable to map coefficient buffer: 0x%08X", resultValue);
                                                    }
                                                }
                                                else
                                                {
                                                    throw GeneratorException(__LINE__, L"Unable to create coefficient copy buffer: 0x%08X", resultValue);
                                                }
                                            }
                                            else
                                            {
                                                throw GeneratorException(__LINE__, L"Unable to create coefficient data buffer: 0x%08X", resultValue);
                                            }
                                        }
                                        else
                                        {
                                            throw GeneratorException(__LINE__, L"Unable to load environment map (%s): 0x%08X", fileNameInput.GetString(), resultValue);
                                        }
                                    }
                                    else
                                    {
                                        throw GeneratorException(__LINE__, L"Unable to load compute program: 0x%08X", resultValue);
                                    }
                                }
                                else
                                {
                                    throw GeneratorException(__LINE__, L"Unable to load generator script from resource: %d", GetLastError());
                                }
                            }
                            else
                            {
                                throw GeneratorException(__LINE__, L"Unable to locate generator script in resources: %d", GetLastError());
                            }
                        }
                        else
                        {
                            throw GeneratorException(__LINE__, L"Unable to initialize video system: 0x%08X", resultValue);
                        }
                    }
                    else
                    {
                        throw GeneratorException(__LINE__, L"Unable to create video system object: 0x%08X", resultValue);
                    }

                    DestroyWindow(window);
                }
                else
                {
                    throw GeneratorException(__LINE__, L"Unable to create window: %d", GetLastError());
                }
            }
            else
            {
                throw GeneratorException(__LINE__, L"Unable to register window class: %d", GetLastError());
            }
        }
        else
        {
            throw GeneratorException(__LINE__, L"Unable to create context object: 0x%08X", resultValue);
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