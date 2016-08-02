#pragma once

#include "GEK\Utility\Exceptions.h"
#include "GEK\Utility\String.h"
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

        struct Node
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
            std::list<Node> children;

            Node(const wchar_t *type, Source source = Source::Code);
            Node(Node &&node);

            void operator = (Node &&node);

            bool isFromFile(void);
            String getAttribute(const wchar_t *name, const wchar_t *value = nullptr) const;
            bool findChild(const wchar_t *type, std::function<void(Node &)> onChildFound);
            Node & getChild(const wchar_t *type);

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

        Node load(const wchar_t *fileName, const wchar_t *expectedRootType, bool validateDTD = false);
        void save(Node &rootData, const wchar_t *fileName);
    }; // namespace Xml
}; // namespace Gek
