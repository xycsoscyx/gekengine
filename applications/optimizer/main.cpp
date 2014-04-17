#include "GEKMath.h"
#include "GEKUtility.h"
#include "GEKMesh.h"
#include <algorithm>
#include <map>

struct MATERIAL
{
    UINT32 m_nFirstVertex;
    UINT32 m_nFirstIndex;
    UINT32 m_nNumIndices;
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
    printf("GEK Mesh Optimizer\r\n");

    CStringW strInput;
    CStringW strOutput;
    float nFaceEpsilon = 0.025f;
    float nPartialEdgeThreshold = 0.01f;
    float nSingularPointThreshold = 0.25f;
    float nNormalEdgeThreshold = 0.01f;
    for (int nArgument = 1; nArgument < nNumArguments; nArgument++)
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
        else if (strArgument.CompareNoCase(L"-epsilon") == 0 && nArgument < (nNumArguments - 1))
        {
            nFaceEpsilon = StrToFloat(astrArguments[++nArgument]);
        }
        else if (strArgument.CompareNoCase(L"-edge") == 0 && nArgument < (nNumArguments - 1))
        {
            nPartialEdgeThreshold = StrToFloat(astrArguments[++nArgument]);
        }
        else if (strArgument.CompareNoCase(L"-point") == 0 && nArgument < (nNumArguments - 1))
        {
            nSingularPointThreshold = StrToFloat(astrArguments[++nArgument]);
        }
        else if (strArgument.CompareNoCase(L"-normal") == 0 && nArgument < (nNumArguments - 1))
        {
            nNormalEdgeThreshold = StrToFloat(astrArguments[++nArgument]);
        }
    }

    if (strInput.IsEmpty() || strOutput.IsEmpty())
    {
        printf("Arguments:\r\n");
        printf("-input <input filename>\r\n");
        printf("-output <output filename>\r\n");
        printf("-epsilon <face epsilon>\r\n");
        printf("-edge <partial edge threshold>\r\n");
        printf("-point <singular point threshold>\r\n");
        printf("-normal <normal edge threshold>\r\n");
        return -1;
    }
    else
    {
        printf("Input: %S\r\n", strInput.GetString());
        printf("Output: %S\r\n", strOutput.GetString());
        printf("Face Epsilon: %f\r\n", nFaceEpsilon);
        printf("Partial Edge Threshold: %f\r\n", nPartialEdgeThreshold);
        printf("Singular Point Threshold: %f\r\n", nSingularPointThreshold);
        printf("Normal Edge Threshold: %f\r\n", nNormalEdgeThreshold);
    }

    try
    {
        std::vector<UINT8> aBuffer;
        if (FAILED(GEKLoadFromFile(strInput, aBuffer)))
        {
            throw CMyException(__LINE__, L"Unable to Load Input: %s", strInput.GetString());
        }

        char *pBuffer = (char *)&aBuffer[0];
        char *pBufferEnd = (pBuffer + aBuffer.size());

        UINT32 nGEKX = *((UINT32 *)pBuffer);
        pBuffer += sizeof(UINT32);
        if (pBuffer >= pBufferEnd)
        {
            throw CMyException(__LINE__, L"End of File Encountered");
        }

        if (nGEKX != *(UINT32 *)"GEKX")
        {
            throw CMyException(__LINE__, L"Invalid magic header encountered (0x%08X): %s", nGEKX, strInput.GetString());
        }

        UINT32 nVersion = *((UINT32 *)pBuffer);
        pBuffer += sizeof(UINT32);
        if (pBuffer >= pBufferEnd)
        {
            throw CMyException(__LINE__, L"End of File Encountered");
        }

        aabb nAABB;
        std::map<CStringA, std::vector<VERTEX>> aMaterials;
        if (nVersion == 1)
        {
            printf("> Version 1 Model Detected\r\n");

            UINT32 nNumMaterials = *((UINT32 *)pBuffer);
            pBuffer += sizeof(UINT32);

            printf("> Num. Materials: %d\r\n", nNumMaterials);

            for(UINT32 nMaterial = 0; nMaterial < nNumMaterials; nMaterial++)
            {
                CStringA strMaterial = pBuffer;
                printf("-> Material: %s\r\n", strMaterial.GetString());
                pBuffer += (strMaterial.GetLength() + 1);
                if (pBuffer >= pBufferEnd)
                {
                    throw CMyException(__LINE__, L"End of File Encountered");
                }

                UINT32 nNumVertices = *((UINT32 *)pBuffer);
                printf("-->  Num. Vertices: %d\r\n", nNumVertices);
                pBuffer += sizeof(UINT32);
                if (pBuffer >= pBufferEnd)
                {
                    throw CMyException(__LINE__, L"End of File Encountered");
                }

                VERTEX *pVertices = (VERTEX *)pBuffer;
                pBuffer += (sizeof(VERTEX) * nNumVertices);
                if (pBuffer >= pBufferEnd)
                {
                    throw CMyException(__LINE__, L"End of File Encountered");
                }

                UINT32 nNumIndices = *((UINT32 *)pBuffer);
                printf("---> Num. Indices: %d\r\n", nNumIndices);
                pBuffer += sizeof(UINT32);
                if (pBuffer >= pBufferEnd)
                {
                    throw CMyException(__LINE__, L"End of File Encountered");
                }

                UINT16 *pIndices = (UINT16 *)pBuffer;
                pBuffer += (sizeof(UINT16) * nNumIndices);
                if (pBuffer >= pBufferEnd)
                {
                    throw CMyException(__LINE__, L"End of File Encountered");
                }

                std::vector<VERTEX> &aVertices = aMaterials[strMaterial];
                for(UINT32 nIndex = 0; nIndex < nNumIndices; nIndex++)
                {
                    UINT32 nVertex = pIndices[nIndex];
                    aVertices.push_back(pVertices[nVertex]);
                    nAABB.Extend(pVertices[nVertex].position);
                }
            }
        }
        else if (nVersion == 2)
        {
            printf("> Version 2 Model Detected\r\n");

            pBuffer += sizeof(aabb);
            if (pBuffer >= pBufferEnd)
            {
                throw CMyException(__LINE__, L"End of File Encountered");
            }

            UINT32 nNumMaterials = *((UINT32 *)pBuffer);
            pBuffer += sizeof(UINT32);

            printf("> Num. Materials: %d\r\n", nNumMaterials);

            std::map<CStringA, MATERIAL> aMaterialGroups;
            for(UINT32 nMaterial = 0; nMaterial < nNumMaterials; nMaterial++)
            {
                CStringA strMaterial = pBuffer;
                printf("-> Material: %s\r\n", strMaterial.GetString());
                pBuffer += (strMaterial.GetLength() + 1);
                if (pBuffer >= pBufferEnd)
                {
                    throw CMyException(__LINE__, L"End of File Encountered");
                }

                MATERIAL &kMaterial = aMaterialGroups[strMaterial];
                kMaterial.m_nFirstVertex = *((UINT32 *)pBuffer);
                printf("--> First Vertex: %d\r\n", kMaterial.m_nFirstVertex);
                pBuffer += sizeof(UINT32);
                if (pBuffer >= pBufferEnd)
                {
                    throw CMyException(__LINE__, L"End of File Encountered");
                }

                kMaterial.m_nFirstIndex = *((UINT32 *)pBuffer);
                printf("--> First Index: %d\r\n", kMaterial.m_nFirstIndex);
                if (pBuffer >= pBufferEnd)
                {
                    throw CMyException(__LINE__, L"End of File Encountered");
                }

                kMaterial.m_nNumIndices = *((UINT32 *)pBuffer);
                printf("--> Num. Indices: %d\r\n", kMaterial.m_nNumIndices);
                pBuffer += sizeof(UINT32);
                if (pBuffer >= pBufferEnd)
                {
                    throw CMyException(__LINE__, L"End of File Encountered");
                }
            }

            UINT32 nNumVertices = *((UINT32 *)pBuffer);
            printf("-> Num. Vertices: %d\r\n", nNumVertices);
            pBuffer += sizeof(UINT32);
            if (pBuffer >= pBufferEnd)
            {
                throw CMyException(__LINE__, L"End of File Encountered");
            }

            VERTEX *pVertices = (VERTEX *)pBuffer;
            pBuffer += (sizeof(VERTEX) * nNumVertices);
            if (pBuffer >= pBufferEnd)
            {
                throw CMyException(__LINE__, L"End of File Encountered");
            }

            UINT32 nNumIndices = *((UINT32 *)pBuffer);
            printf("-> Num. Indices: %d\r\n", nNumIndices);
            pBuffer += sizeof(UINT32);
            if (pBuffer >= pBufferEnd)
            {
                throw CMyException(__LINE__, L"End of File Encountered");
            }

            UINT16 *pIndices = (UINT16 *)pBuffer;
            pBuffer += (sizeof(UINT16) * nNumIndices);
            if (pBuffer != pBufferEnd)
            {
                throw CMyException(__LINE__, L"End of File Encountered");
                return -5;
            }

            for (auto &kPair : aMaterialGroups)
            {
                std::vector<VERTEX> &aVertices = aMaterials[kPair.first];
                aVertices.resize(kPair.second.m_nNumIndices);

                for (UINT32 nIndex = 0; nIndex < kPair.second.m_nNumIndices; nIndex++)
                {
                    UINT32 nVertex = (kPair.second.m_nFirstVertex + pIndices[kPair.second.m_nFirstIndex + nIndex]);
                    aVertices[nIndex] = pVertices[nVertex];
                    nAABB.Extend(pVertices[nVertex].position);
                }
            }
        }
        else
        {
            throw CMyException(__LINE__, L"Invalid header version encountered (%d): %S", nVersion, strInput.GetString());
        }

        printf("< Num. Simplified Materials: %d\r\n", aMaterials.size());

        FILE *pFile = nullptr;
        _wfopen_s(&pFile, strOutput, L"wb");
        if (pFile != nullptr)
        {
            UINT32 nGEKX = *(UINT32 *)"GEKX";
            UINT16 nType = 0;
            UINT16 nVersion = 2;
            UINT32 nNumMaterials = aMaterials.size();
            fwrite(&nGEKX, sizeof(UINT32), 1, pFile);
            fwrite(&nType, sizeof(UINT16), 1, pFile);
            fwrite(&nVersion, sizeof(UINT16), 1, pFile);
            fwrite(&nAABB, sizeof(aabb), 1, pFile);
            fwrite(&nNumMaterials, sizeof(UINT32), 1, pFile);

            std::vector<UINT16> aTotalIndices;
            std::vector<VERTEX> aTotalVertices;
            for (auto &kPair : aMaterials)
            {
                CStringA strMaterial = (kPair.first);
                fwrite(strMaterial.GetBuffer(), (strMaterial.GetLength() + 1), 1, pFile);

                printf("-< Material: %s\r\n", strMaterial.GetString());

                std::vector<VERTEX> &aVertices = (kPair.second);
                printf("--< Num. Base Faces: %d\r\n", aVertices.size());
                printf("--< Optimizing and Generating Tangent Frame\r\n");

                std::vector<VERTEX> aOutputVertices;
                std::vector<UINT16> aOutputIndices;
                if (FAILED(GEKOptimizeMesh(&aVertices[0], aVertices.size(), nullptr, 0, aOutputVertices, aOutputIndices, true, nFaceEpsilon, nPartialEdgeThreshold, nSingularPointThreshold, nNormalEdgeThreshold)))
                {
                    throw CMyException(__LINE__, L"Unable to optimize mesh data");
                }

                UINT32 nFirstVertex = aTotalVertices.size();
                UINT32 nFirstIndex = aTotalIndices.size();
                UINT32 nNumIndices = aOutputIndices.size();
                fwrite(&nFirstVertex, sizeof(UINT32), 1, pFile);
                fwrite(&nFirstIndex, sizeof(UINT32), 1, pFile);
                fwrite(&nNumIndices, sizeof(UINT32), 1, pFile);

                aTotalVertices.insert(aTotalVertices.end(), aOutputVertices.begin(), aOutputVertices.end());
                aTotalIndices.insert(aTotalIndices.end(), aOutputIndices.begin(), aOutputIndices.end());
            }

            UINT32 nNumVertices = aTotalVertices.size();
            fwrite(&nNumVertices, sizeof(UINT32), 1, pFile);
            printf("-< Num. Total Vertices: %d\r\n", aTotalVertices.size());
            for (UINT32 nVertex = 0; nVertex < nNumVertices; nVertex++)
            {
                fwrite(&aTotalVertices[nVertex].position, sizeof(float3), 1, pFile);
            }

            for (UINT32 nVertex = 0; nVertex < nNumVertices; nVertex++)
            {
                fwrite(&aTotalVertices[nVertex].texcoord, sizeof(float2), 1, pFile);
            }

            for (UINT32 nVertex = 0; nVertex < nNumVertices; nVertex++)
            {
                fwrite(&aTotalVertices[nVertex].tangent, sizeof(float3), 1, pFile);
                fwrite(&aTotalVertices[nVertex].bitangent, sizeof(float3), 1, pFile);
                fwrite(&aTotalVertices[nVertex].normal, sizeof(float3), 1, pFile);
            }

            UINT32 nNumIndices = aTotalIndices.size();
            fwrite(&nNumIndices, sizeof(UINT32), 1, pFile);
            fwrite(&aTotalIndices[0], sizeof(UINT16), aTotalIndices.size(), pFile);
            printf("-< Num. Total Indices: %d\r\n", aTotalIndices.size());

            fclose(pFile);
            printf("< Output Saved: %S\r\n", strOutput.GetString());
        }
        else
        {
            throw CMyException(__LINE__, L"! Unable to Save Output: %S\r\n", strOutput.GetString());
        }
    }
    catch (CMyException kException)
    {
        printf("! Error (%d): %S", kException.m_nLine, kException.m_strMessage.GetString());
    }
    catch (...)
    {
    }

    printf("\r\n");
    return 0;
}