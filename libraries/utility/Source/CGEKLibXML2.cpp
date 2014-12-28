#include "CGEKLibXML2.h"
#include "GEKUtility.h"
#include <atlpath.h>

#pragma comment(lib, "libxml2.lib")

CLibXMLNode::CLibXMLNode(xmlNode *pNode)
    : m_pNode(pNode)
{
}

bool CLibXMLNode::IsValid(void) const
{
    return (m_pNode ? true : false);
}

HRESULT CLibXMLNode::Create(LPCWSTR pType)
{
    HRESULT hRetVal = E_FAIL;

    CStringA strTypeUTF8 = CW2A(pType, CP_UTF8);
    xmlNodePtr pNewNode = xmlNewNode(nullptr, BAD_CAST strTypeUTF8.GetString());
    if (pNewNode != nullptr)
    {
        m_pNode = pNewNode;
        hRetVal = S_OK;
    }

    return hRetVal;
}

void CLibXMLNode::SetType(LPCWSTR pType)
{
    if (m_pNode != nullptr)
    {
        CStringA strTypeUTF8 = CW2A(pType, CP_UTF8);
        xmlNodeSetName(m_pNode, BAD_CAST strTypeUTF8.GetString());
    }
}

CStringW CLibXMLNode::GetType(void) const
{
    CStringW strReturn;
    if (m_pNode != nullptr)
    {
        strReturn = CA2W((const char *)m_pNode->name, CP_UTF8);
    }

    return strReturn;
}

CStringW CLibXMLNode::GetText(void) const
{
    CStringW strReturn;
    if (m_pNode != nullptr)
    {
        xmlChar *pProperty = xmlNodeGetContent(m_pNode);
        if (pProperty != nullptr)
        {
            strReturn = CA2W((const char *)pProperty, CP_UTF8);
            xmlFree(pProperty);
        }
    }

    return strReturn;
}

void CLibXMLNode::SetText(LPCWSTR pValueFormat, ...)
{
    if (m_pNode != nullptr)
    {
        CStringW strValue;
        if (pValueFormat != nullptr)
        {
            va_list pArgs;
            va_start(pArgs, pValueFormat);
            strValue.FormatV(pValueFormat, pArgs);
            va_end(pArgs);
        }

        CStringA strValueUTF8 = CW2A(strValue, CP_UTF8);
        xmlNodeSetContent(m_pNode, BAD_CAST strValueUTF8.GetString());
    }
}

bool CLibXMLNode::HasAttribute(LPCWSTR pName) const
{
    if (m_pNode != nullptr)
    {
        CStringA strNameUTF8 = CW2A(pName, CP_UTF8);
        return (xmlHasProp(m_pNode, BAD_CAST strNameUTF8.GetString()) ? true : false);
    }

    return false;
}

CStringW CLibXMLNode::GetAttribute(LPCWSTR pName) const
{
    CStringW strReturn;
    if (m_pNode != nullptr)
    {
        CStringA strNameUTF8 = CW2A(pName, CP_UTF8);
        xmlChar *pProperty = xmlGetProp(m_pNode, BAD_CAST strNameUTF8.GetString());
        if (pProperty != nullptr)
        {
            strReturn = CA2W((const char *)pProperty, CP_UTF8);
            xmlFree(pProperty);
        }
    }

    return strReturn;
}

void CLibXMLNode::SetAttribute(LPCWSTR pName, LPCWSTR pValueFormat, ...)
{
    if (m_pNode != nullptr)
    {
        CStringA strNameUTF8 = CW2A(pName, CP_UTF8);

        CStringW strValue;
        if (pValueFormat != nullptr)
        {
            va_list pArgs;
            va_start(pArgs, pValueFormat);
            strValue.FormatV(pValueFormat, pArgs);
            va_end(pArgs);
        }

        CStringA strValueUTF8 = CW2A(strValue, CP_UTF8);
        if (HasAttribute(pName))
        {
            xmlSetProp(m_pNode, BAD_CAST strNameUTF8.GetString(), BAD_CAST strValueUTF8.GetString());
        }
        else
        {
            xmlNewProp(m_pNode, BAD_CAST strNameUTF8.GetString(), BAD_CAST strValueUTF8.GetString());
        }
    }
}

void CLibXMLNode::ListAttributes(std::function<void(LPCWSTR, LPCWSTR)> OnAttribute) const
{
    if (m_pNode != nullptr)
    {
        for(xmlAttrPtr pAttribute = m_pNode->properties; pAttribute != nullptr; pAttribute = pAttribute->next)
        {
            CStringA strNameUTF8(pAttribute->name);
            CStringW strName(CA2W(strNameUTF8, CP_UTF8));
            CStringW strValue(GetAttribute(strName));
            OnAttribute(strName, strValue);
        }
    }
}

bool CLibXMLNode::HasSiblingElement(LPCWSTR pType) const
{
    bool bReturn = false;
    if (m_pNode != nullptr)
    {
        xmlNode *pNode = m_pNode;
        CStringA strTypeUTF8(CW2A(pType, CP_UTF8));
        for (; pNode; pNode = pNode->next)
        {
            if (pNode->type == XML_ELEMENT_NODE)
            {
                if (strTypeUTF8.IsEmpty() || strTypeUTF8 == pNode->name)
                {
                    bReturn = true;
                    break;
                }
            }
        }
    }

    return bReturn;
}

CLibXMLNode CLibXMLNode::NextSiblingElement(LPCWSTR pType) const
{
    if (m_pNode != nullptr)
    {
        xmlNode *pNode = m_pNode->next;
        CStringA strTypeUTF8(CW2A(pType, CP_UTF8));
        for (; pNode; pNode = pNode->next)
        {
            if (pNode->type == XML_ELEMENT_NODE)
            {
                if (strTypeUTF8.IsEmpty() || strTypeUTF8 == pNode->name)
                {
                    break;
                }
            }
        }

        if (pNode != nullptr)
        {
            return CLibXMLNode(pNode);
        }
    }

    return CLibXMLNode(nullptr);
}

bool CLibXMLNode::HasChildElement(LPCWSTR pType) const
{
    bool bReturn = false;
    if (m_pNode != nullptr)
    {
        xmlNode *pNode = m_pNode->children;
        CStringA strTypeUTF8(CW2A(pType, CP_UTF8));
        for (; pNode; pNode = pNode->next)
        {
            if (pNode->type == XML_ELEMENT_NODE)
            {
                if (strTypeUTF8.IsEmpty() || strTypeUTF8 == pNode->name)
                {
                    bReturn = true;
                    break;
                }
            }
        }
    }

    return bReturn;
}

CLibXMLNode CLibXMLNode::FirstChildElement(LPCWSTR pType) const
{
    if (m_pNode != nullptr)
    {
        xmlNode *pNode = m_pNode->children;
        CStringA strTypeUTF8(CW2A(pType, CP_UTF8));
        for (; pNode; pNode = pNode->next)
        {
            if (pNode->type == XML_ELEMENT_NODE)
            {
                if (strTypeUTF8.IsEmpty() || strTypeUTF8 == pNode->name)
                {
                    break;
                }
            }
        }

        if (pNode != nullptr)
        {
            return CLibXMLNode(pNode);
        }
    }

    return CLibXMLNode(nullptr);
}

CLibXMLNode CLibXMLNode::CreateChildElement(LPCWSTR pType, LPCWSTR pContentFormat, ...)
{
    if (m_pNode != nullptr)
    {
        CStringA strTypeUTF8(CW2A(pType, CP_UTF8));

        CStringW strContent;
        if (pContentFormat != nullptr)
        {
            va_list pArgs;
            va_start(pArgs, pContentFormat);
            strContent.FormatV(pContentFormat, pArgs);
            va_end(pArgs);
        }

        CStringA strContentUTF8(CW2A(strContent, CP_UTF8));
        xmlNodePtr pNewNode = xmlNewChild(m_pNode, nullptr, BAD_CAST strTypeUTF8.GetString(), BAD_CAST strContentUTF8.GetString());
        if (pNewNode != nullptr)
        {
            xmlAddChild(m_pNode, pNewNode);
            return CLibXMLNode(pNewNode);
        }
    }

    return CLibXMLNode(nullptr);
}

CLibXMLDoc::CLibXMLDoc(void)
    : m_pDocument(nullptr)
{
}

CLibXMLDoc::~CLibXMLDoc(void)
{
    if (m_pDocument != nullptr)
    {
        xmlFreeDoc(m_pDocument);
    }
}

HRESULT CLibXMLDoc::Create(LPCWSTR pRootNode)
{
    if (m_pDocument != nullptr)
    {
        xmlFreeDoc(m_pDocument);
        m_pDocument = nullptr;
    }

    HRESULT hRetVal = E_FAIL;
    xmlDoc *pDocument = xmlNewDoc(BAD_CAST "1.0");
    if (pDocument != nullptr)
    {
        CStringA strRootNodeUTF8(CW2A(pRootNode, CP_UTF8));
        xmlNodePtr pRoot = xmlNewNode(nullptr, BAD_CAST strRootNodeUTF8.GetString());
        if (pRoot != nullptr)
        {
            xmlDocSetRootElement(pDocument, pRoot);
            m_pDocument = pDocument;
            hRetVal = S_OK;
        }
    }

    return hRetVal;
}

HRESULT CLibXMLDoc::Load(LPCWSTR pFileName)
{
    if (m_pDocument != nullptr)
    {
        xmlFreeDoc(m_pDocument);
        m_pDocument = nullptr;
    }

    HRESULT hRetVal = E_FAIL;
    CStringW strFullFileName(GEKParseFileName(pFileName));
    CStringA strFileNameUTF8(CW2A(strFullFileName, CP_UTF8));
    m_pDocument = xmlReadFile(strFileNameUTF8, nullptr, XML_PARSE_DTDATTR | XML_PARSE_NOENT | XML_PARSE_DTDVALID);
    if (m_pDocument != nullptr)
    {
        hRetVal = S_OK;
    }

    return hRetVal;
}

HRESULT CLibXMLDoc::Save(LPCWSTR pFileName)
{
    HRESULT hRetVal = E_FAIL;
    if (m_pDocument != nullptr)
    {
        CStringW strFullFileName(GEKParseFileName(pFileName));
        CStringA strFileNameUTF8(CW2A(strFullFileName, CP_UTF8));
        xmlSaveFormatFileEnc(strFileNameUTF8, m_pDocument, "UTF-8", 1);
    }

    return hRetVal;
}

CLibXMLNode CLibXMLDoc::GetRoot(void)
{
    if (m_pDocument != nullptr)
    {
        return CLibXMLNode(xmlDocGetRootElement(m_pDocument));
    }

    return CLibXMLNode(nullptr);
}
