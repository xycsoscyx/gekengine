#pragma once

#include "GEKUtility.h"
#include <unordered_map>

class CGEKConfig
{
private:
    std::unordered_map<CStringW, std::unordered_map<CStringW, CStringW>> m_aGroups;

public:
    CGEKConfig(void);
    virtual ~CGEKConfig(void);

    HRESULT Load(LPCWSTR pFileName);
    HRESULT Save(LPCWSTR pFileName);

    void RemoveGroup(LPCWSTR pTitle);

    bool DoesGroupExists(LPCWSTR pGroup) const;
    bool DoesValueExists(LPCWSTR pGroup, LPCWSTR pString) const;

    void SetValue(LPCWSTR pGroup, LPCWSTR pName, LPCWSTR pFormat, ...);
    CStringW GetValue(LPCWSTR pGroup, LPCWSTR pName, LPCWSTR pDefault);
    CStringW GetValue(LPCWSTR pGroup, LPCWSTR pName, LPCWSTR pDefault) const;
};
