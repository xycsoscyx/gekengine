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
            DECLARE_INTERFACE_IID(Entity, "F5F5D5D3-E409-437D-BF3B-801DFA230C6C") : virtual public IUnknown
            {
                STDMETHOD_(bool, hasComponent)              (THIS_ const std::type_index &type) PURE;
                STDMETHOD_(LPVOID, getComponent)            (THIS_ const std::type_index &type) PURE;

                template <typename CLASS>
                bool hasComponent(void)
                {
                    return hasComponent(typeid(CLASS));
                }

                template<typename... ARGS>
                bool hasComponents(void)
                {
                    std::vector<bool> hasComponents({ hasComponent<ARGS>()... });
                    return std::find_if(hasComponents.begin(), hasComponents.end(), [&](bool hasComponent) -> bool
                    {
                        return !hasComponent;
                    }) == hasComponents.end();
                }

                template <typename CLASS>
                CLASS &getComponent(void)
                {
                    return *static_cast<CLASS *>(getComponent(typeid(CLASS)));
                }
            };

            DECLARE_INTERFACE_IID(Class, "BD97404A-DE56-4DDC-BB34-3190FD51DEE5");

            DECLARE_INTERFACE_IID(Interface, "43DF2FD7-3BE2-4333-86ED-CB1221C6599B") : virtual public IUnknown
            {
                STDMETHOD(initialize)                       (THIS_ IUnknown *initializerContext) PURE;
                
                STDMETHOD_(void, update)                    (THIS_ float frameTime = 0.0f) PURE;

                STDMETHOD(load)                             (THIS_ LPCWSTR fileName) PURE;
                STDMETHOD(save)                             (THIS_ LPCWSTR fileName) PURE;
                STDMETHOD_(void, free)                      (THIS) PURE;

                STDMETHOD_(Entity *, createEntity)          (THIS_ const std::unordered_map<CStringW, std::unordered_map<CStringW, CStringW>> &entityParameterList, LPCWSTR name = nullptr) PURE;
                STDMETHOD_(void, killEntity)                (THIS_ Entity *entity) PURE;
                STDMETHOD_(Entity *, getNamedEntity)        (THIS_ LPCWSTR name) PURE;

                STDMETHOD_(void, listEntities)              (THIS_ std::function<void(Entity *)> onEntity, bool runInParallel = false) PURE;

                template<typename... ARGS>
                void listEntities(std::function<void(Entity *)> onEntity)
                {
                    listEntities([&](Entity *entity) -> void
                    {
                        if (entity->hasComponents<ARGS...>())
                        {
                            onEntity(entity);
                        }
                    });
                }
            };

            DECLARE_INTERFACE_IID(Observer, "51D6E5E6-2AD3-4D61-A704-8E6515F024F9") : virtual public Observer
            {
                STDMETHOD_(void, onLoadBegin)               (THIS) { };
                STDMETHOD_(void, onLoadEnd)                 (THIS_ HRESULT resultValue) { };
                STDMETHOD_(void, onFree)                    (THIS) { };

                STDMETHOD_(void, onEntityCreated)           (THIS_ Entity *entity) { };
                STDMETHOD_(void, onEntityDestroyed)         (THIS_ Entity *entity) { };

                STDMETHOD_(void, onUpdateBegin)             (THIS_ float frameTime) { };
                STDMETHOD_(void, onUpdate)                  (THIS_ float frameTime) { };
                STDMETHOD_(void, onUpdateEnd)               (THIS_ float frameTime) { };
            };
        }; // namespace Population
    }; // namespace Engine
}; // namespace Gek