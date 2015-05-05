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

    FILE *pFile = nullptr;
    _wfopen_s(&pFile, strOutput, L"w+b");
    if (pFile)
    {
        fprintf(pFile, "<?xml version=\"1.0\"?>\r\n");
        fprintf(pFile, "<world name=\"Demo\">\r\n");
        fprintf(pFile, "\t<population>\r\n");

        CLibXMLDoc kDocument;
        HRESULT hRetVal = kDocument.Load(strInput);
        if (SUCCEEDED(hRetVal))
        {
            CLibXMLNode &kMapNode = kDocument.GetRoot();
            if (kMapNode.GetType().CompareNoCase(L"map") == 0)
            {
                UINT32 nXSize = StrToUINT32(kMapNode.GetAttribute(L"width"));
                UINT32 nYSize = StrToUINT32(kMapNode.GetAttribute(L"height"));
                float nTileXSize = StrToFloat(kMapNode.GetAttribute(L"tilewidth"));
                float nTileYSize = StrToFloat(kMapNode.GetAttribute(L"tileheight"));
                CLibXMLNode &kObjectGroupNode = kMapNode.FirstChildElement(L"objectgroup");
                while (kObjectGroupNode)
                {
                    float nZPosition = StrToFloat(kObjectGroupNode.GetAttribute(L"name"));

                    UINT32 nTileIndex = 0;
                    CLibXMLNode &kObjectNode = kObjectGroupNode.FirstChildElement(L"object");
                    while (kObjectNode)
                    {
                        UINT32 nID = StrToUINT32(kObjectNode.GetAttribute(L"id"));
                        CStringW strType(kObjectNode.GetAttribute(L"type"));
                        float nXPosition = (StrToFloat(kObjectNode.GetAttribute(L"x")) / nTileXSize);
                        float nYPosition = (StrToFloat(kObjectNode.GetAttribute(L"y")) / nTileYSize);
                        if (kObjectNode.HasAttribute(L"name"))
                        {
                            CStringW strName = kObjectNode.GetAttribute(L"name");
                            printf("Entity %d (%S): Type(%S), (%f,%f,%f)\r\n", nID, strName.GetString(), strType.GetString(), nXPosition, nYPosition, nZPosition);
                            fprintf(pFile, "\t\t<entity name=\"%S\">\r\n", strName.GetString());
                        }
                        else
                        {
                            printf("Entity %d: Type(%S), (%f,%f,%f)\r\n", nID, strType.GetString(), nXPosition, nYPosition, nZPosition);
                            fprintf(pFile, "\t\t<entity>\r\n");
                        }


                        fprintf(pFile, "\t\t\t<transform position=\"%f,%f,%f\" />\r\n", nXPosition, nYPosition, nZPosition);
                        if (strType.CompareNoCase(L"player") == 0)
                        {
                            fprintf(pFile, "\t\t\t<player outer_radius=\"1\" inner_radius=\".25\" height=\"1.9\" stair_step=\".25\"/>\r\n");
                            fprintf(pFile, "\t\t\t<mass value=\"250\" />\r\n");
                        }
                        else if (strType.CompareNoCase(L"light") == 0)
                        {
                            fprintf(pFile, "\t\t\t<pointlight radius=\"lerp(15, 30, arand(1))\" />\r\n");
                            fprintf(pFile, "\t\t\t<color value=\"lerp(.5,3,arand(1)),lerp(.5,3,arand(1)),lerp(.5,3,arand(1)),1\" />\r\n");
                        }
                        if (strType.CompareNoCase(L"boulder") == 0)
                        {
                            fprintf(pFile, "\t\t\t<dynamicbody shape=\"*sphere\" material=\"rock\" />\r\n");
                            fprintf(pFile, "\t\t\t<mass value=\"lerp(25,50,arand(1))\" />\r\n");
                            fprintf(pFile, "\t\t\t<size value=\"lerp(1,3,arand(1))\" />\r\n");
                            fprintf(pFile, "\t\t\t<size value=\"lerp(1,3,arand(1))\" />\r\n");
                            fprintf(pFile, "\t\t\t<model source=\"boulder\" />\r\n");
                        }

                        fprintf(pFile, "\t\t</entity>\r\n");
                        if (strType.CompareNoCase(L"player") == 0 && kObjectNode.HasAttribute(L"name"))
                        {
                            fprintf(pFile, "\t\t<entity>\r\n");
                            fprintf(pFile, "\t\t\t<transform />\r\n");
                            fprintf(pFile, "\t\t\t<viewer type=\"perspective\" field_of_view=\"90\" minimum_distance=\"0.5\" maximum_distance=\"200\" pass=\"toon\"/>\r\n");
                            fprintf(pFile, "\t\t\t<follow target=\"%S\" offset=\"25,0,0\" />\r\n", kObjectNode.GetAttribute(L"name").GetString());
                            fprintf(pFile, "\t\t</entity>\r\n");
                        }

                        kObjectNode = kObjectNode.NextSiblingElement(L"object");
                    };

                    kObjectGroupNode = kObjectGroupNode.NextSiblingElement(L"objectgroup");
                };

                CLibXMLNode &kLayerNode = kMapNode.FirstChildElement(L"layer");
                while (kLayerNode)
                {
                    float nZPosition = StrToFloat(kLayerNode.GetAttribute(L"name"));
                    CLibXMLNode &kTileNode = kLayerNode.FirstChildElement(L"data").FirstChildElement(L"tile");
                    for (UINT32 nYIndex = 0; nYIndex < nYSize; nYIndex++)
                    {
                        for (UINT32 nXIndex = 0; nXIndex < nXSize; nXIndex++)
                        {
                            UINT32 nGID = StrToUINT32(kTileNode.GetAttribute(L"gid"));
                            if (nGID > 0)
                            {
                                float nXPosition = float(nXIndex);
                                float nYPosition = float(nYSize - 1 - nYIndex);
                                printf("Tile %S%d: (%f,%f,%f)\r\n", strTileSet.GetString(), nGID, nXPosition, nYPosition, nZPosition);

                                fprintf(pFile, "\t\t<entity>\r\n");
                                fprintf(pFile, "\t\t\t<transform position=\"%f,%f,%f\" />\r\n", nXPosition, nYPosition, nZPosition);
                                fprintf(pFile, "\t\t\t<model source=\"%S%d\" />\r\n", strTileSet.GetString(), nGID);
                                fprintf(pFile, "\t\t\t<dynamicbody shape=\"%S%d\" />\r\n", strTileSet.GetString(), nGID);
                                fprintf(pFile, "\t\t\t<mass value=\"0\" />\r\n");
                                fprintf(pFile, "\t\t</entity>\r\n");
                            }

                            kTileNode = kTileNode.NextSiblingElement(L"tile");
                        }
                    }

                    kLayerNode = kLayerNode.NextSiblingElement(L"layer");
                };
            }
        }
        
        fprintf(pFile, "\t</population>\r\n");
        fprintf(pFile, "</world>\r\n");
        fclose(pFile);
    }

    printf("\r\n");
    return 0;
}