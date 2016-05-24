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

        static ContextUserPtr createBase(Context *context, ARGUMENTS... arguments)
        {
            return std::remake_shared<ContextUser, TYPE>(context, arguments...);
        }

        template<std::size_t... Size>
        static ContextUserPtr createUnpack(Context *context, const std::tuple<ARGUMENTS...>& tuple, std::index_sequence<Size...>)
        {
            return createBase(context, std::get<Size>(tuple)...);
        }

        static ContextUserPtr createPacked(Context *context, void *arguments)
        {
            std::tuple<ARGUMENTS...> *tuple = (std::tuple<ARGUMENTS...> *)arguments;
            return createUnpack(context, *tuple, std::index_sequence_for<ARGUMENTS...>());
        }

        static ContextUserPtr create(Context *context, void *parameters)
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
ContextUserPtr CLASS##CreateInstance(Context *context, void *parameters)                                                \
{                                                                                                                       \
    return CLASS::create(context, parameters);                                                                          \
}

#define GEK_DECLARE_PLUGIN(CLASS)                                                                                       \
extern ContextUserPtr CLASS##CreateInstance(Context *context, void *parameters);                                        \

#define GEK_PLUGIN_BEGIN(SOURCENAME)                                                                                    \
extern "C" __declspec(dllexport) void initializePlugin(                                                                 \
    std::function<void(const wchar_t *, std::function<ContextUserPtr(Context *, void *)>)> addClass,                    \
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
