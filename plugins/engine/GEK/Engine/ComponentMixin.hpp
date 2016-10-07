#pragma once

#include "GEK\Utility\Evaluator.hpp"
#include "GEK\Engine\Component.hpp"
#include <concurrent_unordered_map.h>
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

        template <class CLASS, typename... REQUIRED>
        class ProcessorMixin
        {
        private:
            struct Data : public CLASS::Data
            {
            };

        protected:
            using EntityDataMap = concurrency::concurrent_unordered_map<Plugin::Entity *, Data>;
            EntityDataMap entityDataMap;

        public:
            ProcessorMixin(void)
            {
            }

            virtual ~ProcessorMixin(void)
            {
            }

            // ProcessorMixin
            void clear(void)
            {
                entityDataMap.clear();
            }

            void addEntity(Plugin::Entity *entity, std::function<void(Data &data, REQUIRED&... components)> onAdded)
            {
                GEK_REQUIRE(entity);

                if (entity->hasComponents<REQUIRED...>())
                {
                    auto insertSearch = entityDataMap.insert(std::make_pair(entity, Data()));
                    if (insertSearch.second && onAdded)
                    {
                        onAdded(insertSearch.first->second, entity->getComponent<REQUIRED>()...);
                    }
                }
            }

            void removeEntity(Plugin::Entity *entity)
            {
                GEK_REQUIRE(entity);

                auto entitySearch = entityDataMap.find(entity);
                if (entitySearch != entityDataMap.end())
                {
                    entityDataMap.unsafe_erase(entitySearch);
                }
            }

            void list(std::function<void(Plugin::Entity *entity, Data &data, REQUIRED&... components)> onEntity)
            {
                GEK_REQUIRE(onEntity);

                concurrency::parallel_for_each(entityDataMap.begin(), entityDataMap.end(), [&](auto &entitySearch) -> void
                {
                    onEntity(entitySearch.first, entitySearch.second, entitySearch.first->getComponent<REQUIRED>()...);
                });
            }
        };
    }; // namespace Plugin
}; // namespace Gek