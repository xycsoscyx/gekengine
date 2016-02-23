#pragma once

#include "GEK\Engine\Population.h"
#include "GEK\Engine\Component.h"
#include <concurrent_unordered_set.h>

namespace Gek
{
    template <typename... REQUIRED>
    class ProcessorMixin
        : public Processor
        , public PopulationObserver
    {
    private:
        concurrency::concurrent_unordered_set<Entity *> entityList;

    public:
        virtual ~ProcessorMixin(void)
        {
        }

        // ProcessorMixin
        STDMETHOD_(void, onMixinLoadEnd)(THIS_ HRESULT resultValue) { };
        STDMETHOD_(void, onMixinFree)(THIS) { };
        STDMETHOD_(void, onMixinEntityCreated)(THIS_ Entity *entity) { };
        STDMETHOD_(void, onMixinEntityDestroyed)(THIS_ Entity *entity) { };

        // PopulationObserver
        STDMETHODIMP_(void) onLoadEnd(HRESULT resultValue)
        {
            if (FAILED(resultValue))
            {
                entityList.clear();
            }

            onMixinLoadEnd(resultValue);
        }

        STDMETHODIMP_(void) onFree(void)
        {
            entityList.clear();
            onMixinFree();
        }

        STDMETHODIMP_(void) onEntityCreated(Entity *entity)
        {
            REQUIRE_VOID_RETURN(entity);

            if (entity->hasComponents<REQUIRED...>())
            {
                entityList.insert(entity);
                onMixinEntityCreated(entity);
            }
        }

        STDMETHODIMP_(void) onEntityDestroyed(Entity *entity)
        {
            REQUIRE_VOID_RETURN(entity);

            if (entityList.unsafe_erase(entity) > 0)
            {
                onMixinEntityDestroyed(entity);
            }
        }
    };
}; // namespace Gek