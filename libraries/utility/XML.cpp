#include "GEK\Utility\XML.h"
#include "GEK\Utility\FileSystem.h"
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
            HRESULT resultValue = E_FAIL;

            CStringA strTypeUTF8 = CW2A(type, CP_UTF8);
            xmlNodePtr pNewNode = xmlNewNode(nullptr, BAD_CAST strTypeUTF8.GetString());
            if (pNewNode != nullptr)
            {
                node = pNewNode;
                resultValue = S_OK;
            }

            return resultValue;
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
                    va_list variableList;
                    va_start(variableList, format);
                    text.FormatV(format, variableList);
                    va_end(variableList);
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
                    va_list variableList;
                    va_start(variableList, format);
                    value.FormatV(format, variableList);
                    va_end(variableList);
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
            if (node != nullptr)
            {
                CStringA typeUtf8(CW2A(type, CP_UTF8));
                for (xmlNode *checkingNode = node->next; checkingNode; checkingNode = checkingNode->next)
                {
                    if (checkingNode->type == XML_ELEMENT_NODE && (!type || typeUtf8 == checkingNode->name))
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
            if (node != nullptr)
            {
                CStringA typeUtf8(CW2A(type, CP_UTF8));
                for (xmlNode *checkingNode = node->next; checkingNode; checkingNode = checkingNode->next)
                {
                    if (checkingNode->type == XML_ELEMENT_NODE && (!type || typeUtf8 == checkingNode->name))
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
            if (node != nullptr)
            {
                CStringA typeUtf8(CW2A(type, CP_UTF8));
                for (xmlNode *checkingNode = node->children; checkingNode; checkingNode = checkingNode->next)
                {
                    if (checkingNode->type == XML_ELEMENT_NODE && (!type || typeUtf8 == checkingNode->name))
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
            Node childNode(nullptr);
            if (node != nullptr)
            {
                CStringA typeUtf8(CW2A(type, CP_UTF8));
                for (xmlNode *checkingNode = node->children; checkingNode; checkingNode = checkingNode->next)
                {
                    if (checkingNode->type == XML_ELEMENT_NODE && (!type || typeUtf8 == checkingNode->name))
                    {
                        childNode = Node(checkingNode);
                        break;
                    }
                }
            }

            return childNode;
        }

        Node Node::createChildElement(LPCWSTR type, LPCWSTR format, ...)
        {
            Node newChildNode(nullptr);
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

                xmlNodePtr childNode = xmlNewChild(node, nullptr, BAD_CAST LPCSTR(CW2A(type, CP_UTF8)), BAD_CAST LPCSTR(CW2A(content, CP_UTF8)));
                if (childNode != nullptr)
                {
                    xmlAddChild(node, childNode);
                    newChildNode = Node(childNode);
                }
            }

            return newChildNode;
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

            HRESULT resultValue = E_FAIL;
            document = xmlNewDoc(BAD_CAST "1.0");
            if (document != nullptr)
            {
                xmlNodePtr rootNode = xmlNewNode(nullptr, BAD_CAST LPCSTR(CW2A(rootType, CP_UTF8)));
                if (rootNode != nullptr)
                {
                    xmlDocSetRootElement(document, rootNode);
                    resultValue = S_OK;
                }
            }

            return resultValue;
        }

        HRESULT Document::load(LPCWSTR fileName)
        {
            if (document != nullptr)
            {
                xmlFreeDoc(document);
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

        HRESULT Document::save(LPCWSTR fileName)
        {
            HRESULT resultValue = E_FAIL;
            if (document != nullptr)
            {
                CStringW expandedFileName(Gek::FileSystem::expandPath(fileName));
                xmlSaveFormatFileEnc(CW2A(expandedFileName, CP_UTF8), document, "UTF-8", 1);
            }

            return resultValue;
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
