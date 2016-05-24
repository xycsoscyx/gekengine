#pragma once

#include "GEK\Utility\Trace.h"
#include "GEK\Utility\String.h"
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

        static XmlNode create(const wchar_t *type);

        operator bool() const;

        wstring getType(void) const;

        wstring getText(void) const;
        void setText(const wchar_t *formatting, ...);

        bool hasAttribute(const wchar_t *name) const;
        wstring getAttribute(const wchar_t *name) const;
        void setAttribute(const wchar_t *name, const wchar_t *formatting, ...);
        void listAttributes(std::function<void(const wchar_t *, const wchar_t *)> onAttribute) const;

        bool hasChildElement(const wchar_t *type = nullptr) const;
        XmlNode firstChildElement(const wchar_t *type = nullptr) const;
        XmlNode createChildElement(const wchar_t *type, const wchar_t *formatting = nullptr, ...);

        bool hasSiblingElement(const wchar_t *type = nullptr) const;
        XmlNode nextSiblingElement(const wchar_t *type = nullptr) const;
    };

    class XmlDocument
    {
    private:
        void *document;

    private:
        XmlDocument(void *document);

    public:
        virtual ~XmlDocument(void);

        static XmlDocument create(const wchar_t *rootType);
        static XmlDocument load(const wchar_t *fileName, bool validateDTD = false);

        void save(const wchar_t *fileName);

        XmlNode getRoot(void) const;
    };
}; // namespace Gek
