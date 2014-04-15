#include "GEKMath.h"
#include "GEKUtility.h"
#include "GEKMesh.h"
#include <atlbase.h>
#include <atlpath.h>
#include <algorithm>
#include <map>

struct MATERIAL
{
    UINT32 m_nFirstVertex;
    UINT32 m_nFirstIndex;
    UINT32 m_nNumIndices;
};

struct MODEL : public aabb
{
    std::map<CStringA, MATERIAL> m_aMaterials;
    std::vector<VERTEX> m_aVertices;
    std::vector<UINT16> m_aIndices;
};

struct NODE : public plane
{
    INT32 m_nPositiveChild;
    INT32 m_nNegativeChild;
};

struct PORTAL : public plane
{
    UINT32 m_nFirstEdge;
    UINT32 m_nNumEdges;
	INT32 m_nPositiveArea;
	INT32 m_nNegativeArea;
};

class CMyException
{
public:
    CStringW m_strMessage;
    int m_nLine;

public:
    CMyException(int nLine, LPCWSTR pFormat, ...)
        : m_nLine(nLine)
    {
        va_list pArgs;
        va_start(pArgs, pFormat);
        m_strMessage.FormatV(pFormat, pArgs);
        va_end(pArgs);
    }
};

int wmain(int nNumArguments, wchar_t *astrArguments[], wchar_t *astrEnvironmentVariables)
{
    printf("Doom3 Map Converter\r\n");

    CStringW strInput;
    CStringW strOutput;
    float nScale = 1.0f;
    CStringA strGame;
    for(int nArgument = 1; nArgument < nNumArguments; nArgument++)
    {
        CStringW strArgument = astrArguments[nArgument];
        if (strArgument.CompareNoCase(L"-input") == 0 && nArgument < (nNumArguments - 1))
        {
            strInput = astrArguments[++nArgument];
        }
        else if (strArgument.CompareNoCase(L"-output") == 0 && nArgument < (nNumArguments - 1))
        {
            strOutput = astrArguments[++nArgument];
        }
        else if (strArgument.CompareNoCase(L"-scale") == 0 && nArgument < (nNumArguments - 1))
        {
            nScale = (1.0f / StrToFloat(astrArguments[++nArgument]));
        }
        else if (strArgument.CompareNoCase(L"-game") == 0 && nArgument < (nNumArguments - 1))
        {
            strGame = astrArguments[++nArgument];
            strGame += L"/";
        }
    }

    if (strInput.IsEmpty() || strOutput.IsEmpty())
    {
        printf("Arguments:\r\n");
        printf("-input <input filename>\r\n");
        printf("-output <output filename>\r\n");
        printf("-game <game>\r\n");
        printf("-scale <optional scale>\r\n");
        return -1;
    }
    else
    {
        printf("Input: %S\r\n", strInput.GetString());
        printf("Output: %S\r\n", strOutput.GetString());
        printf("Game: %s\r\n", strGame.GetString());
        printf("Scale: %f\r\n", nScale);
    }

    try
    {
        if (true)
        {
            CPathW kOutput = strOutput;
            kOutput.StripPath();

            FILE *pFile = nullptr;
            _wfopen_s(&pFile, strOutput + L".xml", L"w+b");
            if (pFile)
            {
                fprintf(pFile, "<?xml version=\"1.0\"?>\r\n");
                fprintf(pFile, "<world name=\"Demo\" source=\"%S\">\r\n", kOutput.m_strPath.GetString());
                fprintf(pFile, "	<population>\r\n");

                CGEKParser kParser;
                if (SUCCEEDED(kParser.LoadFromFile(strInput + L".map")))
                {
                    kParser.NextToken();
                    if (kParser.GetToken().CompareNoCase(L"Version") != 0)
                    {
                        throw CMyException(__LINE__, L"Invalid map File");
                    }

                    kParser.NextToken();
                    if (StrToUINT32(kParser.GetToken()) == 2)
                    {
                        printf("> Doom 3 map format detected\r\n");
                    }
                    else if (StrToUINT32(kParser.GetToken()) == 3)
                    {
                        printf("> Quake 4 map format detected\r\n");
                    }
                    else
                    {
                        throw CMyException(__LINE__, L"Invalid map File Version");
                    }

                    while (kParser.NextToken())
                    {
                        if (kParser.GetToken()[0] == '{')
                        {
                            printf("> Entity Located\r\n");

                            CStringW strClass;
                            CStringW strName;
                            float3 nPosition;
                            quaternion nRotation;
                            float3 nLightColor(1.0f, 1.0f, 1.0f);
                            float nLightRange = 0;
                            float3 nLightOffset;

                            int iLevel = 1;
                            while (kParser.NextToken())
                            {
                                if (kParser.GetToken()[0] == '{')
                                {
                                    iLevel++;
                                }
                                else if (kParser.GetToken()[0] == '}')
                                {
                                    if (iLevel == 1)
                                    {
                                        break;
                                    }
                                    else
                                    {
                                        iLevel--;
                                    }
                                }
                                else if (iLevel == 1)
                                {
                                    CStringW strToken = kParser.GetToken();
                                    kParser.NextToken();
                                    CStringW strValue = kParser.GetToken();

                                    printf("-> %S: %S\r\n", strToken.GetString(), strValue.GetString());

                                    if (strToken.CompareNoCase(L"origin") == 0)
                                    {
                                        swscanf_s(strValue.GetString(), L"%f %f %f", &nPosition.x, &nPosition.z, &nPosition.y);
                                        nPosition *= nScale;
                                    }
                                    else if (strToken.CompareNoCase(L"rotation") == 0)
                                    {
                                        float4x4 nMatrix;
                                        swscanf_s(strValue.GetString(), L"%f %f %f %f %f %f %f %f %f",
                                            &nMatrix._11, &nMatrix._13, &nMatrix._12,
                                            &nMatrix._31, &nMatrix._33, &nMatrix._32,
                                            &nMatrix._21, &nMatrix._23, &nMatrix._22);
                                        nRotation = nMatrix;
                                    }
                                    else if (strToken.CompareNoCase(L"angle") == 0)
                                    {
                                        nRotation = quaternion(0.0f, _DEGTORAD(StrToFloat(strValue)), 0.0f);
                                    }
                                    else if (strToken.CompareNoCase(L"light_radius") == 0)
                                    {
                                        float3 nRange;
                                        swscanf_s(strValue.GetString(), L"%f %f %f", &nRange.x, &nRange.z, &nRange.y);
                                        nLightRange = (nRange.GetLength() * nScale);
                                    }
                                    else if (strToken.CompareNoCase(L"light") == 0)
                                    {
                                        nLightRange = (float(wcstod(strValue, nullptr)) * nScale);
                                    }
                                    else if (strToken.CompareNoCase(L"light_center") == 0)
                                    {
                                        swscanf_s(strValue.GetString(), L"%f %f %f", &nLightOffset.x, &nLightOffset.z, &nLightOffset.y);
                                        nLightOffset *= nScale;
                                    }
                                    else if (strToken.CompareNoCase(L"_color") == 0)
                                    {
                                        swscanf_s(strValue.GetString(), L"%f %f %f", &nLightColor.r, &nLightColor.g, &nLightColor.b);
                                    }
                                    else if (strToken.CompareNoCase(L"name") == 0)
                                    {
                                        strName = (L" name=\"" + strValue + L"\"");
                                    }
                                    else if (strToken.CompareNoCase(L"classname") == 0)
                                    {
                                        strClass = (L" class=\"" + strValue + L"\"");
                                    }
                                }
                            };

                            fprintf(pFile, "		<entity%S%S>\r\n", strName.GetString(), strClass.GetString());
                            if (nLightRange > 0.0f)
                            {
                                fprintf(pFile, "			<component type=\"light\" color=\"%f, %f, %f\" range=\"%f\" />\r\n",
                                    nLightColor.r, nLightColor.g, nLightColor.b, nLightRange);
                                nPosition += nLightOffset;
                            }

                            fprintf(pFile, "			<component type=\"transform\" position=\"%f, %f, %f\" rotation=\"%f, %f, %f, %f\" />\r\n",
                                nPosition.x, nPosition.y, nPosition.z, nRotation.x, nRotation.y, nRotation.z, nRotation.w);
                            fprintf(pFile, "		</entity>\r\n");
                        }
                    };
                }

                fprintf(pFile, "	</population>\r\n");
                fprintf(pFile, "</world>\r\n");
                fclose(pFile);
            }
        }
        
        if (true)
        {
            aabb nAABB;
            std::map<CStringA, MODEL> aModels;
            std::map<UINT32, MODEL *> aAreas;
            std::vector<NODE> aNodes;
            std::vector<float3> aPortalEdges;
            std::vector<PORTAL> aPortals;

            CGEKParser kParser;
            if (SUCCEEDED(kParser.LoadFromFile(strInput + L".proc")))
            {
                bool bQuake4 = false;

	            kParser.NextToken();
	            if (kParser.GetToken().CompareNoCase(L"mapProcFile003") == 0)
	            {
                    printf("> Doom 3 proc format detected\r\n");
                }
                else if (kParser.GetToken().CompareNoCase(L"PROC") == 0)
                {
                    kParser.NextToken();
                    printf("> Quake 4 proc format detected\r\n");
                    if (StrToFloat(kParser.GetToken()) == 4)
                    {
                        bQuake4 = true;
                        printf("> Quake 4 version format detected\r\n");
                    }
                    else
                    {
                        throw CMyException(__LINE__, L"Unknown proc version detected");
                    }
                }
                else
                {
                    throw CMyException(__LINE__, L"Invalid Map File");
	            }

	            while (kParser.NextToken())
	            {
		            if (kParser.GetToken().CompareNoCase(L"model") == 0)
		            {
	                    kParser.NextToken();
	                    if (kParser.GetToken()[0] != '{')
	                    {
                            throw CMyException(__LINE__, L"Invalid Model Section");
	                    }

	                    kParser.NextToken();
	                    CStringA strModelName = kParser.GetToken();
                        strModelName.MakeLower();

	                    kParser.NextToken();
	                    UINT32 nNumSurfaces = StrToUINT32(kParser.GetToken());

                        printf("> Model: %s, %d surfaces\r\n", strModelName.GetString(), nNumSurfaces);

                        if (bQuake4 && strModelName.Left(5).CompareNoCase("_area") == 0)
                        {
                            kParser.NextToken();
                        }

                        MODEL &kModel = aModels[strModelName];
                        for(UINT32 nSurface = 0; nSurface < nNumSurfaces; nSurface++)
	                    {
		                    kParser.NextToken();
		                    if (kParser.GetToken()[0] != '{')
		                    {
                                throw CMyException(__LINE__, L"Invalid Surface Section");
		                    }

		                    kParser.NextToken();
                            CStringA strMaterialName = kParser.GetToken();
                            strMaterialName.MakeLower();
                            if (strMaterialName.Find("textures/") == 0)
                            {
                                strMaterialName = strMaterialName.Mid(9);
                            }
                            else if (strMaterialName.Find("models/") == 0)
                            {
                                strMaterialName = strMaterialName.Mid(7);
                            }
                            
                            MATERIAL &kSurface = kModel.m_aMaterials[strMaterialName];
                            kSurface.m_nFirstVertex = kModel.m_aVertices.size();
                            kSurface.m_nFirstIndex = kModel.m_aIndices.size();

		                    kParser.NextToken();
	                        UINT32 nNumVertices = StrToUINT32(kParser.GetToken());

		                    kParser.NextToken();
	                        UINT32 nNumIndices = StrToUINT32(kParser.GetToken());
                            kSurface.m_nNumIndices = nNumIndices;

                            printf("-> Surface: %s, %d vertices, %d indices\r\n", strMaterialName.GetString(), nNumVertices, nNumIndices);
                            
                            std::vector<VERTEX> aVertices(nNumVertices);
		                    for(UINT32 nVertex = 0; nVertex < nNumVertices; nVertex++)
		                    {
			                    kParser.NextToken();
			                    if (kParser.GetToken()[0] != '(')
			                    {
                                    throw CMyException(__LINE__, L"Invalid Vertex Section");
			                    }

                                VERTEX &kVertex = aVertices[nVertex];

                                kParser.NextToken();
                                kVertex.position.x = StrToFloat(kParser.GetToken());

                                kParser.NextToken();
                                kVertex.position.z = StrToFloat(kParser.GetToken());

                                kParser.NextToken();
                                kVertex.position.y = StrToFloat(kParser.GetToken());

                                kVertex.position *= nScale;
                                kModel.Extend(kVertex.position);
                                nAABB.Extend(kVertex.position);

                                kParser.NextToken();
                                kVertex.texcoord.u = StrToFloat(kParser.GetToken());

                                kParser.NextToken();
                                kVertex.texcoord.v = StrToFloat(kParser.GetToken());

                                kParser.NextToken();
                                kVertex.normal.x = StrToFloat(kParser.GetToken());

                                kParser.NextToken();
                                kVertex.normal.z = StrToFloat(kParser.GetToken());

                                kParser.NextToken();
                                kVertex.normal.y = StrToFloat(kParser.GetToken());

			                    kParser.NextToken();
			                    if (kParser.GetToken()[0] != ')')
			                    {
                                    throw CMyException(__LINE__, L"Invalid Vertex Section");
			                    }
		                    }

                            std::vector<UINT16> aIndices(nNumIndices);
		                    for(UINT32 nIndex = 0; nIndex < nNumIndices; nIndex++)
		                    {
			                    kParser.NextToken();
                                UINT32 nVertexIndex = StrToUINT32(kParser.GetToken());
                                if (nVertexIndex >= 65535)
                                {
                                    throw CMyException(__LINE__, L"Invalid Index Encountered");
                                    nVertexIndex = 0;
                                }

                                aIndices[nIndex] = nVertexIndex;
		                    }

                            float nFaceEpsilon = 0.0833333f;
                            float nPartialEdgeThreshold = -1.01f;
                            float nSingularPointThreshold = -0.01f;
                            float nNormalEdgeThreshold = -1.01f;

                            std::vector<VERTEX> aOutputVertices;
                            std::vector<UINT16> aOutputIndices;
                            if (FAILED(GEKOptimizeMesh(&aVertices[0], aVertices.size(), &aIndices[0], aIndices.size(), aOutputVertices, aOutputIndices, 
                                nFaceEpsilon, nPartialEdgeThreshold, nSingularPointThreshold, nNormalEdgeThreshold)))
                            {
                                throw CMyException(__LINE__, L"Unable to optimize mesh data");
                            }

                            kModel.m_aVertices.insert(kModel.m_aVertices.end(), aOutputVertices.begin(), aOutputVertices.end());
                            kModel.m_aIndices.insert(kModel.m_aIndices.end(), aOutputIndices.begin(), aOutputIndices.end());

		                    kParser.NextToken();
		                    if (kParser.GetToken()[0] != '}')
		                    {
                                throw CMyException(__LINE__, L"Invalid Surface Section");
		                    }
	                    }

                        if (strModelName.Left(5).CompareNoCase("_area") == 0)
                        {
                            CStringW strArea = strModelName.Mid(5);
                            aAreas[StrToUINT32(strArea)] = &kModel;
                            printf("> Area %S Added to World\r\n", strArea.GetString());
                        }

	                    kParser.NextToken();
	                    if (kParser.GetToken()[0] != '}')
	                    {
                            throw CMyException(__LINE__, L"Invalid Model Section");
	                    }
		            }
		            else if (kParser.GetToken().CompareNoCase(L"nodes") == 0)
		            {
	                    kParser.NextToken();
	                    if (kParser.GetToken()[0] != '{')
	                    {
                            throw CMyException(__LINE__, L"Invalid Nodes Section");
	                    }

	                    kParser.NextToken();
			            UINT32 nNumNodes = StrToUINT32(kParser.GetToken());

                        printf("> Num. Nodes: %d\r\n", nNumNodes);

                        aNodes.resize(nNumNodes);
                        for(UINT32 nNode = 0; nNode < nNumNodes; nNode++)
	                    {
		                    kParser.NextToken();
			                if (kParser.GetToken()[0] != '(')
			                {
                                throw CMyException(__LINE__, L"Invalid Node Section");
			                }

                            NODE &kNode = aNodes[nNode];

                            kParser.NextToken();
                            kNode.normal.x = StrToFloat(kParser.GetToken());

                            kParser.NextToken();
                            kNode.normal.z = StrToFloat(kParser.GetToken());

                            kParser.NextToken();
                            kNode.normal.y = StrToFloat(kParser.GetToken());

                            kParser.NextToken();
                            kNode.distance = -StrToFloat(kParser.GetToken());

                            kNode.distance *= nScale;

                            kParser.NextToken();
			                if (kParser.GetToken()[0] != ')')
			                {
                                throw CMyException(__LINE__, L"Invalid Node Section");
			                }

			                kParser.NextToken();
                            kNode.m_nPositiveChild = StrToINT32(kParser.GetToken());

                            kParser.NextToken();
                            kNode.m_nNegativeChild = StrToINT32(kParser.GetToken());
	                    }

	                    kParser.NextToken();
	                    if (kParser.GetToken()[0] != '}')
	                    {
                            throw CMyException(__LINE__, L"Invalid Nodes Section");
	                    }
		            }
		            else if (kParser.GetToken().CompareNoCase(L"interAreaPortals") == 0)
		            {
	                    kParser.NextToken();
	                    if (kParser.GetToken()[0] != '{')
	                    {
                            throw CMyException(__LINE__, L"Invalid Portals Section");
	                    }

	                    kParser.NextToken();
			            UINT32 nNumAreas = StrToUINT32(kParser.GetToken());

	                    kParser.NextToken();
			            UINT32 nNumPortals = StrToUINT32(kParser.GetToken());

                        printf("> Num. Areas: %d\r\n", nNumAreas);
                        printf("> Num. Portals: %d\r\n", nNumPortals);

                        aPortals.resize(nNumPortals);
                        for(UINT32 nPortal = 0; nPortal < nNumPortals; nPortal++)
	                    {
                            PORTAL &kPortal = aPortals[nPortal];

                            kParser.NextToken();
                            kPortal.m_nNumEdges = StrToUINT32(kParser.GetToken());

			                kParser.NextToken();
                            kPortal.m_nPositiveArea = StrToINT32(kParser.GetToken());

			                kParser.NextToken();
                            kPortal.m_nNegativeArea = StrToINT32(kParser.GetToken());
                            kPortal.m_nFirstEdge = aPortalEdges.size();

                            std::vector<float3> aCurrentEdges(kPortal.m_nNumEdges);
		                    for(UINT32 nEdge = 0; nEdge < kPortal.m_nNumEdges; nEdge++)
		                    {
		                        kParser.NextToken();
			                    if (kParser.GetToken()[0] != '(')
			                    {
                                    throw CMyException(__LINE__, L"Invalid Point Section");
			                    }

                                float3 &nCoordinate = aCurrentEdges[kPortal.m_nNumEdges - nEdge - 1];

			                    kParser.NextToken();
                                nCoordinate.x = StrToFloat(kParser.GetToken());

			                    kParser.NextToken();
                                nCoordinate.z = StrToFloat(kParser.GetToken());

			                    kParser.NextToken();
                                nCoordinate.y = StrToFloat(kParser.GetToken());

                                nCoordinate *= nScale;

                                kParser.NextToken();
			                    if (kParser.GetToken()[0] != ')')
			                    {
                                    throw CMyException(__LINE__, L"Invalid Point Section");
			                    }
		                    }

                            aPortalEdges.insert(aPortalEdges.begin(), aCurrentEdges.begin(), aCurrentEdges.end());
	                    }

	                    kParser.NextToken();
	                    if (kParser.GetToken()[0] != '}')
	                    {
                            throw CMyException(__LINE__, L"Invalid Portals Section");
	                    }
		            }
	            };
            }
            else
            {
                throw CMyException(__LINE__, L"Unable to Load Input: %s\r\n", strInput.GetString());
            }

            printf("> Num. Areas Detected: %d\r\n", aAreas.size());
            printf("> Size: (%5.3f, %5.3f, %5.3f) x\r\n", nAABB.minimum.x, nAABB.minimum.y, nAABB.minimum.z);
            printf("        (%5.3f, %5.3f, %5.3f)\r\n", nAABB.maximum.x, nAABB.maximum.y, nAABB.maximum.z);
                            

            FILE *pFile = nullptr;
            _wfopen_s(&pFile, strOutput + ".gek", L"wb");
            if (pFile != nullptr)
            {
                UINT32 nGEKX = *(UINT32 *)"GEKX";
                UINT16 nType = 10;
                UINT16 nVersion = 1;
                fwrite(&nGEKX, sizeof(UINT32), 1, pFile);
                fwrite(&nType, sizeof(UINT16), 1, pFile);
                fwrite(&nVersion, sizeof(UINT16), 1, pFile);

                fwrite(&nAABB, sizeof(aabb), 1, pFile);

                UINT32 nNumAreas = aAreas.size();
                fwrite(&nNumAreas, sizeof(UINT32), 1, pFile);
                for (auto &kPair : aAreas)
                {
                    UINT32 nNumMaterials = kPair.second->m_aMaterials.size();
                    fwrite(&nNumMaterials, sizeof(UINT32), 1, pFile);
                    for (auto &kMaterial : kPair.second->m_aMaterials)
                    {
                        CStringA strMaterial = (strGame + kMaterial.first);
                        fwrite(strMaterial.GetString(), (strMaterial.GetLength() + 1), 1, pFile);
                        fwrite(&kMaterial.second, sizeof(MATERIAL), 1, pFile);
                    }

                    UINT32 nNumVertices = kPair.second->m_aVertices.size();
                    fwrite(&nNumVertices, sizeof(UINT32), 1, pFile);
                    if (nNumVertices > 0)
                    {
                        for (UINT32 nVertex = 0; nVertex < nNumVertices; nVertex++)
                        {
                            fwrite(&kPair.second->m_aVertices[nVertex].position, sizeof(float3), 1, pFile);
                        }

                        for (UINT32 nVertex = 0; nVertex < nNumVertices; nVertex++)
                        {
                            fwrite(&kPair.second->m_aVertices[nVertex].texcoord, sizeof(float2), 1, pFile);
                        }

                        for (UINT32 nVertex = 0; nVertex < nNumVertices; nVertex++)
                        {
                            fwrite(&kPair.second->m_aVertices[nVertex].tangent, sizeof(float3), 1, pFile);
                            fwrite(&kPair.second->m_aVertices[nVertex].bitangent, sizeof(float3), 1, pFile);
                            fwrite(&kPair.second->m_aVertices[nVertex].normal, sizeof(float3), 1, pFile);
                        }
                    }

                    UINT32 nNumIndices = kPair.second->m_aIndices.size();
                    fwrite(&nNumIndices, sizeof(UINT32), 1, pFile);
                    if (nNumIndices > 0)
                    {
                        fwrite(&kPair.second->m_aIndices[0], sizeof(UINT16), nNumIndices, pFile);
                    }

                    printf("-> Area %d: %d materials, %d vertices, %d indices\r\n", kPair.first, nNumMaterials, nNumVertices, nNumIndices);
                }

                UINT32 nNumNodes = aNodes.size();
                fwrite(&nNumNodes, sizeof(UINT32), 1, pFile);
                if (nNumNodes > 0)
                {
                    fwrite(&aNodes[0], sizeof(NODE), nNumNodes, pFile);
                }

                UINT32 nNumPortalEdges = aPortalEdges.size();
                fwrite(&nNumPortalEdges, sizeof(UINT32), 1, pFile);
                if (nNumPortalEdges > 0)
                {
                    fwrite(&aPortalEdges[0], sizeof(float3), nNumPortalEdges, pFile);
                }

                UINT32 nNumPortals = aPortals.size();
                fwrite(&nNumPortals, sizeof(UINT32), 1, pFile);
                if (nNumPortals > 0)
                {
                    fwrite(&aPortals[0], sizeof(PORTAL), nNumPortals, pFile);
                }

                fclose(pFile);
                printf("< Output Saved: %S\r\n", strOutput.GetString());
            }
            else
            {
                throw CMyException(__LINE__, L"Unable to Save Output: %S\r\n", strOutput.GetString());
            }
        }
        else
        {
            throw CMyException(__LINE__, L"Unable to Detect Input Type: %s\r\n", strInput.GetString());
        }
    }
    catch (CMyException kException)
    {
        printf("! Error (%d): %S", kException.m_nLine, kException.m_strMessage.GetString());
    }
    catch (...)
    {
    }

    return 0;
}