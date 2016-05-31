#include "GEK\Utility\XML.h"
#include "GEK\Utility\FileSystem.h"
#include <libxml/tree.h>
#include <libxml/parser.h>
#include <atlpath.h>

#pragma comment(lib, "libxml2.lib")

namespace Gek
{
    struct XmlConstString
    {
        const xmlChar *text;

        XmlConstString(const xmlChar *text)
            : text(text)
        {
        }

        operator wstring () const
        {
            return reinterpret_cast<const char *>(text);
        }
    };

    struct XmlString
    {
        xmlChar *text;

        XmlString(xmlChar *text)
            : text(text)
        {
        }

        ~XmlString(void)
        {
            xmlFree(text);
        }

        operator wstring () const
        {
            return reinterpret_cast<const char *>(text);
        }
    };

    struct XmlCast
    {
        string text;
        XmlCast(const wstring &text)
            : text(text)
        {
        }

        operator const xmlChar *() const
        {
            return BAD_CAST text.c_str();
        }
    };

    int compare(const char *source, const xmlChar *destination)
    {
        return strcmp(source, reinterpret_cast<const char *>(destination));
    }

    class XmlDummyNode
        : public XmlNode
    {
        bool isValid(void) const
        {
            return false;
        }

        wstring getType(void) const
        {
            return nullptr;
        }

        wstring getText(void) const
        {
            return nullptr;
        }

        void setText(const wstring &text)
        {
        }

        bool hasAttribute(const wstring &name) const
        {
            return false;
        }

        wstring getAttribute(const wstring &name, const wstring &defaultValue) const
        {
            return defaultValue;
        }

        void setAttribute(const wstring &name, const wstring &value)
        {
        }

        void listAttributes(std::function<void(const wstring &, const wstring &)> onAttribute) const
        {
        }

        bool hasSiblingElement(const wstring &type) const
        {
            return false;
        }

        XmlNodePtr nextSiblingElement(const wstring &type) const
        {
            return std::dynamic_pointer_cast<XmlNode>(std::make_shared<XmlDummyNode>());
        }

        bool hasChildElement(const wstring &type) const
        {
            return false;
        }

        XmlNodePtr firstChildElement(const wstring &type, bool create)
        {
            return std::dynamic_pointer_cast<XmlNode>(std::make_shared<XmlDummyNode>());
        }

        XmlNodePtr createChildElement(const wstring &type, const wstring &content)
        {
            return std::dynamic_pointer_cast<XmlNode>(std::make_shared<XmlDummyNode>());
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

        wstring getType(void) const
        {
            return XmlConstString(node->name);
        }

        wstring getText(void) const
        {
            return XmlString(xmlNodeGetContent(node));
        }

        void setText(const wstring &text)
        {
            xmlNodeSetContent(node, XmlCast(text));
        }

        bool hasAttribute(const wstring &name) const
        {
            return (xmlHasProp(node, XmlCast(name)) ? true : false);
        }

        wstring getAttribute(const wstring &name, const wstring &defaultValue) const
        {
            if (hasAttribute(name))
            {
                return XmlString(xmlGetProp(node, XmlCast(name)));
            }
            else
            {
                return defaultValue;
            }
        }

        void setAttribute(const wstring &name, const wstring &value)
        {
            if (hasAttribute(name))
            {
                xmlSetProp(node, XmlCast(name), XmlCast(value));
            }
            else
            {
                xmlNewProp(node, XmlCast(name), XmlCast(value));
            }
        }

        void listAttributes(std::function<void(const wstring &, const wstring &)> onAttribute) const
        {
            for (xmlAttrPtr attribute = node->properties; attribute != nullptr; attribute = attribute->next)
            {
                wstring name(XmlConstString(attribute->name));
                wstring value(XmlString(xmlGetProp(node, attribute->name)));
                onAttribute(name, value);
            }
        }

        bool hasSiblingElement(const wstring &type) const
        {
            string typeUTF8(type);
            for (xmlNode *checkingNode = node->next; checkingNode; checkingNode = checkingNode->next)
            {
                if (checkingNode->type == XML_ELEMENT_NODE && (!type || compare(typeUTF8, checkingNode->name) == 0))
                {
                    return true;
                }
            }

            return false;
        }

        XmlNodePtr nextSiblingElement(const wstring &type) const
        {
            string typeUTF8(type);
            for (xmlNodePtr checkingNode = node->next; checkingNode; checkingNode = checkingNode->next)
            {
                if (checkingNode->type == XML_ELEMENT_NODE && (!type || compare(typeUTF8, checkingNode->name) == 0))
                {
                    return std::dynamic_pointer_cast<XmlNode>(std::make_shared<XmlNodeImplementation>(checkingNode));
                }
            }

            return std::dynamic_pointer_cast<XmlNode>(std::make_shared<XmlDummyNode>());
        }

        bool hasChildElement(const wstring &type) const
        {
            string typeUTF8(type);
            for (xmlNode *checkingNode = node->children; checkingNode; checkingNode = checkingNode->next)
            {
                if (checkingNode->type == XML_ELEMENT_NODE && (!type || compare(typeUTF8, checkingNode->name) == 0))
                {
                    return true;
                }
            }

            return false;
        }

        XmlNodePtr firstChildElement(const wstring &type, bool create)
        {
            string typeUTF8(type);
            for (xmlNodePtr checkingNode = node->children; checkingNode; checkingNode = checkingNode->next)
            {
                if (checkingNode->type == XML_ELEMENT_NODE && (!type || compare(typeUTF8, checkingNode->name) == 0))
                {
                    return std::dynamic_pointer_cast<XmlNode>(std::make_shared<XmlNodeImplementation>(checkingNode));
                }
            }

            if (create)
            {
                return createChildElement(type);
            }
            else
            {
                return std::dynamic_pointer_cast<XmlNode>(std::make_shared<XmlDummyNode>());
            }
        }

        XmlNodePtr createChildElement(const wstring &type, const wstring &content = wstring())
        {
            xmlNodePtr childNode = xmlNewChild(node, nullptr, XmlCast(type), XmlCast(content));
            GEK_CHECK_CONDITION(childNode == nullptr, Xml::Exception, "Unable to create new child node: %v (%v)", type, content);

            xmlAddChild(node, childNode);

            return std::dynamic_pointer_cast<XmlNode>(std::make_shared<XmlNodeImplementation>(childNode));
        }
    };

    XmlNodePtr XmlNode::create(const wstring &type)
    {
        xmlNodePtr node = xmlNewNode(nullptr, XmlCast(type));
        GEK_CHECK_CONDITION(node == nullptr, Xml::Exception, "Unable to create node: %v", type);

        return std::dynamic_pointer_cast<XmlNode>(std::make_shared<XmlNodeImplementation>(node));
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

        void save(const wstring &fileName)
        {
            string expandedFileName(FileSystem::expandPath(fileName));
            xmlSaveFormatFileEnc(expandedFileName, document, "UTF-8", 1);
        }

        XmlNodePtr getRoot(const wstring &type) const
        {
            xmlNodePtr root = xmlDocGetRootElement(document);
            GEK_CHECK_CONDITION(root == nullptr, Xml::Exception, "Unable to get document root node");

            wstring rootType(XmlConstString(root->name));
            GEK_CHECK_CONDITION(rootType.compare(type) != 0, Xml::Exception, "Document root node type doesn't match: (%v vs %v)", type, rootType);

            return std::dynamic_pointer_cast<XmlNode>(std::make_shared<XmlNodeImplementation>(root));
        }
    };

    XmlDocumentPtr XmlDocument::create(const wstring &type)
    {
        xmlDocPtr document = xmlNewDoc(BAD_CAST "1.0");
        GEK_CHECK_CONDITION(document == nullptr, Xml::Exception, "Unable to create new document");

        xmlNodePtr rootNode = xmlNewNode(nullptr, XmlCast(type));
        GEK_CHECK_CONDITION(rootNode == nullptr, Xml::Exception, "Unable to create root node: %v", type);

        xmlDocSetRootElement(static_cast<xmlDocPtr>(document), rootNode);

        return std::dynamic_pointer_cast<XmlDocument>(std::make_shared<XmlDocumentImplementation>(document));
    }

    XmlDocumentPtr XmlDocument::load(const wstring &fileName, bool validateDTD)
    {
        string fileNameUTF8(FileSystem::expandPath(fileName));
        xmlDocPtr document = xmlReadFile(fileNameUTF8, nullptr, (validateDTD ? XML_PARSE_DTDATTR | XML_PARSE_DTDVALID : 0) | XML_PARSE_NOENT);
        GEK_CHECK_CONDITION(document == nullptr, Xml::Exception, "Unable to load document: %v", fileName);

        return std::dynamic_pointer_cast<XmlDocument>(std::make_shared<XmlDocumentImplementation>(document));
    }
}; // namespace Gek
