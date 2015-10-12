#pragma once

#include "GEK\Context\ObserverInterface.h"
#include <atlbase.h>
#include <atlstr.h>
#include <functional>
#include <typeindex>
#include <unordered_map>
#include <vector>

namespace Gek
{
    namespace Engine
    {
        namespace Population
        {
            typedef UINT32 Entity;
            static const Entity InvalidEntity = -1;

            DECLARE_INTERFACE_IID(Class, "BD97404A-DE56-4DDC-BB34-3190FD51DEE5");

            DECLARE_INTERFACE_IID(Interface, "43DF2FD7-3BE2-4333-86ED-CB1221C6599B") : virtual public IUnknown
            {
                STDMETHOD(initialize)                       (THIS_ IUnknown *initializerContext) PURE;
                
                STDMETHOD_(void, update)                    (THIS_ float frameTime = 0.0f) PURE;

                STDMETHOD(load)                             (THIS_ LPCWSTR fileName) PURE;
                STDMETHOD(save)                             (THIS_ LPCWSTR fileName) PURE;
                STDMETHOD_(void, free)                      (THIS) PURE;

                STDMETHOD_(Entity, createEntity)            (THIS_ const std::unordered_map<CStringW, std::unordered_map<CStringW, CStringW>> &entityParameterList, LPCWSTR name = nullptr) PURE;
                STDMETHOD_(void, killEntity)                (THIS_ const Entity &entity) PURE;
                STDMETHOD_(Entity, getNamedEntity)          (THIS_ LPCWSTR name) PURE;

                STDMETHOD_(void, listEntities)              (THIS_ std::function<void(const Entity &)> onEntity, bool runInParallel = false) PURE;

                STDMETHOD_(bool, hasComponent)              (THIS_ const Entity &entity, std::type_index component) PURE;
                STDMETHOD_(LPVOID, getComponent)            (THIS_ const Entity &entity, std::type_index component) PURE;

                template<typename... ARGS>
                void listEntities(std::function<void(const Entity &, const ARGS&...)> onEntity)
                {
                }

                template <typename CLASS>
                bool hasComponent(const Entity &entity)
                {
                    return hasComponent(entity, typeid(CLASS));
                }

                template <typename CLASS>
                typename CLASS &getComponent(const Entity &entity)
                {
                    return *(CLASS *)getComponent(entity, typeid(CLASS));
                }
            };

            DECLARE_INTERFACE_IID(Observer, "51D6E5E6-2AD3-4D61-A704-8E6515F024F9") : virtual public Gek::Observer::Interface
            {
                STDMETHOD_(void, onLoadBegin)               (THIS) { };
                STDMETHOD_(void, onLoadEnd)                 (THIS_ HRESULT resultValue) { };
                STDMETHOD_(void, onFree)                    (THIS) { };

                STDMETHOD_(void, onEntityCreated)           (THIS_ const Entity &entity) { };
                STDMETHOD_(void, onEntityDestroyed)         (THIS_ const Entity &entity) { };

                STDMETHOD_(void, onUpdateBegin)             (THIS_ float frameTime) { };
                STDMETHOD_(void, onUpdate)                  (THIS_ float frameTime) { };
                STDMETHOD_(void, onUpdateEnd)               (THIS_ float frameTime) { };
            };
        }; // namespace Population
    }; // namespace Engine
}; // namespace Gek