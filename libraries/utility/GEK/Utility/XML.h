#pragma once

#include "GEK\Utility\Trace.h"
#include <atlbase.h>
#include <atlstr.h>
#include <functional>

namespace Gek
{
    namespace Xml
    {
        GEK_BASE_EXCEPTION();

        static void initialize(void);
        static void shutdown(void);
    }; // namespace Xml

    class XmlNode
    {
    private:
        void *node;

    private:
        friend class XmlDocument;
        XmlNode(void);
        XmlNode(void *node);

    public:
        virtual ~XmlNode(void);

        static XmlNode create(LPCWSTR type);

        operator bool() const;

        CStringW getType(void) const;

        CStringW getText(void) const;
        void setText(LPCWSTR format, ...);

        bool hasAttribute(LPCWSTR name) const;
        CStringW getAttribute(LPCWSTR name) const;
        void setAttribute(LPCWSTR name, LPCWSTR format, ...);
        void listAttributes(std::function<void(LPCWSTR, LPCWSTR)> onAttribute) const;

        bool hasChildElement(LPCWSTR type = nullptr) const;
        XmlNode firstChildElement(LPCWSTR type = nullptr) const;
        XmlNode createChildElement(LPCWSTR type, LPCWSTR format = nullptr, ...);

        bool hasSiblingElement(LPCWSTR type = nullptr) const;
        XmlNode nextSiblingElement(LPCWSTR type = nullptr) const;
    };

    class XmlDocument
    {
    private:
        void *document;

    private:
        XmlDocument(void *document);

    public:
        virtual ~XmlDocument(void);

        static XmlDocument create(LPCWSTR rootType);
        static XmlDocument load(LPCWSTR fileName, bool validateDTD = false);

        void save(LPCWSTR fileName);

        XmlNode getRoot(void) const;
    };
}; // namespace Gek
