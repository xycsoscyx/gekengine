#pragma once

#include "GEK\Engine\PopulationInterface.h"
#include "GEK\Engine\ComponentInterface.h"

namespace Gek
{
    namespace Engine
    {
        template <typename TYPE, typename CONVERTER>
        void setParameter(const std::unordered_map<CStringW, CStringW> &list, LPCWSTR name, TYPE &value, CONVERTER convert)
        {
            auto iterator = list.find(name);
            if (iterator != list.end())
            {
                value = convert((*iterator).second);
            }
        }

        template <class DATA>
        class BaseComponent : public Component::Interface
        {
        private:
            UINT32 emptyIndex;
            std::unordered_map<Population::Entity, UINT32> entityIndexList;
            std::vector<DATA> dataList;

        public:
            BaseComponent(void)
                : emptyIndex(0)
            {
            }

            virtual ~BaseComponent(void)
            {
            }

            // Component::Interface
            STDMETHODIMP_(void) addComponent(const Population::Entity &entity)
            {
                if (emptyIndex < dataList.size())
                {
                    entityIndexList[entity] = emptyIndex;
                    dataList[emptyIndex] = DATA();
                    emptyIndex++;
                }
                else
                {
                    entityIndexList[entity] = emptyIndex;
                    dataList.push_back(DATA());
                    emptyIndex = dataList.size();
                }
            }

            STDMETHODIMP_(void) removeComponent(const Population::Entity &entity)
            {
                if (entityIndexList.size() == 1)
                {
                    entityIndexList.clear();
                    emptyIndex = 0;
                }
                else
                {
                    auto destroyIterator = entityIndexList.find(entity);
                    if (destroyIterator != entityIndexList.end())
                    {
                        emptyIndex--;
                        auto moveIterator = std::find_if(entityIndexList.begin(), entityIndexList.end(), [&](std::pair<const Population::Entity, UINT32> &entityIndex) -> bool
                        {
                            return (entityIndex.second == emptyIndex);
                        });

                        if (moveIterator != entityIndexList.end())
                        {
                            dataList[(*destroyIterator).second] = std::move(dataList.back());
                            entityIndexList[(*moveIterator).first] = (*destroyIterator).second;
                        }

                        entityIndexList.erase(destroyIterator);
                    }
                }
            }

            STDMETHODIMP_(bool) hasComponent(const Population::Entity &entity) const
            {
                return (entityIndexList.count(entity) > 0);
            }

            STDMETHODIMP_(LPVOID) getComponent(const Population::Entity &entity)
            {
                auto indexIterator = entityIndexList.find(entity);
                if (indexIterator != entityIndexList.end())
                {
                    return LPVOID(&dataList[(*indexIterator).second]);
                }

                return nullptr;
            }

            STDMETHODIMP_(void) clear(void)
            {
                emptyIndex = 0;
                entityIndexList.clear();
                dataList.clear();
            }

            STDMETHODIMP_(void) getIntersectingSet(std::set<Population::Entity> &entityList)
            {
                if (entityList.empty())
                {
                    for (auto &entityIndex : entityIndexList)
                    {
                        entityList.insert(entityIndex.first);
                    }
                }
                else
                {
                    std::set<Population::Entity> intersectingList;
                    for (auto &entityIndex : entityIndexList)
                    {
                        if (entityList.count(entityIndex.first) > 0)
                        {
                            intersectingList.insert(entityIndex.first);
                        }
                    }

                    entityList = std::move(intersectingList);
                }
            }

            STDMETHODIMP getData(const Population::Entity &entity, std::unordered_map<CStringW, CStringW> &componentParameterList)
            {
                HRESULT resultValue = E_FAIL;
                auto indexIterator = entityIndexList.find(entity);
                if (indexIterator != entityIndexList.end())
                {
                    const DATA &data = dataList[(*indexIterator).second];
                    resultValue = data.getData(componentParameterList);
                }

                return resultValue;
            }

            STDMETHODIMP setData(const Population::Entity &entity, const std::unordered_map<CStringW, CStringW> &componentParameterList)
            {
                HRESULT resultValue = E_FAIL;
                auto indexIterator = entityIndexList.find(entity);
                if (indexIterator != entityIndexList.end())
                {
                    DATA &data = dataList[(*indexIterator).second];
                    resultValue = data.setData(componentParameterList);
                }

                return resultValue;
            }
        };
    }; // namespace Engine
}; // namespace Gek