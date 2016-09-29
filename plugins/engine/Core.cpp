#include "GEK\Utility\Exceptions.h"
#include "GEK\Utility\String.h"
#include "GEK\Utility\FileSystem.h"
#include "GEK\Utility\XML.h"
#include "GEK\Utility\Timer.h"
#include "GEK\Engine\Application.h"
#include "GEK\Engine\Core.h"
#include "GEK\Engine\Population.h"
#include "GEK\Engine\Resources.h"
#include "GEK\Engine\Renderer.h"
#include "GEK\Context\ContextUser.h"
#include <concurrent_queue.h>
#include <ppl.h>

#include <imgui.h>

namespace Gek
{
    namespace Implementation
    {
        struct ActionQueue
        {
            concurrency::critical_section lock;
            std::list<std::pair<wchar_t, bool>> queue;

            void addAction(wchar_t character, bool state)
            {
                concurrency::critical_section::scoped_lock scope(lock);
                queue.emplace_back(character, state);
            }

            std::list<std::pair<wchar_t, bool>> getQueue(void)
            {
                concurrency::critical_section::scoped_lock scope(lock);
                return std::move(queue);
            }

            void clear(void)
            {
                queue.clear();
            }
        };

        GEK_INTERFACE(CoreConfiguration)
        {
            virtual void changeConfiguration(Xml::Node &&configuration) = 0;
        };

        class Configuration
            : public Plugin::Configuration
        {
        private:
            CoreConfiguration *core;
            Xml::Node configuration;

        public:
            Configuration(CoreConfiguration *core, const Xml::Node &configuration)
                : core(core)
                , configuration(configuration)
            {
            }

            ~Configuration(void)
            {
                core->changeConfiguration(std::move(configuration));
            }

            operator Xml::Node &()
            {
                return configuration;
            }
        };

        GEK_CONTEXT_USER(Core, HWND)
            , public Application
            , public Plugin::Core
            , public Plugin::PopulationListener
            , public Plugin::PopulationStep
            , public Plugin::RendererListener
            , public CoreConfiguration
        {
        public:
            struct Command
            {
                String function;
                std::vector<String> parameterList;
            };

        private:
            HWND window;
            bool windowActive;
            bool engineRunning;

            Xml::Node configuration;

            Timer timer;
            double updateAccumulator;
            POINT lastCursorPosition;
            float mouseSensitivity;

            Video::DevicePtr device;
            Plugin::RendererPtr renderer;
            Engine::ResourcesPtr resources;
            std::list<Plugin::ProcessorPtr> processorList;
            Plugin::PopulationPtr population;

            ActionQueue actionQueue;

            Video::ObjectPtr vertexProgram;
            Video::ObjectPtr inputLayout;
            Video::BufferPtr constantBuffer;
            Video::ObjectPtr pixelProgram;
            Video::ObjectPtr blendState;
            Video::ObjectPtr renderState;
            Video::ObjectPtr depthState;
            Video::TexturePtr font;
            Video::ObjectPtr samplerState;
            Video::BufferPtr vertexBuffer;
            Video::BufferPtr indexBuffer;

        public:
            Core(Context *context, HWND window)
                : ContextRegistration(context)
                , window(window)
                , windowActive(false)
                , engineRunning(true)
                , configuration(nullptr)
                , updateAccumulator(0.0)
                , mouseSensitivity(0.5f)
            {
                GEK_REQUIRE(window);
/*
                consoleCommandsMap[L"fullscreen"] = [this](const std::vector<String> &parameters) -> void
                {
                    bool fullscreen = !device->isFullScreen();

                    device->setFullScreen(fullscreen);

                    auto &displayNode = configuration.getChild(L"display");
                    displayNode.attributes[L"fullscreen"] = fullscreen;
                    sendShout(&Plugin::CoreListener::onConfigurationChanged);
                };

                consoleCommandsMap[L"setsize"] = [this](const std::vector<String> &parameters) -> void
                {
                    if (parameters.size() == 2)
                    {
                        uint32_t width = parameters[0];
                        uint32_t height = parameters[1];
                        device->setSize(width, height, Video::Format::R8G8B8A8_UNORM_SRGB);

                        auto &displayNode = configuration.getChild(L"display");
                        displayNode.attributes[L"width"] = width;
                        displayNode.attributes[L"height"] = height;
                        sendShout(&Plugin::CoreListener::onConfigurationChanged);

                        sendShout(&Plugin::CoreListener::onResize);
                    }
                };
*/
                try
                {
                    configuration = Xml::load(getContext()->getFileName(L"config.xml"), L"config");
                }
                catch (const Xml::Exception &)
                {
                    configuration = Xml::Node(L"config");
                };

                HRESULT resultValue = CoInitialize(nullptr);
                if (FAILED(resultValue))
                {
                    throw InitializationFailed();
                }

                device = getContext()->createClass<Video::Device>(L"Default::Device::Video", window, Video::Format::R8G8B8A8_UNORM_SRGB, String(L"default"));
                population = getContext()->createClass<Plugin::Population>(L"Engine::Population", (Plugin::Core *)this);
                resources = getContext()->createClass<Engine::Resources>(L"Engine::Resources", (Plugin::Core *)this, device.get());
                renderer = getContext()->createClass<Plugin::Renderer>(L"Engine::Renderer", device.get(), getPopulation(), resources.get());
                getContext()->listTypes(L"ProcessorType", [&](const wchar_t *className) -> void
                {
                    processorList.push_back(getContext()->createClass<Plugin::Processor>(className, (Plugin::Core *)this));
                });

                population->addStep(this, 0, 100);
                renderer->addListener(this);

                ImGuiIO &io = ImGui::GetIO();
                io.KeyMap[ImGuiKey_Tab] = VK_TAB;
                io.KeyMap[ImGuiKey_LeftArrow] = VK_LEFT;
                io.KeyMap[ImGuiKey_RightArrow] = VK_RIGHT;
                io.KeyMap[ImGuiKey_UpArrow] = VK_UP;
                io.KeyMap[ImGuiKey_DownArrow] = VK_DOWN;
                io.KeyMap[ImGuiKey_PageUp] = VK_PRIOR;
                io.KeyMap[ImGuiKey_PageDown] = VK_NEXT;
                io.KeyMap[ImGuiKey_Home] = VK_HOME;
                io.KeyMap[ImGuiKey_End] = VK_END;
                io.KeyMap[ImGuiKey_Delete] = VK_DELETE;
                io.KeyMap[ImGuiKey_Backspace] = VK_BACK;
                io.KeyMap[ImGuiKey_Enter] = VK_RETURN;
                io.KeyMap[ImGuiKey_Escape] = VK_ESCAPE;
                io.KeyMap[ImGuiKey_A] = 'A';
                io.KeyMap[ImGuiKey_C] = 'C';
                io.KeyMap[ImGuiKey_V] = 'V';
                io.KeyMap[ImGuiKey_X] = 'X';
                io.KeyMap[ImGuiKey_Y] = 'Y';
                io.KeyMap[ImGuiKey_Z] = 'Z';
                io.ImeWindowHandle = window;

                static const wchar_t *vertexShader =
                    L"cbuffer vertexBuffer : register(b0)                                   \
                    {                                                                       \
                        float4x4 ProjectionMatrix;                                          \
                    };                                                                      \
                                                                                            \
                    struct VS_INPUT                                                         \
                    {                                                                       \
                        float2 pos : POSITION;                                              \
                        float4 col : COLOR0;                                                \
                        float2 uv  : TEXCOORD0;                                             \
                    };                                                                      \
                                                                                            \
                    struct PS_INPUT                                                         \
                    {                                                                       \
                        float4 pos : SV_POSITION;                                           \
                        float4 col : COLOR0;                                                \
                        float2 uv  : TEXCOORD0;                                             \
                    };                                                                      \
                                                                                            \
                    PS_INPUT main(VS_INPUT input)                                           \
                    {                                                                       \
                        PS_INPUT output;                                                    \
                        output.pos = mul( ProjectionMatrix, float4(input.pos.xy, 0.f, 1.f));\
                        output.col = input.col;                                             \
                        output.uv  = input.uv;                                              \
                        return output;                                                      \
                    }";

                auto &compiled = resources->compileProgram(Video::ProgramType::Vertex, L"uiVertexProgram", L"main", vertexShader);
                vertexProgram = device->createProgram(Video::ProgramType::Vertex, compiled.data(), compiled.size());

                std::vector<Video::InputElement> elementList;
                elementList.push_back(Video::InputElement(Video::Format::R32G32_FLOAT, Video::InputElement::Semantic::Position));
                elementList.push_back(Video::InputElement(Video::Format::R32G32_FLOAT, Video::InputElement::Semantic::TexCoord));
                elementList.push_back(Video::InputElement(Video::Format::R8G8B8A8_UNORM, Video::InputElement::Semantic::Color));
                inputLayout = device->createInputLayout(elementList, compiled.data(), compiled.size());

                constantBuffer = device->createBuffer(sizeof(Math::Float4x4), 1, Video::BufferType::Constant, 0);

                static const wchar_t *pixelShader =
                    L"struct PS_INPUT                                                       \
                    {                                                                       \
                        float4 pos : SV_POSITION;                                           \
                        float4 col : COLOR0;                                                \
                        float2 uv  : TEXCOORD0;                                             \
                    };                                                                      \
                                                                                            \
                    sampler sampler0;                                                       \
                    Texture2D texture0;                                                     \
                                                                                            \
                    float4 main(PS_INPUT input) : SV_Target                                 \
                    {                                                                       \
                        float4 out_col = input.col * texture0.Sample(sampler0, input.uv);   \
                        return out_col;                                                     \
                    }";

                compiled = resources->compileProgram(Video::ProgramType::Pixel, L"uiPixelProgram", L"main", pixelShader);
                pixelProgram = device->createProgram(Video::ProgramType::Pixel, compiled.data(), compiled.size());

                Video::UnifiedBlendStateInformation blendStateInformation;
                blendStateInformation.enable = true;
                blendStateInformation.colorSource = Video::BlendStateInformation::Source::SourceAlpha;
                blendStateInformation.colorDestination = Video::BlendStateInformation::Source::InverseSourceAlpha;
                blendStateInformation.colorOperation = Video::BlendStateInformation::Operation::Add;
                blendStateInformation.alphaSource = Video::BlendStateInformation::Source::InverseSourceAlpha;
                blendStateInformation.alphaDestination = Video::BlendStateInformation::Source::Zero;
                blendStateInformation.alphaOperation = Video::BlendStateInformation::Operation::Add;
                blendState = device->createBlendState(blendStateInformation);

                Video::RenderStateInformation renderStateInformation;
                renderStateInformation.fillMode = Video::RenderStateInformation::FillMode::Solid;
                renderStateInformation.cullMode = Video::RenderStateInformation::CullMode::None;
                renderStateInformation.scissorEnable = true;
                renderStateInformation.depthClipEnable = true;
                renderState = device->createRenderState(renderStateInformation);

                Video::DepthStateInformation depthStateInformation;
                depthState = device->createDepthState(depthStateInformation);

                uint8_t *pixels = nullptr;
                int32_t width = 0, height = 0;
                io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);
                font = device->createTexture(Video::Format::R8G8B8A8_UNORM, width, height, 1, 1, Video::TextureFlags::Resource, pixels);
                io.Fonts->TexID = (Video::Object *)font.get();

                Video::SamplerStateInformation samplerStateInformation;
                samplerStateInformation.filterMode = Video::SamplerStateInformation::FilterMode::AllLinear;
                samplerStateInformation.addressModeU = Video::SamplerStateInformation::AddressMode::Wrap;
                samplerStateInformation.addressModeV = Video::SamplerStateInformation::AddressMode::Wrap;
                samplerStateInformation.addressModeW = Video::SamplerStateInformation::AddressMode::Wrap;
                samplerState = device->createSamplerState(samplerStateInformation);

                io.UserData = this;
                io.RenderDrawListsFn = [](ImDrawData *drawData)
                {
                    ImGuiIO &io = ImGui::GetIO();
                    Core *core = static_cast<Core *>(io.UserData);
                    core->renderDrawData(drawData);
                };

                windowActive = true;
            }

            ~Core(void)
            {
                ImGui::GetIO().Fonts->TexID = 0;
                ImGui::Shutdown();

                vertexProgram = nullptr;
                inputLayout = nullptr;
                constantBuffer = nullptr;
                pixelProgram = nullptr;
                blendState = nullptr;
                renderState = nullptr;
                depthState = nullptr;
                font = nullptr;
                samplerState = nullptr;
                vertexBuffer = nullptr;
                indexBuffer = nullptr;

                if (population)
                {
                    population->removeStep(this);
                }

                if (renderer)
                {
                    renderer->removeListener(this);
                }

                processorList.clear();
                renderer = nullptr;
                resources = nullptr;
                population = nullptr;
                device = nullptr;
                CoUninitialize();
            }

            // CoreConfiguration
            void changeConfiguration(Xml::Node &&configuration)
            {
                this->configuration = std::move(configuration);
                sendShout(&Plugin::CoreListener::onConfigurationChanged);
            }

            // Plugin::Core
            Plugin::ConfigurationPtr changeConfiguration(void)
            {
                return std::make_shared<Configuration>(this, configuration);
            }

            Xml::Node const &getConfiguration(void) const
            {
                return configuration;
            }

            Plugin::Population * getPopulation(void) const
            {
                return population.get();
            }

            Plugin::Resources * getResources(void) const
            {
                return resources.get();
            }

            Plugin::Renderer * getRenderer(void) const
            {
                return renderer.get();
            }

            // Application
            LRESULT windowEvent(uint32_t message, WPARAM wParam, LPARAM lParam)
            {
                ImGuiIO &io = ImGui::GetIO();
                switch (message)
                {
                case WM_SETCURSOR:
                    return 1;

                case WM_ACTIVATE:
                    if (HIWORD(wParam))
                    {
                        windowActive = false;
                    }
                    else
                    {
                        switch (LOWORD(wParam))
                        {
                        case WA_ACTIVE:
                        case WA_CLICKACTIVE:
                            windowActive = true;
                            break;

                        case WA_INACTIVE:
                            windowActive = false;
                            break;
                        };
                    }

                    timer.pause(!windowActive);
                    return 1;

                case WM_SYSCOMMAND:
                    if (SC_KEYMENU == (wParam & 0xFFF0))
                    {
                        device->setFullScreen(!device->isFullScreen());
                        return 1;
                    }

                    break;

                case WM_SIZE:
                    device->resize();
                    return 1;

                case WM_LBUTTONDOWN:
                    io.MouseDown[0] = true;
                    return true;

                case WM_LBUTTONUP:
                    io.MouseDown[0] = false;
                    return true;

                case WM_RBUTTONDOWN:
                    io.MouseDown[1] = true;
                    return true;

                case WM_RBUTTONUP:
                    io.MouseDown[1] = false;
                    return true;

                case WM_MBUTTONDOWN:
                    io.MouseDown[2] = true;
                    return true;

                case WM_MBUTTONUP:
                    io.MouseDown[2] = false;
                    return true;

                case WM_MOUSEWHEEL:
                    io.MouseWheel += GET_WHEEL_DELTA_WPARAM(wParam) > 0 ? +1.0f : -1.0f;
                    return true;

                case WM_MOUSEMOVE:
                    io.MousePos.x = (int16_t)(lParam);
                    io.MousePos.y = (int16_t)(lParam >> 16);
                    return true;

                case WM_KEYDOWN:
                    actionQueue.addAction(wParam, true);
                    if (wParam < 256)
                    {
                        io.KeysDown[wParam] = 1;
                    }

                    return true;

                case WM_KEYUP:
                    if (wParam < 256)
                    {
                        actionQueue.addAction(wParam, false);
                        io.KeysDown[wParam] = 0;
                    }

                    return true;

                case WM_CHAR:
                    // You can also use ToAscii()+GetKeyboardState() to retrieve characters.
                    if (wParam > 0 && wParam < 0x10000)
                    {
                        io.AddInputCharacter((uint16_t)wParam);
                    }

                    return true;
                };

                return 0;
            }

            bool update(void)
            {
                timer.update();
                float frameTime = float(timer.getUpdateTime());
                population->update(!windowActive, frameTime);
                return engineRunning;
            }

            // Plugin::PopulationStep
            void onUpdate(uint32_t order, State state)
            {
                if (order == 0)
                {
                    POINT currentCursorPosition;
                    GetCursorPos(&currentCursorPosition);
                    float cursorMovementX = (float(currentCursorPosition.x - lastCursorPosition.x) * mouseSensitivity);
                    float cursorMovementY = (float(currentCursorPosition.y - lastCursorPosition.y) * mouseSensitivity);
                    lastCursorPosition = currentCursorPosition;
                    if (state == State::Active)
                    {
                        if (std::abs(cursorMovementX) > Math::Epsilon || std::abs(cursorMovementY) > Math::Epsilon)
                        {
                            sendShout(&Plugin::CoreListener::onAction, L"turn", Plugin::ActionParameter(cursorMovementX));
                            sendShout(&Plugin::CoreListener::onAction, L"tilt", Plugin::ActionParameter(cursorMovementY));
                        }

                        std::list<std::pair<wchar_t, bool>> actionCopy(actionQueue.getQueue());
                        for (auto &action : actionCopy)
                        {
                            Plugin::ActionParameter parameter(action.second);
                            switch (action.first)
                            {
                            case 'W':
                            case VK_UP:
                                sendShout(&Plugin::CoreListener::onAction, L"move_forward", parameter);
                                break;

                            case 'S':
                            case VK_DOWN:
                                sendShout(&Plugin::CoreListener::onAction, L"move_backward", parameter);
                                break;

                            case 'A':
                            case VK_LEFT:
                                sendShout(&Plugin::CoreListener::onAction, L"strafe_left", parameter);
                                break;

                            case 'D':
                            case VK_RIGHT:
                                sendShout(&Plugin::CoreListener::onAction, L"strafe_right", parameter);
                                break;

                            case VK_SPACE:
                                sendShout(&Plugin::CoreListener::onAction, L"jump", parameter);
                                break;

                            case VK_LCONTROL:
                                sendShout(&Plugin::CoreListener::onAction, L"crouch", parameter);
                                break;
                            };
                        }
                    }
                    else
                    {
                        actionQueue.clear();
                    }

                    ImGuiIO &io = ImGui::GetIO();

                    auto backBuffer = device->getBackBuffer();
                    uint32_t width = backBuffer->getWidth();
                    uint32_t height = backBuffer->getHeight();
                    io.DisplaySize = ImVec2(float(width), float(height));

                    io.DeltaTime = float(timer.getUpdateTime());

                    // Read keyboard modifiers inputs
                    io.KeyCtrl = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
                    io.KeyShift = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
                    io.KeyAlt = (GetKeyState(VK_MENU) & 0x8000) != 0;
                    io.KeySuper = false;
                    // io.KeysDown : filled by WM_KEYDOWN/WM_KEYUP events
                    // io.MousePos : filled by WM_MOUSEMOVE events
                    // io.MouseDown : filled by WM_*BUTTON* events
                    // io.MouseWheel : filled by WM_MOUSEWHEEL events

                    // Hide OS mouse cursor if ImGui is drawing it
                    SetCursor(io.MouseDrawCursor ? NULL : LoadCursor(NULL, IDC_ARROW));

                    // Start the frame
                    ImGui::NewFrame();

                    static Math::Color clearColor;
                    static float slider = 0.0f;
                    ImGui::Text("Hello, world!");
                    ImGui::SliderFloat("float", &slider, 0.0f, 1.0f);
                    ImGui::ColorEdit3("clear color", clearColor.data);
                    ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
                    if (ImGui::Button("Load"))
                    {
                        population->load(L"sponza");
                    }

                    if (ImGui::Button("Quit"))
                    {
                        engineRunning = false;
                    }

                    device->getDefaultContext()->clearRenderTarget(device->getBackBuffer(), clearColor);
                }
                else if (order == 100)
                {
                    ImGui::Render();
                    device->present(false);
                }
            }

            // ImGui
            void renderDrawData(ImDrawData *drawData)
            {
                if (!vertexBuffer || vertexBuffer->getCount() < drawData->TotalVtxCount)
                {
                    vertexBuffer = device->createBuffer(sizeof(ImDrawVert), drawData->TotalVtxCount, Video::BufferType::Vertex, Video::BufferFlags::Mappable);
                }

                if (!indexBuffer || indexBuffer->getCount() < drawData->TotalIdxCount)
                {
                    switch (sizeof(ImDrawIdx))
                    {
                    case 2:
                        indexBuffer = device->createBuffer(Video::Format::R16_UINT, drawData->TotalIdxCount, Video::BufferType::Index, Video::BufferFlags::Mappable);
                        break;

                    case 4:
                        indexBuffer = device->createBuffer(Video::Format::R32_UINT, drawData->TotalIdxCount, Video::BufferType::Index, Video::BufferFlags::Mappable);
                        break;

                    default:
                        throw std::exception();
                    };
                }

                ImDrawVert* vertexData = nullptr;
                ImDrawIdx* indexData = nullptr;
                device->mapBuffer(vertexBuffer.get(), (void **)&vertexData);
                device->mapBuffer(indexBuffer.get(), (void **)&indexData);
                for (uint32_t commandListIndex = 0; commandListIndex < drawData->CmdListsCount; commandListIndex++)
                {
                    const ImDrawList* commandList = drawData->CmdLists[commandListIndex];
                    memcpy(vertexData, commandList->VtxBuffer.Data, commandList->VtxBuffer.Size * sizeof(ImDrawVert));
                    memcpy(indexData, commandList->IdxBuffer.Data, commandList->IdxBuffer.Size * sizeof(ImDrawIdx));
                    vertexData += commandList->VtxBuffer.Size;
                    indexData += commandList->IdxBuffer.Size;
                }
                
                device->unmapBuffer(indexBuffer.get());
                device->unmapBuffer(vertexBuffer.get());

                auto backBuffer = device->getBackBuffer();
                uint32_t width = backBuffer->getWidth();
                uint32_t height = backBuffer->getHeight();
                auto orthographic = Math::Float4x4::createOrthographic(0.0f, 0.0f, float(width), float(height), 0.0f, 1.0f);
                device->updateResource(constantBuffer.get(), &orthographic);

                auto context = device->getDefaultContext();
                context->setRenderTargets(&backBuffer, 1, nullptr);
                context->setViewports(&backBuffer->getViewPort(), 1);

                context->setInputLayout(inputLayout.get());
                context->setVertexBuffer(0, vertexBuffer.get(), 0);
                context->setIndexBuffer(indexBuffer.get(), 0);
                context->setPrimitiveType(Video::PrimitiveType::TriangleList);
                context->vertexPipeline()->setProgram(vertexProgram.get());
                context->vertexPipeline()->setConstantBuffer(constantBuffer.get(), 0);
                context->pixelPipeline()->setProgram(pixelProgram.get());
                context->pixelPipeline()->setSamplerState(samplerState.get(), 0);

                context->setBlendState(blendState.get(), Math::Color::Black, 0xFFFFFFFF);
                context->setDepthState(depthState.get(), 0);
                context->setRenderState(renderState.get());

                // Render command lists
                uint32_t vertexOffset = 0;
                uint32_t indexOffset = 0;
                for (uint32_t commandListIndex = 0; commandListIndex < drawData->CmdListsCount; commandListIndex++)
                {
                    const ImDrawList* commandList = drawData->CmdLists[commandListIndex];
                    for (uint32_t commandIndex = 0; commandIndex < commandList->CmdBuffer.Size; commandIndex++)
                    {
                        const ImDrawCmd* command = &commandList->CmdBuffer[commandIndex];
                        if (command->UserCallback)
                        {
                            command->UserCallback(commandList, command);
                        }
                        else
                        {
                            const Shapes::Rectangle<uint32_t> scissor =
                            { 
                                uint32_t(command->ClipRect.x),
                                uint32_t(command->ClipRect.y),
                                uint32_t(command->ClipRect.z),
                                uint32_t(command->ClipRect.w),
                            };

                            context->setScissorRect(&scissor, 1);
                            context->pixelPipeline()->setResource((Video::Object *)command->TextureId, 0);
                            context->drawIndexedPrimitive(command->ElemCount, indexOffset, vertexOffset);
                        }

                        indexOffset += command->ElemCount;
                    }

                    vertexOffset += commandList->VtxBuffer.Size;
                }
            }
        };

        GEK_REGISTER_CONTEXT_USER(Core);
    }; // namespace Implementation
}; // namespace Gek
