#include "CGEKRenderFilter.h"
#include "GEKSystem.h"
#include "GEKAPI.h"
#include <algorithm>

#include "GEKSystemCLSIDs.h"

static GEKVIDEO::DATA::FORMAT GetFormat(LPCWSTR pValue)
{
         if (_wcsicmp(pValue, L"R_UINT8") == 0) return GEKVIDEO::DATA::R_UINT8;
    else if (_wcsicmp(pValue, L"RG_UINT8") == 0) return GEKVIDEO::DATA::RG_UINT8;
    else if (_wcsicmp(pValue, L"RGBA_UINT8") == 0) return GEKVIDEO::DATA::RGBA_UINT8;
    else if (_wcsicmp(pValue, L"BGRA_UINT8") == 0) return GEKVIDEO::DATA::BGRA_UINT8;
    else if (_wcsicmp(pValue, L"R_UINT16") == 0) return GEKVIDEO::DATA::R_UINT16;
    else if (_wcsicmp(pValue, L"RG_UINT16") == 0) return GEKVIDEO::DATA::RG_UINT16;
    else if (_wcsicmp(pValue, L"RGBA_UINT16") == 0) return GEKVIDEO::DATA::RGBA_UINT16;
    else if (_wcsicmp(pValue, L"R_UINT32") == 0) return GEKVIDEO::DATA::R_UINT32;
    else if (_wcsicmp(pValue, L"RG_UINT32") == 0) return GEKVIDEO::DATA::RG_UINT32;
    else if (_wcsicmp(pValue, L"RGB_UINT32") == 0) return GEKVIDEO::DATA::RGB_UINT32;
    else if (_wcsicmp(pValue, L"RGBA_UINT32") == 0) return GEKVIDEO::DATA::RGBA_UINT32;
    else if (_wcsicmp(pValue, L"R_FLOAT") == 0) return GEKVIDEO::DATA::R_FLOAT;
    else if (_wcsicmp(pValue, L"RG_FLOAT") == 0) return GEKVIDEO::DATA::RG_FLOAT;
    else if (_wcsicmp(pValue, L"RGB_FLOAT") == 0) return GEKVIDEO::DATA::RGB_FLOAT;
    else if (_wcsicmp(pValue, L"RGBA_FLOAT") == 0) return GEKVIDEO::DATA::RGBA_FLOAT;
    else if (_wcsicmp(pValue, L"D16") == 0) return GEKVIDEO::DATA::D16;
    else if (_wcsicmp(pValue, L"D32") == 0) return GEKVIDEO::DATA::D32;
    else if (_wcsicmp(pValue, L"D24S8") == 0) return GEKVIDEO::DATA::D24_S8;
    else return GEKVIDEO::DATA::UNKNOWN;
}

static GEKVIDEO::DEPTHWRITE::MASK GetDepthWriteMask(LPCWSTR pValue)
{
    if (_wcsicmp(pValue, L"zero") == 0) return GEKVIDEO::DEPTHWRITE::ZERO;
    else if (_wcsicmp(pValue, L"all") == 0) return GEKVIDEO::DEPTHWRITE::ALL;
    else return GEKVIDEO::DEPTHWRITE::ZERO;
}

static GEKVIDEO::COMPARISON::FUNCTION GetComparisonFunction(LPCWSTR pValue)
{
    if (_wcsicmp(pValue, L"always") == 0) return GEKVIDEO::COMPARISON::ALWAYS;
    else if (_wcsicmp(pValue, L"never") == 0) return GEKVIDEO::COMPARISON::NEVER;
    else if (_wcsicmp(pValue, L"equal") == 0) return GEKVIDEO::COMPARISON::EQUAL;
    else if (_wcsicmp(pValue, L"notequal") == 0) return GEKVIDEO::COMPARISON::NOT_EQUAL;
    else if (_wcsicmp(pValue, L"less") == 0) return GEKVIDEO::COMPARISON::LESS;
    else if (_wcsicmp(pValue, L"lessequal") == 0) return GEKVIDEO::COMPARISON::LESS_EQUAL;
    else if (_wcsicmp(pValue, L"greater") == 0) return GEKVIDEO::COMPARISON::GREATER;
    else if (_wcsicmp(pValue, L"greaterequal") == 0) return GEKVIDEO::COMPARISON::GREATER_EQUAL;
    else return GEKVIDEO::COMPARISON::ALWAYS;
}

static GEKVIDEO::STENCIL::OPERATION GetStencilOperation(LPCWSTR pValue)
{
    if (_wcsicmp(pValue, L"ZERO") == 0) return GEKVIDEO::STENCIL::ZERO;
    else if (_wcsicmp(pValue, L"KEEP") == 0) return GEKVIDEO::STENCIL::KEEP;
    else if (_wcsicmp(pValue, L"REPLACE") == 0) return GEKVIDEO::STENCIL::REPLACE;
    else if (_wcsicmp(pValue, L"INVERT") == 0) return GEKVIDEO::STENCIL::INVERT;
    else if (_wcsicmp(pValue, L"INCREASE") == 0) return GEKVIDEO::STENCIL::INCREASE;
    else if (_wcsicmp(pValue, L"INCREASE_SATURATED") == 0) return GEKVIDEO::STENCIL::INCREASE_SATURATED;
    else if (_wcsicmp(pValue, L"DECREASE") == 0) return GEKVIDEO::STENCIL::DECREASE;
    else if (_wcsicmp(pValue, L"DECREASE_SATURATED") == 0) return GEKVIDEO::STENCIL::DECREASE_SATURATED;
    else return GEKVIDEO::STENCIL::ZERO;
}

static void GetStentilStates(GEKVIDEO::STENCILSTATES &kStates, CLibXMLNode &kNode)
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
    INTERFACE_LIST_ENTRY_COM(IGEKSystemUser)
    INTERFACE_LIST_ENTRY_COM(IGEKVideoSystemUser)
    INTERFACE_LIST_ENTRY_COM(IGEKVideoObserver)
    INTERFACE_LIST_ENTRY_COM(IGEKRenderManagerUser)
    INTERFACE_LIST_ENTRY_COM(IGEKRenderFilter)
END_INTERFACE_LIST_UNKNOWN

REGISTER_CLASS(CGEKRenderFilter)

CGEKRenderFilter::CGEKRenderFilter(void)
    : m_nScale(1.0f)
    , m_eDepthFormat(GEKVIDEO::DATA::UNKNOWN)
    , m_nVertexAttributes(0xFFFFFFFF)
    , m_eMode(STANDARD)
    , m_nDispatchXSize(0)
    , m_nDispatchYSize(0)
    , m_nDispatchZSize(0)
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
    return CGEKObservable::AddObserver(GetVideoSystem(), this);
}

STDMETHODIMP_(void) CGEKRenderFilter::Destroy(void)
{
    CGEKObservable::RemoveObserver(GetVideoSystem(), this);
}

STDMETHODIMP_(void) CGEKRenderFilter::OnPreReset(void)
{
    for (auto &kTarget : m_aTargets)
    {
        kTarget.m_spResource = nullptr;
    }

    m_spDepthBuffer = nullptr;
}

STDMETHODIMP CGEKRenderFilter::OnPostReset(void)
{
    HRESULT hRetVal = S_OK;
    UINT32 nXSize = UINT32(float(GetSystem()->GetXSize()) * m_nScale);
    UINT32 nYSize = UINT32(float(GetSystem()->GetYSize()) * m_nScale);
    for (auto &kTarget : m_aTargets)
    {
        if (kTarget.m_eFormat != GEKVIDEO::DATA::UNKNOWN)
        {
            hRetVal = GetVideoSystem()->CreateRenderTarget(nXSize, nYSize, kTarget.m_eFormat, &kTarget.m_spResource);
            if (FAILED(hRetVal))
            {
                break;
            }
        }
    }

    if (SUCCEEDED(hRetVal) && m_eDepthFormat != GEKVIDEO::DATA::UNKNOWN)
    {
        hRetVal = GetVideoSystem()->CreateDepthTarget(nXSize, nYSize, m_eDepthFormat, &m_spDepthBuffer);
    }

    return hRetVal;
}

UINT32 CGEKRenderFilter::EvaluateValue(LPCWSTR pValue)
{
    CStringW strValue(pValue);
    for (auto &kPair : m_aDefines)
    {
        strValue.Replace(CA2W(kPair.first), CA2W(kPair.second));
    }

    return GetSystem()->EvaluateValue(strValue);
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
    GEKVIDEO::DEPTHSTATES kDepthStates;
    CLibXMLNode kDepthNode = kTargetsNode.FirstChildElement(L"depth");
    if (kDepthNode)
    {
        if (kDepthNode.HasAttribute(L"source"))
        {
            m_strDepthSource = kDepthNode.GetAttribute(L"source");
            hRetVal = S_OK;
        }
        else
        {
            GEKVIDEO::DATA::FORMAT eFormat = GEKVIDEO::DATA::D32;
            if (kDepthNode.HasAttribute(L"format"))
            {
                eFormat = GetFormat(kDepthNode.GetAttribute(L"format"));
            }

            if (eFormat == GEKVIDEO::DATA::UNKNOWN)
            {
                hRetVal = E_INVALIDARG;
            }
            else
            {
                m_eDepthFormat = eFormat;
                hRetVal = GetVideoSystem()->CreateDepthTarget(nXSize, nYSize, eFormat, &m_spDepthBuffer);
            }
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
        hRetVal = GetVideoSystem()->CreateDepthStates(kDepthStates, &m_spDepthStates);
    }

    return hRetVal;
}

HRESULT CGEKRenderFilter::LoadRenderStates(CLibXMLNode &kFilterNode)
{
    return CGEKRenderStates::Load(GetVideoSystem(), kFilterNode.FirstChildElement(L"render"));
}

HRESULT CGEKRenderFilter::LoadBlendStates(CLibXMLNode &kFilterNode)
{
    return CGEKBlendStates::Load(GetVideoSystem(), kFilterNode.FirstChildElement(L"blend"));
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
                CStringW strName = kBufferNode.GetAttribute(L"name");
                UINT32 nCount = EvaluateValue(kBufferNode.GetAttribute(L"count"));
                if (kBufferNode.HasAttribute(L"stride"))
                {
                    UINT32 nStride = StrToUINT32(kBufferNode.GetAttribute(L"stride"));

                    CComPtr<IGEKVideoBuffer> spBuffer;
                    hRetVal = GetVideoSystem()->CreateBuffer(nStride, nCount, GEKVIDEO::BUFFER::STRUCTURED_BUFFER | GEKVIDEO::BUFFER::RESOURCE, &spBuffer);
                    if (spBuffer)
                    {
                        m_aBufferMap[strName] = spBuffer;
                    }
                    else
                    {
                        break;
                    }
                }
                else if (kBufferNode.HasAttribute(L"format"))
                {
                    GEKVIDEO::DATA::FORMAT eFormat = GetFormat(kBufferNode.GetAttribute(L"format"));

                    CComPtr<IGEKVideoBuffer> spBuffer;
                    hRetVal = GetVideoSystem()->CreateBuffer(eFormat, nCount, GEKVIDEO::BUFFER::UNORDERED_ACCESS | GEKVIDEO::BUFFER::RESOURCE, &spBuffer);
                    if (spBuffer)
                    {
                        m_aBufferMap[strName] = spBuffer;
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
    HRESULT hRetVal = S_OK;
    CLibXMLNode kTargetsNode = kFilterNode.FirstChildElement(L"targets");
    if (kTargetsNode)
    {
        m_nScale = 1.0f;
        if (kTargetsNode.HasAttribute(L"scale"))
        {
            m_nScale = StrToFloat(kTargetsNode.GetAttribute(L"scale"));
        }

        UINT32 nXSize = UINT32(float(GetSystem()->GetXSize()) * m_nScale);
        UINT32 nYSize = UINT32(float(GetSystem()->GetYSize()) * m_nScale);
        if (SUCCEEDED(hRetVal))
        {
            hRetVal = LoadDepthStates(kTargetsNode, nXSize, nYSize);
        }

        CLibXMLNode kTargetNode = kTargetsNode.FirstChildElement(L"target");
        while (kTargetNode)
        {
            GEKVIDEO::DATA::FORMAT eFormat = GetFormat(kTargetNode.GetAttribute(L"format"));

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
                if (eFormat == GEKVIDEO::DATA::UNKNOWN)
                {
                    hRetVal = E_INVALIDARG;
                    break;
                }

                CComPtr<IGEKVideoTexture> spResource;
                hRetVal = GetVideoSystem()->CreateRenderTarget(nXSize, nYSize, eFormat, &spResource);
                if (spResource != nullptr)
                {
                    kData.m_eFormat = eFormat;
                    kData.m_spResource = spResource;
                    m_aTargets.push_back(kData);
                    m_aTargetMap[kTargetNode.GetAttribute(L"name")] = &m_aTargets.back();
                }
                else
                {
                    break;
                }
            }
            else if (kTargetNode.HasAttribute(L"source"))
            {
                kData.m_strSource = kTargetNode.GetAttribute(L"source");
                m_aTargets.push_back(kData);
            }
            else
            {
                hRetVal = E_INVALIDARG;
                break;
            }

            kTargetNode = kTargetNode.NextSiblingElement(L"target");
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
                    hRetVal = GetRenderManager()->LoadResource(kResourceNode.GetAttribute(L"data"), &spResource);
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
            CStringW strDispatch = kComputeNode.GetAttribute(L"dispatch");
            m_nDispatchXSize = EvaluateValue(strDispatch.Tokenize(L",", nPosition));
            m_nDispatchYSize = EvaluateValue(strDispatch.Tokenize(L",", nPosition));
            m_nDispatchZSize = EvaluateValue(strDispatch.Tokenize(L",", nPosition));

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
                    CStringA strCoreProgram = kComputeNode.GetText();
                    strProgram.Replace("_INSERT_COMPUTE_PROGRAM", strCoreProgram);
                    hRetVal = GetVideoSystem()->CompileComputeProgram(strProgram, "MainComputeProgram", &m_kComputeData.m_spProgram, &m_aDefines);
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
                CStringA strCoreProgram = kPixelNode.GetText();
                strProgram.Replace("_INSERT_PIXEL_PROGRAM", strCoreProgram);
                hRetVal = GetVideoSystem()->CompilePixelProgram(strProgram, "MainPixelProgram", &m_kPixelData.m_spProgram, &m_aDefines);
            }
        }

        if (SUCCEEDED(hRetVal))
        {
            hRetVal = LoadResources(m_kPixelData, kPixelNode);
        }
    }

    return hRetVal;
}

STDMETHODIMP CGEKRenderFilter::Load(LPCWSTR pFileName)
{
    CLibXMLDoc kDocument;
    HRESULT hRetVal = kDocument.Load(pFileName);
    if (SUCCEEDED(hRetVal))
    {
        CLibXMLNode kFilterNode = kDocument.GetRoot();
        if (kFilterNode)
        {
            CStringW strVertexAttributes = kFilterNode.GetAttribute(L"vertexattributes");
            if (strVertexAttributes.IsEmpty())
            {
                m_nVertexAttributes = 0xFFFFFFFF;
            }
            else
            {
                m_nVertexAttributes = 0;

                int nPosition = 0;
                CStringW strAttribute = strVertexAttributes.Tokenize(L",", nPosition);
                while (!strAttribute.IsEmpty())
                {
                    if (strAttribute.CompareNoCase(L"position") == 0)
                    {
                        m_nVertexAttributes |= GEK_VERTEX_POSITION;
                    }
                    else if (strAttribute.CompareNoCase(L"position") == 0)
                    {
                        m_nVertexAttributes |= GEK_VERTEX_TEXCOORD;
                    }
                    else if (strAttribute.CompareNoCase(L"position") == 0)
                    {
                        m_nVertexAttributes |= GEK_VERTEX_BASIS;
                    }

                    strAttribute = strVertexAttributes.Tokenize(L",", nPosition);
                };
            }

            CStringW strMode = kFilterNode.GetAttribute(L"mode");
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
                hRetVal = LoadDefines(kFilterNode);
            }

            if (SUCCEEDED(hRetVal))
            {
                hRetVal = LoadBlendStates(kFilterNode);
            }

            if (SUCCEEDED(hRetVal))
            {
                hRetVal = LoadRenderStates(kFilterNode);
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
                GEKVIDEO::DEPTHSTATES kDepthStates;
                hRetVal = GetVideoSystem()->CreateDepthStates(kDepthStates, &m_spDepthStates);
            }
        }
    }

    return hRetVal;
}

STDMETHODIMP_(UINT32) CGEKRenderFilter::GetVertexAttributes(void)
{
    return m_nVertexAttributes;
}

STDMETHODIMP CGEKRenderFilter::GetBuffer(LPCWSTR pName, IUnknown **ppTexture)
{
    REQUIRE_RETURN(ppTexture, E_INVALIDARG);

    HRESULT hRetVal = E_FAIL;
    auto pTargetIterator = m_aTargetMap.find(pName);
    if (pTargetIterator != m_aTargetMap.end())
    {
        hRetVal = ((*pTargetIterator).second)->m_spResource->QueryInterface(IID_PPV_ARGS(ppTexture));
    }

    if (FAILED(hRetVal))
    {
        auto pBufferIterator = m_aBufferMap.find(pName);
        if (pBufferIterator != m_aBufferMap.end())
        {
            hRetVal = ((*pBufferIterator).second)->QueryInterface(IID_PPV_ARGS(ppTexture));
        }
    }

    return hRetVal;
}

STDMETHODIMP CGEKRenderFilter::GetDepthBuffer(IUnknown **ppBuffer)
{
    HRESULT hRetVal = E_FAIL;
    if (m_spDepthBuffer != nullptr)
    {
        hRetVal = m_spDepthBuffer->QueryInterface(IID_PPV_ARGS(ppBuffer));
    }

    return hRetVal;
}

STDMETHODIMP_(void) CGEKRenderFilter::Draw(void)
{
    CComPtr<IUnknown> spDepthBuffer;
    if (m_spDepthBuffer != nullptr)
    {
        spDepthBuffer = m_spDepthBuffer;
    }
    else if (!m_strDepthSource.IsEmpty())
    {
        GetRenderManager()->GetDepthBuffer(m_strDepthSource, &spDepthBuffer);
    }

    if (spDepthBuffer != nullptr)
    {
        DWORD nFlags = 0;
        if (m_bClearDepth)
        {
            nFlags |= GEKVIDEO::CLEAR::DEPTH;
        }

        if (m_bClearStencil)
        {
            nFlags |= GEKVIDEO::CLEAR::STENCIL;
        }

        if (nFlags > 0)
        {
            GetVideoSystem()->GetImmediateContext()->ClearDepthStencilTarget(spDepthBuffer, nFlags, m_nClearDepth, m_nClearStencil);
        }
    }

    if (m_aTargets.size() > 0)
    {
        std::vector<IGEKVideoTexture *> aTargets;
        for (auto &kTarget : m_aTargets)
        {
            CComQIPtr<IGEKVideoTexture> spTarget;
            if (kTarget.m_spResource != nullptr)
            {
                spTarget = kTarget.m_spResource;
            }
            else if (!kTarget.m_strSource.IsEmpty())
            {
                CComPtr<IUnknown> spUnknown;
                GetRenderManager()->GetBuffer(kTarget.m_strSource, &spUnknown);
                if (spUnknown)
                {
                    spTarget = spUnknown;
                }
            }

            if (spTarget != nullptr)
            {
                if (kTarget.m_bClear)
                {
                    GetVideoSystem()->GetImmediateContext()->ClearRenderTarget(spTarget, kTarget.m_nClearColor);
                }

                aTargets.push_back(spTarget);
            }
        }

        GetVideoSystem()->GetImmediateContext()->SetRenderTargets(aTargets, (spDepthBuffer ? spDepthBuffer : nullptr));
    }
    else
    {
        GetVideoSystem()->SetDefaultTargets(nullptr, (spDepthBuffer ? spDepthBuffer : nullptr));
    }

    std::map<IUnknown *, UINT32> aComputeResources;
    std::map<IUnknown *, UINT32> aComputeUnorderedAccess;
    for (auto &kPair : m_kComputeData.m_aResources)
    {
        if (kPair.second.m_spResource != nullptr)
        {
            if (kPair.second.m_bUnorderedAccess)
            {
                aComputeUnorderedAccess[kPair.second.m_spResource] = kPair.first;
            }
            else
            {
                aComputeResources[kPair.second.m_spResource] = kPair.first;
            }
        }
        else if (!kPair.second.m_strName.IsEmpty())
        {
            CComPtr<IUnknown> spResource;
            GetRenderManager()->GetBuffer(kPair.second.m_strName, &spResource);
            if (spResource != nullptr)
            {
                if (kPair.second.m_bUnorderedAccess)
                {
                    aComputeUnorderedAccess[spResource] = kPair.first;
                }
                else
                {
                    aComputeResources[spResource] = kPair.first;
                }
            }
        }
    }

    std::map<IUnknown *, UINT32> aPixelResources;
    for (auto &kPair : m_kPixelData.m_aResources)
    {
        if (kPair.second.m_spResource != nullptr)
        {
            aPixelResources[kPair.second.m_spResource] = kPair.first;
        }
        else if (!kPair.second.m_strName.IsEmpty())
        {
            CComPtr<IUnknown> spResource;
            GetRenderManager()->GetBuffer(kPair.second.m_strName, &spResource);
            if (spResource != nullptr)
            {
                aPixelResources[spResource] = kPair.first;
            }
        }
    }

    CGEKRenderStates::Enable(GetVideoSystem());
    CGEKBlendStates::Enable(GetVideoSystem());
    GetVideoSystem()->GetImmediateContext()->SetDepthStates(m_nStencilReference, m_spDepthStates);
    GetVideoSystem()->GetImmediateContext()->GetComputeSystem()->SetProgram(m_kComputeData.m_spProgram);
    GetVideoSystem()->GetImmediateContext()->GetPixelSystem()->SetProgram(m_kPixelData.m_spProgram);
    if (m_eMode == FORWARD)
    {
        for (auto &kPair : aPixelResources)
        {
            GetRenderManager()->SetResource(GetVideoSystem()->GetImmediateContext()->GetPixelSystem(), kPair.second, kPair.first);
        }

        GetRenderManager()->DrawScene(m_nVertexAttributes);
    }
    else if (m_eMode == LIGHTING)
    {
        GetRenderManager()->DrawLights([&](void) -> void
        {
            if (m_nDispatchXSize > 0)
            {
                for (auto &kPair : aComputeUnorderedAccess)
                {
                    GetVideoSystem()->GetImmediateContext()->GetComputeSystem()->SetUnorderedAccess(kPair.second, kPair.first);
                }

                for (auto &kPair : aComputeResources)
                {
                    GetRenderManager()->SetResource(GetVideoSystem()->GetImmediateContext()->GetComputeSystem(), kPair.second, kPair.first);
                }

                GetVideoSystem()->GetImmediateContext()->Dispatch(m_nDispatchXSize, m_nDispatchYSize, m_nDispatchZSize);
                for (auto &kPair : aPixelResources)
                {
                    GetRenderManager()->SetResource(GetVideoSystem()->GetImmediateContext()->GetPixelSystem(), kPair.second, kPair.first);
                }
            }
        });
    }
    else
    {
        for (auto &kPair : aPixelResources)
        {
            GetRenderManager()->SetResource(GetVideoSystem()->GetImmediateContext()->GetPixelSystem(), kPair.second, kPair.first);
        }

        GetRenderManager()->DrawOverlay();
    }

    GetVideoSystem()->GetImmediateContext()->ClearResources();
}