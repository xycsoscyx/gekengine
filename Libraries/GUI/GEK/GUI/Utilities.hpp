/// @file
/// @author Todd Zupan <toddzupan@gmail.com>
/// @version $Revision: 1d50799f92d32e762c5d618bca16d2fe7fbeb872 $
/// @section LICENSE
/// https://en.wikipedia.org/wiki/MIT_License
/// @section DESCRIPTION
/// Last Changed: $Date:   Fri Oct 21 04:24:02 2016 +0000 $
#pragma once

#include "GEK/Math/Matrix4x4.hpp"
#include "GEK/Utility/String.hpp"

#include <imgui.h>
#include <ImGuizmo.h>

#ifndef NO_IMGUIHELPER_SERIALIZATION
#define NO_IMGUIHELPER_SERIALIZATION 1
#endif

#include <imguihelper.h>
#include <imguivariouscontrols.h>

namespace ImGui
{
    bool InputString(const char* label, Gek::String &string, ImGuiInputTextFlags flags = 0, ImGuiTextEditCallback callback = NULL, void* user_data = NULL);
}; // namespace ImGui
