#include "GEKMath.h"
#include "GEKShape.h"
#include "GEKUtility.h"
#include <atlpath.h>
#include <algorithm>
#include <map>

int wmain(int nNumArguments, wchar_t *astrArguments[], wchar_t *astrEnvironmentVariables)
{
    printf("GEK Tiled Map Converter\r\n");

    CStringW strInput;
    CStringW strOutput;
    for (int nArgument = 1; nArgument < nNumArguments; nArgument++)
    {
        if (_wcsicmp(astrArguments[nArgument], L"-input") == 0 && ++nArgument < nNumArguments)
        {
            strInput = astrArguments[nArgument];
        }
        else if (_wcsicmp(astrArguments[nArgument], L"-output") == 0 && ++nArgument < nNumArguments)
        {
            strOutput = astrArguments[nArgument];
        }
    }

    printf("\r\n");
    return 0;
}