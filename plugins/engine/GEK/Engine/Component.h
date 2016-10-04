#pragma once

#include "GEK\Context\Context.h"
#include "GEK\Engine\Population.h"
#include <typeindex>

#pragma warning(disable:4503)

#define GEK_COMPONENT(TYPE)         struct TYPE : public Plugin::Component::Data

struct ImGuiContext;

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

            virtual const wchar_t * const getName(void) const = 0;
            virtual std::type_index getIdentifier(void) const = 0;

            virtual std::unique_ptr<Data> create(const Plugin::Population::ComponentDefinition &componentData) = 0;
        };
    }; // namespace Plugin

    namespace Editor
    {
        GEK_INTERFACE(Component)
            : public Plugin::Component
        {
            virtual void showEditor(ImGuiContext *guiContext, const Math::Float4x4 &viewMatrix, const Math::Float4x4 &projectionMatrix, Plugin::Component::Data *data) = 0;
        };
    }; // namespace Editor
}; // namespace Gek
