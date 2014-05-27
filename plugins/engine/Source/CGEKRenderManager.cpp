#include "CGEKRenderManager.h"
#include "IGEKRenderFilter.h"
#include "CGEKProperties.h"
#include <windowsx.h>
#include <algorithm>
#include <atlpath.h>
#include <ppl.h>

#include "GEKEngineCLSIDs.h"
#include "GEKSystemCLSIDs.h"

DECLARE_INTERFACE_IID_(IGEKMaterial, IUnknown, "819CA201-F652-4183-B29D-BB71BB15810E")
{
    STDMETHOD_(LPCWSTR, GetPass)            (THIS) PURE;
    STDMETHOD_(void, Enable)                (THIS_ CGEKRenderManager *pManager, IGEKVideoSystem *pSystem) PURE;
};

class CGEKMaterial : public CGEKUnknown
                   , public IGEKMaterial
                   , public CGEKRenderStates
                   , public CGEKBlendStates
{
private:
    CStringW m_strPass;
    CComPtr<IUnknown> m_spAlbedoMap;
    CComPtr<IUnknown> m_spNormalMap;
    CComPtr<IUnknown> m_spInfoMap;

public:
    DECLARE_UNKNOWN(CGEKMaterial);
    CGEKMaterial(LPCWSTR pPass, IUnknown *pAlbedoMap, IUnknown *pNormalMap, IUnknown *pInfoMap)
        : m_strPass(pPass)
        , m_spAlbedoMap(pAlbedoMap)
        , m_spNormalMap(pNormalMap)
        , m_spInfoMap(pInfoMap)
    {
    }

    ~CGEKMaterial(void)
    {
    }

    // IGEKMaterial
    STDMETHODIMP_(LPCWSTR) GetPass(void)
    {
        return m_strPass.GetString();
    }

    STDMETHODIMP_(void) Enable(CGEKRenderManager *pManager, IGEKVideoSystem *pSystem)
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

DECLARE_INTERFACE_IID_(IGEKWebSurface, IUnknown, "47015E42-8CFD-43BD-A10A-737A778A5122")
{
    STDMETHOD_(IGEKVideoTexture *, GetTexture)          (THIS) PURE;
    STDMETHOD_(void, Update)                            (THIS) PURE;
};

class CGEKWebSurface : public CGEKUnknown
                     , public Awesomium::Surface
                     , public IGEKWebSurface
{
private:
    
    IGEKVideoSystem *m_pVideoSystem;
    CComPtr<IGEKVideoTexture> m_spTexture;
    std::vector<UINT8> m_aBuffer;
    UINT32 m_nXSize;
    UINT32 m_nYSize;
    UINT32 m_nPitch;
    bool m_bDirty;

public:
    DECLARE_UNKNOWN(CGEKWebSurface);
    CGEKWebSurface(IGEKVideoSystem *pVideoSystem, int nXSize, int nYSize)
        : m_pVideoSystem(pVideoSystem)
        , m_nXSize(nXSize)
        , m_nYSize(nYSize)
        , m_nPitch(nXSize * 4)
        , m_bDirty(false)
    {
        m_pVideoSystem->CreateTexture(nXSize, nYSize, 1, GEKVIDEO::DATA::BGRA_UINT8, GEKVIDEO::TEXTURE::RESOURCE, &m_spTexture);
        m_aBuffer.resize(nXSize * nYSize * 4);
    }

    ~CGEKWebSurface(void)
    {
    }

    STDMETHODIMP_(IGEKVideoTexture *) GetTexture(void)
    {
        return m_spTexture;
    }

    STDMETHODIMP_(void) Update(void)
    {
        if (m_bDirty)
        {
            m_bDirty = false;
            RECT kRect =
            {
                0, 0, m_nXSize, m_nYSize,
            };

            m_pVideoSystem->UpdateTexture(m_spTexture, &m_aBuffer[0], m_nPitch, &kRect);
        }
    }

    void Paint(UINT8 *pSourceBuffer, int nSourcePitch, const Awesomium::Rect &nSourceRect, const Awesomium::Rect &nDestRect)
    {
        UINT8 *pDestBuffer = &m_aBuffer[0];
        for (int nRow = 0; nRow < nDestRect.height; nRow++)
        {
            memcpy(pDestBuffer + (nRow + nDestRect.y) * m_nPitch + (nDestRect.x * 4),
                pSourceBuffer + (nRow + nSourceRect.y) * nSourcePitch + (nSourceRect.x * 4),
                nDestRect.width * 4);
        }

        m_bDirty = true;
    }

    void Scroll(int nDX, int nDY, const Awesomium::Rect &nClipRect)
    {
        if (abs(nDX) >= nClipRect.width || abs(nDY) >= nClipRect.height)
        {
            return;
        }

        UINT8 *pDestBuffer = &m_aBuffer[0];
        if (nDX < 0 && nDY == 0)
        {
            std::vector<UINT8> aTempBuffer((nClipRect.width + nDX) * 4);
            UINT8 *pTempBuffer = &aTempBuffer[0];

            for (INT32 nIndex = 0; nIndex < nClipRect.height; nIndex++)
            {
                memcpy(pTempBuffer, (pDestBuffer + (nIndex + nClipRect.y) * m_nPitch + (nClipRect.x - nDX) * 4), ((nClipRect.width + nDX) * 4));
                memcpy((pDestBuffer + (nIndex + nClipRect.y) * m_nPitch + (nClipRect.x) * 4), pTempBuffer, ((nClipRect.width + nDX) * 4));
            }
        }
        else if (nDX > 0 && nDY == 0)
        {
            std::vector<UINT8> aTempBuffer((nClipRect.width + nDX) * 4);
            UINT8 *pTempBuffer = &aTempBuffer[0];

            for (INT32 nIndex = 0; nIndex < nClipRect.height; nIndex++)
            {
                memcpy(pTempBuffer, (pDestBuffer + (nIndex + nClipRect.y) * m_nPitch + (nClipRect.x) * 4), ((nClipRect.width - nDX) * 4));
                memcpy((pDestBuffer + (nIndex + nClipRect.y) * m_nPitch + (nClipRect.x + nDX) * 4), pTempBuffer, ((nClipRect.width - nDX) * 4));
            }
        }
        else if (nDY < 0 && nDX == 0)
        {
            for (INT32 nIndex = 0; nIndex < (nClipRect.height + nDY); nIndex++)
            {
                memcpy((pDestBuffer + (nIndex + nClipRect.y) * m_nPitch + (nClipRect.x * 4)),
                    (pDestBuffer + (nIndex + nClipRect.y - nDY) * m_nPitch + (nClipRect.x * 4)),
                    (nClipRect.width * 4));
            }
        }
        else if (nDY > 0 && nDX == 0)
        {
            for (INT32 nIndex = (nClipRect.height - 1); nIndex >= nDY; nIndex--)
            {
                memcpy((pDestBuffer + (nIndex + nClipRect.y) * m_nPitch + (nClipRect.x * 4)),
                    (pDestBuffer + (nIndex + nClipRect.y - nDY) * m_nPitch + (nClipRect.x * 4)),
                    (nClipRect.width * 4));
            }
        }

        m_bDirty = true;
    }
};

BEGIN_INTERFACE_LIST(CGEKWebSurface)
    INTERFACE_LIST_ENTRY_COM(IGEKWebSurface)
END_INTERFACE_LIST_UNKNOWN

DECLARE_INTERFACE_IID_(IGEKWebView, IUnknown, "646FA93A-D554-4423-B3E0-2C8C6F368976")
{
    STDMETHOD_(Awesomium::WebView *, GetView)   (THIS) PURE;
};

class CGEKWebView : public CGEKUnknown
                  , public IGEKWebView
{
private:
    Awesomium::WebView *m_pView;

public:
    DECLARE_UNKNOWN(CGEKWebView);
    CGEKWebView(Awesomium::WebView *pView)
        : m_pView(pView)
    {
    }
    
    STDMETHODIMP_(Awesomium::WebView *) GetView(void)
    {
        return m_pView;
    }
};

BEGIN_INTERFACE_LIST(CGEKWebView)
    INTERFACE_LIST_ENTRY_COM(IGEKWebView)
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

BEGIN_INTERFACE_LIST(CGEKRenderManager)
    INTERFACE_LIST_ENTRY_COM(IGEKSystemObserver)
    INTERFACE_LIST_ENTRY_COM(IGEKVideoObserver)
    INTERFACE_LIST_ENTRY_COM(IGEKSceneObserver)
    INTERFACE_LIST_ENTRY_COM(IGEKRenderManager)
    INTERFACE_LIST_ENTRY_COM(IGEKProgramManager)
    INTERFACE_LIST_ENTRY_COM(IGEKMaterialManager)
    INTERFACE_LIST_ENTRY_COM(IGEKModelManager)
    INTERFACE_LIST_ENTRY_COM(IGEKViewManager)
END_INTERFACE_LIST_UNKNOWN

REGISTER_CLASS(CGEKRenderManager)

CGEKRenderManager::CGEKRenderManager(void)
    : m_pVideoSystem(nullptr)
    , m_pSystem(nullptr)
    , m_pWebCore(nullptr)
    , m_pWebSession(nullptr)
    , m_pViewer(nullptr)
    , m_pCurrentPass(nullptr)
    , m_pCurrentFilter(nullptr)
    , m_nNumLightInstances(254)
{
}

CGEKRenderManager::~CGEKRenderManager(void)
{
}

STDMETHODIMP_(void) CGEKRenderManager::OnEvent(UINT32 nMessage, WPARAM wParam, LPARAM lParam, LRESULT &nResult)
{
    switch (nMessage)
    {
    case WM_KEYDOWN:
    case WM_KEYUP:
    case WM_CHAR:
        if (true)
        {
            Awesomium::WebKeyboardEvent kEvent(nMessage, wParam, lParam);
            for (auto pView : m_aGUIViews)
            {
                pView->InjectKeyboardEvent(kEvent);
            }
        }

        nResult = 1;
        break;

    case WM_LBUTTONDOWN:
    case WM_RBUTTONDOWN:
    case WM_MBUTTONDOWN:
        for (auto pView : m_aGUIViews)
        {
            switch (nMessage)
            {
            case WM_LBUTTONDOWN:
                pView->InjectMouseDown(Awesomium::kMouseButton_Left);
                break;

            case WM_RBUTTONDOWN:
                pView->InjectMouseDown(Awesomium::kMouseButton_Right);
                break;

            case WM_MBUTTONDOWN:
                pView->InjectMouseDown(Awesomium::kMouseButton_Middle);
                break;
            };
        }

        nResult = 1;
        break;

    case WM_LBUTTONUP:
    case WM_RBUTTONUP:
    case WM_MBUTTONUP:
        for (auto pView : m_aGUIViews)
        {
            switch (nMessage)
            {
            case WM_LBUTTONUP:
                pView->InjectMouseUp(Awesomium::kMouseButton_Left);
                break;

            case WM_RBUTTONUP:
                pView->InjectMouseUp(Awesomium::kMouseButton_Right);
                break;

            case WM_MBUTTONUP:
                pView->InjectMouseUp(Awesomium::kMouseButton_Middle);
                break;
            };
        }

        nResult = 1;
        break;

    case WM_MOUSEMOVE:
        if (true)
        {
            INT32 nXPos = GET_X_LPARAM(lParam);
            INT32 nYPos = GET_Y_LPARAM(lParam);
            for (auto pView : m_aGUIViews)
            {
                pView->InjectMouseMove(nXPos, nYPos);
            }
        }

        nResult = 1;
        break;

    case WM_MOUSEWHEEL:
        if (true)
        {
            INT32 nDelta = GET_WHEEL_DELTA_WPARAM(wParam);
            for (auto pView : m_aGUIViews)
            {
                pView->InjectMouseWheel(nDelta, 0);
            }
        }

        nResult = 1;
        break;
    };
}

STDMETHODIMP_(void) CGEKRenderManager::OnPreReset(void)
{
    for (auto pView : m_aGUIViews)
    {
        pView->Resize(m_pSystem->GetXSize(), m_pSystem->GetYSize());
    }
}

STDMETHODIMP CGEKRenderManager::OnPostReset(void)
{
    return S_OK;
}

STDMETHODIMP CGEKRenderManager::OnLoadEnd(HRESULT hRetVal)
{
    if (SUCCEEDED(hRetVal))
    {
        m_pVideoSystem->SetEvent(m_spFrameEvent);
    }

    return hRetVal;
}

STDMETHODIMP CGEKRenderManager::Initialize(void)
{
    GEKFUNCTION(nullptr);
    HRESULT hRetVal = GetContext()->AddCachedClass(CLSID_GEKRenderManager, GetUnknown());
    if (SUCCEEDED(hRetVal))
    {
        hRetVal = E_FAIL;
        m_pSystem = GetContext()->GetCachedClass<IGEKSystem>(CLSID_GEKSystem);
        m_pVideoSystem = GetContext()->GetCachedClass<IGEKVideoSystem>(CLSID_GEKVideoSystem);
        if (m_pSystem != nullptr && m_pVideoSystem != nullptr)
        {
            m_pWebCore = Awesomium::WebCore::Initialize(Awesomium::WebConfig());
            GEKRESULT(m_pWebCore, L"Unable to create Awesomium WebCore instance");
            if (m_pWebCore != nullptr)
            {
                m_pWebSession = m_pWebCore->CreateWebSession(Awesomium::WSLit(""), Awesomium::WebPreferences());
                GEKRESULT(m_pWebCore, L"Unable to create Awesomium WebSession instance");
                if (m_pWebSession != nullptr)
                {
                    m_pWebSession->AddDataSource(Awesomium::WSLit("Engine"), this);
                    m_pWebCore->set_surface_factory(this);
                    hRetVal = S_OK;
                }
            }
        }
    }

    if (SUCCEEDED(hRetVal))
    {
        hRetVal = GetContext()->AddCachedObserver(CLSID_GEKSystem, (IGEKSystemObserver *)GetUnknown());
    }

    if (SUCCEEDED(hRetVal))
    {
        hRetVal = GetContext()->AddCachedObserver(CLSID_GEKVideoSystem, (IGEKVideoObserver *)GetUnknown());
    }

    if (SUCCEEDED(hRetVal))
    {
        hRetVal = GetContext()->AddCachedObserver(CLSID_GEKPopulationManager, (IGEKSceneObserver *)GetUnknown());
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
        float2 aVertices[8] =
        {
            float2(0.0f, 0.0f), float2(-1.0f, 1.0f),
            float2(1.0f, 0.0f), float2(1.0f, 1.0f),
            float2(1.0f, 1.0f), float2(1.0f, -1.0f),
            float2(0.0f, 1.0f), float2(-1.0f, -1.0f),
        };

        hRetVal = m_pVideoSystem->CreateBuffer((sizeof(float2)* 2), 4, GEKVIDEO::BUFFER::VERTEX_BUFFER | GEKVIDEO::BUFFER::STATIC, &m_spVertexBuffer, aVertices);
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
        hRetVal = m_pVideoSystem->CreateEvent(&m_spFrameEvent);
        GEKRESULT(SUCCEEDED(hRetVal), L"Call to CreateEvent failed: 0x%08X", hRetVal);
    }

    if (SUCCEEDED(hRetVal))
    {
        GetContext()->CreateEachType(CLSID_GEKFactoryType, [&](IUnknown *pObject) -> HRESULT
        {
            CComQIPtr<IGEKFactory> spFactory(pObject);
            if (spFactory)
            {
                m_aFactories.push_back(spFactory);
            }

            return S_OK;
        });
    }

    return hRetVal;
}

STDMETHODIMP_(void) CGEKRenderManager::Destroy(void)
{
    Free();
    if (m_pWebSession != nullptr)
    {
        m_pWebSession->Release();
        m_pWebSession = nullptr;
    }

    Awesomium::WebCore::Shutdown();
    m_pWebCore = nullptr;

    GetContext()->RemoveCachedObserver(CLSID_GEKPopulationManager, (IGEKSceneObserver *)GetUnknown());
    GetContext()->RemoveCachedObserver(CLSID_GEKVideoSystem, (IGEKVideoObserver *)GetUnknown());
    GetContext()->RemoveCachedObserver(CLSID_GEKSystem, (IGEKSystemObserver *)GetUnknown());
    GetContext()->RemoveCachedClass(CLSID_GEKRenderManager);
}

STDMETHODIMP_(void) CGEKRenderManager::Free(void)
{
    m_pViewer = nullptr;
    m_pCurrentPass = nullptr;
    for (auto &kPair : m_aWebViews)
    {
        kPair.second->Destroy();
    }

    m_aWebViews.clear();
    m_aGUIViews.clear();
    m_aTextures.clear();
    m_aMaterials.clear();
    m_aModels.clear();
    m_aFilters.clear();
    m_aPasses.clear();
}

HRESULT CGEKRenderManager::LoadPass(LPCWSTR pName)
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
            hRetVal = E_INVALID;
            CLibXMLNode kPassNode = kDocument.GetRoot();
            if (kPassNode)
            {
                hRetVal = S_OK;
                PASS &kPassData = m_aPasses[pName];
                CLibXMLNode kRequiresNode = kPassNode.FirstChildElement(L"requires");
                if (kRequiresNode)
                {
                    CLibXMLNode kPassNode = kRequiresNode.FirstChildElement(L"pass");
                    while (kPassNode)
                    {
                        CStringW strRequiredPass = kPassNode.GetAttribute(L"source");
                        hRetVal = LoadPass(strRequiredPass);
                        if (SUCCEEDED(hRetVal))
                        {
                            kPassData.m_aRequiredPasses.push_back(&m_aPasses[strRequiredPass]);
                            kPassNode = kPassNode.NextSiblingElement(L"pass");
                        }
                        else
                        {
                            break;
                        }
                    };
                }

                if (SUCCEEDED(hRetVal))
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
            }
        }
    }

    return hRetVal;
}

STDMETHODIMP CGEKRenderManager::LoadResource(LPCWSTR pName, IUnknown **ppResource)
{
    GEKFUNCTION(L"Name(%s)", pName);
    REQUIRE_RETURN(pName, E_INVALIDARG);
    REQUIRE_RETURN(ppResource, E_INVALIDARG);

    HRESULT hRetVal = E_FAIL;
    auto pIterator = m_aTextures.find(pName);
    if (pIterator != m_aTextures.end())
    {
        hRetVal = ((*pIterator).second)->QueryInterface(IID_PPV_ARGS(ppResource));
    }
    else
    {
        CComPtr<IUnknown> spTexture;
        if (pName[0] == L'*')
        {
            int nPosition = 0;
            CStringW strName = &pName[1];
            CStringW strType = strName.Tokenize(L":", nPosition);
            if (strType.CompareNoCase(L"browser") == 0)
            {
                CStringW strBrowserName = strName.Tokenize(L":", nPosition);

                CLibXMLDoc kDocument;
                hRetVal = kDocument.Load(L"%root%\\data\\browsers\\" + strBrowserName + L".xml");
                if (SUCCEEDED(hRetVal))
                {
                    hRetVal = E_INVALIDARG;
                    CLibXMLNode kBrowerNode = kDocument.GetRoot();
                    if (kBrowerNode && kBrowerNode.HasAttribute(L"size"))
                    {
                        nPosition = 0;
                        CStringW strSize = kBrowerNode.GetAttribute(L"size");
                        CStringW strXSize = strSize.Tokenize(L",", nPosition);
                        CStringW strYSize = strSize.Tokenize(L",", nPosition);
                        UINT32 nXSize = m_pSystem->EvaluateValue(strXSize);
                        UINT32 nYSize = m_pSystem->EvaluateValue(strYSize);

                        CLibXMLNode kSourceNode = kBrowerNode.FirstChildElement(L"source");
                        if (kSourceNode && kSourceNode.HasAttribute(L"url"))
                        {
                            Awesomium::WebView *pView = m_pWebCore->CreateWebView(nXSize, nYSize, m_pWebSession);
                            if (pView != nullptr)
                            {
                                pView->set_view_listener(this);

                                bool bGUI = false;
                                CLibXMLNode kFlagsNode = kBrowerNode.FirstChildElement(L"flags");
                                if (kFlagsNode)
                                {
                                    if (kFlagsNode.HasAttribute(L"javascript") && StrToBoolean(kFlagsNode.GetAttribute(L"javascript")))
                                    {
                                        pView->set_js_method_handler(this);
                                        Awesomium::JSValue kResult = pView->CreateGlobalJavascriptObject(Awesomium::WSLit("Engine"));
                                        if (kResult.IsObject())
                                        {
                                            Awesomium::JSObject &kEngineObject = kResult.ToObject();

                                            kEngineObject.SetCustomMethod(Awesomium::WSLit("NewGame"), false);
                                            kEngineObject.SetCustomMethod(Awesomium::WSLit("Quit"), false);
                                            kEngineObject.SetCustomMethod(Awesomium::WSLit("SetResolution"), false);

                                            kEngineObject.SetCustomMethod(Awesomium::WSLit("GetResolutions"), true);
                                            kEngineObject.SetCustomMethod(Awesomium::WSLit("GetValue"), true);
                                        }
                                    }

                                    if (kFlagsNode.HasAttribute(L"transparent") && StrToBoolean(kFlagsNode.GetAttribute(L"transparent")))
                                    {
                                        pView->SetTransparent(true);
                                    }

                                    if (kFlagsNode.HasAttribute(L"gui"))
                                    {
                                        bGUI = StrToBoolean(kFlagsNode.GetAttribute(L"gui"));
                                    }
                                }

                                CStringW strURL = kSourceNode.GetAttribute(L"url");
                                CStringA strURLUTF8 = CW2A(strURL, CP_UTF8);
                                pView->LoadURL(Awesomium::WebURL(Awesomium::WSLit(strURLUTF8)));
                                CComPtr<CGEKWebView> spWebView(new CGEKWebView(pView));
                                GEKRESULT(spWebView, L"Call to new failed to allocate instance");
                                if (spWebView)
                                {
                                    m_aWebViews[pName] = pView;
                                    hRetVal = spWebView->QueryInterface(IID_PPV_ARGS(&spTexture));
                                    if (bGUI)
                                    {
                                        m_aGUIViews.push_back(pView);
                                    }
                                }
                            }
                        }
                    }
                }
            }
            else if (strType.CompareNoCase(L"color") == 0)
            {
                CStringW strColor = strName.Tokenize(L":", nPosition);
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
                else
                {
                    OutputDebugStringW(FormatString(L"-> Unable to create texture: %s\r\n", pName));
                }
            }
            else
            {
                OutputDebugStringW(FormatString(L"-> Unknown texture specifier: %s\r\n", pName));
            }
        }
        else
        {
            CComPtr<IGEKVideoTexture> spFileTexture;
            hRetVal = m_pVideoSystem->LoadTexture(FormatString(L"%%root%%\\data\\textures\\%s", pName), &spFileTexture);
            if (spFileTexture)
            {
                spTexture = spFileTexture;
            }
            else
            {
                OutputDebugStringW(FormatString(L"-> Unable to load texture: %s\r\n", pName));
            }
        }

        if (spTexture)
        {
            m_aTextures[pName] = spTexture;
            hRetVal = spTexture->QueryInterface(IID_PPV_ARGS(ppResource));
        }
    }

    return hRetVal;
}

STDMETHODIMP_(void) CGEKRenderManager::SetResource(IGEKVideoContextSystem *pSystem, UINT32 nStage, IUnknown *pResource)
{
    CComPtr<IUnknown> spResource(pResource);
    CComQIPtr<IGEKWebView> spWebView(pResource);
    if (spWebView)
    {
        auto pIterator = m_aWebSurfaces.find(spWebView->GetView());
        if (pIterator != m_aWebSurfaces.end())
        {
            CComQIPtr<IGEKWebSurface> spWebSurface((*pIterator).second);
            if (spWebSurface)
            {
                spResource = spWebSurface->GetTexture();
            }
        }
    }

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

STDMETHODIMP CGEKRenderManager::GetBuffer(LPCWSTR pName, IUnknown **ppResource)
{
    REQUIRE_RETURN(ppResource, E_INVALIDARG);

    HRESULT hRetVal = E_FAIL;
    if (_wcsicmp(pName, L"Screen") == 0)
    {
        CComPtr<IGEKVideoTexture> spTexture;
        hRetVal = m_pVideoSystem->GetDefaultRenderTarget(&spTexture);
        if (spTexture)
        {
            hRetVal = spTexture->QueryInterface(IID_PPV_ARGS(ppResource));
        }
    }
    else
    {
        int nPosition = 0;
        CStringW strName = pName;
        GEKHASH nFilter = strName.Tokenize(L".", nPosition);
        CStringW strSource = strName.Tokenize(L".", nPosition);
        for (auto &kPair : m_aFilters)
        {
            if (kPair.first == nFilter)
            {
                hRetVal = kPair.second->GetBuffer(strSource, ppResource);
                break;
            }
        }
    }

    return hRetVal;
}

STDMETHODIMP CGEKRenderManager::GetDepthBuffer(LPCWSTR pSource, IUnknown **ppBuffer)
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

STDMETHODIMP CGEKRenderManager::LoadMaterial(LPCWSTR pName, IUnknown **ppMaterial)
{
    GEKFUNCTION(L"Name(%s)", pName);
    REQUIRE_RETURN(ppMaterial, E_INVALIDARG);

    HRESULT hRetVal = E_FAIL;
    auto pIterator = m_aMaterials.find(pName);
    if (pIterator != m_aMaterials.end())
    {
        hRetVal = ((*pIterator).second)->QueryInterface(IID_PPV_ARGS(ppMaterial));
    }
    else
    {
        CLibXMLDoc kDocument;
        hRetVal = kDocument.Load(FormatString(L"%%root%%\\data\\materials\\%s.xml", pName));
        if (SUCCEEDED(hRetVal))
        {
            hRetVal = E_INVALIDARG;
            CLibXMLNode kMaterialNode = kDocument.GetRoot();
            if (kMaterialNode)
            {
                if (kMaterialNode.HasAttribute(L"pass"))
                {
                    CStringW strPass = kMaterialNode.GetAttribute(L"pass");
                    hRetVal = LoadPass(strPass);
                    if (SUCCEEDED(hRetVal))
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

                        CComPtr<CGEKMaterial> spMaterial(new CGEKMaterial(strPass, spAlbedoMap, spNormalMap, spInfoMap));
                        GEKRESULT(spMaterial, L"Call to new failed to allocate instance");
                        if (spMaterial)
                        {
                            spMaterial->CGEKRenderStates::Load(m_pVideoSystem, kMaterialNode.FirstChildElement(L"render"));
                            spMaterial->CGEKBlendStates::Load(m_pVideoSystem, kMaterialNode.FirstChildElement(L"blend"));
                            spMaterial->QueryInterface(IID_PPV_ARGS(&m_aMaterials[pName]));
                            hRetVal = spMaterial->QueryInterface(IID_PPV_ARGS(ppMaterial));
                        }
                    }
                }
            }
        }
    }

    if (!(*ppMaterial))
    {
        OutputDebugStringW(FormatString(L"> Unable to load material: %s\r\n", pName));
        auto pIterator = m_aMaterials.find(L"*default");
        if (pIterator != m_aMaterials.end())
        {
            hRetVal = ((*pIterator).second)->QueryInterface(IID_PPV_ARGS(ppMaterial));
        }
        else
        {
            hRetVal = LoadPass(L"Opaque");
            if (SUCCEEDED(hRetVal))
            {
                CComPtr<IUnknown> spAlbedoMap;
                LoadResource(L"*color:1,1,1,1", &spAlbedoMap);

                CComPtr<IUnknown> spNormalMap;
                LoadResource(L"*color:0.5,0.5,1,1", &spNormalMap);

                CComPtr<IUnknown> spInfoMap;
                LoadResource(L"*color:0.5,0,0,0", &spInfoMap);

                CComPtr<CGEKMaterial> spMaterial(new CGEKMaterial(L"Opaque", spAlbedoMap, spNormalMap, spInfoMap));
                GEKRESULT(spMaterial, L"Call to new failed to allocate instance");
                if (spMaterial)
                {
                    CLibXMLNode kBlankNode(nullptr);
                    spMaterial->CGEKRenderStates::Load(m_pVideoSystem, kBlankNode);
                    spMaterial->CGEKBlendStates::Load(m_pVideoSystem, kBlankNode);
                    spMaterial->QueryInterface(IID_PPV_ARGS(&m_aMaterials[L"*default"]));
                    hRetVal = spMaterial->QueryInterface(IID_PPV_ARGS(ppMaterial));
                }
            }
        }
    }

    return hRetVal;
}

STDMETHODIMP CGEKRenderManager::PrepareMaterial(IUnknown *pMaterial)
{
    REQUIRE_RETURN(pMaterial, E_INVALIDARG);

    HRESULT hRetVal = E_FAIL;
    CComQIPtr<IGEKMaterial> spMaterial(pMaterial);
    if (spMaterial)
    {
        auto pIterator = m_aPasses.find(spMaterial->GetPass());
        if (pIterator != m_aPasses.end())
        {
            hRetVal = S_OK;
            m_aCurrentPasses[&(*pIterator).second] = 1;
        }
    }

    return hRetVal;
}

STDMETHODIMP_(bool) CGEKRenderManager::EnableMaterial(IUnknown *pMaterial)
{
    REQUIRE_RETURN(pMaterial, false);

    bool bReturn = false;
    if (m_pCurrentPass != nullptr)
    {
        CComQIPtr<IGEKMaterial> spMaterial(pMaterial);
        if (spMaterial)
        {
            auto pIterator = m_aPasses.find(spMaterial->GetPass());
            if (pIterator != m_aPasses.end() && m_pCurrentPass == &(*pIterator).second)
            {
                bReturn = true;
                spMaterial->Enable(this, m_pVideoSystem);
            }
        }
    }

    return bReturn;
}

STDMETHODIMP CGEKRenderManager::LoadProgram(LPCWSTR pName, IUnknown **ppProgram)
{
    GEKFUNCTION(L"Name(%s)", pName);
    REQUIRE_RETURN(ppProgram, E_INVALIDARG);

    HRESULT hRetVal = E_FAIL;
    auto pIterator = m_aPrograms.find(pName);
    if (pIterator != m_aPrograms.end())
    {
        hRetVal = ((*pIterator).second)->QueryInterface(IID_PPV_ARGS(ppProgram));
    }
    else
    {
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
                                        GEKRESULT(spProgram, L"Call to new failed to allocate instance");
                                        if (spProgram)
                                        {
                                            hRetVal = spProgram->QueryInterface(IID_PPV_ARGS(ppProgram));
                                            if (*ppProgram)
                                            {
                                                m_aPrograms[pName] = (*ppProgram);
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
    }

    return hRetVal;
}

STDMETHODIMP_(void) CGEKRenderManager::EnableProgram(IUnknown *pProgram)
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

STDMETHODIMP CGEKRenderManager::LoadCollision(LPCWSTR pName, LPCWSTR pParams, IGEKCollision **ppCollision)
{
    GEKFUNCTION(L"Name(%s), Params(%s)", pName, pParams);
    REQUIRE_RETURN(ppCollision, E_INVALIDARG);
    REQUIRE_RETURN(pName, E_INVALIDARG);
    REQUIRE_RETURN(pParams, E_INVALIDARG);

    std::vector<UINT8> aBuffer;
    HRESULT hRetVal = GEKLoadFromFile(FormatString(L"%%root%%\\data\\models\\%s.collision.gek", pName), aBuffer);
    if (SUCCEEDED(hRetVal))
    {
        for (auto &spFactory : m_aFactories)
        {
            hRetVal = spFactory->Create(&aBuffer[0], IID_PPV_ARGS(ppCollision));
            if (*ppCollision)
            {
                CComQIPtr<IGEKResource> spResource(*ppCollision);
                if (spResource)
                {
                    hRetVal = spResource->Load(&aBuffer[0], pParams);
                    if (SUCCEEDED(hRetVal))
                    {
                        break;
                    }
                }
                else
                {
                    hRetVal = E_INVALID;
                }
            }
        }
    }

    return hRetVal;
}

STDMETHODIMP CGEKRenderManager::LoadModel(LPCWSTR pName, LPCWSTR pParams, IUnknown **ppModel)
{
    GEKFUNCTION(L"Name(%s), Params(%s)", pName, pParams);
    REQUIRE_RETURN(ppModel, E_INVALIDARG);
    REQUIRE_RETURN(pName, E_INVALIDARG);
    REQUIRE_RETURN(pParams, E_INVALIDARG);

    HRESULT hRetVal = E_FAIL;
    auto pIterator = m_aModels.find(FormatString(L"%s|%s", pName, pParams));
    if (pIterator != m_aModels.end())
    {
        hRetVal = ((*pIterator).second)->QueryInterface(IID_PPV_ARGS(ppModel));
    }
    else
    {
        std::vector<UINT8> aBuffer;
        hRetVal = GEKLoadFromFile(FormatString(L"%%root%%\\data\\models\\%s.model.gek", pName), aBuffer);
        if (SUCCEEDED(hRetVal))
        {
            for (auto &spFactory : m_aFactories)
            {
                CComPtr<IGEKModel> spModel;
                hRetVal = spFactory->Create(&aBuffer[0], IID_PPV_ARGS(&spModel));
                if (spModel)
                {
                    CComQIPtr<IGEKResource> spResource(spModel);
                    if (spResource)
                    {
                        hRetVal = spResource->Load(&aBuffer[0], pParams);
                        if (SUCCEEDED(hRetVal))
                        {
                            m_aModels[FormatString(L"%s|%s", pName, pParams)] = spModel;
                            hRetVal = spModel->QueryInterface(IID_PPV_ARGS(ppModel));
                            break;
                        }
                    }
                    else
                    {
                        hRetVal = E_INVALID;
                    }
                }
            }
        }
    }

    return hRetVal;
}

STDMETHODIMP CGEKRenderManager::EnablePass(LPCWSTR pName, INT32 nPriority)
{
    REQUIRE_RETURN(pName, E_INVALIDARG);

    HRESULT hRetVal = LoadPass(pName);
    if (SUCCEEDED(hRetVal))
    {
        auto pIterator = m_aPasses.find(pName);
        if (pIterator != m_aPasses.end())
        {
            m_aCurrentPasses[&(*pIterator).second] = nPriority;
        }
    }

    return hRetVal;
}

STDMETHODIMP CGEKRenderManager::SetViewer(IGEKEntity *pEntity)
{
    HRESULT hRetVal = E_INVALID;
    if (pEntity != nullptr)
    {
        if (pEntity->GetComponent(L"viewer"))
        {
            m_pViewer = pEntity;
            hRetVal = S_OK;
        }
    }
    else
    {
        m_pViewer = nullptr;
    }

    return hRetVal;
}

STDMETHODIMP_(IGEKEntity *) CGEKRenderManager::GetViewer(void)
{
    return m_pViewer;
}

STDMETHODIMP_(void) CGEKRenderManager::DrawScene(UINT32 nAttributes)
{
    GEKFUNCTION(L"Attributes: %d", nAttributes);
    REQUIRE_VOID_RETURN(m_pCurrentPass);
    REQUIRE_VOID_RETURN(m_pCurrentFilter);

    for (auto &kPair : m_aVisibleModels)
    {
        kPair.first->Draw(m_pCurrentFilter->GetVertexAttributes(), kPair.second);
    }
}

STDMETHODIMP_(void) CGEKRenderManager::DrawLights(std::function<void(void)> OnLightBatch)
{
    GEKFUNCTION(nullptr);
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

STDMETHODIMP_(void) CGEKRenderManager::DrawOverlay(void)
{
    GEKFUNCTION(nullptr);
    m_pVideoSystem->GetImmediateContext()->GetVertexSystem()->SetProgram(m_spVertexProgram);
    m_pVideoSystem->GetImmediateContext()->GetVertexSystem()->SetConstantBuffer(1, m_spOrthoBuffer);

    m_pVideoSystem->GetImmediateContext()->GetGeometrySystem()->SetProgram(nullptr);

    m_pVideoSystem->GetImmediateContext()->SetVertexBuffer(0, 0, m_spVertexBuffer);
    m_pVideoSystem->GetImmediateContext()->SetIndexBuffer(0, m_spIndexBuffer);
    m_pVideoSystem->GetImmediateContext()->SetPrimitiveType(GEKVIDEO::PRIMITIVE::TRIANGLELIST);

    m_pVideoSystem->GetImmediateContext()->DrawIndexedPrimitive(6, 0, 0);
}

static void CountPasses(std::map<CGEKRenderManager::PASS *, INT32> &aPasses, CGEKRenderManager::PASS *pPass)
{
    if (aPasses.find(pPass) == aPasses.end())
    {
        aPasses[pPass] = 1;
    }
    else
    {
        aPasses[pPass]++;
    }

    for (auto &pRequiredPass : pPass->m_aRequiredPasses)
    {
        CountPasses(aPasses, pRequiredPass);
    }
}

STDMETHODIMP_(void) CGEKRenderManager::Render(void)
{
    GEKFUNCTION(nullptr);
    REQUIRE_VOID_RETURN(m_pWebCore);

    m_pWebCore->Update();
    for (auto &kPair : m_aWebSurfaces)
    {
        CComQIPtr<IGEKWebSurface> spWebSurface(kPair.second);
        if (spWebSurface)
        {
            spWebSurface->Update();
        }
    }

    if (m_pViewer != nullptr)
    {
        IGEKComponent *pViewerComponent = m_pViewer->GetComponent(L"viewer");
        IGEKComponent *pTransformComponent = m_pViewer->GetComponent(L"transform");
        if (pTransformComponent != nullptr && pViewerComponent != nullptr)
        {
            GEKVALUE kFieldOfView;
            GEKVALUE kMinViewDistance;
            GEKVALUE kMaxViewDistance;
            pViewerComponent->GetProperty(L"fieldofview", kFieldOfView);
            pViewerComponent->GetProperty(L"minviewdistance", kMinViewDistance);
            pViewerComponent->GetProperty(L"maxviewdistance", kMaxViewDistance);

            GEKVALUE kPosition;
            GEKVALUE kRotation;
            pTransformComponent->GetProperty(L"position", kPosition);
            pTransformComponent->GetProperty(L"rotation", kRotation);

            float4x4 nCameraMatrix;
            nCameraMatrix = kRotation.GetQuaternion();
            nCameraMatrix.t = kPosition.GetFloat3();

            float nXSize = float(m_pSystem->GetXSize());
            float nYSize = float(m_pSystem->GetYSize());
            float nAspect = (nXSize / nYSize);

            m_kEngineBuffer.m_nCameraSize.x = nXSize;
            m_kEngineBuffer.m_nCameraSize.y = nYSize;
            m_kEngineBuffer.m_nCameraView.x = tan(kFieldOfView.GetFloat() * 0.5f);
            m_kEngineBuffer.m_nCameraView.y = (m_kEngineBuffer.m_nCameraView.x / nAspect);
            m_kEngineBuffer.m_nCameraViewDistance = kMaxViewDistance.GetFloat();
            m_kEngineBuffer.m_nCameraPosition = kPosition.GetFloat3();

            m_kEngineBuffer.m_nViewMatrix = nCameraMatrix.GetInverse();
            m_kEngineBuffer.m_nProjectionMatrix.SetPerspective(kFieldOfView.GetFloat(), nAspect, kMinViewDistance.GetFloat(), kMaxViewDistance.GetFloat());
            m_kEngineBuffer.m_nTransformMatrix = (m_kEngineBuffer.m_nViewMatrix * m_kEngineBuffer.m_nProjectionMatrix);

            m_kFrustum.Create(nCameraMatrix, m_kEngineBuffer.m_nProjectionMatrix);

            IGEKSceneManager *pSceneManager = GetContext()->GetCachedClass<IGEKSceneManager>(CLSID_GEKPopulationManager);
            if (pSceneManager != nullptr)
            {
                concurrency::concurrent_unordered_set<IGEKEntity *> aVisibleEntities;
                pSceneManager->GetVisible(m_kFrustum, aVisibleEntities);

                concurrency::concurrent_vector<LIGHT> aLights;
                concurrency::concurrent_unordered_multimap<IGEKModel *, IGEKModel::INSTANCE> aModels;
                concurrency::parallel_for_each(aVisibleEntities.begin(), aVisibleEntities.end(), [&](IGEKEntity *pEntity) -> void
                {
                    IGEKComponent *pTransformComponent = pEntity->GetComponent(L"transform");
                    if (pTransformComponent != nullptr)
                    {
                        GEKVALUE kPosition;
                        GEKVALUE kRotation;
                        pTransformComponent->GetProperty(L"position", kPosition);
                        pTransformComponent->GetProperty(L"rotation", kRotation);

                        IGEKComponent *pLightComponent = pEntity->GetComponent(L"light");
                        if (pLightComponent != nullptr)
                        {
                            GEKVALUE kColor;
                            GEKVALUE kRange;
                            pLightComponent->GetProperty(L"color", kColor);
                            pLightComponent->GetProperty(L"range", kRange);

                            LIGHT kLight;
                            kLight.m_nPosition = (m_kEngineBuffer.m_nViewMatrix * float4(kPosition.GetFloat3(), 1.0f));
                            kLight.m_nRange = kRange.GetFloat();
                            kLight.m_nInvRange = (1.0f / kLight.m_nRange);
                            kLight.m_nColor = kColor.GetFloat3();
                            aLights.push_back(kLight);
                        }

                        IGEKComponent *pModelComponent = pEntity->GetComponent(L"model");
                        if (pModelComponent != nullptr)
                        {
                            GEKVALUE kModel;
                            pModelComponent->GetProperty(L"model", kModel);
                            CComQIPtr<IGEKModel> spModel(kModel.GetObject());
                            if (spModel)
                            {
                                IGEKModel::INSTANCE kInstance;
                                kInstance.m_nMatrix = kRotation.GetQuaternion();
                                kInstance.m_nMatrix.t = kPosition.GetFloat3();
                                aModels.insert(std::make_pair(spModel, kInstance));
                            }
                        }
                    }
                });

                m_aVisibleLights.assign(aLights.begin(), aLights.end());

                m_aVisibleModels.clear();
                for (auto &kPair : aModels)
                {
                    m_aVisibleModels[kPair.first].push_back(kPair.second);
                }

                for (auto &kPair : m_aVisibleModels)
                {
                    kPair.first->Prepare();
                }
            }
        }

        m_spEngineBuffer->Update((void *)&m_kEngineBuffer);
        m_pVideoSystem->GetImmediateContext()->GetVertexSystem()->SetConstantBuffer(0, m_spEngineBuffer);
        m_pVideoSystem->GetImmediateContext()->GetGeometrySystem()->SetConstantBuffer(0, m_spEngineBuffer);
        m_pVideoSystem->GetImmediateContext()->GetPixelSystem()->SetConstantBuffer(0, m_spEngineBuffer);
        m_pVideoSystem->GetImmediateContext()->GetPixelSystem()->SetSamplerStates(0, m_spPointSampler);
        m_pVideoSystem->GetImmediateContext()->GetPixelSystem()->SetSamplerStates(1, m_spLinearSampler);
        m_pVideoSystem->GetImmediateContext()->GetComputeSystem()->SetConstantBuffer(0, m_spEngineBuffer);

        for (auto &kPair : m_aCurrentPasses)
        {
            CountPasses(m_aCurrentPasses, kPair.first);
        }

        std::map<INT32, std::list<PASS *>> aSortedPasses;
        for (auto &kPair : m_aCurrentPasses)
        {
            aSortedPasses[kPair.second].push_back(kPair.first);
        }

        std::for_each(aSortedPasses.rbegin(), aSortedPasses.rend(), [&](std::map<INT32, std::list<PASS *>>::value_type &kPair) -> void
        {
            std::for_each(kPair.second.rbegin(), kPair.second.rend(), [&](PASS *pPass) -> void
            {
                m_pCurrentPass = pPass;
                for (auto &pFilter : m_pCurrentPass->m_aFilters)
                {
                    m_pCurrentFilter = pFilter;
                    pFilter->Draw();
                }

                m_pCurrentFilter = nullptr;
            });
        });

        m_pCurrentPass = nullptr;
        m_pVideoSystem->GetImmediateContext()->ClearResources();
        m_pVideoSystem->Present(true);

        while (!m_pVideoSystem->IsEventSet(m_spFrameEvent))
        {
            Sleep(0);
        };

        m_pVideoSystem->SetEvent(m_spFrameEvent);
    }
}

void CGEKRenderManager::OnRequest(int nRequestID, const Awesomium::ResourceRequest &kRequest, const Awesomium::WebString &kPath)
{
    CStringW strPath((LPCWSTR)kPath.data());

    std::vector<UINT8> aBuffer;
    if (SUCCEEDED(GEKLoadFromFile(L"%root%\\data\\" + strPath, aBuffer)))
    {
        static const std::map<GEKHASH, CStringA> aMimeTypes = 
        {
            { L".htm", "text/html", },
            { L".html", "text/html", },
            { L".xml", "text/xml", },
            { L".js", "text/javascript", },
            { L".css", "text/css", },
            { L".png", "image/png", },
            { L".gif", "image/gif", },
            { L".bmp", "image/bmp", },
            { L".jpg", "image/jpg", },
        };

        CStringW strExtension = CPathW(strPath).GetExtension();
        auto pIterator = aMimeTypes.find(strExtension);
        if (pIterator != aMimeTypes.end())
        {
            SendResponse(nRequestID, aBuffer.size(), &aBuffer[0], Awesomium::WSLit((*pIterator).second));
        }
        else
        {
            SendResponse(nRequestID, 0, nullptr, Awesomium::WSLit(""));
        }
    }
    else
    {
        SendResponse(nRequestID, 0, nullptr, Awesomium::WSLit(""));
    }
}

void CGEKRenderManager::OnMethodCall(Awesomium::WebView *pCaller, unsigned int nRemoteObjectID, const Awesomium::WebString &kMethodName, const Awesomium::JSArray &aArgs)
{
    IGEKEngine *pEngine = GetContext()->GetCachedClass<IGEKEngine>(CLSID_GEKEngine);
    if (pEngine != nullptr)
    {
        std::vector<CStringW> aParams;
        for (UINT32 nIndex = 0; nIndex < aArgs.size(); nIndex++)
        {
            const Awesomium::JSValue &kValue = aArgs.At(nIndex);
            aParams.push_back((LPCWSTR)kValue.ToString().data());
        }

        if (aParams.size() > 0)
        {
            std::vector<LPCWSTR> aParamPointers(aParams.begin(), aParams.end());
            pEngine->OnCommand((LPCWSTR)kMethodName.data(), &aParamPointers[0], aParamPointers.size());
        }
        else
        {
            pEngine->OnCommand((LPCWSTR)kMethodName.data(), nullptr, 0);
        }
    }
}

Awesomium::JSValue CGEKRenderManager::OnMethodCallWithReturnValue(Awesomium::WebView *pCaller, unsigned int nRemoteObjectID, const Awesomium::WebString &kMethodName, const Awesomium::JSArray &aArgs)
{
    if (_wcsicmp((LPCWSTR)kMethodName.data(), L"GetResolutions") == 0)
    {
        Awesomium::JSArray aArray;
        std::vector<GEKMODE> akModes = GEKGetDisplayModes()[32];
        for (UINT32 nMode = 0; nMode < akModes.size(); nMode++)
        {
            GEKMODE &kMode = akModes[nMode];

            CStringA strAspect("");
            switch (kMode.GetAspect())
            {
            case _ASPECT_4x3:
                strAspect = ", (4x3)";
                break;

            case _ASPECT_16x9:
                strAspect = ", (16x9)";
                break;

            case _ASPECT_16x10:
                strAspect = ", (16x10)";
                break;
            };

            Awesomium::JSObject kObject;
            kObject.SetProperty(Awesomium::WSLit("Label"), Awesomium::JSValue(Awesomium::WSLit(FormatString("%dx%d%s", kMode.xsize, kMode.ysize, strAspect.GetString()))));
            kObject.SetProperty(Awesomium::WSLit("XSize"), Awesomium::JSValue(int(kMode.xsize)));
            kObject.SetProperty(Awesomium::WSLit("YSize"), Awesomium::JSValue(int(kMode.ysize)));
            aArray.Push(kObject);
        }

        return aArray;
    }
    else if (_wcsicmp((LPCWSTR)kMethodName.data(), L"GetValue") == 0)
    {
        if (aArgs.size() == 2)
        {
            CStringW strGroup = (LPCWSTR)aArgs.At(0).ToString().data(); 
            CStringW strName = (LPCWSTR)aArgs.At(1).ToString().data();
            CStringW strValue = m_pSystem->GetConfig().GetValue(strGroup, strName, L"");
            return Awesomium::JSValue(Awesomium::WSLit(CW2A(strValue, CP_UTF8)));
        }
    }

    return Awesomium::JSValue();
}

Awesomium::Surface *CGEKRenderManager::CreateSurface(Awesomium::WebView *pView, int nXSize, int nYSize)
{
    CComPtr<CGEKWebSurface> spWebSurface(new CGEKWebSurface(m_pVideoSystem, nXSize, nYSize));
    GEKRESULT(spWebSurface, L"Call to new failed to allocate instance");
    if (spWebSurface)
    {
        spWebSurface->QueryInterface(IID_PPV_ARGS(&m_aWebSurfaces[pView]));
        return spWebSurface.Detach();
    }

    return nullptr;
}

void CGEKRenderManager::DestroySurface(Awesomium::Surface *pSurface)
{
    CComPtr<CGEKWebSurface> spWebSurface;
    spWebSurface.Attach(dynamic_cast<CGEKWebSurface *>(pSurface));
    auto pIterator = std::find_if(m_aWebSurfaces.begin(), m_aWebSurfaces.end(), [&](std::map<Awesomium::WebView *, CComPtr<IUnknown>>::value_type &kPair) -> bool
    {
        return (spWebSurface.IsEqualObject(kPair.second));
    });

    if (pIterator != m_aWebSurfaces.end())
    {
        m_aWebSurfaces.erase(pIterator);
    }
}

void CGEKRenderManager::OnChangeTitle(Awesomium::WebView *pCaller, const Awesomium::WebString &kTitle)
{
}

void CGEKRenderManager::OnChangeAddressBar(Awesomium::WebView *pCaller, const Awesomium::WebURL &kURL)
{
}

void CGEKRenderManager::OnChangeTooltip(Awesomium::WebView *pCaller, const Awesomium::WebString &kTooltip)
{
}

void CGEKRenderManager::OnChangeTargetURL(Awesomium::WebView *pCaller, const Awesomium::WebURL &kURL)
{
}

void CGEKRenderManager::OnChangeCursor(Awesomium::WebView *pCaller, Awesomium::Cursor eCursor)
{
}

void CGEKRenderManager::OnChangeFocus(Awesomium::WebView *pCaller, Awesomium::FocusedElementType eFocusedType)
{
}

void CGEKRenderManager::OnAddConsoleMessage(Awesomium::WebView *pCaller, const Awesomium::WebString &kMessage, int nLineNumber, const Awesomium::WebString &kSource)
{
    CStringW strMessage((LPCWSTR)kMessage.data());
    CStringW strSource((LPCWSTR)kSource.data());
    OutputDebugStringW(FormatString(L"Console Message (%s: %d): %s\r\n", strSource.GetString(), nLineNumber, strMessage.GetString()));
}

void CGEKRenderManager::OnShowCreatedWebView(Awesomium::WebView *pCaller, Awesomium::WebView *pNewView, const Awesomium::WebURL &kOpenerURL, const Awesomium::WebURL &kTargetURL, const Awesomium::Rect &nInitialPosition, bool bIsPopup)
{
}
