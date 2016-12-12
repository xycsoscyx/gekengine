#include "GEK/GUI/Utilities.hpp"
#include <unordered_map>

namespace ImGui
{
    std::unordered_map<::Gek::StringUTF8, ::Gek::StringUTF8> labelStringMap;
    bool InputString(const char* label, ::Gek::String &string, ImGuiInputTextFlags flags, ImGuiTextEditCallback callback, void* user_data)
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

    namespace Gek
    {
        bool InputFloat(const char* label, float* v, float step, float step_fast, int decimal_precision, ImGuiInputTextFlags extra_flags)
        {
            ImGui::Text(label);
            return ImGui::InputFloat(::Gek::StringUTF8::Format("##%v", label), v, step, step_fast, decimal_precision, extra_flags);
        }

        bool InputFloat2(const char* label, float v[2], int decimal_precision, ImGuiInputTextFlags extra_flags)
        {
            ImGui::Text(label);
            return ImGui::InputFloat2(::Gek::StringUTF8::Format("##%v", label), v, decimal_precision, extra_flags);
        }

        bool InputFloat3(const char* label, float v[3], int decimal_precision, ImGuiInputTextFlags extra_flags)
        {
            ImGui::Text(label);
            return ImGui::InputFloat3(::Gek::StringUTF8::Format("##%v", label), v, decimal_precision, extra_flags);
        }

        bool InputFloat4(const char* label, float v[4], int decimal_precision, ImGuiInputTextFlags extra_flags)
        {
            ImGui::Text(label);
            return ImGui::InputFloat4(::Gek::StringUTF8::Format("##%v", label), v, decimal_precision, extra_flags);
        }

        bool InputString(const char* label, ::Gek::String &string, ImGuiInputTextFlags flags, ImGuiTextEditCallback callback, void* user_data)
        {
            ImGui::Text(label);
            return ImGui::InputString(::Gek::StringUTF8::Format("##%v", label), string, flags, callback, user_data);
        }

        bool ListBox(const char* label, int* current_item, bool(*items_getter)(void* data, int idx, const char** out_text), void* data, int items_count, int height_in_items)
        {
            ImGui::Text(label);
            return ImGui::ListBox(::Gek::StringUTF8::Format("##%v", label), current_item, items_getter, data, items_count, height_in_items);
        }
    }; // namespace Gek
}; // namespace ImGui
