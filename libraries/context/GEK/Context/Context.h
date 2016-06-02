#pragma once

#include "GEK\Utility\Trace.h"
#include "GEK\Utility\String.h"
#include <functional>
#include <memory>
#include <vector>

#define GEK_PREDECLARE(TYPE) struct TYPE; typedef std::shared_ptr<TYPE> TYPE##Ptr;
#define GEK_INTERFACE(TYPE) struct TYPE; typedef std::shared_ptr<TYPE> TYPE##Ptr; struct TYPE

namespace Gek
{
    GEK_PREDECLARE(ContextUser);

    GEK_INTERFACE(Context)
    {
        static ContextPtr create(const std::vector<wstring> &searchPathList);

        virtual ContextUserPtr createBaseClass(const wchar_t *name, void *parameters) const = 0;

        template <typename TYPE, typename... ARGUMENTS>
        std::shared_ptr<TYPE> createClass(const wchar_t *name, ARGUMENTS... arguments) const
        {
            std::tuple<ARGUMENTS...> tuple(arguments...);
            ContextUserPtr baseClass = createBaseClass(name, static_cast<void *>(&tuple));

            std::shared_ptr<TYPE> castClass;
            try
            {
                castClass = std::dynamic_pointer_cast<TYPE>(baseClass);
            }
            catch (const std::bad_cast &badCast)
            {
                throw Gek::Exception(__FUNCTION__, __LINE__, string("Unable to cast to requested type: %v (%v)", typeid(TYPE).name(), badCast.what()));
            };

            return castClass;
        }

        virtual void listTypes(const wchar_t *typeName, std::function<void(const wchar_t *)> onType) const = 0;
    };
}; // namespace Gek
