#include "GEK/Utility/Exceptions.hpp"
#include "GEK/Utility/FileSystem.hpp"
#include "GEK/Utility/String.hpp"
#include "GEK/Utility/Context.hpp"
#include "GEK/Utility/ContextUser.hpp"
#include "GEK/Render/Window.hpp"
#include "GEK/Render/Device.hpp"
#include "GEK/GUI/Utilities.hpp"
#include <concurrent_unordered_map.h>
#include <Windows.h>

using namespace Gek;

namespace Gek
{
    class Core
    {
    private:
        JSON::Object configuration;

        ContextPtr context;
        WindowPtr window;
        Render::DevicePtr renderDevice;

        bool engineRunning = false;
        bool windowActive = false;

        int currentDisplayMode = 0;
        int previousDisplayMode = 0;
        Render::DisplayModeList displayModeList;
        std::vector<StringUTF8> displayModeStringList;
        bool fullScreen = false;

        struct GUI
        {
            ImGui::PanelManager panelManager;

            Render::PipelineStateHandle pipelineState;
            Render::SamplerStateHandle samplerState;
            Render::RenderListHandle renderList;

            Render::ResourceHandle indirectBuffer;
            Render::ResourceHandle constantBuffer;
            Render::ResourceHandle vertexBuffer;
            Render::ResourceHandle indexBuffer;

            Render::ResourceHandle fontTexture;
            Render::ResourceHandle consoleButton;
            Render::ResourceHandle performanceButton;
            Render::ResourceHandle settingsButton;
        };

        std::unique_ptr<GUI> gui = std::make_unique<GUI>();

        bool showCursor = false;
        bool showModeChange = false;
        float modeChangeTimer = 0.0f;
        bool consoleActive = false;

    protected:
        Context * const getContext(void) const
        {
            return context.get();
        }

    public:
        Core(void)
        {
            auto pluginPath(FileSystem::GetModuleFilePath().getParentPath());
            auto rootPath(pluginPath.getParentPath());

            std::vector<FileSystem::Path> searchPathList;
            searchPathList.push_back(pluginPath);

            context = Context::Create(rootPath, searchPathList);

            try
            {
                configuration = JSON::Load(getContext()->getRootFileName(L"config.json"));
            }
            catch (const std::exception &)
            {
            };

            if (!configuration.is_object())
            {
                configuration = JSON::Object();
            }

            if (!configuration.has_member(L"display") || !configuration.get(L"display").has_member(L"mode"))
            {
                configuration[L"display"][L"mode"] = 0;
            }

            previousDisplayMode = currentDisplayMode = configuration[L"display"][L"mode"].as_uint();

            Window::Description windowDescription;
            windowDescription.className = L"GEK_Engine_Demo";
            windowDescription.windowName = L"GEK Engine Demo";
            window = getContext()->createClass<Window>(L"Default::Render::Window", windowDescription);

            window->onClose.connect<Core, &Core::onClose>(this);
            window->onActivate.connect<Core, &Core::onActivate>(this);
            window->onSizeChanged.connect<Core, &Core::onSizeChanged>(this);
            window->onKeyPressed.connect<Core, &Core::onKeyPressed>(this);
            window->onCharacter.connect<Core, &Core::onCharacter>(this);
            window->onSetCursor.connect<Core, &Core::onSetCursor>(this);
            window->onMouseClicked.connect<Core, &Core::onMouseClicked>(this);
            window->onMouseWheel.connect<Core, &Core::onMouseWheel>(this);
            window->onMousePosition.connect<Core, &Core::onMousePosition>(this);
            window->onMouseMovement.connect<Core, &Core::onMouseMovement>(this);

            HRESULT resultValue = CoInitialize(nullptr);
            if (FAILED(resultValue))
            {
                throw std::exception("Failed call to CoInitialize");
            }

            Render::Device::Description deviceDescription;
            renderDevice = getContext()->createClass<Render::Device>(L"Default::Render::Device", window.get(), deviceDescription);
            displayModeList = renderDevice->getDisplayModeList(deviceDescription.displayFormat);
            for (auto &displayMode : displayModeList)
            {
                StringUTF8 displayModeString(StringUTF8::Format("%vx%v, %vhz", displayMode.width, displayMode.height, uint32_t(std::ceil(float(displayMode.refreshRate.numerator) / float(displayMode.refreshRate.denominator)))));
                switch (displayMode.aspectRatio)
                {
                case Render::DisplayMode::AspectRatio::_4x3:
                    displayModeString.append(" (4x3)");
                    break;

                case Render::DisplayMode::AspectRatio::_16x9:
                    displayModeString.append(" (16x9)");
                    break;

                case Render::DisplayMode::AspectRatio::_16x10:
                    displayModeString.append(" (16x10)");
                    break;
                };

                displayModeStringList.push_back(displayModeString);
            }

            String baseFileName(getContext()->getRootFileName(L"data", L"gui"));
            gui->consoleButton = renderDevice->loadTexture(FileSystem::GetFileName(baseFileName, L"console.png"), 0);
            gui->performanceButton = renderDevice->loadTexture(FileSystem::GetFileName(baseFileName, L"performance.png"), 0);
            gui->settingsButton = renderDevice->loadTexture(FileSystem::GetFileName(baseFileName, L"settings.png"), 0);

            auto consolePane = gui->panelManager.addPane(ImGui::PanelManager::BOTTOM, "ConsolePanel##ConsolePanel");
            if (consolePane)
            {
                consolePane->previewOnHover = false;
                consolePane->addButtonAndWindow(
                    ImGui::Toolbutton("Settings", &gui->settingsButton, ImVec2(0, 0), ImVec2(1, 1), ImVec2(32, 32)),
                    ImGui::PanelManagerPaneAssociatedWindow("Settings", -1, [](ImGui::PanelManagerWindowData &windowData) -> void
                {
                    ((Core *)windowData.userData)->drawSettings(windowData);
                }, this, ImGuiWindowFlags_NoScrollbar));
            }

            ImGuiIO &imGuiIo = ImGui::GetIO();
            imGuiIo.Fonts->AddFontDefault();
            imGuiIo.Fonts->Build();

            imGuiIo.KeyMap[ImGuiKey_Tab] = VK_TAB;
            imGuiIo.KeyMap[ImGuiKey_LeftArrow] = VK_LEFT;
            imGuiIo.KeyMap[ImGuiKey_RightArrow] = VK_RIGHT;
            imGuiIo.KeyMap[ImGuiKey_UpArrow] = VK_UP;
            imGuiIo.KeyMap[ImGuiKey_DownArrow] = VK_DOWN;
            imGuiIo.KeyMap[ImGuiKey_PageUp] = VK_PRIOR;
            imGuiIo.KeyMap[ImGuiKey_PageDown] = VK_NEXT;
            imGuiIo.KeyMap[ImGuiKey_Home] = VK_HOME;
            imGuiIo.KeyMap[ImGuiKey_End] = VK_END;
            imGuiIo.KeyMap[ImGuiKey_Delete] = VK_DELETE;
            imGuiIo.KeyMap[ImGuiKey_Backspace] = VK_BACK;
            imGuiIo.KeyMap[ImGuiKey_Enter] = VK_RETURN;
            imGuiIo.KeyMap[ImGuiKey_Escape] = VK_ESCAPE;
            imGuiIo.KeyMap[ImGuiKey_A] = 'A';
            imGuiIo.KeyMap[ImGuiKey_C] = 'C';
            imGuiIo.KeyMap[ImGuiKey_V] = 'V';
            imGuiIo.KeyMap[ImGuiKey_X] = 'X';
            imGuiIo.KeyMap[ImGuiKey_Y] = 'Y';
            imGuiIo.KeyMap[ImGuiKey_Z] = 'Z';

            ImGuiStyle& style = ImGui::GetStyle();
            //ImGui::SetupImGuiStyle(false, 0.9f);
            ImGui::ResetStyle(ImGuiStyle_OSX, style);
            style.WindowTitleAlign = ImVec2(0.5f, 0.5f);
            style.ButtonTextAlign = ImVec2(0.5f, 0.5f);
            style.WindowRounding = 0.0f;
            style.FrameRounding = 3.0f;

            static wchar_t const * const vertexShader =
                L"cbuffer vertexBuffer : register(b0)" \
                L"{" \
                L"    float4x4 ProjectionMatrix;" \
                L"};" \
                L"" \
                L"struct VertexInput" \
                L"{" \
                L"    float2 position : POSITION;" \
                L"    float4 color : COLOR0;" \
                L"    float2 texCoord  : TEXCOORD0;" \
                L"};" \
                L"" \
                L"struct PixelOutput" \
                L"{" \
                L"    float4 position : SV_POSITION;" \
                L"    float4 color : COLOR0;" \
                L"    float2 texCoord  : TEXCOORD0;" \
                L"};" \
                L"" \
                L"PixelOutput main(in VertexInput input)" \
                L"{" \
                L"    PixelOutput output;" \
                L"    output.position = mul(ProjectionMatrix, float4(input.position.xy, 0.0f, 1.0f));" \
                L"    output.color = input.color;" \
                L"    output.texCoord  = input.texCoord;" \
                L"    return output;" \
                L"}";

            static wchar_t const * const pixelShader =
                L"struct PixelInput" \
                L"{" \
                L"    float4 position : SV_POSITION;" \
                L"    float4 color : COLOR0;" \
                L"    float2 texCoord  : TEXCOORD0;" \
                L"};" \
                L"" \
                L"sampler pointSampler;" \
                L"Texture2D<float4> uiTexture : register(t0);" \
                L"" \
                L"float4 main(PixelInput input) : SV_Target" \
                L"{" \
                L"    return (input.color * uiTexture.Sample(pointSampler, input.texCoord));" \
                L"}";

            Render::PipelineStateInformation pipelineStateInformation;
            pipelineStateInformation.compiledVertexShader = renderDevice->compileProgram(Render::PipelineType::Vertex, L"uiVertexProgram", L"main", vertexShader);
            pipelineStateInformation.compiledPixelShader = renderDevice->compileProgram(Render::PipelineType::Pixel, L"uiPixelProgram", L"main", pixelShader);

            Render::InputElement element;
            element.format = Render::Format::R32G32_FLOAT;
            element.semantic = Render::InputElement::Semantic::Position;
            pipelineStateInformation.inputElementList.push_back(element);

            element.format = Render::Format::R32G32_FLOAT;
            element.semantic = Render::InputElement::Semantic::TexCoord;
            pipelineStateInformation.inputElementList.push_back(element);

            element.format = Render::Format::R8G8B8A8_UNORM;
            element.semantic = Render::InputElement::Semantic::Color;
            pipelineStateInformation.inputElementList.push_back(element);

            pipelineStateInformation.blendStateInformation.unifiedBlendState = true;
            pipelineStateInformation.blendStateInformation.targetStateList[0].enable = true;
            pipelineStateInformation.blendStateInformation.targetStateList[0].colorSource = Render::BlendStateInformation::Source::SourceAlpha;
            pipelineStateInformation.blendStateInformation.targetStateList[0].colorDestination = Render::BlendStateInformation::Source::InverseSourceAlpha;
            pipelineStateInformation.blendStateInformation.targetStateList[0].colorOperation = Render::BlendStateInformation::Operation::Add;
            pipelineStateInformation.blendStateInformation.targetStateList[0].alphaSource = Render::BlendStateInformation::Source::InverseSourceAlpha;
            pipelineStateInformation.blendStateInformation.targetStateList[0].alphaDestination = Render::BlendStateInformation::Source::Zero;
            pipelineStateInformation.blendStateInformation.targetStateList[0].alphaOperation = Render::BlendStateInformation::Operation::Add;

            pipelineStateInformation.rasterizerStateInformation.fillMode = Render::RasterizerStateInformation::FillMode::Solid;
            pipelineStateInformation.rasterizerStateInformation.cullMode = Render::RasterizerStateInformation::CullMode::None;
            pipelineStateInformation.rasterizerStateInformation.scissorEnable = true;
            pipelineStateInformation.rasterizerStateInformation.depthClipEnable = true;

            pipelineStateInformation.depthStateInformation.enable = true;
            pipelineStateInformation.depthStateInformation.comparisonFunction = Render::ComparisonFunction::LessEqual;
            pipelineStateInformation.depthStateInformation.writeMask = Render::DepthStateInformation::Write::Zero;

            gui->pipelineState = renderDevice->createPipelineState(pipelineStateInformation);

            Render::BufferDescription indirectBufferDescription;
            indirectBufferDescription.stride = (sizeof(uint32_t) * 5);
            indirectBufferDescription.count = 1;
            indirectBufferDescription.type = Render::BufferDescription::Type::IndirectArguments;
            gui->indirectBuffer = renderDevice->createBuffer(indirectBufferDescription);

            Render::BufferDescription constantBufferDescription;
            constantBufferDescription.stride = sizeof(Math::Float4x4);
            constantBufferDescription.count = 1;
            constantBufferDescription.type = Render::BufferDescription::Type::Constant;
            gui->constantBuffer = renderDevice->createBuffer(constantBufferDescription);

            uint8_t *pixels = nullptr;
            int32_t fontWidth = 0, fontHeight = 0;
            imGuiIo.Fonts->GetTexDataAsRGBA32(&pixels, &fontWidth, &fontHeight);

            Render::TextureDescription fontDescription;
            fontDescription.format = Render::Format::R8G8B8A8_UNORM;
            fontDescription.width = fontWidth;
            fontDescription.height = fontHeight;
            fontDescription.flags = Render::TextureDescription::Flags::Resource;
            gui->fontTexture = renderDevice->createTexture(fontDescription, pixels);
            imGuiIo.Fonts->TexID = &gui->fontTexture;

            Render::SamplerStateInformation samplerStateInformation;
            samplerStateInformation.filterMode = Render::SamplerStateInformation::FilterMode::MinificationMagnificationMipMapPoint;
            samplerStateInformation.addressModeU = Render::SamplerStateInformation::AddressMode::Wrap;
            samplerStateInformation.addressModeV = Render::SamplerStateInformation::AddressMode::Wrap;
            samplerStateInformation.addressModeW = Render::SamplerStateInformation::AddressMode::Wrap;
            gui->samplerState = renderDevice->createSamplerState(samplerStateInformation);

            auto renderQueue = renderDevice->createRenderQueue();
            renderQueue->bindPipelineState(gui->pipelineState);
            renderQueue->bindVertexBufferList({ gui->vertexBuffer }, 0);
            renderQueue->bindIndexBuffer(gui->indexBuffer, 0);
            renderQueue->bindConstantBufferList({ gui->constantBuffer }, 0, Render::PipelineType::Vertex);
            renderQueue->bindSamplerStateList({ gui->samplerState }, 0, Render::PipelineType::Pixel);
            renderQueue->drawInstancedIndexedPrimitive(gui->indirectBuffer);
            gui->renderList = renderDevice->createRenderList(renderQueue.get());

            imGuiIo.UserData = this;
            imGuiIo.RenderDrawListsFn = [](ImDrawData *drawData)
            {
                ImGuiIO &imGuiIo = ImGui::GetIO();
                Core *core = static_cast<Core *>(imGuiIo.UserData);
                core->renderDrawData(drawData);
            };

            setDisplayMode(currentDisplayMode);
            window->setVisibility(true);

            engineRunning = true;
            windowActive = true;
        }

        ~Core(void)
        {
            renderDevice = nullptr;
            window = nullptr;
            context = nullptr;
        }

        void renderDrawData(ImDrawData *drawData)
        {
            if (!gui->vertexBuffer || gui->vertexBuffer->getDescription().count < uint32_t(drawData->TotalVtxCount))
            {
                Render::BufferDescription vertexBufferDescription;
                vertexBufferDescription.stride = sizeof(ImDrawVert);
                vertexBufferDescription.count = drawData->TotalVtxCount;
                vertexBufferDescription.type = Render::BufferDescription::Type::Vertex;
                vertexBufferDescription.flags = Render::BufferDescription::Flags::Mappable;
                gui->vertexBuffer = renderDevice->createBuffer(vertexBufferDescription);
            }

            if (!gui->indexBuffer || gui->indexBuffer->getDescription().count < uint32_t(drawData->TotalIdxCount))
            {
                Render::BufferDescription vertexBufferDescription;
                vertexBufferDescription.count = drawData->TotalIdxCount;
                vertexBufferDescription.type = Render::BufferDescription::Type::Index;
                vertexBufferDescription.flags = Render::BufferDescription::Flags::Mappable;
                switch (sizeof(ImDrawIdx))
                {
                case 2:
                    vertexBufferDescription.format = Render::Format::R16_UINT;
                    break;

                case 4:
                    vertexBufferDescription.format = Render::Format::R32_UINT;
                    break;

                default:
                    throw std::exception("Index buffer can only be 16bit or 32bit");
                };

                gui->indexBuffer = renderDevice->createBuffer(vertexBufferDescription);
            }

            bool dataUploaded = false;
            ImDrawVert* vertexData = nullptr;
            ImDrawIdx* indexData = nullptr;
            if (renderDevice->mapResource(gui->vertexBuffer, vertexData))
            {
                if (renderDevice->mapResource(gui->indexBuffer, indexData))
                {
                    for (int32_t commandListIndex = 0; commandListIndex < drawData->CmdListsCount; ++commandListIndex)
                    {
                        const ImDrawList* commandList = drawData->CmdLists[commandListIndex];
                        std::copy(commandList->VtxBuffer.Data, (commandList->VtxBuffer.Data + commandList->VtxBuffer.Size), vertexData);
                        std::copy(commandList->IdxBuffer.Data, (commandList->IdxBuffer.Data + commandList->IdxBuffer.Size), indexData);
                        vertexData += commandList->VtxBuffer.Size;
                        indexData += commandList->IdxBuffer.Size;
                    }

                    dataUploaded = true;
                    renderDevice->unmapResource(gui->indexBuffer);
                }

                renderDevice->unmapResource(gui->vertexBuffer);
            }

            if (dataUploaded)
            {
                auto backBuffer = renderDevice->getBackBuffer();
                uint32_t width = backBuffer->getDescription().width;
                uint32_t height = backBuffer->getDescription().height;
                auto orthographic = Math::Float4x4::MakeOrthographic(0.0f, 0.0f, float(width), float(height), 0.0f, 1.0f);
                renderDevice->updateResource(gui->constantBuffer, &orthographic);

                auto videoContext = renderDevice->getDefaultContext();
                resources->setBackBuffer(videoContext, nullptr);

                uint32_t vertexOffset = 0;
                uint32_t indexOffset = 0;
                for (int32_t commandListIndex = 0; commandListIndex < drawData->CmdListsCount; ++commandListIndex)
                {
                    const ImDrawList* commandList = drawData->CmdLists[commandListIndex];
                    for (int32_t commandIndex = 0; commandIndex < commandList->CmdBuffer.Size; ++commandIndex)
                    {
                        const ImDrawCmd* command = &commandList->CmdBuffer[commandIndex];
                        if (command->UserCallback)
                        {
                            command->UserCallback(commandList, command);
                        }
                        else
                        {
                            std::vector<Math::UInt4> scissorBoxList(1);
                            scissorBoxList[0].minimum.x = uint32_t(command->ClipRect.x);
                            scissorBoxList[0].minimum.y = uint32_t(command->ClipRect.y);
                            scissorBoxList[0].maximum.x = uint32_t(command->ClipRect.z);
                            scissorBoxList[0].maximum.y = uint32_t(command->ClipRect.w);
                            videoContext->setScissorList(scissorBoxList);

                            std::vector<Render::ResourceHandle> textureList(1);
                            textureList[0] = *(Render::ResourceHandle *)command->TextureId;
                            videoContext->pixelPipeline()->setResourceList(textureList, 0);

                            renderDevice->executeRenderList(gui->renderList);
                        }

                        indexOffset += command->ElemCount;
                    }

                    vertexOffset += commandList->VtxBuffer.Size;
                }
            }
        }

        void drawSettings(ImGui::PanelManagerWindowData &windowData)
        {
            if (ImGui::Checkbox("FullScreen", &fullScreen))
            {
                if (fullScreen)
                {
                    window->move(Math::Int2::Zero);
                }

                renderDevice->setFullScreenState(fullScreen);
                if (!fullScreen)
                {
                    window->move();
                }
            }

            ImGui::PushItemWidth(350.0f);
            if (ImGui::Gek::ListBox("Display Mode", &currentDisplayMode, [](void *data, int index, const char **text) -> bool
            {
                Core *core = static_cast<Core *>(data);
                auto &mode = core->displayModeStringList[index];
                (*text) = mode.c_str();
                return true;
            }, this, displayModeStringList.size(), 5))
            {
                configuration[L"display"][L"mode"] = currentDisplayMode;
                setDisplayMode(currentDisplayMode);
                showModeChange = true;
                modeChangeTimer = 10.0f;
            }

            ImGui::PopItemWidth();
        }

        void setDisplayMode(uint32_t displayMode)
        {
            auto &displayModeData = displayModeList[displayMode];
            if (displayMode >= displayModeList.size())
            {
                throw std::exception("Invalid display mode encountered");
            }

            currentDisplayMode = displayMode;
            renderDevice->setDisplayMode(displayModeData);
            window->move();
        }

        bool update(void)
        {
            window->readEvents();
            return engineRunning;
        }

        // Window slots
        void onClose(void)
        {
            engineRunning = false;
        }

        void onActivate(bool isActive)
        {
            windowActive = isActive;
        }

        void onSetCursor(bool &showCursor)
        {
            showCursor = false;
        }

        void onSizeChanged(bool isMinimized)
        {
            if (renderDevice && !isMinimized)
            {
                renderDevice->handleResize();
            }
        }

        void onCharacter(wchar_t character)
        {
            //ImGuiIO &imGuiIo = ImGui::GetIO();
            //imGuiIo.AddInputCharacter(character);
        }

        void onKeyPressed(uint16_t key, bool state)
        {
            //ImGuiIO &imGuiIo = ImGui::GetIO();
            //imGuiIo.KeysDown[key] = state;
        }

        void onMouseClicked(Window::Button button, bool state)
        {
            /*
            ImGuiIO &imGuiIo = ImGui::GetIO();
            switch (button)
            {
            case Window::Button::Left:
                imGuiIo.MouseDown[0] = state;
                break;

            case Window::Button::Middle:
                imGuiIo.MouseDown[2] = state;
                break;

            case Window::Button::Right:
                imGuiIo.MouseDown[1] = state;
                break;
            };
            */
        }

        void onMouseWheel(int32_t offset)
        {
            //ImGuiIO &imGuiIo = ImGui::GetIO();
            //imGuiIo.MouseWheel += (offset > 0 ? +1.0f : -1.0f);
        }

        void onMousePosition(int32_t xPosition, int32_t yPosition)
        {
            //ImGuiIO &imGuiIo = ImGui::GetIO();
            //imGuiIo.MousePos.x = xPosition;
            //imGuiIo.MousePos.y = yPosition;
        }

        void onMouseMovement(int32_t xMovement, int32_t yMovement)
        {
        }
    };
}; // namespace Gek
int CALLBACK wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ wchar_t *strCommandLine, _In_ int nCmdShow)
{
    try
    {
        Gek::Core core;
        while (core.update())
        {
        };
    }
    catch (const std::exception &exception)
    {
        MessageBoxA(nullptr, StringUTF8::Format("Caught: %v\r\nType: %v", exception.what(), typeid(exception).name()), "GEK Engine - Error", MB_OK | MB_ICONERROR);
    }
    catch (...)
    {
        MessageBox(nullptr, L"Caught: Non-standard exception", L"GEK Engine - Error", MB_OK | MB_ICONERROR);
    };

    return 0;
}