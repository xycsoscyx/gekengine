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
            return String::from<wchar_t>(reinterpret_cast<const char *>(text));
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
            return String::from<wchar_t>(reinterpret_cast<const char *>(text));
        }
    };

    struct XmlCast
    {
        string text;
        XmlCast(const wchar_t *text)
        {
            if (text)
            {
                this->text = String::from<char>(text);
            }
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

        void setText(const wchar_t *text)
        {
        }

        bool hasAttribute(const wchar_t *name) const
        {
            return false;
        }

        wstring getAttribute(const wchar_t *name, const wstring &defaultValue) const
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
            return std::dynamic_pointer_cast<XmlNode>(std::make_shared<XmlDummyNode>());
        }

        bool hasChildElement(const wchar_t *type) const
        {
            return false;
        }

        XmlNodePtr firstChildElement(const wchar_t *type, bool create)
        {
            return std::dynamic_pointer_cast<XmlNode>(std::make_shared<XmlDummyNode>());
        }

        XmlNodePtr createChildElement(const wchar_t *type, const wchar_t *content)
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

        void setText(const wchar_t *text)
        {
            xmlNodeSetContent(node, XmlCast(text));
        }

        bool hasAttribute(const wchar_t *name) const
        {
            return (xmlHasProp(node, XmlCast(name)) ? true : false);
        }

        wstring getAttribute(const wchar_t *name, const wstring &defaultValue) const
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

        void setAttribute(const wchar_t *name, const wchar_t *value)
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

        void listAttributes(std::function<void(const wchar_t *, const wchar_t *)> onAttribute) const
        {
            for (xmlAttrPtr attribute = node->properties; attribute != nullptr; attribute = attribute->next)
            {
                wstring name(XmlConstString(attribute->name));
                wstring value(XmlString(xmlGetProp(node, attribute->name)));
                onAttribute(name, value);
            }
        }

        bool hasSiblingElement(const wchar_t *type) const
        {
            string typeUTF8(String::from<char>(type));
            for (xmlNode *checkingNode = node->next; checkingNode; checkingNode = checkingNode->next)
            {
                if (checkingNode->type == XML_ELEMENT_NODE && (!type || compare(typeUTF8, checkingNode->name) == 0))
                {
                    return true;
                }
            }

            return false;
        }

        XmlNodePtr nextSiblingElement(const wchar_t *type) const
        {
            string typeUTF8(String::from<char>(type));
            for (xmlNodePtr checkingNode = node->next; checkingNode; checkingNode = checkingNode->next)
            {
                if (checkingNode->type == XML_ELEMENT_NODE && (!type || compare(typeUTF8, checkingNode->name) == 0))
                {
                    return std::dynamic_pointer_cast<XmlNode>(std::make_shared<XmlNodeImplementation>(checkingNode));
                }
            }

            return std::dynamic_pointer_cast<XmlNode>(std::make_shared<XmlDummyNode>());
        }

        bool hasChildElement(const wchar_t *type) const
        {
            string typeUTF8(String::from<char>(type));
            for (xmlNode *checkingNode = node->children; checkingNode; checkingNode = checkingNode->next)
            {
                if (checkingNode->type == XML_ELEMENT_NODE && (!type || compare(typeUTF8, checkingNode->name) == 0))
                {
                    return true;
                }
            }

            return false;
        }

        XmlNodePtr firstChildElement(const wchar_t *type, bool create)
        {
            string typeUTF8(String::from<char>(type));
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

        XmlNodePtr createChildElement(const wchar_t *type, const wchar_t *content = nullptr)
        {
            xmlNodePtr childNode = xmlNewChild(node, nullptr, XmlCast(type), XmlCast(content));
            GEK_THROW_ERROR(childNode == nullptr, Xml::Exception, "Unable to create new child node: %v (%v)", type, content);

            xmlAddChild(node, childNode);

            return std::dynamic_pointer_cast<XmlNode>(std::make_shared<XmlNodeImplementation>(childNode));
        }
    };

    XmlNodePtr XmlNode::create(const wchar_t *type)
    {
        xmlNodePtr node = xmlNewNode(nullptr, XmlCast(type));
        GEK_THROW_ERROR(node == nullptr, Xml::Exception, "Unable to create node: %v", type);

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

        void save(const wchar_t *fileName)
        {
            wstring expandedFileName(Gek::FileSystem::expandPath(fileName));
            xmlSaveFormatFileEnc(String::from<char>(fileName), document, "UTF-8", 1);
        }

        XmlNodePtr getRoot(const wchar_t *type) const
        {
            xmlNodePtr root = xmlDocGetRootElement(document);
            GEK_THROW_ERROR(root == nullptr, Xml::Exception, "Unable to get document root node");

            wstring rootType(XmlConstString(root->name));
            GEK_THROW_ERROR(rootType.compare(type) != 0, Xml::Exception, "Document root node type doesn't match: (%v vs %v)", type, rootType);

            return std::dynamic_pointer_cast<XmlNode>(std::make_shared<XmlNodeImplementation>(root));
        }
    };

    XmlDocumentPtr XmlDocument::create(const wchar_t *type)
    {
        xmlDocPtr document = xmlNewDoc(BAD_CAST "1.0");
        GEK_THROW_ERROR(document == nullptr, Xml::Exception, "Unable to create new document");

        xmlNodePtr rootNode = xmlNewNode(nullptr, XmlCast(type));
        GEK_THROW_ERROR(rootNode == nullptr, Xml::Exception, "Unable to create root node: %v", type);

        xmlDocSetRootElement(static_cast<xmlDocPtr>(document), rootNode);

        return std::dynamic_pointer_cast<XmlDocument>(std::make_shared<XmlDocumentImplementation>(document));
    }

    XmlDocumentPtr XmlDocument::load(const wchar_t *fileName, bool validateDTD)
    {
        string fileNameUTF8(String::from<char>(Gek::FileSystem::expandPath(fileName)));
        xmlDocPtr document = xmlReadFile(fileNameUTF8, nullptr, (validateDTD ? XML_PARSE_DTDATTR | XML_PARSE_DTDVALID : 0) | XML_PARSE_NOENT);
        GEK_THROW_ERROR(document == nullptr, Xml::Exception, "Unable to load document: %v", fileName);

        return std::dynamic_pointer_cast<XmlDocument>(std::make_shared<XmlDocumentImplementation>(document));
    }
}; // namespace Gek
