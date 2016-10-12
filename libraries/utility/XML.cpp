#include "GEK\Utility\XML.hpp"
#include "GEK\Utility\FileSystem.hpp"
#include <libxml/tree.h>
#include <libxml/parser.h>

namespace Gek
{
    namespace Xml
    {
        static const Node nullNode;

        Leaf::Leaf(void)
            : valid(false)
        {
        }

        Leaf::Leaf(const wchar_t *type)
            : valid(true)
            , type(type)
        {
        }

        Leaf::Leaf(const Leaf &leaf)
            : valid(leaf.valid)
            , type(leaf.type)
            , text(leaf.text)
            , attributes(leaf.attributes)
        {
        }

        Leaf::Leaf(Leaf &&leaf)
            : valid(leaf.valid)
            , type(std::move(leaf.type))
            , text(std::move(leaf.text))
            , attributes(std::move(leaf.attributes))
        {
        }

        Leaf &Leaf::operator = (const Leaf &leaf)
        {
            *const_cast<bool *>(&valid) = leaf.valid;
            type = leaf.type;
            text = leaf.text;
            attributes = leaf.attributes;
            return (*this);
        }

        Leaf &Leaf::operator = (Leaf &&leaf)
        {
            *const_cast<bool *>(&valid) = leaf.valid;
            type = std::move(leaf.type);
            text = std::move(leaf.text);
            attributes = std::move(leaf.attributes);
            return (*this);
        }

        String Leaf::getAttribute(const wchar_t *name, const wchar_t *defaultValue) const
        {
            auto &attributeSearch = attributes.find(name);
            if (attributeSearch == attributes.end())
            {
                return defaultValue;
            }
            else
            {
                return attributeSearch->second;
            }
        }

        Node::Node(void)
        {
        }

        Node::Node(const wchar_t *type)
            : Leaf(type)
        {
        }

        Node::Node(const Node &node)
            : Leaf(node)
            , children(node.children)
        {
        }

        Node::Node(Node &&node)
            : Leaf(std::move(node))
            , children(std::move(node.children))
        {
        }

        Node &Node::operator = (const Node &node)
        {
            (Leaf &)(*this) = node;
            children = node.children;
            return (*this);
        }

        Node &Node::operator = (Node &&node)
        {
            (Leaf &)(*this) = std::move(node);
            children = std::move(node.children);
            return (*this);
        }

        const Node & Node::getChild(const wchar_t *type) const
        {
            auto childSearch = std::find_if(children.begin(), children.end(), [type](const Node &node) -> bool
            {
                return (node.type.compare(type) == 0);
            });

            if (childSearch == children.end())
            {
                return nullNode;
            }
            else
            {
                return *childSearch;
            }
        }

        Node & Node::getChild(const wchar_t *type)
        {
            auto childSearch = std::find_if(children.begin(), children.end(), [type](const Node &node) -> bool
            {
                return (node.type.compare(type) == 0);
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
                    nodeData.children.push_back(Node(childName));
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
            XmlDocument document(xmlReadFile(StringUTF8(fileName), nullptr, (validateDTD ? XML_PARSE_DTDATTR | XML_PARSE_DTDVALID : 0) | XML_PARSE_NOENT));
            if (document == nullptr)
            {
                throw UnableToLoad();
            }

            xmlNodePtr root = xmlDocGetRootElement(document);
            if (root == nullptr)
            {
                throw InvalidRootNode();
            }

            String rootType(reinterpret_cast<const char *>(root->name));
            if (rootType.compare(expectedRootType) != 0)
            {
                throw InvalidRootNode();
            }

            Node rootData(rootType);
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
                if (child == nullptr)
                {
                    throw UnableToSave();
                }

                setNodeData(child, childNode);
            }
        }

        void save(Node &rootData, const wchar_t *fileName)
        {
            XmlDocument document(xmlNewDoc(BAD_CAST "1.0"));
            if (document == nullptr)
            {
                throw UnableToSave();
            }

            xmlNodePtr rootNode = xmlNewNode(nullptr, BAD_CAST StringUTF8(rootData.type).c_str());
            if (rootNode == nullptr)
            {
                throw UnableToSave();
            }

            xmlDocSetRootElement(static_cast<xmlDocPtr>(document), rootNode);
            
            setNodeData(rootNode, rootData);

			xmlSaveFormatFileEnc(StringUTF8(fileName), document, "UTF-8", 2);
        }
    };
}; // namespace Gek
