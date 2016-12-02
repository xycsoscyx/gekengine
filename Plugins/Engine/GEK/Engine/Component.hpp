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
        GEK_INTERFACE(Component)
        {
            struct Data
            {
                virtual ~Data(void) = default;
            };

            virtual ~Component(void) = default;

            virtual const wchar_t * const getName(void) const = 0;
            virtual std::type_index getIdentifier(void) const = 0;

            virtual std::unique_ptr<Data> create(void) = 0;
            virtual void save(const Data *data, JSON::Object &componentData) const = 0;
            virtual void load(Data *data, const JSON::Object &componentData) = 0;
        };
    }; // namespace Plugin

    namespace Edit
    {
        GEK_INTERFACE(Component)
            : public Plugin::Component
        {
            virtual ~Component(void) = default;
            
            virtual void show(ImGuiContext *guiContext, Plugin::Component::Data *data) = 0;
            virtual void edit(ImGuiContext *guiContext, const Math::Float4x4 &viewMatrix, const Math::Float4x4 &projectionMatrix, Plugin::Component::Data *data) = 0;
        };
    }; // namespace Edit
}; // namespace Gek
