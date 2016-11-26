/// @file
/// @author Todd Zupan <toddzupan@gmail.com>
/// @version $Revision: fc6dba5a2aba4d25dcd872ccbb719e3619e40901 $
/// @section LICENSE
/// https://en.wikipedia.org/wiki/MIT_License
/// @section DESCRIPTION
/// Last Changed: $Date:   Wed Oct 19 17:38:40 2016 +0000 $
#pragma once

#include "GEK/Utility/Evaluator.hpp"
#include "GEK/Engine/Component.hpp"
#include "GEK/Engine/Population.hpp"
#include <concurrent_unordered_map.h>
#include <new>

namespace Gek
{
    namespace Plugin
    {
        GEK_PREDECLARE(Entity);

        template <class COMPONENT, class BASE = Plugin::Component>
        class ComponentMixin
            : public BASE
        {
        private:
            String name;

        protected:
            Population *population = nullptr;

        public:
            ComponentMixin(Population *population)
                : population(population)
            {
                name = typeid(COMPONENT).name();
                auto colonPosition = name.rfind(':');
                if (colonPosition != StringUTF8::npos)
                {
                    name = name.subString(colonPosition + 1);
                }
            }

            virtual ~ComponentMixin(void) = default;

            // Plugin::Component
            const wchar_t * const getName(void) const
            {
                return name;
            }

            std::type_index getIdentifier(void) const
            {
                return typeid(COMPONENT);
            }

            std::unique_ptr<Plugin::Component::Data> create(void)
            {
                return std::make_unique<COMPONENT>();
            }

            template <typename TYPE>
            TYPE getValue(const JSON::Object &data, const TYPE &defaultValue)
            {
                if (data.is_string())
                {
                    return Evaluator::Get<TYPE>(population->getShuntingYard(), data.as_cstring());
                }
                else if (data.is<TYPE>())
                {
                    return data.as<TYPE>();
                }

                return defaultValue;
            }

            template <typename TYPE>
            TYPE getValue(const JSON::Object &componentData, const wchar_t *name, const TYPE &defaultValue)
            {
                if (componentData.is_object() && componentData.has_member(name))
                {
                    auto data = componentData.get(name);
                    return getValue(data, defaultValue);
                }

                return defaultValue;
            }

            virtual void save(const COMPONENT *component, JSON::Object &componentData) const { };
            virtual void load(COMPONENT *component, const JSON::Object &componentData) { };

            void save(const Plugin::Component::Data *component, JSON::Object &componentData) const
            {
                save(static_cast<const COMPONENT *>(component), componentData);
            }

            void load(Plugin::Component::Data *component, const JSON::Object &componentData)
            {
                load(static_cast<COMPONENT *>(component), componentData);
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
            virtual ~ProcessorMixin(void) = default;

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
                if (entitySearch != std::end(entityDataMap))
                {
                    entityDataMap.unsafe_erase(entitySearch);
                }
            }

            uint32_t getEntityCount(void)
            {
                return entityDataMap.size();
            }

            void list(std::function<void(Plugin::Entity *entity, Data &data, REQUIRED&... components)> onEntity)
            {
                GEK_REQUIRE(onEntity);

                concurrency::parallel_for_each(std::begin(entityDataMap), std::end(entityDataMap), [&](auto &entitySearch) -> void
                {
                    onEntity(entitySearch.first, entitySearch.second, entitySearch.first->getComponent<REQUIRED>()...);
                });
            }
        };
    }; // namespace Plugin
}; // namespace Gek