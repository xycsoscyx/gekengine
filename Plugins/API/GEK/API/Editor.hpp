/// @file
/// @author Todd Zupan <toddzupan@gmail.com>
/// @version $Revision$
/// @section LICENSE
/// https://en.wikipedia.org/wiki/MIT_License
/// @section DESCRIPTION
/// Last Changed: $Date$
#pragma once

#include "GEK/Utility/Context.hpp"
#include "GEK/Utility/JSON.hpp"
#include "GEK/API/Component.hpp"
#include "GEK/API/Entity.hpp"
#include "GEK/API/Population.hpp"
#include "GEK/GUI/Utilities.hpp"
#include <wink/signal.hpp>

namespace Gek
{
    namespace Edit
    {
        GEK_INTERFACE(Entity)
            : public Plugin::Entity
        {
            virtual ~Entity(void) = default;

            using ComponentMap = std::unordered_map<Hash, std::unique_ptr<Plugin::Component::Data>>;
            virtual ComponentMap &getComponentMap(void) = 0;
        };

        GEK_INTERFACE(Component)
            : public Plugin::Component
        {
            virtual ~Component(void) = default;

            virtual bool onUserInterface(ImGuiContext * const guiContext, Plugin::Entity * const entity, Plugin::Component::Data *data) = 0;
        };

        GEK_INTERFACE(Population)
            : public Plugin::Population
        {
            virtual ~Population(void) = default;

            using ComponentMap = std::unordered_map<Hash, Plugin::ComponentPtr>;
            virtual ComponentMap &getComponentMap(void) = 0;

            using EntityList = std::list<Plugin::EntityPtr>;
            virtual EntityList &getEntityList(void) = 0;

            virtual Edit::Component *getComponent(Hash type) = 0;
        };

        GEK_INTERFACE(Events)
        {
            virtual ~Events(void) = default;

            wink::signal<wink::slot<void(Plugin::Entity *entity, Hash type)>> onModified;
        };
    }; // namespace Edit
}; // namespace Gek
