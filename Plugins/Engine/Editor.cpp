#include "GEK/Math/Common.hpp"
#include "GEK/Math/Quaternion.hpp"
#include "GEK/Utility/String.hpp"
#include "GEK/Utility/FileSystem.hpp"
#include "GEK/Utility/JSON.hpp"
#include "GEK/Utility/ContextUser.hpp"
#include "GEK/Engine/Core.hpp"
#include "GEK/Engine/Renderer.hpp"
#include "GEK/Engine/Population.hpp"
#include "GEK/Engine/Entity.hpp"
#include "GEK/Engine/Component.hpp"
#include "GEK/Engine/ComponentMixin.hpp"
#include "GEK/Engine/Editor.hpp"
#include "GEK/Components/Transform.hpp"
#include <concurrent_vector.h>
#include <ppl.h>

namespace Gek
{
    namespace Implementation
    {
        GEK_CONTEXT_USER(Editor, Plugin::Core *)
            , public Plugin::Processor
            , public Plugin::Editor
        {
        private:
            Plugin::Core *core = nullptr;
            Edit::Population *population = nullptr;
            Plugin::Renderer *renderer = nullptr;

            float headingAngle = 0.0f;
            float lookingAngle = 0.0f;
            Math::Float3 position = Math::Float3::Zero;
            bool moveForward = false;
            bool moveBackward = false;
            bool strafeLeft = false;
            bool strafeRight = false;

        public:
            Editor(Context *context, Plugin::Core *core)
                : ContextRegistration(context)
                , core(core)
                , population(dynamic_cast<Edit::Population *>(core->getPopulation()))
                , renderer(core->getRenderer())
            {
                assert(population);
                assert(core);

                core->setOption("editor", "active", false);

                population->onAction.connect<Editor, &Editor::onAction>(this);
                population->onUpdate[90].connect<Editor, &Editor::onUpdate>(this);

                renderer->onShowUserInterface.connect<Editor, &Editor::onShowUserInterface>(this);
            }

            ~Editor(void)
            {
                renderer->onShowUserInterface.disconnect<Editor, &Editor::onShowUserInterface>(this);

                population->onUpdate[90].disconnect<Editor, &Editor::onUpdate>(this);
                population->onAction.disconnect<Editor, &Editor::onAction>(this);
            }

            // Renderer
            void showEntities(void)
            {
                //ImGui::SetNextDock(ImGuiDockSlot_Left);
                if (ImGui::BeginDock("Entities"))
                {
                }

                ImGui::EndDock();
            }

            void showComponents(void)
            {
                //ImGui::SetNextDock(ImGuiDockSlot_Right);
                if (ImGui::BeginDock("Components"))
                {
                }

                ImGui::EndDock();
            }

            void showTools(void)
            {
                //ImGui::SetNextDock(ImGuiDockSlot_Top);
                if (ImGui::BeginDock("Tools"))
                {
                }

                ImGui::EndDock();
            }

            void showResources(void)
            {
                //ImGui::SetNextDock(ImGuiDockSlot_Bottom);
                if (ImGui::BeginDock("Resources"))
                {
                }

                ImGui::EndDock();
            }

            void showScene(void)
            {
                //ImGui::SetNextDock(ImGuiDockSlot_None);
                if (ImGui::BeginDock("Scene"))
                {
                }

                ImGui::EndDock();
            }

            void onShowUserInterface(ImGuiContext * const guiContext)
            {
                bool editorActive = core->getOption("editor", "active").convert(false);
                if (!editorActive)
                {
                    return;
                }

                ImGuiIO &imGuiIo = ImGui::GetIO();
                auto displayPosition = ImVec2(0.0f, 0.0f);
                auto displaySize = imGuiIo.DisplaySize;
                if (imGuiIo.MouseDrawCursor)
                {
                    displayPosition.y = ImGui::GetTextLineHeightWithSpacing();
                }

                ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
                ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
                ImGui::SetNextWindowPos(displayPosition);
                if (ImGui::Begin("Editor", nullptr, displaySize, 0.0f, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_AlwaysAutoResize))
                {
                    ImGui::BeginDockspace();

                    showEntities();
                    showTools();
                    showScene();
                    showResources();
                    showComponents();

                    ImGui::EndDockspace();
                }

                ImGui::End();
                ImGui::PopStyleVar(2);
            }

            // Plugin::Population Slots
            void onAction(Plugin::Population::Action const &action)
            {
                bool editorActive = core->getOption("editor", "active").convert(false);
                if (!editorActive)
                {
                    return;
                }

                if (action.name == "turn")
                {
                    headingAngle += (action.value * 0.01f);
                }
                else if (action.name == "tilt")
                {
                    lookingAngle += (action.value * 0.01f);
                    lookingAngle = Math::Clamp(lookingAngle, -Math::Pi * 0.5f, Math::Pi * 0.5f);
                }
                else if (action.name == "move_forward")
                {
                    moveForward = action.state;
                }
                else if (action.name == "move_backward")
                {
                    moveBackward = action.state;
                }
                else if (action.name == "strafe_left")
                {
                    strafeLeft = action.state;
                }
                else if (action.name == "strafe_right")
                {
                    strafeRight = action.state;
                }
            }

            void onUpdate(float frameTime)
            {
                bool editorActive = core->getOption("editor", "active").convert(false);
                if (editorActive)
                {
                    Math::Float4x4 viewMatrix(Math::Float4x4::FromPitch(lookingAngle) * Math::Float4x4::FromYaw(headingAngle));
                    position += (viewMatrix.rz.xyz * (((moveForward ? 1.0f : 0.0f) + (moveBackward ? -1.0f : 0.0f)) * 5.0f) * frameTime);
                    position += (viewMatrix.rx.xyz * (((strafeLeft ? -1.0f : 0.0f) + (strafeRight ? 1.0f : 0.0f)) * 5.0f) * frameTime);
                    viewMatrix.translation.xyz = position;
                    viewMatrix.invert();

                    const auto backBuffer = core->getVideoDevice()->getBackBuffer();
                    const float width = float(backBuffer->getDescription().width);
                    const float height = float(backBuffer->getDescription().height);
                    auto projectionMatrix(Math::Float4x4::MakePerspective(Math::DegreesToRadians(90.0f), (width / height), 0.1f, 200.0f));

                    renderer->queueCamera(viewMatrix, projectionMatrix, 0.5f, 200.0f, ResourceHandle());
                }
            }
        };

        GEK_REGISTER_CONTEXT_USER(Editor);
    }; // namespace Implementation
}; // namespace Gek
