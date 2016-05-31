#pragma once

#include "GEK\Utility\Trace.h"
#include "GEK\Utility\String.h"
#include <functional>
#include <memory>
#include <vector>

#define GEK_PREDECLARE(TYPE) struct TYPE; typedef std::shared_ptr<TYPE> TYPE##Ptr;
#define GEK_INTERFACE(TYPE) struct TYPE; typedef std::shared_ptr<TYPE> TYPE##Ptr; struct TYPE

namespace std
{
    using namespace Gek;

    template <typename RETURN, typename CREATE, typename... ARGUMENTS>
    std::shared_ptr<RETURN> remake_shared(ARGUMENTS... arguments)
    {
        std::shared_ptr<CREATE> baseObject;
        try
        {
            baseObject = std::make_shared<CREATE>(arguments...);
        }
        catch (std::bad_alloc badAllocation)
        {
            GEK_THROW_EXCEPTION(Gek::Exception, "Unable to allocate new object: %v", badAllocation.what());
        };

        std::shared_ptr<RETURN> remadeObject(std::dynamic_pointer_cast<RETURN>(baseObject));
        GEK_CHECK_CONDITION(!remadeObject, Gek::Exception, "Unable to cast base object to alternate type");

        return remadeObject;
    }
}; // namespace std

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
            return std::dynamic_pointer_cast<TYPE>(createBaseClass(name, static_cast<void *>(&tuple)));
        }

        virtual void listTypes(const wchar_t *typeName, std::function<void(const wchar_t *)> onType) const = 0;
    };
}; // namespace Gek
