#pragma once

#include "GEK\Utility\Exceptions.hpp"
#include "GEK\Utility\String.hpp"
#include <functional>
#include <unordered_map>
#include <list>

namespace Gek
{
    namespace Xml
    {
        GEK_START_EXCEPTIONS();
        GEK_ADD_EXCEPTION(UnableToLoad);
        GEK_ADD_EXCEPTION(UnableToSave);
        GEK_ADD_EXCEPTION(InvalidRootNode);

        struct Leaf
        {
            const bool valid;

            String type;
            String text;
            std::unordered_map<String, String> attributes;

            Leaf(void);
            Leaf(const wchar_t *type);
            Leaf(const Leaf &node);
            Leaf(Leaf &&node);

            Leaf &operator = (const Leaf &leaf);
            Leaf &operator = (Leaf &&leaf);

            String getAttribute(const wchar_t *name, const wchar_t *defaultValue = nullptr) const;

            template <typename TYPE>
            TYPE getValue(const wchar_t *name, const TYPE &defaultValue) const
            {
                auto attributeSearch = attributes.find(name);
                if (attributeSearch != attributes.end())
                {
                    return attributeSearch->second;
                }

                return defaultValue;
            }
        };

        struct Node
            : public Leaf
        {
            std::list<Node> children;

            Node(void);
            Node(const wchar_t *type);
            Node(const Node &node);
            Node(Node &&node);

            Node &operator = (const Node &node);
            Node &operator = (Node &&node);

            const Node & getChild(const wchar_t *type) const;
            Node & getChild(const wchar_t *type);
        };

        Node load(const wchar_t *fileName, const wchar_t *expectedRootType, bool validateDTD = false);
        void save(Node &rootData, const wchar_t *fileName);
    }; // namespace Xml
}; // namespace Gek
