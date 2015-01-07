#include "CGEKConfig.h"
#include "CGEKParser.h"
#include "GEKUtility.h"
#include <algorithm>

CGEKConfig::CGEKConfig(void)
{
}

CGEKConfig::~CGEKConfig(void)
{
}

HRESULT CGEKConfig::Load(LPCWSTR pFileName)
{
    CGEKParser kParser;
    HRESULT hRetVal = kParser.LoadFromFile(pFileName);
    if (FAILED(hRetVal))
    {
        return hRetVal;
    }

    std::unordered_map<CStringW, CStringW> *pGroup = nullptr;
    while (kParser.NextToken())
    {
        if (kParser.GetToken().GetAt(0) == L'[')
        {
            CStringW strTitle(kParser.GetToken());
            pGroup = &m_aGroups[strTitle.Mid(1, (strTitle.GetLength() - 2))];
        }
        else if (pGroup != nullptr)
        {
            CStringW strToken(kParser.GetToken());
            kParser.NextToken(); // =
            kParser.NextToken();
            CStringW strValue(kParser.GetToken());
            (*pGroup)[strToken] = strValue;
        }
    };

    return S_OK;
}

HRESULT CGEKConfig::Save(LPCWSTR pFileName)
{
    CStringW strBuffer;
    for (auto &kGroupPair : m_aGroups)
    {
        std::unordered_map<CStringW, CStringW> &aGroup = kGroupPair.second;
        if (aGroup.size() > 0)
        {
            strBuffer += (L"[" + kGroupPair.first + L"]\r\n");
            for (auto &kNameValue : aGroup)
            {
                strBuffer += (L"    \"" + kNameValue.first + L"\" = \"" + kNameValue.second + L"\"\r\n");
            }
        }
    }

    // Save To File
    return GEKSaveToFile(pFileName, strBuffer);
}

void CGEKConfig::RemoveGroup(LPCWSTR pTitle)
{
    m_aGroups.erase(pTitle);
}

bool CGEKConfig::DoesGroupExists(LPCWSTR pGroup) const
{
    return (m_aGroups.count(pGroup) > 0);
}

bool CGEKConfig::DoesValueExists(LPCWSTR pGroup, LPCWSTR pString) const
{
    bool bExists = false;
    auto pIterator = m_aGroups.find(pGroup);
    if (pIterator != m_aGroups.end())
    {
        bExists = (((*pIterator).second).count(pString) > 0);
    }

    return bExists;
}

void CGEKConfig::SetValue(LPCWSTR pGroup, LPCWSTR pName, LPCWSTR pFormat, ...)
{
    if (pFormat != nullptr)
    {
        va_list pArgs;
        CStringW strValue;
        va_start(pArgs, pFormat);
        strValue.FormatV(pFormat, pArgs);
        va_end(pArgs);

        m_aGroups[pGroup][pName] = strValue;
    }
}

CStringW CGEKConfig::GetValue(LPCWSTR pGroup, LPCWSTR pName, LPCWSTR pDefault)
{
    auto pGroupIterator = m_aGroups.find(pGroup);
    if (pGroupIterator != m_aGroups.end())
    {
        auto pValueIterator = ((*pGroupIterator).second).find(pName);
        if (pValueIterator != ((*pGroupIterator).second).end())
        {
            return ((*pValueIterator).second);
        }
    }

    m_aGroups[pGroup][pName] = pDefault;
    return pDefault;
}

CStringW CGEKConfig::GetValue(LPCWSTR pGroup, LPCWSTR pName, LPCWSTR pDefault) const
{
    auto pGroupIterator = m_aGroups.find(pGroup);
    if (pGroupIterator != m_aGroups.end())
    {
        auto pValueIterator = ((*pGroupIterator).second).find(pName);
        if (pValueIterator != ((*pGroupIterator).second).end())
        {
            return ((*pValueIterator).second);
        }
    }

    return pDefault;
}
