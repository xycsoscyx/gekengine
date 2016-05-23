#pragma once

#include <Windows.h>
#include <functional>
#include <string>
#include <memory>
#include <vector>

#define GEK_PREDECLARE(TYPE) class TYPE;
#define GEK_INTERFACE(TYPE) class TYPE; typedef std::shared_ptr<TYPE> TYPE##Ptr; class TYPE

namespace Gek
{
    GEK_PREDECLARE(ContextUser);

    GEK_INTERFACE(Context)
    {
    public:
        static ContextPtr create(const std::vector<std::wstring> &searchPathList);

        virtual std::function<std::shared_ptr<ContextUser>(Context *, void *)> getCreator(const wchar_t *name) = 0;

        template <typename TYPE, typename... ARGUMENTS>
        std::shared_ptr<TYPE> createClass(const wchar_t *name, ARGUMENTS... arguments)
        {
            std::tuple<ARGUMENTS...> tuple(arguments...);
            return std::dynamic_pointer_cast<TYPE>(getCreator(name)(this, reinterpret_cast<void *>(&tuple)));
        }

        virtual void listTypes(const wchar_t *typeName, std::function<void(const wchar_t *)> onType) = 0;
    };
}; // namespace Gek
