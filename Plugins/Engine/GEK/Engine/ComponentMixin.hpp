/// @file
/// @author Todd Zupan <toddzupan@gmail.com>
/// @version $Revision: fc6dba5a2aba4d25dcd872ccbb719e3619e40901 $
/// @section LICENSE
/// https://en.wikipedia.org/wiki/MIT_License
/// @section DESCRIPTION
/// Last Changed: $Date:   Wed Oct 19 17:38:40 2016 +0000 $
#pragma once

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
            std::string name;

        protected:
            Population *population = nullptr;

        public:
            ComponentMixin(Population *population)
                : population(population)
            {
                name = typeid(COMPONENT).name();
                auto colonPosition = name.rfind(':');
                if (colonPosition != std::string::npos)
                {
                    name = name.substr(colonPosition + 1);
                }
            }

            virtual ~ComponentMixin(void) = default;

            // Plugin::Component
            std::string const &getName(void) const
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
            TYPE parse(JSON::Reference object, TYPE defaultValue)
            {
                return object.parse(population->getShuntingYard(), defaultValue);
            }

            virtual void save(COMPONENT const * const component, JSON::Object &componentData) const { };
            virtual void load(COMPONENT * const component, JSON::Reference componentData) { };

            void save(Plugin::Component::Data const * const component, JSON::Object &componentData) const
            {
                save(static_cast<COMPONENT const * const>(component), componentData);
            }

            void load(Plugin::Component::Data * const component, JSON::Reference componentData)
            {
                load(static_cast<COMPONENT * const>(component), componentData);
            }

            bool editorElement(char const *text, std::function<bool(void)> &&element)
            {
                ImGui::AlignFirstTextHeightToWidgets();
                ImGui::Text(text);
                ImGui::SameLine();
                ImGui::PushItemWidth(-1.0f);
                bool changed = element();
                ImGui::PopItemWidth();
                return changed;
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

            void addEntity(Plugin::Entity * const entity, std::function<void(bool isNewInsert, Data &data, REQUIRED&... components)> onAdded = nullptr)
            {
                assert(entity);

                if (entity->hasComponents<REQUIRED...>())
                {
                    auto insertSearch = entityDataMap.insert(std::make_pair(entity, Data()));
                    if (onAdded)
                    {
                        onAdded(insertSearch.second, insertSearch.first->second, entity->getComponent<REQUIRED>()...);
                    }
                }
            }

            void removeEntity(Plugin::Entity * const entity)
            {
                assert(entity);

                auto entitySearch = entityDataMap.find(entity);
                if (entitySearch != std::end(entityDataMap))
                {
                    entityDataMap.unsafe_erase(entitySearch);
                }
            }

            size_t getEntityCount(void)
            {
                return entityDataMap.size();
            }

            void listEntities(std::function<void(Plugin::Entity * const entity, Data &data, REQUIRED&... components)> onEntity)
            {
                assert(onEntity);

                std::for_each(std::begin(entityDataMap), std::end(entityDataMap), [&](auto &entitySearch) -> void
                {
                    onEntity(entitySearch.first, entitySearch.second, entitySearch.first->getComponent<REQUIRED>()...);
                });
            }

            void parallelListEntities(std::function<void(Plugin::Entity * const entity, Data &data, REQUIRED&... components)> onEntity)
            {
                assert(onEntity);

                concurrency::parallel_for_each(std::begin(entityDataMap), std::end(entityDataMap), [&](auto &entitySearch) -> void
                {
                    onEntity(entitySearch.first, entitySearch.second, entitySearch.first->getComponent<REQUIRED>()...);
                });
            }
        };
    }; // namespace Plugin
}; // namespace Gek