/// @file
/// @author Todd Zupan <toddzupan@gmail.com>
/// @version $Revision: 31a07b88fab4425367fa0aa67fe970fbff7dc9dc $
/// @section LICENSE
/// https://en.wikipedia.org/wiki/MIT_License
/// @section DESCRIPTION
/// Last Changed: $Date:   Thu Oct 27 08:51:53 2016 -0700 $
#pragma once

#include <any>
#include "GEK/Utility/ShuntingYard.hpp"
#include "GEK/Utility/JSON.hpp"

namespace Gek
{
    namespace Any
    {
        template <size_t, typename...>
        struct select_type
        {
        };

        template <size_t INDEX, typename VALUE, typename... PARAMETERS>
        struct select_type<INDEX, VALUE, PARAMETERS...> : public select_type<INDEX - 1, PARAMETERS...>
        {
        };

        template <typename VALUE, typename... PARAMETERS>
        struct select_type<0, VALUE, PARAMETERS...>
        {
            using type = VALUE;
        };

        template <typename TYPE>
        struct function_traits : public function_traits<decltype(&TYPE::operator())>
        {
        };

        template <typename RETURN, typename CLASS, typename... PARAMETERS>
        struct function_traits<RETURN(CLASS::*)(PARAMETERS...) const>
        {
            using result_type = RETURN;

            template <size_t COUNT>
            using argument_type = select_type<COUNT, PARAMETERS...>;
        };

        template <typename TYPE, typename... FUNCTORS>
        struct Converter
        {
            static TYPE Get(std::any &, TYPE const & defaultValue, FUNCTORS const & ...)
            {
                return defaultValue;
            }

            static TYPE Get(std::any const &, TYPE const & defaultValue, FUNCTORS const & ...)
            {
                return defaultValue;
            }
        };

        template <typename TYPE, typename VALUE, typename... FUNCTORS>
        struct Converter<TYPE, VALUE, FUNCTORS...>
        {
            static TYPE Get(std::any & value, TYPE const & defaultValue, VALUE const & functor, FUNCTORS const & ... remaining)
            {
                using arg = typename function_traits<VALUE>::template argument_type<0>::type;
                using arg_bare = typename std::remove_cv<typename std::remove_reference<arg>::type>::type;
                if (value.type() == typeid(arg_bare))
                {
                    return functor(*std::any_cast<arg_bare>(&value));
                }

                return Converter<TYPE, FUNCTORS...>::Get(value, defaultValue, remaining...);
            }

            static TYPE Get(std::any const & value, TYPE const & defaultValue, VALUE const & functor, FUNCTORS const & ... remaining)
            {
                using arg = typename function_traits<VALUE>::template argument_type<0>::type;
                using arg_bare = typename std::remove_cv<typename std::remove_reference<arg>::type>::type;
                if (value.type() == typeid(arg_bare))
                {
                    return functor(*std::any_cast<arg_bare>(&value));
                }

                return Converter<TYPE, FUNCTORS...>::Get(value, defaultValue, remaining...);
            }
        };

        template <typename TYPE, typename... FUNCTORS>
        TYPE Convert(std::any & value, TYPE const & defaultValue, FUNCTORS const & ... functors)
        {
            return Converter<TYPE, FUNCTORS...>::Get(value, defaultValue, functors...);
        }

        template <typename TYPE, typename... FUNCTORS>
        TYPE Convert(std::any const & value, TYPE const & defaultValue, FUNCTORS const & ... functors)
        {
            return Converter<TYPE, FUNCTORS...>::Get(value, defaultValue, functors...);
        }
    }; // namespace Any
}; // namespace Gek