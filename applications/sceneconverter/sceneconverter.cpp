#include "GEK\Utility\Exceptions.h"
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
    }
    catch (const std::exception &exception)
    {
        printf("[error] Exception occurred: %s", exception.what());
    }
    catch (...)
    {
        printf("[error] Unhandled Exception Occurred!");
    }

    printf("\r\n");
    CoUninitialize();
    return 0;
}