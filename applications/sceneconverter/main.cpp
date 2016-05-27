#include "GEK\Utility\Trace.h"
#include "GEK\Utility\String.h"
#include "GEK\Utility\XML.h"
#include "GEK\Engine\Population.h"

using namespace Gek;

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
        XmlDocumentPtr xmlDocument(XmlDocument::load(fileNameInput));

        XmlNodePtr xmlWorldNode = xmlDocument.getRoot();
        GEK_THROW_ERROR(!xmlWorldNode, BaseException, "XML document missing root node");
        GEK_THROW_ERROR(xmlWorldNode.getText().CompareNoCase(L"world") == 0, BaseException, "XML document root node not 'world'");

        XmlNodePtr xmlPopulationNode = xmlWorldNode.firstChildElement(L"population");
        GEK_THROW_ERROR(!xmlPopulationNode, BaseException, "XML document missing population node");

        XmlNodePtr xmlEntityNode = xmlPopulationNode.firstChildElement(L"entity");
        while (xmlEntityNode)
        {
            Population::EntityDefinition entityData;
            XmlNodePtr xmlComponentNode = xmlEntityNode.firstChildElement();
            while (xmlComponentNode)
            {
                Population::ComponentDefinition &componentData = entityData[xmlComponentNode.getType()];
                xmlComponentNode.listAttributes([&componentData](const wchar_t *name, const wchar_t *value) -> void
                {
                    componentData.insert(std::make_pair(name, value));
                });

                if (!xmlComponentNode.getText().IsEmpty())
                {
                    componentData.SetString(xmlComponentNode.getText());
                }
                xmlComponentNode = xmlComponentNode.nextSiblingElement();
            };

            xmlEntityNode = xmlEntityNode.nextSiblingElement(L"entity");
        };
    }
    catch (BaseException exception)
    {
        printf("[error] Error (%): %", exception.when(), exception.what());
    }
    catch (...)
    {
        printf("[error] Unhandled Exception Occurred!");
    }

    printf("\r\n");
    CoUninitialize();
    return 0;
}