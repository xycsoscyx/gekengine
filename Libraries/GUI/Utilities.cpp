#include "GEK/GUI/Utilities.hpp"

namespace Gek
{
    namespace UI
    {
        bool InputString(const char* label, std::string &string, ImGuiInputTextFlags flags, ImGuiTextEditCallback callback, void* user_data)
        {
            char text[256];
            strcpy(text, string.c_str());
            bool changed = ImGui::InputText(label, text, 255, flags, callback, user_data);
            if (changed)
            {
                string = text;
            }

            return changed;
        }
    }; // namespace UI
}; // namespace Gek