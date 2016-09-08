#pragma once

#include "GEK\Context\Context.h"
#include "GEK\Engine\Resources.h"
#include <unordered_map>

namespace Gek
{
    namespace Engine
    {
        GEK_INTERFACE(Material)
        {
            GEK_START_EXCEPTIONS();
            GEK_ADD_EXCEPTION(MissingParameters);

            using ResourceMap = std::unordered_map<String, ResourceHandle>;
            using PassMap = std::unordered_map<String, ResourceMap>;

            GEK_INTERFACE(Data)
            {
                virtual ~Data(void) = default;
            };

            virtual Data * const getData(void) const = 0;
			virtual RenderStateHandle getRenderState(void) const = 0;
		};
    }; // namespace Engine
}; // namespace Gek
