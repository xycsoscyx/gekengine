#pragma once

#include "GEK\Utility\Exceptions.h"
#include "GEK\Context\Context.h"
#include <unordered_map>

#define GEK_CONTEXT_USER(CLASS, ...) struct CLASS : public ContextRegistration<CLASS, __VA_ARGS__>

#define GEK_REGISTER_CONTEXT_USER(CLASS)                                                                                \
ContextUserPtr CLASS##CreateInstance(Context *context, void *parameters)                                                \
{                                                                                                                       \
    return CLASS::createObject(context, parameters);                                                                    \
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
    using InitializePlugin = void(*)(std::function<void(const wchar_t *, std::function<ContextUserPtr(Context *, void *)>)> addClass, std::function<void(const wchar_t *, const wchar_t *)> addType);

    GEK_PREDECLARE(Context);

    GEK_INTERFACE(ContextUser)
    {
        virtual ~ContextUser(void) = default;
    };

    template <typename TYPE, typename... PARAMETERS>
    struct ContextRegistration
        : public ContextUser
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
        static ContextUserPtr createBase(Context *context, PARAMETERS... arguments)
        {
            return std::make_shared<TYPE>(context, arguments...);
        }

        template<std::size_t... SIZE>
        static ContextUserPtr createFromTupleArguments(Context *context, const std::tuple<PARAMETERS...>& packedArguments, std::index_sequence<SIZE...>)
        {
            return createBase(context, std::get<SIZE>(packedArguments)...);
        }

        static ContextUserPtr createFromPackedArguments(Context *context, void *typelessArguments)
        {
            auto &packedArguments = *(std::tuple<PARAMETERS...> *)typelessArguments;
            return createFromTupleArguments(context, packedArguments, std::index_sequence_for<PARAMETERS...>());
        }

        static ContextUserPtr createObject(Context *context, void *typelessArguments)
        {
            return createFromPackedArguments(context, typelessArguments);
        }
    };
}; // namespace Gek
