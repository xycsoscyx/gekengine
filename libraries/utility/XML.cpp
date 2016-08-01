#include "GEK\Utility\XML.h"
#include "GEK\Utility\FileSystem.h"
#include <libxml/tree.h>
#include <libxml/parser.h>

#pragma comment(lib, "libxml2.lib")

namespace Gek
{
    namespace Xml
    {
        Node::Node(Source source = Source::Code)
            : source(source)
        {
        }

        String Node::attribute(const wchar_t *name, const wchar_t *value = nullptr) const
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

        Node & Node::child(const wchar_t *name)
        {
            auto childSearch = children.find(name);
            GEK_CHECK_CONDITION(childSearch == children.end(), Exception, "Unable to find child node: %v", name);
            return childSearch->second;
        }

        Root::Root(Source source = Source::Code)
            : Node(source)
        {
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

            for (xmlNodePtr child = node->children; child; child = child->next)
            {
                if (child->type == XML_ELEMENT_NODE)
                {
                    auto childData = nodeData.children.insert(std::make_pair(reinterpret_cast<const char *>(child->name), Node(Node::Source::File)));
                    getNodeData(child, childData.first->second);
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

        Root load(const wchar_t *fileName, bool validateDTD)
        {
            StringUTF8 fileNameUTF8(FileSystem::expandPath(fileName));
            XmlDocument document(xmlReadFile(fileNameUTF8, nullptr, (validateDTD ? XML_PARSE_DTDATTR | XML_PARSE_DTDVALID : 0) | XML_PARSE_NOENT));
            GEK_CHECK_CONDITION(document == nullptr, Xml::Exception, "Unable to load document: %v", fileName);

            xmlNodePtr root = xmlDocGetRootElement(document);
            GEK_CHECK_CONDITION(root == nullptr, Xml::Exception, "Unable to get document root node");

            Root rootData(Node::Source::File);
            rootData.type = reinterpret_cast<const char *>(root->name);
            getNodeData(root, rootData);
            return rootData;
        }

        void setNodeData(xmlNodePtr node, Node &nodeData)
        {
            for (auto &attribute : nodeData.attributes)
            {
                xmlNewProp(node, BAD_CAST StringUTF8(attribute.first).c_str(), BAD_CAST StringUTF8(attribute.second).c_str());
            }

            for (auto &childData : nodeData.children)
            {
                xmlNodePtr child = xmlNewChild(node, nullptr, BAD_CAST StringUTF8(childData.first).c_str(), BAD_CAST StringUTF8(childData.second.text).c_str());
                GEK_CHECK_CONDITION(child == nullptr, Xml::Exception, "Unable to create new child node: %v (%v)", childData.first);
                setNodeData(child, childData.second);
            }
        }

        void save(Root &rootData, const wchar_t *fileName)
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
