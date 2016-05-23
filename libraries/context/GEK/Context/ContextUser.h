#pragma once

#include "GEK\Context\Context.h"
#include <unordered_map>

namespace Gek
{
    GEK_PREDECLARE(Context);

    GEK_INTERFACE(ContextUser)
    {
    private:
        Context *context;

    public:
        ContextUser(Context *context)
            : context(context)
        {
        }

        virtual ~ContextUser(void) = default;

        Context * const getContext(void) const
        {
            return context;
        }
    };

    template <typename TYPE, typename... ARGUMENTS>
    interface Plugin : public ContextUser
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
    interface Implementation : public Plugin<Implementation, int>
    {
    public:
        Implementation(Context *context, int A)
            : Plugin(context)
        {
        }
    };
*/
}; // namespace Gek
