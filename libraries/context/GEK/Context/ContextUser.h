#pragma once

#include "GEK\Context\Context.h"
#include <unordered_map>

#define GEK_REGISTER_CONTEXT_USER(CLASS)                                                                                \
ContextUserPtr CLASS##CreateInstance(Context *context, void *parameters)                                                \
{                                                                                                                       \
    return CLASS::create(context, parameters);                                                                          \
}

#define GEK_DECLARE_CONTEXT_USER(CLASS)                                                                                 \
extern ContextUserPtr CLASS##CreateInstance(Context *context, void *parameters);                                        \

#define GEK_CONTEXT_BEGIN(SOURCENAME)                                                                                   \
extern "C" __declspec(dllexport) void initializePlugin(                                                                 \
    std::function<void(const wchar_t *, std::function<ContextUserPtr(Context *, void *)>)> addClass,                    \
    std::function<void(const wchar_t *, const wchar_t *)> addType)                                                      \
{                                                                                                                       \
    const wchar_t *lastClassName = nullptr;

#define GEK_CONTEXT_ADD_CLASS(CLASSNAME, CLASS)                                                                         \
    addClass(L#CLASSNAME, CLASS##CreateInstance);                                                                       \
    lastClassName = L#CLASSNAME;

#define GEK_CONTEXT_ADD_TYPE(TYPEID)                                                                                    \
    addType(L#TYPEID, lastClassName);

#define GEK_CONTEXT_END()                                                                                               \
}

namespace Gek
{
    GEK_PREDECLARE(Context);

    GEK_INTERFACE(ContextUser)
    {
        virtual ~ContextUser(void) = default;
    };

    template <typename TYPE, typename... ARGUMENTS>
    struct ContextRegistration
    {
    private:
        Context *context;

    public:
        ContextRegistration(Context *context)
            : context(context)
        {
        }

        virtual ~ContextRegistration(void) = default;

        Context * const getContext(void) const
        {
            return context;
        }

    public:
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
}; // namespace Gek
