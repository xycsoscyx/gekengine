#pragma once

#include <atlbase.h>
#include <atlstr.h>
#include <functional>

namespace Gek
{
    class XmlNode
    {
    private:
        LPVOID node;

    public:
        XmlNode(LPVOID node);

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
        XmlNode firstChildElement(LPCWSTR type = nullptr) const;
        XmlNode createChildElement(LPCWSTR type, LPCWSTR format = nullptr, ...);

        bool hasSiblingElement(LPCWSTR type = nullptr) const;
        XmlNode nextSiblingElement(LPCWSTR type = nullptr) const;
    };

    class XmlDocument
    {
    private:
        LPVOID document;

    public:
        XmlDocument(void);
        ~XmlDocument(void);

        HRESULT create(LPCWSTR rootType);
        HRESULT load(LPCWSTR fileName, bool validateDTD = false);
        HRESULT save(LPCWSTR fileName);

        XmlNode getRoot(void) const;
    };
}; // namespace Gek
