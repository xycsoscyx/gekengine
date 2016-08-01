#pragma once

#include "GEK\Utility\Trace.h"
#include "GEK\Utility\String.h"
#include <functional>
#include <map>

namespace Gek
{
    namespace Xml
    {
        GEK_START_EXCEPTIONS();

        struct Node
        {
            enum class Source
            {
                File = 0,
                Code,
            };

            struct Key : public String
            {
                int16_t index;

                inline Key(const String &name, int16_t index = -1)
                    : String(name)
                    , index(index)
                {
                }

                inline Key(const wchar_t *name, int16_t index = -1)
                    : String(name)
                    , index(index)
                {
                }

                inline operator String &(void)
                {
                    return *this;
                }

                inline operator String const &(void) const
                {
                    return *this;
                }

                inline bool operator < (const Key &key) const
                {
                    if (index < 0 || key.index < 0)
                    {
                        return ((const String &)*this < (const String &)key);
                    }
                    else
                    {
                        return (index < key.index);
                    }
                }
            };

            const Source source;

            String text;
            std::unordered_map<String, String> attributes;
            std::map<Key, Node> children;

            Node(Source source = Source::Code);
            Node(Node &&node);

            void operator = (Node &&node);

            bool isFromFile(void);
            String getAttribute(const wchar_t *name, const wchar_t *value = nullptr) const;
            Node & getChild(const wchar_t *name);

            template <typename TYPE>
            bool getValue(const wchar_t *name, TYPE &value) const
            {
                auto attributeSearch = attributes.find(name);
                if (attributeSearch != attributes.end())
                {
                    value = attributeSearch->second;
                    return true;
                }

                return false;
            }
        };

        struct Root : public Node
        {
            String type;

            Root(const wchar_t *type = nullptr, Source source = Source::Code);
            Root(Root &&root);

            void operator = (Root &&root);
        };

        Root load(const wchar_t *fileName, const wchar_t *expectedRootType, bool validateDTD = false);
        void save(Root &rootData, const wchar_t *fileName);
    }; // namespace Xml
}; // namespace Gek
