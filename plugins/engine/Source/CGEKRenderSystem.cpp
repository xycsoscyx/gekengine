#include "CGEKRenderSystem.h"
#include "IGEKRenderFilter.h"
#include "CGEKProperties.h"
#include <windowsx.h>
#include <atlpath.h>
#include <algorithm>
#include <concurrent_vector.h>
#include <ppl.h>

#include "GEKEngineCLSIDs.h"
#include "GEKSystemCLSIDs.h"

DECLARE_INTERFACE_IID_(IGEKMaterial, IUnknown, "819CA201-F652-4183-B29D-BB71BB15810E")
{
    STDMETHOD_(void, Enable)                (THIS_ CGEKRenderSystem *pManager, IGEKVideoSystem *pSystem) PURE;
};

class CGEKMaterial : public CGEKUnknown
                   , public IGEKMaterial
                   , public CGEKRenderStates
                   , public CGEKBlendStates
{
private:
    CComPtr<IUnknown> m_spAlbedoMap;
    CComPtr<IUnknown> m_spNormalMap;
    CComPtr<IUnknown> m_spInfoMap;

public:
    DECLARE_UNKNOWN(CGEKMaterial);
    CGEKMaterial(IUnknown *pAlbedoMap, IUnknown *pNormalMap, IUnknown *pInfoMap)
        : m_spAlbedoMap(pAlbedoMap)
        , m_spNormalMap(pNormalMap)
        , m_spInfoMap(pInfoMap)
    {
    }

    ~CGEKMaterial(void)
    {
    }

    // IGEKMaterial
    STDMETHODIMP_(void) Enable(CGEKRenderSystem *pManager, IGEKVideoSystem *pSystem)
    {
        pManager->SetResource(nullptr, 0, m_spAlbedoMap);
        pManager->SetResource(nullptr, 1, m_spNormalMap);
        pManager->SetResource(nullptr, 2, m_spInfoMap);
        CGEKRenderStates::Enable(pSystem);
        CGEKBlendStates::Enable(pSystem);
    }
};

BEGIN_INTERFACE_LIST(CGEKMaterial)
    INTERFACE_LIST_ENTRY_COM(IGEKMaterial)
END_INTERFACE_LIST_UNKNOWN

DECLARE_INTERFACE_IID_(IGEKProgram, IUnknown, "0387E446-E858-4F3C-9E19-1F0E36D914E3")
{
    STDMETHOD_(IUnknown *, GetVertexProgram)    (THIS) PURE;
    STDMETHOD_(IUnknown *, GetGeometryProgram)  (THIS) PURE;
};

class CGEKProgram : public CGEKUnknown
                  , public IGEKProgram
{
private:
    CComPtr<IUnknown> m_spVertexProgram;
    CComPtr<IUnknown> m_spGeometryProgram;

public:
    DECLARE_UNKNOWN(CGEKProgram);
    CGEKProgram(IUnknown *pVertexProgram, IUnknown *pGeometryProgram)
        : m_spVertexProgram(pVertexProgram)
        , m_spGeometryProgram(pGeometryProgram)
    {
    }

    ~CGEKProgram(void)
    {
    }

    STDMETHODIMP_(IUnknown *) GetVertexProgram(void)
    {
        return m_spVertexProgram;
    }

    STDMETHODIMP_(IUnknown *) GetGeometryProgram(void)
    {
        return (m_spGeometryProgram ? m_spGeometryProgram : nullptr);
    }
};

BEGIN_INTERFACE_LIST(CGEKProgram)
    INTERFACE_LIST_ENTRY_COM(IGEKProgram)
END_INTERFACE_LIST_UNKNOWN

static GEKVIDEO::DATA::FORMAT GetFormatType(LPCWSTR pValue)
{
         if (_wcsicmp(pValue, L"R_FLOAT") == 0) return GEKVIDEO::DATA::R_FLOAT;
    else if (_wcsicmp(pValue, L"RG_FLOAT") == 0) return GEKVIDEO::DATA::RG_FLOAT;
    else if (_wcsicmp(pValue, L"RGB_FLOAT") == 0) return GEKVIDEO::DATA::RGB_FLOAT;
    else if (_wcsicmp(pValue, L"RGBA_FLOAT") == 0) return GEKVIDEO::DATA::RGBA_FLOAT;
    else if (_wcsicmp(pValue, L"R_UINT32") == 0) return GEKVIDEO::DATA::R_UINT32;
    else if (_wcsicmp(pValue, L"RG_UINT32") == 0) return GEKVIDEO::DATA::RG_UINT32;
    else if (_wcsicmp(pValue, L"RGB_UINT32") == 0) return GEKVIDEO::DATA::RGB_UINT32;
    else if (_wcsicmp(pValue, L"RGBA_UINT32") == 0) return GEKVIDEO::DATA::RGBA_UINT32;
    else return GEKVIDEO::DATA::UNKNOWN;
}

static GEKVIDEO::INPUT::SOURCE GetElementClass(LPCWSTR pValue)
{
         if (_wcsicmp(pValue, L"vertex") == 0) return GEKVIDEO::INPUT::VERTEX;
    else if (_wcsicmp(pValue, L"instance") == 0) return GEKVIDEO::INPUT::INSTANCE;
    else return GEKVIDEO::INPUT::UNKNOWN;
}

BEGIN_INTERFACE_LIST(CGEKRenderSystem)
    INTERFACE_LIST_ENTRY_COM(IGEKObservable)
    INTERFACE_LIST_ENTRY_COM(IGEKSceneObserver)
    INTERFACE_LIST_ENTRY_COM(IGEKRenderSystem)
    INTERFACE_LIST_ENTRY_COM(IGEKProgramManager)
    INTERFACE_LIST_ENTRY_COM(IGEKMaterialManager)
END_INTERFACE_LIST_UNKNOWN

REGISTER_CLASS(CGEKRenderSystem)

CGEKRenderSystem::CGEKRenderSystem(void)
    : m_pSystem(nullptr)
    , m_pVideoSystem(nullptr)
    , m_pEngine(nullptr)
    , m_pSceneManager(nullptr)
    , m_pCurrentPass(nullptr)
    , m_pCurrentFilter(nullptr)
    , m_nNumLightInstances(254)
{
}

CGEKRenderSystem::~CGEKRenderSystem(void)
{
}

STDMETHODIMP CGEKRenderSystem::Initialize(void)
{
    GEKFUNCTION(nullptr);
    HRESULT hRetVal = GetContext()->AddCachedClass(CLSID_GEKRenderSystem, GetUnknown());
    if (SUCCEEDED(hRetVal))
    {
        m_pSystem = GetContext()->GetCachedClass<IGEKSystem>(CLSID_GEKSystem);
        m_pVideoSystem = GetContext()->GetCachedClass<IGEKVideoSystem>(CLSID_GEKVideoSystem);
        m_pEngine = GetContext()->GetCachedClass<IGEKEngine>(CLSID_GEKEngine);
        m_pSceneManager = GetContext()->GetCachedClass<IGEKSceneManager>(CLSID_GEKPopulationSystem);
        if (m_pSystem == nullptr ||
            m_pVideoSystem == nullptr ||
            m_pEngine == nullptr ||
            m_pSceneManager == nullptr)
        {
            hRetVal = E_FAIL;
        }
    }

    if (SUCCEEDED(hRetVal))
    {
        hRetVal = CGEKObservable::AddObserver(m_pSceneManager, (IGEKSceneObserver *)GetUnknown());
    }

    if (SUCCEEDED(hRetVal))
    {
        std::vector<GEKVIDEO::INPUTELEMENT> aLayout;
        aLayout.push_back(GEKVIDEO::INPUTELEMENT(GEKVIDEO::DATA::RG_FLOAT, "POSITION", 0));
        aLayout.push_back(GEKVIDEO::INPUTELEMENT(GEKVIDEO::DATA::RG_FLOAT, "TEXCOORD", 0));
        hRetVal = m_pVideoSystem->LoadVertexProgram(L"%root%\\data\\programs\\vertex\\overlay.hlsl", "MainVertexProgram", aLayout, &m_spVertexProgram);
        GEKRESULT(SUCCEEDED(hRetVal), L"Call to LoadVertexProgram failed: 0x%08X", hRetVal);
    }

    if (SUCCEEDED(hRetVal))
    {
        hRetVal = m_pVideoSystem->LoadPixelProgram(L"%root%\\data\\programs\\pixel\\overlay.hlsl", "MainPixelProgram", &m_spPixelProgram);
        GEKRESULT(SUCCEEDED(hRetVal), L"Call to LoadPixelProgram failed: 0x%08X", hRetVal);
    }

    if (SUCCEEDED(hRetVal))
    {
        float2 aVertices[8] =
        {
            float2(0.0f, 0.0f), float2(-1.0f, 1.0f),
            float2(1.0f, 0.0f), float2(1.0f, 1.0f),
            float2(1.0f, 1.0f), float2(1.0f, -1.0f),
            float2(0.0f, 1.0f), float2(-1.0f, -1.0f),
        };

        hRetVal = m_pVideoSystem->CreateBuffer((sizeof(float2) * 2), 4, GEKVIDEO::BUFFER::VERTEX_BUFFER | GEKVIDEO::BUFFER::STATIC, &m_spVertexBuffer, aVertices);
        GEKRESULT(SUCCEEDED(hRetVal), L"Call to CreateBuffer failed: 0x%08X", hRetVal);
    }

    if (SUCCEEDED(hRetVal))
    {
        UINT16 aIndices[6] =
        {
            0, 1, 2,
            0, 2, 3,
        };

        hRetVal = m_pVideoSystem->CreateBuffer(sizeof(UINT16), 6, GEKVIDEO::BUFFER::INDEX_BUFFER | GEKVIDEO::BUFFER::STATIC, &m_spIndexBuffer, aIndices);
        GEKRESULT(SUCCEEDED(hRetVal), L"Call to CreateBuffer failed: 0x%08X", hRetVal);
    }

    if (SUCCEEDED(hRetVal))
    {
        hRetVal = m_pVideoSystem->CreateBuffer(sizeof(float4x4), 1, GEKVIDEO::BUFFER::CONSTANT_BUFFER, &m_spOrthoBuffer);
        GEKRESULT(SUCCEEDED(hRetVal), L"Call to CreateBuffer failed: 0x%08X", hRetVal);
        if (m_spOrthoBuffer)
        {
            float4x4 nOverlayMatrix;
            nOverlayMatrix.SetOrthographic(0.0f, 0.0f, 1.0f, 1.0f, -1.0f, 1.0f);
            m_spOrthoBuffer->Update((void *)&nOverlayMatrix);
        }
    }

    if (SUCCEEDED(hRetVal))
    {
        hRetVal = m_pVideoSystem->CreateBuffer(sizeof(ENGINEBUFFER), 1, GEKVIDEO::BUFFER::CONSTANT_BUFFER, &m_spEngineBuffer);
        GEKRESULT(SUCCEEDED(hRetVal), L"Call to CreateBuffer failed: 0x%08X", hRetVal);
    }

    if (SUCCEEDED(hRetVal))
    {
        hRetVal = m_pVideoSystem->CreateBuffer(sizeof(UINT32) * 4, 1, GEKVIDEO::BUFFER::CONSTANT_BUFFER, &m_spLightCountBuffer);
        GEKRESULT(SUCCEEDED(hRetVal), L"Call to CreateBuffer failed: 0x%08X", hRetVal);
    }

    if (SUCCEEDED(hRetVal))
    {
        hRetVal = m_pVideoSystem->CreateBuffer(sizeof(LIGHT), m_nNumLightInstances, GEKVIDEO::BUFFER::DYNAMIC | GEKVIDEO::BUFFER::STRUCTURED_BUFFER | GEKVIDEO::BUFFER::RESOURCE, &m_spLightBuffer);
        GEKRESULT(SUCCEEDED(hRetVal), L"Call to CreateBuffer failed: 0x%08X", hRetVal);
    }

    if (SUCCEEDED(hRetVal))
    {
        GEKVIDEO::SAMPLERSTATES kStates;
        kStates.m_eFilter = GEKVIDEO::FILTER::MIN_MAG_MIP_POINT;
        kStates.m_eAddressU = GEKVIDEO::ADDRESS::CLAMP;
        kStates.m_eAddressV = GEKVIDEO::ADDRESS::CLAMP;
        hRetVal = m_pVideoSystem->CreateSamplerStates(kStates, &m_spPointSampler);
        GEKRESULT(SUCCEEDED(hRetVal), L"Call to CreateSamplerStates failed: 0x%08X", hRetVal);
    }

    if (SUCCEEDED(hRetVal))
    {
        GEKVIDEO::SAMPLERSTATES kStates;
        if (m_pSystem->GetConfig().DoesValueExists(L"render", L"anisotropy"))
        {
            kStates.m_nMaxAnisotropy = StrToUINT32(m_pSystem->GetConfig().GetValue(L"render", L"anisotropy", L"1"));
            kStates.m_eFilter = GEKVIDEO::FILTER::ANISOTROPIC;
        }
        else
        {
            kStates.m_eFilter = GEKVIDEO::FILTER::MIN_MAG_MIP_LINEAR;
        }

        kStates.m_eAddressU = GEKVIDEO::ADDRESS::WRAP;
        kStates.m_eAddressV = GEKVIDEO::ADDRESS::WRAP;
        hRetVal = m_pVideoSystem->CreateSamplerStates(kStates, &m_spLinearSampler);
        GEKRESULT(SUCCEEDED(hRetVal), L"Call to CreateSamplerStates failed: 0x%08X", hRetVal);
    }

    if (SUCCEEDED(hRetVal))
    {
        UINT32 nXSize = m_pSystem->GetXSize();
        UINT32 nYSize = m_pSystem->GetYSize();
        hRetVal = m_pVideoSystem->CreateRenderTarget(nXSize, nYSize, GEKVIDEO::DATA::RGBA_UINT8, &m_spScreenBuffer);
    }

    if (SUCCEEDED(hRetVal))
    {
        GEKVIDEO::RENDERSTATES kRenderStates;
        hRetVal = m_pVideoSystem->CreateRenderStates(kRenderStates, &m_spRenderStates);
    }

    if (SUCCEEDED(hRetVal))
    {
        GEKVIDEO::UNIFIEDBLENDSTATES kBlendStates;
        hRetVal = m_pVideoSystem->CreateBlendStates(kBlendStates, &m_spBlendStates);
    }
   
    if (SUCCEEDED(hRetVal))
    {
        GEKVIDEO::DEPTHSTATES kDepthStates;
        hRetVal = m_pVideoSystem->CreateDepthStates(kDepthStates, &m_spDepthStates);
    }

    if (SUCCEEDED(hRetVal))
    {
        hRetVal = m_pVideoSystem->CreateEvent(&m_spFrameEvent);
        GEKRESULT(SUCCEEDED(hRetVal), L"Call to CreateEvent failed: 0x%08X", hRetVal);
    }

    return hRetVal;
}

STDMETHODIMP_(void) CGEKRenderSystem::Destroy(void)
{
    m_pCurrentPass = nullptr;
    m_pCurrentFilter = nullptr;
    m_aVisibleLights.clear();
    m_aResources.clear();
    m_aFilters.clear();
    m_aPasses.clear();

    GetContext()->RemoveCachedObserver(CLSID_GEKPopulationSystem, (IGEKSceneObserver *)GetUnknown());
    GetContext()->RemoveCachedClass(CLSID_GEKRenderSystem);
}

STDMETHODIMP_(void) CGEKRenderSystem::OnLoadBegin(void)
{
    m_aResources.clear();
    m_aFilters.clear();
    m_aPasses.clear();
}

STDMETHODIMP CGEKRenderSystem::OnLoadEnd(HRESULT hRetVal)
{
    if (SUCCEEDED(hRetVal))
    {
        m_pVideoSystem->SetEvent(m_spFrameEvent);
    }

    return hRetVal;
}

STDMETHODIMP_(void) CGEKRenderSystem::OnFree(void)
{
    m_aResources.clear();
    m_aFilters.clear();
    m_aPasses.clear();
    m_pCurrentPass = nullptr;
    m_pCurrentFilter = nullptr;
    m_aVisibleLights.clear();
}

HRESULT CGEKRenderSystem::LoadPass(LPCWSTR pName)
{
    GEKFUNCTION(L"Name(%s)", pName);
    HRESULT hRetVal = E_FAIL;
    auto pPassIterator = m_aPasses.find(pName);
    if (pPassIterator != m_aPasses.end())
    {
        hRetVal = S_OK;
    }
    else
    {
        CLibXMLDoc kDocument;
        CStringW strFileName(FormatString(L"%%root%%\\data\\passes\\%s.xml", pName));
        hRetVal = kDocument.Load(strFileName);
        if (SUCCEEDED(hRetVal))
        {
            PASS kPassData;
            hRetVal = E_INVALID;
            CLibXMLNode kPassNode = kDocument.GetRoot();
            if (kPassNode)
            {
                hRetVal = E_INVALID;
                CLibXMLNode kFiltersNode = kPassNode.FirstChildElement(L"filters");
                if (kFiltersNode)
                {
                    CLibXMLNode kFilterNode = kFiltersNode.FirstChildElement(L"filter");
                    while (kFilterNode)
                    {
                        CStringW strFilter = kFilterNode.GetAttribute(L"source");
                        auto pFilterIterator = m_aFilters.find(strFilter);
                        if (pFilterIterator != m_aFilters.end())
                        {
                            kPassData.m_aFilters.push_back((*pFilterIterator).second);
                            hRetVal = S_OK;
                        }
                        else
                        {
                            CComPtr<IGEKRenderFilter> spFilter;
                            hRetVal = GetContext()->CreateInstance(CLSID_GEKRenderFilter, IID_PPV_ARGS(&spFilter));
                            if (spFilter)
                            {
                                CStringW strFilterFileName(L"%root%\\data\\filters\\" + strFilter + L".xml");
                                hRetVal = spFilter->Load(strFilterFileName);
                                if (SUCCEEDED(hRetVal))
                                {
                                    m_aFilters[strFilter] = spFilter;
                                    kPassData.m_aFilters.push_back(spFilter);
                                }
                                else
                                {
                                    break;
                                }
                            }
                        }

                        kFilterNode = kFilterNode.NextSiblingElement(L"filter");
                    };
                }
            }

            if (SUCCEEDED(hRetVal))
            {
                m_aPasses[pName] = kPassData;
            }
        }
    }

    return hRetVal;
}

STDMETHODIMP CGEKRenderSystem::LoadResource(LPCWSTR pName, IUnknown **ppResource)
{
    REQUIRE_RETURN(pName, E_INVALIDARG);
    REQUIRE_RETURN(ppResource, E_INVALIDARG);

    HRESULT hRetVal = E_FAIL;
    auto pIterator = m_aResources.find(pName);
    if (pIterator != m_aResources.end())
    {
        hRetVal = (*pIterator).second->QueryInterface(IID_PPV_ARGS(ppResource));
    }
    else
    {
        GEKFUNCTION(L"Name(%s)", pName);
        CComPtr<IUnknown> spTexture;
        if (pName[0] == L'*')
        {
            int nPosition = 0;
            CStringW strName = &pName[1];
            CStringW strType = strName.Tokenize(L":", nPosition);
            if (strType.CompareNoCase(L"color") == 0)
            {
                CStringW strColor = strName.Tokenize(L":", nPosition);
                GEKLOG(L"Creating Color Texture: %s", strColor.GetString());
                float4 nColor = StrToFloat4(strColor);

                CComPtr<IGEKVideoTexture> spColorTexture;
                hRetVal = m_pVideoSystem->CreateTexture(1, 1, 1, GEKVIDEO::DATA::RGBA_UINT8, GEKVIDEO::TEXTURE::RESOURCE, &spColorTexture);
                if (spColorTexture)
                {
                    UINT32 nColorValue = UINT32(UINT8(nColor.r * 255.0f)) |
                        UINT32(UINT8(nColor.g * 255.0f) << 8) |
                        UINT32(UINT8(nColor.b * 255.0f) << 16) |
                        UINT32(UINT8(nColor.a * 255.0f) << 24);
                    m_pVideoSystem->UpdateTexture(spColorTexture, &nColorValue, 4);
                    spTexture = spColorTexture;
                }
            }
        }
        else
        {
            GEKLOG(L"Loading Texture: %s", pName);
            CComPtr<IGEKVideoTexture> spFileTexture;
            hRetVal = m_pVideoSystem->LoadTexture(FormatString(L"%%root%%\\data\\textures\\%s", pName), &spFileTexture);
            if (spFileTexture)
            {
                spTexture = spFileTexture;
            }
        }

        if (spTexture)
        {
            spTexture->QueryInterface(IID_PPV_ARGS(&m_aResources[pName]));
            hRetVal = spTexture->QueryInterface(IID_PPV_ARGS(ppResource));
        }
    }

    return hRetVal;
}

STDMETHODIMP_(void) CGEKRenderSystem::SetResource(IGEKVideoContextSystem *pSystem, UINT32 nStage, IUnknown *pResource)
{
    CComPtr<IUnknown> spResource(pResource);
    if (spResource)
    {
        if (pSystem == nullptr)
        {
            m_pVideoSystem->GetImmediateContext()->GetPixelSystem()->SetResource(nStage, spResource);
        }
        else
        {
            pSystem->SetResource(nStage, spResource);
        }
    }
}

STDMETHODIMP CGEKRenderSystem::GetBuffer(LPCWSTR pName, IUnknown **ppResource)
{
    REQUIRE_RETURN(ppResource, E_INVALIDARG);

    HRESULT hRetVal = E_FAIL;

    int nPosition = 0;
    CStringW strName = pName;
    CStringW strFilter = strName.Tokenize(L".", nPosition);
    CStringW strSource = strName.Tokenize(L".", nPosition);
    for (auto &kPair : m_aFilters)
    {
        if (kPair.first == strFilter)
        {
            hRetVal = kPair.second->GetBuffer(strSource, ppResource);
            break;
        }
    }

    return hRetVal;
}

STDMETHODIMP CGEKRenderSystem::GetDepthBuffer(LPCWSTR pSource, IUnknown **ppBuffer)
{
    HRESULT hRetVal = E_FAIL;
    for (auto &kPair : m_aFilters)
    {
        if (kPair.first == pSource)
        {
            hRetVal = kPair.second->GetDepthBuffer(ppBuffer);
            break;
        }
    }

    return hRetVal;
}

STDMETHODIMP_(void) CGEKRenderSystem::SetScreenTargets(IUnknown *pDepthBuffer)
{
    REQUIRE_VOID_RETURN(m_pVideoSystem);

    m_pVideoSystem->GetImmediateContext()->SetRenderTargets({ m_spScreenBuffer }, (pDepthBuffer ? pDepthBuffer : nullptr));
    m_pVideoSystem->GetImmediateContext()->SetViewports({ m_kScreenViewPort });
}

STDMETHODIMP CGEKRenderSystem::LoadMaterial(LPCWSTR pName, IUnknown **ppMaterial)
{
    REQUIRE_RETURN(ppMaterial, E_INVALIDARG);

    HRESULT hRetVal = E_FAIL;
    auto pIterator = m_aResources.find(pName);
    if (pIterator != m_aResources.end())
    {
        hRetVal = (*pIterator).second->QueryInterface(IID_PPV_ARGS(ppMaterial));
    }
    else
    {
        GEKFUNCTION(L"Name(%s)", pName);

        CLibXMLDoc kDocument;
        hRetVal = kDocument.Load(FormatString(L"%%root%%\\data\\materials\\%s.xml", pName));
        if (SUCCEEDED(hRetVal))
        {
            hRetVal = E_INVALIDARG;
            CLibXMLNode kMaterialNode = kDocument.GetRoot();
            if (kMaterialNode)
            {
                CPathW kName(pName);
                kName.RemoveFileSpec();

                CComPtr<IUnknown> spAlbedoMap;
                CLibXMLNode kAlbedoNode = kMaterialNode.FirstChildElement(L"albedo");
                CStringW strAlbedo = kAlbedoNode.GetAttribute(L"source");
                strAlbedo.Replace(L"%material%", pName);
                strAlbedo.Replace(L"%directory%", kName.m_strPath.GetString());
                LoadResource(strAlbedo, &spAlbedoMap);
                if (!spAlbedoMap)
                {
                    LoadResource(L"*color:1,1,1,1", &spAlbedoMap);
                }

                CComPtr<IUnknown> spNormalMap;
                CLibXMLNode kNormalNode = kMaterialNode.FirstChildElement(L"normal");
                CStringW strNormal = kNormalNode.GetAttribute(L"source");
                strNormal.Replace(L"%material%", pName);
                strNormal.Replace(L"%directory%", kName.m_strPath.GetString());
                LoadResource(strNormal, &spNormalMap);
                if (!spNormalMap)
                {
                    LoadResource(L"*color:0.5,0.5,1,1", &spNormalMap);
                }

                CComPtr<IUnknown> spInfoMap;
                CLibXMLNode kInfoNode = kMaterialNode.FirstChildElement(L"info");
                CStringW strInfo = kInfoNode.GetAttribute(L"source");
                strInfo.Replace(L"%material%", pName);
                strInfo.Replace(L"%directory%", kName.m_strPath.GetString());
                LoadResource(strInfo, &spInfoMap);
                if (!spInfoMap)
                {
                    LoadResource(L"*color:0.5,0,0,0", &spInfoMap);
                }

                CComPtr<CGEKMaterial> spMaterial(new CGEKMaterial(spAlbedoMap, spNormalMap, spInfoMap));
                GEKRESULT(spMaterial, L"Unable to allocate new material instance");
                if (spMaterial)
                {
                    spMaterial->CGEKRenderStates::Load(m_pVideoSystem, kMaterialNode.FirstChildElement(L"render"));
                    spMaterial->CGEKBlendStates::Load(m_pVideoSystem, kMaterialNode.FirstChildElement(L"blend"));
                    spMaterial->QueryInterface(IID_PPV_ARGS(&m_aResources[pName]));
                    hRetVal = spMaterial->QueryInterface(IID_PPV_ARGS(ppMaterial));
                }
            }
        }
    }

    if (!(*ppMaterial))
    {
        HRESULT hRetVal = E_FAIL;
        auto pIterator = m_aResources.find(L"*default");
        if (pIterator != m_aResources.end())
        {
            hRetVal = (*pIterator).second->QueryInterface(IID_PPV_ARGS(ppMaterial));
        }
        else
        {
            CComPtr<IUnknown> spAlbedoMap;
            LoadResource(L"*color:1,1,1,1", &spAlbedoMap);

            CComPtr<IUnknown> spNormalMap;
            LoadResource(L"*color:0.5,0.5,1,1", &spNormalMap);

            CComPtr<IUnknown> spInfoMap;
            LoadResource(L"*color:0.5,0,0,0", &spInfoMap);

            CComPtr<CGEKMaterial> spMaterial(new CGEKMaterial(spAlbedoMap, spNormalMap, spInfoMap));
            GEKRESULT(spMaterial, L"Unable to allocate material new instance");
            if (spMaterial)
            {
                CLibXMLNode kBlankNode(nullptr);
                spMaterial->CGEKRenderStates::Load(m_pVideoSystem, kBlankNode);
                spMaterial->CGEKBlendStates::Load(m_pVideoSystem, kBlankNode);
                spMaterial->QueryInterface(IID_PPV_ARGS(&m_aResources[L"*default"]));
                hRetVal = spMaterial->QueryInterface(IID_PPV_ARGS(ppMaterial));
            }
        }
    }

    return hRetVal;
}

STDMETHODIMP_(bool) CGEKRenderSystem::EnableMaterial(IUnknown *pMaterial)
{
    REQUIRE_RETURN(pMaterial, false);

    bool bReturn = false;
    CComQIPtr<IGEKMaterial> spMaterial(pMaterial);
    if (spMaterial)
    {
        bReturn = true;
        spMaterial->Enable(this, m_pVideoSystem);
    }

    return bReturn;
}

STDMETHODIMP CGEKRenderSystem::LoadProgram(LPCWSTR pName, IUnknown **ppProgram)
{
    REQUIRE_RETURN(ppProgram, E_INVALIDARG);

    HRESULT hRetVal = E_FAIL;
    auto pIterator = m_aResources.find(pName);
    if (pIterator != m_aResources.end())
    {
        hRetVal = (*pIterator).second->QueryInterface(IID_PPV_ARGS(ppProgram));
    }
    else
    {
        GEKFUNCTION(L"Name(%s)", pName);

        CStringA strDeferredProgram;
        hRetVal = GEKLoadFromFile(L"%root%\\data\\programs\\vertex\\deferred.hlsl", strDeferredProgram);
        if (SUCCEEDED(hRetVal))
        {
            if (strDeferredProgram.Find("_INSERT_WORLD_PROGRAM") < 0)
            {
                hRetVal = E_INVALID;
            }
            else
            {
                CLibXMLDoc kDocument;
                hRetVal = kDocument.Load(FormatString(L"%%root%%\\data\\programs\\vertex\\%s.xml", pName));
                if (SUCCEEDED(hRetVal))
                {
                    hRetVal = E_INVALIDARG;
                    CLibXMLNode kProgramNode = kDocument.GetRoot();
                    if (kProgramNode)
                    {
                        CLibXMLNode kLayoutNode = kProgramNode.FirstChildElement(L"layout");
                        if (kLayoutNode)
                        {
                            std::vector<CStringA> aNames;
                            std::vector<GEKVIDEO::INPUTELEMENT> aLayout;
                            CLibXMLNode kElementNode = kLayoutNode.FirstChildElement(L"element");
                            while (kElementNode)
                            {
                                if (kElementNode.HasAttribute(L"type") && 
                                   kElementNode.HasAttribute(L"name") &&
                                   kElementNode.HasAttribute(L"index"))
                                {
                                    aNames.push_back((LPCSTR)CW2A(kElementNode.GetAttribute(L"name")));

                                    GEKVIDEO::INPUTELEMENT kData;
                                    kData.m_eType = GetFormatType(kElementNode.GetAttribute(L"type"));
                                    kData.m_pName = aNames.back().GetString();
                                    kData.m_nIndex = StrToUINT32(kElementNode.GetAttribute(L"index"));
                                    if (kElementNode.HasAttribute(L"class") &&
                                       kElementNode.HasAttribute(L"slot"))
                                    {
                                        kData.m_eClass = GetElementClass(kElementNode.GetAttribute(L"class"));
                                        kData.m_nSlot = StrToUINT32(kElementNode.GetAttribute(L"slot"));
                                    }

                                    aLayout.push_back(kData);
                                }
                                else
                                {
                                    break;
                                }

                                kElementNode = kElementNode.NextSiblingElement(L"element");
                            };

                            hRetVal = S_OK;
                            CComPtr<IUnknown> spGeometryProgram;
                            CLibXMLNode kGeometryNode = kProgramNode.FirstChildElement(L"geometry");
                            if (kGeometryNode)
                            {
                                CStringA strGeometryProgram = kGeometryNode.GetText();
                                hRetVal = m_pVideoSystem->CompileGeometryProgram(strGeometryProgram, "MainGeometryProgram", &spGeometryProgram);
                            }

                            if (SUCCEEDED(hRetVal))
                            {
                                hRetVal = E_INVALIDARG;
                                CLibXMLNode kVertexNode = kProgramNode.FirstChildElement(L"vertex");
                                if (kVertexNode)
                                {
                                    CStringA strVertexProgram = kVertexNode.GetText();
                                    strDeferredProgram.Replace("_INSERT_WORLD_PROGRAM", (strVertexProgram + "\r\n"));

                                    CComPtr<IUnknown> spVertexProgram;
                                    hRetVal = m_pVideoSystem->CompileVertexProgram(strDeferredProgram, "MainVertexProgram", aLayout, &spVertexProgram);
                                    if (spVertexProgram)
                                    {
                                        CComPtr<CGEKProgram> spProgram(new CGEKProgram(spVertexProgram, spGeometryProgram));
                                        GEKRESULT(spProgram, L"Unable to allocate new program instance");
                                        if (spProgram)
                                        {
                                            spProgram->QueryInterface(IID_PPV_ARGS(&m_aResources[pName]));
                                            hRetVal = spProgram->QueryInterface(IID_PPV_ARGS(ppProgram));
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    return hRetVal;
}

STDMETHODIMP_(void) CGEKRenderSystem::EnableProgram(IUnknown *pProgram)
{
    REQUIRE_VOID_RETURN(m_pVideoSystem);
    REQUIRE_VOID_RETURN(pProgram);

    CComQIPtr<IGEKProgram> spProgram(pProgram);
    if (spProgram)
    {
        m_pVideoSystem->GetImmediateContext()->GetVertexSystem()->SetProgram(spProgram->GetVertexProgram());
        m_pVideoSystem->GetImmediateContext()->GetGeometrySystem()->SetProgram(spProgram->GetGeometryProgram());
    }
}

STDMETHODIMP_(void) CGEKRenderSystem::DrawScene(UINT32 nAttributes)
{
    CGEKObservable::SendEvent(TGEKEvent<IGEKRenderObserver>(std::bind(&IGEKRenderObserver::OnDrawScene, std::placeholders::_1, nAttributes)));
}

STDMETHODIMP_(void) CGEKRenderSystem::DrawLights(std::function<void(void)> OnLightBatch)
{
    m_pVideoSystem->GetImmediateContext()->GetVertexSystem()->SetProgram(m_spVertexProgram);
    m_pVideoSystem->GetImmediateContext()->GetVertexSystem()->SetConstantBuffer(1, m_spOrthoBuffer);
    m_pVideoSystem->GetImmediateContext()->GetGeometrySystem()->SetProgram(nullptr);
    m_pVideoSystem->GetImmediateContext()->GetPixelSystem()->SetResource(0, m_spLightBuffer);
    m_pVideoSystem->GetImmediateContext()->GetPixelSystem()->SetConstantBuffer(1, m_spLightCountBuffer);
    m_pVideoSystem->GetImmediateContext()->GetComputeSystem()->SetResource(0, m_spLightBuffer);
    m_pVideoSystem->GetImmediateContext()->GetComputeSystem()->SetConstantBuffer(1, m_spLightCountBuffer);

    m_pVideoSystem->GetImmediateContext()->SetVertexBuffer(0, 0, m_spVertexBuffer);
    m_pVideoSystem->GetImmediateContext()->SetIndexBuffer(0, m_spIndexBuffer);
    m_pVideoSystem->GetImmediateContext()->SetPrimitiveType(GEKVIDEO::PRIMITIVE::TRIANGLELIST);

    for (UINT32 nPass = 0; nPass < m_aVisibleLights.size(); nPass += m_nNumLightInstances)
    {
        UINT32 nNumLights = min(m_nNumLightInstances, (m_aVisibleLights.size() - nPass));
        if (nNumLights > 1)
        {
            UINT32 aCounts[4] =
            {
                nNumLights, 0, 0, 0,
            };

            m_spLightCountBuffer->Update(aCounts);

            LIGHT *pLights = nullptr;
            if (SUCCEEDED(m_spLightBuffer->Map((LPVOID *)&pLights)))
            {
                memcpy(pLights, &m_aVisibleLights[nPass], (sizeof(LIGHT)* nNumLights));
                m_spLightBuffer->UnMap();

                OnLightBatch();

                m_pVideoSystem->GetImmediateContext()->DrawIndexedPrimitive(6, 0, 0);
            }
        }
    }
}

STDMETHODIMP_(void) CGEKRenderSystem::DrawOverlay(void)
{
    m_pVideoSystem->GetImmediateContext()->GetVertexSystem()->SetProgram(m_spVertexProgram);
    m_pVideoSystem->GetImmediateContext()->GetVertexSystem()->SetConstantBuffer(1, m_spOrthoBuffer);

    m_pVideoSystem->GetImmediateContext()->GetGeometrySystem()->SetProgram(nullptr);

    m_pVideoSystem->GetImmediateContext()->SetVertexBuffer(0, 0, m_spVertexBuffer);
    m_pVideoSystem->GetImmediateContext()->SetIndexBuffer(0, m_spIndexBuffer);
    m_pVideoSystem->GetImmediateContext()->SetPrimitiveType(GEKVIDEO::PRIMITIVE::TRIANGLELIST);

    m_pVideoSystem->GetImmediateContext()->DrawIndexedPrimitive(6, 0, 0);
}

STDMETHODIMP_(void) CGEKRenderSystem::Render(void)
{
    REQUIRE_VOID_RETURN(m_pSceneManager && m_pVideoSystem);
    m_pVideoSystem->GetImmediateContext()->GetPixelSystem()->SetSamplerStates(0, m_spPointSampler);
    m_pVideoSystem->GetImmediateContext()->GetPixelSystem()->SetSamplerStates(1, m_spLinearSampler);

    m_pSceneManager->ListComponentsEntities({ L"transform", L"viewer" }, [&](const GEKENTITYID &nViewerID)->void
    {
        GEKVALUE kPass;
        m_pSceneManager->GetProperty(nViewerID, L"viewer", L"pass", kPass);
        if (SUCCEEDED(LoadPass(kPass.GetRawString())))
        {
            CGEKObservable::SendEvent(TGEKEvent<IGEKRenderObserver>(std::bind(&IGEKRenderObserver::OnPreRender, std::placeholders::_1)));

            GEKVALUE kProjection;
            m_pSceneManager->GetProperty(nViewerID, L"viewer", L"projection", kProjection);

            GEKVALUE kMinViewDistance;
            GEKVALUE kMaxViewDistance;
            m_pSceneManager->GetProperty(nViewerID, L"viewer", L"minviewdistance", kMinViewDistance);
            m_pSceneManager->GetProperty(nViewerID, L"viewer", L"maxviewdistance", kMaxViewDistance);

            GEKVALUE kFieldOfView;
            m_pSceneManager->GetProperty(nViewerID, L"viewer", L"fieldofview", kFieldOfView);
            float nFieldOfView = _DEGTORAD(kFieldOfView.GetFloat());

            GEKVALUE kViewPort;
            m_pSceneManager->GetProperty(nViewerID, L"viewer", L"viewport", kViewPort);
            m_kScreenViewPort.m_nTopLeftX = kViewPort.GetFloat4().x * m_pSystem->GetXSize();
            m_kScreenViewPort.m_nTopLeftY = kViewPort.GetFloat4().y * m_pSystem->GetYSize();
            m_kScreenViewPort.m_nXSize = kViewPort.GetFloat4().z * m_pSystem->GetXSize();
            m_kScreenViewPort.m_nYSize = kViewPort.GetFloat4().w * m_pSystem->GetYSize();
            m_kScreenViewPort.m_nMinDepth = 0.0f;
            m_kScreenViewPort.m_nMaxDepth = 1.0f;

            GEKVALUE kPosition;
            GEKVALUE kRotation;
            m_pSceneManager->GetProperty(nViewerID, L"transform", L"position", kPosition);
            m_pSceneManager->GetProperty(nViewerID, L"transform", L"rotation", kRotation);

            float4x4 nCameraMatrix;
            nCameraMatrix = kRotation.GetQuaternion();
            nCameraMatrix.t = kPosition.GetFloat3();

            float nXSize = float(m_pSystem->GetXSize());
            float nYSize = float(m_pSystem->GetYSize());
            float nAspect = (nXSize / nYSize);

            m_kCurrentBuffer.m_nCameraSize.x = nXSize;
            m_kCurrentBuffer.m_nCameraSize.y = nYSize;
            m_kCurrentBuffer.m_nCameraView.x = tan(nFieldOfView * 0.5f);
            m_kCurrentBuffer.m_nCameraView.y = (m_kCurrentBuffer.m_nCameraView.x / nAspect);
            m_kCurrentBuffer.m_nCameraViewDistance = kMaxViewDistance.GetFloat();
            m_kCurrentBuffer.m_nCameraPosition = kPosition.GetFloat3();

            m_kCurrentBuffer.m_nViewMatrix = nCameraMatrix.GetInverse();
            m_kCurrentBuffer.m_nProjectionMatrix = kProjection.GetFloat4x4();
            m_kCurrentBuffer.m_nTransformMatrix = (m_kCurrentBuffer.m_nViewMatrix * m_kCurrentBuffer.m_nProjectionMatrix);

            m_nCurrentFrustum.Create(nCameraMatrix, m_kCurrentBuffer.m_nProjectionMatrix);

            LIGHT kLight;
            m_aVisibleLights.clear();
            m_pSceneManager->ListComponentsEntities({ L"transform", L"light" }, [&](const GEKENTITYID &nEntityID)->void
            {
                GEKVALUE kValue;
                m_pSceneManager->GetProperty(nEntityID, L"transform", L"position", kValue);
                kLight.m_nPosition = (m_kCurrentBuffer.m_nViewMatrix * float4(kValue.GetFloat3(), 1.0f));

                m_pSceneManager->GetProperty(nEntityID, L"light", L"range", kValue);
                kLight.m_nInvRange = (1.0f / (kLight.m_nRange = kValue.GetFloat()));

                m_pSceneManager->GetProperty(nEntityID, L"light", L"color", kValue);
                kLight.m_nColor = kValue.GetFloat3();

                m_aVisibleLights.push_back(kLight);
            });

            CGEKObservable::SendEvent(TGEKEvent<IGEKRenderObserver>(std::bind(&IGEKRenderObserver::OnCullScene, std::placeholders::_1)));

            m_spEngineBuffer->Update((void *)&m_kCurrentBuffer);
            m_pVideoSystem->GetImmediateContext()->GetVertexSystem()->SetConstantBuffer(0, m_spEngineBuffer);
            m_pVideoSystem->GetImmediateContext()->GetGeometrySystem()->SetConstantBuffer(0, m_spEngineBuffer);
            m_pVideoSystem->GetImmediateContext()->GetPixelSystem()->SetConstantBuffer(0, m_spEngineBuffer);
            m_pVideoSystem->GetImmediateContext()->GetComputeSystem()->SetConstantBuffer(0, m_spEngineBuffer);

            m_pCurrentPass = &m_aPasses[kPass.GetRawString()];
            for (auto &pFilter : m_pCurrentPass->m_aFilters)
            {
                m_pCurrentFilter = pFilter;
                pFilter->Draw();
                m_pCurrentFilter = nullptr;
            }

            m_pCurrentPass = nullptr;
            CGEKObservable::SendEvent(TGEKEvent<IGEKRenderObserver>(std::bind(&IGEKRenderObserver::OnPostRender, std::placeholders::_1)));
        }
    });

    m_pVideoSystem->SetDefaultTargets();
    m_pVideoSystem->GetImmediateContext()->SetRenderStates(m_spRenderStates);
    m_pVideoSystem->GetImmediateContext()->SetBlendStates(float4(1.0f), 0xFFFFFFFF, m_spBlendStates);
    m_pVideoSystem->GetImmediateContext()->SetDepthStates(0x0, m_spDepthStates);
    m_pVideoSystem->GetImmediateContext()->GetComputeSystem()->SetProgram(nullptr);
    m_pVideoSystem->GetImmediateContext()->GetVertexSystem()->SetProgram(m_spVertexProgram);
    m_pVideoSystem->GetImmediateContext()->GetVertexSystem()->SetConstantBuffer(1, m_spOrthoBuffer);
    m_pVideoSystem->GetImmediateContext()->GetGeometrySystem()->SetProgram(nullptr);
    m_pVideoSystem->GetImmediateContext()->GetPixelSystem()->SetProgram(m_spPixelProgram);
    m_pVideoSystem->GetImmediateContext()->GetPixelSystem()->SetResource(0, m_spScreenBuffer);
    SetResource(m_pVideoSystem->GetImmediateContext()->GetPixelSystem(), 1, nullptr);
    m_pVideoSystem->GetImmediateContext()->SetVertexBuffer(0, 0, m_spVertexBuffer);
    m_pVideoSystem->GetImmediateContext()->SetIndexBuffer(0, m_spIndexBuffer);
    m_pVideoSystem->GetImmediateContext()->SetPrimitiveType(GEKVIDEO::PRIMITIVE::TRIANGLELIST);
    m_pVideoSystem->GetImmediateContext()->DrawIndexedPrimitive(6, 0, 0);

    IGEKInterfaceSystem *pInterfaceSystem = GetContext()->GetCachedClass<IGEKInterfaceSystem>(CLSID_GEKInterfaceSystem);
    if (pInterfaceSystem)
    {
        static UINT64 nLastTime = 0;
        static UINT32 nNumFrames = 0;
        static UINT32 nFPS = 0;
        nNumFrames++;
        UINT64 nCurrentTime = GetTickCount64();
        if (nCurrentTime - nLastTime > 1000)
        {
            nLastTime = nCurrentTime;
            nFPS = nNumFrames;
            nNumFrames = 0;
        }

        GEKVIDEO::RECT<float> kRect = { 0.0f, 0.0f, 640.0f, 480.0f, };
        pInterfaceSystem->Print(L"Arial", 32.0f, 0xFF0099FF, kRect, kRect, L"FPS: %d", nFPS);
    }

    m_pVideoSystem->GetImmediateContext()->ClearResources();

    m_pVideoSystem->Present(true);

    while (!m_pVideoSystem->IsEventSet(m_spFrameEvent))
    {
        Sleep(0);
    };

    m_pVideoSystem->SetEvent(m_spFrameEvent);
}

STDMETHODIMP_(float2) CGEKRenderSystem::GetScreenSize(void) const
{
    REQUIRE_RETURN(m_spScreenBuffer, 0.0f);
    return float2(float(m_spScreenBuffer->GetXSize()), float(m_spScreenBuffer->GetYSize()));
}