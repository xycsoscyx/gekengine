#include "GEK\Math\Vector3.h"
#include "GEK\Math\Matrix4x4.h"
#include "GEK\Shape\AlignedBox.h"
#include "GEK\Utility\String.h"
#include "GEK\Utility\XML.h"
#include "GEK\Context\Common.h"
#include <atlpath.h>
#include <algorithm>
#include <map>

int wmain(int argumentCount, wchar_t *argumentList[], wchar_t *environmentVariableList)
{
    printf("GEK Tiled Map Converter\r\n");

    CStringW fileNameInput;
    CStringW fileNameOutput;
    CStringW tileSet;
    for (int argumentIndex = 1; argumentIndex < argumentCount; argumentIndex++)
    {
        if (_wcsicmp(argumentList[argumentIndex], L"-input") == 0 && ++argumentIndex < argumentCount)
        {
            fileNameInput = argumentList[argumentIndex];
        }
        else if (_wcsicmp(argumentList[argumentIndex], L"-output") == 0 && ++argumentIndex < argumentCount)
        {
            fileNameOutput = argumentList[argumentIndex];
        }
        else if (_wcsicmp(argumentList[argumentIndex], L"-tileset") == 0 && ++argumentIndex < argumentCount)
        {
            tileSet = argumentList[argumentIndex];
        }
    }

    if (fileNameInput.IsEmpty() || fileNameOutput.IsEmpty())
    {
        CPathW strFile(argumentList[0]);
        strFile.StripPath();
        printf("%S -input <file> -output <file> -tileset <set, optional>\r\n", strFile.m_strPath.GetString());
        return 0;
    }

    if (!tileSet.IsEmpty())
    {
        CPathW kTileSet(tileSet);
        kTileSet.AddBackslash();
        tileSet = kTileSet.m_strPath;
    }

    FILE *file = nullptr;
    _wfopen_s(&file, fileNameOutput, L"w+b");
    if (file)
    {
        fprintf(file, "<?xml version=\"1.0\"?>\r\n");
        fprintf(file, "<world name=\"Demo\">\r\n");
        fprintf(file, "\t<population>\r\n");

        Gek::Xml::Document xmlDocument;
        HRESULT hRetVal = xmlDocument.load(fileNameInput);
        if (SUCCEEDED(hRetVal))
        {
            Gek::Xml::Node xmpMapNode = xmlDocument.getRoot();
            if (xmpMapNode && xmpMapNode.getType().CompareNoCase(L"map") == 0)
            {
                UINT32 mapWidth = Gek::String::getUINT32(xmpMapNode.getAttribute(L"width"));
                UINT32 mapHeight = Gek::String::getUINT32(xmpMapNode.getAttribute(L"height"));
                float tileWidth = Gek::String::getFloat(xmpMapNode.getAttribute(L"tilewidth"));
                float tileHeight = Gek::String::getFloat(xmpMapNode.getAttribute(L"tileheight"));
                Gek::Xml::Node xmlObjectGroupNode = xmpMapNode.firstChildElement(L"objectgroup");
                while (xmlObjectGroupNode)
                {
                    Gek::Math::Float3 tilePosition;
                    tilePosition.z = Gek::String::getFloat(xmlObjectGroupNode.getAttribute(L"name"));

                    Gek::Xml::Node xmlObjectNode = xmlObjectGroupNode.firstChildElement(L"object");
                    while (xmlObjectNode)
                    {
                        CStringW tileType(xmlObjectNode.getAttribute(L"type"));
                        tilePosition.x = (Gek::String::getFloat(xmlObjectNode.getAttribute(L"x")) / tileWidth);
                        tilePosition.y = (Gek::String::getFloat(xmlObjectNode.getAttribute(L"y")) / tileHeight);
                        if (xmlObjectNode.hasAttribute(L"name"))
                        {
                            CStringW tileName = xmlObjectNode.getAttribute(L"name");
                            printf("Entity (%S): Type(%S), (%S)\r\n", tileName.GetString(), tileType.GetString(), Gek::String::setFloat3(tilePosition).GetString());
                            fprintf(file, "\t\t<entity name=\"%S\">\r\n", tileName.GetString());
                        }
                        else
                        {
                            printf("Entity: Type(%S), (%S)\r\n", tileType.GetString(), Gek::String::setFloat3(tilePosition).GetString());
                            fprintf(file, "\t\t<entity>\r\n");
                        }


                        fprintf(file, "\t\t\t<transform position=\"%S\" />\r\n", Gek::String::setFloat3(tilePosition).GetString());
                        if (tileType.CompareNoCase(L"player") == 0)
                        {
                            fprintf(file, "\t\t\t<player outer_radius=\"1\" inner_radius=\".25\" height=\"1.9\" stair_step=\".25\"/>\r\n");
                            fprintf(file, "\t\t\t<mass value=\"250\" />\r\n");
                        }
                        else if (tileType.CompareNoCase(L"light") == 0)
                        {
                            fprintf(file, "\t\t\t<pointlight radius=\"lerp(15, 30, arand(1))\" />\r\n");
                            fprintf(file, "\t\t\t<color value=\"lerp(.5,3,arand(1)),lerp(.5,3,arand(1)),lerp(.5,3,arand(1)),1\" />\r\n");
                        }
                        if (tileType.CompareNoCase(L"boulder") == 0)
                        {
                            fprintf(file, "\t\t\t<dynamicbody shape=\"*sphere\" material=\"rock\" />\r\n");
                            fprintf(file, "\t\t\t<mass value=\"lerp(25,50,arand(1))\" />\r\n");
                            fprintf(file, "\t\t\t<size value=\"lerp(1,3,arand(1))\" />\r\n");
                            fprintf(file, "\t\t\t<size value=\"lerp(1,3,arand(1))\" />\r\n");
                            fprintf(file, "\t\t\t<model source=\"boulder\" />\r\n");
                        }

                        fprintf(file, "\t\t</entity>\r\n");
                        if (tileType.CompareNoCase(L"player") == 0 && xmlObjectNode.hasAttribute(L"name"))
                        {
                            fprintf(file, "\t\t<entity>\r\n");
                            fprintf(file, "\t\t\t<transform />\r\n");
                            fprintf(file, "\t\t\t<viewer type=\"perspective\" field_of_view=\"90\" minimum_distance=\"0.5\" maximum_distance=\"200\" pass=\"toon\"/>\r\n");
                            fprintf(file, "\t\t\t<follow target=\"%S\" offset=\"25,0,0\" />\r\n", xmlObjectNode.getAttribute(L"name").GetString());
                            fprintf(file, "\t\t</entity>\r\n");
                        }

                        xmlObjectNode = xmlObjectNode.nextSiblingElement(L"object");
                    };

                    xmlObjectGroupNode = xmlObjectGroupNode.nextSiblingElement(L"objectgroup");
                };

                Gek::Xml::Node xmlLayerNode = xmpMapNode.firstChildElement(L"layer");
                while (xmlLayerNode)
                {
                    Gek::Math::Float3 tilePosition;
                    tilePosition.z = Gek::String::getFloat(xmlLayerNode.getAttribute(L"name"));
                    Gek::Xml::Node xmlTileNode = xmlLayerNode.firstChildElement(L"data").firstChildElement(L"tile");
                    for (UINT32 tileY = 0; tileY < mapHeight; tileY++)
                    {
                        for (UINT32 tileX = 0; tileX < mapWidth; tileX++)
                        {
                            UINT32 tileID = Gek::String::getUINT32(xmlTileNode.getAttribute(L"gid"));
                            if (tileID > 0)
                            {
                                tilePosition.x = float(tileX);
                                tilePosition.y = float(mapHeight - 1 - tileY);
                                printf("Tile %S%d: (%S)\r\n", tileSet.GetString(), tileID, Gek::String::setFloat3(tilePosition).GetString());

                                fprintf(file, "\t\t<entity>\r\n");
                                fprintf(file, "\t\t\t<transform position=\"%S\" />\r\n", Gek::String::setFloat3(tilePosition).GetString());
                                fprintf(file, "\t\t\t<model source=\"%S%d\" />\r\n", tileSet.GetString(), tileID);
                                fprintf(file, "\t\t\t<dynamicbody shape=\"%S%d\" />\r\n", tileSet.GetString(), tileID);
                                fprintf(file, "\t\t\t<mass value=\"0\" />\r\n");
                                fprintf(file, "\t\t</entity>\r\n");
                            }

                            xmlTileNode = xmlTileNode.nextSiblingElement(L"tile");
                        }
                    }

                    xmlLayerNode = xmlLayerNode.nextSiblingElement(L"layer");
                };
            }
        }
        
        fprintf(file, "\t</population>\r\n");
        fprintf(file, "</world>\r\n");
        fclose(file);
    }

    printf("\r\n");
    return 0;
}