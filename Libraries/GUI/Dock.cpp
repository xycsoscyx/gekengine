#include "GEK/GUI/Dock.hpp"
#include <imgui_internal.h>

namespace Gek
{
    namespace UI
    {
        namespace Dock
        {
            struct Context
            {
                enum class EndAction
                {
                    None,
                    Panel,
                    End,
                    EndChild
                };

                enum class Status
                {
                    Docked,
                    Float,
                    Dragged
                };

                struct Tab
                {
                    Tab(ImGuiWindowFlags extraFlags)
                        : extraFlags(extraFlags)
                    {
                    }

                    ImVec2 getMinimumSize() const
                    {
                        if (!children[0])
                        {
                            return ImVec2(16, 16 + ImGui::GetTextLineHeightWithSpacing());
                        }

                        ImVec2 s0 = children[0]->getMinimumSize();
                        ImVec2 s1 = children[1]->getMinimumSize();
                        return isHorizontal() ?
                            ImVec2(s0.x + s1.x, ImMax(s0.y, s1.y)) :
                            ImVec2(ImMax(s0.x, s1.x), s0.y + s1.y);
                    }

                    bool isHorizontal(void) const
                    {
                        return children[0]->position.x < children[1]->position.x;
                    }

                    void setParent(Tab *tab)
                    {
                        parent = tab;
                        for (Tab *temporary = previousTab; temporary; temporary = temporary->previousTab)
                        {
                            temporary->parent = tab;
                        }

                        for (Tab *temporary = nextTab; temporary; temporary = temporary->nextTab)
                        {
                            temporary->parent = tab;
                        }
                    }

                    Tab &getRoot(void)
                    {
                        Tab *tab = this;
                        while (tab->parent)
                        {
                            tab = tab->parent;
                        };

                        return *tab;
                    }

                    Tab &getSibling(void)
                    {
                        IM_ASSERT(parent);
                        if (parent->children[0] == &getFirstTab())
                        {
                            return *parent->children[1];
                        }

                        return *parent->children[0];
                    }


                    Tab &getFirstTab(void)
                    {
                        Tab *temporary = this;
                        while (temporary->previousTab)
                        {
                            temporary = temporary->previousTab;
                        };

                        return *temporary;
                    }

                    void setActive(void)
                    {
                        active = true;
                        for (Tab *temporary = previousTab; temporary; temporary = temporary->previousTab)
                        {
                            temporary->active = false;
                        }

                        for (Tab *temporary = nextTab; temporary; temporary = temporary->nextTab)
                        {
                            temporary->active = false;
                        }
                    }

                    bool isContainer(void) const
                    {
                        return children[0] != nullptr;
                    }

                    void setChildrenPositionAndSize(ImVec2 const &_pos, ImVec2 const &_size)
                    {
                        ImVec2 containerSize = children[0]->size;
                        if (isHorizontal())
                        {
                            containerSize.y = _size.y;
                            containerSize.x = (float)int(_size.x * children[0]->size.x / (children[0]->size.x + children[1]->size.x));
                            if (containerSize.x < children[0]->getMinimumSize().x)
                            {
                                containerSize.x = children[0]->getMinimumSize().x;
                            }
                            else if (_size.x - containerSize.x < children[1]->getMinimumSize().x)
                            {
                                containerSize.x = _size.x - children[1]->getMinimumSize().x;
                            }

                            children[0]->setPositionAndSize(_pos, containerSize);
                            containerSize.x = _size.x - children[0]->size.x;

                            ImVec2 containerPosition = _pos;
                            containerPosition.x += children[0]->size.x;
                            children[1]->setPositionAndSize(containerPosition, containerSize);
                        }
                        else
                        {
                            containerSize.x = _size.x;
                            containerSize.y = (float)int(_size.y * children[0]->size.y / (children[0]->size.y + children[1]->size.y));
                            if (containerSize.y < children[0]->getMinimumSize().y)
                            {
                                containerSize.y = children[0]->getMinimumSize().y;
                            }
                            else if (_size.y - containerSize.y < children[1]->getMinimumSize().y)
                            {
                                containerSize.y = _size.y - children[1]->getMinimumSize().y;
                            }

                            children[0]->setPositionAndSize(_pos, containerSize);
                            containerSize.y = _size.y - children[0]->size.y;

                            ImVec2 containerPosition = _pos;
                            containerPosition.y += children[0]->size.y;
                            children[1]->setPositionAndSize(containerPosition, containerSize);
                        }
                    }

                    void setPositionAndSize(ImVec2 const &_pos, ImVec2 const &_size)
                    {
                        size = _size;
                        position = _pos;
                        for (Tab *temporary = previousTab; temporary; temporary = temporary->previousTab)
                        {
                            temporary->size = _size;
                            temporary->position = _pos;
                        }

                        for (Tab *temporary = nextTab; temporary; temporary = temporary->nextTab)
                        {
                            temporary->size = _size;
                            temporary->position = _pos;
                        }

                        if (isContainer())
                        {
                            setChildrenPositionAndSize(_pos, _size);
                        }
                    }

                    std::string label;
                    ImU32 identifier = 0;
                    Tab *nextTab = nullptr;
                    Tab *previousTab = nullptr;
                    Tab *children[2] = { nullptr, nullptr };
                    Tab *parent = nullptr;
                    bool active = true;
                    ImVec2 position = ImVec2(0.0f, 0.0f);
                    ImVec2 size = ImVec2(-1.0f, -1.0f);
                    Status status = Status::Float;
                    int lastFrame = 0;
                    int invalidFrames = 0;
                    char location[16] = { 0 };
                    bool opened = false;
                    bool first = false;
                    ImGuiWindowFlags extraFlags = 0;
                };

                ImVector<Tab *> tabList;
                ImVec2 dragOffset = ImVec2(0.0f, 0.0f);
                Tab *currentTab = nullptr;
                Tab *nextParentTab = nullptr;
                int lastFrame = 0;
                EndAction endAction;
                ImVec2 position;
                ImVec2 size;
                Location nextTabLocation = Location::Tab;
                ImVec2 splitterSize = ImVec2(3.0f, 3.0f);

                ~Context(void)
                {
                    for (int index = 0; index < tabList.size(); ++index)
                    {
                        tabList[index]->~Tab();
                        ImGui::MemFree(tabList[index]);
                    }

                    tabList.clear();
                }

                Tab &getTab(std::string_view label, bool opened, ImVec2 const &defaultSize, ImGuiWindowFlags extraFlags)
                {
                    ImU32 identifier = ImHash(label.data(), 0);
                    for (int index = 0; index < tabList.size(); ++index)
                    {
                        if (tabList[index]->identifier == identifier)
                        {
                            return *tabList[index];
                        }
                    }

                    Tab* newTab = (Tab *)ImGui::MemAlloc(sizeof(Tab));
                    IM_PLACEMENT_NEW(newTab) Tab(extraFlags);
                    tabList.push_back(newTab);
                    newTab->label = label;
                    newTab->identifier = identifier;
                    newTab->setActive();
                    newTab->status = ((tabList.size() == 1) ? Status::Docked : Status::Float);
                    newTab->position = ImVec2(0, 0);
                    newTab->size.x = ((defaultSize.x < 0) ? ImGui::GetIO().DisplaySize.x : defaultSize.x);
                    newTab->size.y = ((defaultSize.y < 0) ? ImGui::GetIO().DisplaySize.y : defaultSize.y);
                    newTab->opened = opened;
                    newTab->first = true;
                    newTab->lastFrame = 0;
                    newTab->invalidFrames = 0;
                    newTab->location[0] = 0;
                    return *newTab;
                }

                void putInBackground(void)
                {
                    ImGuiWindow* currentWindow = ImGui::GetCurrentWindow();
                    ImGuiContext& currentContext = *ImGui::GetCurrentContext();
                    if (currentContext.Windows[0] == currentWindow)
                    {
                        return;
                    }

                    for (int index = 0; index < currentContext.Windows.Size; index++)
                    {
                        if (currentContext.Windows[index] == currentWindow)
                        {
                            for (int searchIndex = index - 1; searchIndex >= 0; --searchIndex)
                            {
                                currentContext.Windows[searchIndex + 1] = currentContext.Windows[searchIndex];
                            }

                            currentContext.Windows[0] = currentWindow;
                            break;
                        }
                    }
                }

                void drawSplits(void)
                {
                    if (ImGui::GetFrameCount() == lastFrame)
                    {
                        return;
                    }

                    lastFrame = ImGui::GetFrameCount();

                    putInBackground();
                    for (int index = 0; index < tabList.size(); ++index)
                    {
                        Tab &tab = *tabList[index];
                        if (!tab.parent && (tab.status == Status::Docked))
                        {
                            tab.setPositionAndSize(position, size);
                        }
                    }

                    ImU32 color = ImGui::GetColorU32(ImGuiCol_Button);
                    ImU32 colorHovered = ImGui::GetColorU32(ImGuiCol_ButtonHovered);
                    ImDrawList *drawList = ImGui::GetWindowDrawList();
                    ImGuiIO& io = ImGui::GetIO();
                    for (int index = 0; index < tabList.size(); ++index)
                    {
                        Tab &tab = *tabList[index];
                        if (!tab.isContainer())
                        {
                            continue;
                        }

                        ImGui::PushID(index);
                        if (!ImGui::IsMouseDown(0))
                        {
                            tab.status = Status::Docked;
                        }

                        ImVec2 position0 = tab.children[0]->position;
                        ImVec2 position1 = tab.children[1]->position;
                        ImVec2 size0 = tab.children[0]->size;
                        ImVec2 size1 = tab.children[1]->size;

                        ImGuiMouseCursor cursor;

                        ImVec2 tabSize(0, 0);
                        ImVec2 minimumSize0 = tab.children[0]->getMinimumSize();
                        ImVec2 minimumSize1 = tab.children[1]->getMinimumSize();
                        if (tab.isHorizontal())
                        {
                            cursor = ImGuiMouseCursor_ResizeEW;
                            ImGui::SetCursorScreenPos(ImVec2(tab.position.x + size0.x - splitterSize.x * 0.5f + 1.0f, tab.position.y));
                            ImGui::InvisibleButton("split", ImVec2(splitterSize.x, tab.size.y));
                            if (tab.status == Status::Dragged)
                            {
                                tabSize.x = io.MouseDelta.x;
                            }

                            tabSize.x = -ImMin(-tabSize.x, tab.children[0]->size.x - minimumSize0.x);
                            tabSize.x = ImMin(tabSize.x, tab.children[1]->size.x - minimumSize1.x);
                            size0 += tabSize;
                            size1 -= tabSize;
                            position0 = tab.position;
                            position1.x = position0.x + size0.x;
                            position1.y = tab.position.y;
                            size0.y = size1.y = tab.size.y;
                            size1.x = ImMax(minimumSize1.x, tab.size.x - size0.x);
                            size0.x = ImMax(minimumSize0.x, tab.size.x - size1.x);
                        }
                        else
                        {
                            cursor = ImGuiMouseCursor_ResizeNS;
                            ImGui::SetCursorScreenPos(ImVec2(tab.position.x, tab.position.y + size0.y - splitterSize.y * 0.5f + 1.0f));
                            ImGui::InvisibleButton("split", ImVec2(tab.size.x, splitterSize.y));
                            if (tab.status == Status::Dragged)
                            {
                                tabSize.y = io.MouseDelta.y;
                            }

                            tabSize.y = -ImMin(-tabSize.y, tab.children[0]->size.y - minimumSize0.y);
                            tabSize.y = ImMin(tabSize.y, tab.children[1]->size.y - minimumSize1.y);
                            size0 += tabSize;
                            size1 -= tabSize;
                            position0 = tab.position;
                            position1.x = tab.position.x;
                            position1.y = position0.y + size0.y;
                            size0.x = size1.x = tab.size.x;
                            size1.y = ImMax(minimumSize1.y, tab.size.y - size0.y);
                            size0.y = ImMax(minimumSize0.y, tab.size.y - size1.y);
                        }

                        tab.children[0]->setPositionAndSize(position0, size0);
                        tab.children[1]->setPositionAndSize(position1, size1);
                        if (ImGui::IsItemHovered())
                        {
                            ImGui::SetMouseCursor(cursor);
                        }

                        if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(0))
                        {
                            if (!(tab.extraFlags & ImGuiWindowFlags_NoMove))
                            {
                                tab.status = Status::Dragged;
                            }
                        }

                        drawList->AddRectFilled(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImGui::IsItemHovered() ? colorHovered : color);
                        ImGui::PopID();
                    }
                }

                void checkNonExistent(void)
                {
                    int frame_limit = ImMax(0, ImGui::GetFrameCount() - 2);
                    for (int index = 0; index < tabList.size(); ++index)
                    {
                        Tab &tab = *tabList[index];
                        if (tab.isContainer())
                        {
                            continue;
                        }

                        if (tab.status == Status::Float)
                        {
                            continue;
                        }

                        if (tab.lastFrame < frame_limit)
                        {
                            ++tab.invalidFrames;
                            if (tab.invalidFrames > 2)
                            {
                                undockTab(tab);
                                tab.status = Status::Float;
                            }

                            return;
                        }

                        tab.invalidFrames = 0;
                    }
                }

                Tab *getTabAtPosition(ImVec2 const &/*position*/) const
                {
                    for (int index = 0; index < tabList.size(); ++index)
                    {
                        Tab &tab = *tabList[index];
                        if (tab.isContainer())
                        {
                            continue;
                        }

                        if (tab.status != Status::Docked)
                        {
                            continue;
                        }

                        if (ImGui::IsMouseHoveringRect(tab.position, tab.position + tab.size, false))
                        {
                            return &tab;
                        }
                    }

                    return nullptr;
                }

                static ImRect getDockedRectangle(ImRect const &rectangle, Location location)
                {
                    ImVec2 half_size = (rectangle.GetSize() * 0.5f);
                    switch (location)
                    {
                    default:
                        return rectangle;

                    case Location::Top:
                        return ImRect(rectangle.Min, rectangle.Min + ImVec2(rectangle.Max.x - rectangle.Min.x, half_size.y)); //  @r-lyeh

                    case Location::Right:
                        return ImRect(rectangle.Min + ImVec2(half_size.x, 0), rectangle.Max);

                    case Location::Bottom:
                        return ImRect(rectangle.Min + ImVec2(0, half_size.y), rectangle.Max);

                    case Location::Left:
                        return ImRect(rectangle.Min, ImVec2(rectangle.Min.x + half_size.x, rectangle.Max.y));
                    };
                }

                static ImRect getTabRectangle(ImRect parentRectangle, Location location)
                {
                    ImVec2 size = parentRectangle.Max - parentRectangle.Min;
                    ImVec2 center = parentRectangle.Min + size * 0.5f;
                    switch (location)
                    {
                    default:
                        return ImRect(center - ImVec2(20, 20), center + ImVec2(20, 20));

                    case Location::Top:
                        return ImRect(center + ImVec2(-20, -50), center + ImVec2(20, -30));

                    case Location::Right:
                        return ImRect(center + ImVec2(30, -20), center + ImVec2(50, 20));

                    case Location::Bottom:
                        return ImRect(center + ImVec2(-20, +30), center + ImVec2(20, 50));

                    case Location::Left:
                        return ImRect(center + ImVec2(-50, -20), center + ImVec2(-30, 20));
                    };
                }

                static ImRect getBorderRectangle(ImRect parentRectangle, Location location)
                {
                    ImVec2 size = parentRectangle.Max - parentRectangle.Min;
                    ImVec2 center = parentRectangle.Min + size * 0.5f;
                    switch (location)
                    {
                    case Location::Top:
                        return ImRect(
                            ImVec2(center.x - 20, parentRectangle.Min.y + 10),
                            ImVec2(center.x + 20, parentRectangle.Min.y + 30));

                    case Location::Left:
                        return ImRect(
                            ImVec2(parentRectangle.Min.x + 10, center.y - 20),
                            ImVec2(parentRectangle.Min.x + 30, center.y + 20));

                    case Location::Bottom:
                        return ImRect(
                            ImVec2(center.x - 20, parentRectangle.Max.y - 30),
                            ImVec2(center.x + 20, parentRectangle.Max.y - 10));

                    case Location::Right:
                        return ImRect(
                            ImVec2(parentRectangle.Max.x - 30, center.y - 20),
                            ImVec2(parentRectangle.Max.x - 10, center.y + 20));

                    default:
                        IM_ASSERT(false);
                    };

                    IM_ASSERT(false);
                    return ImRect();
                }

                Tab *getRoot()
                {
                    for (auto &tab : tabList)
                    {
                        if (!tab->parent && (tab->status == Status::Docked || tab->children[0]))
                        {
                            return tab;
                        }
                    }

                    return nullptr;
                }

                bool calculatePositions(Tab &tab, Tab *destinationDock, ImRect const &rectangle, bool isOnBorder)
                {
                    ImDrawList *canvas = ImGui::GetWindowDrawList();
                    ImU32 color = ImGui::GetColorU32(ImGuiCol_Button);		    // Color of all the available "spots"
                    ImU32 colorHovered = ImGui::GetColorU32(ImGuiCol_ButtonHovered);  // Color of the hovered "spot"
                    ImU32 dockedRectangleColor = color;
                    ImVec2 mousePosition = ImGui::GetIO().MousePos;
                    ImTextureID texture = nullptr;
                    if (gImGuiDockReuseTabWindowTextureIfAvailable)
                    {
#	ifdef IMGUITABWINDOW_H_
                        texture = ImGui::TabWindow::DockPanelIconTextureID;	// Nope. It doesn't look OK.
                        if (texture)
                        {
                            color = 0x00FFFFFF | 0x90000000;
                            colorHovered = (colorHovered & 0x00FFFFFF) | 0x90000000;
                            dockedRectangleColor = (dockedRectangleColor & 0x00FFFFFF) | 0x80000000;
                            canvas->ChannelsSplit(2);	// Solves overlay order. But won't it break something else ?
                        }
#	endif ////IMGUITABWINDOW_H_
                    }

                    for (int index = 0; index < (isOnBorder ? 4 : 5); ++index)
                    {
                        const Location location = static_cast<Location>(index);
                        ImRect rectangle = (isOnBorder ? getBorderRectangle(rectangle, location) : getTabRectangle(rectangle, location));
                        bool isHovered = rectangle.Contains(mousePosition);
                        ImU32 finalColor = isHovered ? colorHovered : color;
                        if (!texture)
                        {
                            canvas->AddRectFilled(rectangle.Min, rectangle.Max, finalColor);
                        }
                        else
                        {
#		ifdef IMGUITABWINDOW_H_
                            canvas->ChannelsSetCurrent(0);	// Background
                            switch (location)
                            {
                            case Location::Left:
                            case Location::Right:
                            case Location::Top:
                            case Location::Bottom:
                                if (true)
                                {
                                    const int uvIndex = (index == 0) ? 3 : (index == 2) ? 0 : (index == 3) ? 2 : index;
                                    ImVec2 uv0(0.75f, (float)uvIndex*0.25f), uv1(uv0.x + 0.25f, uv0.y + 0.25f);
                                    canvas->AddImage(texture, rectangle.Min, rectangle.Max, uv0, uv1, finalColor);
                                    break;
                                }

                            case Location::Tab:
                                canvas->AddImage(texture, rectangle.Min, rectangle.Max, ImVec2(0.22916f, 0.22916f), ImVec2(0.45834f, 0.45834f), finalColor);
                                break;

                            default:
                                canvas->AddRectFilled(rectangle.Min, rectangle.Max, finalColor);
                                break;
                            };

                            canvas->ChannelsSetCurrent(1);	// Foreground
#		endif ////IMGUITABWINDOW_H_
                        }

                        if (!isHovered)
                        {
                            continue;
                        }

                        if (!ImGui::IsMouseDown(0))
                        {
#		ifdef IMGUITABWINDOW_H_
                            if (texture)
                            {
                                canvas->ChannelsMerge();
                            }
#		endif ////IMGUITABWINDOW_H_

                            handleTab(tab, (destinationDock ? destinationDock : getRoot()), location);
                            return true;
                        }

                        ImRect dockedRectangle = getDockedRectangle(rectangle, location);
                        canvas->AddRectFilled(dockedRectangle.Min, dockedRectangle.Max, dockedRectangleColor);
                    }

#	ifdef IMGUITABWINDOW_H_
                    if (texture)
                    {
                        canvas->ChannelsMerge();
                    }
#	endif ////IMGUITABWINDOW_H_

                    return false;
                }

                void handleDrag(Tab &tab)
                {
                    Tab *destinationDock = getTabAtPosition(ImGui::GetIO().MousePos);

                    ImGui::Begin("##draggingOverlay", nullptr,
                        ImGuiWindowFlags_Tooltip | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove |
                        ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings |
                        ImGuiWindowFlags_AlwaysAutoResize);

                    ImDrawList *canvas = ImGui::GetWindowDrawList();

                    canvas->PushClipRectFullScreen();

                    ImU32 dockedColor = ImGui::GetColorU32(ImGuiCol_FrameBg);
                    dockedColor = (dockedColor & 0x00ffFFFF) | 0x80000000;
                    tab.position = ImGui::GetIO().MousePos - dragOffset;
                    if (destinationDock)
                    {
                        if (calculatePositions(tab, destinationDock, ImRect(destinationDock->position, destinationDock->position + destinationDock->size), false))
                        {
                            canvas->PopClipRect();
                            ImGui::End();
                            return;
                        }
                    }

                    if (calculatePositions(tab, nullptr, ImRect(position, position + size), true))
                    {
                        canvas->PopClipRect();
                        ImGui::End();
                        return;
                    }

                    canvas->AddRectFilled(tab.position, tab.position + tab.size, dockedColor);
                    canvas->PopClipRect();

                    if (!ImGui::IsMouseDown(0))
                    {
                        tab.status = Status::Float;
                        tab.location[0] = 0;
                        tab.setActive();
                    }

                    ImGui::End();
                }

                void fillLocation(Tab &tab)
                {
                    if (tab.status == Status::Float)
                    {
                        return;
                    }

                    char* location = tab.location;
                    Tab *temporary = &tab;
                    while (temporary->parent)
                    {
                        *location = getCodeFromTab(temporary);
                        temporary = temporary->parent;
                        ++location;
                    };

                    *location = 0;
                }

                void undockTab(Tab &tab)
                {
                    if (tab.previousTab)
                    {
                        tab.previousTab->setActive();
                    }
                    else if (tab.nextTab)
                    {
                        tab.nextTab->setActive();
                    }
                    else
                    {
                        tab.active = false;
                    }

                    Tab *container = tab.parent;
                    if (container)
                    {
                        Tab &sibling = tab.getSibling();
                        if (container->children[0] == &tab)
                        {
                            container->children[0] = tab.nextTab;
                        }
                        else if (container->children[1] == &tab)
                        {
                            container->children[1] = tab.nextTab;
                        }

                        bool removeContainer = !container->children[0] || !container->children[1];
                        if (removeContainer)
                        {
                            if (container->parent)
                            {
                                Tab*& child = (container->parent->children[0] == container) ? container->parent->children[0] : container->parent->children[1];
                                child = &sibling;
                                child->setPositionAndSize(container->position, container->size);
                                child->setParent(container->parent);
                            }
                            else
                            {
                                if (container->children[0])
                                {
                                    container->children[0]->setParent(nullptr);
                                    container->children[0]->setPositionAndSize(container->position, container->size);
                                }

                                if (container->children[1])
                                {
                                    container->children[1]->setParent(nullptr);
                                    container->children[1]->setPositionAndSize(container->position, container->size);
                                }
                            }

                            for (int index = 0; index < tabList.size(); ++index)
                            {
                                if (tabList[index] == container)
                                {
                                    tabList.erase(tabList.begin() + index);
                                    break;
                                }
                            }

                            if (container == nextParentTab)
                            {
                                nextParentTab = nullptr;
                            }

                            container->~Tab();
                            ImGui::MemFree(container);
                        }
                    }

                    if (tab.previousTab)
                    {
                        tab.previousTab->nextTab = tab.nextTab;
                    }

                    if (tab.nextTab)
                    {
                        tab.nextTab->previousTab = tab.previousTab;
                    }

                    tab.parent = nullptr;
                    tab.previousTab = tab.nextTab = nullptr;
                }

                void drawTabList(Tab &tab)
                {
                    if (!tab.nextTab)
                    {
                        return;
                    }

                    ImDrawList *drawList = ImGui::GetWindowDrawList();
                    if (ImGui::InvisibleButton("list", ImVec2(16, 16)))
                    {
                        ImGui::OpenPopup("tab_list_popup");
                    }

                    if (ImGui::BeginPopup("tab_list_popup"))
                    {
                        Tab *temporary = &tab;
                        while (temporary)
                        {
                            bool dummy = false;
                            if (ImGui::Selectable(temporary->label.data(), &dummy))
                            {
                                temporary->setActive();
                                nextParentTab = temporary;
                            }

                            temporary = temporary->nextTab;
                        }

                        ImGui::EndPopup();
                    }

                    bool isHovered = ImGui::IsItemHovered();
                    ImVec2 minimum = ImGui::GetItemRectMin();
                    ImVec2 maximum = ImGui::GetItemRectMax();
                    ImVec2 center = (minimum + maximum) * 0.5f;
                    ImU32 colorText = ImGui::GetColorU32(ImGuiCol_Text);
                    ImU32 colorActive = ImGui::GetColorU32(ImGuiCol_FrameBgActive);
                    drawList->AddRectFilled(
                        ImVec2(center.x - 4, minimum.y + 3),
                        ImVec2(center.x + 4, minimum.y + 5),
                        isHovered ? colorActive : colorText);
                    drawList->AddTriangleFilled(
                        ImVec2(center.x - 4, minimum.y + 7),
                        ImVec2(center.x + 4, minimum.y + 7),
                        ImVec2(center.x, minimum.y + 12),
                        isHovered ? colorActive : colorText);
                }

                bool drawTabs(Tab &tab, bool useCloseButton)
                {
                    float tabBarHeight = 2 * ImGui::GetTextLineHeightWithSpacing();
                    ImVec2 size(tab.size.x, tabBarHeight);
                    bool isTabClosed = false;

                    ImGui::SetCursorScreenPos(tab.position);
                    if (ImGui::BeginChild(String::Format("tabs_{}", tab.identifier).data(), size, true))
                    {
                        Tab *dockedTab = &tab;
                        ImDrawList *drawList = ImGui::GetWindowDrawList();
                        ImU32 color = ImGui::GetColorU32(ImGuiCol_FrameBg);
                        ImU32 colorActive = ImGui::GetColorU32(ImGuiCol_FrameBgActive);
                        ImU32 colorHovered = ImGui::GetColorU32(ImGuiCol_FrameBgHovered);
                        ImU32 colorText = ImGui::GetColorU32(ImGuiCol_Text);
                        float lineHeight = ImGui::GetTextLineHeightWithSpacing();
                        float tabBase;

                        drawTabList(tab);
                        while (dockedTab)
                        {
                            ImGui::SameLine(0, 15);
                            char const *endOfLabel = &dockedTab->label.back() + 1;
                            ImVec2 size(ImGui::CalcTextSize(dockedTab->label.data(), endOfLabel).x, lineHeight);
                            if (ImGui::InvisibleButton(dockedTab->label.data(), size))
                            {
                                dockedTab->setActive();
                                nextParentTab = dockedTab;
                            }

                            if (ImGui::IsItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Left))
                            {
                                if (!(dockedTab->extraFlags & ImGuiWindowFlags_NoMove))
                                {
                                    dragOffset = ImGui::GetMousePos() - dockedTab->position;
                                    undockTab(*dockedTab);
                                    dockedTab->status = Status::Dragged;
                                }
                            }

                            bool isHovered = ImGui::IsItemHovered();
                            ImVec2 position = ImGui::GetItemRectMin();
                            if (dockedTab->active && useCloseButton)
                            {
                                size.x += 16 + ImGui::GetStyle().ItemSpacing.x;
                                ImGui::SameLine();
                                isTabClosed = ImGui::InvisibleButton("close", ImVec2(16, 16));
                                ImVec2 center = (ImGui::GetItemRectMin() + ImGui::GetItemRectMax()) * 0.5f;
                                drawList->AddLine(center + ImVec2(-3.5f, -3.5f), center + ImVec2(3.5f, 3.5f), colorText);
                                drawList->AddLine(center + ImVec2(3.5f, -3.5f), center + ImVec2(-3.5f, 3.5f), colorText);
                            }

                            tabBase = position.y;
                            drawList->PathClear();
                            drawList->PathLineTo(position + ImVec2(-15, size.y));
                            drawList->PathBezierCurveTo(position + ImVec2(-10, size.y), position + ImVec2(-5, 0), position + ImVec2(0, 0), 10);
                            drawList->PathLineTo(position + ImVec2(size.x, 0));
                            drawList->PathBezierCurveTo(position + ImVec2(size.x + 5, 0), position + ImVec2(size.x + 10, size.y), position + ImVec2(size.x + 15, size.y), 10);
                            drawList->PathFillConvex( isHovered ? colorHovered : (dockedTab->active ? colorActive : color));
                            drawList->AddText(position + ImVec2(0, 1), colorText, dockedTab->label.data(), endOfLabel);
                            dockedTab = dockedTab->nextTab;
                        }

                        ImVec2 dividerLine(tab.position.x, tabBase + lineHeight);
                        drawList->AddLine(dividerLine, dividerLine + ImVec2(tab.size.x, 0), color);
                    }

                    ImGui::EndChild();
                    return isTabClosed;
                }

                static void setTabPositionAndSize(Tab &destination, Tab &tab, Location location, Tab &container, ImVec2 const &defaultSize)
                {
                    IM_ASSERT(!tab.previousTab && !tab.nextTab && !tab.children[0] && !tab.children[1]);

                    destination.position = container.position;
                    tab.position = container.position;
                    destination.size = container.size;
                    tab.size.x = defaultSize.x <= 0.0f ? container.size.x * 0.5f : defaultSize.x;
                    tab.size.y = defaultSize.y <= 0.0f ? container.size.y * 0.5f : defaultSize.y;
                    if (location == Location::Bottom)
                    {
                        destination.size.y -= tab.size.y;
                        tab.position.y += destination.size.y;
                    }
                    else if (location == Location::Right)
                    {
                        destination.size.x -= tab.size.x;
                        tab.position.x += destination.size.x;
                    }
                    else if (location == Location::Left)
                    {
                        destination.size.x -= tab.size.x;
                        destination.position.x += tab.size.x;
                    }
                    else if (location == Location::Top)
                    {
                        destination.size.y -= tab.size.y;
                        destination.position.y += tab.size.y;
                    }

                    destination.setPositionAndSize(destination.position, destination.size);
                    if (container.children[1]->position.x < container.children[0]->position.x ||
                        container.children[1]->position.y < container.children[0]->position.y)
                    {
                        Tab *temporary = container.children[0];
                        container.children[0] = container.children[1];
                        container.children[1] = temporary;
                    }
                }

                void handleTab(Tab &tab, Tab *destination, Location location, ImVec2 const &defaultSize = ImVec2(0.0f, 0.0f))
                {
                    IM_ASSERT(!tab.parent);
                    if (!destination)
                    {
                        tab.status = Status::Docked;
                        tab.setPositionAndSize(position, size);
                    }
                    else if (location == Location::Tab)
                    {
                        Tab *temporary = destination;
                        while (temporary->nextTab)
                        {
                            temporary = temporary->nextTab;
                        };

                        temporary->nextTab = &tab;
                        tab.previousTab = temporary;
                        tab.size = temporary->size;
                        tab.position = temporary->position;
                        tab.parent = destination->parent;
                        tab.status = Status::Docked;
                    }
                    else if (location == Location::None)
                    {
                        tab.status = Status::Float;
                    }
                    else
                    {
                        Tab* container = (Tab *)ImGui::MemAlloc(sizeof(Tab));
                        IM_PLACEMENT_NEW(container) Tab(0);
                        tabList.push_back(container);

                        container->children[0] = &destination->getFirstTab();
                        container->children[1] = &tab;
                        container->nextTab = nullptr;
                        container->previousTab = nullptr;
                        container->parent = destination->parent;
                        container->size = destination->size;
                        container->position = destination->position;
                        container->status = Status::Docked;
                        container->label = ImStrdup("");
                        if (!destination->parent)
                        {
                        }
                        else if (&destination->getFirstTab() == destination->parent->children[0])
                        {
                            destination->parent->children[0] = container;
                        }
                        else
                        {
                            destination->parent->children[1] = container;
                        }

                        destination->setParent(container);
                        tab.parent = container;
                        tab.status = Status::Docked;
                        setTabPositionAndSize(*destination, tab, location, *container, defaultSize);
                    }

                    tab.setActive();
                }

                void setTabActive(void)
                {
                    IM_ASSERT(currentTab);
                    if (currentTab)
                    {
                        currentTab->setActive();
                    }
                }

                static Location getLocationFromCode(char code)
                {
                    switch (code)
                    {
                    case '1':
                        return Location::Left;

                    case '2':
                        return Location::Top;

                    case '3':
                        return Location::Bottom;

                    default:
                        return Location::Right;
                    };
                }

                static char getCodeFromTab(Tab *tab)
                {
                    if (!tab)
                    {
                        return '0';
                    }

                    if (tab->parent->isHorizontal())
                    {
                        if (tab->position.x < tab->parent->children[0]->position.x)
                        {
                            return '1';
                        }

                        if (tab->position.x < tab->parent->children[1]->position.x)
                        {
                            return '1';
                        }

                        return '0';
                    }
                    else
                    {
                        if (tab->position.y < tab->parent->children[0]->position.y)
                        {
                            return '2';
                        }

                        if (tab->position.y < tab->parent->children[1]->position.y)
                        {
                            return '2';
                        }

                        return '3';
                    }
                }

                void tryDockingTab(Tab &tab)
                {
                    if (tab.status == Status::Docked)
                    {
                        return;
                    }

                    if (tab.location[0] == 0)
                    {
                        return;
                    }

                    Tab *temporary = getRoot();
                    if (!temporary)
                    {
                        return;
                    }

                    Tab *previous = nullptr;
                    char* location = tab.location + strlen(tab.location) - 1;
                    while (location >= tab.location && temporary)
                    {
                        previous = temporary;
                        temporary = *location == getCodeFromTab(temporary->children[0]) ? temporary->children[0] : temporary->children[1];
                        if (temporary) --location;
                    };

                    if (temporary && temporary->children[0])
                    {
                        temporary = temporary->parent;
                    }

                    handleTab(tab, temporary ? temporary : previous, temporary && !temporary->children[0] ? Location::Tab : getLocationFromCode(*location));
                }

                bool begin(std::string_view label, bool* opened, ImGuiWindowFlags extraFlags, ImVec2 const &defaultSize)
                {
                    Location nextLocation = nextTabLocation;
                    nextTabLocation = Location::Tab;

                    Tab &tab = getTab(label, !opened || *opened, defaultSize, extraFlags);
                    if (!tab.opened && (!opened || *opened))
                    {
                        tryDockingTab(tab);
                    }

                    tab.lastFrame = ImGui::GetFrameCount();
                    if (tab.label != label)
                    {
                        tab.label = label;
                    }

                    endAction = EndAction::None;

                    bool isPreviousOpened = tab.opened;
                    bool first = tab.first;
                    if (tab.first && opened)
                    {
                        *opened = tab.opened;
                    }

                    tab.first = false;
                    if (opened && !*opened)
                    {
                        if (tab.status != Status::Float)
                        {
                            fillLocation(tab);
                            undockTab(tab);
                            tab.status = Status::Float;
                        }

                        tab.opened = false;
                        return false;
                    }

                    tab.opened = true;
                    checkNonExistent();
                    if (first || (isPreviousOpened != tab.opened))
                    {
                        Tab *root = (nextParentTab ? nextParentTab : getRoot());
                        if (root && (&tab != root) && !tab.parent)
                        {
                            handleTab(tab, root, nextLocation, defaultSize);
                        }

                        nextParentTab = &tab;
                    }

                    currentTab = &tab;
                    if (tab.status == Status::Dragged)
                    {
                        handleDrag(tab);
                    }

                    bool isFloating = tab.status == Status::Float;
                    if (isFloating)
                    {
                        ImGui::SetNextWindowPos(tab.position);
                        ImGui::SetNextWindowSize(tab.size);
						ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
                        bool ret = ImGui::Begin(label.data(), opened, ImGuiWindowFlags_NoCollapse | extraFlags);
                        endAction = EndAction::End;
                        tab.position = ImGui::GetWindowPos();
                        tab.size = ImGui::GetWindowSize();

                        ImGuiContext& currentContext = *ImGui::GetCurrentContext();
                        if (currentContext.ActiveId == ImGui::GetCurrentWindow()->MoveId && currentContext.IO.MouseDown[0])
                        {
                            dragOffset = ImGui::GetMousePos() - tab.position;
                            undockTab(tab);
                            tab.status = Status::Dragged;
                        }

                        return ret;
                    }

                    if (!tab.active && tab.status != Status::Dragged)
                    {
                        return false;
                    }

                    endAction = EndAction::EndChild;

                    drawSplits();

                    PushStyleColor(ImGuiCol_Border, ImVec4(0, 0, 0, 0));
                    float tabBarHeight = 0.0f;
                    if (!(extraFlags & ImGuiWindowFlags_NoTitleBar))
                    {
                        tabBarHeight = ImGui::GetTextLineHeightWithSpacing();
                        if (drawTabs(tab.getFirstTab(), opened != nullptr))
                        {
                            fillLocation(tab);
                            *opened = false;
                        }
                    }

                    ImVec2 position = tab.position;
                    ImVec2 size = tab.size;
                    position.y += tabBarHeight + ImGui::GetStyle().WindowPadding.y;
                    size.y -= tabBarHeight + ImGui::GetStyle().WindowPadding.y;

                    ImGui::SetCursorScreenPos(position);
                    ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse |
                        ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBringToFrontOnFocus |
                        extraFlags;
                    bool isOpened = ImGui::BeginChild(label.data(), size, true, flags);
                    ImGui::PopStyleColor();
                    return isOpened;
                }

                void end(void)
                {
                    currentTab = nullptr;
                    if (endAction != EndAction::None)
                    {
                        if (endAction == EndAction::End)
                        {
							ImGui::PopStyleVar();
                            ImGui::End();
                        }
                        else if (endAction == EndAction::EndChild)
                        {
                            PushStyleColor(ImGuiCol_Border, ImVec4(0, 0, 0, 0));
                            ImGui::EndChild();
                            ImGui::PopStyleColor();
                        }
                    }
                }
            };

            WorkSpace::WorkSpace(void)
                : context((Context *)ImGui::MemAlloc(sizeof(Context)))
            {
                IM_PLACEMENT_NEW(context) Context();
            }

            WorkSpace::~WorkSpace(void)
            {
                ImGui::MemFree(context);
            }

            void WorkSpace::Begin(std::string_view label, ImVec2 const &workspace, bool showBorder, ImVec2 const &splitSize)
            {
                ImGuiWindowFlags flags = ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoScrollbar;
                ImGui::BeginChild(label.data(), workspace, showBorder, flags);
                context->position = ImGui::GetWindowPos();
                context->size = ImGui::GetWindowSize();
                context->splitterSize = splitSize;
            }

            void WorkSpace::End(void)
            {
                ImGui::EndChild();
            }

            void WorkSpace::SetActive(void)
            {
                context->setTabActive();
            }

            void WorkSpace::SetNextLocation(Location location)
            {
                context->nextTabLocation = location;
            }

            bool WorkSpace::BeginTab(std::string_view label, bool *opened, ImGuiWindowFlags extraFlags, ImVec2 const &defaultSize)
            {
                return context->begin(label, opened, extraFlags, defaultSize);
            }

            void WorkSpace::EndTab(void)
            {
                context->end();
            }
        }; // namespace Dock
    }; // namespace UI
}; // namespace Gek