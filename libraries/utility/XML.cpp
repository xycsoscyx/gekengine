#include "Utility\XML.h"
#include "Utility\FileSystem.h"
#include <libxml/parser.h>
#include <atlpath.h>

#pragma comment(lib, "libxml2.lib")

namespace Gek
{
    namespace Xml
    {
        Node::Node(xmlNode *node)
            : node(node)
        {
        }

        HRESULT Node::create(LPCWSTR type)
        {
            HRESULT returnValue = E_FAIL;

            CStringA strTypeUTF8 = CW2A(type, CP_UTF8);
            xmlNodePtr pNewNode = xmlNewNode(nullptr, BAD_CAST strTypeUTF8.GetString());
            if (pNewNode != nullptr)
            {
                node = pNewNode;
                returnValue = S_OK;
            }

            return returnValue;
        }

        void Node::setType(LPCWSTR type)
        {
            if (node != nullptr)
            {
                CStringA strTypeUTF8 = CW2A(type, CP_UTF8);
                xmlNodeSetName(node, BAD_CAST strTypeUTF8.GetString());
            }
        }

        CStringW Node::getType(void) const
        {
            CStringW type;
            if (node != nullptr)
            {
                type = CA2W((const char *)node->name, CP_UTF8);
            }

            return type;
        }

        CStringW Node::getText(void) const
        {
            CStringW text;
            if (node != nullptr)
            {
                xmlChar *content = xmlNodeGetContent(node);
                if (content != nullptr)
                {
                    text = CA2W((const char *)content, CP_UTF8);
                    xmlFree(content);
                }
            }

            return text;
        }

        void Node::setText(LPCWSTR format, ...)
        {
            if (node != nullptr)
            {
                CStringW text;
                if (format != nullptr)
                {
                    va_list pArgs;
                    va_start(pArgs, format);
                    text.FormatV(format, pArgs);
                    va_end(pArgs);
                }

                xmlNodeSetContent(node, BAD_CAST LPCSTR(CW2A(text, CP_UTF8)));
            }
        }

        bool Node::hasAttribute(LPCWSTR name) const
        {
            if (node != nullptr)
            {
                return (xmlHasProp(node, BAD_CAST LPCSTR(CW2A(name, CP_UTF8))) ? true : false);
            }

            return false;
        }

        CStringW Node::getAttribute(LPCWSTR name) const
        {
            CStringW value;
            if (node != nullptr)
            {
                xmlChar *attribute = xmlGetProp(node, BAD_CAST LPCSTR(CW2A(name, CP_UTF8)));
                if (attribute != nullptr)
                {
                    value = CA2W((const char *)attribute, CP_UTF8);
                    xmlFree(attribute);
                }
            }

            return value;
        }

        void Node::setAttribute(LPCWSTR name, LPCWSTR format, ...)
        {
            if (node != nullptr)
            {
                CStringW value;
                if (format != nullptr)
                {
                    va_list pArgs;
                    va_start(pArgs, format);
                    value.FormatV(format, pArgs);
                    va_end(pArgs);
                }

                if (hasAttribute(name))
                {
                    xmlSetProp(node, BAD_CAST LPCSTR(CW2A(name, CP_UTF8)), BAD_CAST LPCSTR(CW2A(value, CP_UTF8)));
                }
                else
                {
                    xmlNewProp(node, BAD_CAST LPCSTR(CW2A(name, CP_UTF8)), BAD_CAST LPCSTR(CW2A(value, CP_UTF8)));
                }
            }
        }

        void Node::listAttributes(std::function<void(LPCWSTR, LPCWSTR)> onAttribute) const
        {
            if (node != nullptr)
            {
                for (xmlAttrPtr attribute = node->properties; attribute != nullptr; attribute = attribute->next)
                {
                    CA2W name(LPCSTR(attribute->name), CP_UTF8);
                    onAttribute(name, getAttribute(name));
                }
            }
        }

        bool Node::hasSiblingElement(LPCWSTR type) const
        {
            bool hasSiblingElement = false;
            if (type != nullptr && node != nullptr)
            {
                CStringA typeUtf8(CW2A(type, CP_UTF8));
                for (xmlNode *checkingNode = node; checkingNode; checkingNode = checkingNode->next)
                {
                    if (checkingNode->type == XML_ELEMENT_NODE && typeUtf8 == checkingNode->name)
                    {
                        hasSiblingElement = true;
                        break;
                    }
                }
            }

            return hasSiblingElement;
        }

        Node Node::nextSiblingElement(LPCWSTR type) const
        {
            Node nextNode(nullptr);
            if (type != nullptr && node != nullptr)
            {
                CStringA typeUtf8(CW2A(type, CP_UTF8));
                for (xmlNode *checkingNode = node->next; checkingNode; checkingNode = checkingNode->next)
                {
                    if (checkingNode->type == XML_ELEMENT_NODE && typeUtf8 == checkingNode->name)
                    {
                        nextNode = Node(checkingNode);
                        break;
                    }
                }
            }

            return nextNode;
        }

        bool Node::hasChildElement(LPCWSTR type) const
        {
            bool hasChildElement = false;
            if (type != nullptr && node != nullptr)
            {
                CStringA typeUtf8(CW2A(type, CP_UTF8));
                for (xmlNode *checkingNode = node->children; checkingNode; checkingNode = checkingNode->next)
                {
                    if (checkingNode->type == XML_ELEMENT_NODE && typeUtf8 == checkingNode->name)
                    {
                        hasChildElement = true;
                        break;
                    }
                }
            }

            return hasChildElement;
        }

        Node Node::firstChildElement(LPCWSTR type) const
        {
            Node nextNode(nullptr);
            if (type != nullptr && node != nullptr)
            {
                CStringA typeUtf8(CW2A(type, CP_UTF8));
                for (xmlNode *checkingNode = node->children; checkingNode; checkingNode = checkingNode->next)
                {
                    if (checkingNode->type == XML_ELEMENT_NODE && typeUtf8 == checkingNode->name)
                    {
                        nextNode = Node(checkingNode);
                        break;
                    }
                }
            }

            return nextNode;
        }

        Node Node::createChildElement(LPCWSTR type, LPCWSTR format, ...)
        {
            Node newNode(nullptr);
            if (node != nullptr)
            {
                CStringW content;
                if (format != nullptr)
                {
                    va_list pArgs;
                    va_start(pArgs, format);
                    content.FormatV(format, pArgs);
                    va_end(pArgs);
                }

                xmlNodePtr childNode = xmlNewChild(node, nullptr, BAD_CAST LPCSTR(CW2A(type, CP_UTF8)), BAD_CAST LPCSTR(CW2A(content, CP_UTF8)));
                if (childNode != nullptr)
                {
                    xmlAddChild(node, childNode);
                    newNode = Node(childNode);
                }
            }

            return newNode;
        }

        Document::Document(void)
            : document(nullptr)
        {
        }

        Document::~Document(void)
        {
            if (document != nullptr)
            {
                xmlFreeDoc(document);
            }
        }

        HRESULT Document::create(LPCWSTR rootType)
        {
            if (document != nullptr)
            {
                xmlFreeDoc(document);
                document = nullptr;
            }

            HRESULT returnValue = E_FAIL;
            document = xmlNewDoc(BAD_CAST "1.0");
            if (document != nullptr)
            {
                xmlNodePtr rootNode = xmlNewNode(nullptr, BAD_CAST LPCSTR(CW2A(rootType, CP_UTF8)));
                if (rootNode != nullptr)
                {
                    xmlDocSetRootElement(document, rootNode);
                    returnValue = S_OK;
                }
            }

            return returnValue;
        }

        HRESULT Document::load(LPCWSTR basePath)
        {
            if (document != nullptr)
            {
                xmlFreeDoc(document);
                document = nullptr;
            }

            HRESULT returnValue = E_FAIL;
            CStringW fullPath(Gek::FileSystem::expandPath(basePath));
            document = xmlReadFile(CW2A(fullPath, CP_UTF8), nullptr, XML_PARSE_DTDATTR | XML_PARSE_NOENT | XML_PARSE_DTDVALID);
            if (document != nullptr)
            {
                returnValue = S_OK;
            }

            return returnValue;
        }

        HRESULT Document::save(LPCWSTR basePath)
        {
            HRESULT returnValue = E_FAIL;
            if (document != nullptr)
            {
                CStringW fullPath(Gek::FileSystem::expandPath(basePath));
                xmlSaveFormatFileEnc(CW2A(fullPath, CP_UTF8), document, "UTF-8", 1);
            }

            return returnValue;
        }

        Node Document::getRoot(void) const
        {
            if (document != nullptr)
            {
                return Node(xmlDocGetRootElement(document));
            }

            return Node(nullptr);
        }
    }; // namespace Xml
}; // namespace Gek
