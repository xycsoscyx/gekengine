/// @file
/// @author Todd Zupan <toddzupan@gmail.com>
/// @version $Revision: 895bd642687b9b4b70b544b22192a2aa11a6e721 $
/// @section LICENSE
/// https://en.wikipedia.org/wiki/MIT_License
/// @section DESCRIPTION
/// Last Changed: $Date:   Thu Oct 13 20:39:05 2016 +0000 $
#pragma once

#include "GEK/Utility/Context.hpp"
#include <unordered_map>
#include <assert.h>

#define GEK_CONTEXT_USER(CLASS, ...) struct CLASS : public ContextRegistration<CLASS, __VA_ARGS__>

#define GEK_REGISTER_CONTEXT_USER(CLASS)                                                                                                    \
ContextUserPtr CLASS##CreateInstance(Context *context, void *typelessArguments, std::vector<std::type_index> &argumentTypes)                \
{                                                                                                                                           \
    return CLASS::createObject(context, typelessArguments, argumentTypes);                                                                  \
}

#define GEK_DECLARE_CONTEXT_USER(CLASS)                                                                                                     \
extern ContextUserPtr CLASS##CreateInstance(Context *, void *, std::vector<std::type_index> &);

#define GEK_CONTEXT_BEGIN(SOURCENAME)                                                                                                       \
extern "C" __declspec(dllexport) void initializePlugin(                                                                                     \
    std::function<void(std::string const &, std::function<ContextUserPtr(Context *, void *, std::vector<std::type_index> &)>)> addClass,    \
    std::function<void(std::string const &, std::string const &)> addType)                                                                  \
{                                                                                                                                           \
    std::string lastClassName;

#define GEK_CONTEXT_ADD_CLASS(CLASSNAME, CLASS)                                                                                             \
    addClass(#CLASSNAME, CLASS##CreateInstance);                                                                                            \
    lastClassName = #CLASSNAME;

#define GEK_CONTEXT_ADD_TYPE(TYPEID)                                                                                                        \
    addType(#TYPEID, lastClassName);

#define GEK_CONTEXT_END()                                                                                                                   \
}

namespace Gek
{
    using InitializePlugin = void(*)(std::function<void(std::string const & className,
        std::function<ContextUserPtr(Context *, void *, std::vector<std::type_index> &)>)> addClass, 
        std::function<void(std::string const &, std::string const &)> addType);

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
        Context *context = nullptr;

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
            return std::make_unique<TYPE>(context, arguments...);
        }

        template<std::size_t... SIZE>
        static ContextUserPtr createFromPackedArguments(Context *context, const std::tuple<PARAMETERS...>& packedArguments, std::index_sequence<SIZE...> sequence, std::vector<std::type_index> &argumentTypes)
        {
            if (argumentTypes.size() != sequence.size())
            {
                std::cerr << "Invalid number of arguments passed to constructor, received " << sequence.size() << ", expected " << argumentTypes.size() << std::endl;
                return nullptr;
            }

            std::vector<std::type_index> expectedTypes = { typeid(PARAMETERS)... };
            if (expectedTypes != argumentTypes)
            {
                std::cerr << "Parameter types passed to creator don't match constructor types" << std::endl;
                return nullptr;
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
