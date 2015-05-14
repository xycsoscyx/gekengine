#pragma once

#include <windows.h>
#include <atlbase.h>
#include <atlstr.h>
#include <functional>
#include <libxml/tree.h>

namespace Gek
{
    namespace Xml
    {
        class Node
        {
        private:
            xmlNode *node;

        public:
            Node(xmlNode *node);

            operator bool() const
            {
                return (node ? true : false);
            }

            HRESULT create(LPCWSTR type);
            void setType(LPCWSTR type);
            CStringW getType(void) const;

            CStringW getText(void) const;
            void setText(LPCWSTR format, ...);

            bool hasAttribute(LPCWSTR name) const;
            CStringW getAttribute(LPCWSTR name) const;
            void setAttribute(LPCWSTR name, LPCWSTR format, ...);
            void listAttributes(std::function<void(LPCWSTR, LPCWSTR)> onAttribute) const;

            bool hasChildElement(LPCWSTR type = nullptr) const;
            Node firstChildElement(LPCWSTR type = nullptr) const;
            Node createChildElement(LPCWSTR type, LPCWSTR format = nullptr, ...);

            bool hasSiblingElement(LPCWSTR type = nullptr) const;
            Node nextSiblingElement(LPCWSTR type = nullptr) const;
        };

        class Document
        {
        private:
            xmlDoc *document;

        public:
            Document(void);
            ~Document(void);

            HRESULT create(LPCWSTR rootType);
            HRESULT load(LPCWSTR basePath);
            HRESULT save(LPCWSTR basePath);

            Node getRoot(void) const;
        };
    }; // namespace Xml
}; // namespace Gek
