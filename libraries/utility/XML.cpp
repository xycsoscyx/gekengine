#include "GEK\Utility\XML.h"
#include "GEK\Utility\FileSystem.h"
#include <libxml/tree.h>
#include <libxml/parser.h>
#include <atlpath.h>

#pragma comment(lib, "libxml2.lib")

namespace Gek
{
    XmlNode::XmlNode(LPVOID node)
        : node(node)
    {
    }

    HRESULT XmlNode::create(LPCWSTR type)
    {
        HRESULT resultValue = E_FAIL;

        xmlNodePtr pNewNode = xmlNewNode(nullptr, BAD_CAST LPCSTR(CW2A(type, CP_UTF8)));
        if (pNewNode != nullptr)
        {
            node = pNewNode;
            resultValue = S_OK;
        }

        return resultValue;
    }

    void XmlNode::setType(LPCWSTR type)
    {
        if (node != nullptr)
        {
            xmlNodeSetName(static_cast<xmlNodePtr>(node), BAD_CAST LPCSTR(CW2A(type, CP_UTF8)));
        }
    }

    CStringW XmlNode::getType(void) const
    {
        CStringW type;
        if (node != nullptr)
        {
            type = CA2W((const char *)static_cast<xmlNodePtr>(node)->name, CP_UTF8);
        }

        return type;
    }

    CStringW XmlNode::getText(void) const
    {
        CStringW text;
        if (node != nullptr)
        {
            xmlChar *content = xmlNodeGetContent(static_cast<xmlNodePtr>(node));
            if (content != nullptr)
            {
                text = CA2W((const char *)content, CP_UTF8);
                xmlFree(content);
            }
        }

        return text;
    }

    void XmlNode::setText(LPCWSTR format, ...)
    {
        if (node != nullptr)
        {
            CStringW text;
            if (format != nullptr)
            {
                va_list variableList;
                va_start(variableList, format);
                text.FormatV(format, variableList);
                va_end(variableList);
            }

            xmlNodeSetContent(static_cast<xmlNodePtr>(node), BAD_CAST LPCSTR(CW2A(text, CP_UTF8)));
        }
    }

    bool XmlNode::hasAttribute(LPCWSTR name) const
    {
        if (node != nullptr)
        {
            return (xmlHasProp(static_cast<xmlNodePtr>(node), BAD_CAST LPCSTR(CW2A(name, CP_UTF8))) ? true : false);
        }

        return false;
    }

    CStringW XmlNode::getAttribute(LPCWSTR name) const
    {
        CStringW value;
        if (node != nullptr)
        {
            xmlChar *attribute = xmlGetProp(static_cast<xmlNodePtr>(node), BAD_CAST LPCSTR(CW2A(name, CP_UTF8)));
            if (attribute != nullptr)
            {
                value = CA2W((const char *)attribute, CP_UTF8);
                xmlFree(attribute);
            }
        }

        return value;
    }

    void XmlNode::setAttribute(LPCWSTR name, LPCWSTR format, ...)
    {
        if (node != nullptr)
        {
            CStringW value;
            if (format != nullptr)
            {
                va_list variableList;
                va_start(variableList, format);
                value.FormatV(format, variableList);
                va_end(variableList);
            }

            if (hasAttribute(name))
            {
                xmlSetProp(static_cast<xmlNodePtr>(node), BAD_CAST LPCSTR(CW2A(name, CP_UTF8)), BAD_CAST LPCSTR(CW2A(value, CP_UTF8)));
            }
            else
            {
                xmlNewProp(static_cast<xmlNodePtr>(node), BAD_CAST LPCSTR(CW2A(name, CP_UTF8)), BAD_CAST LPCSTR(CW2A(value, CP_UTF8)));
            }
        }
    }

    void XmlNode::listAttributes(std::function<void(LPCWSTR, LPCWSTR)> onAttribute) const
    {
        if (node != nullptr)
        {
            for (xmlAttrPtr attribute = static_cast<xmlNodePtr>(node)->properties; attribute != nullptr; attribute = attribute->next)
            {
                CA2W name(LPCSTR(attribute->name), CP_UTF8);
                onAttribute(name, getAttribute(name));
            }
        }
    }

    bool XmlNode::hasSiblingElement(LPCWSTR type) const
    {
        bool hasSiblingElement = false;
        if (node != nullptr)
        {
            CW2A typeUtf8(type, CP_UTF8);
            for (xmlNode *checkingNode = static_cast<xmlNodePtr>(node)->next; checkingNode; checkingNode = checkingNode->next)
            {
                if (checkingNode->type == XML_ELEMENT_NODE && (!type || strcmp(typeUtf8, LPCSTR(checkingNode->name)) == 0))
                {
                    hasSiblingElement = true;
                    break;
                }
            }
        }

        return hasSiblingElement;
    }

    XmlNode XmlNode::nextSiblingElement(LPCWSTR type) const
    {
        XmlNode nextNode(nullptr);
        if (node != nullptr)
        {
            CW2A typeUtf8(type, CP_UTF8);
            for (xmlNode *checkingNode = static_cast<xmlNodePtr>(node)->next; checkingNode; checkingNode = checkingNode->next)
            {
                if (checkingNode->type == XML_ELEMENT_NODE && (!type || strcmp(typeUtf8, LPCSTR(checkingNode->name)) == 0))
                {
                    nextNode = XmlNode(checkingNode);
                    break;
                }
            }
        }

        return nextNode;
    }

    bool XmlNode::hasChildElement(LPCWSTR type) const
    {
        bool hasChildElement = false;
        if (node != nullptr)
        {
            CW2A typeUtf8(type, CP_UTF8);
            for (xmlNode *checkingNode = static_cast<xmlNodePtr>(node)->children; checkingNode; checkingNode = checkingNode->next)
            {
                if (checkingNode->type == XML_ELEMENT_NODE && (!type || strcmp(typeUtf8, LPCSTR(checkingNode->name)) == 0))
                {
                    hasChildElement = true;
                    break;
                }
            }
        }

        return hasChildElement;
    }

    XmlNode XmlNode::firstChildElement(LPCWSTR type) const
    {
        XmlNode childNode(nullptr);
        if (node != nullptr)
        {
            CW2A typeUtf8(type, CP_UTF8);
            for (xmlNode *checkingNode = static_cast<xmlNodePtr>(node)->children; checkingNode; checkingNode = checkingNode->next)
            {
                if (checkingNode->type == XML_ELEMENT_NODE && (!type || strcmp(typeUtf8, LPCSTR(checkingNode->name)) == 0))
                {
                    childNode = XmlNode(checkingNode);
                    break;
                }
            }
        }

        return childNode;
    }

    XmlNode XmlNode::createChildElement(LPCWSTR type, LPCWSTR format, ...)
    {
        XmlNode newChildNode(nullptr);
        if (node != nullptr)
        {
            CStringW content;
            if (format != nullptr)
            {
                va_list variableList;
                va_start(variableList, format);
                content.FormatV(format, variableList);
                va_end(variableList);
            }

            xmlNodePtr childNode = xmlNewChild(static_cast<xmlNodePtr>(node), nullptr, BAD_CAST LPCSTR(CW2A(type, CP_UTF8)), BAD_CAST LPCSTR(CW2A(content, CP_UTF8)));
            if (childNode != nullptr)
            {
                xmlAddChild(static_cast<xmlNodePtr>(node), childNode);
                newChildNode = XmlNode(childNode);
            }
        }

        return newChildNode;
    }

    XmlDocument::XmlDocument(void)
        : document(nullptr)
    {
    }

    XmlDocument::~XmlDocument(void)
    {
        if (document != nullptr)
        {
            xmlFreeDoc(static_cast<xmlDocPtr>(document));
        }
    }

    HRESULT XmlDocument::create(LPCWSTR rootType)
    {
        if (document != nullptr)
        {
            xmlFreeDoc(static_cast<xmlDocPtr>(document));
            document = nullptr;
        }

        HRESULT resultValue = E_FAIL;
        document = xmlNewDoc(BAD_CAST "1.0");
        if (document != nullptr)
        {
            xmlNodePtr rootNode = xmlNewNode(nullptr, BAD_CAST LPCSTR(CW2A(rootType, CP_UTF8)));
            if (rootNode != nullptr)
            {
                xmlDocSetRootElement(static_cast<xmlDocPtr>(document), rootNode);
                resultValue = S_OK;
            }
        }

        return resultValue;
    }

    HRESULT XmlDocument::load(LPCWSTR fileName)
    {
        if (document != nullptr)
        {
            xmlFreeDoc(static_cast<xmlDocPtr>(document));
            document = nullptr;
        }

        HRESULT resultValue = E_FAIL;
        CStringW expandedFileName(Gek::FileSystem::expandPath(fileName));
        document = xmlReadFile(CW2A(expandedFileName, CP_UTF8), nullptr, XML_PARSE_DTDATTR | XML_PARSE_NOENT | XML_PARSE_DTDVALID);
        if (document != nullptr)
        {
            resultValue = S_OK;
        }

        return resultValue;
    }

    HRESULT XmlDocument::save(LPCWSTR fileName)
    {
        HRESULT resultValue = E_FAIL;
        if (document != nullptr)
        {
            CStringW expandedFileName(Gek::FileSystem::expandPath(fileName));
            xmlSaveFormatFileEnc(CW2A(expandedFileName, CP_UTF8), static_cast<xmlDocPtr>(document), "UTF-8", 1);
        }

        return resultValue;
    }

    XmlNode XmlDocument::getRoot(void) const
    {
        if (document != nullptr)
        {
            return XmlNode(xmlDocGetRootElement(static_cast<xmlDocPtr>(document)));
        }

        return XmlNode(nullptr);
    }
}; // namespace Gek
