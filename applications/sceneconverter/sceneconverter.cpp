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
            if (argument.compare(L"-input") == 0 && ++argumentIndex < argumentCount)
            {
                fileNameInput = argumentList[argumentIndex];
            }
            else if (argument.compare(L"-output") == 0 && ++argumentIndex < argumentCount)
            {
                fileNameOutput = argumentList[argumentIndex];
            }
        }

        XmlDocumentPtr document(XmlDocument::load(fileNameInput));
        XmlNodePtr worldNode = document->getRoot(L"world");
        XmlNodePtr populationNode = worldNode->firstChildElement(L"population");
        XmlNodePtr entityNode = populationNode->firstChildElement(L"entity");
        while (entityNode->isValid())
        {
            Population::EntityDefinition entityData;
            XmlNodePtr componentNode = entityNode->firstChildElement();
            while (componentNode->isValid())
            {
                Population::ComponentDefinition &componentData = entityData[componentNode->getType()];
                componentNode->listAttributes([&componentData](const wchar_t *name, const wchar_t *value) -> void
                {
                    componentData.insert(std::make_pair(name, value));
                });

                if (!componentNode->getText().empty())
                {
                    componentData.value = componentNode->getText();
                }

                componentNode = componentNode->nextSiblingElement();
            };

            entityNode = entityNode->nextSiblingElement(L"entity");
        };
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