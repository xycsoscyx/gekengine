#pragma once

#include "GEK\Engine\PopulationInterface.h"
#include <concurrent_unordered_map.h>
#include <concurrent_vector.h>
#include <concurrent_queue.h>
#include <unordered_map>
#include <typeindex>
#include <set>

#pragma warning(disable:4503)

namespace Gek
{
    namespace Engine
    {
        namespace Component
        {
            DECLARE_INTERFACE_IID(Type, "7B71BEF1-A2B2-4791-A664-2EA631E04392");

            DECLARE_INTERFACE_IID(Interface, "F1CA9EEC-0F09-45DA-BF24-0C70F5F96E3E") : virtual public IUnknown
            {
                STDMETHOD_(LPCWSTR, getName)                (THIS) const PURE;
                STDMETHOD_(std::type_index, getIdentifier)           (THIS) const PURE;

                STDMETHOD_(void, addComponent)              (THIS_ const Engine::Population::Entity &entity) PURE;
                STDMETHOD_(void, removeComponent)           (THIS_ const Engine::Population::Entity &entity) PURE;
                STDMETHOD_(bool, hasComponent)              (THIS_ const Engine::Population::Entity &entity) const PURE;
                STDMETHOD_(LPVOID, getComponent)            (THIS_ const Engine::Population::Entity &entity) PURE;
                STDMETHOD_(void, clear)                     (THIS) PURE;

                STDMETHOD_(void, getIntersectingSet)        (THIS_ std::set<Engine::Population::Entity> &entityList) PURE;

                STDMETHOD(getData)                          (THIS_ const Engine::Population::Entity &entity, std::unordered_map<CStringW, CStringW> &componentParameterList) PURE;
                STDMETHOD(setData)                          (THIS_ const Engine::Population::Entity &entity, const std::unordered_map<CStringW, CStringW> &componentParameterList) PURE;

                template <typename CLASS>
                CLASS &getComponent(const Engine::Population::Entity &entity)
                {
                    return *(CLASS *)getComponent(nEntityID);
                }
            };
        }; // namespace Component
    }; // namespace Engine
}; // namespace Gek
