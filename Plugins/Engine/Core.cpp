
#include "GEK/Engine/Core.hpp"
#include "API/Engine/Processor.hpp"
#include "API/Engine/Visualizer.hpp"
#include "GEK/Engine/Population.hpp"
#include "GEK/Engine/Resources.hpp"
#include "GEK/GUI/Utilities.hpp"
#include "GEK/Utility/ContextUser.hpp"
#include "GEK/Utility/FileSystem.hpp"
#include "GEK/Utility/String.hpp"
#include "GEK/Utility/Timer.hpp"
#include <algorithm>
#include <array>
#include <atomic>
#include <cfloat>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <imgui_internal.h>
#include <limits>
#include <numeric>
#include <thread>
#include <unordered_map>
#include <vector>

#ifdef _WIN32
#include <Windows.h>
#endif

namespace Gek
{
    namespace Implementation
    {
        ImGuiKey GetImGuiKey(Window::Key key)
        {
            static std::unordered_map<Window::Key, ImGuiKey> windowKeyToImGuiKey = {
                { Window::Key::A, ImGuiKey_A },
                { Window::Key::B, ImGuiKey_B },
                { Window::Key::C, ImGuiKey_C },
                { Window::Key::D, ImGuiKey_D },
                { Window::Key::E, ImGuiKey_E },
                { Window::Key::F, ImGuiKey_F },
                { Window::Key::G, ImGuiKey_G },
                { Window::Key::H, ImGuiKey_H },
                { Window::Key::I, ImGuiKey_I },
                { Window::Key::J, ImGuiKey_J },
                { Window::Key::K, ImGuiKey_K },
                { Window::Key::L, ImGuiKey_L },
                { Window::Key::M, ImGuiKey_M },
                { Window::Key::N, ImGuiKey_N },
                { Window::Key::O, ImGuiKey_O },
                { Window::Key::P, ImGuiKey_P },
                { Window::Key::Q, ImGuiKey_Q },
                { Window::Key::R, ImGuiKey_R },
                { Window::Key::S, ImGuiKey_S },
                { Window::Key::T, ImGuiKey_T },
                { Window::Key::U, ImGuiKey_U },
                { Window::Key::V, ImGuiKey_V },
                { Window::Key::W, ImGuiKey_W },
                { Window::Key::X, ImGuiKey_X },
                { Window::Key::Y, ImGuiKey_Y },
                { Window::Key::Z, ImGuiKey_Z },
                { Window::Key::Key1, ImGuiKey_1 },
                { Window::Key::Key2, ImGuiKey_2 },
                { Window::Key::Key3, ImGuiKey_3 },
                { Window::Key::Key4, ImGuiKey_4 },
                { Window::Key::Key5, ImGuiKey_5 },
                { Window::Key::Key6, ImGuiKey_6 },
                { Window::Key::Key7, ImGuiKey_7 },
                { Window::Key::Key8, ImGuiKey_8 },
                { Window::Key::Key9, ImGuiKey_9 },
                { Window::Key::Key0, ImGuiKey_0 },
                { Window::Key::Return, ImGuiKey_Enter },
                { Window::Key::Escape, ImGuiKey_Escape },
                { Window::Key::Backspace, ImGuiKey_Backspace },
                { Window::Key::Tab, ImGuiKey_Tab },
                { Window::Key::Space, ImGuiKey_Space },
                { Window::Key::Minus, ImGuiKey_Minus },
                { Window::Key::Equals, ImGuiKey_Equal },
                { Window::Key::LeftBracket, ImGuiKey_LeftBracket },
                { Window::Key::RightBracket, ImGuiKey_RightBracket },
                { Window::Key::Backslash, ImGuiKey_Backslash },
                { Window::Key::Semicolon, ImGuiKey_Semicolon },
                { Window::Key::Quote, ImGuiKey_Apostrophe },
                { Window::Key::Grave, ImGuiKey_GraveAccent },
                { Window::Key::Comma, ImGuiKey_Comma },
                { Window::Key::Period, ImGuiKey_Period },
                { Window::Key::Slash, ImGuiKey_Slash },
                { Window::Key::CapsLock, ImGuiKey_CapsLock },
                { Window::Key::F1, ImGuiKey_F1 },
                { Window::Key::F2, ImGuiKey_F2 },
                { Window::Key::F3, ImGuiKey_F3 },
                { Window::Key::F4, ImGuiKey_F4 },
                { Window::Key::F5, ImGuiKey_F5 },
                { Window::Key::F6, ImGuiKey_F6 },
                { Window::Key::F7, ImGuiKey_F7 },
                { Window::Key::F8, ImGuiKey_F8 },
                { Window::Key::F9, ImGuiKey_F9 },
                { Window::Key::F10, ImGuiKey_F10 },
                { Window::Key::F11, ImGuiKey_F11 },
                { Window::Key::F12, ImGuiKey_F12 },
                { Window::Key::PrintScreen, ImGuiKey_PrintScreen },
                { Window::Key::ScrollLock, ImGuiKey_ScrollLock },
                { Window::Key::Pause, ImGuiKey_Pause },
                { Window::Key::Insert, ImGuiKey_Insert },
                { Window::Key::Delete, ImGuiKey_Delete },
                { Window::Key::Home, ImGuiKey_Home },
                { Window::Key::End, ImGuiKey_End },
                { Window::Key::PageUp, ImGuiKey_PageUp },
                { Window::Key::PageDown, ImGuiKey_PageDown },
                { Window::Key::Right, ImGuiKey_RightArrow },
                { Window::Key::Left, ImGuiKey_LeftArrow },
                { Window::Key::Down, ImGuiKey_DownArrow },
                { Window::Key::Up, ImGuiKey_UpArrow },
                { Window::Key::KeyPadNumLock, ImGuiKey_NumLock },
                { Window::Key::KeyPadDivide, ImGuiKey_KeypadDivide },
                { Window::Key::KeyPadMultiply, ImGuiKey_KeypadMultiply },
                { Window::Key::KeyPadSubtract, ImGuiKey_KeypadSubtract },
                { Window::Key::KeyPadAdd, ImGuiKey_KeypadAdd },
                { Window::Key::KeyPad1, ImGuiKey_Keypad1 },
                { Window::Key::KeyPad2, ImGuiKey_Keypad2 },
                { Window::Key::KeyPad3, ImGuiKey_Keypad3 },
                { Window::Key::KeyPad4, ImGuiKey_Keypad4 },
                { Window::Key::KeyPad5, ImGuiKey_Keypad5 },
                { Window::Key::KeyPad6, ImGuiKey_Keypad6 },
                { Window::Key::KeyPad7, ImGuiKey_Keypad7 },
                { Window::Key::KeyPad8, ImGuiKey_Keypad8 },
                { Window::Key::KeyPad9, ImGuiKey_Keypad9 },
                { Window::Key::KeyPad0, ImGuiKey_Keypad0 },
                { Window::Key::KeyPadPoint, ImGuiKey_KeypadDecimal },
                { Window::Key::LeftControl, ImGuiKey_LeftCtrl },
                { Window::Key::LeftShift, ImGuiKey_LeftShift },
                { Window::Key::LeftAlt, ImGuiKey_LeftAlt },
                { Window::Key::RightControl, ImGuiKey_RightCtrl },
                { Window::Key::RightShift, ImGuiKey_RightShift },
                { Window::Key::RightAlt, ImGuiKey_RightAlt },
            };

            return windowKeyToImGuiKey[key];
        }

        bool IsKeyDown(Window::Key key)
        {
            ImGuiKey imguiKey = GetImGuiKey(key);
            ImGuiIO &imGuiIo = ImGui::GetIO();
            return ImGui::IsKeyDown(imguiKey);
        }

        GEK_CONTEXT_USER_BASE(Core)
        , virtual Engine::Core
        {
          private:
            Window::DevicePtr window;
            bool windowActive = false;

            JSON::Object configuration;
            JSON::Object shadersSettings;
            JSON::Object filtersSettings;
            bool changedVisualOptions = false;

            Render::DisplayModeList displayModeList;
            std::vector<std::string> displayModeStringList;
            struct Display
            {
                int mode = -1;
                bool fullScreen = false;
            } current, previous, next;

            bool showResetDialog = false;

            bool showLoadMenu = false;
            std::vector<std::string> scenes;
            uint32_t currentSelectedScene = 0;
            bool pendingPopulationLoad = false;
            std::string pendingPopulationName;

            bool showSettings = false;
            bool showRuntimeDiagnostics = false;

            struct RuntimeMetricPlot
            {
                const char *key = nullptr;
                const char *label = nullptr;
                ImVec4 color = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
                std::array<float, 240> samples = {};
                size_t sampleCount = 0;
            };

            std::array<RuntimeMetricPlot, 71> runtimeMetricPlots = { {
                { "render.fpsInstant", "FPS (Instant)", ImVec4(0.95f, 0.82f, 0.26f, 1.0f) },
                { "render.fpsSmoothed", "FPS (Smoothed)", ImVec4(0.95f, 0.62f, 0.20f, 1.0f) },
                { "render.frameTimeMs", "Frame CPU (ms)", ImVec4(0.88f, 0.88f, 0.30f, 1.0f) },
                { "render.presentCpuMs", "Present CPU (ms)", ImVec4(0.75f, 0.60f, 0.95f, 1.0f) },
                { "vulkan.waitFenceCpuMs", "Vulkan Wait Fence (ms)", ImVec4(0.95f, 0.28f, 0.28f, 1.0f) },
                { "vulkan.recordCpuMs", "Vulkan Record (ms)", ImVec4(0.95f, 0.45f, 0.22f, 1.0f) },
                { "vulkan.acquireCpuMs", "Vulkan Acquire (ms)", ImVec4(0.95f, 0.58f, 0.24f, 1.0f) },
                { "vulkan.frameCpuMs", "Vulkan Frame CPU (ms)", ImVec4(0.95f, 0.72f, 0.30f, 1.0f) },
                { "vulkan.cleanupCpuMs", "Vulkan Cleanup (ms)", ImVec4(0.95f, 0.40f, 0.56f, 1.0f) },
                { "vulkan.cleanupDescriptorCpuMs", "Vulkan Cleanup Descriptor (ms)", ImVec4(0.95f, 0.50f, 0.62f, 1.0f) },
                { "vulkan.cleanupFramebufferCpuMs", "Vulkan Cleanup Framebuffer (ms)", ImVec4(0.95f, 0.62f, 0.72f, 1.0f) },
                { "vulkan.drawCommandLockCpuMs", "Vulkan Draw Lock (ms)", ImVec4(0.92f, 0.52f, 0.72f, 1.0f) },
                { "vulkan.untrackedFrameCpuMs", "Vulkan Untracked (ms)", ImVec4(0.82f, 0.62f, 0.90f, 1.0f) },
                { "vulkan.uiSnapshotCreated", "Vulkan Snapshots Created", ImVec4(0.96f, 0.70f, 0.24f, 1.0f) },
                { "vulkan.uiSnapshotReused", "Vulkan Snapshots Reused", ImVec4(0.88f, 0.78f, 0.22f, 1.0f) },
                { "vulkan.uiSnapshotBytes", "Vulkan Snapshot Bytes", ImVec4(0.96f, 0.52f, 0.22f, 1.0f) },
                { "vulkan.uiSnapshotPending", "Vulkan Snapshot Pending", ImVec4(0.92f, 0.62f, 0.30f, 1.0f) },
                { "vulkan.uiSnapshotInFlight", "Vulkan Snapshot In-Flight", ImVec4(0.88f, 0.72f, 0.38f, 1.0f) },
                { "vulkan.constantBufferVersioningEnabled", "Vulkan CB Versioning Enabled", ImVec4(0.92f, 0.83f, 0.26f, 1.0f) },
                { "vulkan.vertexBufferVersioningEnabled", "Vulkan VB Versioning Enabled", ImVec4(0.84f, 0.83f, 0.28f, 1.0f) },
                { "vulkan.indexBufferVersioningEnabled", "Vulkan IB Versioning Enabled", ImVec4(0.76f, 0.83f, 0.30f, 1.0f) },
                { "vulkan.constantBufferRingSize", "Vulkan CB Ring Size", ImVec4(0.92f, 0.66f, 0.30f, 1.0f) },
                { "vulkan.vertexBufferRingSize", "Vulkan VB Ring Size", ImVec4(0.84f, 0.66f, 0.34f, 1.0f) },
                { "vulkan.indexBufferRingSize", "Vulkan IB Ring Size", ImVec4(0.76f, 0.66f, 0.38f, 1.0f) },
                { "vulkan.constantVersionRotations", "Vulkan CB Rotations", ImVec4(0.95f, 0.40f, 0.26f, 1.0f) },
                { "vulkan.constantVersionExhaustions", "Vulkan CB Exhaustions", ImVec4(0.95f, 0.28f, 0.28f, 1.0f) },
                { "vulkan.constantVersionWaitRecoveries", "Vulkan CB Wait Recoveries", ImVec4(0.95f, 0.22f, 0.22f, 1.0f) },
                { "vulkan.constantVersionWriteAttempts", "Vulkan CB Write Attempts", ImVec4(0.95f, 0.56f, 0.30f, 1.0f) },
                { "vulkan.constantVersionExhaustionsNoFence", "Vulkan CB Exhaustions (No Fence)", ImVec4(0.95f, 0.44f, 0.44f, 1.0f) },
                { "vulkan.constantVersionWaitAttempts", "Vulkan CB Wait Attempts", ImVec4(0.95f, 0.50f, 0.38f, 1.0f) },
                { "vulkan.constantVersionPostWaitExhaustions", "Vulkan CB Post-Wait Exhaustions", ImVec4(0.95f, 0.38f, 0.48f, 1.0f) },
                { "vulkan.vertexVersionRotations", "Vulkan VB Rotations", ImVec4(0.62f, 0.48f, 0.95f, 1.0f) },
                { "vulkan.vertexVersionExhaustions", "Vulkan VB Exhaustions", ImVec4(0.54f, 0.38f, 0.95f, 1.0f) },
                { "vulkan.vertexVersionWaitRecoveries", "Vulkan VB Wait Recoveries", ImVec4(0.46f, 0.30f, 0.95f, 1.0f) },
                { "vulkan.vertexVersionWriteAttempts", "Vulkan VB Write Attempts", ImVec4(0.70f, 0.56f, 0.95f, 1.0f) },
                { "vulkan.vertexVersionExhaustionsNoFence", "Vulkan VB Exhaustions (No Fence)", ImVec4(0.66f, 0.50f, 0.95f, 1.0f) },
                { "vulkan.vertexVersionWaitAttempts", "Vulkan VB Wait Attempts", ImVec4(0.74f, 0.58f, 0.95f, 1.0f) },
                { "vulkan.vertexVersionPostWaitExhaustions", "Vulkan VB Post-Wait Exhaustions", ImVec4(0.62f, 0.52f, 0.95f, 1.0f) },
                { "vulkan.indexVersionRotations", "Vulkan IB Rotations", ImVec4(0.36f, 0.76f, 0.62f, 1.0f) },
                { "vulkan.indexVersionExhaustions", "Vulkan IB Exhaustions", ImVec4(0.30f, 0.70f, 0.56f, 1.0f) },
                { "vulkan.indexVersionWaitRecoveries", "Vulkan IB Wait Recoveries", ImVec4(0.24f, 0.64f, 0.50f, 1.0f) },
                { "vulkan.indexVersionWriteAttempts", "Vulkan IB Write Attempts", ImVec4(0.44f, 0.84f, 0.66f, 1.0f) },
                { "vulkan.indexVersionExhaustionsNoFence", "Vulkan IB Exhaustions (No Fence)", ImVec4(0.52f, 0.90f, 0.74f, 1.0f) },
                { "vulkan.indexVersionWaitAttempts", "Vulkan IB Wait Attempts", ImVec4(0.60f, 0.92f, 0.78f, 1.0f) },
                { "vulkan.indexVersionPostWaitExhaustions", "Vulkan IB Post-Wait Exhaustions", ImVec4(0.48f, 0.86f, 0.70f, 1.0f) },
                { "vulkan.constantVersionRotationsTotal", "Vulkan CB Rotations (Total)", ImVec4(0.95f, 0.46f, 0.34f, 1.0f) },
                { "vulkan.constantVersionExhaustionsTotal", "Vulkan CB Exhaustions (Total)", ImVec4(0.95f, 0.36f, 0.36f, 1.0f) },
                { "vulkan.constantVersionWaitRecoveriesTotal", "Vulkan CB Wait Recoveries (Total)", ImVec4(0.95f, 0.30f, 0.30f, 1.0f) },
                { "vulkan.constantVersionWriteAttemptsTotal", "Vulkan CB Write Attempts (Total)", ImVec4(0.95f, 0.62f, 0.40f, 1.0f) },
                { "vulkan.constantVersionExhaustionsNoFenceTotal", "Vulkan CB Exhaustions (No Fence, Total)", ImVec4(0.95f, 0.52f, 0.52f, 1.0f) },
                { "vulkan.constantVersionWaitAttemptsTotal", "Vulkan CB Wait Attempts (Total)", ImVec4(0.95f, 0.58f, 0.48f, 1.0f) },
                { "vulkan.constantVersionPostWaitExhaustionsTotal", "Vulkan CB Post-Wait Exhaustions (Total)", ImVec4(0.95f, 0.46f, 0.58f, 1.0f) },
                { "vulkan.vertexVersionRotationsTotal", "Vulkan VB Rotations (Total)", ImVec4(0.70f, 0.56f, 0.95f, 1.0f) },
                { "vulkan.vertexVersionExhaustionsTotal", "Vulkan VB Exhaustions (Total)", ImVec4(0.62f, 0.46f, 0.95f, 1.0f) },
                { "vulkan.vertexVersionWaitRecoveriesTotal", "Vulkan VB Wait Recoveries (Total)", ImVec4(0.54f, 0.38f, 0.95f, 1.0f) },
                { "vulkan.vertexVersionWriteAttemptsTotal", "Vulkan VB Write Attempts (Total)", ImVec4(0.78f, 0.64f, 0.95f, 1.0f) },
                { "vulkan.vertexVersionExhaustionsNoFenceTotal", "Vulkan VB Exhaustions (No Fence, Total)", ImVec4(0.74f, 0.60f, 0.95f, 1.0f) },
                { "vulkan.vertexVersionWaitAttemptsTotal", "Vulkan VB Wait Attempts (Total)", ImVec4(0.82f, 0.68f, 0.95f, 1.0f) },
                { "vulkan.vertexVersionPostWaitExhaustionsTotal", "Vulkan VB Post-Wait Exhaustions (Total)", ImVec4(0.70f, 0.64f, 0.95f, 1.0f) },
                { "vulkan.indexVersionRotationsTotal", "Vulkan IB Rotations (Total)", ImVec4(0.46f, 0.84f, 0.70f, 1.0f) },
                { "vulkan.indexVersionExhaustionsTotal", "Vulkan IB Exhaustions (Total)", ImVec4(0.38f, 0.78f, 0.62f, 1.0f) },
                { "vulkan.indexVersionWaitRecoveriesTotal", "Vulkan IB Wait Recoveries (Total)", ImVec4(0.32f, 0.72f, 0.56f, 1.0f) },
                { "vulkan.indexVersionWriteAttemptsTotal", "Vulkan IB Write Attempts (Total)", ImVec4(0.54f, 0.88f, 0.74f, 1.0f) },
                { "vulkan.indexVersionExhaustionsNoFenceTotal", "Vulkan IB Exhaustions (No Fence, Total)", ImVec4(0.62f, 0.94f, 0.82f, 1.0f) },
                { "vulkan.indexVersionWaitAttemptsTotal", "Vulkan IB Wait Attempts (Total)", ImVec4(0.70f, 0.96f, 0.86f, 1.0f) },
                { "vulkan.indexVersionPostWaitExhaustionsTotal", "Vulkan IB Post-Wait Exhaustions (Total)", ImVec4(0.58f, 0.90f, 0.78f, 1.0f) },
                { "d3d11.presentCpuMs", "D3D11 Present (ms)", ImVec4(0.58f, 0.78f, 0.95f, 1.0f) },
                { "render.totalCommands", "Render Total Commands", ImVec4(0.55f, 0.42f, 0.92f, 1.0f) },
                { "render.sceneDraws", "Render Scene Draws", ImVec4(0.35f, 0.50f, 0.95f, 1.0f) },
                { "visualizer.queuedDrawCalls", "Queued Draw Calls", ImVec4(0.95f, 0.30f, 0.30f, 1.0f) },
                { "model.visibleModels", "Visible Models", ImVec4(0.40f, 0.78f, 0.33f, 1.0f) },
            } };
            std::array<bool, 71> runtimeMetricVisible = []() -> std::array<bool, 71>
            {
                std::array<bool, 71> initialVisibility{};
                initialVisibility.fill(true);
                return initialVisibility;
            }();
            bool runtimeMetricViewAll = true;
            uint64_t runtimeMetricsLastFrame = 0;
            std::array<char, 260> runtimeLogFilePath = {};
            bool runtimeLogFileAppend = true;
            double runtimeFpsSmoothed = 0.0;
            bool runtimeFpsSmoothedInitialized = false;
            bool runtimeEnableVulkanCharts = false;
            bool runtimeDumpMetricsOnExit = true;
            bool runtimeDumpMetricsCsvFormat = true;

            bool showModeChange = false;
            float modeChangeTimer = 0.0f;
            bool shutdownIssued = false;
            bool ignoredFirstCloseRequest = false;

            Timer timer;
            float mouseSensitivity = 0.5f;
            bool enableInterfaceControl = true;

            std::string renderDeviceName;
            Render::DevicePtr renderDevice;
            Plugin::VisualizerPtr visualizer;
            Engine::ResourcesPtr resources;
            std::vector<Plugin::ProcessorPtr> processorList;
            Engine::PopulationPtr population;

            std::atomic<int32_t> pendingMouseXMovement = 0;
            std::atomic<int32_t> pendingMouseYMovement = 0;
            std::atomic<bool> populationLoadCompleted = false;

            static constexpr int32_t MaxMouseEventDelta = 4096;
            static constexpr int32_t MaxPendingMouseDelta = 32768;
            static constexpr int32_t MaxAppliedMouseDeltaPerFrame = 512;

            void accumulateMouseMovement(std::atomic<int32_t> & pendingMovement, int32_t delta)
            {
                if (delta < -MaxMouseEventDelta || delta > MaxMouseEventDelta)
                {
                    delta = std::clamp(delta, -MaxMouseEventDelta, MaxMouseEventDelta);
                }

                int32_t current = pendingMovement.load(std::memory_order_relaxed);
                while (true)
                {
                    int32_t next = std::clamp(current + delta, -MaxPendingMouseDelta, MaxPendingMouseDelta);
                    if (pendingMovement.compare_exchange_weak(current, next, std::memory_order_relaxed, std::memory_order_relaxed))
                    {
                        break;
                    }
                }
            }

            void logRuntimeMetricSnapshot(std::unordered_map<std::string, double> const &runtimeMetrics, bool visibleOnly, char const *reason)
            {
                std::vector<std::pair<std::string, double>> metricsToLog;
                metricsToLog.reserve(visibleOnly ? runtimeMetricPlots.size() : runtimeMetrics.size());

                if (visibleOnly)
                {
                    for (size_t plotIndex = 0; plotIndex < runtimeMetricPlots.size(); ++plotIndex)
                    {
                        if (!runtimeMetricVisible[plotIndex])
                        {
                            continue;
                        }

                        auto const &plot = runtimeMetricPlots[plotIndex];
                        if (!plot.key)
                        {
                            continue;
                        }

                        auto metricSearch = runtimeMetrics.find(plot.key);
                        if (metricSearch != std::end(runtimeMetrics))
                        {
                            metricsToLog.emplace_back(metricSearch->first, metricSearch->second);
                        }
                    }
                }
                else
                {
                    for (auto const &[metricKey, metricValue] : runtimeMetrics)
                    {
                        metricsToLog.emplace_back(metricKey, metricValue);
                    }
                }

                std::sort(std::begin(metricsToLog), std::end(metricsToLog), [](auto const &left, auto const &right) -> bool
                          { return left.first < right.first; });

                getContext()->log(
                    Context::Info,
                    "Runtime metric snapshot [{}] - {} metrics",
                    (reason ? reason : "unspecified"),
                    metricsToLog.size());

                for (auto const &[metricKey, metricValue] : metricsToLog)
                {
                    if (runtimeDumpMetricsCsvFormat)
                    {
                        getContext()->log(Context::Info, "RUNTIME_METRIC_CSV,{},{},{:.6f}", (reason ? reason : "unspecified"), metricKey, metricValue);
                    }
                    else
                    {
                        getContext()->log(Context::Info, "RUNTIME_METRIC {}={:.6f}", metricKey, metricValue);
                    }
                }
            }

            void logRuntimeMetricSnapshot(bool visibleOnly, char const *reason)
            {
                auto runtimeMetrics = getContext()->getRuntimeMetricSnapshot();
                logRuntimeMetricSnapshot(runtimeMetrics, visibleOnly, reason);
            }

            bool loadingPopulation = false;

          public:
            Core(Context * context)
                : ContextRegistration(context)
            {
#ifdef _WIN32
                HRESULT resultValue = CoInitializeEx(nullptr, COINITBASE_MULTITHREADED);
                if (FAILED(resultValue))
                {
                    getContext()->log(Context::Error, "Call to CoInitialize failed: {}", static_cast<uint32_t>(resultValue));
                    return;
                }
#endif
                configuration = JSON::Load(getContext()->findDataPath("config.json"s));

                auto devicePlugin = Plugin::Core::getOption("render", "device", "renderd3d11"s);
                Gek::String::Replace(devicePlugin, "render", "");
                renderDeviceName = devicePlugin;

                if (auto environmentLogFile = std::getenv("gek_log_file"); environmentLogFile && environmentLogFile[0])
                {
                    std::snprintf(runtimeLogFilePath.data(), runtimeLogFilePath.size(), "%s", environmentLogFile);
                }
                else
                {
                    std::snprintf(runtimeLogFilePath.data(), runtimeLogFilePath.size(), "%s", "gek.log");
                }

                getContext()->log(Context::Info, "Starting GEK Engine");

                const uint32_t defaultSinkMask = static_cast<uint32_t>(getContext()->getLogSinkMask());
                uint32_t configuredSinkMask = JSON::Value(getOption("logging", "sinkMask"), defaultSinkMask);
                configuredSinkMask &= static_cast<uint32_t>(Context::LogSink_Console | Context::LogSink_Debugger | Context::LogSink_File);
                getContext()->setLogSinkMask(static_cast<uint8_t>(configuredSinkMask));

                runtimeLogFileAppend = JSON::Value(getOption("logging", "appendFile"), true);
                runtimeDumpMetricsOnExit = JSON::Value(getOption("logging", "dumpRuntimeMetricsOnExit"), true);
                runtimeDumpMetricsCsvFormat = JSON::Value(getOption("logging", "dumpRuntimeMetricsCsvFormat"), true);
                std::string configuredLogFilePath = JSON::Value(getOption("logging", "filePath"), std::string(runtimeLogFilePath.data()));
                if (!configuredLogFilePath.empty())
                {
                    std::snprintf(runtimeLogFilePath.data(), runtimeLogFilePath.size(), "%s", configuredLogFilePath.c_str());
                }

                if ((configuredSinkMask & Context::LogSink_File) != 0 && runtimeLogFilePath[0] != '\0')
                {
                    getContext()->setLogFilePath(FileSystem::Path(runtimeLogFilePath.data()), runtimeLogFileAppend);
                }

                // Use the selected window handler module (class name can be mapped as needed)
                window = getContext()->createClass<Window::Device>("Default::System::Window");
                if (window)
                {
                    window->onCreated.connect(this, &Core::onWindowCreated);
                    window->onCloseRequested.connect(this, &Core::onCloseRequested);
                    window->onActivate.connect(this, &Core::onWindowActivate);
                    window->onIdle.connect(this, &Core::onWindowIdle);
                    window->onSizeChanged.connect(this, &Core::onWindowSizeChanged);
                    window->onKeyPressed.connect(this, &Core::onKeyPressed);
                    window->onCharacter.connect(this, &Core::onCharacter);
                    window->onMouseClicked.connect(this, &Core::onMouseClicked);
                    window->onMouseWheel.connect(this, &Core::onMouseWheel);
                    window->onMousePosition.connect(this, &Core::onMousePosition);
                    window->onMouseMovement.connect(this, &Core::onMouseMovement);

                    Window::Description description;
                    description.allowResize = true;
                    description.className = "GEK_Engine_Demo";
                    description.windowName = "GEK Engine Demo";
                    window->create(description);
                }

                getContext()->log(Gek::Context::Info, "Exiting core application");
            }

            ~Core(void)
            {
                if (!shutdownIssued)
                {
                    shutdownIssued = true;
                    onShutdown.emit();
                }

                if (population)
                {
                    population->onLoad.disconnect(this, &Core::onPopulationLoaded);
                }

                if (runtimeDumpMetricsOnExit)
                {
                    logRuntimeMetricSnapshot(false, "shutdown-final");
                }

                if (window)
                {
                    window->onCreated.disconnect(this, &Core::onWindowCreated);
                    window->onCloseRequested.disconnect(this, &Core::onCloseRequested);
                    window->onActivate.disconnect(this, &Core::onWindowActivate);
                    window->onIdle.disconnect(this, &Core::onWindowIdle);
                    window->onSizeChanged.disconnect(this, &Core::onWindowSizeChanged);
                    window->onKeyPressed.disconnect(this, &Core::onKeyPressed);
                    window->onCharacter.disconnect(this, &Core::onCharacter);
                    window->onMouseClicked.disconnect(this, &Core::onMouseClicked);
                    window->onMouseWheel.disconnect(this, &Core::onMouseWheel);
                    window->onMousePosition.disconnect(this, &Core::onMousePosition);
                    window->onMouseMovement.disconnect(this, &Core::onMouseMovement);
                }

                processorList.clear();
                visualizer = nullptr;
                resources = nullptr;
                population = nullptr;
                renderDevice = nullptr;
                window = nullptr;

                // Always save config on shutdown, even after error
                JSON::Save(configuration, getContext()->getCachePath("config.json"s));
#ifdef _WIN32
                CoUninitialize();
#endif
            }

            bool setFullScreen(bool requestFullScreen)
            {
                if (current.fullScreen != requestFullScreen)
                {
                    current.fullScreen = requestFullScreen;
                    setOption("render"s, "fullScreen"s, requestFullScreen);
                    if (requestFullScreen)
                    {
                        window->move(Math::Int2::Zero);
                    }

                    renderDevice->setFullScreenState(requestFullScreen);
                    onChangedDisplay();
                    if (!requestFullScreen)
                    {
                        window->move();
                    }

                    return true;
                }

                return false;
            }

            bool setDisplayMode(uint32_t requestDisplayMode)
            {
                if (current.mode != requestDisplayMode)
                {
                    auto &displayModeData = displayModeList[requestDisplayMode];
                    if (requestDisplayMode < displayModeList.size())
                    {
                        next.mode = requestDisplayMode;
                        current.mode = requestDisplayMode;

                        auto deviceOptions = getOption("render", renderDeviceName);
                        deviceOptions["mode"] = requestDisplayMode;
                        setOption("render", renderDeviceName, deviceOptions);
                        renderDevice->setDisplayMode(displayModeData);
                        window->move();
                        onChangedDisplay();

                        return true;
                    }
                }

                return false;
            }

            void forceClose(void)
            {
                getContext()->log(Gek::Context::Warning, "Core::forceClose invoked");
                if (!shutdownIssued)
                {
                    shutdownIssued = true;
                    onShutdown.emit();
                }

                window->close();
            }

            void confirmClose(void)
            {
                bool isModified = false;
                listProcessors([&](Plugin::Processor *processor) -> void
                               {
                    auto castCheck = dynamic_cast<Edit::Events*>(processor);
                    if (castCheck)
                    {
                        isModified = castCheck->isModified();
                    } });

                if (isModified)
                {
                    showSaveModified = true;
                    closeOnModified = true;
                }
                else
                {
                    forceClose();
                }
            }

            // Window slots
            void onWindowCreated(void)
            {
                getContext()->log(Gek::Context::Info, "Window Created, Finishing Core Initialization");

                Render::Device::Description deviceDescription;
                auto clampVersionRingSize = [](int32_t requestedRingSize) -> uint8_t
                {
                    return static_cast<uint8_t>(std::clamp(
                        requestedRingSize,
                        0,
                        static_cast<int32_t>(Render::Device::Description::DefaultVersioningRingSize)));
                };

                deviceDescription.constantBufferVersioningPolicy.ringSize = clampVersionRingSize(
                    Plugin::Core::getOption("render", "vulkanConstantBufferRing", static_cast<int32_t>(deviceDescription.constantBufferVersioningPolicy.ringSize)));
                deviceDescription.vertexBufferVersioningPolicy.ringSize = clampVersionRingSize(
                    Plugin::Core::getOption("render", "vulkanVertexBufferRing", static_cast<int32_t>(deviceDescription.vertexBufferVersioningPolicy.ringSize)));
                deviceDescription.indexBufferVersioningPolicy.ringSize = clampVersionRingSize(
                    Plugin::Core::getOption("render", "vulkanIndexBufferRing", static_cast<int32_t>(deviceDescription.indexBufferVersioningPolicy.ringSize)));

                deviceDescription.constantBufferVersioningPolicy.mode =
                    (deviceDescription.constantBufferVersioningPolicy.ringSize >= 2) ? Render::BufferVersioningMode::FixedRing : Render::BufferVersioningMode::Disabled;
                deviceDescription.vertexBufferVersioningPolicy.mode =
                    (deviceDescription.vertexBufferVersioningPolicy.ringSize >= 2) ? Render::BufferVersioningMode::FixedRing : Render::BufferVersioningMode::Disabled;
                deviceDescription.indexBufferVersioningPolicy.mode =
                    (deviceDescription.indexBufferVersioningPolicy.ringSize >= 2) ? Render::BufferVersioningMode::FixedRing : Render::BufferVersioningMode::Disabled;

                // Use the selected render device module (class name can be mapped as needed)
                renderDevice = getContext()->createClass<Render::Device>("Default::Device::Video", window.get(), deviceDescription);

                uint32_t preferredDisplayMode = 0;
                auto fullDisplayModeList = renderDevice->getDisplayModeList(deviceDescription.displayFormat);
                if (!fullDisplayModeList.empty())
                {
                    for (auto const &displayMode : fullDisplayModeList)
                    {
                        if (displayMode.height >= 800)
                        {
                            displayModeList.push_back(displayMode);
                        }
                    }

                    for (auto const &displayMode : displayModeList)
                    {
                        auto currentDisplayMode = static_cast<uint32_t>(displayModeStringList.size());
                        std::string displayModeString(std::format("{}x{}, {}hz", displayMode.width, displayMode.height, uint32_t(std::ceil(float(displayMode.refreshRate.numerator) / float(displayMode.refreshRate.denominator)))));
                        switch (displayMode.aspectRatio)
                        {
                        case Render::DisplayMode::AspectRatio::_4x3:
                            displayModeString.append(" (4x3)");
                            break;

                        case Render::DisplayMode::AspectRatio::_16x9:
                            preferredDisplayMode = (preferredDisplayMode == 0 && displayMode.height > 800 ? currentDisplayMode : preferredDisplayMode);
                            displayModeString.append(" (16x9)");
                            break;

                        case Render::DisplayMode::AspectRatio::_16x10:
                            preferredDisplayMode = (preferredDisplayMode == 0 && displayMode.height > 800 ? currentDisplayMode : preferredDisplayMode);
                            displayModeString.append(" (16x10)");
                            break;
                        };

                        displayModeStringList.push_back(displayModeString);
                    }

                    auto deviceOptions = getOption("render", renderDeviceName);
                    auto displayMode = JSON::Value(deviceOptions, "mode", preferredDisplayMode);
                    setDisplayMode(displayMode);
                }

                population = getContext()->createClass<Engine::Population>("Engine::Population", (Engine::Core *)this);
                population->onLoad.connect(this, &Core::onPopulationLoaded);

                resources = getContext()->createClass<Engine::Resources>("Engine::Resources", (Engine::Core *)this);

                visualizer = getContext()->createClass<Plugin::Visualizer>("Engine::Visualizer", (Engine::Core *)this);
                visualizer->onShowUserInterface.connect(this, &Core::onShowUserInterface);

                getContext()->log(Context::Info, "Loading processor plugins");

                std::vector<std::string_view> processorNameList;
                getContext()->listTypes("ProcessorType", [&](std::string_view className) -> void
                                        {
                    processorNameList.push_back(className);
					getContext()->log(Context::Info, "- {} processor found", className); });

                processorList.reserve(processorNameList.size());
                for (auto const &processorName : processorNameList)
                {
                    processorList.push_back(getContext()->createClass<Plugin::Processor>(processorName, (Plugin::Core *)this));
                }

                onInitialized.emit();

                queueStartupSceneLoad();

                ImGuiIO &imGuiIo = ImGui::GetIO();
                imGuiIo.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
                imGuiIo.MouseDrawCursor = enableInterfaceControl;
                window->setCursorVisibility(enableInterfaceControl);

                windowActive = true;

                window->setVisibility(true);
                setFullScreen(Plugin::Core::getOption("render", "fullScreen", false));
                getContext()->log(Gek::Context::Info, "Finished Core Initialization");
            }

            void onCloseRequested(void)
            {
                if (!ignoredFirstCloseRequest)
                {
                    ignoredFirstCloseRequest = true;
                    getContext()->log(Gek::Context::Warning, "Core::onCloseRequested ignored (first close request) ");
                    return;
                }

                getContext()->log(Gek::Context::Warning, "Core::onCloseRequested invoked");
                forceClose();
            }

            void onWindowIdle(void)
            {
                timer.update();

                if (pendingPopulationLoad && population)
                {
                    pendingPopulationLoad = false;
                    population->load(pendingPopulationName);
                }

                if (populationLoadCompleted.exchange(false, std::memory_order_acq_rel))
                {
                    loadingPopulation = false;
                }

                ImGuiIO &imGuiIo = ImGui::GetIO();
                if (windowActive)
                {
                    float frameTime = static_cast<float>(timer.getUpdateTime());
                    if (std::isfinite(frameTime) && frameTime > 0.0f)
                    {
                        const double frameTimeMs = static_cast<double>(frameTime) * 1000.0;
                        const double fpsInstant = std::clamp(1.0 / static_cast<double>(frameTime), 0.0, 10000.0);
                        constexpr double FpsSmoothingAlpha = 0.1;
                        if (!runtimeFpsSmoothedInitialized)
                        {
                            runtimeFpsSmoothed = fpsInstant;
                            runtimeFpsSmoothedInitialized = true;
                        }
                        else
                        {
                            runtimeFpsSmoothed += ((fpsInstant - runtimeFpsSmoothed) * FpsSmoothingAlpha);
                        }

                        getContext()->setRuntimeMetric("render.frameTimeMs", frameTimeMs);
                        getContext()->setRuntimeMetric("render.fpsInstant", fpsInstant);
                        getContext()->setRuntimeMetric("render.fpsSmoothed", runtimeFpsSmoothed);
                    }

                    modeChangeTimer -= frameTime;

                    const float updateFrameTime = (enableInterfaceControl || loadingPopulation) ? 0.0f : frameTime;
                    if (population)
                    {
                        population->update(updateFrameTime);
                    }

                    if (!enableInterfaceControl && !loadingPopulation)
                    {
                        auto rectangle = window->getScreenRectangle();
                        window->setCursorPosition(Math::Int2(
                            int(Math::Interpolate(float(rectangle.minimum.x), float(rectangle.maximum.x), 0.5f)),
                            int(Math::Interpolate(float(rectangle.minimum.y), float(rectangle.maximum.y), 0.5f))));
                    }

                    int32_t xMovement = pendingMouseXMovement.exchange(0, std::memory_order_acq_rel);
                    int32_t yMovement = pendingMouseYMovement.exchange(0, std::memory_order_acq_rel);

                    xMovement = std::clamp(xMovement, -MaxAppliedMouseDeltaPerFrame, MaxAppliedMouseDeltaPerFrame);
                    yMovement = std::clamp(yMovement, -MaxAppliedMouseDeltaPerFrame, MaxAppliedMouseDeltaPerFrame);

                    if (!enableInterfaceControl && !loadingPopulation && population)
                    {
                        if (xMovement || yMovement)
                        {
                            population->action(Plugin::Population::Action("turn", xMovement * mouseSensitivity));
                            population->action(Plugin::Population::Action("tilt", yMovement * mouseSensitivity));
                        }
                    }
                }
            }

            void onWindowActivate(bool isActive)
            {
                windowActive = isActive;
            }

            void onWindowSizeChanged(bool isMinimized)
            {
                if (renderDevice && !isMinimized)
                {
                    renderDevice->handleResize();
                    onChangedDisplay();
                }
            }

            void onCharacter(uint32_t character)
            {
                ImGuiIO &imGuiIo = ImGui::GetIO();
                imGuiIo.AddInputCharacter(character);
            }

            void onKeyPressed(Window::Key keyCode, bool state)
            {
                ImGuiIO &imGuiIo = ImGui::GetIO();
                if (enableInterfaceControl)
                {
                    imGuiIo.AddKeyEvent(GetImGuiKey(keyCode), state);
                }

                if (!state)
                {
                    switch (keyCode)
                    {
                    case Window::Key::Escape:
                        enableInterfaceControl = !enableInterfaceControl;
                        imGuiIo.MouseDrawCursor = enableInterfaceControl;
                        window->setCursorVisibility(enableInterfaceControl);
                        if (enableInterfaceControl)
                        {
                            auto client = window->getClientRectangle();
                            imGuiIo.MousePos.x = ((float(client.maximum.x - client.minimum.x) * 0.5f) + client.minimum.x);
                            imGuiIo.MousePos.y = ((float(client.maximum.y - client.minimum.y) * 0.5f) + client.minimum.y);
                        }
                        else
                        {
                            imGuiIo.MousePos.x = -1.0f;
                            imGuiIo.MousePos.y = -1.0f;
                        }

                        break;

                    case Window::Key::F10:
                        showRuntimeDiagnostics = !showRuntimeDiagnostics;
                        break;
                    };
                }

                if (!enableInterfaceControl && population)
                {
                    switch (keyCode)
                    {
                    case Window::Key::W:
                    case Window::Key::Up:
                        population->action(Plugin::Population::Action("move_forward", state));
                        break;

                    case Window::Key::S:
                    case Window::Key::Down:
                        population->action(Plugin::Population::Action("move_backward", state));
                        break;

                    case Window::Key::A:
                    case Window::Key::Left:
                        population->action(Plugin::Population::Action("strafe_left", state));
                        break;

                    case Window::Key::D:
                    case Window::Key::Right:
                        population->action(Plugin::Population::Action("strafe_right", state));
                        break;

                    case Window::Key::Space:
                        population->action(Plugin::Population::Action("jump", state));
                        break;

                    case Window::Key::LeftControl:
                        population->action(Plugin::Population::Action("crouch", state));
                        break;
                    };
                }
            }

            void onMouseClicked(Window::Button button, bool state)
            {
                ImGuiIO &imGuiIo = ImGui::GetIO();
                if (enableInterfaceControl)
                {
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
            }

            void onMouseWheel(float numberOfRotations)
            {
                ImGuiIO &imGuiIo = ImGui::GetIO();
                if (enableInterfaceControl)
                {
                    imGuiIo.MouseWheel += numberOfRotations;
                }
            }

            void onMousePosition(int32_t xPosition, int32_t yPosition)
            {
                ImGuiIO &imGuiIo = ImGui::GetIO();
                if (enableInterfaceControl)
                {
                    imGuiIo.MousePos.x = static_cast<float>(xPosition);
                    imGuiIo.MousePos.y = static_cast<float>(yPosition);
                }
            }

            void onMouseMovement(int32_t xMovement, int32_t yMovement)
            {
                accumulateMouseMovement(pendingMouseXMovement, xMovement);
                accumulateMouseMovement(pendingMouseYMovement, yMovement);
            }

            void onPopulationLoaded(std::string const &)
            {
                populationLoadCompleted.store(true, std::memory_order_release);
            }

            // Renderer
            void onShowUserInterface(void)
            {
                ImGuiIO &imGuiIo = ImGui::GetIO();
                auto runtimeMetrics = getContext()->getRuntimeMetricSnapshot();
                auto runtimeFrameSearch = runtimeMetrics.find("visualizer.frame");
                if (runtimeFrameSearch != std::end(runtimeMetrics))
                {
                    uint64_t runtimeFrame = static_cast<uint64_t>(runtimeFrameSearch->second);
                    if (runtimeFrame > runtimeMetricsLastFrame)
                    {
                        runtimeMetricsLastFrame = runtimeFrame;
                        for (auto &plot : runtimeMetricPlots)
                        {
                            float value = 0.0f;
                            auto metricSearch = runtimeMetrics.find(plot.key);
                            if (metricSearch != std::end(runtimeMetrics))
                            {
                                value = static_cast<float>(metricSearch->second);
                            }

                            if (plot.sampleCount < plot.samples.size())
                            {
                                plot.samples[plot.sampleCount++] = value;
                            }
                            else
                            {
                                std::move(std::begin(plot.samples) + 1, std::end(plot.samples), std::begin(plot.samples));
                                plot.samples.back() = value;
                            }
                        }
                    }
                }

                if (enableInterfaceControl)
                {
                    ImGui::BeginMainMenuBar();
                    ImGui::PushStyleVar(ImGuiStyleVar_ItemInnerSpacing, ImVec2(5.0f, 10.0f));
                    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(5.0f, 10.0f));
                    if (ImGui::BeginMenu("File"))
                    {
                        if (ImGui::MenuItem("Load Scene"))
                        {
                            bool isModified = false;
                            listProcessors([&](Plugin::Processor *processor) -> void
                                           {
                                auto castCheck = dynamic_cast<Edit::Events*>(processor);
                                if (castCheck)
                                {
                                    isModified = castCheck->isModified();
                                } });

                            if (isModified)
                            {
                                showSaveModified = true;
                                loadOnModified = true;
                            }
                            else
                            {
                                triggerLoadWindow();
                            }
                        }

                        if (ImGui::MenuItem("Save Scene"))
                        {
                            population->save("demo_save");
                        }

                        ImGui::Separator();
                        if (ImGui::MenuItem("Clear Scene"))
                        {
                            showResetDialog = true;
                        }

                        ImGui::Separator();
                        if (ImGui::MenuItem("Settings", nullptr, &showSettings))
                        {
                            next = previous = current;
                            shadersSettings = configuration["shaders"];
                            filtersSettings = configuration["filters"];
                            changedVisualOptions = false;
                        }

                        ImGui::Separator();
                        if (ImGui::MenuItem("Quit"))
                        {
                            confirmClose();
                        }

                        ImGui::EndMenu();
                    }

                    if (ImGui::BeginMenu("View"))
                    {
                        ImGui::MenuItem("Runtime Diagnostics", nullptr, &showRuntimeDiagnostics);
                        ImGui::EndMenu();
                    }

                    ImGui::PopStyleVar(2);
                    ImGui::EndMainMenuBar();

                    showSettingsWindow();
                    showDisplayBackup();
                    showModifiedPrompt();
                    showLoadWindow();
                    showReset();
                }

                showRuntimeDiagnosticsWindow(runtimeMetrics);

                showLoading();
            }

            void showRuntimeDiagnosticsWindow(std::unordered_map<std::string, double> const &runtimeMetrics)
            {
                if (!showRuntimeDiagnostics)
                {
                    return;
                }

                ImGui::SetNextWindowSize(ImVec2(680.0f, 520.0f), ImGuiCond_Once);
                if (ImGui::Begin("Runtime Diagnostics", &showRuntimeDiagnostics))
                {
                    auto readMetricValue = [&](char const *key, double fallbackValue = 0.0) -> double
                    {
                        auto search = runtimeMetrics.find(key);
                        return (search != std::end(runtimeMetrics) ? search->second : fallbackValue);
                    };

                    bool isVulkanBackend = true;
                    auto renderBackendSearch = runtimeMetrics.find("render.backend");
                    if (renderBackendSearch != std::end(runtimeMetrics))
                    {
                        isVulkanBackend = (renderBackendSearch->second < 0.5);
                    }

                    auto applyMetricFilter = [&](std::function<bool(std::string_view)> const &predicate) -> void
                    {
                        for (size_t plotIndex = 0; plotIndex < runtimeMetricPlots.size(); ++plotIndex)
                        {
                            auto key = runtimeMetricPlots[plotIndex].key;
                            runtimeMetricVisible[plotIndex] = (key ? predicate(key) : false);
                        }

                        runtimeMetricViewAll = std::all_of(std::begin(runtimeMetricVisible), std::end(runtimeMetricVisible), [](bool value) -> bool
                                                           { return value; });
                    };

                    bool viewAllChanged = ImGui::Checkbox("View all metrics", &runtimeMetricViewAll);
                    if (viewAllChanged && runtimeMetricViewAll)
                    {
                        for (size_t plotIndex = 0; plotIndex < runtimeMetricVisible.size(); ++plotIndex)
                        {
                            runtimeMetricVisible[plotIndex] = true;
                        }
                    }

                    if (ImGui::Button("All"))
                    {
                        applyMetricFilter([](std::string_view) -> bool
                                          { return true; });
                    }

                    ImGui::SameLine();
                    if (ImGui::Button("None"))
                    {
                        applyMetricFilter([](std::string_view) -> bool
                                          { return false; });
                    }

                    ImGui::SameLine();
                    if (ImGui::Button("Visualizer"))
                    {
                        applyMetricFilter([](std::string_view key) -> bool
                                          { return key.starts_with("visualizer."); });
                    }

                    ImGui::SameLine();
                    if (ImGui::Button("Model"))
                    {
                        applyMetricFilter([](std::string_view key) -> bool
                                          { return key.starts_with("model."); });
                    }

                    ImGui::SameLine();
                    if (ImGui::Button("Resources"))
                    {
                        applyMetricFilter([](std::string_view key) -> bool
                                          { return key.starts_with("resources."); });
                    }

                    ImGui::SameLine();
                    if (ImGui::Button("Render"))
                    {
                        applyMetricFilter([](std::string_view key) -> bool
                                          { return key.starts_with("render."); });
                    }

                    ImGui::SameLine();
                    if (ImGui::Button("Backend"))
                    {
                        applyMetricFilter([](std::string_view key) -> bool
                                          { return key.starts_with("render.") || key.starts_with("vulkan.") || key.starts_with("d3d11."); });
                    }

                    ImGui::Separator();
                    ImGui::Text(
                        "Backend: %s | FPS: %.1f (smooth %.1f) | Frame: %.2f ms | Present CPU: %.3f ms",
                        isVulkanBackend ? "Vulkan" : "D3D11",
                        readMetricValue("render.fpsInstant"),
                        readMetricValue("render.fpsSmoothed"),
                        readMetricValue("render.frameTimeMs"),
                        readMetricValue("render.presentCpuMs"));

                    if (isVulkanBackend)
                    {
                        ImGui::Text(
                            "Vulkan submit: %.3f ms | Vulkan present: %.3f ms",
                            readMetricValue("vulkan.submitCpuMs"),
                            readMetricValue("vulkan.presentCpuMs"));
                        ImGui::Text(
                            "Vulkan waitFence: %.3f ms | acquire: %.3f ms | record: %.3f ms | frameCPU: %.3f ms",
                            readMetricValue("vulkan.waitFenceCpuMs"),
                            readMetricValue("vulkan.acquireCpuMs"),
                            readMetricValue("vulkan.recordCpuMs"),
                            readMetricValue("vulkan.frameCpuMs"));
                        ImGui::Text(
                            "Vulkan cleanup: %.3f ms | lock: %.3f ms | untracked: %.3f ms",
                            readMetricValue("vulkan.cleanupCpuMs"),
                            readMetricValue("vulkan.drawCommandLockCpuMs"),
                            readMetricValue("vulkan.untrackedFrameCpuMs"));
                        ImGui::Text(
                            "Vulkan cleanup split -> descriptor: %.3f ms | framebuffer: %.3f ms",
                            readMetricValue("vulkan.cleanupDescriptorCpuMs"),
                            readMetricValue("vulkan.cleanupFramebufferCpuMs"));
                        ImGui::Text(
                            "Vulkan snapshots -> created: %.0f | reused: %.0f | pending: %.0f | inFlight: %.0f",
                            readMetricValue("vulkan.uiSnapshotCreated"),
                            readMetricValue("vulkan.uiSnapshotReused"),
                            readMetricValue("vulkan.uiSnapshotPending"),
                            readMetricValue("vulkan.uiSnapshotInFlight"));

                        const double fpsSmoothed = readMetricValue("render.fpsSmoothed");
                        const double presentCpuMs = readMetricValue("render.presentCpuMs");
                        const double waitFenceCpuMs = readMetricValue("vulkan.waitFenceCpuMs");
                        const double recordCpuMs = readMetricValue("vulkan.recordCpuMs");
                        if (fpsSmoothed <= 6.0 && presentCpuMs <= 1.0)
                        {
                            const char *dominantCost = (waitFenceCpuMs >= recordCpuMs)
                                                           ? "GPU sync stall (waitFence)"
                                                           : "CPU command recording";
                            ImGui::TextColored(ImVec4(1.0f, 0.70f, 0.30f, 1.0f), "Likely bottleneck: %s", dominantCost);
                        }
                    }
                    else
                    {
                        ImGui::Text("D3D11 present: %.3f ms", readMetricValue("d3d11.presentCpuMs"));
                    }

                    if (ImGui::CollapsingHeader("Metric Visibility", ImGuiTreeNodeFlags_DefaultOpen))
                    {
                        ImGui::BeginChild("##RuntimeMetricList", ImVec2(0.0f, 190.0f), true, ImGuiWindowFlags_AlwaysVerticalScrollbar);
                        ImGuiListClipper clipper;
                        clipper.Begin(static_cast<int>(runtimeMetricPlots.size()));
                        while (clipper.Step())
                        {
                            for (int clippedIndex = clipper.DisplayStart; clippedIndex < clipper.DisplayEnd; ++clippedIndex)
                            {
                                const size_t plotIndex = static_cast<size_t>(clippedIndex);
                                auto const &plot = runtimeMetricPlots[plotIndex];
                                ImGui::PushID(clippedIndex);
                                ImGui::ColorButton("##legendColor", plot.color, ImGuiColorEditFlags_NoTooltip, ImVec2(12.0f, 12.0f));
                                ImGui::SameLine();
                                if (ImGui::Checkbox(plot.label, &runtimeMetricVisible[plotIndex]))
                                {
                                    runtimeMetricViewAll = std::all_of(std::begin(runtimeMetricVisible), std::end(runtimeMetricVisible), [](bool value) -> bool
                                                                       { return value; });
                                }

                                ImGui::PopID();
                            }
                        }

                        ImGui::EndChild();
                    }

                    ImGui::BeginChild("##RuntimeChartRegion", ImVec2(0.0f, 240.0f), true, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
                    ImVec2 canvasSize = ImGui::GetContentRegionAvail();
                    ImDrawList *drawList = ImGui::GetWindowDrawList();
                    if (canvasSize.x < 8.0f || canvasSize.y < 8.0f)
                    {
                        ImGui::TextDisabled("Chart region too small to render.");
                        ImGui::EndChild();
                        ImGui::Separator();
                        ImGui::TextUnformatted("Log Sinks");
                        showLoggingSinkControls("runtime_diagnostics");
                        ImGui::End();
                        return;
                    }

                    bool useVulkanCompatibilityChart = true;
                    useVulkanCompatibilityChart = isVulkanBackend;

                    if (isVulkanBackend)
                    {
                        ImGui::Checkbox("Enable Vulkan charts (experimental)", &runtimeEnableVulkanCharts);
                    }

                    ImVec2 canvasPosition = ImGui::GetCursorScreenPos();

                    bool hasVisibleSeries = false;
                    size_t maxSampleCount = 0;
                    for (size_t plotIndex = 0; plotIndex < runtimeMetricPlots.size(); ++plotIndex)
                    {
                        if (runtimeMetricVisible[plotIndex])
                        {
                            hasVisibleSeries = true;
                            maxSampleCount = std::max(maxSampleCount, runtimeMetricPlots[plotIndex].sampleCount);
                        }
                    }

                    bool drewAnySeries = false;

                    if (useVulkanCompatibilityChart)
                    {
                        if (!runtimeEnableVulkanCharts)
                        {
                            ImGui::TextDisabled("Vulkan chart rendering is disabled by default for stability.");
                            ImGui::TextDisabled("Enable the experimental toggle above only for short profiling checks.");
                        }
                        else
                        {
                            ImGui::TextDisabled("Vulkan compatibility chart mode");
                            const ImVec2 clipMin = ImGui::GetCursorScreenPos();
                            const ImVec2 clipMax = ImVec2(clipMin.x + canvasSize.x, clipMin.y + canvasSize.y);
                            drawList->PushClipRect(clipMin, clipMax, true);
                            constexpr size_t VulkanPlotSeriesLimit = 6;
                            constexpr size_t VulkanPlotSampleLimit = 64;
                            size_t plottedSeriesCount = 0;
                            for (size_t plotIndex = 0; plotIndex < runtimeMetricPlots.size(); ++plotIndex)
                            {
                                if (!runtimeMetricVisible[plotIndex])
                                {
                                    continue;
                                }

                                auto const &plot = runtimeMetricPlots[plotIndex];
                                if (plot.sampleCount == 0)
                                {
                                    continue;
                                }

                                if (plottedSeriesCount >= VulkanPlotSeriesLimit)
                                {
                                    break;
                                }

                                const size_t displayedSampleCount = std::min(plot.sampleCount, VulkanPlotSampleLimit);
                                const size_t sampleStride = std::max<size_t>(1, plot.sampleCount / displayedSampleCount);

                                std::array<float, VulkanPlotSampleLimit> sanitizedSamples = {};
                                size_t outputSampleIndex = 0;
                                for (size_t sampleIndex = 0; sampleIndex < plot.sampleCount && outputSampleIndex < displayedSampleCount; sampleIndex += sampleStride)
                                {
                                    float value = plot.samples[sampleIndex];
                                    if (!std::isfinite(value))
                                    {
                                        value = (outputSampleIndex > 0 ? sanitizedSamples[outputSampleIndex - 1] : 0.0f);
                                    }

                                    sanitizedSamples[outputSampleIndex++] = std::clamp(value, -1000000.0f, 1000000.0f);
                                }

                                if (outputSampleIndex == 0)
                                {
                                    continue;
                                }

                                ImGui::PushID(static_cast<int>(plotIndex));
                                ImGui::PushStyleColor(ImGuiCol_PlotLines, plot.color);
                                ImGui::PlotLines("##RuntimePlotLine", sanitizedSamples.data(), static_cast<int>(outputSampleIndex), 0, nullptr, FLT_MAX, FLT_MAX, ImVec2(-1.0f, 28.0f));
                                ImGui::PopStyleColor();
                                ImGui::PopID();
                                drewAnySeries = true;
                                ++plottedSeriesCount;
                            }

                            drawList->PopClipRect();

                            if (plottedSeriesCount == VulkanPlotSeriesLimit)
                            {
                                ImGui::TextDisabled("Showing first 6 selected metrics for stability.");
                            }
                        }
                    }
                    else
                    {
                        ImGui::InvisibleButton("##RuntimeLineChartCanvas", canvasSize);
                        const ImVec2 chartMin = canvasPosition;
                        const ImVec2 chartMax = ImVec2(canvasPosition.x + canvasSize.x, canvasPosition.y + canvasSize.y);
                        drawList->AddRectFilled(chartMin, chartMax, IM_COL32(24, 24, 24, 255), 4.0f);
                        drawList->AddRect(chartMin, chartMax, IM_COL32(90, 90, 90, 255), 4.0f);
                        drawList->PushClipRect(chartMin, chartMax, true);

                        bool hasAnyFiniteSample = false;
                        float valueMin = 0.0f;
                        float valueMax = 1.0f;
                        if (hasVisibleSeries)
                        {
                            for (size_t plotIndex = 0; plotIndex < runtimeMetricPlots.size(); ++plotIndex)
                            {
                                if (!runtimeMetricVisible[plotIndex])
                                {
                                    continue;
                                }

                                auto const &plot = runtimeMetricPlots[plotIndex];
                                if (plot.sampleCount == 0)
                                {
                                    continue;
                                }

                                for (size_t sampleIndex = 0; sampleIndex < plot.sampleCount; ++sampleIndex)
                                {
                                    float value = plot.samples[sampleIndex];
                                    if (!std::isfinite(value))
                                    {
                                        continue;
                                    }

                                    value = std::clamp(value, -1000000.0f, 1000000.0f);
                                    if (!hasAnyFiniteSample)
                                    {
                                        valueMin = value;
                                        valueMax = value;
                                    }
                                    else
                                    {
                                        valueMin = std::min(valueMin, value);
                                        valueMax = std::max(valueMax, value);
                                    }

                                    hasAnyFiniteSample = true;
                                }
                            }

                            if (hasAnyFiniteSample && maxSampleCount > 1)
                            {
                                const float valueRange = std::max(valueMax - valueMin, 0.001f);
                                for (size_t plotIndex = 0; plotIndex < runtimeMetricPlots.size(); ++plotIndex)
                                {
                                    if (!runtimeMetricVisible[plotIndex])
                                    {
                                        continue;
                                    }

                                    auto const &plot = runtimeMetricPlots[plotIndex];
                                    if (plot.sampleCount < 2)
                                    {
                                        continue;
                                    }

                                    const size_t startOffset = (maxSampleCount - plot.sampleCount);
                                    for (size_t sampleIndex = 1; sampleIndex < plot.sampleCount; ++sampleIndex)
                                    {
                                        float previousValue = plot.samples[sampleIndex - 1];
                                        float currentValue = plot.samples[sampleIndex];
                                        if (!std::isfinite(previousValue) || !std::isfinite(currentValue))
                                        {
                                            continue;
                                        }

                                        previousValue = std::clamp(previousValue, -1000000.0f, 1000000.0f);
                                        currentValue = std::clamp(currentValue, -1000000.0f, 1000000.0f);

                                        const size_t globalPrevious = startOffset + sampleIndex - 1;
                                        const size_t globalCurrent = startOffset + sampleIndex;
                                        const float previousX = chartMin.x + (static_cast<float>(globalPrevious) / static_cast<float>(maxSampleCount - 1)) * canvasSize.x;
                                        const float currentX = chartMin.x + (static_cast<float>(globalCurrent) / static_cast<float>(maxSampleCount - 1)) * canvasSize.x;

                                        const float previousNormalized = (previousValue - valueMin) / valueRange;
                                        const float currentNormalized = (currentValue - valueMin) / valueRange;
                                        const float previousY = chartMax.y - (previousNormalized * canvasSize.y);
                                        const float currentY = chartMax.y - (currentNormalized * canvasSize.y);

                                        drawList->AddLine(
                                            ImVec2(std::clamp(previousX, chartMin.x, chartMax.x), std::clamp(previousY, chartMin.y, chartMax.y)),
                                            ImVec2(std::clamp(currentX, chartMin.x, chartMax.x), std::clamp(currentY, chartMin.y, chartMax.y)),
                                            ImGui::ColorConvertFloat4ToU32(plot.color),
                                            1.5f);
                                        drewAnySeries = true;
                                    }
                                }
                            }
                        }

                        if (!drewAnySeries)
                        {
                            const char *emptyMessage = hasVisibleSeries ? "Waiting for runtime metrics..." : "No metrics selected.";
                            ImVec2 textSize = ImGui::CalcTextSize(emptyMessage);
                            ImVec2 textPosition(
                                chartMin.x + (canvasSize.x - textSize.x) * 0.5f,
                                chartMin.y + (canvasSize.y - textSize.y) * 0.5f);
                            drawList->AddText(textPosition, IM_COL32(180, 180, 180, 255), emptyMessage);
                        }

                        drawList->PopClipRect();
                    }

                    if (useVulkanCompatibilityChart && !drewAnySeries)
                    {
                        ImGui::TextDisabled(hasVisibleSeries ? "Waiting for runtime metrics..." : "No metrics selected.");
                    }
                    ImGui::EndChild();

                    if (ImGui::CollapsingHeader("Metric Summaries (Advanced)", ImGuiTreeNodeFlags_None))
                    {
                        std::vector<size_t> summaryPlotIndices;
                        summaryPlotIndices.reserve(runtimeMetricPlots.size());
                        for (size_t plotIndex = 0; plotIndex < runtimeMetricPlots.size(); ++plotIndex)
                        {
                            if (!runtimeMetricVisible[plotIndex])
                            {
                                continue;
                            }

                            if (runtimeMetricPlots[plotIndex].sampleCount == 0)
                            {
                                continue;
                            }

                            summaryPlotIndices.push_back(plotIndex);
                        }

                        ImGui::BeginChild("##RuntimeMetricSummaryList", ImVec2(0.0f, 180.0f), true, ImGuiWindowFlags_AlwaysVerticalScrollbar);
                        if (summaryPlotIndices.empty())
                        {
                            ImGui::TextDisabled("No visible metrics with samples.");
                        }
                        else
                        {
                            ImGuiListClipper clipper;
                            clipper.Begin(static_cast<int>(summaryPlotIndices.size()));
                            while (clipper.Step())
                            {
                                for (int clippedIndex = clipper.DisplayStart; clippedIndex < clipper.DisplayEnd; ++clippedIndex)
                                {
                                    const size_t plotIndex = summaryPlotIndices[clippedIndex];
                                    auto const &plot = runtimeMetricPlots[plotIndex];

                                    float minimum = 0.0f;
                                    float maximum = 0.0f;
                                    float total = 0.0f;
                                    float current = 0.0f;
                                    size_t validCount = 0;
                                    for (size_t sampleIndex = 0; sampleIndex < plot.sampleCount; ++sampleIndex)
                                    {
                                        float value = plot.samples[sampleIndex];
                                        if (!std::isfinite(value))
                                        {
                                            continue;
                                        }

                                        value = std::clamp(value, -1000000.0f, 1000000.0f);
                                        if (validCount == 0)
                                        {
                                            minimum = value;
                                            maximum = value;
                                        }
                                        else
                                        {
                                            minimum = std::min(minimum, value);
                                            maximum = std::max(maximum, value);
                                        }

                                        total += value;
                                        current = value;
                                        ++validCount;
                                    }

                                    if (validCount > 0)
                                    {
                                        float average = (total / static_cast<float>(validCount));
                                        ImGui::Text("%s | current=%.1f min=%.1f max=%.1f avg=%.1f", plot.label, current, minimum, maximum, average);
                                    }
                                    else
                                    {
                                        ImGui::Text("%s | no valid samples", plot.label);
                                    }
                                }
                            }
                        }

                        ImGui::EndChild();
                    }

                    auto fallbackEntitySearch = runtimeMetrics.find("model.entityCullingFallback");
                    auto fallbackModelSearch = runtimeMetrics.find("model.modelCullingFallback");
                    if ((fallbackEntitySearch != std::end(runtimeMetrics) && fallbackEntitySearch->second > 0.0) ||
                        (fallbackModelSearch != std::end(runtimeMetrics) && fallbackModelSearch->second > 0.0))
                    {
                        ImGui::Separator();
                        ImGui::TextUnformatted("Model culling fallback triggered this frame.");
                    }

                    ImGui::Separator();
                    if (ImGui::Button("Dump visible metrics to log"))
                    {
                        logRuntimeMetricSnapshot(runtimeMetrics, true, "runtime-window-visible");
                    }

                    ImGui::SameLine();
                    if (ImGui::Button("Dump all metrics to log"))
                    {
                        logRuntimeMetricSnapshot(runtimeMetrics, false, "runtime-window-all");
                    }

                    ImGui::Separator();
                    ImGui::TextUnformatted("Log Sinks");
                    showLoggingSinkControls("runtime_diagnostics");
                }

                ImGui::End();
            }

            void showLoggingSinkControls(const char *scope)
            {
                ImGui::PushID(scope);

                const uint8_t sinkMask = getContext()->getLogSinkMask();
                bool sinkConsole = ((sinkMask & Context::LogSink_Console) != 0);
                bool sinkDebugger = ((sinkMask & Context::LogSink_Debugger) != 0);
                bool sinkFile = ((sinkMask & Context::LogSink_File) != 0);

                bool sinkSettingsChanged = false;
                sinkSettingsChanged |= ImGui::Checkbox("Console", &sinkConsole);
                ImGui::SameLine();
                sinkSettingsChanged |= ImGui::Checkbox("Debugger", &sinkDebugger);
                ImGui::SameLine();
                sinkSettingsChanged |= ImGui::Checkbox("File", &sinkFile);

                if (sinkSettingsChanged)
                {
                    uint8_t updatedSinkMask = Context::LogSink_None;
                    if (sinkConsole)
                    {
                        updatedSinkMask |= Context::LogSink_Console;
                    }

                    if (sinkDebugger)
                    {
                        updatedSinkMask |= Context::LogSink_Debugger;
                    }

                    if (sinkFile)
                    {
                        updatedSinkMask |= Context::LogSink_File;
                    }

                    getContext()->setLogSinkMask(updatedSinkMask);
                    setOption("logging"s, "sinkMask"s, static_cast<uint32_t>(updatedSinkMask));
                    if (sinkFile && runtimeLogFilePath[0] != '\0')
                    {
                        getContext()->setLogFilePath(FileSystem::Path(runtimeLogFilePath.data()), runtimeLogFileAppend);
                        setOption("logging"s, "filePath"s, std::string(runtimeLogFilePath.data()));
                    }
                }

                if (ImGui::Checkbox("Append File", &runtimeLogFileAppend))
                {
                    setOption("logging"s, "appendFile"s, runtimeLogFileAppend);
                    if (sinkFile && runtimeLogFilePath[0] != '\0')
                    {
                        getContext()->setLogFilePath(FileSystem::Path(runtimeLogFilePath.data()), runtimeLogFileAppend);
                    }
                }

                if (ImGui::Checkbox("Dump metrics on exit", &runtimeDumpMetricsOnExit))
                {
                    setOption("logging"s, "dumpRuntimeMetricsOnExit"s, runtimeDumpMetricsOnExit);
                }

                if (ImGui::Checkbox("CSV metric dump format", &runtimeDumpMetricsCsvFormat))
                {
                    setOption("logging"s, "dumpRuntimeMetricsCsvFormat"s, runtimeDumpMetricsCsvFormat);
                }

                bool applyPath = ImGui::InputText("Log File Path", runtimeLogFilePath.data(), runtimeLogFilePath.size(), ImGuiInputTextFlags_EnterReturnsTrue);
                ImGui::SameLine();
                applyPath |= ImGui::Button("Apply");
                if (applyPath && runtimeLogFilePath[0] != '\0')
                {
                    getContext()->setLogFilePath(FileSystem::Path(runtimeLogFilePath.data()), runtimeLogFileAppend);
                    setOption("logging"s, "filePath"s, std::string(runtimeLogFilePath.data()));
                }

                const uint8_t activeSinkMask = getContext()->getLogSinkMask();
                std::string sinkSummary = "Active sinks: ";
                bool hasSink = false;
                if ((activeSinkMask & Context::LogSink_Console) != 0)
                {
                    sinkSummary += "Console";
                    hasSink = true;
                }

                if ((activeSinkMask & Context::LogSink_Debugger) != 0)
                {
                    sinkSummary += (hasSink ? " + Debugger" : "Debugger");
                    hasSink = true;
                }

                if ((activeSinkMask & Context::LogSink_File) != 0)
                {
                    sinkSummary += (hasSink ? " + File" : "File");
                    sinkSummary += (runtimeLogFileAppend ? " (append)" : " (overwrite)");
                    hasSink = true;
                }

                if (!hasSink)
                {
                    sinkSummary += "None";
                }

                ImGui::TextUnformatted(sinkSummary.c_str());

                ImGui::PopID();
            }

            void showDisplay(void)
            {
                if (ImGui::BeginTabItem("Display", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings))
                {
                    ImGui::ShowStyleSelector("style");

                    ImGui::PushItemWidth(-1.0f);
                    ImGui::ListBox("##DisplayMode", &next.mode, [](void *data, int index) -> const char *
                                   {
                        Core *core = static_cast<Core *>(data);
                        if (index < 0 || index >= static_cast<int>(core->displayModeStringList.size()))
                        {
                            return "";
                        }

                        auto &mode = core->displayModeStringList[index];
                        return mode.c_str(); }, this, static_cast<int>(displayModeStringList.size()), 10);

                    ImGui::PopItemWidth();
                    ImGui::Spacing();
                    ImGui::Checkbox("FullScreen", &next.fullScreen);
                    ImGui::EndTabItem();
                }
            }

            void showVisual(void)
            {
                if (ImGui::BeginTabItem("Visual", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings))
                {
                    std::function<void(JSON::Object &)> showSetting;
                    showSetting = [&](JSON::Object &settingNode) -> void
                    {
                        for (auto &[settingName, settingNode] : settingNode.items())
                        {
                            auto label(std::format("##{}{}", settingName, settingName));
                            if (settingNode.is_object())
                            {
                                auto optionsSearch = settingNode.find("options");
                                if (optionsSearch != settingNode.end())
                                {
                                    auto optionsNode = optionsSearch.value();
                                    if (optionsNode.is_array())
                                    {
                                        std::vector<std::string> optionList;
                                        for (auto &choice : optionsNode)
                                        {
                                            optionList.push_back(choice.get<std::string>());
                                        }

                                        int selection = 0;
                                        const auto &selectorSearch = settingNode.find("selection");
                                        if (selectorSearch != settingNode.end())
                                        {
                                            auto selectionNode = selectorSearch.value();
                                            if (selectionNode.is_string())
                                            {
                                                auto selectedName = selectionNode.get<std::string>();
                                                auto optionsSearch = std::find_if(std::begin(optionList), std::end(optionList), [selectedName](std::string_view choice) -> bool
                                                                                  { return (selectedName == choice); });

                                                if (optionsSearch != std::end(optionList))
                                                {
                                                    selection = static_cast<int>(std::distance(std::begin(optionList), optionsSearch));
                                                    settingNode["selection"] = selection;
                                                }
                                            }
                                            else if (selectionNode.is_number())
                                            {
                                                selection = selectionNode.get<int32_t>();
                                            }
                                        }

                                        ImGui::TextUnformatted(settingName.data());
                                        ImGui::SameLine();
                                        ImGui::PushItemWidth(-1.0f);
                                        if (ImGui::Combo(label.data(), &selection, [](void *userData, int index) -> const char *
                                                         {
                                            auto &optionList = *static_cast<std::vector<std::string> *>(userData);
                                            if (index >= 0 && index < static_cast<int>(optionList.size()))
                                            {
                                                return optionList[index].c_str();
                                            }

                                            return ""; }, &optionList, static_cast<int>(optionList.size()), 10))
                                        {
                                            settingNode["selection"] = selection;
                                            changedVisualOptions = true;
                                        }

                                        ImGui::PopItemWidth();
                                    }
                                }
                                else
                                {
                                    if (ImGui::TreeNodeEx(settingName.data(), ImGuiTreeNodeFlags_Framed))
                                    {
                                        showSetting(settingNode);
                                        ImGui::TreePop();
                                    }
                                }
                            }
                            else if (settingNode.is_array())
                            {
                                ImGui::TextUnformatted(settingName.data());
                                ImGui::SameLine();
                                ImGui::PushItemWidth(-1.0f);
                                switch (settingNode.size())
                                {
                                case 1:
                                    [&](void) -> void
                                    {
                                        float data = settingNode[0].get<float>();
                                        if (ImGui::InputFloat(label.data(), &data))
                                        {
                                            settingNode = data;
                                            changedVisualOptions = true;
                                        }
                                    }();
                                    break;

                                case 2:
                                    [&](void) -> void
                                    {
                                        Math::Float2 data(
                                            settingNode[0].get<float>(),
                                            settingNode[1].get<float>());
                                        if (ImGui::InputFloat2(label.data(), data.data))
                                        {
                                            settingNode = { data.x, data.y };
                                            changedVisualOptions = true;
                                        }
                                    }();
                                    break;

                                case 3:
                                    [&](void) -> void
                                    {
                                        Math::Float3 data(
                                            settingNode[0].get<float>(),
                                            settingNode[1].get<float>(),
                                            settingNode[2].get<float>());
                                        if (ImGui::InputFloat3(label.data(), data.data))
                                        {
                                            settingNode = { data.x, data.y, data.z };
                                            changedVisualOptions = true;
                                        }
                                    }();
                                    break;

                                case 4:
                                    [&](void) -> void
                                    {
                                        Math::Float4 data(
                                            settingNode[0].get<float>(),
                                            settingNode[1].get<float>(),
                                            settingNode[2].get<float>(),
                                            settingNode[3].get<float>());
                                        if (ImGui::InputFloat4(label.data(), data.data))
                                        {
                                            settingNode = { data.x, data.y, data.z, data.w };
                                            changedVisualOptions = true;
                                        }
                                    }();
                                    break;
                                };

                                ImGui::PopItemWidth();
                            }
                            else if (settingNode.is_string())
                            {
                                ImGui::TextUnformatted(settingName.data());
                                ImGui::SameLine();
                                ImGui::PushItemWidth(-1.0f);
                                auto value = settingNode.get<std::string>();
                                if (UI::Input(label, &value))
                                {
                                    settingNode = value;
                                    changedVisualOptions = true;
                                }

                                ImGui::PopItemWidth();
                            }
                            else if (settingNode.is_number_float())
                            {
                                ImGui::TextUnformatted(settingName.data());
                                ImGui::SameLine();
                                ImGui::PushItemWidth(-1.0f);
                                auto value = settingNode.get<float>();
                                if (UI::Input(label, &value))
                                {
                                    settingNode = value;
                                    changedVisualOptions = true;
                                }

                                ImGui::PopItemWidth();
                            }
                            else if (settingNode.is_number_unsigned())
                            {
                                ImGui::TextUnformatted(settingName.data());
                                ImGui::SameLine();
                                ImGui::PushItemWidth(-1.0f);
                                auto value = settingNode.get<uint32_t>();
                                if (UI::Input(label, &value))
                                {
                                    settingNode = value;
                                    changedVisualOptions = true;
                                }

                                ImGui::PopItemWidth();
                            }
                            else if (settingNode.is_number())
                            {
                                ImGui::TextUnformatted(settingName.data());
                                ImGui::SameLine();
                                ImGui::PushItemWidth(-1.0f);
                                auto value = settingNode.get<int32_t>();
                                if (UI::Input(label, &value))
                                {
                                    settingNode = value;
                                    changedVisualOptions = true;
                                }

                                ImGui::PopItemWidth();
                            }
                            else if (settingNode.is_boolean())
                            {
                                ImGui::TextUnformatted(settingName.data());
                                ImGui::SameLine();
                                ImGui::PushItemWidth(-1.0f);
                                auto value = settingNode.get<bool>();
                                if (UI::Input(label, &value))
                                {
                                    settingNode = value;
                                    changedVisualOptions = true;
                                }

                                ImGui::PopItemWidth();
                            }
                        }
                    };

                    auto invertedDepthBuffer = Plugin::Core::getOption("render", "invertedDepthBuffer", true);
                    if (ImGui::Checkbox("Inverted Depth Buffer", &invertedDepthBuffer))
                    {
                        setOption("render"s, "invertedDepthBuffer"s, invertedDepthBuffer);
                        resources->reload();
                    }

                    if (ImGui::TreeNodeEx("Shaders", ImGuiTreeNodeFlags_Framed))
                    {
                        showSetting(shadersSettings);
                        ImGui::TreePop();
                    }

                    if (ImGui::TreeNodeEx("Filters", ImGuiTreeNodeFlags_Framed))
                    {
                        showSetting(filtersSettings);
                        ImGui::TreePop();
                    }

                    if (ImGui::TreeNodeEx("Logging", ImGuiTreeNodeFlags_Framed))
                    {
                        showLoggingSinkControls("settings_dialog");
                        ImGui::TreePop();
                    }

                    if (ImGui::TreeNodeEx("Vulkan Buffer Versioning", ImGuiTreeNodeFlags_Framed))
                    {
                        auto clampVersionRingSize = [](int32_t requestedRingSize) -> int32_t
                        {
                            return std::clamp(
                                requestedRingSize,
                                0,
                                static_cast<int32_t>(Render::Device::Description::DefaultVersioningRingSize));
                        };

                        int32_t constantRing = clampVersionRingSize(Plugin::Core::getOption(
                            "render",
                            "vulkanConstantBufferRing",
                            static_cast<int32_t>(Render::Device::Description::DefaultVersioningRingSize)));
                        int32_t vertexRing = clampVersionRingSize(Plugin::Core::getOption(
                            "render",
                            "vulkanVertexBufferRing",
                            static_cast<int32_t>(Render::Device::Description::DefaultVersioningRingSize)));
                        int32_t indexRing = clampVersionRingSize(Plugin::Core::getOption(
                            "render",
                            "vulkanIndexBufferRing",
                            static_cast<int32_t>(Render::Device::Description::DefaultVersioningRingSize)));

                        if (ImGui::SliderInt("Constant Ring Size", &constantRing, 0, static_cast<int32_t>(Render::Device::Description::DefaultVersioningRingSize)))
                        {
                            setOption("render", "vulkanConstantBufferRing", clampVersionRingSize(constantRing));
                        }

                        if (ImGui::SliderInt("Vertex Ring Size", &vertexRing, 0, static_cast<int32_t>(Render::Device::Description::DefaultVersioningRingSize)))
                        {
                            setOption("render", "vulkanVertexBufferRing", clampVersionRingSize(vertexRing));
                        }

                        if (ImGui::SliderInt("Index Ring Size", &indexRing, 0, static_cast<int32_t>(Render::Device::Description::DefaultVersioningRingSize)))
                        {
                            setOption("render", "vulkanIndexBufferRing", clampVersionRingSize(indexRing));
                        }

                        ImGui::TextDisabled("0-1 disables discard renaming, 2-3 enables fixed-ring versioning.");
                        ImGui::TextDisabled("Changes apply when the render device is recreated (restart).");
                        ImGui::TreePop();
                    }

                    ImGui::EndTabItem();
                }
            }

            void showSettingsWindow(void)
            {
                if (!showSettings)
                {
                    return;
                }

                auto &io = ImGui::GetIO();
                auto &style = ImGui::GetStyle();
                ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10.0f, 10.0f));
                ImGui::SetNextWindowPos(io.DisplaySize * 0.5f, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
                if (ImGui::Begin("Settings", &showSettings))
                {
                    if (ImGui::BeginTabBar("##Settings"))
                    {
                        showDisplay();
                        showVisual();
                        ImGui::EndTabBar();
                    }

                    bool applySettings = false;
                    if (ImGui::Button("Apply") || IsKeyDown(Window::Key::Return))
                    {
                        applySettings = true;
                    }

                    ImGui::SameLine();
                    if (ImGui::Button("Accept"))
                    {
                        applySettings = true;
                        showSettings = false;
                    }

                    if (applySettings)
                    {
                        bool changedDisplayMode = setDisplayMode(next.mode);
                        bool changedFullScreen = setFullScreen(next.fullScreen);
                        if (changedDisplayMode || changedFullScreen)
                        {
                            showModeChange = true;
                            modeChangeTimer = 10.0f;
                        }

                        if (changedVisualOptions)
                        {
                            configuration["shaders"] = shadersSettings;
                            configuration["filters"] = filtersSettings;
                            onChangedSettings();
                        }
                    }

                    ImGui::SameLine();
                    if (ImGui::Button("Cancel") || IsKeyDown(Window::Key::Escape))
                    {
                        showSettings = false;
                    }
                }

                ImGui::PopStyleVar();
                ImGui::End();
            }

            void showDisplayBackup(void)
            {
                if (!showModeChange)
                {
                    return;
                }

                auto &io = ImGui::GetIO();
                ImGui::SetNextWindowPos(io.DisplaySize * 0.5f, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
                if (ImGui::Begin("Keep Display Mode", &showModeChange, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings))
                {
                    ImGui::TextUnformatted("Keep Display Mode?");

                    if (ImGui::Button("Yes") || IsKeyDown(Window::Key::Return))
                    {
                        showModeChange = false;
                        previous = current;
                    }

                    ImGui::SameLine();
                    if (modeChangeTimer <= 0.0f || ImGui::Button("No") || IsKeyDown(Window::Key::Escape))
                    {
                        showModeChange = false;
                        setDisplayMode(previous.mode);
                        setFullScreen(previous.fullScreen);
                    }

                    ImGui::TextUnformatted(std::format("(Revert in {} seconds)", uint32_t(modeChangeTimer)).data());
                }

                ImGui::End();
            }

            bool showSaveModified = false;
            bool closeOnModified = false;
            bool loadOnModified = false;

            void refreshSceneList(void)
            {
                scenes.clear();
                currentSelectedScene = 0;
                getContext()->findDataFiles("scenes"s, [&scenes = scenes](FileSystem::Path const &filePath) -> bool
                                            {
                    if (filePath.isFile())
                    {
                        scenes.push_back(filePath.withoutExtension().getFileName());
                    }

                    return true; });
            }

            void queuePopulationLoad(std::string const &sceneName)
            {
                if (!population)
                {
                    return;
                }

                loadingPopulation = true;
                populationLoadCompleted.store(false, std::memory_order_release);
                pendingPopulationName = sceneName;
                pendingPopulationLoad = true;
            }

            void queueStartupSceneLoad(void)
            {
                const bool autoLoadScene = JSON::Value(getOption("application", "autoLoadDemoScene"), false);
                if (!autoLoadScene || pendingPopulationLoad || loadingPopulation)
                {
                    return;
                }

                refreshSceneList();
                if (scenes.empty())
                {
                    getContext()->log(Context::Warning, "Startup scene auto-load requested but no scenes were found");
                    return;
                }

                std::string startupScene = JSON::Value(getOption("application", "startupScene"), "demo"s);
                auto sceneSearch = std::find(std::begin(scenes), std::end(scenes), startupScene);
                if (sceneSearch == std::end(scenes))
                {
                    startupScene = scenes.front();
                    sceneSearch = std::begin(scenes);
                }

                currentSelectedScene = static_cast<uint32_t>(std::distance(std::begin(scenes), sceneSearch));
                queuePopulationLoad(startupScene);
                getContext()->log(Context::Info, "Auto-loading startup scene '{}'", startupScene);
            }

            void showModifiedPrompt(void)
            {
                if (!showSaveModified)
                {
                    return;
                }

                auto &io = ImGui::GetIO();
                ImGui::SetNextWindowPos(io.DisplaySize * 0.5f, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
                if (ImGui::Begin("Save Changes?", &showSaveModified, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings))
                {
                    ImGui::TextUnformatted("Changes have been made.");
                    ImGui::TextUnformatted("Do you want to save them?");

                    if (ImGui::Button("Yes") || IsKeyDown(Window::Key::Return))
                    {
                        population->save("demo_save");
                        showSaveModified = false;
                        if (closeOnModified)
                        {
                            forceClose();
                        }
                        else if (loadOnModified)
                        {
                            triggerLoadWindow();
                        }
                    }

                    ImGui::SameLine();
                    if (ImGui::Button("No") || IsKeyDown(Window::Key::Escape))
                    {
                        showSaveModified = false;
                        if (closeOnModified)
                        {
                            forceClose();
                        }
                        else if (loadOnModified)
                        {
                            triggerLoadWindow();
                        }
                    }

                    ImGui::SameLine();
                    if (ImGui::Button("Cancel") || IsKeyDown(Window::Key::Escape))
                    {
                        showSaveModified = false;
                    }
                }

                ImGui::End();
                if (!showSaveModified)
                {
                    closeOnModified = false;
                    loadOnModified = false;
                }
            }

            void triggerLoadWindow(void)
            {
                refreshSceneList();

                showLoadMenu = true;
            }

            void showLoadWindow(void)
            {
                if (!showLoadMenu)
                {
                    return;
                }

                auto &io = ImGui::GetIO();
                ImGui::SetNextWindowPos(io.DisplaySize * 0.5f, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
                if (ImGui::Begin("Load", &showLoadMenu, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoTitleBar))
                {
                    auto &style = ImGui::GetStyle();
                    if (scenes.empty())
                    {
                        ImGui::TextUnformatted("No scenes found");
                    }
                    else
                    {
                        ImGui::PushItemWidth(350.0f);
                        if (ImGui::BeginListBox("##loadscene"))
                        {
                            uint32_t sceneIndex = 0;
                            for (auto &scene : scenes)
                            {
                                ImGui::PushID(sceneIndex);
                                bool selected = (sceneIndex == currentSelectedScene);
                                if (ImGui::Selectable(scene.data(), &selected))
                                {
                                    currentSelectedScene = sceneIndex;
                                }

                                sceneIndex++;
                                ImGui::PopID();
                            }

                            ImGui::EndListBox();
                        }
                    }

                    if (!scenes.empty())
                    {
                        if (ImGui::Button("Load") || IsKeyDown(Window::Key::Return))
                        {
                            queuePopulationLoad(scenes[currentSelectedScene]);
                            showLoadMenu = false;
                        }
                    }

                    ImGui::SameLine();
                    if (ImGui::Button("Cancel") || IsKeyDown(Window::Key::Escape))
                    {
                        showLoadMenu = false;
                    }
                }

                ImGui::End();
            }

            void showReset(void)
            {
                if (!showResetDialog)
                {
                    return;
                }

                auto &io = ImGui::GetIO();
                ImGui::SetNextWindowPos(io.DisplaySize * 0.5f, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
                if (ImGui::Begin("Reset?", &showResetDialog, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings))
                {
                    ImGui::TextUnformatted("Reset Scene?");

                    if (ImGui::Button("Yes") || IsKeyDown(Window::Key::Return))
                    {
                        showResetDialog = false;
                        population->reset();
                    }

                    ImGui::SameLine();
                    if (ImGui::Button("No") || IsKeyDown(Window::Key::Escape))
                    {
                        showResetDialog = false;
                    }
                }

                ImGui::End();
            }

            void showLoading(void)
            {
                if (!loadingPopulation)
                {
                    return;
                }

                auto &io = ImGui::GetIO();
                ImGui::SetNextWindowPos(io.DisplaySize * 0.5f, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
                if (ImGui::Begin("Loading", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoTitleBar))
                {
                    ImGui::TextUnformatted("Loading...");
                }

                ImGui::End();
            }

            // Plugin::Core
            Context *const getContext(void) const
            {
                return ContextRegistration::getContext();
            }

            JSON::Object getOption(std::string_view system, std::string_view name) const
            {
                return JSON::Find(JSON::Find(configuration, system), name);
            }

            void setOption(std::string_view system, std::string_view name, JSON::Object const &value)
            {
                configuration[system][name] = value;
            }

            void deleteOption(std::string_view system, std::string_view name)
            {
                auto groupNode = configuration[system];
                auto search = groupNode.find(name.data());
                if (search != std::end(groupNode))
                {
                    groupNode.erase(search);
                }
            }

            Window::Device *getWindowDevice(void) const
            {
                return window.get();
            }

            Render::Device *getRenderDevice(void) const
            {
                return renderDevice.get();
            }

            Engine::Population *getFullPopulation(void) const
            {
                return population.get();
            }

            Engine::Resources *getFullResources(void) const
            {
                return resources.get();
            }

            Plugin::Population *getPopulation(void) const
            {
                return population.get();
            }

            Plugin::Resources *getResources(void) const
            {
                return resources.get();
            }

            Plugin::Visualizer *getVisualizer(void) const
            {
                return visualizer.get();
            }

            void listProcessors(std::function<void(Plugin::Processor *)> onProcessor)
            {
                for (auto const &processor : processorList)
                {
                    onProcessor(processor.get());
                }
            }
        };

        GEK_REGISTER_CONTEXT_USER(Core);
    }; // namespace Implementation
}; // namespace Gek
#include <string>
