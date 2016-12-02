#include "GEK/GUI/Utilities.hpp"
#include <unordered_map>

namespace ImGui
{
    std::unordered_map<Gek::StringUTF8, Gek::StringUTF8> labelStringMap;
    bool InputString(const char* label, Gek::String &string, ImGuiInputTextFlags flags, ImGuiTextEditCallback callback, void* user_data)
    {
        auto &internalSearch = labelStringMap.insert(std::make_pair(label, string));
        auto &internalString = internalSearch.first->second;
        if (internalSearch.second)
        {
            internalString.reserve(256);
        }
            
        if (InputText(label, &internalString.front(), 255, flags, callback, user_data))
        {
            string = internalString;
            return true;
        }

        return false;
    }
}; // namespace ImGui
