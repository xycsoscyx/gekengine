#pragma once

#include "GEK\Utility\Exceptions.h"
#include "GEK\Context\Context.h"
#include <unordered_map>

#define GEK_CONTEXT_USER(CLASS, ...) struct CLASS : public ContextRegistration<CLASS, __VA_ARGS__>

#define GEK_REGISTER_CONTEXT_USER(CLASS)                                                                                            \
ContextUserPtr CLASS##CreateInstance(Context *context, void *typelessArguments, std::vector<std::type_index> &argumentTypes)        \
{                                                                                                                                   \
    return CLASS::createObject(context, typelessArguments, argumentTypes);                                                          \
}

#define GEK_DECLARE_CONTEXT_USER(CLASS)                                                                                             \
extern ContextUserPtr CLASS##CreateInstance(Context *, void *, std::vector<std::type_index> &);

#define GEK_CONTEXT_BEGIN(SOURCENAME)                                                                                               \
extern "C" __declspec(dllexport) void initializePlugin(                                                                             \
    std::function<void(const wchar_t *, std::function<ContextUserPtr(Context *, void *, std::vector<std::type_index> &)>)> addClass,\
    std::function<void(const wchar_t *, const wchar_t *)> addType)                                                                  \
{                                                                                                                                   \
    const wchar_t *lastClassName = nullptr;

#define GEK_CONTEXT_ADD_CLASS(CLASSNAME, CLASS)                                                                                     \
    addClass(L#CLASSNAME, CLASS##CreateInstance);                                                                                   \
    lastClassName = L#CLASSNAME;

#define GEK_CONTEXT_ADD_TYPE(TYPEID)                                                                                                \
    addType(L#TYPEID, lastClassName);

#define GEK_CONTEXT_END()                                                                                                           \
}

namespace Gek
{
    using InitializePlugin = void(*)(std::function<void(const wchar_t * className, 
        std::function<ContextUserPtr(Context *, void *, std::vector<std::type_index> &)>)> addClass, 
        std::function<void(const wchar_t *, const wchar_t *)> addType);

    GEK_PREDECLARE(Context);

    GEK_INTERFACE(ContextUser)
    {
        GEK_START_EXCEPTIONS();
        GEK_ADD_EXCEPTION(InvalidParameterCount);
        GEK_ADD_EXCEPTION(InvalidParameterType);

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
        static ContextUserPtr createFromPackedArguments(Context *context, const std::tuple<PARAMETERS...>& packedArguments, std::index_sequence<SIZE...> sequence, std::vector<std::type_index> &argumentTypes)
        {
            auto argumentCount = sequence.size();
            if (argumentTypes.size() != argumentCount)
            {
                throw ContextUser::InvalidParameterCount();
            }

            std::vector<std::type_index> expectedTypes = { typeid(PARAMETERS)... };
            if (expectedTypes != argumentTypes)
            {
                throw ContextUser::InvalidParameterType();
            }

            return createBase(context, std::get<SIZE>(packedArguments)...);
        }

        static ContextUserPtr createFromTypelessArguments(Context *context, void *typelessArguments, std::vector<std::type_index> &argumentTypes)
        {
            auto &packedArguments = *(std::tuple<PARAMETERS...> *)typelessArguments;
            return createFromPackedArguments(context, packedArguments, std::index_sequence_for<PARAMETERS...>(), argumentTypes);
        }

        static ContextUserPtr createObject(Context *context, void *typelessArguments, std::vector<std::type_index> &argumentTypes)
        {
            return createFromTypelessArguments(context, typelessArguments, argumentTypes);
        }
    };
}; // namespace Gek
