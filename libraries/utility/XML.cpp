#include "GEK\Utility\XML.h"
#include "GEK\Utility\FileSystem.h"
#include <libxml/tree.h>
#include <libxml/parser.h>
#include <atlpath.h>

#pragma comment(lib, "libxml2.lib")

namespace Gek
{
    static xmlNodePtr dummyNode = nullptr;
    namespace Xml
    {
        void initialize(void)
        {
            dummyNode = xmlNewNode(nullptr, BAD_CAST "");
        }

        void shutdown(void)
        {
            if (dummyNode)
            {
                xmlFreeNode(dummyNode);
                dummyNode = nullptr;
            }
        }
    };

    XmlNode::XmlNode(void)
        : node(dummyNode)
    {
        GEK_CHECK_EXCEPTION(node == nullptr, Xml::Exception, "Gek::Xml::initialize() not called");
    }

    XmlNode::XmlNode(LPVOID node)
        : node(node)
    {
        GEK_CHECK_EXCEPTION(node == nullptr, Xml::Exception, "Invalid node encountered");
    }

    XmlNode::~XmlNode(void)
    {
    }

    XmlNode XmlNode::create(LPCWSTR type)
    {
        xmlNodePtr node = xmlNewNode(nullptr, BAD_CAST LPCSTR(CW2A(type, CP_UTF8)));
        GEK_CHECK_EXCEPTION(node == nullptr, Xml::Exception, "Unable to create node: %S", type);
        return XmlNode(node);
    }

    XmlNode::operator bool() const
    {
        return (node == dummyNode);
    }

    CStringW XmlNode::getType(void) const
    {
        GEK_CHECK_EXCEPTION(node == nullptr, Xml::Exception, "Invalid node encountered");
        return LPCWSTR(CA2W(reinterpret_cast<LPCSTR>(static_cast<xmlNodePtr>(node)->name), CP_UTF8));
    }

    CStringW XmlNode::getText(void) const
    {
        GEK_CHECK_EXCEPTION(node == nullptr, Xml::Exception, "Invalid node encountered");

        xmlChar *content = xmlNodeGetContent(static_cast<xmlNodePtr>(node));
        GEK_CHECK_EXCEPTION(content == nullptr, Xml::Exception, "Unable to get node content");

        CA2W text(reinterpret_cast<LPCSTR>(content), CP_UTF8);
        xmlFree(content);
        return LPCWSTR(text);
    }

    void XmlNode::setText(LPCWSTR format, ...)
    {
        GEK_CHECK_EXCEPTION(node == nullptr, Xml::Exception, "Invalid node encountered");

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

    bool XmlNode::hasAttribute(LPCWSTR name) const
    {
        GEK_CHECK_EXCEPTION(node == nullptr, Xml::Exception, "Invalid node encountered");
        return (xmlHasProp(static_cast<xmlNodePtr>(node), BAD_CAST LPCSTR(CW2A(name, CP_UTF8))) ? true : false);
    }

    CStringW XmlNode::getAttribute(LPCWSTR name) const
    {
        GEK_CHECK_EXCEPTION(node == nullptr, Xml::Exception, "Invalid node encountered");

        xmlChar *attribute = xmlGetProp(static_cast<xmlNodePtr>(node), BAD_CAST LPCSTR(CW2A(name, CP_UTF8)));
        GEK_CHECK_EXCEPTION(attribute == nullptr, Xml::Exception, "Unable to get node attribute: %S", name);

        CA2W value(reinterpret_cast<LPCSTR>(attribute), CP_UTF8);
        xmlFree(attribute);
        return LPCWSTR(value);
    }

    void XmlNode::setAttribute(LPCWSTR name, LPCWSTR format, ...)
    {
        GEK_CHECK_EXCEPTION(node == nullptr, Xml::Exception, "Invalid node encountered");

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

    void XmlNode::listAttributes(std::function<void(LPCWSTR, LPCWSTR)> onAttribute) const
    {
        GEK_CHECK_EXCEPTION(node == nullptr, Xml::Exception, "Invalid node encountered");

        for (xmlAttrPtr attribute = static_cast<xmlNodePtr>(node)->properties; attribute != nullptr; attribute = attribute->next)
        {
            CA2W name(LPCSTR(attribute->name), CP_UTF8);
            onAttribute(name, getAttribute(name));
        }
    }

    bool XmlNode::hasSiblingElement(LPCWSTR type) const
    {
        GEK_CHECK_EXCEPTION(node == nullptr, Xml::Exception, "Invalid node encountered");

        bool hasSiblingElement = false;
        CW2A typeUtf8(type, CP_UTF8);
        for (xmlNode *checkingNode = static_cast<xmlNodePtr>(node)->next; checkingNode; checkingNode = checkingNode->next)
        {
            if (checkingNode->type == XML_ELEMENT_NODE && (!type || strcmp(typeUtf8, LPCSTR(checkingNode->name)) == 0))
            {
                hasSiblingElement = true;
                break;
            }
        }

        return hasSiblingElement;
    }

    XmlNode XmlNode::nextSiblingElement(LPCWSTR type) const
    {
        GEK_CHECK_EXCEPTION(node == nullptr, Xml::Exception, "Invalid node encountered");

        XmlNode nextNode();
        CW2A typeUtf8(type, CP_UTF8);
        for (xmlNodePtr checkingNode = static_cast<xmlNodePtr>(node)->next; checkingNode; checkingNode = checkingNode->next)
        {
            if (checkingNode->type == XML_ELEMENT_NODE && (!type || strcmp(typeUtf8, LPCSTR(checkingNode->name)) == 0))
            {
                return XmlNode(checkingNode);
            }
        }

        return XmlNode();
    }

    bool XmlNode::hasChildElement(LPCWSTR type) const
    {
        GEK_CHECK_EXCEPTION(node == nullptr, Xml::Exception, "Invalid node encountered");

        bool hasChildElement = false;
        CW2A typeUtf8(type, CP_UTF8);
        for (xmlNode *checkingNode = static_cast<xmlNodePtr>(node)->children; checkingNode; checkingNode = checkingNode->next)
        {
            if (checkingNode->type == XML_ELEMENT_NODE && (!type || strcmp(typeUtf8, LPCSTR(checkingNode->name)) == 0))
            {
                hasChildElement = true;
                break;
            }
        }

        return hasChildElement;
    }

    XmlNode XmlNode::firstChildElement(LPCWSTR type) const
    {
        GEK_CHECK_EXCEPTION(node == nullptr, Xml::Exception, "Invalid node encountered");

        CW2A typeUtf8(type, CP_UTF8);
        for (xmlNodePtr checkingNode = static_cast<xmlNodePtr>(node)->children; checkingNode; checkingNode = checkingNode->next)
        {
            if (checkingNode->type == XML_ELEMENT_NODE && (!type || strcmp(typeUtf8, LPCSTR(checkingNode->name)) == 0))
            {
                return XmlNode(checkingNode);
            }
        }

        return XmlNode();
    }

    XmlNode XmlNode::createChildElement(LPCWSTR type, LPCWSTR format, ...)
    {
        GEK_CHECK_EXCEPTION(node == nullptr, Xml::Exception, "Invalid node encountered");

        CStringW content;
        if (format != nullptr)
        {
            va_list variableList;
            va_start(variableList, format);
            content.FormatV(format, variableList);
            va_end(variableList);
        }

        xmlNodePtr childNode = xmlNewChild(static_cast<xmlNodePtr>(node), nullptr, BAD_CAST LPCSTR(CW2A(type, CP_UTF8)), BAD_CAST LPCSTR(CW2A(content, CP_UTF8)));
        GEK_CHECK_EXCEPTION(childNode == nullptr, Xml::Exception, "Unable to create new child node: %S (%S)", type, content);

        xmlAddChild(static_cast<xmlNodePtr>(node), childNode);

        return XmlNode(childNode);
    }

    XmlDocument::XmlDocument(void *document)
        : document(document)
    {
    }

    XmlDocument::~XmlDocument(void)
    {
        if (document != nullptr)
        {
            xmlFreeDoc(static_cast<xmlDocPtr>(document));
        }
    }

    XmlDocument XmlDocument::create(LPCWSTR rootType)
    {
        xmlDocPtr document = xmlNewDoc(BAD_CAST "1.0");
        GEK_CHECK_EXCEPTION(document == nullptr, Xml::Exception, "Unable to create new document");

        xmlNodePtr rootNode = xmlNewNode(nullptr, BAD_CAST LPCSTR(CW2A(rootType, CP_UTF8)));
        GEK_CHECK_EXCEPTION(rootNode == nullptr, Xml::Exception, "Unable to create root node: %S", rootType);

        xmlDocSetRootElement(static_cast<xmlDocPtr>(document), rootNode);

        return XmlDocument(document);
    }

    XmlDocument XmlDocument::load(LPCWSTR fileName, bool validateDTD)
    {
        CStringW expandedFileName(Gek::FileSystem::expandPath(fileName));
        xmlDocPtr document = xmlReadFile(CW2A(expandedFileName, CP_UTF8), nullptr, (validateDTD ? XML_PARSE_DTDATTR | XML_PARSE_DTDVALID : 0) | XML_PARSE_NOENT);
        GEK_CHECK_EXCEPTION(document == nullptr, Xml::Exception, "Unable to load document: %S", fileName);

        return XmlDocument(document);
    }

    void XmlDocument::save(LPCWSTR fileName)
    {
        GEK_CHECK_EXCEPTION(document == nullptr, Xml::Exception, "Invalid document encountered");

        CStringW expandedFileName(Gek::FileSystem::expandPath(fileName));
        xmlSaveFormatFileEnc(CW2A(expandedFileName, CP_UTF8), static_cast<xmlDocPtr>(document), "UTF-8", 1);
    }

    XmlNode XmlDocument::getRoot(void) const
    {
        GEK_CHECK_EXCEPTION(document == nullptr, Xml::Exception, "Invalid document encountered");

        return XmlNode(xmlDocGetRootElement(static_cast<xmlDocPtr>(document)));
    }
}; // namespace Gek
