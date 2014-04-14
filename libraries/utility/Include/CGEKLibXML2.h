#pragma once

#include "GEKUtility.h"
#include <libxml/parser.h>
#include <libxml/tree.h>

class CLibXMLNode
{
private:
    xmlNode *m_pNode;

public:
    CLibXMLNode(xmlNode *pNode);

    bool IsValid(void) const;
    operator bool() const
    {
        return IsValid();
    }

    HRESULT Create(LPCWSTR pType);
    void SetType(LPCWSTR pType);
    CStringW GetType(void) const;

    CStringW GetText(void) const;
    void SetText(LPCWSTR pValueFormat, ...);

    bool HasAttribute(LPCWSTR pName) const;
    CStringW GetAttribute(LPCWSTR pName) const;
    void SetAttribute(LPCWSTR pName, LPCWSTR pValueFormat, ...);
    void ListAttributes(std::function<void(LPCWSTR, LPCWSTR)> OnAttribute) const;

    bool HasSiblingElement(LPCWSTR pType = nullptr) const;
    CLibXMLNode NextSiblingElement(LPCWSTR pType = nullptr) const;

    bool HasChildElement(LPCWSTR pType = nullptr) const;
    CLibXMLNode FirstChildElement(LPCWSTR pType = nullptr) const;
    CLibXMLNode CreateChildElement(LPCWSTR pType, LPCWSTR pContentFormat = nullptr, ...);
};

class CLibXMLDoc
{
private:
    xmlDoc *m_pDocument;

public:
    CLibXMLDoc(void);
    ~CLibXMLDoc(void);

    HRESULT Create(LPCWSTR pRootNode);
    HRESULT Load(LPCWSTR pFileName);
    HRESULT Save(LPCWSTR pFileName);

    CLibXMLNode GetRoot(void);
};
