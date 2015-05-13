#include "GEKUtility.h"
#include <atlpath.h>
#include <stdio.h>
#include <time.h>
#include <list>
#include <algorithm>
#include <random>

#ifdef min
#undef min
#undef max
#endif

#include "exprtk.hpp"

namespace Gek
{
    namespace Display
    {
        UINT8 getAspectRatio(UINT32 width, UINT32 height)
        {
            const float AspectRatio4x3 = (float(INT32((4.0f / 3.0f) * 100.0f)) / 100.0f);
            const float AspectRatio16x9 = (float(INT32((16.0f / 9.0f) * 100.0f)) / 100.0f);
            const float AspectRatio16x10 = (float(INT32((16.0f / 10.0f) * 100.0f)) / 100.0f);
            float aspectRatio = (float(INT32((float(width) / float(height)) * 100.0f)) / 100.0f);
            if (aspectRatio == AspectRatio4x3)
            {
                return AspectRatio::_4x3;
            }
            else if (aspectRatio == AspectRatio16x9)
            {
                return AspectRatio::_16x9;
            }
            else if (aspectRatio == AspectRatio16x10)
            {
                return AspectRatio::_16x10;
            }
            else
            {
                return AspectRatio::Unknown;
            }
        }

        std::map<UINT32, std::vector<Mode>> getModes(void)
        {
            UINT32 displayMode = 0;
            DEVMODE displayModeData = { 0 };
            std::map<UINT32, std::vector<Mode>> availableModes;
            while (EnumDisplaySettings(0, displayMode++, &displayModeData))
            {
                std::vector<Mode> &currentModes = availableModes[displayModeData.dmBitsPerPel];
                auto findIterator = std::find_if(currentModes.begin(), currentModes.end(), [&](const Mode &mode) -> bool
                {
                    if (mode.width != displayModeData.dmPelsWidth) return false;
                    if (mode.height != displayModeData.dmPelsHeight) return false;
                    return true;
                });

                if (findIterator == currentModes.end())
                {
                    currentModes.emplace_back(displayModeData.dmPanningWidth, displayModeData.dmPanningHeight,
                        getAspectRatio(displayModeData.dmPanningWidth, displayModeData.dmPanningHeight));
                }
            };

            return availableModes;
        }
    }; // namespace Display

    namespace Evaluator
    {
        static std::random_device randomDevice;
        static std::mt19937 mersineTwister(randomDevice());

        template <typename TYPE>
        struct RandomFunction : public exprtk::ifunction<TYPE>
        {
            std::uniform_real_distribution<TYPE> uniformRealDistribution;
            RandomFunction(TYPE minimum, TYPE maximum)
                : exprtk::ifunction<TYPE>(1)
                , uniformRealDistribution(minimum, maximum)
            {
            }

            inline TYPE operator()(const TYPE& range)
            {
                return (range * uniformRealDistribution(mersineTwister));
            }
        };

        template <typename TYPE>
        struct LerpFunction : public exprtk::ifunction<TYPE>
        {
            LerpFunction(void) : exprtk::ifunction<TYPE>(3)
            {
            }

            inline TYPE operator()(const TYPE& x, const TYPE& y, const TYPE& step)
            {
                return Math::lerp(x, y, step);
            }
        };

        template <typename TYPE>
        class EquationEvaluator
        {
        private:
            RandomFunction<TYPE> signedRandomFunction;
            RandomFunction<TYPE> unsignedRandomFunction;
            LerpFunction<TYPE> lerpFunction;
            exprtk::symbol_table<TYPE> symbolTable;
            exprtk::expression<TYPE> expression;
            exprtk::parser<TYPE> parser;

        public:
            EquationEvaluator(void)
                : signedRandomFunction(TYPE(-1), TYPE(1))
                , unsignedRandomFunction(TYPE(0), TYPE(1))
            {
                symbolTable.add_function("rand", signedRandomFunction);
                symbolTable.add_function("arand", unsignedRandomFunction);
                symbolTable.add_function("lerp", lerpFunction);

                symbolTable.add_constants();
                expression.register_symbol_table(symbolTable);
            }

            bool getValue(LPCWSTR equation, TYPE &value)
            {
                symbolTable.remove_vector("value");
                if (parser.compile(CW2A(equation).m_psz, expression))
                {
                    value = expression.value();
                    return true;
                }
                else
                {
                    return false;
                }
            }

            template <std::size_t SIZE>
            bool getVector(LPCWSTR equation, TYPE(&vector)[SIZE])
            {
                symbolTable.remove_vector("value");
                symbolTable.add_vector("value", vector);
                if (parser.compile(String::format("var vector[%d] := {%S}; value := vector;", SIZE, equation).GetString(), expression))
                {
                    expression.value();
                    return true;
                }
                else
                {
                    return false;
                }
            }
        };

        static EquationEvaluator<float> evaluateFloat;
        static EquationEvaluator<double> evaluateDouble;

        bool getDouble(LPCWSTR expression, double &result)
        {
            return evaluateDouble.getValue(expression, result);
        }

        bool getFloat(LPCWSTR expression, float &result)
        {
            return evaluateFloat.getValue(expression, result);
        }

        bool getFloat2(LPCWSTR expression, Gek::Math::Float2 &result)
        {
            return evaluateFloat.getVector(expression, result.xy);
        }

        bool getFloat3(LPCWSTR expression, Gek::Math::Float3 &result)
        {
            return evaluateFloat.getVector(expression, result.xyz);
        }

        bool getFloat4(LPCWSTR expression, Gek::Math::Float4 &result)
        {
            return evaluateFloat.getVector(expression, result.xyzw);
        }

        bool getQuaternion(LPCWSTR expression, Gek::Math::Quaternion &result)
        {
            return evaluateFloat.getVector(expression, result.xyzw);
        }

        bool getINT32(LPCWSTR expression, INT32 &result)
        {
            float value = 0.0f;
            if (getFloat(expression, value))
            {
                result = INT32(value);
                return true;
            }
            else
            {
                return false;
            }
        }

        bool getUINT32(LPCWSTR expression, UINT32 &result)
        {
            float value = 0.0f;
            if (getFloat(expression, value))
            {
                result = UINT32(value);
                return true;
            }
            else
            {
                return false;
            }
        }

        bool getINT64(LPCWSTR expression, INT64 &result)
        {
            double value = 0.0;
            if (getDouble(expression, value))
            {
                result = INT64(value);
                return true;
            }
            else
            {
                return false;
            }
        }

        bool getUINT64(LPCWSTR expression, UINT64 &result)
        {
            double value = 0.0;
            if (getDouble(expression, value))
            {
                result = UINT64(value);
                return true;
            }
            else
            {
                return false;
            }
        }

        bool getBoolean(LPCWSTR expression, bool &result)
        {
            if (_wcsicmp(expression, L"true") == 0 ||
                _wcsicmp(expression, L"yes") == 0)
            {
                result = true;
                return true;
            }

            INT32 value = 0;
            if (getINT32(expression, value))
            {
                result = (value == 0 ? false : true);
                return true;
            }
            else
            {
                return false;
            }
        }
    }; // namespace Evaluator

    namespace String
    {
        double getDouble(LPCWSTR expression)
        {
            double value = 0.0;
            Evaluator::getDouble(expression, value);
            return value;
        }

        float getFloat(LPCWSTR expression)
        {
            float value = 0.0f;
            Evaluator::getFloat(expression, value);
            return value;
        }

        Gek::Math::Float2 getFloat2(LPCWSTR expression)
        {
            Gek::Math::Float2 vector;
            if (!Evaluator::getFloat2(expression, vector))
            {
                if (Evaluator::getFloat(expression, vector.x))
                {
                    vector.y = vector.x;
                }
            }

            return vector;
        }

        Gek::Math::Float3 getFloat3(LPCWSTR expression)
        {
            Gek::Math::Float3 vector;
            if (!Evaluator::getFloat3(expression, vector))
            {
                if (Evaluator::getFloat(expression, vector.x))
                {
                    vector.y = vector.z = vector.x;
                }
            }

            return vector;
        }

        Gek::Math::Float4 getFloat4(LPCWSTR expression)
        {
            Gek::Math::Float4 vector;
            if (!Evaluator::getFloat4(expression, vector))
            {
                if (Evaluator::getFloat3(expression, *(Gek::Math::Float3 *)&vector))
                {
                    vector.w = 1.0f;
                }
                else
                {
                    if (Evaluator::getFloat(expression, vector.x))
                    {
                        vector.y = vector.z = vector.w = vector.x;
                    }
                }
            }

            return vector;
        }

        Gek::Math::Quaternion getQuaternion(LPCWSTR expression)
        {
            Gek::Math::Quaternion rotation;
            if (!Evaluator::getQuaternion(expression, rotation))
            {
                Gek::Math::Float3 euler;
                if (Evaluator::getFloat3(expression, euler))
                {
                    rotation.setEuler(euler);
                }
            }

            return rotation;
        }

        INT32 getINT32(LPCWSTR expression)
        {
            INT32 value = 0;
            Evaluator::getINT32(expression, value);
            return value;
        }

        UINT32 getUINT32(LPCWSTR expression)
        {
            UINT32 value = 0;
            Evaluator::getUINT32(expression, value);
            return value;
        }

        INT64 getINT64(LPCWSTR expression)
        {
            INT64 value = 0;
            Evaluator::getINT64(expression, value);
            return value;
        }

        UINT64 getUINT64(LPCWSTR expression)
        {
            UINT64 value = 0;
            Evaluator::getUINT64(expression, value);
            return value;
        }

        bool getBoolean(LPCWSTR expression)
        {
            bool value = false;
            Evaluator::getBoolean(expression, value);
            return value;
        }

        CStringW setDouble(double value)
        {
            CStringW strValue;
            strValue.Format(L"%f", value);
            return strValue;
        }

        CStringW setFloat(float value)
        {
            CStringW strValue;
            strValue.Format(L"%f", value);
            return strValue;
        }

        CStringW setFloat2(Gek::Math::Float2 value)
        {
            CStringW strValue;
            strValue.Format(L"%f,%f", value.x, value.y);
            return strValue;
        }

        CStringW setFloat3(Gek::Math::Float3 value)
        {
            CStringW strValue;
            strValue.Format(L"%f,%f,%f", value.x, value.y, value.z);
            return strValue;
        }

        CStringW setFloat4(Gek::Math::Float4 value)
        {
            CStringW strValue;
            strValue.Format(L"%f,%f,%f,%f", value.x, value.y, value.z, value.w);
            return strValue;
        }

        CStringW setQuaternion(Gek::Math::Quaternion value)
        {
            CStringW strValue;
            strValue.Format(L"%f,%f,%f,%f", value.x, value.y, value.z, value.w);
            return strValue;
        }

        CStringW setINT32(INT32 value)
        {
            CStringW strValue;
            strValue.Format(L"%d", value);
            return strValue;
        }

        CStringW setUINT32(UINT32 value)
        {
            CStringW strValue;
            strValue.Format(L"%u", value);
            return strValue;
        }

        CStringW setINT64(INT64 value)
        {
            CStringW strValue;
            strValue.Format(L"%lld", value);
            return strValue;
        }

        CStringW setUINT64(UINT64 value)
        {
            CStringW strValue;
            strValue.Format(L"%llu", value);
            return strValue;
        }

        CStringW setBoolean(bool value)
        {
            return (value ? L"true" : L"false");
        }

        CStringA format(LPCSTR format, ...)
        {
            CStringA result;
            if (format != nullptr)
            {
                va_list variableList;
                va_start(variableList, format);
                result.FormatV(format, variableList);
                va_end(variableList);
            }

            return result;
        }

        CStringW format(LPCWSTR format, ...)
        {
            CStringW result;
            if (format != nullptr)
            {
                va_list variableList;
                va_start(variableList, format);
                result.FormatV(format, variableList);
                va_end(variableList);
            }

            return result;
        }
    }; // namespace String

    namespace FileSystem
    {
        CStringW expandPath(LPCWSTR basePath)
        {
            CStringW fullPath(basePath);
            if (fullPath.Find(L"%root%") >= 0)
            {
                CStringW currentModuleName;
                GetModuleFileName(nullptr, currentModuleName.GetBuffer(MAX_PATH + 1), MAX_PATH);
                currentModuleName.ReleaseBuffer();

                CPathW currentModulePath;
                CStringW &currentModulePathString = currentModulePath;
                GetFullPathName(currentModuleName, MAX_PATH, currentModulePathString.GetBuffer(MAX_PATH + 1), nullptr);
                currentModulePathString.ReleaseBuffer();

                // Remove filename from path
                currentModulePath.RemoveFileSpec();

                // Remove debug/release form path
                currentModulePath.RemoveFileSpec();

                fullPath.Replace(L"%root%", currentModulePath);
            }

            fullPath.Replace(L"/", L"\\");
            return fullPath;
        }

        HRESULT find(LPCWSTR basePath, LPCWSTR filterTypes, bool searchRecursively, std::function<HRESULT(LPCWSTR)> onFileFound)
        {
            HRESULT returnValue = S_OK;

            CStringW fullPath(expandPath(basePath));
            PathAddBackslashW(fullPath.GetBuffer(MAX_PATH + 1));
            fullPath.ReleaseBuffer();

            WIN32_FIND_DATA findData;
            CStringW fullPathFilter(fullPath + filterTypes);
            HANDLE findHandle = FindFirstFile(fullPathFilter, &findData);
            if (findHandle != INVALID_HANDLE_VALUE)
            {
                do
                {
                    if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                    {
                        if (searchRecursively && findData.cFileName[0] != L'.')
                        {
                            returnValue = find((fullPath + findData.cFileName), filterTypes, searchRecursively, onFileFound);
                        }
                    }
                    else
                    {
                        returnValue = onFileFound(fullPath + findData.cFileName);
                    }

                    if (FAILED(returnValue))
                    {
                        break;
                    }
                } while (FindNextFile(findHandle, &findData));

                FindClose(findHandle);
            }

            return returnValue;
        }

        HMODULE loadLibrary(LPCWSTR basePath)
        {
            return LoadLibraryW(expandPath(basePath));
        }

        HRESULT load(LPCWSTR basePath, std::vector<UINT8> &buffer, size_t limitReadSize)
        {
            HRESULT returnValue = E_FAIL;
            CStringW fullPath(expandPath(basePath));
            HANDLE fileHandle = CreateFile(fullPath, GENERIC_READ, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
            if (fileHandle == INVALID_HANDLE_VALUE)
            {
                returnValue = E_FAIL;
            }
            else
            {
                DWORD fileSize = GetFileSize(fileHandle, nullptr);
                if (fileSize == 0)
                {
                    returnValue = S_OK;
                }
                else
                {
                    if (limitReadSize > 0)
                    {
                        buffer.resize(limitReadSize);
                    }
                    else
                    {
                        buffer.resize(fileSize);
                    }

                    if (!buffer.empty())
                    {
                        DWORD bytesRead = 0;
                        if (ReadFile(fileHandle, buffer.data(), buffer.size(), &bytesRead, nullptr))
                        {
                            returnValue = (bytesRead == buffer.size() ? S_OK : E_FAIL);
                        }
                        else
                        {
                            returnValue = E_FAIL;
                        }
                    }
                    else
                    {
                        returnValue = E_OUTOFMEMORY;
                    }
                }

                CloseHandle(fileHandle);
            }

            return returnValue;
        }

        HRESULT load(LPCWSTR basePath, CStringA &string)
        {
            std::vector<UINT8> buffer;
            HRESULT returnValue = load(basePath, buffer);
            if (SUCCEEDED(returnValue))
            {
                buffer.push_back('\0');
                string = LPCSTR(buffer.data());
            }

            return returnValue;
        }

        HRESULT load(LPCWSTR basePath, CStringW &string, bool convertUTF8)
        {
            CStringA readString;
            HRESULT returnValue = load(basePath, readString);
            if (SUCCEEDED(returnValue))
            {
                string = CA2W(readString, (convertUTF8 ? CP_UTF8 : CP_ACP));
            }

            return returnValue;
        }

        HRESULT GEKSaveToFile(LPCWSTR basePath, const std::vector<UINT8> &buffer)
        {
            HRESULT returnValue = E_FAIL;
            CStringW fullPath(expandPath(basePath));
            HANDLE fileHandle = CreateFile(fullPath, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
            if (fileHandle != INVALID_HANDLE_VALUE)
            {
                DWORD bytesWritten = 0;
                WriteFile(fileHandle, buffer.data(), buffer.size(), &bytesWritten, nullptr);
                returnValue = (bytesWritten == buffer.size() ? S_OK : E_FAIL);
                CloseHandle(fileHandle);
            }

            return returnValue;
        }

        HRESULT GEKSaveToFile(LPCWSTR basePath, LPCSTR string)
        {
            HRESULT returnValue = E_FAIL;
            UINT32 stringLength = strlen(string);
            std::vector<UINT8> buffer(stringLength);
            if (buffer.size() == stringLength)
            {
                memcpy(buffer.data(), string, stringLength);
                returnValue = save(basePath, buffer);
            }

            return returnValue;
        }

        HRESULT GEKSaveToFile(LPCWSTR basePath, LPCWSTR string, bool convertUTF8)
        {
            CStringA writeString = CW2A(string, (convertUTF8 ? CP_UTF8 : CP_ACP));
            return save(basePath, writeString);
        }
    } // namespace FileSystem
}; // namespace Gek
