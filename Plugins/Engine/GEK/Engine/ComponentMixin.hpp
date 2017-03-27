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
            wchar_t const * const getName(void) const
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

            bool getValue(JSON::Object const &data, bool defaultValue)
            {
                try
                {
                    return data.as_bool();
                }
                catch (...)
                {
                    return defaultValue;
                };
            }

            int32_t getValue(JSON::Object const &data, int32_t defaultValue)
            {
                try
                {
                    if (data.is_string())
                    {
                        return int32_t(population->getShuntingYard().evaluate(data.as_cstring()));
                    }
                    else
                    {
                        return data.as_integer();
                    }
                }
                catch (...)
                {
                    return defaultValue;
                };
            }

            uint32_t getValue(JSON::Object const &data, uint32_t defaultValue)
            {
                try
                {
                    if (data.is_string())
                    {
                        return uint32_t(population->getShuntingYard().evaluate(data.as_cstring()));
                    }
                    else
                    {
                        return data.as_uinteger();
                    }
                }
                catch (...)
                {
                    return defaultValue;
                };
            }

            float getValue(JSON::Object const &data, float defaultValue)
            {
                try
                {
                    if (data.is_string())
                    {
                        return population->getShuntingYard().evaluate(data.as_cstring());
                    }
                    else
                    {
                        return data.as<float>();
                    }
                }
                catch (...)
                {
                    return defaultValue;
                };
            }

            template <typename TYPE>
            Math::Vector2<TYPE> getValue(JSON::Object const &data, Math::Vector2<TYPE> const &defaultValue)
            {
                try
                {
                    if (data.is_array() && data.size() == 2)
                    {
                        return Math::Vector2<TYPE>(
                            getValue(data.at(0), defaultValue.x),
                            getValue(data.at(1), defaultValue.y));
                    }
                }
                catch (...)
                {
                };

                return defaultValue;
            }

            template <typename TYPE>
            Math::Vector3<TYPE> getValue(JSON::Object const &data, Math::Vector3<TYPE> const &defaultValue)
            {
                try
                {
                    if (data.is_array() && data.size() == 3)
                    {
                        return Math::Vector3<TYPE>(
                            getValue(data.at(0), defaultValue.x),
                            getValue(data.at(1), defaultValue.y),
                            getValue(data.at(2), defaultValue.z));
                    }
                }
                catch (...)
                {
                };

                return defaultValue;
            }

            template <typename TYPE>
            Math::Vector4<TYPE> getValue(JSON::Object const &data, Math::Vector4<TYPE> const &defaultValue)
            {
                try
                {
                    if (data.is_array())
                    {
                        if (data.size() == 3)
                        {
                            return Math::Vector4<TYPE>(
                                getValue(data.at(0), defaultValue.x),
                                getValue(data.at(1), defaultValue.y),
                                getValue(data.at(2), defaultValue.z), 1.0f);
                        }
                        else if (data.size() == 4)
                        {
                            return Math::Vector4<TYPE>(
                                getValue(data.at(0), defaultValue.x),
                                getValue(data.at(1), defaultValue.y),
                                getValue(data.at(2), defaultValue.z),
                                getValue(data.at(3), defaultValue.w));
                        }
                    }
                }
                catch (...)
                {
                };

                return defaultValue;
            }

            Math::Quaternion getValue(JSON::Object const &data, Math::Quaternion const &defaultValue)
            {
                try
                {
                    if (data.is_array())
                    {
                        if (data.size() == 3)
                        {
                            return Math::Quaternion::FromEuler(
                                getValue(data.at(0), defaultValue.x),
                                getValue(data.at(1), defaultValue.y),
                                getValue(data.at(2), defaultValue.z));
                        }
                        else if (data.size() == 4)
                        {
                            return Math::Quaternion(
                                getValue(data.at(0), defaultValue.x),
                                getValue(data.at(1), defaultValue.y),
                                getValue(data.at(2), defaultValue.z),
                                getValue(data.at(3), defaultValue.w));
                        }
                    }
                }
                catch (...)
                {
                };

                return defaultValue;
            }

            String getValue(JSON::Object const &data, String const &defaultValue)
            {
                try
                {
                    return data.as_string();
                }
                catch (...)
                {
                    return defaultValue;
                };
            }

            template <typename TYPE>
            TYPE getValue(JSON::Object const &componentData, wchar_t const * const name, TYPE const &defaultValue)
            {
                if (componentData.is_object() && componentData.has_member(name))
                {
                    auto &data = componentData.get(name);
                    return getValue(data, defaultValue);
                }

                return defaultValue;
            }

            virtual void save(COMPONENT const * const component, JSON::Object &componentData) const { };
            virtual void load(COMPONENT * const component, JSON::Object const &componentData) { };

            void save(Plugin::Component::Data const * const component, JSON::Object &componentData) const
            {
                save(static_cast<COMPONENT const * const>(component), componentData);
            }

            void load(Plugin::Component::Data * const component, JSON::Object const &componentData)
            {
                load(static_cast<COMPONENT * const>(component), componentData);
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

            void addEntity(Plugin::Entity * const entity, std::function<void(Data &data, REQUIRED&... components)> onAdded)
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

            void removeEntity(Plugin::Entity * const entity)
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

            void listEntities(std::function<void(Plugin::Entity * const entity, Data &data, REQUIRED&... components)> onEntity)
            {
                GEK_REQUIRE(onEntity);

                std::for_each(std::begin(entityDataMap), std::end(entityDataMap), [&](auto &entitySearch) -> void
                {
                    onEntity(entitySearch.first, entitySearch.second, entitySearch.first->getComponent<REQUIRED>()...);
                });
            }

            void parallelListEntities(std::function<void(Plugin::Entity * const entity, Data &data, REQUIRED&... components)> onEntity)
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