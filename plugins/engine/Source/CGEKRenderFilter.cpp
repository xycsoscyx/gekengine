#include "CGEKRenderFilter.h"
#include "GEKSystem.h"
#include "GEKAPI.h"
#include <algorithm>

#include "GEKSystemCLSIDs.h"

GEKVIDEO::DATA::FORMAT GetTargetFormat(LPCWSTR pValue)
{
    if (_wcsicmp(pValue, L"RGBA_FLOAT") == 0) return GEKVIDEO::DATA::RGBA_FLOAT;
    else if (_wcsicmp(pValue, L"RGBA_UINT8") == 0) return GEKVIDEO::DATA::RGBA_UINT8;
    else if (_wcsicmp(pValue, L"D16") == 0) return GEKVIDEO::DATA::D16;
    else if (_wcsicmp(pValue, L"D32") == 0) return GEKVIDEO::DATA::D32;
    else if (_wcsicmp(pValue, L"D24S8") == 0) return GEKVIDEO::DATA::D24_S8;
    else return GEKVIDEO::DATA::UNKNOWN;
}

GEKVIDEO::DEPTHWRITE::MASK GetDepthWriteMask(LPCWSTR pValue)
{
    if (_wcsicmp(pValue, L"zero") == 0) return GEKVIDEO::DEPTHWRITE::ZERO;
    else if (_wcsicmp(pValue, L"all") == 0) return GEKVIDEO::DEPTHWRITE::ALL;
    else return GEKVIDEO::DEPTHWRITE::ZERO;
}

GEKVIDEO::COMPARISON::FUNCTION GetComparisonFunction(LPCWSTR pValue)
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

GEKVIDEO::STENCIL::OPERATION GetStencilOperation(LPCWSTR pValue)
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

void GetStentilStates(GEKVIDEO::STENCILSTATES &kStates, CLibXMLNode &kNode)
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
        kTarget.m_spTexture = nullptr;
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
            hRetVal = GetVideoSystem()->CreateRenderTarget(nXSize, nYSize, kTarget.m_eFormat, &kTarget.m_spTexture);
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
                eFormat = GetTargetFormat(kDepthNode.GetAttribute(L"format"));
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
            GEKVIDEO::DATA::FORMAT eFormat = GetTargetFormat(kTargetNode.GetAttribute(L"format"));

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

                CComPtr<IGEKVideoTexture> spTexture;
                hRetVal = GetVideoSystem()->CreateRenderTarget(nXSize, nYSize, eFormat, &spTexture);
                if (spTexture != nullptr)
                {
                    kData.m_eFormat = eFormat;
                    kData.m_spTexture = spTexture;
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

HRESULT CGEKRenderFilter::LoadResources(DATA &kData, CLibXMLNode &kFilterNode)
{
    HRESULT hRetVal = S_OK;
    CLibXMLNode kResourcesNode = kFilterNode.FirstChildElement(L"resources");
    if (kResourcesNode)
    {
        CLibXMLNode nResourceNode = kResourcesNode.FirstChildElement(L"resource");
        while (nResourceNode)
        {
            if (nResourceNode.HasAttribute(L"stage"))
            {
                UINT32 nStage = StrToUINT32(nResourceNode.GetAttribute(L"stage"));

                RESOURCE kResource;
                if (nResourceNode.HasAttribute(L"source"))
                {
                    kResource.m_strName = nResourceNode.GetAttribute(L"source");
                }
                else if (nResourceNode.HasAttribute(L"data"))
                {
                    CComPtr<IUnknown> spTexture;
                    hRetVal = GetRenderManager()->LoadResource(nResourceNode.GetAttribute(L"data"), &spTexture);
                    if (SUCCEEDED(hRetVal))
                    {
                        kResource.m_spResource = spTexture;
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
                
                kData.m_aResources[nStage] = kResource;
                nResourceNode = nResourceNode.NextSiblingElement(L"resource");
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
                hRetVal = GetVideoSystem()->CompileComputeProgram(strProgram, "MainComputeProgram", &m_kComputeData.m_spProgram);
            }
        }

        if (SUCCEEDED(hRetVal))
        {
            hRetVal = LoadResources(m_kComputeData, kComputeNode);
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
                hRetVal = GetVideoSystem()->CompilePixelProgram(strProgram, "MainPixelProgram", &m_kPixelData.m_spProgram);
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
                hRetVal = LoadBlendStates(kFilterNode);
            }

            if (SUCCEEDED(hRetVal))
            {
                hRetVal = LoadRenderStates(kFilterNode);
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
    auto pIterator = m_aTargetMap.find(pName);
    if (pIterator != m_aTargetMap.end())
    {
        hRetVal = ((*pIterator).second)->m_spTexture->QueryInterface(IID_PPV_ARGS(ppTexture));
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

    std::vector<IGEKVideoTexture *> aTargets;
    if (m_aTargets.size() > 0)
    {
        for (auto &kTarget : m_aTargets)
        {
            CComQIPtr<IGEKVideoTexture> spTarget;
            if (kTarget.m_spTexture != nullptr)
            {
                spTarget = kTarget.m_spTexture;
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

    for (auto &kPair : m_kPixelData.m_aResources)
    {
        if (kPair.second.m_spResource != nullptr)
        {
            GetRenderManager()->SetResource(kPair.first, kPair.second.m_spResource);
        }
        else if (!kPair.second.m_strName.IsEmpty())
        {
            CComPtr<IUnknown> spTexture;
            GetRenderManager()->GetBuffer(kPair.second.m_strName, &spTexture);
            if (spTexture != nullptr)
            {
                GetRenderManager()->SetResource(kPair.first, spTexture);
            }
        }
    }
    
    CGEKRenderStates::Enable(GetVideoSystem());
    CGEKBlendStates::Enable(GetVideoSystem());
    GetVideoSystem()->GetImmediateContext()->SetDepthStates(m_nStencilReference, m_spDepthStates);
    GetVideoSystem()->GetImmediateContext()->GetPixelSystem()->SetProgram(m_kPixelData.m_spProgram);
    GetVideoSystem()->GetImmediateContext()->GetComputeSystem()->SetProgram(m_kComputeData.m_spProgram);
    if (m_eMode == FORWARD)
    {
        GetRenderManager()->DrawScene(m_nVertexAttributes);
    }
    else if (m_eMode == LIGHTING)
    {
        GetRenderManager()->DrawOverlay(true);
    }
    else
    {
        GetRenderManager()->DrawOverlay(false);
    }
}