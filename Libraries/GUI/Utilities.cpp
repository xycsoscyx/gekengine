#include "GEK/GUI/Utilities.hpp"
#include <unordered_map>

namespace Gek
{
    namespace UI
    {
        ImVec2 GetWindowContentRegionSize(void)
        {
            return (ImGui::GetWindowContentRegionMax() - ImGui::GetWindowContentRegionMin());
        }

        ImVec4 PushStyleColor(ImGuiCol styleIndex, const ImVec4& color)
        {
            ImVec4 oldValue = ImGui::GetStyle().Colors[styleIndex];
            ImGui::PushStyleColor(styleIndex, color);
            return oldValue;
        }

#define GET_STYLE(OFFSET) (void *)((unsigned char *)&GImGui->Style + OFFSET)

        static const void *GlobalStyleModifiers[ImGuiStyleVar_Count_] =
        {
            GET_STYLE((ImU32)IM_OFFSETOF(ImGuiStyle, Alpha)),                // ImGuiStyleVar_Alpha
            GET_STYLE((ImU32)IM_OFFSETOF(ImGuiStyle, WindowPadding)),        // ImGuiStyleVar_WindowPadding
            GET_STYLE((ImU32)IM_OFFSETOF(ImGuiStyle, WindowRounding)),       // ImGuiStyleVar_WindowRounding
            GET_STYLE((ImU32)IM_OFFSETOF(ImGuiStyle, WindowMinSize)),        // ImGuiStyleVar_WindowMinSize
            GET_STYLE((ImU32)IM_OFFSETOF(ImGuiStyle, ChildWindowRounding)),  // ImGuiStyleVar_ChildWindowRounding
            GET_STYLE((ImU32)IM_OFFSETOF(ImGuiStyle, FramePadding)),         // ImGuiStyleVar_FramePadding
            GET_STYLE((ImU32)IM_OFFSETOF(ImGuiStyle, FrameRounding)),        // ImGuiStyleVar_FrameRounding
            GET_STYLE((ImU32)IM_OFFSETOF(ImGuiStyle, ItemSpacing)),          // ImGuiStyleVar_ItemSpacing
            GET_STYLE((ImU32)IM_OFFSETOF(ImGuiStyle, ItemInnerSpacing)),     // ImGuiStyleVar_ItemInnerSpacing
            GET_STYLE((ImU32)IM_OFFSETOF(ImGuiStyle, IndentSpacing)),        // ImGuiStyleVar_IndentSpacing
            GET_STYLE((ImU32)IM_OFFSETOF(ImGuiStyle, GrabMinSize)),          // ImGuiStyleVar_GrabMinSize
            GET_STYLE((ImU32)IM_OFFSETOF(ImGuiStyle, ButtonTextAlign)),      // ImGuiStyleVar_ButtonTextAlign
        };

        float PushStyleVar(ImGuiStyleVar styleIndex, float value)
        {
            float oldValue = *(float *)GlobalStyleModifiers[styleIndex];
            ImGui::PushStyleVar(styleIndex, value);
            return oldValue;
        }

        ImVec2 PushStyleVar(ImGuiStyleVar styleIndex, ImVec2 const &value)
        {
            ImVec2 oldValue = *(ImVec2 *)GlobalStyleModifiers[styleIndex];
            ImGui::PushStyleVar(styleIndex, value);
            return oldValue;
        }

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

        bool CheckButton(char const *label, bool *storedState, ImVec2 const &size)
        {
            const bool state = storedState ? *storedState : false;
            if (!state)
            {
                const auto &style(ImGui::GetStyle());
                ImVec4 CheckButtonColor = ImVec4(1.0f, 1.0f, 1.0f, 0.0f) - style.Colors[ImGuiCol_Button];
                ImVec4 CheckButtonActiveColor = ImVec4(1.0f, 1.0f, 1.0f, 0.0f) - style.Colors[ImGuiCol_ButtonActive];
                ImGui::PushStyleColor(ImGuiCol_Button, CheckButtonColor);
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, CheckButtonActiveColor);
            }

            bool clicked = ImGui::Button(label, size);
            if (clicked && storedState)
            {
                *storedState = !(*storedState);
            }

            if (!state)
            {
                ImGui::PopStyleColor(2);
            }

            return clicked;
        }

        bool CheckButton(char const *label, bool state, ImVec2 const &size)
        {
            return CheckButton(label, &state, size);
        }

        bool RadioButton(char const *label, int *storedState, int buttonState, ImVec2 const &size)
        {
            bool clicked = CheckButton(label, *storedState == buttonState, size);
            if (clicked)
            {
                *storedState = buttonState;
            }

            return clicked;
        }

        void TextFrame(char const *label, ImVec2 const &requestedSize, ImGuiButtonFlags flags)
        {
            ImGuiWindow *window = ImGui::GetCurrentWindow();
            if (window->SkipItems)
            {
                return;
            }

            const ImGuiStyle &style = ImGui::GetStyle();
            const ImGuiID labelIdentity = window->GetID(label);
            const ImVec2 labelSize = ImGui::CalcTextSize(label, nullptr, true);

            ImVec2 position = window->DC.CursorPos;
            // Try to vertically align buttons that are smaller/have no padding so that text baseline matches (bit hacky, since it shouldn't be a flag)
            if ((flags & ImGuiButtonFlags_AlignTextBaseLine) && style.FramePadding.y < window->DC.CurrentLineTextBaseOffset)
            {
                position.y += window->DC.CurrentLineTextBaseOffset - style.FramePadding.y;
            }

            ImVec2 size = ImGui::CalcItemSize(requestedSize, labelSize.x + style.FramePadding.x * 2.0f, labelSize.y + style.FramePadding.y * 2.0f);

            const ImRect boundingBox(position, position + size);
            ImGui::ItemSize(boundingBox, style.FramePadding.y);
            if (ImGui::ItemAdd(boundingBox, &labelIdentity))
            {
                const ImU32 color = ImGui::GetColorU32(ImGuiCol_Button);
                ImGui::RenderFrame(boundingBox.Min, boundingBox.Max, color, true, style.FrameRounding);
                ImGui::RenderTextClipped(boundingBox.Min + style.FramePadding, boundingBox.Max - style.FramePadding, label, nullptr, &labelSize, style.ButtonTextAlign, &boundingBox);
            }
        }

        bool SliderAngle2(char const *label, float v_rad[2], float v_degrees_min, float v_degrees_max)
        {
            float v_deg[]
            {
            v_rad[0] * 360.0f / (2 * IM_PI),
                v_rad[1] * 360.0f / (2 * IM_PI),
            };

            bool value_changed = ImGui::SliderFloat2(label, v_deg, v_degrees_min, v_degrees_max, "%.0f deg", 1.0f);
            v_rad[0] = v_deg[0] * (2 * IM_PI) / 360.0f;
            v_rad[1] = v_deg[1] * (2 * IM_PI) / 360.0f;
            return value_changed;
        }

        bool SliderAngle3(char const *label, float v_rad[3], float v_degrees_min, float v_degrees_max)
        {
            float v_deg[]
            {
           v_rad[0] * 360.0f / (2 * IM_PI),
               v_rad[1] * 360.0f / (2 * IM_PI),
               v_rad[2] * 360.0f / (2 * IM_PI),
            };

            bool value_changed = ImGui::SliderFloat3(label, v_deg, v_degrees_min, v_degrees_max, "%.0f deg", 1.0f);
            v_rad[0] = v_deg[0] * (2 * IM_PI) / 360.0f;
            v_rad[1] = v_deg[1] * (2 * IM_PI) / 360.0f;
            v_rad[2] = v_deg[2] * (2 * IM_PI) / 360.0f;
            return value_changed;
        }

        bool SliderAngle4(char const *label, float v_rad[4], float v_degrees_min, float v_degrees_max)
        {
            float v_deg[]
            {
           v_rad[0] * 360.0f / (2 * IM_PI),
               v_rad[1] * 360.0f / (2 * IM_PI),
               v_rad[2] * 360.0f / (2 * IM_PI),
               v_rad[3] * 360.0f / (2 * IM_PI),
            };

            bool value_changed = ImGui::SliderFloat4(label, v_deg, v_degrees_min, v_degrees_max, "%.0f deg", 1.0f);
            v_rad[0] = v_deg[0] * (2 * IM_PI) / 360.0f;
            v_rad[1] = v_deg[1] * (2 * IM_PI) / 360.0f;
            v_rad[2] = v_deg[2] * (2 * IM_PI) / 360.0f;
            v_rad[3] = v_deg[3] * (2 * IM_PI) / 360.0f;
            return value_changed;
        }

        struct DockContext
        {
            enum EndAction_
            {
                EndAction_None,
                EndAction_Panel,
                EndAction_End,
                EndAction_EndChild
            };

            enum Status_
            {
                Status_Docked,
                Status_Float,
                Status_Dragged
            };

            struct Dock
            {
                Dock(ImGuiWindowFlags extra_flags)
                    : label(nullptr)
                    , id(0)
                    , next_tab(nullptr)
                    , prev_tab(nullptr)
                    , parent(nullptr)
                    , active(true)
                    , pos(0, 0)
                    , size(-1, -1)
                    , status(Status_Float)
                    , extra_flags(extra_flags)
                    , last_frame(0)
                    , invalid_frames(0)
                    , opened(false)
                    , first(false)
                    , location{ 0 }
                    , children{ nullptr, nullptr }

                {
                }

                ~Dock()
                {
                    ImGui::MemFree(label);
                }

                ImVec2 getMinSize() const
                {
                    if (!children[0]) return ImVec2(16, 16 + ImGui::GetTextLineHeightWithSpacing());

                    ImVec2 s0 = children[0]->getMinSize();
                    ImVec2 s1 = children[1]->getMinSize();
                    return isHorizontal() ?
                        ImVec2(s0.x + s1.x, ImMax(s0.y, s1.y))
                        : ImVec2(ImMax(s0.x, s1.x), s0.y + s1.y);
                }

                bool isHorizontal() const
                {
                    return children[0]->pos.x < children[1]->pos.x;
                }

                void setParent(Dock* dock)
                {
                    parent = dock;
                    for (Dock* tmp = prev_tab; tmp; tmp = tmp->prev_tab) tmp->parent = dock;
                    for (Dock* tmp = next_tab; tmp; tmp = tmp->next_tab) tmp->parent = dock;
                }

                Dock& getRoot()
                {
                    Dock *dock = this;
                    while (dock->parent)
                        dock = dock->parent;
                    return *dock;
                }

                Dock& getSibling()
                {
                    IM_ASSERT(parent);
                    if (parent->children[0] == &getFirstTab()) return *parent->children[1];
                    return *parent->children[0];
                }


                Dock& getFirstTab()
                {
                    Dock* tmp = this;
                    while (tmp->prev_tab) tmp = tmp->prev_tab;
                    return *tmp;
                }

                void setActive()
                {
                    active = true;
                    for (Dock* tmp = prev_tab; tmp; tmp = tmp->prev_tab) tmp->active = false;
                    for (Dock* tmp = next_tab; tmp; tmp = tmp->next_tab) tmp->active = false;
                }

                bool isContainer() const
                {
                    return children[0] != nullptr;
                }

                void setChildrenPosSize(const ImVec2& _pos, const ImVec2& _size)
                {
                    ImVec2 s = children[0]->size;
                    if (isHorizontal())
                    {
                        s.y = _size.y;
                        s.x = (float)int(_size.x * children[0]->size.x / (children[0]->size.x + children[1]->size.x));
                        if (s.x < children[0]->getMinSize().x)
                        {
                            s.x = children[0]->getMinSize().x;
                        }
                        else if (_size.x - s.x < children[1]->getMinSize().x)
                        {
                            s.x = _size.x - children[1]->getMinSize().x;
                        }

                        children[0]->setPosSize(_pos, s);
                        s.x = _size.x - children[0]->size.x;

                        ImVec2 p = _pos;
                        p.x += children[0]->size.x;
                        children[1]->setPosSize(p, s);
                    }
                    else
                    {
                        s.x = _size.x;
                        s.y = (float)int(_size.y * children[0]->size.y / (children[0]->size.y + children[1]->size.y));
                        if (s.y < children[0]->getMinSize().y)
                        {
                            s.y = children[0]->getMinSize().y;
                        }
                        else if (_size.y - s.y < children[1]->getMinSize().y)
                        {
                            s.y = _size.y - children[1]->getMinSize().y;
                        }

                        children[0]->setPosSize(_pos, s);
                        s.y = _size.y - children[0]->size.y;

                        ImVec2 p = _pos;
                        p.y += children[0]->size.y;
                        children[1]->setPosSize(p, s);
                    }
                }

                void setPosSize(const ImVec2& _pos, const ImVec2& _size)
                {
                    size = _size;
                    pos = _pos;
                    for (Dock* tmp = prev_tab; tmp; tmp = tmp->prev_tab)
                    {
                        tmp->size = _size;
                        tmp->pos = _pos;
                    }

                    for (Dock* tmp = next_tab; tmp; tmp = tmp->next_tab)
                    {
                        tmp->size = _size;
                        tmp->pos = _pos;
                    }

                    if (!isContainer()) return;
                    setChildrenPosSize(_pos, _size);
                }

                char* label;
                ImU32 id;
                Dock* next_tab;
                Dock* prev_tab;
                Dock* children[2];
                Dock* parent;
                bool active;
                ImVec2 pos;
                ImVec2 size;
                Status_ status;
                int last_frame;
                int invalid_frames;
                char location[16];
                bool opened;
                bool first;
                ImGuiWindowFlags extra_flags;
            };

            ImVector<Dock*> m_docks;
            ImVec2 m_drag_offset;
            Dock* m_current;
            Dock *m_next_parent;
            int m_last_frame;
            EndAction_ m_end_action;
            ImVec2 m_workspace_pos;
            ImVec2 m_workspace_size;
            DockSlot m_next_dock_slot;
            ImVec2 m_splitSize;

            DockContext()
                : m_current(nullptr)
                , m_next_parent(nullptr)
                , m_last_frame(0)
                , m_next_dock_slot(DockSlot::Tab)
                , m_splitSize(3.0f, 3.0f)
            {
            }

            ~DockContext()
            {
                ShutdownDock();//New
            }

            Dock& getDock(char const *label, bool opened, const ImVec2& default_size, ImGuiWindowFlags extra_flags)
            {
                ImU32 id = ImHash(label, 0);
                for (int i = 0; i < m_docks.size(); ++i)
                {
                    if (m_docks[i]->id == id) return *m_docks[i];
                }

                Dock* new_dock = (Dock*)ImGui::MemAlloc(sizeof(Dock));
                IM_PLACEMENT_NEW(new_dock) Dock(extra_flags);
                m_docks.push_back(new_dock);
                new_dock->label = ImStrdup(label);
                IM_ASSERT(new_dock->label);
                new_dock->id = id;
                new_dock->setActive();
                new_dock->status = (m_docks.size() == 1) ? Status_Docked : Status_Float;
                new_dock->pos = ImVec2(0, 0);
                //new_dock->size = ImGui::GetIO().DisplaySize;
                new_dock->size.x = default_size.x < 0 ? ImGui::GetIO().DisplaySize.x : default_size.x;
                new_dock->size.y = default_size.y < 0 ? ImGui::GetIO().DisplaySize.y : default_size.y;
                new_dock->opened = opened;
                new_dock->first = true;
                new_dock->last_frame = 0;
                new_dock->invalid_frames = 0;
                new_dock->location[0] = 0;
                return *new_dock;
            }

            void putInBackground()
            {
                ImGuiWindow* win = ImGui::GetCurrentWindow();
                ImGuiContext& g = *GImGui;
                if (g.Windows[0] == win) return;

                for (int i = 0; i < g.Windows.Size; i++)
                {
                    if (g.Windows[i] == win)
                    {
                        for (int j = i - 1; j >= 0; --j)
                        {
                            g.Windows[j + 1] = g.Windows[j];
                        }

                        g.Windows[0] = win;
                        break;
                    }
                }
            }

            void splits()
            {
                if (ImGui::GetFrameCount() == m_last_frame) return;
                m_last_frame = ImGui::GetFrameCount();

                putInBackground();
                for (int i = 0; i < m_docks.size(); ++i)
                {
                    Dock& dock = *m_docks[i];
                    if (!dock.parent && (dock.status == Status_Docked))
                    {
                        dock.setPosSize(m_workspace_pos, m_workspace_size);
                    }
                }

                ImU32 color = ImGui::GetColorU32(ImGuiCol_Button);
                ImU32 color_hovered = ImGui::GetColorU32(ImGuiCol_ButtonHovered);
                ImDrawList* draw_list = ImGui::GetWindowDrawList();
                ImGuiIO& io = ImGui::GetIO();
                for (int i = 0; i < m_docks.size(); ++i)
                {
                    Dock& dock = *m_docks[i];
                    if (!dock.isContainer()) continue;

                    ImGui::PushID(i);
                    if (!ImGui::IsMouseDown(0)) dock.status = Status_Docked;

                    ImVec2 pos0 = dock.children[0]->pos;
                    ImVec2 pos1 = dock.children[1]->pos;
                    ImVec2 size0 = dock.children[0]->size;
                    ImVec2 size1 = dock.children[1]->size;

                    ImGuiMouseCursor cursor;

                    ImVec2 dsize(0, 0);
                    ImVec2 min_size0 = dock.children[0]->getMinSize();
                    ImVec2 min_size1 = dock.children[1]->getMinSize();
                    if (dock.isHorizontal())
                    {
                        cursor = ImGuiMouseCursor_ResizeEW;
                        ImGui::SetCursorScreenPos(ImVec2(dock.pos.x + size0.x - m_splitSize.x * 0.5f + 1.0f, dock.pos.y));
                        ImGui::InvisibleButton("split", ImVec2(m_splitSize.x, dock.size.y));
                        if (dock.status == Status_Dragged) dsize.x = io.MouseDelta.x;
                        dsize.x = -ImMin(-dsize.x, dock.children[0]->size.x - min_size0.x);
                        dsize.x = ImMin(dsize.x, dock.children[1]->size.x - min_size1.x);
                        size0 += dsize;
                        size1 -= dsize;
                        pos0 = dock.pos;
                        pos1.x = pos0.x + size0.x;
                        pos1.y = dock.pos.y;
                        size0.y = size1.y = dock.size.y;
                        size1.x = ImMax(min_size1.x, dock.size.x - size0.x);
                        size0.x = ImMax(min_size0.x, dock.size.x - size1.x);
                    }
                    else
                    {
                        cursor = ImGuiMouseCursor_ResizeNS;
                        ImGui::SetCursorScreenPos(ImVec2(dock.pos.x, dock.pos.y + size0.y - m_splitSize.y * 0.5f + 1.0f));
                        ImGui::InvisibleButton("split", ImVec2(dock.size.x, m_splitSize.y));
                        if (dock.status == Status_Dragged) dsize.y = io.MouseDelta.y;
                        dsize.y = -ImMin(-dsize.y, dock.children[0]->size.y - min_size0.y);
                        dsize.y = ImMin(dsize.y, dock.children[1]->size.y - min_size1.y);
                        size0 += dsize;
                        size1 -= dsize;
                        pos0 = dock.pos;
                        pos1.x = dock.pos.x;
                        pos1.y = pos0.y + size0.y;
                        size0.x = size1.x = dock.size.x;
                        size1.y = ImMax(min_size1.y, dock.size.y - size0.y);
                        size0.y = ImMax(min_size0.y, dock.size.y - size1.y);
                    }

                    dock.children[0]->setPosSize(pos0, size0);
                    dock.children[1]->setPosSize(pos1, size1);
                    if (ImGui::IsItemHovered())
                    {
                        ImGui::SetMouseCursor(cursor);
                    }

                    if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(0))
                    {
                        if (!(dock.extra_flags & ImGuiWindowFlags_NoMove))
                        {
                            dock.status = Status_Dragged;
                        }
                    }

                    draw_list->AddRectFilled(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImGui::IsItemHovered() ? color_hovered : color);
                    ImGui::PopID();
                }
            }

            void checkNonexistent()
            {
                int frame_limit = ImMax(0, ImGui::GetFrameCount() - 2);
                for (int i = 0; i < m_docks.size(); ++i)
                {
                    Dock *dock = m_docks[i];
                    if (dock->isContainer()) continue;
                    if (dock->status == Status_Float) continue;
                    if (dock->last_frame < frame_limit)
                    {
                        ++dock->invalid_frames;
                        if (dock->invalid_frames > 2)
                        {
                            doUndock(*dock);
                            dock->status = Status_Float;
                        }

                        return;
                    }

                    dock->invalid_frames = 0;
                }
            }

            Dock* getDockAt(const ImVec2& /*pos*/) const
            {
                for (int i = 0; i < m_docks.size(); ++i)
                {
                    Dock& dock = *m_docks[i];
                    if (dock.isContainer()) continue;
                    if (dock.status != Status_Docked) continue;
                    if (ImGui::IsMouseHoveringRect(dock.pos, dock.pos + dock.size, false))
                    {
                        return &dock;
                    }
                }

                return nullptr;
            }

            static ImRect getDockedRect(const ImRect& rect, DockSlot dock_slot)
            {
                ImVec2 half_size = rect.GetSize() * 0.5f;
                switch (dock_slot)
                {
                default:
                    return rect;

                case DockSlot::Top:
                    return ImRect(rect.Min, rect.Min + ImVec2(rect.Max.x - rect.Min.x, half_size.y)); //  @r-lyeh

                case DockSlot::Right:
                    return ImRect(rect.Min + ImVec2(half_size.x, 0), rect.Max);

                case DockSlot::Bottom:
                    return ImRect(rect.Min + ImVec2(0, half_size.y), rect.Max);

                case DockSlot::Left:
                    return ImRect(rect.Min, ImVec2(rect.Min.x + half_size.x, rect.Max.y));
                };
            }

            static ImRect getSlotRect(ImRect parent_rect, DockSlot dock_slot)
            {
                ImVec2 size = parent_rect.Max - parent_rect.Min;
                ImVec2 center = parent_rect.Min + size * 0.5f;
                switch (dock_slot)
                {
                default: return ImRect(center - ImVec2(20, 20), center + ImVec2(20, 20));
                
                case DockSlot::Top:
                    return ImRect(center + ImVec2(-20, -50), center + ImVec2(20, -30));
                
                case DockSlot::Right:
                    return ImRect(center + ImVec2(30, -20), center + ImVec2(50, 20));
                
                case DockSlot::Bottom:
                    return ImRect(center + ImVec2(-20, +30), center + ImVec2(20, 50));
                
                case DockSlot::Left:
                    return ImRect(center + ImVec2(-50, -20), center + ImVec2(-30, 20));
                };
            }

            static ImRect getSlotRectOnBorder(ImRect parent_rect, DockSlot dock_slot)
            {
                ImVec2 size = parent_rect.Max - parent_rect.Min;
                ImVec2 center = parent_rect.Min + size * 0.5f;
                switch (dock_slot)
                {
                case DockSlot::Top:
                    return ImRect(ImVec2(center.x - 20, parent_rect.Min.y + 10),
                        ImVec2(center.x + 20, parent_rect.Min.y + 30));

                case DockSlot::Left:
                    return ImRect(ImVec2(parent_rect.Min.x + 10, center.y - 20),
                        ImVec2(parent_rect.Min.x + 30, center.y + 20));

                case DockSlot::Bottom:
                    return ImRect(ImVec2(center.x - 20, parent_rect.Max.y - 30),
                        ImVec2(center.x + 20, parent_rect.Max.y - 10));

                case DockSlot::Right:
                    return ImRect(ImVec2(parent_rect.Max.x - 30, center.y - 20),
                        ImVec2(parent_rect.Max.x - 10, center.y + 20));

                default:
                    IM_ASSERT(false);
                };

                IM_ASSERT(false);
                return ImRect();
            }

            Dock* getRootDock()
            {
                for (int i = 0; i < m_docks.size(); ++i)
                {
                    if (!m_docks[i]->parent &&
                        (m_docks[i]->status == Status_Docked || m_docks[i]->children[0]))
                    {
                        return m_docks[i];
                    }
                }

                return nullptr;
            }

            bool dockSlots(Dock& dock, Dock* dest_dock, const ImRect& rect, bool on_border)
            {
                ImDrawList* canvas = ImGui::GetWindowDrawList();
                ImU32 color = ImGui::GetColorU32(ImGuiCol_Button);		    // Color of all the available "spots"
                ImU32 color_hovered = ImGui::GetColorU32(ImGuiCol_ButtonHovered);  // Color of the hovered "spot"
                ImU32 docked_rect_color = color;
                ImVec2 mouse_pos = ImGui::GetIO().MousePos;
                ImTextureID texture = nullptr;
                if (gImGuiDockReuseTabWindowTextureIfAvailable)
                {
#	ifdef IMGUITABWINDOW_H_
                    texture = ImGui::TabWindow::DockPanelIconTextureID;	// Nope. It doesn't look OK.
                    if (texture)
                    {
                        color = 0x00FFFFFF | 0x90000000;
                        color_hovered = (color_hovered & 0x00FFFFFF) | 0x90000000;
                        docked_rect_color = (docked_rect_color & 0x00FFFFFF) | 0x80000000;

                        canvas->ChannelsSplit(2);	// Solves overlay order. But won't it break something else ?
                    }
#	endif ////IMGUITABWINDOW_H_
                }

                for (int i = 0; i < (on_border ? 4 : 5); ++i)
                {
                    const DockSlot iSlot = static_cast<DockSlot>(i);
                    ImRect r =
                        on_border ? getSlotRectOnBorder(rect, iSlot) : getSlotRect(rect, iSlot);
                    bool hovered = r.Contains(mouse_pos);
                    ImU32 color_to_use = hovered ? color_hovered : color;
                    if (!texture) canvas->AddRectFilled(r.Min, r.Max, color_to_use);
                    else
                    {
#		ifdef IMGUITABWINDOW_H_
                        canvas->ChannelsSetCurrent(0);	// Background
                        switch (iSlot)
                        {
                        case DockSlot::Left:
                        case DockSlot::Right:
                        case DockSlot::Top:
                        case DockSlot::Bottom:
                        {
                            const int uvIndex = (i == 0) ? 3 : (i == 2) ? 0 : (i == 3) ? 2 : i;
                            ImVec2 uv0(0.75f, (float)uvIndex*0.25f), uv1(uv0.x + 0.25f, uv0.y + 0.25f);
                            canvas->AddImage(texture, r.Min, r.Max, uv0, uv1, color_to_use);
                        }
                        break;

                        case DockSlot::Tab:
                            canvas->AddImage(texture, r.Min, r.Max, ImVec2(0.22916f, 0.22916f), ImVec2(0.45834f, 0.45834f), color_to_use);
                            break;

                        default:
                            canvas->AddRectFilled(r.Min, r.Max, color_to_use);
                            break;
                        };

                        canvas->ChannelsSetCurrent(1);	// Foreground
#		endif ////IMGUITABWINDOW_H_
                    }

                    if (!hovered) continue;

                    if (!ImGui::IsMouseDown(0))
                    {
#		ifdef IMGUITABWINDOW_H_
                        if (texture) canvas->ChannelsMerge();
#		endif ////IMGUITABWINDOW_H_
                        doDock(dock, dest_dock ? dest_dock : getRootDock(), iSlot);
                        return true;
                    }

                    ImRect docked_rect = getDockedRect(rect, iSlot);
                    canvas->AddRectFilled(docked_rect.Min, docked_rect.Max, docked_rect_color);
                }

#	ifdef IMGUITABWINDOW_H_
                if (texture) canvas->ChannelsMerge();
#	endif ////IMGUITABWINDOW_H_

                return false;
            }

            void handleDrag(Dock& dock)
            {
                Dock* dest_dock = getDockAt(ImGui::GetIO().MousePos);

                ImGui::Begin("##Overlay",
                    nullptr,
                    ImVec2(0, 0),
                    0.f,
                    ImGuiWindowFlags_Tooltip | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove |
                    ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings |
                    ImGuiWindowFlags_AlwaysAutoResize);
                ImDrawList* canvas = ImGui::GetWindowDrawList();

                canvas->PushClipRectFullScreen();

                ImU32 docked_color = ImGui::GetColorU32(ImGuiCol_FrameBg);
                docked_color = (docked_color & 0x00ffFFFF) | 0x80000000;
                dock.pos = ImGui::GetIO().MousePos - m_drag_offset;
                if (dest_dock)
                {
                    if (dockSlots(dock,
                        dest_dock,
                        ImRect(dest_dock->pos, dest_dock->pos + dest_dock->size),
                        false))
                    {
                        canvas->PopClipRect();
                        ImGui::End();
                        return;
                    }
                }

                if (dockSlots(dock, nullptr, ImRect(m_workspace_pos, m_workspace_pos + m_workspace_size), true))
                {
                    canvas->PopClipRect();
                    ImGui::End();
                    return;
                }

                canvas->AddRectFilled(dock.pos, dock.pos + dock.size, docked_color);
                canvas->PopClipRect();

                if (!ImGui::IsMouseDown(0))
                {
                    dock.status = Status_Float;
                    dock.location[0] = 0;
                    dock.setActive();
                }

                ImGui::End();
            }

            void fillLocation(Dock& dock)
            {
                if (dock.status == Status_Float) return;
                char* c = dock.location;
                Dock* tmp = &dock;
                while (tmp->parent)
                {
                    *c = getLocationCode(tmp);
                    tmp = tmp->parent;
                    ++c;
                }

                *c = 0;
            }

            void doUndock(Dock& dock)
            {
                if (dock.prev_tab)
                    dock.prev_tab->setActive();
                else if (dock.next_tab)
                    dock.next_tab->setActive();
                else
                    dock.active = false;

                Dock* container = dock.parent;
                if (container)
                {
                    Dock& sibling = dock.getSibling();
                    if (container->children[0] == &dock)
                    {
                        container->children[0] = dock.next_tab;
                    }
                    else if (container->children[1] == &dock)
                    {
                        container->children[1] = dock.next_tab;
                    }

                    bool remove_container = !container->children[0] || !container->children[1];
                    if (remove_container)
                    {
                        if (container->parent)
                        {
                            Dock*& child = container->parent->children[0] == container
                                ? container->parent->children[0]
                                : container->parent->children[1];
                            child = &sibling;
                            child->setPosSize(container->pos, container->size);
                            child->setParent(container->parent);
                        }
                        else
                        {
                            if (container->children[0])
                            {
                                container->children[0]->setParent(nullptr);
                                container->children[0]->setPosSize(container->pos, container->size);
                            }

                            if (container->children[1])
                            {
                                container->children[1]->setParent(nullptr);
                                container->children[1]->setPosSize(container->pos, container->size);
                            }
                        }

                        for (int i = 0; i < m_docks.size(); ++i)
                        {
                            if (m_docks[i] == container)
                            {
                                m_docks.erase(m_docks.begin() + i);
                                break;
                            }
                        }

                        if (container == m_next_parent)
                            m_next_parent = nullptr;
                        container->~Dock();
                        ImGui::MemFree(container);
                    }
                }

                if (dock.prev_tab) dock.prev_tab->next_tab = dock.next_tab;
                if (dock.next_tab) dock.next_tab->prev_tab = dock.prev_tab;
                dock.parent = nullptr;
                dock.prev_tab = dock.next_tab = nullptr;
            }

            void drawTabbarListButton(Dock& dock)
            {
                if (!dock.next_tab) return;

                ImDrawList* draw_list = ImGui::GetWindowDrawList();
                if (ImGui::InvisibleButton("list", ImVec2(16, 16)))
                {
                    ImGui::OpenPopup("tab_list_popup");
                }

                if (ImGui::BeginPopup("tab_list_popup"))
                {
                    Dock* tmp = &dock;
                    while (tmp)
                    {
                        bool dummy = false;
                        if (ImGui::Selectable(tmp->label, &dummy))
                        {
                            tmp->setActive();
                            m_next_parent = tmp;
                        }
                        tmp = tmp->next_tab;
                    }
                    ImGui::EndPopup();
                }

                bool hovered = ImGui::IsItemHovered();
                ImVec2 min = ImGui::GetItemRectMin();
                ImVec2 max = ImGui::GetItemRectMax();
                ImVec2 center = (min + max) * 0.5f;
                ImU32 text_color = ImGui::GetColorU32(ImGuiCol_Text);
                ImU32 color_active = ImGui::GetColorU32(ImGuiCol_FrameBgActive);
                draw_list->AddRectFilled(ImVec2(center.x - 4, min.y + 3),
                    ImVec2(center.x + 4, min.y + 5),
                    hovered ? color_active : text_color);
                draw_list->AddTriangleFilled(ImVec2(center.x - 4, min.y + 7),
                    ImVec2(center.x + 4, min.y + 7),
                    ImVec2(center.x, min.y + 12),
                    hovered ? color_active : text_color);
            }

            bool tabbar(Dock& dock, bool close_button)
            {
                float tabbar_height = 2 * ImGui::GetTextLineHeightWithSpacing();
                ImVec2 size(dock.size.x, tabbar_height);
                bool tab_closed = false;

                ImGui::SetCursorScreenPos(dock.pos);
                char tmp[20];
                ImFormatString(tmp, IM_ARRAYSIZE(tmp), "tabs%d", (int)dock.id);
                if (ImGui::BeginChild(tmp, size, true))
                {
                    Dock* dock_tab = &dock;
                    ImDrawList* draw_list = ImGui::GetWindowDrawList();
                    ImU32 color = ImGui::GetColorU32(ImGuiCol_FrameBg);
                    ImU32 color_active = ImGui::GetColorU32(ImGuiCol_FrameBgActive);
                    ImU32 color_hovered = ImGui::GetColorU32(ImGuiCol_FrameBgHovered);
                    ImU32 text_color = ImGui::GetColorU32(ImGuiCol_Text);
                    float line_height = ImGui::GetTextLineHeightWithSpacing();
                    float tab_base;

                    drawTabbarListButton(dock);
                    while (dock_tab)
                    {
                        ImGui::SameLine(0, 15);
                        char const *text_end = ImGui::FindRenderedTextEnd(dock_tab->label);
                        ImVec2 size(ImGui::CalcTextSize(dock_tab->label, text_end).x, line_height);
                        if (ImGui::InvisibleButton(dock_tab->label, size))
                        {
                            dock_tab->setActive();
                            m_next_parent = dock_tab;
                        }

                        if (ImGui::IsItemActive() && ImGui::IsMouseDragging())
                        {
                            if (!(dock_tab->extra_flags & ImGuiWindowFlags_NoMove))
                            {
                                m_drag_offset = ImGui::GetMousePos() - dock_tab->pos;
                                doUndock(*dock_tab);
                                dock_tab->status = Status_Dragged;
                            }
                        }

                        bool hovered = ImGui::IsItemHovered();
                        ImVec2 pos = ImGui::GetItemRectMin();
                        if (dock_tab->active && close_button)
                        {
                            size.x += 16 + ImGui::GetStyle().ItemSpacing.x;
                            ImGui::SameLine();
                            tab_closed = ImGui::InvisibleButton("close", ImVec2(16, 16));
                            ImVec2 center = (ImGui::GetItemRectMin() + ImGui::GetItemRectMax()) * 0.5f;
                            draw_list->AddLine(
                                center + ImVec2(-3.5f, -3.5f), center + ImVec2(3.5f, 3.5f), text_color);
                            draw_list->AddLine(
                                center + ImVec2(3.5f, -3.5f), center + ImVec2(-3.5f, 3.5f), text_color);
                        }

                        tab_base = pos.y;
                        draw_list->PathClear();
                        draw_list->PathLineTo(pos + ImVec2(-15, size.y));
                        draw_list->PathBezierCurveTo(
                            pos + ImVec2(-10, size.y), pos + ImVec2(-5, 0), pos + ImVec2(0, 0), 10);
                        draw_list->PathLineTo(pos + ImVec2(size.x, 0));
                        draw_list->PathBezierCurveTo(pos + ImVec2(size.x + 5, 0),
                            pos + ImVec2(size.x + 10, size.y),
                            pos + ImVec2(size.x + 15, size.y),
                            10);
                        draw_list->PathFillConvex(
                            hovered ? color_hovered : (dock_tab->active ? color_active : color));
                        draw_list->AddText(pos + ImVec2(0, 1), text_color, dock_tab->label, text_end);

                        dock_tab = dock_tab->next_tab;
                    }

                    ImVec2 cp(dock.pos.x, tab_base + line_height);
                    draw_list->AddLine(cp, cp + ImVec2(dock.size.x, 0), color);
                }

                ImGui::EndChild();
                return tab_closed;
            }

            static void setDockPosSize(Dock& dest, Dock& dock, DockSlot dock_slot, Dock& container, ImVec2 const &default_size)
            {
                IM_ASSERT(!dock.prev_tab && !dock.next_tab && !dock.children[0] && !dock.children[1]);

                dest.pos = container.pos;
                dock.pos = container.pos;
                dest.size = container.size;
                dock.size.x = default_size.x <= 0.0f ? container.size.x * 0.5f : default_size.x;
                dock.size.y = default_size.y <= 0.0f ? container.size.y * 0.5f : default_size.y;
                if (dock_slot == DockSlot::Bottom)
                {
                    dest.size.y -= dock.size.y;
                    dock.pos.y += dest.size.y;
                }
                else if (dock_slot == DockSlot::Right)
                {
                    dest.size.x -= dock.size.x;
                    dock.pos.x += dest.size.x;
                }
                else if (dock_slot == DockSlot::Left)
                {
                    dest.size.x -= dock.size.x;
                    dest.pos.x += dock.size.x;
                }
                else if (dock_slot == DockSlot::Top)
                {
                    dest.size.y -= dock.size.y;
                    dest.pos.y += dock.size.y;
                }

                dest.setPosSize(dest.pos, dest.size);
                if (container.children[1]->pos.x < container.children[0]->pos.x ||
                    container.children[1]->pos.y < container.children[0]->pos.y)
                {
                    Dock* tmp = container.children[0];
                    container.children[0] = container.children[1];
                    container.children[1] = tmp;
                }
            }

            void doDock(Dock& dock, Dock* dest, DockSlot dock_slot, ImVec2 const &default_size = ImVec2(0.0f, 0.0f))
            {
                IM_ASSERT(!dock.parent);
                if (!dest)
                {
                    dock.status = Status_Docked;
                    dock.setPosSize(m_workspace_pos, m_workspace_size);
                }
                else if (dock_slot == DockSlot::Tab)
                {
                    Dock* tmp = dest;
                    while (tmp->next_tab)
                    {
                        tmp = tmp->next_tab;
                    };

                    tmp->next_tab = &dock;
                    dock.prev_tab = tmp;
                    dock.size = tmp->size;
                    dock.pos = tmp->pos;
                    dock.parent = dest->parent;
                    dock.status = Status_Docked;
                }
                else if (dock_slot == DockSlot::None)
                {
                    dock.status = Status_Float;
                }
                else
                {
                    Dock* container = (Dock*)ImGui::MemAlloc(sizeof(Dock));
                    IM_PLACEMENT_NEW(container) Dock(0);
                    m_docks.push_back(container);
                    container->children[0] = &dest->getFirstTab();
                    container->children[1] = &dock;
                    container->next_tab = nullptr;
                    container->prev_tab = nullptr;
                    container->parent = dest->parent;
                    container->size = dest->size;
                    container->pos = dest->pos;
                    container->status = Status_Docked;
                    container->label = ImStrdup("");
                    if (!dest->parent)
                    {
                    }
                    else if (&dest->getFirstTab() == dest->parent->children[0])
                    {
                        dest->parent->children[0] = container;
                    }
                    else
                    {
                        dest->parent->children[1] = container;
                    }

                    dest->setParent(container);
                    dock.parent = container;
                    dock.status = Status_Docked;
                    setDockPosSize(*dest, dock, dock_slot, *container, default_size);
                }

                dock.setActive();
            }

            void rootDock(const ImVec2& pos, const ImVec2& size)
            {
                Dock* root = getRootDock();
                if (!root) return;

                ImVec2 min_size = root->getMinSize();
                ImVec2 requested_size = size;
                root->setPosSize(pos, ImMax(min_size, requested_size));
            }

            void setDockActive()
            {
                IM_ASSERT(m_current);
                if (m_current) m_current->setActive();
            }

            static DockSlot getSlotFromLocationCode(char code)
            {
                switch (code)
                {
                case '1':
                    return DockSlot::Left;

                case '2':
                    return DockSlot::Top;

                case '3':
                    return DockSlot::Bottom;

                default:
                    return DockSlot::Right;
                };
            }

            static char getLocationCode(Dock* dock)
            {
                if (!dock) return '0';

                if (dock->parent->isHorizontal())
                {
                    if (dock->pos.x < dock->parent->children[0]->pos.x) return '1';
                    if (dock->pos.x < dock->parent->children[1]->pos.x) return '1';
                    return '0';
                }
                else
                {
                    if (dock->pos.y < dock->parent->children[0]->pos.y) return '2';
                    if (dock->pos.y < dock->parent->children[1]->pos.y) return '2';
                    return '3';
                }
            }

            void tryDockToStoredLocation(Dock& dock)
            {
                if (dock.status == Status_Docked) return;
                if (dock.location[0] == 0) return;

                Dock* tmp = getRootDock();
                if (!tmp) return;

                Dock* prev = nullptr;
                char* c = dock.location + strlen(dock.location) - 1;
                while (c >= dock.location && tmp)
                {
                    prev = tmp;
                    tmp = *c == getLocationCode(tmp->children[0]) ? tmp->children[0] : tmp->children[1];
                    if (tmp) --c;
                };

                if (tmp && tmp->children[0]) tmp = tmp->parent;
                doDock(dock, tmp ? tmp : prev, tmp && !tmp->children[0] ? DockSlot::Tab : getSlotFromLocationCode(*c));
            }

            bool begin(char const *label, bool* opened, ImGuiWindowFlags extra_flags, const ImVec2& default_size)
            {
                DockSlot next_slot = m_next_dock_slot;
                m_next_dock_slot = DockSlot::Tab;
                Dock& dock = getDock(label, !opened || *opened, default_size, extra_flags);
                if (!dock.opened && (!opened || *opened)) tryDockToStoredLocation(dock);
                dock.last_frame = ImGui::GetFrameCount();
                if (strcmp(dock.label, label) != 0)
                {
                    ImGui::MemFree(dock.label);
                    dock.label = ImStrdup(label);
                }

                m_end_action = EndAction_None;

                bool prev_opened = dock.opened;
                bool first = dock.first;
                if (dock.first && opened) *opened = dock.opened;
                dock.first = false;
                if (opened && !*opened)
                {
                    if (dock.status != Status_Float)
                    {
                        fillLocation(dock);
                        doUndock(dock);
                        dock.status = Status_Float;
                    }
                    dock.opened = false;
                    return false;
                }

                dock.opened = true;

                checkNonexistent();

                if (first || (prev_opened != dock.opened))
                {
                    Dock* root = (m_next_parent ? m_next_parent : getRootDock());
                    if (root && (&dock != root) && !dock.parent)
                    {
                        doDock(dock, root, next_slot, default_size);
                    }

                    m_next_parent = &dock;
                }

                m_current = &dock;
                if (dock.status == Status_Dragged) handleDrag(dock);

                bool is_float = dock.status == Status_Float;
                if (is_float)
                {
                    ImGui::SetNextWindowPos(dock.pos);
                    ImGui::SetNextWindowSize(dock.size);
                    bool ret = ImGui::Begin(label,
                        opened,
                        dock.size,
                        -1.0f,
                        ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_ShowBorders | extra_flags);
                    m_end_action = EndAction_End;
                    dock.pos = ImGui::GetWindowPos();
                    dock.size = ImGui::GetWindowSize();

                    ImGuiContext& g = *GImGui;
                    if (g.ActiveId == ImGui::GetCurrentWindow()->MoveId && g.IO.MouseDown[0])
                    {
                        m_drag_offset = ImGui::GetMousePos() - dock.pos;
                        doUndock(dock);
                        dock.status = Status_Dragged;
                    }

                    return ret;
                }

                if (!dock.active && dock.status != Status_Dragged) return false;

                //beginPanel();

                m_end_action = EndAction_EndChild;

                splits();

                PushStyleColor(ImGuiCol_Border, ImVec4(0, 0, 0, 0));
                float tabbar_height = 0.0f;
                if (!(extra_flags & ImGuiWindowFlags_NoTitleBar))
                {
                    tabbar_height = ImGui::GetTextLineHeightWithSpacing();
                    if (tabbar(dock.getFirstTab(), opened != nullptr))
                    {
                        fillLocation(dock);
                        *opened = false;
                    }
                }

                ImVec2 pos = dock.pos;
                ImVec2 size = dock.size;
                pos.y += tabbar_height + ImGui::GetStyle().WindowPadding.y;
                size.y -= tabbar_height + ImGui::GetStyle().WindowPadding.y;

                ImGui::SetCursorScreenPos(pos);
                ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                    ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse |
                    ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBringToFrontOnFocus |
                    extra_flags;
                bool ret = ImGui::BeginChild(label, size, true, flags);
                ImGui::PopStyleColor();

                return ret;
            }

            void end()
            {
                m_current = nullptr;
                if (m_end_action != EndAction_None)
                {
                    if (m_end_action == EndAction_End)
                    {
                        ImGui::End();
                    }
                    else if (m_end_action == EndAction_EndChild)
                    {
                        PushStyleColor(ImGuiCol_Border, ImVec4(0, 0, 0, 0));
                        ImGui::EndChild();
                        ImGui::PopStyleColor();
                    }

                    //endPanel();
                }
            }

            int getDockIndex(Dock* dock)
            {
                if (!dock) return -1;

                for (int i = 0; i < m_docks.size(); ++i)
                {
                    if (dock == m_docks[i]) return i;
                }

                IM_ASSERT(false);
                return -1;
            }
        };

        static ::std::unordered_map<const char*, DockContext> g_dockSpaces;
        static DockContext* g_dock = nullptr;

        void ShutdownDock(void)
        {
            for (auto &g_dock : g_dockSpaces)
            {
                for (auto &dock : g_dock.second.m_docks)
                {
                    dock->~Dock();
                    ImGui::MemFree(dock);
                }
                
                g_dock.second.m_docks.clear();
            }
            
            g_dockSpaces.clear();
        }

        void SetNextDock(DockSlot slot)
        {
            g_dock->m_next_dock_slot = slot;
        }

        void BeginDockspace(char const *label, ImVec2 const &workspace, bool showBorder, ImVec2 const &splitSize)
        {
            g_dock = &g_dockSpaces[label];
            ImGuiWindowFlags flags = ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoScrollbar;
            ImGui::BeginChild(label ? label : "##workspace", workspace, showBorder, flags);
            g_dock->m_workspace_pos = ImGui::GetWindowPos();
            g_dock->m_workspace_size = ImGui::GetWindowSize();
            g_dock->m_splitSize = splitSize;
        }

        void EndDockspace(void)
        {
            ImGui::EndChild();
            g_dock = nullptr;
        }

        void SetDockActive(void)
        {
            g_dock->setDockActive();
        }

        bool BeginDock(char const *label, bool *opened, ImGuiWindowFlags extra_flags, ImVec2 const &default_size)
        {
            return g_dock->begin(label, opened, extra_flags, default_size);
        }

        void EndDock(void)
        {
            g_dock->end();
        }
    }; // namespace UI
}; // namespace Gek