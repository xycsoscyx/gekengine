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
#include <queue>
#include <ppl.h>

namespace Gek
{
    namespace Implementation
    {
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

            struct Action
            {
                const wchar_t *name;
                Plugin::ActionParameter parameter;

                Action(void)
                    : name(nullptr)
                {
                }

                Action(const wchar_t *name, const Plugin::ActionParameter &parameter)
                    : name(name)
                    , parameter(parameter)
                {
                }
            };

        private:
            HWND window;
            bool windowActive = false;
            bool engineRunning = false;

            int currentDisplayMode = 0;
            std::vector<StringUTF8> displayModeStringList;
            bool fullScreen = false;

            Xml::Node configuration = Xml::Node::Empty;

            Timer timer;
            float mouseSensitivity = 0.5f;

            Video::DevicePtr device;
            Plugin::RendererPtr renderer;
            Engine::ResourcesPtr resources;
            std::list<Plugin::ProcessorPtr> processorList;
            Plugin::PopulationPtr population;

            concurrency::concurrent_queue<Action> actionQueue;

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

            bool showDebugMenu = true;
            char loadLevelName[256] = "sponza";

        public:
            Core(Context *context, HWND window)
                : ContextRegistration(context)
                , window(window)
            {
                GEK_REQUIRE(window);

                try
                {
                    configuration = Xml::load(getContext()->getFileName(L"config.xml"), L"config");
                }
                catch (const Xml::Exception &)
                {
                    configuration = Xml::Node(L"config");
                };

                auto &displayNode = configuration.getChild(L"display");
                fullScreen = displayNode.getAttribute(L"fullscreen", L"false");
                currentDisplayMode = displayNode.getAttribute(L"mode", L"-1");

                HRESULT resultValue = CoInitialize(nullptr);
                if (FAILED(resultValue))
                {
                    throw InitializationFailed();
                }

                device = getContext()->createClass<Video::Device>(L"Default::Device::Video", window, Video::Format::R8G8B8A8_UNORM_SRGB, String(L"default"));

                auto &displayModeList = device->getDisplayModeList();
                if (currentDisplayMode < 0 || currentDisplayMode >= displayModeList.size())
                {
                    currentDisplayMode = 0;
                    for (auto &displayMode : displayModeList)
                    {
                        if (displayMode.width >= 800 && displayMode.height >= 600)
                        {
                            break;
                        }

                        currentDisplayMode++;
                    }
                }

                device->setDisplayMode(currentDisplayMode);
                for (auto &displayMode : displayModeList)
                {
                    StringUTF8 displayModeString(StringUTF8::create("%vx%v, %vhz", displayMode.width, displayMode.height, uint32_t(std::ceil(float(displayMode.refreshRate.numerator) / float(displayMode.refreshRate.denominator)))));
                    switch (displayMode.aspectRatio)
                    {
                    case Video::DisplayMode::AspectRatio::_4x3:
                        displayModeString.append(" (4x3)");
                        break;
                    case Video::DisplayMode::AspectRatio::_16x9:
                        displayModeString.append(" (16x9)");
                        break;
                    case Video::DisplayMode::AspectRatio::_16x10:
                        displayModeString.append(" (16x10)");
                        break;
                    };

                    displayModeStringList.push_back(displayModeString);
                }

                population = getContext()->createClass<Plugin::Population>(L"Engine::Population", (Plugin::Core *)this);
                resources = getContext()->createClass<Engine::Resources>(L"Engine::Resources", (Plugin::Core *)this, device.get());
                renderer = getContext()->createClass<Plugin::Renderer>(L"Engine::Renderer", device.get(), getPopulation(), resources.get());
                getContext()->listTypes(L"ProcessorType", [&](const wchar_t *className) -> void
                {
                    processorList.push_back(getContext()->createClass<Plugin::Processor>(className, (Plugin::Core *)this));
                });

                population->addStep(this, 0, 100);
                renderer->addListener(this);

                ImGuiIO &imGuiIo = ImGui::GetIO();
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
                imGuiIo.ImeWindowHandle = window;
                imGuiIo.MouseDrawCursor = true;

                auto SetupImGuiStyle = [](bool bStyleDark_, float alpha_)
                {
                    ImGuiStyle& style = ImGui::GetStyle();

                    style.WindowTitleAlign = ImVec2(0.5f, 0.5f);

                    // light style from Pacôme Danhiez (user itamago) https://github.com/ocornut/imgui/pull/511#issuecomment-175719267
                    style.Alpha = 1.0f;
                    style.FrameRounding = 3.0f;
                    style.Colors[ImGuiCol_Text] = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
                    style.Colors[ImGuiCol_TextDisabled] = ImVec4(0.60f, 0.60f, 0.60f, 1.00f);
                    style.Colors[ImGuiCol_WindowBg] = ImVec4(0.94f, 0.94f, 0.94f, 0.94f);
                    style.Colors[ImGuiCol_ChildWindowBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
                    style.Colors[ImGuiCol_PopupBg] = ImVec4(1.00f, 1.00f, 1.00f, 0.94f);
                    style.Colors[ImGuiCol_Border] = ImVec4(0.00f, 0.00f, 0.00f, 0.39f);
                    style.Colors[ImGuiCol_BorderShadow] = ImVec4(1.00f, 1.00f, 1.00f, 0.10f);
                    style.Colors[ImGuiCol_FrameBg] = ImVec4(1.00f, 1.00f, 1.00f, 0.94f);
                    style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.40f);
                    style.Colors[ImGuiCol_FrameBgActive] = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
                    style.Colors[ImGuiCol_TitleBg] = ImVec4(0.96f, 0.96f, 0.96f, 1.00f);
                    style.Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(1.00f, 1.00f, 1.00f, 0.51f);
                    style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.82f, 0.82f, 0.82f, 1.00f);
                    style.Colors[ImGuiCol_MenuBarBg] = ImVec4(0.86f, 0.86f, 0.86f, 1.00f);
                    style.Colors[ImGuiCol_ScrollbarBg] = ImVec4(0.98f, 0.98f, 0.98f, 0.53f);
                    style.Colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.69f, 0.69f, 0.69f, 1.00f);
                    style.Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.59f, 0.59f, 0.59f, 1.00f);
                    style.Colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.49f, 0.49f, 0.49f, 1.00f);
                    style.Colors[ImGuiCol_ComboBg] = ImVec4(0.86f, 0.86f, 0.86f, 0.99f);
                    style.Colors[ImGuiCol_CheckMark] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
                    style.Colors[ImGuiCol_SliderGrab] = ImVec4(0.24f, 0.52f, 0.88f, 1.00f);
                    style.Colors[ImGuiCol_SliderGrabActive] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
                    style.Colors[ImGuiCol_Button] = ImVec4(0.26f, 0.59f, 0.98f, 0.40f);
                    style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
                    style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.06f, 0.53f, 0.98f, 1.00f);
                    style.Colors[ImGuiCol_Header] = ImVec4(0.26f, 0.59f, 0.98f, 0.31f);
                    style.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.80f);
                    style.Colors[ImGuiCol_HeaderActive] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
                    style.Colors[ImGuiCol_Column] = ImVec4(0.39f, 0.39f, 0.39f, 1.00f);
                    style.Colors[ImGuiCol_ColumnHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.78f);
                    style.Colors[ImGuiCol_ColumnActive] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
                    style.Colors[ImGuiCol_ResizeGrip] = ImVec4(1.00f, 1.00f, 1.00f, 0.50f);
                    style.Colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
                    style.Colors[ImGuiCol_ResizeGripActive] = ImVec4(0.26f, 0.59f, 0.98f, 0.95f);
                    style.Colors[ImGuiCol_CloseButton] = ImVec4(0.59f, 0.59f, 0.59f, 0.50f);
                    style.Colors[ImGuiCol_CloseButtonHovered] = ImVec4(0.98f, 0.39f, 0.36f, 1.00f);
                    style.Colors[ImGuiCol_CloseButtonActive] = ImVec4(0.98f, 0.39f, 0.36f, 1.00f);
                    style.Colors[ImGuiCol_PlotLines] = ImVec4(0.39f, 0.39f, 0.39f, 1.00f);
                    style.Colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
                    style.Colors[ImGuiCol_PlotHistogram] = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
                    style.Colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
                    style.Colors[ImGuiCol_TextSelectedBg] = ImVec4(0.26f, 0.59f, 0.98f, 0.35f);
                    style.Colors[ImGuiCol_ModalWindowDarkening] = ImVec4(0.20f, 0.20f, 0.20f, 0.35f);

                    if (bStyleDark_)
                    {
                        for (int columnIndex = 0; columnIndex <= ImGuiCol_COUNT; ++columnIndex)
                        {
                            ImVec4& color = style.Colors[columnIndex];
                            float H, S, V;
                            ImGui::ColorConvertRGBtoHSV(color.x, color.y, color.z, H, S, V);

                            if (S < 0.1f)
                            {
                                V = 1.0f - V;
                            }
                            ImGui::ColorConvertHSVtoRGB(H, S, V, color.x, color.y, color.z);
                            if (color.w < 1.00f)
                            {
                                color.w *= alpha_;
                            }
                        }
                    }
                    else
                    {
                        for (int columnIndex = 0; columnIndex <= ImGuiCol_COUNT; ++columnIndex)
                        {
                            ImVec4& color = style.Colors[columnIndex];
                            if (color.w < 1.00f)
                            {
                                color.x *= alpha_;
                                color.y *= alpha_;
                                color.z *= alpha_;
                                color.w *= alpha_;
                            }
                        }
                    }
                };

                SetupImGuiStyle(true, 1.0f);

                static const wchar_t *vertexShader =
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
                    L"PixelOutput main(VertexInput input)" \
                    L"{" \
                    L"    PixelOutput output;" \
                    L"    output.position = mul( ProjectionMatrix, float4(input.position.xy, 0.f, 1.f));" \
                    L"    output.color = input.color;" \
                    L"    output.texCoord  = input.texCoord;" \
                    L"    return output;" \
                    L"}";

                auto &compiled = resources->compileProgram(Video::ProgramType::Vertex, L"uiVertexProgram", L"main", vertexShader);
                vertexProgram = device->createProgram(Video::ProgramType::Vertex, compiled.data(), compiled.size());

                std::vector<Video::InputElement> elementList;
                elementList.push_back(Video::InputElement(Video::Format::R32G32_FLOAT, Video::InputElement::Semantic::Position));
                elementList.push_back(Video::InputElement(Video::Format::R32G32_FLOAT, Video::InputElement::Semantic::TexCoord));
                elementList.push_back(Video::InputElement(Video::Format::R8G8B8A8_UNORM, Video::InputElement::Semantic::Color));
                inputLayout = device->createInputLayout(elementList, compiled.data(), compiled.size());

                constantBuffer = device->createBuffer(sizeof(Math::Float4x4), 1, Video::BufferType::Constant, 0);

                static const wchar_t *pixelShader =
                    L"struct PixelInput" \
                    L"{" \
                    L"    float4 position : SV_POSITION;" \
                    L"    float4 color : COLOR0;" \
                    L"    float2 texCoord  : TEXCOORD0;" \
                    L"};" \
                    L"" \
                    L"sampler sampler0;" \
                    L"Texture2D texture0;" \
                    L"" \
                    L"float4 main(PixelInput input) : SV_Target" \
                    L"{" \
                    L"    return (input.color * texture0.Sample(sampler0, input.texCoord));" \
                    L"}";

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
                depthStateInformation.enable = true;
                depthStateInformation.comparisonFunction = Video::ComparisonFunction::LessEqual;
                depthStateInformation.writeMask = Video::DepthStateInformation::Write::Zero;
                depthState = device->createDepthState(depthStateInformation);

                uint8_t *pixels = nullptr;
                int32_t fontWidth = 0, fontHeight = 0;
                imGuiIo.Fonts->GetTexDataAsRGBA32(&pixels, &fontWidth, &fontHeight);
                font = device->createTexture(Video::Format::R8G8B8A8_UNORM, fontWidth, fontHeight, 1, 1, Video::TextureFlags::Resource, pixels);
                imGuiIo.Fonts->TexID = (Video::Object *)font.get();

                Video::SamplerStateInformation samplerStateInformation;
                samplerStateInformation.filterMode = Video::SamplerStateInformation::FilterMode::AllLinear;
                samplerStateInformation.addressModeU = Video::SamplerStateInformation::AddressMode::Wrap;
                samplerStateInformation.addressModeV = Video::SamplerStateInformation::AddressMode::Wrap;
                samplerStateInformation.addressModeW = Video::SamplerStateInformation::AddressMode::Wrap;
                samplerState = device->createSamplerState(samplerStateInformation);

                imGuiIo.UserData = this;
                imGuiIo.RenderDrawListsFn = [](ImDrawData *drawData)
                {
                    ImGuiIO &imGuiIo = ImGui::GetIO();
                    Core *core = static_cast<Core *>(imGuiIo.UserData);
                    core->renderDrawData(drawData);
                };

#ifndef HID_USAGE_PAGE_GENERIC
#define HID_USAGE_PAGE_GENERIC         ((USHORT) 0x01)
#endif

#ifndef HID_USAGE_GENERIC_MOUSE
#define HID_USAGE_GENERIC_MOUSE        ((USHORT) 0x02)
#endif

                RAWINPUTDEVICE inputDevice;
                inputDevice.usUsagePage = HID_USAGE_PAGE_GENERIC;
                inputDevice.usUsage = HID_USAGE_GENERIC_MOUSE;
                inputDevice.dwFlags = 0;
                inputDevice.hwndTarget = window;
                RegisterRawInputDevices(&inputDevice, 1, sizeof(RAWINPUTDEVICE));

                windowActive = true;
                engineRunning = true;
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
                Xml::save(configuration, getContext()->getFileName(L"config.xml"));
                CoUninitialize();
            }

            void addAction(WPARAM wParam, bool state)
            {
                switch (wParam)
                {
                case 'W':
                case VK_UP:
                    actionQueue.push(Action(L"move_forward", state));
                    break;

                case 'S':
                case VK_DOWN:
                    actionQueue.push(Action(L"move_backward", state));
                    break;

                case 'A':
                case VK_LEFT:
                    actionQueue.push(Action(L"strafe_left", state));
                    break;

                case 'D':
                case VK_RIGHT:
                    actionQueue.push(Action(L"strafe_right", state));
                    break;

                case VK_SPACE:
                    actionQueue.push(Action(L"jump", state));
                    break;

                case VK_LCONTROL:
                    actionQueue.push(Action(L"crouch", state));
                    break;
                };
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
                ImGuiIO &imGuiIo = ImGui::GetIO();
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

                case WM_SIZE:
                    device->resize();
                    return 1;

                case WM_LBUTTONDOWN:
                    imGuiIo.MouseDown[0] = true;
                    return true;

                case WM_LBUTTONUP:
                    imGuiIo.MouseDown[0] = false;
                    return true;

                case WM_RBUTTONDOWN:
                    imGuiIo.MouseDown[1] = true;
                    return true;

                case WM_RBUTTONUP:
                    imGuiIo.MouseDown[1] = false;
                    return true;

                case WM_MBUTTONDOWN:
                    imGuiIo.MouseDown[2] = true;
                    return true;

                case WM_MBUTTONUP:
                    imGuiIo.MouseDown[2] = false;
                    return true;

                case WM_MOUSEWHEEL:
                    imGuiIo.MouseWheel += GET_WHEEL_DELTA_WPARAM(wParam) > 0 ? +1.0f : -1.0f;
                    return true;

                case WM_MOUSEMOVE:
                    imGuiIo.MousePos.x = (int16_t)(lParam);
                    imGuiIo.MousePos.y = (int16_t)(lParam >> 16);
                    return true;

                case WM_KEYDOWN:
                    addAction(wParam, true);
                    if (wParam < 256)
                    {
                        imGuiIo.KeysDown[wParam] = 1;
                    }

                    return true;

                case WM_KEYUP:
                    addAction(wParam, false);
                    if (wParam < 256)
                    {
                        imGuiIo.KeysDown[wParam] = 0;
                    }

                    return true;

                case WM_CHAR:
                    // You can also use ToAscii()+GetKeyboardState() to retrieve characters.
                    if (wParam > 0 && wParam < 0x10000)
                    {
                        imGuiIo.AddInputCharacter((uint16_t)wParam);
                    }

                    return true;

                case WM_INPUT:
                    if (true)
                    {
                        UINT inputSize = 40;
                        static BYTE rawInputBuffer[40];
                        GetRawInputData((HRAWINPUT)lParam, RID_INPUT, rawInputBuffer, &inputSize, sizeof(RAWINPUTHEADER));

                        RAWINPUT *rawInput = (RAWINPUT*)rawInputBuffer;
                        if (rawInput->header.dwType == RIM_TYPEMOUSE)
                        {
                            float xMovement = (float(rawInput->data.mouse.lLastX) * mouseSensitivity);
                            float yMovement = (float(rawInput->data.mouse.lLastY) * mouseSensitivity);
                            actionQueue.push(Action(L"turn", xMovement));
                            actionQueue.push(Action(L"tilt", yMovement));
                        }

                        break;
                    }
                };

                return 0;
            }

            bool update(void)
            {
                ImGuiIO &imGuiIo = ImGui::GetIO();

                auto backBuffer = device->getBackBuffer();
                uint32_t width = backBuffer->getWidth();
                uint32_t height = backBuffer->getHeight();
                imGuiIo.DisplaySize = ImVec2(float(width), float(height));

                imGuiIo.DeltaTime = float(timer.getUpdateTime());

                // Read keyboard modifiers inputs
                imGuiIo.KeyCtrl = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
                imGuiIo.KeyShift = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
                imGuiIo.KeyAlt = (GetKeyState(VK_MENU) & 0x8000) != 0;
                imGuiIo.KeySuper = false;
                // imGuiIo.KeysDown : filled by WM_KEYDOWN/WM_KEYUP events
                // imGuiIo.MousePos : filled by WM_MOUSEMOVE events
                // imGuiIo.MouseDown : filled by WM_*BUTTON* events
                // imGuiIo.MouseWheel : filled by WM_MOUSEWHEEL events

                // Hide OS mouse cursor if ImGui is drawing it
                SetCursor(imGuiIo.MouseDrawCursor ? NULL : LoadCursor(NULL, IDC_ARROW));

                // Start the frame
                ImGui::NewFrame();

                if (ImGui::BeginMainMenuBar())
                {
                    if (ImGui::BeginMenu("Windows"))
                    {
                        if (ImGui::MenuItem("Debug", "D", nullptr))
                        {
                            showDebugMenu = true;
                        }

                        if (ImGui::MenuItem("Editor", "E", nullptr))
                        {
                        }

                        ImGui::EndMenu();
                    }

                    ImGui::EndMainMenuBar();
                }

                if (showDebugMenu)
                {
                    ImGui::Begin("Debug Menu", &showDebugMenu, ImVec2(0, 0), -1.0f, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysUseWindowPadding);

                    ImGui::PushItemWidth(-1.0f);
                    ImGui::Text("%.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
                    ImGui::PopItemWidth();

                    ImGui::NewLine();
                    ImGui::InputText("##Load Level", loadLevelName, 256);
                    ImGui::SameLine();
                    if (ImGui::Button("Load"))
                    {
                        population->load(String(loadLevelName));
                    }

                    ImGui::NewLine();
                    ImGui::PushItemWidth(-1.0f);
                    if (ImGui::ListBox("##Resolution", &currentDisplayMode, [](void *data, int index, const char **text) -> bool
                    {
                        Core *core = static_cast<Core *>(data);
                        auto &mode = core->displayModeStringList[index];
                        (*text) = mode.c_str();
                        return true;
                    }, this, displayModeStringList.size(), 5))
                    {
                        device->setDisplayMode(currentDisplayMode);
                        auto &displayNode = configuration.getChild(L"display");
                        displayNode.attributes[L"mode"] = currentDisplayMode;
                        sendShout(&Plugin::CoreListener::onConfigurationChanged);
                        sendShout(&Plugin::CoreListener::onResize);
                    }

                    ImGui::PopItemWidth();
                    if (ImGui::Checkbox("FullScreen", &fullScreen))
                    {
                        device->setFullScreen(fullScreen);
                        auto &displayNode = configuration.getChild(L"display");
                        displayNode.attributes[L"fullscreen"] = fullScreen;
                        sendShout(&Plugin::CoreListener::onConfigurationChanged);
                        sendShout(&Plugin::CoreListener::onResize);
                    }

                    ImGui::NewLine();
                    if (ImGui::Button("Quit"))
                    {
                        engineRunning = false;
                    }

                    ImGui::End();
                }

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
                    if (state == State::Active)
                    {
                        Action action;
                        while (actionQueue.try_pop(action))
                        {
                            sendShout(&Plugin::CoreListener::onAction, action.name, action.parameter);
                        };
                    }
                    else
                    {
                        actionQueue.clear();
                    }
                }
                else if (order == 100)
                {
                    ImGui::Render();
                    device->present(false);
                }
            }

            ImGuiContext *getDefaultUIContext(void) const
            {
                return ImGui::GetCurrentContext();
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
                for (uint32_t commandListIndex = 0; commandListIndex < drawData->CmdListsCount; ++commandListIndex)
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

                uint32_t vertexOffset = 0;
                uint32_t indexOffset = 0;
                for (uint32_t commandListIndex = 0; commandListIndex < drawData->CmdListsCount; ++commandListIndex)
                {
                    const ImDrawList* commandList = drawData->CmdLists[commandListIndex];
                    for (uint32_t commandIndex = 0; commandIndex < commandList->CmdBuffer.Size; ++commandIndex)
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
