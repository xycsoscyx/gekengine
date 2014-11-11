#include "CGEKComponentViewer.h"
#include <algorithm>
#include <ppl.h>

#include "GEKEngineCLSIDs.h"

BEGIN_INTERFACE_LIST(CGEKComponentViewer)
    INTERFACE_LIST_ENTRY_COM(IGEKComponent)
END_INTERFACE_LIST_UNKNOWN

REGISTER_CLASS(CGEKComponentViewer)

CGEKComponentViewer::CGEKComponentViewer(void)
{
}

CGEKComponentViewer::~CGEKComponentViewer(void)
{
}

STDMETHODIMP_(LPCWSTR) CGEKComponentViewer::GetName(void) const
{
    return L"viewer";
};

STDMETHODIMP_(void) CGEKComponentViewer::Clear(void)
{
    m_aData.clear();
}

STDMETHODIMP CGEKComponentViewer::AddComponent(const GEKENTITYID &nEntityID)
{
    m_aData[nEntityID] = DATA();
    return S_OK;
}

STDMETHODIMP CGEKComponentViewer::RemoveComponent(const GEKENTITYID &nEntityID)
{
    return (m_aData.unsafe_erase(nEntityID) > 0 ? S_OK : E_FAIL);
}

STDMETHODIMP_(bool) CGEKComponentViewer::HasComponent(const GEKENTITYID &nEntityID) const
{
    return (m_aData.find(nEntityID) != m_aData.end());
}

STDMETHODIMP_(void) CGEKComponentViewer::ListProperties(const GEKENTITYID &nEntityID, std::function<void(LPCWSTR, const GEKVALUE &)> OnProperty) const
{
    auto pIterator = m_aData.find(nEntityID);
    if (pIterator != m_aData.end())
    {
        OnProperty(L"type", (*pIterator).second.m_nType);
        OnProperty(L"fieldofview", (*pIterator).second.m_nFieldOfView);
        OnProperty(L"minviewdistance", (*pIterator).second.m_nMinViewDistance);
        OnProperty(L"maxviewdistance", (*pIterator).second.m_nMaxViewDistance);
        OnProperty(L"viewport", (*pIterator).second.m_nViewPort);
        OnProperty(L"pass", (*pIterator).second.m_strPass.GetString());
    }
}

STDMETHODIMP_(bool) CGEKComponentViewer::GetProperty(const GEKENTITYID &nEntityID, LPCWSTR pName, GEKVALUE &kValue) const
{
    bool bReturn = false;
    auto pIterator = m_aData.find(nEntityID);
    if (pIterator != m_aData.end())
    {
        if (_wcsicmp(pName, L"type") == 0)
        {
            kValue = (*pIterator).second.m_nType;
            bReturn = true;
        }
        else if (_wcsicmp(pName, L"fieldofview") == 0)
        {
            kValue = (*pIterator).second.m_nFieldOfView;
            bReturn = true;
        }
        else if (_wcsicmp(pName, L"minviewdistance") == 0)
        {
            kValue = (*pIterator).second.m_nMinViewDistance;
            bReturn = true;
        }
        else if (_wcsicmp(pName, L"maxviewdistance") == 0)
        {
            kValue = (*pIterator).second.m_nMaxViewDistance;
            bReturn = true;
        }
        else if (_wcsicmp(pName, L"viewport") == 0)
        {
            kValue = (*pIterator).second.m_nViewPort;
            bReturn = true;
        }
        else if (_wcsicmp(pName, L"pass") == 0)
        {
            kValue = (*pIterator).second.m_strPass.GetString();
            bReturn = true;
        }
        else if (_wcsicmp(pName, L"projection") == 0)
        {
            float nAspect = 1.0f;
            const IGEKRenderManager *pRenderManager = GetContext()->GetCachedClass<IGEKRenderManager>(CLSID_GEKRenderSystem);
            if (pRenderManager)
            {
                float2 nScreenSize = pRenderManager->GetScreenSize();
                nAspect = (nScreenSize.x / nScreenSize.y);
            }

            static float4x4 nProjection;
            switch ((*pIterator).second.m_nType)
            {
            case _PERSPECTIVE:
                nProjection.SetPerspective(_DEGTORAD((*pIterator).second.m_nFieldOfView), nAspect, (*pIterator).second.m_nMinViewDistance, (*pIterator).second.m_nMaxViewDistance);
                break;

            case _ORTHOGRAPHIC:
                nProjection.SetOrthographic(-((*pIterator).second.m_nFieldOfView * nAspect) / 2.0f, -(*pIterator).second.m_nFieldOfView / 2.0f,
                                             ((*pIterator).second.m_nFieldOfView * nAspect) / 2.0f,  (*pIterator).second.m_nFieldOfView / 2.0f,
                                              (*pIterator).second.m_nMinViewDistance, (*pIterator).second.m_nMaxViewDistance);
                break;
            };

            kValue = nProjection;
            bReturn = true;
        }
    }

    return bReturn;
}

STDMETHODIMP_(bool) CGEKComponentViewer::SetProperty(const GEKENTITYID &nEntityID, LPCWSTR pName, const GEKVALUE &kValue)
{
    bool bReturn = false;
    auto pIterator = m_aData.find(nEntityID);
    if (pIterator != m_aData.end())
    {
        if (_wcsicmp(pName, L"type") == 0)
        {
            if (kValue.GetType() == GEKVALUE::STRING)
            {
                CStringW strType = kValue.GetString();
                if (strType.CompareNoCase(L"perspective") == 0)
                {
                    (*pIterator).second.m_nType = _PERSPECTIVE;
                }
                else if(strType.CompareNoCase(L"orthographic") == 0)
                {
                    (*pIterator).second.m_nType = _ORTHOGRAPHIC;
                }
                else
                {
                    (*pIterator).second.m_nType = _UNKNOWN;
                }
            }
            else
            {
                (*pIterator).second.m_nType = kValue.GetUINT32();
            }

            bReturn = true;
        }
        if (_wcsicmp(pName, L"fieldofview") == 0)
        {
            (*pIterator).second.m_nFieldOfView = kValue.GetFloat();
            bReturn = true;
        }
        else if (_wcsicmp(pName, L"minviewdistance") == 0)
        {
            (*pIterator).second.m_nMinViewDistance = kValue.GetFloat();
            bReturn = true;
        }
        else if (_wcsicmp(pName, L"maxviewdistance") == 0)
        {
            (*pIterator).second.m_nMaxViewDistance = kValue.GetFloat();
            bReturn = true;
        }
        else if (_wcsicmp(pName, L"viewport") == 0)
        {
            (*pIterator).second.m_nViewPort = kValue.GetFloat4();
            bReturn = true;
        }
        else if (_wcsicmp(pName, L"pass") == 0)
        {
            (*pIterator).second.m_strPass = kValue.GetRawString();
            bReturn = true;
        }
    }

    return bReturn;
}
