/// @file
/// @author Todd Zupan <toddzupan@gmail.com>
/// @version $Revision: 1d50799f92d32e762c5d618bca16d2fe7fbeb872 $
/// @section LICENSE
/// https://en.wikipedia.org/wiki/MIT_License
/// @section DESCRIPTION
/// Last Changed: $Date:   Fri Oct 21 04:24:02 2016 +0000 $
#pragma once

#include "GEK/GUI/Utilities.hpp"

namespace Gek
{
    namespace UI
    {
        namespace Dock
        {
            enum class Location
            {
                Left = 0,
                Right,
                Top,
                Bottom,
                Tab,
                Float,
                None,
            };

            struct Context;

            struct WorkSpace
            {
                WorkSpace(void);
                ~WorkSpace(void);

                void Begin(char const *label = nullptr, ImVec2 const &workspace = ImVec2(0, 0), bool showBorder = false, ImVec2 const &splitterSize = ImVec2(3.0f, 3.0f));
                void End(void);

                void SetNextLocation(Location location);
                bool BeginTab(char const *label, bool *opened = NULL, ImGuiWindowFlags extraFlags = 0, ImVec2 const &defaultSize = ImVec2(-1, -1));
                void EndTab(void);

                void SetActive(void);

            private:
                Context *context = nullptr;
            };
        }; // Dock
    }; // namespace UI
}; // namespace Gek
