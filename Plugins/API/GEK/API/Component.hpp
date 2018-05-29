/// @file
/// @author Todd Zupan <toddzupan@gmail.com>
/// @version $Revision: 1d50799f92d32e762c5d618bca16d2fe7fbeb872 $
/// @section LICENSE
/// https://en.wikipedia.org/wiki/MIT_License
/// @section DESCRIPTION
/// Last Changed: $Date:   Fri Oct 21 04:24:02 2016 +0000 $
#pragma once

#include "GEK/Utility/Context.hpp"
#include "GEK/Utility/JSON.hpp"

#pragma warning(disable:4503)

#define GEK_COMPONENT(TYPE)         struct TYPE : public Plugin::Component::Data

#define GEK_COMPONENT_DATA(TYPE) \
static std::string_view GetFullName(void) \
{ \
	return std::string_view(typeid(TYPE).name()); \
} \
\
static std::string_view GetName(void) \
{ \
	return GetFullName().substr(GetFullName().rfind(":") + 1); \
} \
\
static Hash GetIdentifier(void) \
{ \
	return typeid(TYPE).hash_code(); \
}

namespace Gek
{
    namespace Plugin
    {
        GEK_PREDECLARE(Entity);

        GEK_INTERFACE(Component)
        {
            struct Data
            {
                virtual ~Data(void) = default;
            };

            virtual ~Component(void) = default;

			virtual std::string_view getName(void) const = 0;
            virtual Hash getIdentifier(void) const = 0;

            virtual std::unique_ptr<Data> create(void) = 0;
            virtual void save(Data const * const data, JSON &exportData) const = 0;
            virtual void load(Data * const data, JSON const &exportData) = 0;
        };
    }; // namespace Plugin
}; // namespace Gek
