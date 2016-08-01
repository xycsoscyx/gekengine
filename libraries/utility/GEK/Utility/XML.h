#pragma once

#include "GEK\Utility\Trace.h"
#include "GEK\Utility\String.h"
#include <functional>

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

            const Source source;

            String text;
            std::unordered_map<String, String> attributes;
            std::unordered_map<String, Node> children;

            Node(Source source = Source::Code);

            String attribute(const wchar_t *name, const wchar_t *value = nullptr) const;
            Node & child(const wchar_t *name);
        };

        struct Root : public Node
        {
            String type;

            Root(Source source = Source::Code);
        };

        Root load(const wchar_t *fileName, bool validateDTD = false);
        void save(Root &rootData, const wchar_t *fileName);
    }; // namespace Xml
}; // namespace Gek
