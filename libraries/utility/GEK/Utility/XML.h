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

        static XmlNodePtr create(const wstring &type);

        virtual bool isValid(void) const = 0;

        virtual wstring getType(void) const = 0;

        virtual wstring getText(void) const = 0;
        virtual void setText(const wstring &text) = 0;

        virtual bool hasAttribute(const wstring &name) const = 0;
        virtual wstring getAttribute(const wstring &name, const wstring &defaultValue = wstring()) const = 0;
        virtual void setAttribute(const wstring &name, const wstring &value) = 0;
        virtual void listAttributes(std::function<void(const wstring &, const wstring &)> onAttribute) const = 0;

        virtual bool hasChildElement(const wstring &type = nullptr) const = 0;
        virtual XmlNodePtr firstChildElement(const wstring &type = nullptr, bool create = false) = 0;
        virtual XmlNodePtr createChildElement(const wstring &type, const wstring &content = wstring()) = 0;

        virtual bool hasSiblingElement(const wstring &type = nullptr) const = 0;
        virtual XmlNodePtr nextSiblingElement(const wstring &type = nullptr) const = 0;
    };

    struct XmlDocument;
    typedef std::shared_ptr<XmlDocument> XmlDocumentPtr;
    struct XmlDocument
    {
        virtual ~XmlDocument(void) = default;

        static XmlDocumentPtr create(const wstring &type);
        static XmlDocumentPtr load(const wstring &fileName, bool validateDTD = false);

        virtual void save(const wstring &fileName) = 0;;

        virtual XmlNodePtr getRoot(const wstring &type) const = 0;
    };
}; // namespace Gek
