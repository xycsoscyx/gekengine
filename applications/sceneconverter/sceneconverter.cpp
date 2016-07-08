#include "GEK\Utility\Trace.h"
#include "GEK\Utility\String.h"
#include "GEK\Utility\XML.h"
#include "GEK\Engine\Population.h"

using namespace Gek;

int wmain(int argumentCount, const wchar_t *argumentList[], const wchar_t *environmentVariableList)
{
    CoInitialize(nullptr);
    try
    {
        printf("GEK Scene Converter\r\n");

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

        XmlDocumentPtr document(XmlDocument::load(fileNameInput));
        XmlNodePtr worldNode(document->getRoot(L"world"));
        XmlNodePtr populationNode(worldNode->firstChildElement(L"population"));
        for (XmlNodePtr entityNode(populationNode->firstChildElement(L"entity")); entityNode->isValid(); entityNode = entityNode->nextSiblingElement(L"entity"))
        {
            Plugin::Population::EntityDefinition data;
            for (XmlNodePtr componentNode(entityNode->firstChildElement()); componentNode->isValid(); entityNode = entityNode->nextSiblingElement(L"entity"))
            {
                Plugin::Population::ComponentDefinition &componentData = data[componentNode->getType()];
                componentNode->listAttributes([&componentData](const wchar_t *name, const wchar_t *value) -> void
                {
                    componentData.insert(std::make_pair(name, value));
                });

                if (!componentNode->getText().empty())
                {
                    componentData.value = componentNode->getText();
                }
            }
        }
    }
    catch (const Exception &exception)
    {
        printf("[error] Error (%d): %s", exception.at(), exception.what());
    }
    catch (...)
    {
        printf("[error] Unhandled Exception Occurred!");
    }

    printf("\r\n");
    CoUninitialize();
    return 0;
}