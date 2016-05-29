#pragma once

#include "GEK\Utility\Trace.h"
#include "GEK\Utility\String.h"
#include <functional>

namespace Gek
{
    namespace Xml
    {
        GEK_BASE_EXCEPTION();
    }; // namespace Xml

    struct XmlNode;
    typedef std::shared_ptr<XmlNode> XmlNodePtr;
    struct XmlNode
    {
        virtual ~XmlNode(void) = default;

        static XmlNodePtr create(const wchar_t *type);

        virtual bool isValid(void) const = 0;

        virtual wstring getType(void) const = 0;

        virtual wstring getText(void) const = 0;
        virtual void setText(const wchar_t *text) = 0;

        virtual bool hasAttribute(const wchar_t *name) const = 0;
        virtual wstring getAttribute(const wchar_t *name, const wstring &defaultValue = wstring()) const = 0;
        virtual void setAttribute(const wchar_t *name, const wchar_t *value) = 0;
        virtual void listAttributes(std::function<void(const wchar_t *, const wchar_t *)> onAttribute) const = 0;

        virtual bool hasChildElement(const wchar_t *type = nullptr) const = 0;
        virtual XmlNodePtr firstChildElement(const wchar_t *type = nullptr, bool create = false) = 0;
        virtual XmlNodePtr createChildElement(const wchar_t *type, const wchar_t *content = nullptr) = 0;

        virtual bool hasSiblingElement(const wchar_t *type = nullptr) const = 0;
        virtual XmlNodePtr nextSiblingElement(const wchar_t *type = nullptr) const = 0;
    };

    struct XmlDocument;
    typedef std::shared_ptr<XmlDocument> XmlDocumentPtr;
    struct XmlDocument
    {
        virtual ~XmlDocument(void) = default;

        static XmlDocumentPtr create(const wchar_t *type);
        static XmlDocumentPtr load(const wchar_t *fileName, bool validateDTD = false);

        virtual void save(const wchar_t *fileName) = 0;;

        virtual XmlNodePtr getRoot(const wchar_t *type) const = 0;
    };
}; // namespace Gek
