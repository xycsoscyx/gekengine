#pragma once

#include "GEK\Utility\Evaluator.hpp"
#include "GEK\Engine\Component.hpp"
#include <unordered_map>
#include <new>

namespace Gek
{
    namespace Components
    {
        template <typename TYPE>
        TYPE loadText(const Xml::Leaf &componentData, const TYPE &defaultValue)
        {
            if (componentData.text.empty())
            {
                return defaultValue;
            }
            else
            {
                return Evaluator::get<TYPE>(componentData.text);
            }
        }

        template <typename TYPE>
        TYPE loadAttribute(const Xml::Leaf &componentData, const wchar_t *name, const TYPE &defaultValue)
        {
            auto attributeSearch = componentData.attributes.find(name);
            if (attributeSearch == componentData.attributes.end())
            {
                return defaultValue;
            }
            else
            {
                return Evaluator::get<TYPE>(attributeSearch->second);
            }

        }
    }; // namespace Components

    namespace Plugin
    {
        GEK_PREDECLARE(Entity);

        template <class COMPONENT, class BASE = Plugin::Component>
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
                return typeid(COMPONENT);
            }

            std::unique_ptr<Plugin::Component::Data> create(void)
            {
                return std::make_unique<COMPONENT>();
            }

            void save(Plugin::Component::Data *component, Xml::Leaf &componentData) const
            {
                static_cast<COMPONENT *>(component)->save(componentData);
            }

            void load(Plugin::Component::Data *component, const Xml::Leaf &componentData)
            {
                static_cast<COMPONENT *>(component)->load(componentData);
            }
        };

        template <class DATA, typename... REQUIRED>
        class ProcessorHelper
        {
        protected:
            using EntityDataMap = std::unordered_map<Plugin::Entity *, DATA>;
            EntityDataMap entityDataMap;

        public:
            ProcessorHelper(void)
            {
            }

            virtual ~ProcessorHelper(void)
            {
            }

            // ProcessorHelper
            void clear(void)
            {
                entityDataMap.clear();
            }

            void addEntity(Plugin::Entity *entity, std::function<void(DATA &)> onAdded)
            {
                if (entity->hasComponents<REQUIRED...>())
                {
                    auto insertSearch = entityDataMap.insert(std::make_pair(entity, DATA()));
                    if (insertSearch.second)
                    {
                        onAdded(insertSearch.first->second);
                    }
                }
            }

            void removeEntity(Plugin::Entity *entity)
            {
                auto entitySearch = entityDataMap.find(entity);
                if (entitySearch != entityDataMap.end())
                {
                    entityDataMap.erase(entitySearch);
                }
            }

            void list(std::function<void(Plugin::Entity *entity, DATA &data)> onEntity)
            {
                for (auto &entitySearch : entityDataMap)
                {
                    onEntity(entitySearch.first, entitySearch.second);
                }
            }
        };
    }; // namespace Plugin
}; // namespace Gek