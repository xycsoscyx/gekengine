#include "GEK/Math/Matrix4x4.hpp"
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
        std::vector<std::string> displayModeStringList;
        bool fullScreen = false;

        struct GUI
        {
            Render::PipelineStateHandle pipelineState;
            Render::SamplerStateHandle samplerState;

            Render::Device::QueuePtr renderQueue;

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
            configuration = JSON::Load(getContext()->getRootFileName("config.json"));
            previousDisplayMode = currentDisplayMode = JSON::Reference(configuration).get("display").get("mode").convert(0);
            configuration["display"]["mode"] = currentDisplayMode;

            Window::Description windowDescription;
            windowDescription.className = "GEK_Engine_Demo";
            windowDescription.windowName = "GEK Engine Demo";
            window = getContext()->createClass<Window>("Default::Render::Window", windowDescription);

            window->onClose.connect(this, &Core::onClose);
            window->onActivate.connect(this, &Core::onActivate);
            window->onSizeChanged.connect(this, &Core::onSizeChanged);
            window->onKeyPressed.connect(this, &Core::onKeyPressed);
            window->onCharacter.connect(this, &Core::onCharacter);
            window->onSetCursor.connect(this, &Core::onSetCursor);
            window->onMouseClicked.connect(this, &Core::onMouseClicked);
            window->onMouseWheel.connect(this, &Core::onMouseWheel);
            window->onMousePosition.connect(this, &Core::onMousePosition);
            window->onMouseMovement.connect(this, &Core::onMouseMovement);

            HRESULT resultValue = CoInitializeEx(nullptr, COINITBASE_MULTITHREADED);
            if (FAILED(resultValue))
            {
                throw std::exception("Call to CoInitialize failed");
            }

            Render::Device::Description deviceDescription;
            renderDevice = getContext()->createClass<Render::Device>("Default::Render::Device", window.get(), deviceDescription);
            displayModeList = renderDevice->getDisplayModeList(deviceDescription.displayFormat);
            for (const auto &displayMode : displayModeList)
            {
                std::string displayModeString(String::Format("%vx%v, %vhz", displayMode.width, displayMode.height, uint32_t(std::ceil(float(displayMode.refreshRate.numerator) / float(displayMode.refreshRate.denominator)))));
                switch (displayMode.aspectRatio)
                {
                case Render::DisplayMode::AspectRatio::_4x3:
                    displayModeString += " (4x3)";
                    break;

                case Render::DisplayMode::AspectRatio::_16x9:
                    displayModeString += " (16x9)";
                    break;

                case Render::DisplayMode::AspectRatio::_16x10:
                    displayModeString += " (16x10)";
                    break;
                };

                displayModeStringList.push_back(displayModeString);
            }

            auto baseFileName(getContext()->getRootFileName("data", "gui"));
            gui->consoleButton = renderDevice->loadTexture(FileSystem::GetFileName(baseFileName, "console.png"), 0);
            gui->performanceButton = renderDevice->loadTexture(FileSystem::GetFileName(baseFileName, "performance.png"), 0);
            gui->settingsButton = renderDevice->loadTexture(FileSystem::GetFileName(baseFileName, "settings.png"), 0);
            gui->renderQueue = renderDevice->createQueue(0);

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
            ImGui::ResetStyle(ImGuiStyle_EdinBlack, style);
            style.WindowTitleAlign = ImVec2(0.5f, 0.5f);
            style.ButtonTextAlign = ImVec2(0.5f, 0.5f);
            style.WindowRounding = 0.0f;
            style.FrameRounding = 3.0f;

            static char const * const guiProgram =
                "DeclareConstantBuffer(Constants, 0)\r\n" \
                "{\r\n" \
                "    float4x4 ProjectionMatrix;\r\n" \
                "};\r\n" \
                "\r\n" \
                "DeclareSamplerState(PointSampler, 0);\r\n" \
                "DeclareTexture2D(GuiTexture, float4, 0);\r\n" \
                "\r\n" \
                "Pixel mainVertexProgram(in Vertex input)\r\n" \
                "{\r\n" \
                "    Pixel output;\r\n" \
                "    output.position = mul(ProjectionMatrix, float4(input.position.xy, 0.0f, 1.0f));\r\n" \
                "    output.color = input.color;\r\n" \
                "    output.texCoord  = input.texCoord;\r\n" \
                "    return output;\r\n" \
                "}\r\n" \
                "\r\n" \
                "Output mainPixelProgram(in Pixel input)\r\n" \
                "{\r\n" \
                "    Output output;\r\n" \
                "    output.screen = (input.color * SampleTexture(GuiTexture, PointSampler, input.texCoord));\r\n" \
                "    return output;\r\n" \
                "}";

            Render::PipelineStateInformation pipelineStateInformation;
            pipelineStateInformation.vertexShader = guiProgram;
            pipelineStateInformation.vertexShaderEntryFunction = "mainVertexProgram";
            pipelineStateInformation.pixelShader = guiProgram;
            pipelineStateInformation.pixelShaderEntryFunction = "mainPixelProgram";

            Render::VertexDeclaration vertexDeclaration;
            vertexDeclaration.name = "position";
            vertexDeclaration.format = Render::Format::R32G32_FLOAT;
            vertexDeclaration.semantic = Render::VertexDeclaration::Semantic::Position;
            pipelineStateInformation.vertexDeclaration.push_back(vertexDeclaration);

            vertexDeclaration.name = "texCoord";
            vertexDeclaration.format = Render::Format::R32G32_FLOAT;
            vertexDeclaration.semantic = Render::VertexDeclaration::Semantic::TexCoord;
            pipelineStateInformation.vertexDeclaration.push_back(vertexDeclaration);

            vertexDeclaration.name = "color";
            vertexDeclaration.format = Render::Format::R8G8B8A8_UNORM;
            vertexDeclaration.semantic = Render::VertexDeclaration::Semantic::Color;
            pipelineStateInformation.vertexDeclaration.push_back(vertexDeclaration);

            Render::ElementDeclaration pixelDeclaration;
            pixelDeclaration.name = "position";
            pixelDeclaration.format = Render::Format::R32G32B32A32_FLOAT;
            pixelDeclaration.semantic = Render::VertexDeclaration::Semantic::Position;
            pipelineStateInformation.pixelDeclaration.push_back(pixelDeclaration);

            pixelDeclaration.name = "texCoord";
            pixelDeclaration.format = Render::Format::R32G32_FLOAT;
            pixelDeclaration.semantic = Render::VertexDeclaration::Semantic::TexCoord;
            pipelineStateInformation.pixelDeclaration.push_back(pixelDeclaration);

            pixelDeclaration.name = "color";
            pixelDeclaration.format = Render::Format::R8G8B8A8_UNORM;
            pixelDeclaration.semantic = Render::VertexDeclaration::Semantic::Color;
            pipelineStateInformation.pixelDeclaration.push_back(pixelDeclaration);

            Render::NamedDeclaration targetDeclaration;
            targetDeclaration.name = "screen";
            targetDeclaration.format = Render::Format::R8G8B8A8_UNORM_SRGB;
            pipelineStateInformation.renderTargetList.push_back(targetDeclaration);

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

            gui->pipelineState = renderDevice->createPipelineState(pipelineStateInformation, "GUI");

            Render::BufferDescription constantBufferDescription;
            constantBufferDescription.stride = sizeof(Math::Float4x4);
            constantBufferDescription.count = 1;
            constantBufferDescription.type = Render::BufferDescription::Type::Constant;
            gui->constantBuffer = renderDevice->createBuffer(constantBufferDescription, nullptr, "ImGui::Constants");

            uint8_t *pixels = nullptr;
            int32_t fontWidth = 0, fontHeight = 0;
            imGuiIo.Fonts->GetTexDataAsRGBA32(&pixels, &fontWidth, &fontHeight);

            Render::TextureDescription fontDescription;
            fontDescription.format = Render::Format::R8G8B8A8_UNORM;
            fontDescription.width = fontWidth;
            fontDescription.height = fontHeight;
            fontDescription.flags = Render::TextureDescription::Flags::Resource;
            gui->fontTexture = renderDevice->createTexture(fontDescription, pixels, "ImGui::Font");
            imGuiIo.Fonts->TexID = &gui->fontTexture;

            Render::SamplerStateInformation samplerStateInformation;
            samplerStateInformation.filterMode = Render::SamplerStateInformation::FilterMode::MinificationMagnificationMipMapPoint;
            samplerStateInformation.addressModeU = Render::SamplerStateInformation::AddressMode::Clamp;
            samplerStateInformation.addressModeV = Render::SamplerStateInformation::AddressMode::Clamp;
            samplerStateInformation.addressModeW = Render::SamplerStateInformation::AddressMode::Clamp;
            gui->samplerState = renderDevice->createSamplerState(samplerStateInformation, "ImGui::Sampler");

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
            gui = nullptr;
            ImGui::GetIO().Fonts->TexID = 0;
            ImGui::Shutdown();

            renderDevice = nullptr;
            window = nullptr;
            context = nullptr;

            CoUninitialize();
        }

        void setDisplayMode(uint32_t displayMode)
        {
            auto &displayModeData = displayModeList[displayMode];
            if (displayMode < displayModeList.size())
            {
                currentDisplayMode = displayMode;
                renderDevice->setDisplayMode(displayModeData);
                window->move();
            }
        }

        void run(void)
        {
            while (engineRunning)
            {
                window->readEvents();

                auto description = renderDevice->getTextureDescription(Render::Device::SwapChain);

                ImGuiIO &imGuiIo = ImGui::GetIO();

                uint32_t width = description->width;
                uint32_t height = description->height;
                imGuiIo.DisplaySize = ImVec2(float(width), float(height));
                float barWidth = float(width);

                // Read keyboard modifiers inputs
                imGuiIo.KeyCtrl = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
                imGuiIo.KeyShift = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
                imGuiIo.KeyAlt = (GetKeyState(VK_MENU) & 0x8000) != 0;
                imGuiIo.KeySuper = false;
                // imGuiIo.KeysDown : filled by WM_KEYDOWN/WM_KEYUP events
                // imGuiIo.MousePos : filled by WM_MOUSEMOVE events
                // imGuiIo.MouseDown : filled by WM_*BUTTON* events
                // imGuiIo.MouseWheel : filled by WM_MOUSEWHEEL events

                ImGui::NewFrame();
                ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
                ImGui::Begin("GEK Engine", nullptr, ImVec2(0, 0), 0.0f, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoTitleBar);
                if (windowActive)
                {
                    if (showCursor)
                    {
                        if (showModeChange)
                        {
                            ImGui::SetNextWindowPosCenter();
                            ImGui::Begin("Keep Display Mode", nullptr, ImGuiWindowFlags_ShowBorders | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysUseWindowPadding);
                            ImGui::Text("Keep Display Mode?");

                            if (ImGui::Button("Yes"))
                            {
                                showModeChange = false;
                                previousDisplayMode = currentDisplayMode;
                            }

                            ImGui::SameLine();
                            modeChangeTimer -= 0.001f;
                            if (modeChangeTimer <= 0.0f || ImGui::Button("No"))
                            {
                                showModeChange = false;
                                setDisplayMode(previousDisplayMode);
                            }

                            ImGui::Text(String::Format("(Revert in %v seconds)", uint32_t(modeChangeTimer)).c_str());

                            ImGui::End();
                        }
                    }
                }

                if (!windowActive)
                {
                    ImGui::SetNextWindowPosCenter();
                    ImGui::Begin("GEK Engine##Paused", nullptr, ImGuiWindowFlags_ShowBorders | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysUseWindowPadding | ImGuiWindowFlags_NoCollapse);
                    ImGui::Dummy(ImVec2(200, 0));
                    ImGui::Text("Paused");
                    ImGui::End();
                }

                if (windowActive && !showCursor)
                {
                    auto rectangle = window->getScreenRectangle();
                    window->setCursorPosition(Math::Int2(
                        int(Math::Interpolate(float(rectangle.minimum.x), float(rectangle.maximum.x), 0.5f)),
                        int(Math::Interpolate(float(rectangle.minimum.y), float(rectangle.maximum.y), 0.5f))));
                }

                ImGui::End();

                ImGui::Render();
                renderDevice->present(false);
            };
        }

        // ImGui Callbacks
        void renderDrawData(ImDrawData *drawData)
        {
            if (!gui->vertexBuffer || renderDevice->getBufferDescription(gui->vertexBuffer)->count < uint32_t(drawData->TotalVtxCount))
            {
                Render::BufferDescription vertexBufferDescription;
                vertexBufferDescription.stride = sizeof(ImDrawVert);
                vertexBufferDescription.count = drawData->TotalVtxCount;
                vertexBufferDescription.type = Render::BufferDescription::Type::Vertex;
                vertexBufferDescription.flags = Render::BufferDescription::Flags::Mappable;
                gui->vertexBuffer = renderDevice->createBuffer(vertexBufferDescription);
            }

            if (!gui->indexBuffer || renderDevice->getBufferDescription(gui->indexBuffer)->count < uint32_t(drawData->TotalIdxCount))
            {
                Render::BufferDescription vertexBufferDescription;
                vertexBufferDescription.count = drawData->TotalIdxCount;
                vertexBufferDescription.type = Render::BufferDescription::Type::Index;
                vertexBufferDescription.flags = Render::BufferDescription::Flags::Mappable;
                switch (sizeof(ImDrawIdx))
                {
                case 4:
                    vertexBufferDescription.format = Render::Format::R32_UINT;
                    break;

                case 2:
                default:
                    vertexBufferDescription.format = Render::Format::R16_UINT;
                    break;
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
                ImGuiIO &imGuiIo = ImGui::GetIO();
                uint32_t width = imGuiIo.DisplaySize.x;
                uint32_t height = imGuiIo.DisplaySize.y;
                auto orthographic = Math::Float4x4::MakeOrthographic(0.0f, 0.0f, float(width), float(height), 0.0f, 1.0f);
                renderDevice->updateResource(gui->constantBuffer, &orthographic);

                Render::ViewPort viewPort(Math::Float2::Zero, Math::Float2(width, height), 0.0f, 1.0f);

                gui->renderQueue->reset();
                gui->renderQueue->bindPipelineState(gui->pipelineState);
                gui->renderQueue->bindVertexBufferList({ gui->vertexBuffer }, 0);
                gui->renderQueue->bindIndexBuffer(gui->indexBuffer, 0);
                gui->renderQueue->bindConstantBufferList({ gui->constantBuffer }, 0, Render::Pipeline::Vertex);
                gui->renderQueue->bindSamplerStateList({ gui->samplerState }, 0, Render::Pipeline::Pixel);
                gui->renderQueue->bindRenderTargetList({ Render::Device::SwapChain }, Render::ResourceHandle());
                gui->renderQueue->setViewportList({ viewPort });

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
                            gui->renderQueue->setScissorList(scissorBoxList);

                            gui->renderQueue->bindResourceList({ *(Render::ResourceHandle *)command->TextureId }, 0, Render::Pipeline::Pixel);

                            gui->renderQueue->drawIndexedPrimitive(command->ElemCount, indexOffset, vertexOffset);
                        }

                        indexOffset += command->ElemCount;
                    }

                    vertexOffset += commandList->VtxBuffer.Size;
                }
            }

            renderDevice->runQueue(gui->renderQueue.get());
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
            auto listBoxSize = (windowData.size - (ImGui::GetStyle().WindowPadding * 2.0f));
            listBoxSize.y -= ImGui::GetTextLineHeightWithSpacing();
            listBoxSize.y -= ImGui::GetTextLineHeightWithSpacing();
            if (ImGui::ListBoxHeader("##Display Mode", listBoxSize))
            {
                ImGuiListClipper clipper(displayModeStringList.size(), ImGui::GetTextLineHeightWithSpacing());
                while (clipper.Step())
                {
                    for (auto mode = clipper.DisplayStart; mode < clipper.DisplayEnd; ++mode)
                    {
                        bool isSelected = (mode == currentDisplayMode);
                        auto data = std::next(std::begin(displayModeStringList), mode);
                        if (ImGui::Selectable(data->c_str(), &isSelected))
                        {
                            setDisplayMode(mode);
                        }
                    }
                };

                ImGui::ListBoxFooter();
            }

            ImGui::PopItemWidth();
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
            ImGuiIO &imGuiIo = ImGui::GetIO();
            imGuiIo.AddInputCharacter(character);
        }

        void onKeyPressed(Window::Key key, bool state)
        {
            ImGuiIO &imGuiIo = ImGui::GetIO();
            imGuiIo.KeysDown[static_cast<int>(key)] = state;
            if (state)
            {
                switch (key)
                {
                case Window::Key::Escape:
                    showCursor = !showCursor;
                    imGuiIo.MouseDrawCursor = showCursor;
                    break;
                };
            }
        }

        void onMouseClicked(Window::Button button, bool state)
        {
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
        }

        void onMouseWheel(float numberOfRotations)
        {
            ImGuiIO &imGuiIo = ImGui::GetIO();
            imGuiIo.MouseWheel += numberOfRotations;
        }

        void onMousePosition(int32_t xPosition, int32_t yPosition)
        {
            ImGuiIO &imGuiIo = ImGui::GetIO();
            imGuiIo.MousePos.x = xPosition;
            imGuiIo.MousePos.y = yPosition;
        }

        void onMouseMovement(int32_t xMovement, int32_t yMovement)
        {
        }
    };
}; // namespace Gek

int CALLBACK wWinMain(_In_ HINSTANCE instance, _In_opt_ HINSTANCE previousInstance, _In_ PWSTR commandLine, _In_ int commandShow)
{
    try
    {
        Gek::Core core;
        core.run();
    }
    catch (std::exception const &)
    {
    };

    return 0;
}