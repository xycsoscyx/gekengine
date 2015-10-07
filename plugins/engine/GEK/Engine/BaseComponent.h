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

        template <class DATA, std::size_t ALIGNMENT>
        class AlignedAllocator
        {
        public:
            typedef DATA* pointer;
            typedef const DATA* const_pointer;
            typedef DATA& reference;
            typedef const DATA& const_reference;
            typedef DATA value_type;
            typedef std::size_t size_type;
            typedef ptrdiff_t difference_type;

        public:
            AlignedAllocator(void)
            {
            }

            AlignedAllocator(const AlignedAllocator&)
            {
            }

            template <typename NEWTYPE>
            AlignedAllocator(const AlignedAllocator<NEWTYPE, ALIGNMENT>&)
            {
            }

            ~AlignedAllocator()
            {
            }

            DATA* address(DATA& value) const
            {
                return &value;
            }

            const DATA* address(const DATA& value) const
            {
                return &value;
            }

            std::size_t max_size() const
            {
                return (static_cast<std::size_t>(0) - static_cast<std::size_t>(1)) / sizeof(DATA);
            }

            template <typename NEWTYPE>
            struct rebind
            {
                typedef AlignedAllocator<NEWTYPE, ALIGNMENT> other;
            };

            bool operator != (const AlignedAllocator& allocator) const
            {
                return !(*this == allocator);
            }

            void construct(DATA* const pointer, const DATA& value) const
            {
                void * const voidPointer = static_cast<void *>(pointer);

                new (voidPointer) DATA(value);
            }

            void destroy(DATA* const pointer) const
            {
                pointer->~DATA();
            }

            bool operator == (const AlignedAllocator& allocator) const
            {
                return true;
            }

            DATA* allocate(const std::size_t count) const
            {
                if (count == 0)
                {
                    return NULL;
                }

                if (count > max_size())
                {
                    throw std::length_error("AlignedAllocator<DATA>::allocate() - Integer overflow.");
                }

                void * const voidPointer = _mm_malloc(count * sizeof(DATA), ALIGNMENT);
                if (voidPointer == NULL)
                {
                    throw std::bad_alloc();
                }

                return static_cast<DATA *>(voidPointer);
            }

            void deallocate(DATA* const pointer, const std::size_t count) const
            {
                _mm_free(pointer);
            }

            template <typename NEWTYPE>
            DATA* allocate(const std::size_t count, const NEWTYPE *) const
            {
                return allocate(count);
            }
        };

        template <class DATA, class ALLOCATOR = std::allocator<DATA>>
        class BaseComponent : public Component::Interface
        {
        private:
            UINT32 emptyIndex;
            std::unordered_map<Population::Entity, UINT32> entityIndexList;
            std::vector<DATA, ALLOCATOR> dataList;

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