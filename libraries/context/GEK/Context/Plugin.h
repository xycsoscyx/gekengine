#pragma once

#include "GEK\Context\ContextUser.h"
#include <unordered_map>

namespace Gek
{
    template <typename TYPE, typename... ARGUMENTS>
    interface Plugin
        : public ContextUser
    {
    public:
        using ContextUser::ContextUser;

        static std::shared_ptr<ContextUser> createBase(Context *context, ARGUMENTS... arguments)
        {
            return std::dynamic_pointer_cast<Plugin>(std::make_shared<TYPE>(context, arguments...));
        }

        template<std::size_t... Size>
        static std::shared_ptr<ContextUser> createUnpack(Context *context, const std::tuple<ARGUMENTS...>& tuple, std::index_sequence<Size...>)
        {
            return createBase(context, std::get<Size>(tuple)...);
        }

        static std::shared_ptr<ContextUser> createPacked(Context *context, void *arguments)
        {
            std::tuple<ARGUMENTS...> *tuple = (std::tuple<ARGUMENTS...> *)arguments;
            return createUnpack(context, *tuple, std::index_sequence_for<ARGUMENTS...>());
        }

        static std::shared_ptr<ContextUser> create(Context *context, void *parameters)
        {
            return createPacked(context, parameters);
        }
    };
/*
    interface Implementation
        : public Plugin<Implementation, int>
    {
    public:
        Implementation(Context *context, int A)
            : Plugin(context)
        {
        }
    };
*/
}; // namespace Gek

#define GEK_REGISTER_PLUGIN(CLASS)                                                                                      \
std::shared_ptr<ContextUser> CLASS##CreateInstance(Context *context, void *parameters)                                  \
{                                                                                                                       \
    return CLASS::create(context, parameters);                                                                          \
}

#define GEK_DECLARE_PLUGIN(CLASS)                                                                                       \
extern std::shared_ptr<ContextUser> CLASS##CreateInstance(Context *context, void *parameters);                          \

#define GEK_PLUGIN_BEGIN(SOURCENAME)                                                                                    \
extern "C" __declspec(dllexport) void initializePlugin(                                                                 \
    std::function<void(const wchar_t *, std::function<std::shared_ptr<ContextUser>(Context *, void *)>)> addClass,      \
    std::function<void(const wchar_t *, const wchar_t *)> addType)                                                      \
{                                                                                                                       \
    std::wstring lastClassName;

#define GEK_PLUGIN_CLASS(CLASSNAME, CLASS)                                                                              \
    addClass(CLASSNAME, CLASS##CreateInstance);                                                                         \
    lastClassName = CLASSNAME;

#define GEK_PLUGIN_TYPE(TYPEID)                                                                                         \
    addType(TYPEID, lastClassName);
    
#define GEK_PLUGIN_END()                                                                                                \
}
