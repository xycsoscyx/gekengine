#include "GEK/GUI/Utilities.hpp"

namespace Gek
{
    namespace UI
    {
        bool InputString(char const *label, std::string &string, ImGuiInputTextFlags flags, ImGuiTextEditCallback callback, void *userData)
        {
            char text[256];
            strcpy(text, string.c_str());
            bool changed = ImGui::InputText(label, text, 255, flags, callback, userData);
            if (changed)
            {
                string = text;
            }

            return changed;
        }
    }; // namespace UI
}; // namespace Gek