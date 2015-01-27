#include "CGEKRenderFilter.h"
#include "GEKSystem.h"
#include "GEKAPI.h"
#include <algorithm>

#include "GEKSystemCLSIDs.h"
#include "GEKEngineCLSIDs.h"

static GEK3DVIDEO::DATA::FORMAT GetFormat(LPCWSTR pValue)
{
         if (_wcsicmp(pValue, L"R_UINT8") == 0) return GEK3DVIDEO::DATA::R_UINT8;
    else if (_wcsicmp(pValue, L"RG_UINT8") == 0) return GEK3DVIDEO::DATA::RG_UINT8;
    else if (_wcsicmp(pValue, L"RGBA_UINT8") == 0) return GEK3DVIDEO::DATA::RGBA_UINT8;
    else if (_wcsicmp(pValue, L"BGRA_UINT8") == 0) return GEK3DVIDEO::DATA::BGRA_UINT8;
    else if (_wcsicmp(pValue, L"R_UINT16") == 0) return GEK3DVIDEO::DATA::R_UINT16;
    else if (_wcsicmp(pValue, L"RG_UINT16") == 0) return GEK3DVIDEO::DATA::RG_UINT16;
    else if (_wcsicmp(pValue, L"RGBA_UINT16") == 0) return GEK3DVIDEO::DATA::RGBA_UINT16;
    else if (_wcsicmp(pValue, L"R_UINT32") == 0) return GEK3DVIDEO::DATA::R_UINT32;
    else if (_wcsicmp(pValue, L"RG_UINT32") == 0) return GEK3DVIDEO::DATA::RG_UINT32;
    else if (_wcsicmp(pValue, L"RGB_UINT32") == 0) return GEK3DVIDEO::DATA::RGB_UINT32;
    else if (_wcsicmp(pValue, L"RGBA_UINT32") == 0) return GEK3DVIDEO::DATA::RGBA_UINT32;
    else if (_wcsicmp(pValue, L"R_FLOAT") == 0) return GEK3DVIDEO::DATA::R_FLOAT;
    else if (_wcsicmp(pValue, L"RG_FLOAT") == 0) return GEK3DVIDEO::DATA::RG_FLOAT;
    else if (_wcsicmp(pValue, L"RGB_FLOAT") == 0) return GEK3DVIDEO::DATA::RGB_FLOAT;
    else if (_wcsicmp(pValue, L"RGBA_FLOAT") == 0) return GEK3DVIDEO::DATA::RGBA_FLOAT;
    else if (_wcsicmp(pValue, L"R_HALF") == 0) return GEK3DVIDEO::DATA::R_HALF;
    else if (_wcsicmp(pValue, L"RG_HALF") == 0) return GEK3DVIDEO::DATA::RG_HALF;
    else if (_wcsicmp(pValue, L"RGBA_HALF") == 0) return GEK3DVIDEO::DATA::RGBA_HALF;
    else if (_wcsicmp(pValue, L"D16") == 0) return GEK3DVIDEO::DATA::D16;
    else if (_wcsicmp(pValue, L"D32") == 0) return GEK3DVIDEO::DATA::D32;
    else if (_wcsicmp(pValue, L"D24S8") == 0) return GEK3DVIDEO::DATA::D24_S8;
    else return GEK3DVIDEO::DATA::UNKNOWN;
}

static GEK3DVIDEO::DEPTHWRITE::MASK GetDepthWriteMask(LPCWSTR pValue)
{
    if (_wcsicmp(pValue, L"zero") == 0) return GEK3DVIDEO::DEPTHWRITE::ZERO;
    else if (_wcsicmp(pValue, L"all") == 0) return GEK3DVIDEO::DEPTHWRITE::ALL;
    else return GEK3DVIDEO::DEPTHWRITE::ZERO;
}

static GEK3DVIDEO::COMPARISON::FUNCTION GetComparisonFunction(LPCWSTR pValue)
{
    if (_wcsicmp(pValue, L"always") == 0) return GEK3DVIDEO::COMPARISON::ALWAYS;
    else if (_wcsicmp(pValue, L"never") == 0) return GEK3DVIDEO::COMPARISON::NEVER;
    else if (_wcsicmp(pValue, L"equal") == 0) return GEK3DVIDEO::COMPARISON::EQUAL;
    else if (_wcsicmp(pValue, L"notequal") == 0) return GEK3DVIDEO::COMPARISON::NOT_EQUAL;
    else if (_wcsicmp(pValue, L"less") == 0) return GEK3DVIDEO::COMPARISON::LESS;
    else if (_wcsicmp(pValue, L"lessequal") == 0) return GEK3DVIDEO::COMPARISON::LESS_EQUAL;
    else if (_wcsicmp(pValue, L"greater") == 0) return GEK3DVIDEO::COMPARISON::GREATER;
    else if (_wcsicmp(pValue, L"greaterequal") == 0) return GEK3DVIDEO::COMPARISON::GREATER_EQUAL;
    else return GEK3DVIDEO::COMPARISON::ALWAYS;
}

static GEK3DVIDEO::STENCIL::OPERATION GetStencilOperation(LPCWSTR pValue)
{
    if (_wcsicmp(pValue, L"ZERO") == 0) return GEK3DVIDEO::STENCIL::ZERO;
    else if (_wcsicmp(pValue, L"KEEP") == 0) return GEK3DVIDEO::STENCIL::KEEP;
    else if (_wcsicmp(pValue, L"REPLACE") == 0) return GEK3DVIDEO::STENCIL::REPLACE;
    else if (_wcsicmp(pValue, L"INVERT") == 0) return GEK3DVIDEO::STENCIL::INVERT;
    else if (_wcsicmp(pValue, L"INCREASE") == 0) return GEK3DVIDEO::STENCIL::INCREASE;
    else if (_wcsicmp(pValue, L"INCREASE_SATURATED") == 0) return GEK3DVIDEO::STENCIL::INCREASE_SATURATED;
    else if (_wcsicmp(pValue, L"DECREASE") == 0) return GEK3DVIDEO::STENCIL::DECREASE;
    else if (_wcsicmp(pValue, L"DECREASE_SATURATED") == 0) return GEK3DVIDEO::STENCIL::DECREASE_SATURATED;
    else return GEK3DVIDEO::STENCIL::ZERO;
}

static void GetStentilStates(GEK3DVIDEO::STENCILSTATES &kStates, CLibXMLNode &kNode)
{
    if (kNode.HasAttribute(L"pass"))
    {
        kStates.m_eStencilPassOperation = GetStencilOperation(kNode.GetAttribute(L"pass"));
    }
            
    if (kNode.HasAttribute(L"fail"))
    {
        kStates.m_eStencilFailOperation = GetStencilOperation(kNode.GetAttribute(L"fail"));
    }

    if (kNode.HasAttribute(L"depthfail"))
    {
        kStates.m_eStencilDepthFailOperation = GetStencilOperation(kNode.GetAttribute(L"depthfail"));
    }

    if (kNode.HasAttribute(L"comparison"))
    {
        kStates.m_eStencilComparison = GetComparisonFunction(kNode.GetAttribute(L"comparison"));
    }
}

BEGIN_INTERFACE_LIST(CGEKRenderFilter)
    INTERFACE_LIST_ENTRY_COM(IGEKRenderFilter)
END_INTERFACE_LIST_UNKNOWN

REGISTER_CLASS(CGEKRenderFilter)

CGEKRenderFilter::CGEKRenderFilter(void)
    : m_pVideoSystem(nullptr)
    , m_pRenderManager(nullptr)
    , m_nVertexAttributes(0xFFFFFFFF)
    , m_eMode(STANDARD)
    , m_nDispatchXSize(0)
    , m_nDispatchYSize(0)
    , m_nDispatchZSize(0)
    , m_bFlip(false)
    , m_bClearDepth(false)
    , m_bClearStencil(false)
    , m_nClearDepth(0.0f)
    , m_nClearStencil(0)
    , m_nStencilReference(0)
{
}

CGEKRenderFilter::~CGEKRenderFilter(void)
{
}

STDMETHODIMP CGEKRenderFilter::Initialize(void)
{
    HRESULT hRetVal = E_FAIL;
    m_pVideoSystem = GetContext()->GetCachedClass<IGEK3DVideoSystem>(CLSID_GEKVideoSystem);
    m_pRenderManager = GetContext()->GetCachedClass<IGEKRenderSystem>(CLSID_GEKRenderSystem);
    if (m_pVideoSystem != nullptr && m_pRenderManager != nullptr)
    {
        hRetVal = S_OK;
    }

    return hRetVal;
}

CStringW CGEKRenderFilter::ParseValue(LPCWSTR pValue)
{
    CStringW strValue(pValue);
    for (auto &kPair : m_aDefines)
    {
        strValue.Replace(CA2W(kPair.first), CA2W(kPair.second));
    }

    IGEKSystem *pSystem = GetContext()->GetCachedClass<IGEKSystem>(CLSID_GEKSystem);
    if (pSystem != nullptr)
    {
        strValue.Replace(L"%xsize%", FormatString(L"%d", pSystem->GetXSize()));
        strValue.Replace(L"%ysize%", FormatString(L"%d", pSystem->GetYSize()));
    }

    return strValue;
}

HRESULT CGEKRenderFilter::LoadDefines(CLibXMLNode &kFilterNode)
{
    HRESULT hRetVal = S_OK;
    CLibXMLNode kDefinesNode = kFilterNode.FirstChildElement(L"defines");
    if (kDefinesNode)
    {
        CLibXMLNode kDefineNode = kDefinesNode.FirstChildElement(L"define");
        while (kDefineNode)
        {
            if (kDefineNode.HasAttribute(L"name") &&
                kDefineNode.HasAttribute(L"value"))
            {
                CStringA strName = CW2A(kDefineNode.GetAttribute(L"name"), CP_UTF8);
                CStringA strValue = CW2A(kDefineNode.GetAttribute(L"value"), CP_UTF8);

                m_aDefines[strName] = strValue;
                kDefineNode = kDefineNode.NextSiblingElement(L"define");
            }
            else
            {
                hRetVal = E_INVALIDARG;
                break;
            }
        };
    }

    return hRetVal;
}

HRESULT CGEKRenderFilter::LoadDepthStates(CLibXMLNode &kTargetsNode, UINT32 nXSize, UINT32 nYSize)
{
    HRESULT hRetVal = S_OK;
    GEK3DVIDEO::DEPTHSTATES kDepthStates;
    CLibXMLNode kDepthNode = kTargetsNode.FirstChildElement(L"depth");
    if (kDepthNode)
    {
        if (kDepthNode.HasAttribute(L"source"))
        {
            m_strDepthSource = kDepthNode.GetAttribute(L"source");
            hRetVal = S_OK;
        }
        else if (kDepthNode.HasAttribute(L"name"))
        {
            m_strDepthSource = kDepthNode.GetAttribute(L"name");
            GEK3DVIDEO::DATA::FORMAT eFormat = GEK3DVIDEO::DATA::D32;
            if (kDepthNode.HasAttribute(L"format"))
            {
                eFormat = GetFormat(kDepthNode.GetAttribute(L"format"));
            }

            if (eFormat == GEK3DVIDEO::DATA::UNKNOWN)
            {
                hRetVal = E_INVALIDARG;
            }
            else
            {
                hRetVal = m_pRenderManager->CreateBuffer(m_strDepthSource, nXSize, nYSize, eFormat);
            }
        }
        else
        {
            hRetVal = E_INVALIDARG;
        }

        if (SUCCEEDED(hRetVal))
        {
            kDepthStates.m_bDepthEnable = true;
            if (kDepthNode.HasAttribute(L"clear"))
            {
                m_bClearDepth = true;
                m_nClearDepth = StrToFloat(kDepthNode.GetAttribute(L"clear"));
            }

            if (kDepthNode.HasAttribute(L"comparison"))
            {
                kDepthStates.m_eDepthComparison = GetComparisonFunction(kDepthNode.GetAttribute(L"comparison"));
            }

            if (kDepthNode.HasAttribute(L"writemask"))
            {
                kDepthStates.m_eDepthWriteMask = GetDepthWriteMask(kDepthNode.GetAttribute(L"writemask"));
            }
        }

        CLibXMLNode kStencilNode = kDepthNode.FirstChildElement(L"stencil");
        if (kStencilNode)
        {
            kDepthStates.m_bStencilEnable = true;
            if (kStencilNode.HasAttribute(L"clear"))
            {
                m_bClearStencil = true;
                m_nClearStencil = StrToUINT32(kStencilNode.GetAttribute(L"clear"));
            }

            if (kStencilNode.HasAttribute(L"reference"))
            {
                m_nStencilReference = StrToUINT32(kStencilNode.GetAttribute(L"reference"));
            }

            if (kStencilNode.HasAttribute(L"readmask"))
            {
                kDepthStates.m_nStencilReadMask = StrToUINT32(kStencilNode.GetAttribute(L"readmask"));
            }

            if (kStencilNode.HasAttribute(L"writemask"))
            {
                kDepthStates.m_nStencilReadMask = StrToUINT32(kStencilNode.GetAttribute(L"writemask"));
            }

            CLibXMLNode kFrontNode = kStencilNode.FirstChildElement(L"front");
            if (kFrontNode)
            {
                GetStentilStates(kDepthStates.m_kStencilFrontStates, kFrontNode);
            }

            CLibXMLNode kBackNode = kStencilNode.FirstChildElement(L"back");
            if (kBackNode)
            {
                GetStentilStates(kDepthStates.m_kStencilBackStates, kBackNode);
            }
        }
    }

    if (SUCCEEDED(hRetVal))
    {
        hRetVal = m_pVideoSystem->CreateDepthStates(kDepthStates, &m_spDepthStates);
    }

    return hRetVal;
}

HRESULT CGEKRenderFilter::LoadBuffers(CLibXMLNode &kFilterNode)
{
    HRESULT hRetVal = S_OK;
    CLibXMLNode kBuffersNode = kFilterNode.FirstChildElement(L"buffers");
    if (kBuffersNode)
    {
        CLibXMLNode kBufferNode = kBuffersNode.FirstChildElement(L"buffer");
        while (kBufferNode)
        {
            if (kBufferNode.HasAttribute(L"name") &&
                kBufferNode.HasAttribute(L"count"))
            {
                CStringW strName(kBufferNode.GetAttribute(L"name"));
                UINT32 nCount = StrToUINT32(ParseValue(kBufferNode.GetAttribute(L"count")));
                if (kBufferNode.HasAttribute(L"stride"))
                {
                    UINT32 nStride = StrToUINT32(kBufferNode.GetAttribute(L"stride"));
                    hRetVal = m_pRenderManager->CreateBuffer(strName, nStride, nCount);
                    if (FAILED(hRetVal))
                    {
                        break;
                    }
                }
                else if (kBufferNode.HasAttribute(L"format"))
                {
                    GEK3DVIDEO::DATA::FORMAT eFormat = GetFormat(kBufferNode.GetAttribute(L"format"));
                    hRetVal = m_pRenderManager->CreateBuffer(strName, eFormat, nCount);
                    if (FAILED(hRetVal))
                    {
                        break;
                    }
                }
                else
                {
                    hRetVal = E_INVALIDARG;
                    break;
                }
            }
            else
            {
                hRetVal = E_INVALIDARG;
                break;
            }

            kBufferNode = kBufferNode.NextSiblingElement(L"buffer");
        };
    }

    return hRetVal;
}

HRESULT CGEKRenderFilter::LoadTargets(CLibXMLNode &kFilterNode)
{
    HRESULT hRetVal = E_FAIL;
    IGEKSystem *pSystem = GetContext()->GetCachedClass<IGEKSystem>(CLSID_GEKSystem);
    if (pSystem != nullptr)
    {
        hRetVal = S_OK;
        CLibXMLNode kTargetsNode = kFilterNode.FirstChildElement(L"targets");
        if (kTargetsNode)
        {
            UINT32 nXSize = pSystem->GetXSize();
            UINT32 nYSize = pSystem->GetYSize();
            if (SUCCEEDED(hRetVal))
            {
                hRetVal = LoadDepthStates(kTargetsNode, nXSize, nYSize);
            }

            CLibXMLNode kTargetNode = kTargetsNode.FirstChildElement(L"target");
            while (kTargetNode)
            {
                TARGET kData;
                if (kTargetNode.HasAttribute(L"clear"))
                {
                    kData.m_bClear = true;
                    kData.m_nClearColor = StrToFloat4(kTargetNode.GetAttribute(L"clear"));
                }
                else
                {
                    kData.m_bClear = false;
                }

                if (kTargetNode.HasAttribute(L"name"))
                {
                    if (!kTargetNode.HasAttribute(L"format"))
                    {
                        hRetVal = E_INVALIDARG;
                        break;
                    }

                    GEK3DVIDEO::DATA::FORMAT eFormat = GetFormat(kTargetNode.GetAttribute(L"format"));
                    if (eFormat == GEK3DVIDEO::DATA::UNKNOWN)
                    {
                        hRetVal = E_INVALIDARG;
                        break;
                    }

                    hRetVal = m_pRenderManager->CreateBuffer(kTargetNode.GetAttribute(L"name"), nXSize, nYSize, eFormat);
                    if (FAILED(hRetVal))
                    {
                        break;
                    }

                    kData.m_strSource = kTargetNode.GetAttribute(L"name");
                }
                else if (kTargetNode.HasAttribute(L"source"))
                {
                    kData.m_strSource = kTargetNode.GetAttribute(L"source");
                }
                else
                {
                    hRetVal = E_INVALIDARG;
                    break;
                }

                m_aTargets.push_back(kData);
                kTargetNode = kTargetNode.NextSiblingElement(L"target");
            }
        }
    }

    return hRetVal;
}

HRESULT CGEKRenderFilter::LoadResources(DATA &kData, CLibXMLNode &kNode)
{
    HRESULT hRetVal = S_OK;
    CLibXMLNode kResourcesNode = kNode.FirstChildElement(L"resources");
    if (kResourcesNode)
    {
        CLibXMLNode kResourceNode = kResourcesNode.FirstChildElement(L"resource");
        while (kResourceNode)
        {
            if (kResourceNode.HasAttribute(L"stage"))
            {
                UINT32 nStage = StrToUINT32(kResourceNode.GetAttribute(L"stage"));

                RESOURCE kResource;
                if (kResourceNode.HasAttribute(L"source"))
                {
                    kResource.m_strName = kResourceNode.GetAttribute(L"source");
                }
                else if (kResourceNode.HasAttribute(L"data"))
                {
                    CComPtr<IUnknown> spResource;
                    hRetVal = m_pRenderManager->LoadResource(kResourceNode.GetAttribute(L"data"), &spResource);
                    if (SUCCEEDED(hRetVal))
                    {
                        kResource.m_spResource = spResource;
                    }
                    else
                    {
                        break;
                    }
                }
                else
                {
                    hRetVal = E_INVALIDARG;
                    break;
                }
                
                if (kResourceNode.HasAttribute(L"unorderedaccess"))
                {
                    kResource.m_bUnorderedAccess = StrToBoolean(kResourceNode.GetAttribute(L"unorderedaccess"));
                }
                else
                {
                    kResource.m_bUnorderedAccess = false;
                }

                kData.m_aResources[nStage] = kResource;
                kResourceNode = kResourceNode.NextSiblingElement(L"resource");
            }
            else
            {
                hRetVal = E_INVALIDARG;
                break;
            }
        };
    }

    return hRetVal;
}

HRESULT CGEKRenderFilter::LoadComputeProgram(CLibXMLNode &kFilterNode)
{
    HRESULT hRetVal = S_OK;
    CLibXMLNode kComputeNode = kFilterNode.FirstChildElement(L"compute");
    if (kComputeNode)
    {
        if (kComputeNode.HasAttribute(L"dispatch"))
        {
            int nPosition = 0;
            m_strDispatchSize = kComputeNode.GetAttribute(L"dispatch");
            float3 nDispatchSize = StrToFloat3(ParseValue(m_strDispatchSize));
            m_nDispatchXSize = UINT32(nDispatchSize.x);
            m_nDispatchYSize = UINT32(nDispatchSize.y);
            m_nDispatchZSize = UINT32(nDispatchSize.z);

            CStringA strProgram;
            switch (m_eMode)
            {
            case FORWARD:
                hRetVal = GEKLoadFromFile(L"%root%\\data\\programs\\compute\\forward.hlsl", strProgram);
                break;

            case LIGHTING:
                hRetVal = GEKLoadFromFile(L"%root%\\data\\programs\\compute\\lighting.hlsl", strProgram);
                break;

            default:
                hRetVal = GEKLoadFromFile(L"%root%\\data\\programs\\compute\\default.hlsl", strProgram);
                break;
            };

            if (SUCCEEDED(hRetVal))
            {
                if (strProgram.Find("_INSERT_COMPUTE_PROGRAM") < 0)
                {
                    hRetVal = E_INVALIDARG;
                }
                else
                {
                    std::unordered_map<CStringA, CStringA> aDefines(m_aDefines);
                    aDefines["gs_nDispatchXSize"].Format("%d", m_nDispatchXSize);
                    aDefines["gs_nDispatchYSize"].Format("%d", m_nDispatchYSize);
                    aDefines["gs_nDispatchZSize"].Format("%d", m_nDispatchZSize);

                    CStringA strCoreProgram = kComputeNode.GetText();
                    strProgram.Replace("_INSERT_COMPUTE_PROGRAM", strCoreProgram);
                    hRetVal = m_pVideoSystem->CompileComputeProgram(strProgram, "MainComputeProgram", &m_kComputeData.m_spProgram, &aDefines);
                }
            }

            if (SUCCEEDED(hRetVal))
            {
                hRetVal = LoadResources(m_kComputeData, kComputeNode);
            }
        }
        else
        {
            hRetVal = E_INVALIDARG;
        }
    }

    return hRetVal;
}

HRESULT CGEKRenderFilter::LoadPixelProgram(CLibXMLNode &kFilterNode)
{
    HRESULT hRetVal = E_INVALIDARG;
    CLibXMLNode kPixelNode = kFilterNode.FirstChildElement(L"pixel");
    if (kPixelNode)
    {
        CStringA strProgram;
        switch (m_eMode)
        {
        case FORWARD:
            hRetVal = GEKLoadFromFile(L"%root%\\data\\programs\\pixel\\forward.hlsl", strProgram);
            break;

        case LIGHTING:
            hRetVal = GEKLoadFromFile(L"%root%\\data\\programs\\pixel\\lighting.hlsl", strProgram);
            break;

        default:
            hRetVal = GEKLoadFromFile(L"%root%\\data\\programs\\pixel\\default.hlsl", strProgram);
            break;
        };

        if (SUCCEEDED(hRetVal))
        {
            if (strProgram.Find("_INSERT_PIXEL_PROGRAM") < 0)
            {
                hRetVal = E_INVALIDARG;
            }
            else
            {
                std::unordered_map<CStringA, CStringA> aDefines(m_aDefines);
                aDefines["gs_nDispatchXSize"].Format("%d", m_nDispatchXSize);
                aDefines["gs_nDispatchYSize"].Format("%d", m_nDispatchYSize);
                aDefines["gs_nDispatchZSize"].Format("%d", m_nDispatchZSize);

                CStringA strCoreProgram = kPixelNode.GetText();
                strProgram.Replace("_INSERT_PIXEL_PROGRAM", strCoreProgram);
                hRetVal = m_pVideoSystem->CompilePixelProgram(strProgram, "MainPixelProgram", &m_kPixelData.m_spProgram, &aDefines);
            }
        }

        if (SUCCEEDED(hRetVal))
        {
            hRetVal = LoadResources(m_kPixelData, kPixelNode);
        }
    }

    return hRetVal;
}

STDMETHODIMP CGEKRenderFilter::Load(LPCWSTR pFileName, const std::unordered_map<CStringA, CStringA> &aDefines)
{
    m_aDefines = aDefines;

    CLibXMLDoc kDocument;
    HRESULT hRetVal = kDocument.Load(pFileName);
    if (SUCCEEDED(hRetVal))
    {
        CLibXMLNode kFilterNode = kDocument.GetRoot();
        if (kFilterNode)
        {
            if (kFilterNode.HasAttribute(L"flip"))
            {
                m_bFlip = StrToBoolean(kFilterNode.GetAttribute(L"flip"));
            }

            CStringW strVertexAttributes(kFilterNode.GetAttribute(L"vertexattributes"));
            if (strVertexAttributes.IsEmpty())
            {
                m_nVertexAttributes = 0xFFFFFFFF;
            }
            else
            {
                m_nVertexAttributes = 0;

                int nPosition = 0;
                CStringW strAttribute(strVertexAttributes.Tokenize(L",", nPosition));
                while (!strAttribute.IsEmpty())
                {
                    if (strAttribute.CompareNoCase(L"position") == 0)
                    {
                        m_nVertexAttributes |= GEK_VERTEX_POSITION;
                    }
                    else if (strAttribute.CompareNoCase(L"texcoord") == 0)
                    {
                        m_nVertexAttributes |= GEK_VERTEX_TEXCOORD;
                    }
                    else if (strAttribute.CompareNoCase(L"normal") == 0)
                    {
                        m_nVertexAttributes |= GEK_VERTEX_NORMAL;
                    }

                    strAttribute = strVertexAttributes.Tokenize(L",", nPosition);
                };
            }

            CStringW strMode(kFilterNode.GetAttribute(L"mode"));
            if (strMode.CompareNoCase(L"forward") == 0)
            {
                m_eMode = FORWARD;
            }
            else if (strMode.CompareNoCase(L"lighting") == 0)
            {
                m_eMode = LIGHTING;
            }
            else
            {
                m_eMode = STANDARD;
            }

            if (SUCCEEDED(hRetVal))
            {
                hRetVal = m_kRenderStates.Load(m_pVideoSystem, kFilterNode.FirstChildElement(L"states"));
            }

            if (SUCCEEDED(hRetVal))
            {
                hRetVal = m_kBlendStates.Load(m_pVideoSystem, kFilterNode.FirstChildElement(L"blend"));
            }

            if (SUCCEEDED(hRetVal))
            {
                hRetVal = LoadDefines(kFilterNode);
            }

            if (SUCCEEDED(hRetVal))
            {
                hRetVal = LoadBuffers(kFilterNode);
            }

            if (SUCCEEDED(hRetVal))
            {
                hRetVal = LoadTargets(kFilterNode);
            }

            if (SUCCEEDED(hRetVal))
            {
                hRetVal = LoadComputeProgram(kFilterNode);
            }

            if (SUCCEEDED(hRetVal))
            {
                hRetVal = LoadPixelProgram(kFilterNode);
            }

            if (SUCCEEDED(hRetVal) && !m_spDepthStates)
            {
                GEK3DVIDEO::DEPTHSTATES kDepthStates;
                hRetVal = m_pVideoSystem->CreateDepthStates(kDepthStates, &m_spDepthStates);
            }
        }
    }

    return hRetVal;
}

STDMETHODIMP_(void) CGEKRenderFilter::Draw(IGEK3DVideoContext *pContext)
{
    REQUIRE_VOID_RETURN(pContext);

    if (m_bFlip)
    {
        m_pRenderManager->FlipCurrentBuffers();
    }

    CComPtr<IUnknown> spDepthBuffer;
    if (!m_strDepthSource.IsEmpty())
    {
        m_pRenderManager->GetBuffer(m_strDepthSource, &spDepthBuffer);
    }

    if (spDepthBuffer)
    {
        DWORD nFlags = 0;
        if (m_bClearDepth)
        {
            nFlags |= GEK3DVIDEO::CLEAR::DEPTH;
        }

        if (m_bClearStencil)
        {
            nFlags |= GEK3DVIDEO::CLEAR::STENCIL;
        }

        if (nFlags > 0)
        {
            pContext->ClearDepthStencilTarget(spDepthBuffer, nFlags, m_nClearDepth, m_nClearStencil);
        }
    }

    if (m_aTargets.size() > 0)
    {
        std::vector<IGEK3DVideoTexture *> aTargets;
        std::vector<GEK3DVIDEO::VIEWPORT> aViewPorts;
        for (auto &kTarget : m_aTargets)
        {
            CComPtr<IUnknown> spUnknown;
            m_pRenderManager->GetBuffer(kTarget.m_strSource, &spUnknown);
            if (spUnknown)
            {
                CComQIPtr<IGEK3DVideoTexture> spTarget = spUnknown;
                if (spTarget)
                {
                    if (kTarget.m_bClear)
                    {
                        pContext->ClearRenderTarget(spTarget, kTarget.m_nClearColor);
                    }

                    aTargets.push_back(spTarget);

                    GEK3DVIDEO::VIEWPORT kViewPort;
                    kViewPort.m_nTopLeftX = 0.0f;
                    kViewPort.m_nTopLeftY = 0.0f;
                    kViewPort.m_nXSize = float(spTarget->GetXSize());
                    kViewPort.m_nYSize = float(spTarget->GetYSize());
                    kViewPort.m_nMinDepth = 0.0f;
                    kViewPort.m_nMaxDepth = 1.0f;
                    aViewPorts.push_back(kViewPort);
                }
            }
        }

        pContext->SetRenderTargets(aTargets, (spDepthBuffer ? spDepthBuffer : nullptr));
        pContext->SetViewports(aViewPorts);
    }
    else
    {
        m_pRenderManager->SetScreenTargets(pContext, spDepthBuffer ? spDepthBuffer : nullptr);
    }

    std::unordered_map<UINT32, IUnknown *> aComputeResources;
    std::unordered_map<UINT32, IUnknown *> aComputeUnorderedAccess;
    for (auto &kPair : m_kComputeData.m_aResources)
    {
        if (kPair.second.m_spResource)
        {
            if (kPair.second.m_bUnorderedAccess)
            {
                aComputeUnorderedAccess[kPair.first] = kPair.second.m_spResource;
            }
            else
            {
                aComputeResources[kPair.first] = kPair.second.m_spResource;
            }
        }
        else if (!kPair.second.m_strName.IsEmpty())
        {
            CComPtr<IUnknown> spResource;
            m_pRenderManager->GetBuffer(kPair.second.m_strName, &spResource);
            if (spResource)
            {
                if (kPair.second.m_bUnorderedAccess)
                {
                    aComputeUnorderedAccess[kPair.first] = spResource;
                }
                else
                {
                    aComputeResources[kPair.first] = spResource;
                }
            }
        }
    }

    std::unordered_map<UINT32, IUnknown *> aPixelResources;
    for (auto &kPair : m_kPixelData.m_aResources)
    {
        if (kPair.second.m_spResource)
        {
            aPixelResources[kPair.first] = kPair.second.m_spResource;
        }
        else if (!kPair.second.m_strName.IsEmpty())
        {
            CComPtr<IUnknown> spResource;
            m_pRenderManager->GetBuffer(kPair.second.m_strName, &spResource);
            if (spResource)
            {
                aPixelResources[kPair.first] = spResource;
            }
        }
    }

    m_kRenderStates.SetDefault(m_pRenderManager);
    m_kBlendStates.SetDefault(m_pRenderManager);

    m_kRenderStates.Enable(pContext);
    m_kBlendStates.Enable(pContext);
    pContext->SetDepthStates(m_nStencilReference, m_spDepthStates);
    pContext->GetPixelSystem()->SetProgram(m_kPixelData.m_spProgram);

    if (m_eMode == FORWARD)
    {
        for (auto &kPair : aPixelResources)
        {
            m_pRenderManager->SetResource(pContext->GetPixelSystem(), kPair.first, kPair.second);
        }

        m_pRenderManager->DrawScene(pContext, m_nVertexAttributes);
    }
    else if (m_eMode == LIGHTING)
    {
        if (m_nDispatchXSize > 0)
        {
            pContext->GetComputeSystem()->SetProgram(m_kComputeData.m_spProgram);
        }

        m_pRenderManager->DrawLights(pContext, [&](void) -> void
        {
            if (m_nDispatchXSize > 0)
            {
                for (auto &kPair : aPixelResources)
                {
                    pContext->GetPixelSystem()->SetResource(kPair.first, nullptr);
                }

                for (auto &kPair : aComputeResources)
                {
                    m_pRenderManager->SetResource(pContext->GetComputeSystem(), kPair.first, kPair.second);
                }

                for (auto &kPair : aComputeUnorderedAccess)
                {
                    pContext->GetComputeSystem()->SetUnorderedAccess(kPair.first, kPair.second);
                }

                pContext->Dispatch(m_nDispatchXSize, m_nDispatchYSize, m_nDispatchZSize);

                for (auto &kPair : aComputeResources)
                {
                    pContext->GetComputeSystem()->SetResource(kPair.first, nullptr);
                }

                for (auto &kPair : aComputeUnorderedAccess)
                {
                    pContext->GetComputeSystem()->SetUnorderedAccess(kPair.first, nullptr);
                }
            }

            for (auto &kPair : aPixelResources)
            {
                m_pRenderManager->SetResource(pContext->GetPixelSystem(), kPair.first, kPair.second);
            }
        });
    }
    else
    {
        for (auto &kPair : aPixelResources)
        {
            m_pRenderManager->SetResource(pContext->GetPixelSystem(), kPair.first, kPair.second);
        }

        m_pRenderManager->DrawOverlay(pContext);
    }

    pContext->ClearResources();
}