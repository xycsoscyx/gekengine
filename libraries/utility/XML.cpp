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
        GEK_THROW_ERROR(node == nullptr, Xml::Exception, "Gek::Xml::initialize() not called");
    }

    XmlNode::XmlNode(LPVOID node)
        : node(node)
    {
        GEK_THROW_ERROR(node == nullptr, Xml::Exception, "Invalid node encountered");
    }

    XmlNode::~XmlNode(void)
    {
    }

    XmlNode XmlNode::create(const wchar_t *type)
    {
        xmlNodePtr node = xmlNewNode(nullptr, BAD_CAST static_cast<const char *>(CW2A(type, CP_UTF8)));
        GEK_THROW_ERROR(node == nullptr, Xml::Exception, "Unable to create node: %", type);
        return XmlNode(node);
    }

    XmlNode::operator bool() const
    {
        return (node != dummyNode);
    }

    wstring XmlNode::getType(void) const
    {
        GEK_THROW_ERROR(node == nullptr, Xml::Exception, "Invalid node encountered");
        return static_cast<const wchar_t *>(CA2W(reinterpret_cast<const char *>(static_cast<xmlNodePtr>(node)->name), CP_UTF8));
    }

    wstring XmlNode::getText(void) const
    {
        GEK_THROW_ERROR(node == nullptr, Xml::Exception, "Invalid node encountered");

        xmlChar *content = xmlNodeGetContent(static_cast<xmlNodePtr>(node));
        GEK_THROW_ERROR(content == nullptr, Xml::Exception, "Unable to get node content");

        CA2W text(reinterpret_cast<const char *>(content), CP_UTF8);
        xmlFree(content);
        return static_cast<const wchar_t *>(text);
    }

    void XmlNode::setText(const wchar_t *formatting, ...)
    {
        GEK_THROW_ERROR(node == nullptr, Xml::Exception, "Invalid node encountered");

        wstring text;
        if (formatting != nullptr)
        {
            va_list variableList;
            va_start(variableList, formatting);
            text.format(formatting, variableList);
            va_end(variableList);
        }

        xmlNodeSetContent(static_cast<xmlNodePtr>(node), BAD_CAST static_cast<const char *>(CW2A(text, CP_UTF8)));
    }

    bool XmlNode::hasAttribute(const wchar_t *name) const
    {
        GEK_THROW_ERROR(node == nullptr, Xml::Exception, "Invalid node encountered");
        return (xmlHasProp(static_cast<xmlNodePtr>(node), BAD_CAST static_cast<const char *>(CW2A(name, CP_UTF8))) ? true : false);
    }

    wstring XmlNode::getAttribute(const wchar_t *name) const
    {
        GEK_THROW_ERROR(node == nullptr, Xml::Exception, "Invalid node encountered");

        xmlChar *attribute = xmlGetProp(static_cast<xmlNodePtr>(node), BAD_CAST static_cast<const char *>(CW2A(name, CP_UTF8)));
        GEK_THROW_ERROR(attribute == nullptr, Xml::Exception, "Unable to get node attribute: %", name);

        CA2W value(reinterpret_cast<const char *>(attribute), CP_UTF8);
        xmlFree(attribute);
        return static_cast<const wchar_t *>(value);
    }

    void XmlNode::setAttribute(const wchar_t *name, const wchar_t *formatting, ...)
    {
        GEK_THROW_ERROR(node == nullptr, Xml::Exception, "Invalid node encountered");

        wstring value;
        if (formatting != nullptr)
        {
            va_list variableList;
            va_start(variableList, formatting);
            value.format(formatting, variableList);
            va_end(variableList);
        }

        if (hasAttribute(name))
        {
            xmlSetProp(static_cast<xmlNodePtr>(node), BAD_CAST static_cast<const char *>(CW2A(name, CP_UTF8)), BAD_CAST static_cast<const char *>(CW2A(value, CP_UTF8)));
        }
        else
        {
            xmlNewProp(static_cast<xmlNodePtr>(node), BAD_CAST static_cast<const char *>(CW2A(name, CP_UTF8)), BAD_CAST static_cast<const char *>(CW2A(value, CP_UTF8)));
        }
    }

    void XmlNode::listAttributes(std::function<void(const wchar_t *, const wchar_t *)> onAttribute) const
    {
        GEK_THROW_ERROR(node == nullptr, Xml::Exception, "Invalid node encountered");

        for (xmlAttrPtr attribute = static_cast<xmlNodePtr>(node)->properties; attribute != nullptr; attribute = attribute->next)
        {
            CA2W name(reinterpret_cast<const char *>(attribute->name), CP_UTF8);
            onAttribute(name, getAttribute(name));
        }
    }

    bool XmlNode::hasSiblingElement(const wchar_t *type) const
    {
        GEK_THROW_ERROR(node == nullptr, Xml::Exception, "Invalid node encountered");

        bool hasSiblingElement = false;
        CW2A typeUtf8(type, CP_UTF8);
        for (xmlNode *checkingNode = static_cast<xmlNodePtr>(node)->next; checkingNode; checkingNode = checkingNode->next)
        {
            if (checkingNode->type == XML_ELEMENT_NODE && (!type || strcmp(typeUtf8, reinterpret_cast<const char *>(checkingNode->name)) == 0))
            {
                hasSiblingElement = true;
                break;
            }
        }

        return hasSiblingElement;
    }

    XmlNode XmlNode::nextSiblingElement(const wchar_t *type) const
    {
        GEK_THROW_ERROR(node == nullptr, Xml::Exception, "Invalid node encountered");

        CW2A typeUtf8(type, CP_UTF8);
        for (xmlNodePtr checkingNode = static_cast<xmlNodePtr>(node)->next; checkingNode; checkingNode = checkingNode->next)
        {
            if (checkingNode->type == XML_ELEMENT_NODE && (!type || strcmp(typeUtf8, reinterpret_cast<const char *>(checkingNode->name)) == 0))
            {
                return XmlNode(checkingNode);
            }
        }

        return XmlNode();
    }

    bool XmlNode::hasChildElement(const wchar_t *type) const
    {
        GEK_THROW_ERROR(node == nullptr, Xml::Exception, "Invalid node encountered");

        bool hasChildElement = false;
        CW2A typeUtf8(type, CP_UTF8);
        for (xmlNode *checkingNode = static_cast<xmlNodePtr>(node)->children; checkingNode; checkingNode = checkingNode->next)
        {
            if (checkingNode->type == XML_ELEMENT_NODE && (!type || strcmp(typeUtf8, reinterpret_cast<const char *>(checkingNode->name)) == 0))
            {
                hasChildElement = true;
                break;
            }
        }

        return hasChildElement;
    }

    XmlNode XmlNode::firstChildElement(const wchar_t *type) const
    {
        GEK_THROW_ERROR(node == nullptr, Xml::Exception, "Invalid node encountered");

        CW2A typeUtf8(type, CP_UTF8);
        for (xmlNodePtr checkingNode = static_cast<xmlNodePtr>(node)->children; checkingNode; checkingNode = checkingNode->next)
        {
            if (checkingNode->type == XML_ELEMENT_NODE && (!type || strcmp(typeUtf8, reinterpret_cast<const char *>(checkingNode->name)) == 0))
            {
                return XmlNode(checkingNode);
            }
        }

        return XmlNode();
    }

    XmlNode XmlNode::createChildElement(const wchar_t *type, const wchar_t *formatting, ...)
    {
        GEK_THROW_ERROR(node == nullptr, Xml::Exception, "Invalid node encountered");

        wstring content;
        if (formatting != nullptr)
        {
            va_list variableList;
            va_start(variableList, formatting);
            content.format(formatting, variableList);
            va_end(variableList);
        }

        xmlNodePtr childNode = xmlNewChild(static_cast<xmlNodePtr>(node), nullptr, BAD_CAST static_cast<const char *>(CW2A(type, CP_UTF8)), BAD_CAST static_cast<const char *>(CW2A(content, CP_UTF8)));
        GEK_THROW_ERROR(childNode == nullptr, Xml::Exception, "Unable to create new child node: % (%)", type, content);

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

    XmlDocument XmlDocument::create(const wchar_t *rootType)
    {
        xmlDocPtr document = xmlNewDoc(BAD_CAST "1.0");
        GEK_THROW_ERROR(document == nullptr, Xml::Exception, "Unable to create new document");

        xmlNodePtr rootNode = xmlNewNode(nullptr, BAD_CAST static_cast<const char *>(CW2A(rootType, CP_UTF8)));
        GEK_THROW_ERROR(rootNode == nullptr, Xml::Exception, "Unable to create root node: %", rootType);

        xmlDocSetRootElement(static_cast<xmlDocPtr>(document), rootNode);

        return XmlDocument(document);
    }

    XmlDocument XmlDocument::load(const wchar_t *fileName, bool validateDTD)
    {
        wstring expandedFileName(Gek::FileSystem::expandPath(fileName));
        xmlDocPtr document = xmlReadFile(CW2A(expandedFileName, CP_UTF8), nullptr, (validateDTD ? XML_PARSE_DTDATTR | XML_PARSE_DTDVALID : 0) | XML_PARSE_NOENT);
        GEK_THROW_ERROR(document == nullptr, Xml::Exception, "Unable to load document: %", fileName);

        return XmlDocument(document);
    }

    void XmlDocument::save(const wchar_t *fileName)
    {
        GEK_THROW_ERROR(document == nullptr, Xml::Exception, "Invalid document encountered");

        wstring expandedFileName(Gek::FileSystem::expandPath(fileName));
        xmlSaveFormatFileEnc(CW2A(expandedFileName, CP_UTF8), static_cast<xmlDocPtr>(document), "UTF-8", 1);
    }

    XmlNode XmlDocument::getRoot(void) const
    {
        GEK_THROW_ERROR(document == nullptr, Xml::Exception, "Invalid document encountered");

        return XmlNode(xmlDocGetRootElement(static_cast<xmlDocPtr>(document)));
    }
}; // namespace Gek
