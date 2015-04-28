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
    CStringW strTileSet;
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
        else if (_wcsicmp(astrArguments[nArgument], L"-tileset") == 0 && ++nArgument < nNumArguments)
        {
            strTileSet = astrArguments[nArgument];
        }
    }

    if (strInput.IsEmpty() || strOutput.IsEmpty())
    {
        CPathW strFile(astrArguments[0]);
        strFile.StripPath();
        printf("%S -input <file> -output <file> -tileset <set, optional>\r\n", strFile.m_strPath.GetString());
        return 0;
    }

    if (!strTileSet.IsEmpty())
    {
        CPathW kTileSet(strTileSet);
        kTileSet.AddBackslash();
        strTileSet = kTileSet.m_strPath;
    }

    CLibXMLDoc kDocument;
    HRESULT hRetVal = kDocument.Load(strInput);
    if (SUCCEEDED(hRetVal))
    {
        CLibXMLNode &kMapNode = kDocument.GetRoot();
        if (kMapNode.GetType().CompareNoCase(L"map") == 0)
        {
            UINT32 nXSize = StrToUINT32(kMapNode.GetAttribute(L"width"));
            UINT32 nYSize = StrToUINT32(kMapNode.GetAttribute(L"height"));
            CLibXMLNode &kLayerNode = kMapNode.FirstChildElement(L"layer");
            while (kLayerNode)
            {
                float nZPosition = StrToFloat(kLayerNode.GetAttribute(L"name"));
                CLibXMLNode &kTileNode = kLayerNode.FirstChildElement(L"data").FirstChildElement(L"tile");
                for (UINT32 nXPosition = 0; nXPosition < nXSize; nXPosition++)
                {
                    for (UINT32 nYPosition = 0; nYPosition < nYSize; nYPosition++)
                    {
                        UINT32 nGID = StrToUINT32(kTileNode.GetAttribute(L"gid"));
                        if (nGID > 0)
                        {
                            printf("Tile %S%d: (%d,%d,%f)\r\n", strTileSet.GetString(), nGID, nXPosition, nYPosition, nZPosition);
                        }

                        kTileNode = kTileNode.NextSiblingElement(L"tile");
                    }
                }

                kLayerNode = kLayerNode.NextSiblingElement(L"layer");
            };

            CLibXMLNode &kObjectGroupNode = kMapNode.FirstChildElement(L"objectgroup");
            while (kObjectGroupNode)
            {
                float nZPosition = StrToFloat(kObjectGroupNode.GetAttribute(L"name"));

                UINT32 nTileIndex = 0;
                CLibXMLNode &kObjectNode = kObjectGroupNode.FirstChildElement(L"object");
                while (kObjectNode)
                {
                    CStringW strName;
                    if (kObjectNode.HasAttribute(L"name"))
                    {
                        strName = kObjectNode.GetAttribute(L"name");
                    }

                    UINT32 nID = StrToUINT32(kObjectNode.GetAttribute(L"id"));
                    CStringW strType(kObjectNode.GetAttribute(L"type"));
                    float nXPosition = StrToFloat(kObjectNode.GetAttribute(L"x"));
                    float nYPosition = StrToFloat(kObjectNode.GetAttribute(L"y"));

                    printf("Entity %d (%S): Type(%S), (%f,%f,%f)\r\n", nID, strName.GetString(), strType.GetString(), nXPosition, nYPosition, nZPosition);

                    kObjectNode = kObjectNode.NextSiblingElement(L"object");
                };

                kObjectGroupNode = kObjectGroupNode.NextSiblingElement(L"objectgroup");
            };
        }
    }

    printf("\r\n");
    return 0;
}