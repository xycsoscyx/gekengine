#include "GEK\Context\Common.h"
#include "GEK\Utility\String.h"
#include "GEK\Utility\XML.h"
#include <atlpath.h>
#include <algorithm>
#include <vector>
#include <unordered_map>
#include <map>

#include <sqlite3.h>
#pragma comment(lib, "sqlite3.lib")

class OptimizerException
{
public:
    CStringW message;
    int line;

public:
    OptimizerException(int line, LPCWSTR format, ...)
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
}; // namespace Gek

int wmain(int argumentCount, wchar_t *argumentList[], wchar_t *environmentVariableList)
{
    printf("GEK Scene Converter\r\n");

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
        sqlite3 *database = nullptr;
        sqlite3_open(CW2A(fileNameOutput, CP_UTF8), &database);
        if (database)
        {
            Gek::XmlDocument xmlDocument;
            HRESULT resultValue = xmlDocument.load(fileNameInput);
            if (SUCCEEDED(resultValue))
            {
                Gek::XmlNode xmlWorldNode = xmlDocument.getRoot();
                if (xmlWorldNode && xmlWorldNode.getType().CompareNoCase(L"world") == 0)
                {
                    Gek::XmlNode xmlPopulationNode = xmlWorldNode.firstChildElement(L"population");
                    if (xmlPopulationNode)
                    {
                        Gek::XmlNode xmlEntityNode = xmlPopulationNode.firstChildElement(L"entity");
                        while (xmlEntityNode)
                        {
                            std::unordered_map<CStringW, std::unordered_map<CStringW, CStringW>> entityParameterList;
                            Gek::XmlNode xmlComponentNode = xmlEntityNode.firstChildElement();
                            while (xmlComponentNode)
                            {
                                std::unordered_map<CStringW, CStringW> &componentParameterList = entityParameterList[xmlComponentNode.getType()];
                                xmlComponentNode.listAttributes([&componentParameterList](LPCWSTR name, LPCWSTR value) -> void
                                {
                                    componentParameterList.insert(std::make_pair(name, value));
                                });

                                componentParameterList[L""] = xmlComponentNode.getText();
                                xmlComponentNode = xmlComponentNode.nextSiblingElement();
                            };

                            xmlEntityNode = xmlEntityNode.nextSiblingElement(L"entity");
                        };
                    }
                    else
                    {
                        throw OptimizerException(__LINE__, L"[error] Unable to locate \"population\" node");
                        resultValue = E_UNEXPECTED;
                    }
                }
                else
                {
                    throw OptimizerException(__LINE__, L"[error] Unable to locate \"world\" node");
                    resultValue = E_UNEXPECTED;
                }
            }
            else
            {
                throw OptimizerException(__LINE__, L"[error] Unable to load population");
            }
        }
        else
        {
            throw OptimizerException(__LINE__, L"[error] Unable to create database to store population");
        }
    }
    catch (OptimizerException exception)
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