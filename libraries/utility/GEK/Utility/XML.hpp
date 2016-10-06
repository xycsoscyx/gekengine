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
            enum class Source
            {
                File = 0,
                Code,
            };

            const Source source;

            String type;
            String text;
            std::unordered_map<String, String> attributes;

            Leaf(void);
            Leaf(const wchar_t *type, Source source = Source::Code);
            Leaf(const Leaf &node);
            Leaf(Leaf &&node);

            bool isFromFile(void);

            void operator = (const Leaf &node);
            void operator = (Leaf &&node);

            String getAttribute(const wchar_t *name, const wchar_t *defaultValue = String(L"")) const;

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
            Node(const wchar_t *type, Source source = Source::Code);
            Node(const Node &node);
            Node(Node &&node);

            void operator = (const Node &node);
            void operator = (Node &&node);

            bool findChild(const wchar_t *type, std::function<void(Node &)> onChildFound);
            Node & getChild(const wchar_t *type);
        };

        Node load(const wchar_t *fileName, const wchar_t *expectedRootType, bool validateDTD = false);
        void save(Node &rootData, const wchar_t *fileName);
    }; // namespace Xml
}; // namespace Gek
