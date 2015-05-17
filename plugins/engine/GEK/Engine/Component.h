#pragma once

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
            std::unordered_map<Handle, UINT32> entityIndexList;
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
            STDMETHODIMP_(void) addComponent(Handle entityHandle)
            {
                if (emptyIndex < dataList.size())
                {
                    entityIndexList[entityHandle] = emptyIndex;
                    dataList[emptyIndex] = DATA();
                    emptyIndex++;
                }
                else
                {
                    entityIndexList[entityHandle] = emptyIndex;
                    dataList.push_back(DATA());
                    emptyIndex = dataList.size();
                }
            }

            STDMETHODIMP_(void) removeComponent(Handle entityHandle)
            {
                if (entityIndexList.size() == 1)
                {
                    entityIndexList.clear();
                    emptyIndex = 0;
                }
                else
                {
                    auto destroyIterator = entityIndexList.find(entityHandle);
                    if (destroyIterator != entityIndexList.end())
                    {
                        emptyIndex--;
                        auto moveIterator = std::find_if(entityIndexList.begin(), entityIndexList.end(), [&](std::pair<const Handle, UINT32> &entityIndex) -> bool
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

            STDMETHODIMP_(bool) hasComponent(Handle entityHandle) const
            {
                return (entityIndexList.count(entityHandle) > 0);
            }

            STDMETHODIMP_(LPVOID) getComponent(Handle entityHandle)
            {
                auto indexIterator = entityIndexList.find(entityHandle);
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

            STDMETHODIMP_(void) getIntersectingSet(std::set<Handle> &entityList)
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
                    std::set<Handle> intersectingList;
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

            STDMETHODIMP getData(Handle entityHandle, std::unordered_map<CStringW, CStringW> &componentParameterList)
            {
                HRESULT resultValue = E_FAIL;
                auto indexIterator = entityIndexList.find(entityHandle);
                if (indexIterator != entityIndexList.end())
                {
                    const DATA &data = dataList[(*indexIterator).second];
                    resultValue = data.getData(componentParameterList);
                }

                return resultValue;
            }

            STDMETHODIMP setData(Handle entityHandle, const std::unordered_map<CStringW, CStringW> &componentParameterList)
            {
                HRESULT resultValue = E_FAIL;
                auto indexIterator = entityIndexList.find(entityHandle);
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