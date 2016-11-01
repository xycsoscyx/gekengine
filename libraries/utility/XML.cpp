#include "GEK\Utility\XML.hpp"
#include "GEK\Utility\FileSystem.hpp"
#include <TinyXML2.h>

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
            if (attributeSearch == std::end(attributes))
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
            auto childSearch = std::find_if(std::begin(children), std::end(children), [type](const Node &node) -> bool
            {
                return (node.type.compare(type) == 0);
            });

            if (childSearch == std::end(children))
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
            auto childSearch = std::find_if(std::begin(children), std::end(children), [type](const Node &node) -> bool
            {
                return (node.type.compare(type) == 0);
            });

            if (childSearch == std::end(children))
            {
                children.push_back(Node(type));
                return children.back();
            }
            else
            {
                return *childSearch;
            }
        }

        void getNodeData(tinyxml2::XMLElement *node, Node &nodeData)
        {
            for (auto attribute = node->FirstAttribute(); attribute; attribute = attribute->Next())
            {
                String name(attribute->Name());
                String value(attribute->Value());
                nodeData.attributes[name] = value;
            }

            for (auto child = node->FirstChildElement(); child; child = child->NextSiblingElement())
            {
                String childName(child->Name());
                nodeData.children.push_back(Node(childName));
                getNodeData(child, nodeData.children.back());
            }

            if (nodeData.children.empty())
            {
                nodeData.text = node->GetText();
            }
        }

        Node load(const wchar_t *fileName, const wchar_t *expectedRootType, bool validateDTD)
        {
            tinyxml2::XMLDocument document;
            auto result = document.LoadFile(StringUTF8(fileName));
            if (result != tinyxml2::XML_SUCCESS)
            {
                throw UnableToLoad();
            }

            auto root = document.RootElement();
            if (root == nullptr)
            {
                throw InvalidRootNode();
            }

            String rootType(root->Name());
            if (rootType.compare(expectedRootType) != 0)
            {
                throw InvalidRootNode();
            }

            Node rootData(rootType);
            getNodeData(root, rootData);
            return rootData;
        }

        void setNodeData(tinyxml2::XMLDocument &document, tinyxml2::XMLElement *node, Node &nodeData)
        {
            for (auto &attribute : nodeData.attributes)
            {
                node->SetAttribute(StringUTF8(attribute.first), StringUTF8(attribute.second).data());
            }

            for (auto &childNode : nodeData.children)
            {
                auto child = document.NewElement(StringUTF8(childNode.type));
                if (child == nullptr)
                {
                    throw UnableToSave();
                }

                node->InsertEndChild(child);
                setNodeData(document, child, childNode);
            }
        }

        void save(Node &rootData, const wchar_t *fileName)
        {
            tinyxml2::XMLDocument document;
            auto root = document.NewElement(StringUTF8(rootData.type));
            if (root == nullptr)
            {
                throw UnableToSave();
            }

            document.InsertFirstChild(root);
           
            setNodeData(document, root, rootData);

            auto result = document.SaveFile(StringUTF8(fileName));
            if (result != tinyxml2::XML_SUCCESS)
            {
                throw UnableToSave();
            }
        }
    };
}; // namespace Gek
