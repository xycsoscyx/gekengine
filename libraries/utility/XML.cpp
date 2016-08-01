#include "GEK\Utility\XML.h"
#include "GEK\Utility\FileSystem.h"
#include <libxml/tree.h>
#include <libxml/parser.h>

#pragma comment(lib, "libxml2.lib")

namespace Gek
{
    namespace Xml
    {
        Node::Node(const wchar_t *type, Source source)
            : source(source)
            , type(type)
        {
        }

        Node::Node(Node &&node)
            : source(node.source)
            , type(std::move(node.type))
            , text(std::move(node.text))
            , attributes(std::move(node.attributes))
            , children(std::move(node.children))
        {
        }

        void Node::operator = (Node &&node)
        {
            const_cast<Source>(source) = node.source;
            type = std::move(node.type);
            text = std::move(node.text);
            attributes = std::move(node.attributes);
            children = std::move(node.children);
        }

        bool Node::isFromFile(void)
        {
            return (source == Source::File);
        }

        String Node::getAttribute(const wchar_t *name, const wchar_t *value) const
        {
            auto &attributeSearch = attributes.find(name);
            if (attributeSearch == attributes.end())
            {
                return value;
            }
            else
            {
                return attributeSearch->second;
            }
        }

        Node & Node::findChild(const wchar_t *type)
        {
            auto childSearch = std::find_if(children.begin(), children.end(), [type](const Node &node) -> bool
            {
                return (node.type.compare(type));
            });

            GEK_THROW_EXCEPTION(Exception, "Unable to find child node: %v", type);
            return *childSearch;
        }

        Node & Node::getChild(const wchar_t *type)
        {
            auto childSearch = std::find_if(children.begin(), children.end(), [type](const Node &node) -> bool
            {
                return (node.type.compare(type));
            });

            if (childSearch == children.end())
            {
                children.push_back(Node(type));
                return children.back();
            }
            else
            {
                return *childSearch;
            }
        }

        void getNodeData(xmlNodePtr node, Node &nodeData)
        {
            for (xmlAttrPtr attribute = node->properties; attribute != nullptr; attribute = attribute->next)
            {
                String name(reinterpret_cast<const char *>(attribute->name));

                xmlChar *valueUTF8 = xmlGetProp(node, attribute->name);
                String value(reinterpret_cast<const char *>(valueUTF8));
                xmlFree(valueUTF8);

                nodeData.attributes[name] = value;
            }

            uint16_t index = 0;
            for (xmlNodePtr child = node->children; child; child = child->next)
            {
                if (child->type == XML_ELEMENT_NODE)
                {
                    String childName(reinterpret_cast<const char *>(child->name));
                    nodeData.children.push_back(Node(childName, Node::Source::File));
                    getNodeData(child, nodeData.children.back());
                }
            }

            if (nodeData.children.empty())
            {
                xmlChar *textUTF8 = xmlNodeGetContent(node);
                nodeData.text = reinterpret_cast<const char *>(textUTF8);
                xmlFree(textUTF8);
            }
        }

        struct XmlDocument
        {
            xmlDocPtr document;
            XmlDocument(xmlDocPtr document)
                : document(document)
            {
            }

            ~XmlDocument(void)
            {
                xmlFreeDoc(document);
            }

            operator xmlDocPtr ()
            {
                return document;
            }
        };

        Node load(const wchar_t *fileName, const wchar_t *expectedRootType, bool validateDTD)
        {
            StringUTF8 fileNameUTF8(FileSystem::expandPath(fileName));
            XmlDocument document(xmlReadFile(fileNameUTF8, nullptr, (validateDTD ? XML_PARSE_DTDATTR | XML_PARSE_DTDVALID : 0) | XML_PARSE_NOENT));
            GEK_CHECK_CONDITION(document == nullptr, Xml::Exception, "Unable to load document: %v", fileName);

            xmlNodePtr root = xmlDocGetRootElement(document);
            GEK_CHECK_CONDITION(root == nullptr, Xml::Exception, "Unable to get document root node");
            String rootType(reinterpret_cast<const char *>(root->name));
            if (expectedRootType)
            {
                GEK_CHECK_CONDITION(rootType.compare(expectedRootType) != 0, Exception, "XML root node not expected type: %v (%v)", rootType, expectedRootType);
            }

            Node rootData(rootType, Node::Source::File);
            getNodeData(root, rootData);
            return rootData;
        }

        void setNodeData(xmlNodePtr node, Node &nodeData)
        {
            for (auto &attribute : nodeData.attributes)
            {
                xmlNewProp(node, BAD_CAST StringUTF8(attribute.first).c_str(), BAD_CAST StringUTF8(attribute.second).c_str());
            }

            for (auto &childNode : nodeData.children)
            {
                xmlNodePtr child = xmlNewChild(node, nullptr, BAD_CAST StringUTF8(childNode.type).c_str(), BAD_CAST StringUTF8(childNode.text).c_str());
                GEK_CHECK_CONDITION(child == nullptr, Xml::Exception, "Unable to create new child node: %v)", childNode.type);
                setNodeData(child, childNode);
            }
        }

        void save(Node &rootData, const wchar_t *fileName)
        {
            StringUTF8 expandedFileName(FileSystem::expandPath(fileName));

            XmlDocument document(xmlNewDoc(BAD_CAST "1.0"));
            GEK_CHECK_CONDITION(document == nullptr, Xml::Exception, "Unable to create new document");

            xmlNodePtr rootNode = xmlNewNode(nullptr, BAD_CAST StringUTF8(rootData.type).c_str());
            GEK_CHECK_CONDITION(rootNode == nullptr, Xml::Exception, "Unable to create root node: %v", rootData.type);

            xmlDocSetRootElement(static_cast<xmlDocPtr>(document), rootNode);
            
            setNodeData(rootNode, rootData);

            xmlSaveFormatFileEnc(expandedFileName, document, "UTF-8", 1);
        }
    };
}; // namespace Gek
