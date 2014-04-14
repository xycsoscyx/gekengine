#include "GEKMath.h"
#include "GEKUtility.h"
#include <ShlObj.h>
#include <atlbase.h>
#include <atlpath.h>
#include <algorithm>
#include <map>

#include <IL/il.h>
#include <IL/ilu.h>

#pragma comment(lib, "Gdiplus.lib")
#pragma comment(lib, "devil.lib")
#pragma comment(lib, "ilu.lib")

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

CStringW GetFullFileName(LPCWSTR pFileName)
{
    CStringW strCorrectFileName = pFileName;
    strCorrectFileName.Replace(L"/", L"\\");

    HANDLE hFile = CreateFile(strCorrectFileName, GENERIC_READ, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (hFile != INVALID_HANDLE_VALUE)
    {
        CStringW strFullFileName;
        GetFinalPathNameByHandleW(hFile, strFullFileName.GetBuffer(MAX_PATH + 1), MAX_PATH, 0);
        strFullFileName.ReleaseBuffer();
        CloseHandle(hFile);

        if (strFullFileName.Find(L"\\\\?\\") == 0)
        {
            strFullFileName = strFullFileName.Mid(4);
        }

        return strFullFileName;
    }

    return strCorrectFileName;
}

CStringW StripRootDirectory(LPCWSTR pPath)
{
    CStringW strShortened = pPath;
    strShortened.MakeLower();
    strShortened.Replace(L"/", L"\\");
    if (strShortened.Find(L"textures\\") == 0)
    {
        strShortened = strShortened.Mid(9);
    }
    else if (strShortened.Find(L"models\\") == 0)
    {
        strShortened = strShortened.Mid(9);
    }

    return strShortened;
}

class CMyImage
{
private:
    std::vector<UINT8> m_aBuffer;
    int m_nXSize;
    int m_nYSize;

public:
    CMyImage(void)
        : m_nXSize(0)
        , m_nYSize(0)
    {
    }

    ~CMyImage(void)
    {
    }

    int GetXSize(void)
    {
        return m_nXSize;
    }

    int GetYSize(void)
    {
        return m_nYSize;
    }

    void Create(int nXSize, int nYSize)
    {
        m_nXSize = nXSize;
        m_nYSize = nYSize;
        m_aBuffer.resize(nXSize * nYSize * 4);
    }

    void Resize(int nXSize, int nYSize)
    {
        if (m_nXSize == nXSize && m_nYSize == nYSize)
        {
            return;
        }

        std::vector<UINT8> aBuffer(nXSize * nYSize * 4);
		float nScaleX = (float)(m_nXSize - 1);
		float nScaleY = (float)(m_nYSize - 1);
		if (nXSize > 1)
        {
            nScaleX /= (float)(nXSize - 1);
        }

		if (nYSize > 1)
        {
            nScaleY /= (float)(nYSize - 1);
        }

		float nXPercent = 0.0f, nXDelta = (1.0f / (float)nXSize);
		float nYPercent = 0.0f, nYDelta = (1.0f / (float)nYSize);
		for(int nYPos = 0; nYPos < nYSize; nYPos++, nYPercent += nYDelta)
		{
			int nY0 = (int)((float)nYPos * nScaleY);
			int nY1 = (nY0 + 1);
			if (nY1 >= m_nYSize)
            {
                nY1 = (m_nYSize - 1);
            }

			nXPercent = 0.0f;
			for(int nXPos = 0; nXPos < nXSize; nXPos++, nXPercent += nXDelta)
			{
				int nX0 = (int)((float)nXPos * nScaleX);
				int nX1 = (nX0 + 1);
				if (nX1 >= m_nXSize)
                {
                    nX1 = (m_nXSize - 1);
                }

				int nR = 0;
				int nG = 0;
				int nB = 0;
				int nA = 0;
				for(int nY = nY0; nY <= nY1; nY++)
				{
					for(int nX = nX0; nX <= nX1; nX++)
					{
						nR += m_aBuffer[(((nY * m_nXSize) + nX) * 4) + 0];
						nG += m_aBuffer[(((nY * m_nXSize) + nX) * 4) + 1];
						nB += m_aBuffer[(((nY * m_nXSize) + nX) * 4) + 2];
						nA += m_aBuffer[(((nY * m_nXSize) + nX) * 4) + 3];
					}
				}

				nR /= ((nY1 - nY0 + 1) * (nX1 - nX0 + 1));
				nG /= ((nY1 - nY0 + 1) * (nX1 - nX0 + 1));
				nB /= ((nY1 - nY0 + 1) * (nX1 - nX0 + 1));
				nA /= ((nY1 - nY0 + 1) * (nX1 - nX0 + 1));
				aBuffer[(((nYPos * nXSize) + nXPos) * 4) + 0] = (unsigned char)(nR > 255 ? 255 : nR);
				aBuffer[(((nYPos * nXSize) + nXPos) * 4) + 1] = (unsigned char)(nG > 255 ? 255 : nG);
				aBuffer[(((nYPos * nXSize) + nXPos) * 4) + 2] = (unsigned char)(nB > 255 ? 255 : nB);
				aBuffer[(((nYPos * nXSize) + nXPos) * 4) + 3] = (unsigned char)(nA > 255 ? 255 : nA);
			}
		}

		m_nXSize = nXSize;
		m_nYSize = nYSize;
        m_aBuffer = aBuffer;
    }

    HRESULT Load(LPCWSTR pTexturesDirectory, LPCWSTR pFileName)
    {
        CStringW strFullName = GetFullFileName(pTexturesDirectory + StripRootDirectory(pFileName));

        std::vector<UINT8> aBuffer;
        HRESULT hRetVal = GEKLoadFromFile(strFullName, aBuffer);
        if (SUCCEEDED(hRetVal))
        {
            unsigned int nImageID = 0;
	        ilGenImages(1, &nImageID);
	        ilBindImage(nImageID);

	        if (ilLoadL(IL_TYPE_UNKNOWN, &aBuffer[0], aBuffer.size()))
	        {
	            ilConvertImage(IL_RGBA, IL_UNSIGNED_BYTE);
	            m_nXSize = ilGetInteger(IL_IMAGE_WIDTH);
	            m_nYSize = ilGetInteger(IL_IMAGE_HEIGHT);
                m_aBuffer.resize(m_nXSize * m_nYSize * 4);
                memcpy(&m_aBuffer[0], ilGetData(), (m_nXSize * m_nYSize * 4));
	        }
            else
            {
		        hRetVal = E_FAIL;
            }

            ilDeleteImages(1, &nImageID);
        }

        return hRetVal;
    }

    HRESULT Save(LPCWSTR pTexturesDirectory, LPCWSTR pFileName)
    {
        CStringW strFullName = GetFullFileName(pTexturesDirectory + StripRootDirectory(pFileName));

        unsigned int uiImageID = 0;
	    ilGenImages(1, &uiImageID);
	    ilBindImage(uiImageID);

	    ilTexImage(m_nXSize, m_nYSize, 1, 4, IL_RGBA, IL_UNSIGNED_BYTE, (void *)&m_aBuffer[0]);
	    iluFlipImage();

	    HRESULT hRetVal = (ilSaveImage(strFullName.GetString()) ? S_OK : E_FAIL);
	    ilDeleteImages(1, &uiImageID);
	    return hRetVal;
    }

    float4 GetPixel(int nXPos, int nYPos)
    {
        if (nXPos < 0 || nXPos >= m_nXSize ||
           nYPos < 0 || nYPos >= m_nYSize)
        {
            return float4(0.0f, 0.0f, 0.0f, 0.0f);
        }

        UINT8 *pData = &m_aBuffer[0];
        pData += (((nYPos * m_nXSize) + nXPos) * 4);
        return float4(float(pData[0]), float(pData[1]), float(pData[2]), float(pData[3]));
    }

    void SetPixel(int nXPos, int nYPos, const float4 &nColor)
    {
        if (nXPos >= 0 && nXPos < m_nXSize &&
           nYPos >= 0 && nYPos < m_nYSize)
        {
            UINT8 *pData = &m_aBuffer[0];
            pData += (((nYPos * m_nXSize) + nXPos) * 4);
            pData[0] = UINT8(nColor.r < 0.0f ? 0.0f : (nColor.r > 255.0f ? 255.0f : nColor.r));
            pData[1] = UINT8(nColor.a < 0.0f ? 0.0f : (nColor.g > 255.0f ? 255.0f : nColor.g));
            pData[2] = UINT8(nColor.b < 0.0f ? 0.0f : (nColor.b > 255.0f ? 255.0f : nColor.b));
            pData[3] = UINT8(nColor.a < 0.0f ? 0.0f : (nColor.a > 255.0f ? 255.0f : nColor.a));
        }
    }
};

CStringW GetFullMapValue(CGEKParser &kParser)
{
    kParser.NextToken();
    if (kParser.GetToken().CompareNoCase(L"heightmap") == 0 ||
       kParser.GetToken().CompareNoCase(L"addnormals") == 0 ||
       kParser.GetToken().CompareNoCase(L"smoothnormals") == 0 ||
       kParser.GetToken().CompareNoCase(L"add") == 0 ||
       kParser.GetToken().CompareNoCase(L"scale") == 0 ||
       kParser.GetToken().CompareNoCase(L"invertAlpha") == 0 ||
       kParser.GetToken().CompareNoCase(L"invertColor") == 0 ||
       kParser.GetToken().CompareNoCase(L"makeIntensity") == 0 ||
       kParser.GetToken().CompareNoCase(L"makeAlpha") == 0)
    {
        CStringW strMap = kParser.GetToken();
        int nDepth = 0;
        do
        {
            kParser.NextToken();
            if (kParser.GetToken()[0] == '(')
            {
                nDepth++;
            }
            else if (kParser.GetToken()[0] == ')')
            {
                nDepth--;
            }

            strMap += kParser.GetToken();
        } while (nDepth > 0);
        return strMap;
    }
    else 
    {
        return kParser.GetToken();
    }
}

CStringW GetCombinedMap(LPCWSTR pTexturesDirectory, const CStringW &strValue)
{
    if (strValue.Find(L"(") < 0)
    {
        return strValue;
    }

    int nInside = strValue.ReverseFind(L'(');
    int nOutside = strValue.Find(L')');
    if (nInside < 0 || nOutside < 0)
    {
        throw CMyException(__LINE__, L"Invalid map block encountered");
    }

    int nAction = strValue.Mid(0, nInside).ReverseFind(L',');
    if (nAction < 0)
    {
        nAction = 0;
    }
    else
    {
        nAction++;
    }

    CStringW strFinalMap;
    CStringW strAction = strValue.Mid(nAction, (nInside - nAction));
    CStringW strData = strValue.Mid(nInside + 1, (nOutside - nInside - 1));
    printf("--> Action %S: %S\r\n", strAction.GetString(), strData.GetString());
    if (strAction.CompareNoCase(L"heightmap") == 0)
    {
        int nPosition = 0;
        CStringW strMap = strData.Tokenize(L",", nPosition);
        float nScale = StrToFloat(strData.Tokenize(L",", nPosition));

        CMyImage kHeightMap;
        if (SUCCEEDED(kHeightMap.Load(pTexturesDirectory, strMap)))
        {
            CMyImage kNormalMap;
            kNormalMap.Create(kHeightMap.GetXSize(), kHeightMap.GetYSize());
	        for(int nYPos = 0; nYPos < kHeightMap.GetYSize(); nYPos++)
	        {
		        for(int nXPos = 0; nXPos < kHeightMap.GetXSize(); nXPos++)
		        {
                    float3 nDeltaU(0.0f, 0.0f, 0.0f);
                    nDeltaU += (kHeightMap.GetPixel((nXPos - 1), (nYPos - 1)) * nScale * -1.0f * (1.0f / 6.0f));
                    nDeltaU += (kHeightMap.GetPixel((nXPos - 1), (nYPos + 0)) * nScale * -1.0f * (1.0f / 6.0f));
                    nDeltaU += (kHeightMap.GetPixel((nXPos - 1), (nYPos + 1)) * nScale * -1.0f * (1.0f / 6.0f));
                    nDeltaU += (kHeightMap.GetPixel((nXPos + 1), (nYPos - 1)) * nScale *  1.0f * (1.0f / 6.0f));
                    nDeltaU += (kHeightMap.GetPixel((nXPos + 1), (nYPos + 0)) * nScale *  1.0f * (1.0f / 6.0f));
                    nDeltaU += (kHeightMap.GetPixel((nXPos + 1), (nYPos + 1)) * nScale *  1.0f * (1.0f / 6.0f));

                    float3 nDeltaV(0.0f, 0.0f, 0.0f);
                    nDeltaV += (kHeightMap.GetPixel((nXPos - 1), (nYPos - 1)) * nScale * -1.0f * (1.0f / 6.0f));
                    nDeltaV += (kHeightMap.GetPixel((nXPos + 0), (nYPos - 1)) * nScale * -1.0f * (1.0f / 6.0f));
                    nDeltaV += (kHeightMap.GetPixel((nXPos + 1), (nYPos - 1)) * nScale * -1.0f * (1.0f / 6.0f));
                    nDeltaV += (kHeightMap.GetPixel((nXPos - 1), (nYPos + 1)) * nScale *  1.0f * (1.0f / 6.0f));
                    nDeltaV += (kHeightMap.GetPixel((nXPos + 0), (nYPos + 1)) * nScale *  1.0f * (1.0f / 6.0f));
                    nDeltaV += (kHeightMap.GetPixel((nXPos + 1), (nYPos + 1)) * nScale *  1.0f * (1.0f / 6.0f));

                    float nU = (((nDeltaU.r + nDeltaU.g + nDeltaU.b) / 3.0f) / 255.0f);
                    float nV = (((nDeltaV.r + nDeltaV.g + nDeltaV.b) / 3.0f) / 255.0f);
			        float nMagnitude = ((nU * nU) + (nV * nV) + 1.0f);
			        nMagnitude = sqrt(nMagnitude);

                    float4 nColor = kHeightMap.GetPixel(nXPos, nYPos);
                    nColor.a = ((nColor.r + nColor.g + nColor.b) / 3.0f);
                    nColor.r = ((( -nU / nMagnitude) + 1.0f) * 127.5f);
                    nColor.g = ((( -nV / nMagnitude) + 1.0f) * 127.5f);
                    nColor.b = (((1.0f / nMagnitude) + 1.0f) * 127.5f);
                    kNormalMap.SetPixel(nXPos, nYPos, nColor);
		        }
	        }

            CPathW kPath = strMap;
            kPath.RemoveExtension();
            strFinalMap = (kPath.m_strPath + ".n.png");
            kNormalMap.Save(pTexturesDirectory, strFinalMap);
        }
    }
    else if (strAction.CompareNoCase(L"addnormals") == 0)
    {
        int nPosition = 0;
        CStringW strFirstMap = strData.Tokenize(L",", nPosition);
        CStringW strSecondMap = strData.Tokenize(L",", nPosition);

        CMyImage kFirstImage, kSecondImage;
        if (SUCCEEDED(kFirstImage.Load(pTexturesDirectory, strFirstMap)) &&
           SUCCEEDED(kSecondImage.Load(pTexturesDirectory, strSecondMap)))
        {
            int nXSize = max(kFirstImage.GetXSize(), kSecondImage.GetXSize());
            int nYSize = max(kFirstImage.GetYSize(), kSecondImage.GetYSize());

            kFirstImage.Resize(nXSize, nYSize);
            kSecondImage.Resize(nXSize, nYSize);
	        for(int nYPos = 0; nYPos < nYSize; nYPos++)
	        {
		        for(int nXPos = 0; nXPos < nXSize; nXPos++)
		        {
                    float4 nFirstColor = kFirstImage.GetPixel(nXPos, nYPos);
                    float4 nSecondColor = kSecondImage.GetPixel(nXPos, nYPos);
                    float3 nNormal = ((nFirstColor / 127.5f) - 1.0f);
                    nNormal += ((nSecondColor / 127.5f) - 1.0f);
                    nNormal.Normalize();
                    nNormal = ((nNormal + 1.0f) * 127.5f);

                    float4 nColor(nNormal, ((nFirstColor.a + nSecondColor.a) / 2.0f));
                    kFirstImage.SetPixel(nXPos, nYPos, nColor);
                }
            }

            CPathW kPath = strFirstMap;
            kPath.RemoveExtension();
            strFinalMap = (kPath.m_strPath + ".n.png");
            kFirstImage.Save(pTexturesDirectory, strFinalMap);
        }
    }
    else if (strAction.CompareNoCase(L"smoothnormals") == 0)
    {
        strFinalMap = strData;
    }
    else if (strAction.CompareNoCase(L"add") == 0)
    {
        int nPosition = 0;
        CStringW strFirstMap = strData.Tokenize(L",", nPosition);
        CStringW strSecondMap = strData.Tokenize(L",", nPosition);

        CMyImage kFirstImage, kSecondImage;
        if (SUCCEEDED(kFirstImage.Load(pTexturesDirectory, strFirstMap)) &&
           SUCCEEDED(kSecondImage.Load(pTexturesDirectory, strSecondMap)))
        {
            int nXSize = max(kFirstImage.GetXSize(), kSecondImage.GetXSize());
            int nYSize = max(kFirstImage.GetYSize(), kSecondImage.GetYSize());

            kFirstImage.Resize(nXSize, nYSize);
            kSecondImage.Resize(nXSize, nYSize);
	        for(int nYPos = 0; nYPos < nYSize; nYPos++)
	        {
		        for(int nXPos = 0; nXPos < nXSize; nXPos++)
		        {
                    float4 nFirstColor = kFirstImage.GetPixel(nXPos, nYPos);
                    float4 nSecondColor = kSecondImage.GetPixel(nXPos, nYPos);
                    kFirstImage.SetPixel(nXPos, nYPos, ((nFirstColor + nSecondColor) / 2.0f));
                }
            }

            CPathW kPath = strFirstMap;
            kPath.RemoveExtension();
            strFinalMap = (kPath.m_strPath + L".png");
            kFirstImage.Save(pTexturesDirectory, strFinalMap);
        }
    }
    else if (strAction.CompareNoCase(L"scale") == 0)
    {
        int nPosition = 0;
        CStringW strMap = strData.Tokenize(L",", nPosition);
        float4 nScale = StrToFloat(strData.Tokenize(L",", nPosition));
        if (nPosition >= 0)
        {
            nScale.g = StrToFloat(strData.Tokenize(L",", nPosition));
            nScale.b = StrToFloat(strData.Tokenize(L",", nPosition));
            nScale.a = StrToFloat(strData.Tokenize(L",", nPosition));
        }

        CMyImage kImage;
        if (SUCCEEDED(kImage.Load(pTexturesDirectory, strMap)))
        {
	        for(int nYPos = 0; nYPos < kImage.GetYSize(); nYPos++)
	        {
		        for(int nXPos = 0; nXPos < kImage.GetXSize(); nXPos++)
		        {
                    float4 nColor = kImage.GetPixel(nXPos, nYPos);
                    nColor *= nScale;
                    kImage.SetPixel(nXPos, nYPos, nColor);
                }
            }

            CPathW kPath = strMap;
            kPath.RemoveExtension();
            strFinalMap = (kPath.m_strPath + L".png");
            kImage.Save(pTexturesDirectory, strFinalMap);
        }
    }
    else if (strAction.CompareNoCase(L"invertAlpha") == 0)
    {
        CMyImage kImage;
        CStringW strMap = strData;
        if (SUCCEEDED(kImage.Load(pTexturesDirectory, strMap)))
        {
	        for(int nYPos = 0; nYPos < kImage.GetYSize(); nYPos++)
	        {
		        for(int nXPos = 0; nXPos < kImage.GetXSize(); nXPos++)
		        {
                    float4 nColor = kImage.GetPixel(nXPos, nYPos);
                    nColor.a = (255.0f - nColor.a);
                    kImage.SetPixel(nXPos, nYPos, nColor);
                }
            }

            CPathW kPath = strMap;
            kPath.RemoveExtension();
            strFinalMap = (kPath.m_strPath + L".png");
            kImage.Save(pTexturesDirectory, strFinalMap);
        }
    }
    else if (strAction.CompareNoCase(L"invertColor") == 0)
    {
        CMyImage kImage;
        CStringW strMap = strData;
        if (SUCCEEDED(kImage.Load(pTexturesDirectory, strMap)))
        {
	        for(int nYPos = 0; nYPos < kImage.GetYSize(); nYPos++)
	        {
		        for(int nXPos = 0; nXPos < kImage.GetXSize(); nXPos++)
		        {
                    float4 nColor = kImage.GetPixel(nXPos, nYPos);
                    nColor.r = (255.0f - nColor.r);
                    nColor.g = (255.0f - nColor.g);
                    nColor.b = (255.0f - nColor.b);
                    kImage.SetPixel(nXPos, nYPos, nColor);
                }
            }

            CPathW kPath = strMap;
            kPath.RemoveExtension();
            strFinalMap = (kPath.m_strPath + L".png");
            kImage.Save(pTexturesDirectory, strFinalMap);
        }
    }
    else if (strAction.CompareNoCase(L"makeIntensity") == 0)
    {
        CMyImage kImage;
        CStringW strMap = strData;
        if (SUCCEEDED(kImage.Load(pTexturesDirectory, strMap)))
        {
	        for(int nYPos = 0; nYPos < kImage.GetYSize(); nYPos++)
	        {
		        for(int nXPos = 0; nXPos < kImage.GetXSize(); nXPos++)
		        {
                    float4 nColor = kImage.GetPixel(nXPos, nYPos);
                    nColor.g = nColor.b = nColor.a = nColor.r;
                    kImage.SetPixel(nXPos, nYPos, nColor);
                }
            }

            CPathW kPath = strMap;
            kPath.RemoveExtension();
            strFinalMap = (kPath.m_strPath + L".png");
            kImage.Save(pTexturesDirectory, strFinalMap);
        }
    }
    else if (strAction.CompareNoCase(L"makeAlpha") == 0)
    {
        CMyImage kImage;
        CStringW strMap = strData;
        if (SUCCEEDED(kImage.Load(pTexturesDirectory, strMap)))
        {
	        for(int nYPos = 0; nYPos < kImage.GetYSize(); nYPos++)
	        {
		        for(int nXPos = 0; nXPos < kImage.GetXSize(); nXPos++)
		        {
                    float4 nColor = kImage.GetPixel(nXPos, nYPos);
                    nColor.a = ((nColor.r + nColor.g + nColor.b) / 3.0f);
                    nColor.r = nColor.g = nColor.b = 1.0f;
                    kImage.SetPixel(nXPos, nYPos, nColor);
                }
            }

            CPathW kPath = strMap;
            kPath.RemoveExtension();
            strFinalMap = (kPath.m_strPath + L".png");
            kImage.Save(pTexturesDirectory, strFinalMap);
        }
    }

    if (strFinalMap.IsEmpty())
    {
        printf("---> Unable to load resources\r\n");
        return L"";
    }
    else
    {
        printf("---> Final: %S\r\n", strFinalMap.GetString());
        if (nAction == 0)
        {
            return strFinalMap;
        }
        else
        {
            return GetCombinedMap(pTexturesDirectory, strValue.Mid(0, nAction) + strFinalMap + strValue.Mid(nOutside + 1));
        }
    }
}

int wmain(int nNumArguments, wchar_t *astrArguments[], wchar_t *astrEnvironmentVariables)
{
    printf("Doom3 Material Converter\r\n");

    CStringW strGame;
    CStringW strMaterialsDirectory;
    CStringW strTexturesDirectory;
    for(int nArgument = 1; nArgument < nNumArguments; nArgument++)
    {
        CStringW strArgument = astrArguments[nArgument];
        if (strArgument.CompareNoCase(L"-materials") == 0 && nArgument < (nNumArguments - 1))
        {
            strMaterialsDirectory = astrArguments[++nArgument];
        }
        else if (strArgument.CompareNoCase(L"-textures") == 0 && nArgument < (nNumArguments - 1))
        {
            strTexturesDirectory = astrArguments[++nArgument];
        }
        else if (strArgument.CompareNoCase(L"-game") == 0 && nArgument < (nNumArguments - 1))
        {
            strGame = astrArguments[++nArgument];
        }
    }


    if (nNumArguments < 2)
    {
        printf("Arguments:\r\n");
        printf("-materials <materials directory>\r\n");
        printf("-textures <textures directory>\r\n");
        printf("-game <game>\r\n");
        return -1;
    }
    else
    {
        PathAddBackslashW(strMaterialsDirectory.GetBuffer(MAX_PATH + 1));
        strMaterialsDirectory.ReleaseBuffer();

        PathAddBackslashW(strTexturesDirectory.GetBuffer(MAX_PATH + 1));
        strTexturesDirectory.ReleaseBuffer();

        printf("Materials: %S\r\n", strMaterialsDirectory.GetString());
        printf("Textures: %S\r\n", strTexturesDirectory.GetString());
        printf("Game: %S\r\n", strGame.GetString());
    }

	ilInit();
	iluInit();
	ilOriginFunc(IL_ORIGIN_UPPER_LEFT);
	ilEnable(IL_ORIGIN_SET);
    try
    {
        GEKFindFiles(strMaterialsDirectory, L"*.mtr", true, [&] (LPCWSTR pFileName) -> HRESULT
        {
            printf("File: %S\r\n", pFileName);

            CPathW kFullPath = GetFullFileName(pFileName);
            kFullPath.RemoveFileSpec();

            CGEKParser kParser;
            HRESULT hRetVal = kParser.LoadFromFile(pFileName);
            if (SUCCEEDED(hRetVal))
            {
                while (kParser.NextToken())
                {
                    if (kParser.GetToken().CompareNoCase(L"table") == 0 ||
                       kParser.GetToken().CompareNoCase(L"material") == 0 ||
                       kParser.GetToken().CompareNoCase(L"skin") == 0 ||
                       kParser.GetToken().CompareNoCase(L"particle") == 0)
                    {
                        CStringW strType = kParser.GetToken();

                        kParser.NextToken();
                        CStringW strName = kParser.GetToken();

                        printf("> Skipping %S: %S\r\n", strType.GetString(), strName.GetString());

                        int nDepth = 0;
                        do
                        {
                            kParser.NextToken();
                            if (kParser.GetToken()[0] == '{') nDepth++;
                            if (kParser.GetToken()[0] == '}') nDepth--;
                        } while (nDepth > 0);
                    }
                    else
                    {
                        CStringW strName = kParser.GetToken();

                        printf("> Material: %S\r\n", strName.GetString());

                        kParser.NextToken();
                        if (kParser.GetToken()[0] != '{')
                        {
                            throw CMyException(__LINE__, L"Invalid material block encountered: %s", strName.GetString());
                        }

                        CStringW strDiffuseMap;
                        CStringW strNormalMap;
                        CStringW strSpecularMap;
                        while (kParser.NextToken() && kParser.GetToken()[0] != '}')
                        {
                            if (kParser.GetToken().CompareNoCase(L"diffusemap") == 0)
                            {
                                strDiffuseMap = GetFullMapValue(kParser);
                            }
                            else if (kParser.GetToken().CompareNoCase(L"bumpmap") == 0)
                            {
                                strNormalMap = GetFullMapValue(kParser);
                            }
                            else if (kParser.GetToken().CompareNoCase(L"specularmap") == 0)
                            {
                                strSpecularMap = GetFullMapValue(kParser);
                            }
                            else if (kParser.GetToken()[0] == '{')
                            {
                                CStringW strBlend;
                                CStringW strMap;
                                while (kParser.NextToken() && kParser.GetToken()[0] != '}')
                                {
                                    if (kParser.GetToken().CompareNoCase(L"blend") == 0)
                                    {
                                        kParser.NextToken();
                                        strBlend = kParser.GetToken();
                                    }
                                    else if (kParser.GetToken().CompareNoCase(L"map") == 0)
                                    {
                                        strMap = GetFullMapValue(kParser);
                                    }
                                };

                                if (strBlend.CompareNoCase(L"diffusemap") == 0)
                                {
                                    strDiffuseMap = strMap;
                                }
                                else if (strBlend.CompareNoCase(L"bumpmap") == 0)
                                {
                                    strNormalMap = strMap;
                                }
                                else if (strBlend.CompareNoCase(L"specularmap") == 0)
                                {
                                    strSpecularMap = strMap;
                                }
                            }
                        };

                        if (kParser.GetToken()[0] != '}')
                        {
                            throw CMyException(__LINE__, L"Material block not ended: %s", strName.GetString());
                        }

                        if (strDiffuseMap.IsEmpty())
                        {
                            printf("-> Diffuse Map not found\r\n");
                        }
                        else
                        {
                            strName.Replace(L"/", L"\\");
                            strName = StripRootDirectory(strName);

                            CPathW kName = strName;
                            kName.RemoveExtension();
                            strName = (kName.m_strPath + L".xml");

                            printf("-> Diffuse Map: %S\r\n", strDiffuseMap.GetString());
                            strDiffuseMap = GetCombinedMap(strTexturesDirectory, strDiffuseMap);
                            strDiffuseMap = (strGame + L"/" + StripRootDirectory(strDiffuseMap));
                            printf("                %S\r\n", strDiffuseMap.GetString());

                            if (strNormalMap.IsEmpty())
                            {
                                strNormalMap = L"*normal";
                            }
                            else
                            {
                                printf("-> Normal Map: %S\r\n", strNormalMap.GetString());
                                strNormalMap = GetCombinedMap(strTexturesDirectory, strNormalMap);
                                strNormalMap = (strGame + L"/" + StripRootDirectory(strNormalMap));
                                printf("               %S\r\n", strNormalMap.GetString());
                            }

                            if (strSpecularMap.IsEmpty())
                            {
                                strSpecularMap = L"*info";
                            }
                            else
                            {
                                printf("-> Specular Map: %S\r\n", strSpecularMap.GetString());
                                strSpecularMap = GetCombinedMap(strTexturesDirectory, strSpecularMap);
                                strSpecularMap = (strGame + L"/" + StripRootDirectory(strSpecularMap));
                                printf("                 %S\r\n", strSpecularMap.GetString());
                            }

                            if (strDiffuseMap.IsEmpty() ||
                               strNormalMap.IsEmpty() || 
                               strSpecularMap.IsEmpty())
                            {
                                printf("-> Partial material encountered\r\n");
                            }
                            else
                            {
                                CStringW strMaterial = GetFullFileName(kFullPath.m_strPath + L"\\" + strName);

                                CPathW kPath = strMaterial;
                                kPath.RemoveFileSpec();
                                SHCreateDirectoryEx(nullptr, kPath.m_strPath, nullptr);

                                CStringA strXML;
                                strXML.Format("<?xml version=\"1.0\"?>\r\n" \
                                    "<material pass=\"Opaque\">\r\n" \
                                    " <albedo source=\"%S\" />\r\n" \
                                    " <normal source=\"%S\" />\r\n" \
                                    " <info source=\"%S\" />\r\n" \
                                    "</material>", strDiffuseMap.GetString(), strNormalMap.GetString(), strSpecularMap.GetString());

                                GEKSaveToFile(strMaterial, strXML);
                            }
                        }
                    }
                };
            }

            return hRetVal;
        } );
    }
    catch (CMyException kException)
    {
        printf("! Error (%d): %S", kException.m_nLine, kException.m_strMessage.GetString());
    }
    catch (...)
    {
    }

	ilShutDown();
    return 0;
}