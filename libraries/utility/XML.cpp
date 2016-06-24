#include "GEK\Utility\XML.h"
#include "GEK\Utility\FileSystem.h"
#include <libxml/tree.h>
#include <libxml/parser.h>
#include <atlpath.h>

#pragma comment(lib, "libxml2.lib")

namespace Gek
{
    class XmlDummyNode
        : public XmlNode
    {
        bool isValid(void) const
        {
            return false;
        }

        String getType(void) const
        {
            return String();
        }

        String getText(void) const
        {
            return String();
        }

        void setText(const wchar_t *text)
        {
        }

        bool hasAttribute(const wchar_t *name) const
        {
            return false;
        }

        String getAttribute(const wchar_t *name, const wchar_t *defaultValue) const
        {
            return defaultValue;
        }

        void setAttribute(const wchar_t *name, const wchar_t *value)
        {
        }

        void listAttributes(std::function<void(const wchar_t *, const wchar_t *)> onAttribute) const
        {
        }

        bool hasSiblingElement(const wchar_t *type) const
        {
            return false;
        }

        XmlNodePtr nextSiblingElement(const wchar_t *type) const
        {
            return makeShared<XmlNode, XmlDummyNode>();
        }

        bool hasChildElement(const wchar_t *type) const
        {
            return false;
        }

        XmlNodePtr firstChildElement(const wchar_t *type, bool create)
        {
            return makeShared<XmlNode, XmlDummyNode>();
        }

        XmlNodePtr createChildElement(const wchar_t *type, const wchar_t *content)
        {
            return makeShared<XmlNode, XmlDummyNode>();
        }
    };

    class XmlNodeImplementation
        : public XmlNode
    {
    private:
        xmlNodePtr node;

    public:
        XmlNodeImplementation(xmlNodePtr node)
            : node(node)
        {
            GEK_REQUIRE(node);
        }

        ~XmlNodeImplementation(void)
        {
        }

        bool isValid(void) const
        {
            return true;
        }

        String getType(void) const
        {
            return reinterpret_cast<const char *>(node->name);
        }

        String getText(void) const
        {
            xmlChar *textUTF8 = xmlNodeGetContent(node);
            String text(reinterpret_cast<const char *>(textUTF8));
            xmlFree(textUTF8);
            return text;
        }

        void setText(const wchar_t *text)
        {
            xmlNodeSetContent(node, BAD_CAST StringUTF8(text).c_str());
        }

        bool hasAttribute(const wchar_t *name) const
        {
            return (xmlHasProp(node, BAD_CAST StringUTF8(name).c_str()) ? true : false);
        }

        String getAttribute(const wchar_t *name, const wchar_t *defaultValue) const
        {
            StringUTF8 nameUTF8(name);
            if (xmlHasProp(node, BAD_CAST nameUTF8.c_str()))
            {
                xmlChar *textUTF8 = xmlGetProp(node, BAD_CAST nameUTF8.c_str());
                if (textUTF8)
                {
                    String text(reinterpret_cast<const char *>(textUTF8));
                    xmlFree(textUTF8);
                    return text;
                }
            }

            return defaultValue;
        }

        void setAttribute(const wchar_t *name, const wchar_t *value)
        {
            if (hasAttribute(name))
            {
                xmlSetProp(node, BAD_CAST StringUTF8(name).c_str(), BAD_CAST StringUTF8(value).c_str());
            }
            else
            {
                xmlNewProp(node, BAD_CAST StringUTF8(name).c_str(), BAD_CAST StringUTF8(value).c_str());
            }
        }

        void listAttributes(std::function<void(const wchar_t *, const wchar_t *)> onAttribute) const
        {
            for (xmlAttrPtr attribute = node->properties; attribute != nullptr; attribute = attribute->next)
            {
                String name(reinterpret_cast<const char *>(attribute->name));
                
                xmlChar *valueUTF8 = xmlGetProp(node, attribute->name);
                String value(reinterpret_cast<const char *>(valueUTF8));
                xmlFree(valueUTF8);

                onAttribute(name, value);
            }
        }

        bool hasSiblingElement(const wchar_t *type) const
        {
            StringUTF8 typeUTF8(type);
            for (xmlNodePtr checkingNode = node->next; checkingNode; checkingNode = checkingNode->next)
            {
                if (checkingNode->type == XML_ELEMENT_NODE && typeUTF8.compareNoCase(reinterpret_cast<const char *>(checkingNode->name)) == 0)
                {
                    return true;
                }
            }

            return false;
        }

        XmlNodePtr nextSiblingElement(const wchar_t *type) const
        {
            StringUTF8 typeUTF8(type);
            for (xmlNodePtr checkingNode = node->next; checkingNode; checkingNode = checkingNode->next)
            {
                if (checkingNode->type == XML_ELEMENT_NODE && (type == nullptr || typeUTF8.compareNoCase(reinterpret_cast<const char *>(checkingNode->name)) == 0))
                {
                    return makeShared<XmlNode, XmlNodeImplementation>(checkingNode);
                }
            }

            return makeShared<XmlNode, XmlDummyNode>();
        }

        bool hasChildElement(const wchar_t *type) const
        {
            StringUTF8 typeUTF8(type);
            for (xmlNodePtr checkingNode = node->children; checkingNode; checkingNode = checkingNode->next)
            {
                if (checkingNode->type == XML_ELEMENT_NODE && typeUTF8.compareNoCase(reinterpret_cast<const char *>(checkingNode->name)) == 0)
                {
                    return true;
                }
            }

            return false;
        }

        XmlNodePtr firstChildElement(const wchar_t *type, bool create)
        {
            StringUTF8 typeUTF8(type);
            for (xmlNodePtr checkingNode = node->children; checkingNode; checkingNode = checkingNode->next)
            {
                if (checkingNode->type == XML_ELEMENT_NODE && (type == nullptr || typeUTF8.compareNoCase(reinterpret_cast<const char *>(checkingNode->name)) == 0))
                {
                    return makeShared<XmlNode, XmlNodeImplementation>(checkingNode);
                }
            }

            if (create && type)
            {
                return createChildElement(type);
            }
            else
            {
                return makeShared<XmlNode, XmlDummyNode>();
            }
        }

        XmlNodePtr createChildElement(const wchar_t *type, const wchar_t *content = nullptr)
        {
            xmlNodePtr childNode = xmlNewChild(node, nullptr, BAD_CAST StringUTF8(type).c_str(), BAD_CAST StringUTF8(content).c_str());
            GEK_CHECK_CONDITION(childNode == nullptr, Xml::Exception, "Unable to create new child node: %v (%v)", type, content);

            return makeShared<XmlNode, XmlNodeImplementation>(childNode);
        }
    };

    XmlNodePtr XmlNode::create(const wchar_t *type)
    {
        xmlNodePtr node = xmlNewNode(nullptr, BAD_CAST StringUTF8(type).c_str());
        GEK_CHECK_CONDITION(node == nullptr, Xml::Exception, "Unable to create node: %v", type);

        return makeShared<XmlNode, XmlNodeImplementation>(node);
    }

    class XmlDocumentImplementation
        : public XmlDocument
    {
    private:
        xmlDocPtr document;

    public:
        XmlDocumentImplementation(xmlDocPtr document)
            : document(document)
        {
            GEK_REQUIRE(document);
        }

        ~XmlDocumentImplementation(void)
        {
            xmlFreeDoc(document);
        }

        void save(const wchar_t *fileName)
        {
            StringUTF8 expandedFileName(FileSystem::expandPath(fileName));
            xmlSaveFormatFileEnc(expandedFileName, document, "UTF-8", 1);
        }

        XmlNodePtr getRoot(const wchar_t *type) const
        {
            xmlNodePtr root = xmlDocGetRootElement(document);
            GEK_CHECK_CONDITION(root == nullptr, Xml::Exception, "Unable to get document root node");

            String rootType(reinterpret_cast<const char *>(root->name));
            GEK_CHECK_CONDITION(rootType.compareNoCase(type) != 0, Xml::Exception, "Document root node type doesn't match: (%v vs %v)", type, rootType);

            return makeShared<XmlNode, XmlNodeImplementation>(root);
        }
    };

    XmlDocumentPtr XmlDocument::create(const wchar_t *type)
    {
        xmlDocPtr document = xmlNewDoc(BAD_CAST "1.0");
        GEK_CHECK_CONDITION(document == nullptr, Xml::Exception, "Unable to create new document");

        xmlNodePtr rootNode = xmlNewNode(nullptr, BAD_CAST StringUTF8(type).c_str());
        GEK_CHECK_CONDITION(rootNode == nullptr, Xml::Exception, "Unable to create root node: %v", type);

        xmlDocSetRootElement(static_cast<xmlDocPtr>(document), rootNode);

        return makeShared<XmlDocument, XmlDocumentImplementation>(document);
    }

    XmlDocumentPtr XmlDocument::load(const wchar_t *fileName, bool validateDTD)
    {
        StringUTF8 fileNameUTF8(FileSystem::expandPath(fileName));
        xmlDocPtr document = xmlReadFile(fileNameUTF8, nullptr, (validateDTD ? XML_PARSE_DTDATTR | XML_PARSE_DTDVALID : 0) | XML_PARSE_NOENT);
        GEK_CHECK_CONDITION(document == nullptr, Xml::Exception, "Unable to load document: %v", fileName);

        return makeShared<XmlDocument, XmlDocumentImplementation>(document);
    }
}; // namespace Gek
