/// @file
/// @author Todd Zupan <toddzupan@gmail.com>
/// @version $Revision: 1d50799f92d32e762c5d618bca16d2fe7fbeb872 $
/// @section LICENSE
/// https://en.wikipedia.org/wiki/MIT_License
/// @section DESCRIPTION
/// Last Changed: $Date:   Fri Oct 21 04:24:02 2016 +0000 $
#pragma once

#include "GEK/Math/Matrix4x4.hpp"
#include "GEK/Utility/Context.hpp"
#include "GEK/Utility/JSON.hpp"
#include "GEK/GUI/Utilities.hpp"
#include <typeindex>

#pragma warning(disable:4503)

#define GEK_COMPONENT(TYPE)         struct TYPE : public Plugin::Component::Data

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

            virtual wchar_t const * const getName(void) const = 0;
            virtual std::type_index getIdentifier(void) const = 0;

            virtual std::unique_ptr<Data> create(void) = 0;
            virtual void save(Data const * const data, JSON::Object &componentData) const = 0;
            virtual void load(Data * const data, const JSON::Object &componentData) = 0;
        };
    }; // namespace Plugin

    namespace Edit
    {
        GEK_INTERFACE(Component)
            : public Plugin::Component
        {
            virtual ~Component(void) = default;
            
            virtual void show(ImGuiContext * const guiContext, Plugin::Entity * const entity, Plugin::Component::Data *data) = 0;
            virtual bool edit(ImGuiContext * const guiContext, Math::Float4x4 const &viewMatrix, Math::Float4x4 const &projectionMatrix, Plugin::Entity * const entity, Plugin::Component::Data *data) = 0;
        };
    }; // namespace Edit
}; // namespace Gek
