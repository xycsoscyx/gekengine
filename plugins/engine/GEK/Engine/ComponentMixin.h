#pragma once

#include "GEK\Utility\Evaluator.h"
#include "GEK\Engine\Component.h"
#include <new>

namespace Gek
{
    namespace Plugin
    {
        template <class DATA, class BASE = Plugin::Component>
        class ComponentMixin
            : public BASE
        {
        public:
            ComponentMixin(void)
            {
            }

            virtual ~ComponentMixin(void)
            {
            }

            // Plugin::Component
            std::type_index getIdentifier(void) const
            {
                return typeid(DATA);
            }

            std::unique_ptr<Plugin::Component::Data> create(void)
            {
                return std::make_unique<DATA>();
            }

            void save(Plugin::Component::Data *data, Xml::Leaf &componentData) const
            {
                static_cast<DATA *>(data)->save(componentData);
            }

            void load(Plugin::Component::Data *data, const Xml::Leaf &componentData)
            {
                static_cast<DATA *>(data)->load(componentData);
            }
        };
    }; // namespace Plugin
}; // namespace Gek