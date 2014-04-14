﻿#include "CGEKMaterial.h"
#include "GEKSystem.h"

BEGIN_INTERFACE_LIST(CGEKMaterial)
    INTERFACE_LIST_ENTRY_COM(IGEKMaterial)
END_INTERFACE_LIST_UNKNOWN

REGISTER_CLASS(CGEKMaterial)

CGEKMaterial::CGEKMaterial(void)
{
}

CGEKMaterial::~CGEKMaterial(void)
{
}

STDMETHODIMP_(void) CGEKMaterial::SetPass(LPCWSTR pPass)
{
    m_strPass = pPass;
}

STDMETHODIMP_(void) CGEKMaterial::SetAlbedoMap(IUnknown *pTexture)
{
    m_spAlbedoMap = pTexture;
}

STDMETHODIMP_(void) CGEKMaterial::SetNormalMap(IUnknown *pTexture)
{
    m_spNormalMap = pTexture;
}

STDMETHODIMP_(void) CGEKMaterial::SetInfoMap(IUnknown *pTexture)
{
    m_spInfoMap = pTexture;
}

STDMETHODIMP_(LPCWSTR) CGEKMaterial::GetPass(void)
{
    return m_strPass.GetString();
}

STDMETHODIMP_(IUnknown *) CGEKMaterial::GetAlbedoMap(void)
{
    return m_spAlbedoMap;
}

STDMETHODIMP_(IUnknown *) CGEKMaterial::GetNormalMap(void)
{
    return m_spNormalMap;
}

STDMETHODIMP_(IUnknown *) CGEKMaterial::GetInfoMap(void)
{
    return m_spInfoMap;
}
