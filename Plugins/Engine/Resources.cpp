#define _ENABLE_ATOMIC_ALIGNMENT_FIX

#include "GEK/Utility/String.hpp"
#include "GEK/Utility/ThreadPool.hpp"
#include "GEK/Utility/ShuntingYard.hpp"
#include "GEK/Utility/FileSystem.hpp"
#include "GEK/Utility/JSON.hpp"
#include "GEK/Utility/ContextUser.hpp"
#include "GEK/Shapes/Sphere.hpp"
#include "GEK/GUI/Utilities.hpp"
#include "GEK/API/Renderer.hpp"
#include "GEK/API/Population.hpp"
#include "GEK/API/Entity.hpp"
#include "GEK/API/Component.hpp"
#include "GEK/Components/Transform.hpp"
#include "GEK/Components/Light.hpp"
#include "GEK/Components/Color.hpp"
#include "GEK/Engine/Core.hpp"
#include "GEK/Engine/Resources.hpp"
#include "GEK/Engine/Shader.hpp"
#include "GEK/Engine/Filter.hpp"
#include "GEK/Engine/Material.hpp"
#include "GEK/Engine/Visual.hpp"
#include <tbb/concurrent_unordered_map.h>
#include <tbb/concurrent_unordered_set.h>
#include <imgui_internal.h>

class Float16Compressor
{
    union Bits
    {
        float f;
        int32_t si;
        uint32_t ui;
    };

    static int const shift = 13;
    static int const shiftSign = 16;

    static int32_t const infN = 0x7F800000; // flt32 infinity
    static int32_t const maxN = 0x477FE000; // max flt16 normal as a flt32
    static int32_t const minN = 0x38800000; // min flt16 normal as a flt32
    static int32_t const signN = 0x80000000; // flt32 sign bit

    static int32_t const infC = infN >> shift;
    static int32_t const nanN = (infC + 1) << shift; // minimum flt16 nan as a flt32
    static int32_t const maxC = maxN >> shift;
    static int32_t const minC = minN >> shift;
    static int32_t const signC = signN >> shiftSign; // flt16 sign bit

    static int32_t const mulN = 0x52000000; // (1 << 23) / minN
    static int32_t const mulC = 0x33800000; // minN / (1 << (23 - shift))

    static int32_t const subC = 0x003FF; // max flt32 subnormal down shifted
    static int32_t const norC = 0x00400; // min flt32 normal down shifted

    static int32_t const maxD = infC - maxC - 1;
    static int32_t const minD = minC - subC - 1;

public:
    static uint16_t compress(float value)
    {
        Bits v, s;
        v.f = value;
        uint32_t sign = v.si & signN;
        v.si ^= sign;
        sign >>= shiftSign; // logical shift
        s.si = mulN;
        s.si = s.f * v.f; // correct subnormals
        v.si ^= (s.si ^ v.si) & -(minN > v.si);
        v.si ^= (infN ^ v.si) & -((infN > v.si) & (v.si > maxN));
        v.si ^= (nanN ^ v.si) & -((nanN > v.si) & (v.si > infN));
        v.ui >>= shift; // logical shift
        v.si ^= ((v.si - maxD) ^ v.si) & -(v.si > maxC);
        v.si ^= ((v.si - minD) ^ v.si) & -(v.si > subC);
        return v.ui | sign;
    }

    static float decompress(uint16_t value)
    {
        Bits v;
        v.ui = value;
        int32_t sign = v.si & signC;
        v.si ^= sign;
        sign <<= shiftSign;
        v.si ^= ((v.si + minD) ^ v.si) & -(v.si > subC);
        v.si ^= ((v.si + maxD) ^ v.si) & -(v.si > maxC);
        Bits s;
        s.si = mulC;
        s.f *= v.si;
        int32_t mask = -(norC > v.si);
        v.si <<= shift;
        v.si ^= (s.si ^ v.si) & mask;
        v.si |= sign;
        return v.f;
    }
};

namespace Gek
{
    namespace Implementation
    {
        static ShuntingYard shuntingYard;

        template <typename HANDLE, typename TYPE>
        class ResourceCache
        {
        public:
            using TypePtr = std::shared_ptr<TYPE>;
            //using AtomicPtr = std::atomic<TypePtr>;
            using ResourceHandleMap = tbb::concurrent_unordered_map<std::size_t, HANDLE>;
            using ResourceMap = tbb::concurrent_unordered_map<HANDLE, TypePtr>;
            using ResourceType = ResourceMap::value_type;
            using HandleType = HANDLE;

        private:
            uint32_t validationIdentifier = 0;
            std::atomic_uint32_t nextIdentifier = 0;

        protected:
            ThreadPool& loadPool;
            ResourceHandleMap resourceHandleMap;
            ResourceMap resourceMap;

        public:
            ResourceCache(ThreadPool& loadPool)
                : loadPool(loadPool)
            {
            }

            virtual ~ResourceCache(void) = default;

            void visit(std::function<void(HandleType, TypePtr)> onResource)
            {
                for (auto& resourcePair : resourceMap)
                {
                    onResource(resourcePair.first, std::atomic_load(&resourcePair.second));
                }
            }

            virtual void clearExtra(void)
            {
            }

            void clear(void)
            {
                clearExtra();
                validationIdentifier = nextIdentifier;
                resourceHandleMap.clear();
                resourceMap.clear();
            }

            bool setResource(HANDLE handle, TypePtr&& data, HANDLE *fallback = nullptr)
            {
                TypePtr blankObject;
                auto resourceSearch = resourceMap.insert(std::make_pair(handle, blankObject));
                if (data.get())
                {
                    std::atomic_store(&resourceSearch.first->second, data);
                    return true;
                }
                else if (fallback)
                {
                    auto fallbackResource = getResource(*fallback);
                    if (fallbackResource)
                    {
                        std::atomic_store(&resourceSearch.first->second, TypePtr(fallbackResource));
                        return true;
                    }
                }

                return false;
            }

            Task scheduleResource(HANDLE handle, std::function<TypePtr(HandleType)>&& load, HANDLE *fallback = nullptr)
            {
                auto localLoad = std::move(load);
                co_await loadPool.schedule();
                auto resource = localLoad(handle);
                setResource(handle, std::move(resource), fallback);
            }

            virtual TYPE* const getResource(HANDLE handle) const
            {
                if (handle.identifier >= validationIdentifier)
                {
                    auto resourceSearch = resourceMap.find(handle);
                    if (resourceSearch != std::end(resourceMap))
                    {
                        return std::atomic_load(&resourceSearch->second).get();
                    }
                }

                return nullptr;
            }

            uint32_t getNextHandle(void)
            {
                return ++nextIdentifier;
            }
        };

        template <typename HANDLE, typename TYPE>
        class GeneralResourceCache
            : public ResourceCache<HANDLE, TYPE>
        {
        public:
            using TypePtr = ResourceCache<HANDLE, TYPE>::TypePtr;
            using HandleType = ResourceCache<HANDLE, TYPE>::HandleType;

        private:
            tbb::concurrent_unordered_set<std::size_t> requestedLoadSet;

        public:
            GeneralResourceCache(ThreadPool& loadPool)
                : ResourceCache<HANDLE, TYPE>(loadPool)
            {
            }

            void clearExtra(void)
            {
                requestedLoadSet.clear();
            }

            std::pair<bool, HANDLE> getHandle(std::size_t hash, std::function<TypePtr(HandleType)>&& load)
            {
                if (requestedLoadSet.count(hash) > 0)
                {
                    auto resourceSearch = ResourceCache<HANDLE, TYPE>::resourceHandleMap.find(hash);
                    if (resourceSearch != std::end(ResourceCache<HANDLE, TYPE>::resourceHandleMap))
                    {
                        return std::make_pair(false, resourceSearch->second);
                    }
                }
                else
                {
                    requestedLoadSet.insert(hash);
                    HANDLE handle = ResourceCache<HANDLE, TYPE>::getNextHandle();
                    ResourceCache<HANDLE, TYPE>::resourceHandleMap[hash] = handle;
                    ResourceCache<HANDLE, TYPE>::scheduleResource(handle, std::move(load));
                    return std::make_pair(true, handle);
                }

                return std::make_pair(false, HANDLE());
            }

            HANDLE getHandle(std::size_t hash) const
            {
                if (requestedLoadSet.count(hash) > 0)
                {
                    auto resourceSearch = ResourceCache<HANDLE, TYPE>::resourceHandleMap.find(hash);
                    if (resourceSearch != std::end(ResourceCache<HANDLE, TYPE>::resourceHandleMap))
                    {
                        return resourceSearch->second;
                    }
                }

                return HANDLE();
            }
        };

        template <typename HANDLE, typename TYPE>
        class DynamicResourceCache
            : public ResourceCache<HANDLE, TYPE>
        {
        public:
            using TypePtr = ResourceCache<HANDLE, TYPE>::TypePtr;
            using HandleType = ResourceCache<HANDLE, TYPE>::HandleType;

        private:
            tbb::concurrent_unordered_set<std::size_t> requestedLoadSet;
            tbb::concurrent_unordered_map<HANDLE, std::size_t> loadParameters;

        public:
            DynamicResourceCache(ThreadPool& loadPool)
                : ResourceCache<HANDLE, TYPE>(loadPool)
            {
            }

            void clearExtra(void)
            {
                loadParameters.clear();
                requestedLoadSet.clear();
            }

            void setHandle(std::size_t hash, HANDLE handle, TypePtr data)
            {
                requestedLoadSet.insert(hash);
                ResourceCache<HANDLE, TYPE>::resourceHandleMap[hash] = handle;
                ResourceCache<HANDLE, TYPE>::setResource(handle, data);
            }

            std::pair<bool, HANDLE> getHandle(std::size_t hash, std::size_t parameters, std::function<TypePtr(HandleType)>&& load, uint32_t flags, HANDLE *fallback = nullptr)
            {
                HANDLE handle;
                if (requestedLoadSet.count(hash) > 0)
                {
                    auto resourceSearch = ResourceCache<HANDLE, TYPE>::resourceHandleMap.find(hash);
                    if (resourceSearch != std::end(ResourceCache<HANDLE, TYPE>::resourceHandleMap))
                    {
                        HANDLE handle = resourceSearch->second;
                        if (!(flags & Plugin::Resources::Flags::LoadFromCache))
                        {
                            auto loadParametersSearch = loadParameters.find(handle);
                            if (loadParametersSearch == std::end(loadParameters) || loadParametersSearch->second != parameters)
                            {
                                loadParameters[handle] = parameters;
                                if (flags & Plugin::Resources::Flags::LoadImmediately)
                                {
                                    if (ResourceCache<HANDLE, TYPE>::setResource(handle, load(handle), fallback))
                                    {
                                        return std::make_pair(true, handle);
                                    }
                                }
                                else
                                {
                                    ResourceCache<HANDLE, TYPE>::scheduleResource(handle, std::move(load), fallback);
                                    return std::make_pair(true, handle);
                                }
                            }
                        }

                        return std::make_pair(false, handle);
                    }
                }
                else
                {
                    requestedLoadSet.insert(hash);
                    HANDLE handle = ResourceCache<HANDLE, TYPE>::getNextHandle();
                    ResourceCache<HANDLE, TYPE>::resourceHandleMap[hash] = handle;
                    loadParameters[handle] = parameters;
                    if (flags & Plugin::Resources::Flags::LoadImmediately)
                    {
                        if (ResourceCache<HANDLE, TYPE>::setResource(handle, load(handle)))
                        {
                            return std::make_pair(true, handle);
                        }
                    }
                    else
                    {
                        ResourceCache<HANDLE, TYPE>::scheduleResource(handle, std::move(load));
                        return std::make_pair(true, handle);
                    }
                }

                return std::make_pair(false, *fallback);
            }

            HANDLE getHandle(std::size_t hash) const
            {
                if (requestedLoadSet.count(hash) > 0)
                {
                    auto resourceSearch = ResourceCache<HANDLE, TYPE>::resourceHandleMap.find(hash);
                    if (resourceSearch != std::end(ResourceCache<HANDLE, TYPE>::resourceHandleMap))
                    {
                        return resourceSearch->second;
                    }
                }

                return HANDLE();
            }
        };

        template <typename HANDLE, typename TYPE>
        class ProgramResourceCache
            : public ResourceCache<HANDLE, TYPE>
        {
        public:
            using TypePtr = ResourceCache<HANDLE, TYPE>::TypePtr;
            using HandleType = ResourceCache<HANDLE, TYPE>::HandleType;

        public:
            ProgramResourceCache(ThreadPool& loadPool)
                : ResourceCache<HANDLE, TYPE>(loadPool)
            {
            }

            HANDLE getHandle(std::function<TypePtr(HandleType)>&& load)
            {
                HANDLE handle;
                handle = ResourceCache<HANDLE, TYPE>::getNextHandle();
                ResourceCache<HANDLE, TYPE>::scheduleResource(handle, std::move(load));
                return handle;
            }
        };

        template <typename HANDLE, typename TYPE>
        class StaticProgramResourceCache
            : public ResourceCache<HANDLE, TYPE>
        {
        public:
            StaticProgramResourceCache(ThreadPool& loadPool)
                : ResourceCache<HANDLE, TYPE>(loadPool)
            {
            }

            template <typename FUNCTOR>
            HANDLE getHandle(FUNCTOR&& load)
            {
                HANDLE handle;
                handle = ResourceCache<HANDLE, TYPE>::getNextHandle();
                ResourceCache<HANDLE, TYPE>::setResource(handle, load(handle));
                return handle;
            }
        };

        template <typename HANDLE, typename TYPE>
        class ReloadResourceCache
            : public ResourceCache<HANDLE, TYPE>
        {
        public:
            using TypePtr = ResourceCache<HANDLE, TYPE>::TypePtr;
            using HandleType = ResourceCache<HANDLE, TYPE>::HandleType;

        private:
            tbb::concurrent_unordered_set<std::size_t> requestedLoadSet;

        public:
            ReloadResourceCache(ThreadPool& loadPool)
                : ResourceCache<HANDLE, TYPE>(loadPool)
            {
            }

            void reload(void)
            {
                for (auto& resourceSearch : ResourceCache<HANDLE, TYPE>::resourceMap)
                {
                    auto resource = std::atomic_load(&resourceSearch.second);
                    if (resource)
                    {
                        resource->reload();
                    }
                }
            }

            void clearExtra(void)
            {
                requestedLoadSet.clear();
            }

            std::pair<bool, HANDLE> getHandle(std::size_t hash, std::function<TypePtr(HandleType)>&& load)
            {
                HANDLE handle;
                if (requestedLoadSet.count(hash) > 0)
                {
                    auto resourceSearch = ResourceCache<HANDLE, TYPE>::resourceHandleMap.find(hash);
                    if (resourceSearch != std::end(ResourceCache<HANDLE, TYPE>::resourceHandleMap))
                    {
                        return std::make_pair(false, resourceSearch->second);
                    }
                }
                else
                {
                    requestedLoadSet.insert(hash);
                    HANDLE handle = ResourceCache<HANDLE, TYPE>::getNextHandle();
                    ResourceCache<HANDLE, TYPE>::resourceHandleMap[hash] = handle;
                    ResourceCache<HANDLE, TYPE>::setResource(handle, load(handle));
                    return std::make_pair(true, handle);
                }

                return std::make_pair(false, HANDLE());
            }

            HANDLE getHandle(std::size_t hash) const
            {
                if (requestedLoadSet.count(hash) > 0)
                {
                    auto resourceSearch = ResourceCache<HANDLE, TYPE>::resourceHandleMap.find(hash);
                    if (resourceSearch != std::end(ResourceCache<HANDLE, TYPE>::resourceHandleMap))
                    {
                        return resourceSearch->second;
                    }
                }

                return HANDLE();
            }
        };

        template <typename TYPE>
        struct ObjectCache
        {
            std::vector<TYPE*> objectList;

            template <typename INPUT, typename HANDLE, typename SOURCE>
            bool set(std::vector<INPUT>const& inputList, ResourceCache<HANDLE, SOURCE>& cache, TYPE* defaultObject = nullptr)
            {
                const auto listCount = inputList.size();
                objectList.reserve(std::max(listCount, objectList.size()));
                objectList.resize(listCount);

                for (uint32_t object = 0; object < listCount; ++object)
                {
                    auto& input = inputList[object];
                    if (input)
                    {
                        auto resource = cache.getResource(inputList[object]);
                        if (!resource)
                        {
                            return false;
                        }

                        objectList[object] = dynamic_cast<TYPE*>(resource);
                        if (!objectList[object])
                        {
                            return false;
                        }
                    }
                    else
                    {
                        objectList[object] = defaultObject;
                    }
                }

                return true;
            }

            template <typename INPUT, typename HANDLE>
            bool set(std::vector<INPUT> const& inputList, ResourceCache<HANDLE, TYPE>& cache)
            {
                const auto listCount = inputList.size();
                objectList.reserve(std::max(listCount, objectList.size()));
                objectList.resize(listCount);

                for (uint32_t object = 0; object < listCount; ++object)
                {
                    auto resource = cache.getResource(inputList[object]);
                    if (!resource)
                    {
                        return false;
                    }

                    objectList[object] = resource;
                }

                return true;
            }

            std::vector<TYPE*>& get(void)
            {
                return objectList;
            }

            const std::vector<TYPE*>& get(void) const
            {
                return objectList;
            }
        };

        GEK_CONTEXT_USER(Resources, Engine::Core*)
            , public Engine::Resources
        {
        private:
            Engine::Core * core = nullptr;
            Video::Device* videoDevice = nullptr;
            Plugin::Renderer* renderer = nullptr;

            ThreadPool loadPool;
            std::recursive_mutex shaderMutex;

            StaticProgramResourceCache<ProgramHandle, Video::Program> staticProgramCache;
            ProgramResourceCache<ProgramHandle, Video::Program> programCache;
            GeneralResourceCache<VisualHandle, Engine::Visual> visualCache;
            GeneralResourceCache<MaterialHandle, Engine::Material> materialCache;
            ReloadResourceCache<ShaderHandle, Engine::Shader> shaderCache;
            ReloadResourceCache<ResourceHandle, Engine::Filter> filterCache;
            DynamicResourceCache<ResourceHandle, Video::Object> dynamicCache;
            GeneralResourceCache<RenderStateHandle, Video::RenderState> renderStateCache;
            GeneralResourceCache<DepthStateHandle, Video::DepthState> depthStateCache;
            GeneralResourceCache<BlendStateHandle, Video::BlendState> blendStateCache;

            tbb::concurrent_unordered_map<MaterialHandle, ShaderHandle> materialShaderMap;
            tbb::concurrent_unordered_map<ResourceHandle, Video::Texture::Description> textureDescriptionMap;
            tbb::concurrent_unordered_map<ResourceHandle, Video::Buffer::Description> bufferDescriptionMap;

            struct Validate
            {
                bool state;
                Validate(void)
                    : state(false)
                {
                }

                operator bool()
                {
                    return state;
                }

                bool operator = (bool state)
                {
                    Validate::state = state;
                    return state;
                }
            };

            Validate drawPrimitiveValid;
            Validate dispatchValid;

        public:
            Resources(Context* context, Engine::Core* core)
                : ContextRegistration(context)
                , core(core)
                , videoDevice(core->getVideoDevice())
                , staticProgramCache(loadPool)
                , programCache(loadPool)
                , visualCache(loadPool)
                , materialCache(loadPool)
                , shaderCache(loadPool)
                , filterCache(loadPool)
                , dynamicCache(loadPool)
                , renderStateCache(loadPool)
                , depthStateCache(loadPool)
                , blendStateCache(loadPool)
                , loadPool(5)
            {
                assert(core);
                assert(videoDevice);

                core->onChangedDisplay.connect(this, &Resources::onReload);
                core->onChangedSettings.connect(this, &Resources::onReload);
                core->onInitialized.connect(this, &Resources::onInitialized);
                core->onShutdown.connect(this, &Resources::onShutdown);
            }

            Validate& getValid(Video::Device::Context::Pipeline* videoPipeline)
            {
                assert(videoPipeline);

                return (videoPipeline->getType() == Video::Device::Context::Pipeline::Type::Compute ? dispatchValid : drawPrimitiveValid);
            }

            // Renderer
            bool showResources = false;
            void onShowUserInterface(void)
            {
                ImGuiIO& imGuiIo = ImGui::GetIO();
                auto mainMenu = ImGui::FindWindowByName("##MainMenuBar");
                auto mainMenuShowing = (mainMenu ? mainMenu->Active : false);
                if (mainMenuShowing)
                {
                    ImGui::BeginMainMenuBar();
                    ImGui::PushStyleVar(ImGuiStyleVar_ItemInnerSpacing, ImVec2(5.0f, 10.0f));
                    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(5.0f, 10.0f));
                    if (ImGui::BeginMenu("Edit"))
                    {
                        ImGui::MenuItem("Show Resources", nullptr, &showResources);
                        if (ImGui::MenuItem("Reload Resources"))
                        {
                            onReload();
                        }

                        ImGui::EndMenu();
                    }

                    ImGui::PopStyleVar(2);
                    ImGui::EndMainMenuBar();
                }

                if (showResources)
                {
                    ImGui::SetNextWindowSize(ImVec2(500.0f, 350.0f), ImGuiCond_Once);
                    if (ImGui::Begin("Resources"))
                    {
                        showProgramCache();
                        showVisualCache();
                        showMaterialCache();
                        showShaderCache();
                        showFilterCache();
                        showDynamicCache();
                        showRenderStateCache();
                        showDepthStateCache();
                        showBlendStateCache();
                    }

                    ImGui::End();
                }
            }

            template <typename CACHE, typename FUNCTOR>
            void showObjectMap(CACHE& cache, std::string_view group, FUNCTOR&& onObject)
            {
                if (ImGui::TreeNodeEx(group.data(), ImGuiTreeNodeFlags_Framed))
                {
                    cache.visit([&](CACHE::HandleType handle, CACHE::TypePtr object) -> void
                    {
                        auto nodeName = fmt::format("{}_{}", object->getName(), static_cast<uint64_t>(handle.identifier));
                        if (ImGui::TreeNodeEx(nodeName.data(), ImGuiTreeNodeFlags_Framed))
                        {
                            onObject(object);
                            ImGui::TreePop();
                        }
                    });

                    ImGui::TreePop();
                }
            }

            template <typename HANDLE, typename TYPE, typename FUNCTOR>
            void showVideoResourceMap(ResourceCache<HANDLE, TYPE>* cache, std::string_view group, FUNCTOR&& onObject)
            {
                if (ImGui::TreeNodeEx(group.data(), ImGuiTreeNodeFlags_Framed))
                {
                    std::unordered_map<std::string_view, std::vector<std::pair<HANDLE, std::shared_ptr<TYPE>>>> typeDataMap;
                    cache->visit([&](ResourceCache<HANDLE, TYPE>::HandleType handle, ResourceCache<HANDLE, TYPE>::TypePtr object) -> void
                    {
                        if (dynamic_cast<Video::Target*>(object.get()))
                        {
                            typeDataMap["Target"].push_back(std::make_pair(handle, object));
                        }
                        else if (dynamic_cast<Video::Texture*>(object.get()))
                        {
                            typeDataMap["Texture"].push_back(std::make_pair(handle, object));
                        }
                        else if (dynamic_cast<Video::Buffer*>(object.get()))
                        {
                            typeDataMap["Buffer"].push_back(std::make_pair(handle, object));
                        }
                        else
                        {
                            auto program = dynamic_cast<Video::Program*>(object.get());
                            if (program)
                            {
                                static const std::unordered_map<Video::Program::Type, std::string_view> ProgramTypeMap =
                                {
                                    { Video::Program::Type::Compute, "Compute" },
                                    { Video::Program::Type::Geometry, "Geometry" },
                                    { Video::Program::Type::Vertex, "Vertex" },
                                    { Video::Program::Type::Pixel, "Pixel" },
                                };

                                auto typeSearch = ProgramTypeMap.find(program->getInformation().type);
                                if (typeSearch != std::end(ProgramTypeMap))
                                {
                                    typeDataMap[typeSearch->second].push_back(std::make_pair(handle, object));
                                }
                            }
                        }
                    });

                    for (auto& typePair : typeDataMap)
                    {
                        if (ImGui::TreeNodeEx(typePair.first.data(), ImGuiTreeNodeFlags_Framed))
                        {
                            for (auto& resourcePair : typePair.second)
                            {
                                auto handle = resourcePair.first;
                                auto& object = resourcePair.second;
                                auto nodeName = fmt::format("{} - {}", object->getName(), static_cast<uint64_t>(handle.identifier));
                                if (ImGui::TreeNodeEx(nodeName.data(), ImGuiTreeNodeFlags_Framed))
                                {
                                    onObject(object);
                                    ImGui::TreePop();
                                }
                            }

                            ImGui::TreePop();
                        }
                    }

                    ImGui::TreePop();
                }
            }

            void showVisualCache(void)
            {
                showObjectMap(visualCache, "Visuals"s, [&](auto& object) -> void
                {
                });
            }

            void showShaderCache(void)
            {
                showObjectMap(shaderCache, "Shaders"s, [&](auto& object) -> void
                {
                });
            }

            void showFilterCache(void)
            {
                showObjectMap(filterCache, "Filters"s, [&](auto& object) -> void
                {
                });
            }

            void showProgramCache(void)
            {
                if (ImGui::TreeNodeEx("Programs", ImGuiTreeNodeFlags_Framed))
                {
                    showVideoResourceMap(&staticProgramCache, "Global"s, [&](auto& object) -> void
                    {
                        auto information = object->getInformation();
                        auto pathString = information.debugPath.getString();
                        ImGui::PushItemWidth(-1.0f);
                        ImGui::InputText("##path", const_cast<char*>(pathString.data()), pathString.size(), ImGuiInputTextFlags_ReadOnly);
                        ImGui::InputTextMultiline("##data", const_cast<char*>(information.uncompiledData.data()), information.uncompiledData.size(), ImVec2(-1.0f, 500.0f), ImGuiInputTextFlags_ReadOnly);
                        ImGui::PopItemWidth();
                    });

                    showVideoResourceMap(&programCache, "Local"s, [&](auto& object) -> void
                    {
                        auto information = object->getInformation();
                        auto pathString = information.debugPath.getString();
                        ImGui::PushItemWidth(-1.0f);
                        ImGui::InputText("##path", const_cast<char*>(pathString.data()), pathString.size(), ImGuiInputTextFlags_ReadOnly);
                        ImGui::InputTextMultiline("##data", const_cast<char*>(information.uncompiledData.data()), information.uncompiledData.size(), ImVec2(-1.0f, 500.0f), ImGuiInputTextFlags_ReadOnly);
                        ImGui::PopItemWidth();
                    });

                    ImGui::TreePop();
                }
            }

            void showMaterialCache(void)
            {
                showObjectMap(materialCache, "Materials"s, [&](auto& object) -> void
                {
                });
            }

            void showResourceValue(std::string_view text, std::string_view label, const std::string& value)
            {
                ImGui::AlignTextToFramePadding();
                ImGui::TextUnformatted(text.data());
                ImGui::SameLine();
                ImGui::PushItemWidth(-1.0f);
                UI::InputString(label.data(), value, 0);
                ImGui::PopItemWidth();
            }

            void showDynamicCache(void)
            {
                showVideoResourceMap(&dynamicCache, "Resources"s, [&](auto& object) -> void
                {
                    if (dynamic_cast<Video::Texture*>(object.get()))
                    {
                        float start = ImGui::GetCursorPosY();

                        ImGui::BeginGroup();
                        auto texture = dynamic_cast<Video::Texture*>(object.get());
                        auto const& description = texture->getDescription();
                        float width = 32.0f;
                        float ratio = (width / float(description.width));
                        float height = (float(description.height) * ratio);
                        ImGui::Image(reinterpret_cast<ImTextureID>(texture), ImVec2(width, height), ImVec2(0, 0), ImVec2(1, 1), ImVec4(1, 1, 1, 1), ImVec4(0, 0, 0, 1));
                        ImGui::EndGroup();

                        ImGui::SetCursorPosY(start);
                        ImGui::Indent(40.0f);
                        ImGui::BeginGroup();
                        showResourceValue("Format", "##format", Video::GetFormat(description.format));
                        showResourceValue("Width", "##width", std::to_string(description.width));
                        showResourceValue("Height", "##height", std::to_string(description.height));
                        showResourceValue("Depth", "##depth", std::to_string(description.depth));
                        showResourceValue("MipMap Levels", "##mipMapCount", std::to_string(description.mipMapCount));
                        showResourceValue("MultiSample Count", "##sampleCount", std::to_string(description.sampleCount));
                        showResourceValue("MultiSample Quality", "##sampleQuality", std::to_string(description.sampleQuality));
                        showResourceValue("Flags", "##flags", std::to_string(description.flags));
                        ImGui::EndGroup();
                        ImGui::Unindent(40.0f);
                    }
                    else if (dynamic_cast<Video::Buffer*>(object.get()))
                    {
                        auto buffer = dynamic_cast<Video::Buffer*>(object.get());
                        auto const& description = buffer->getDescription();
                        showResourceValue("Format", "##format", Video::GetFormat(description.format));
                        showResourceValue("Count", "##count", std::to_string(description.count));
                        showResourceValue("Stride", "##stride", std::to_string(description.stride));
                        showResourceValue("Flags", "##flags", std::to_string(description.flags));
                        showResourceValue("Type", "##type", Video::Buffer::GetType(description.type));
                    }
                });
            }

            void showRenderStateCache(void)
            {
                showObjectMap(renderStateCache, "Render States"s, [&](auto& object) -> void
                {
                    auto const& description = object->getDescription();
                    showResourceValue("Fill Mode", "##fillMode", Video::RenderState::GetFillMode(description.fillMode));
                    showResourceValue("Cull Mode", "##cullMode", Video::RenderState::GetCullMode(description.cullMode));
                    showResourceValue("Front Counter Clockwise", "##frontCounterClockwise", std::to_string(description.frontCounterClockwise));
                    showResourceValue("Depth Bias", "##depthBias", std::to_string(description.depthBias));
                    showResourceValue("Depth Bias Clamp", "##depthBiasClamp", std::to_string(description.depthBiasClamp));
                    showResourceValue("Slope Scaled Depth Bias", "##slopeScaledDepthBias", std::to_string(description.slopeScaledDepthBias));
                    showResourceValue("Depth Clip Enable", "##depthClipEnable", std::to_string(description.depthClipEnable));
                    showResourceValue("Scissor Enable", "##scissorEnable", std::to_string(description.scissorEnable));
                    showResourceValue("Multisample Enable", "##multisampleEnable", std::to_string(description.multisampleEnable));
                    showResourceValue("AntiAliased Line Enable", "##antialiasedLineEnable", std::to_string(description.antialiasedLineEnable));
                });
            }

            void showStencilState(std::string_view text, Video::DepthState::Description::StencilState const& stencilState)
            {
                if (ImGui::TreeNodeEx(text.data(), ImGuiTreeNodeFlags_Framed))
                {
                    showResourceValue("failOperation", "##failOperation", Video::DepthState::GetOperation(stencilState.failOperation));
                    showResourceValue("depthFailOperation", "##depthFailOperation", Video::DepthState::GetOperation(stencilState.depthFailOperation));
                    showResourceValue("passOperation", "##passOperation", Video::DepthState::GetOperation(stencilState.passOperation));
                    showResourceValue("comparisonFunction", "##comparisonFunction", Video::GetComparisonFunction(stencilState.comparisonFunction));
                    ImGui::TreePop();
                }
            }

            void showDepthStateCache(void)
            {
                showObjectMap(depthStateCache, "Depth States"s, [&](auto& object) -> void
                {
                    auto const& description = object->getDescription();
                    showResourceValue("Enable", "##enable", std::to_string(description.enable));
                    showResourceValue("Write Mask", "##writeMask", Video::DepthState::GetWrite(description.writeMask));
                    showResourceValue("Comparison Function", "##comparisonFunction", Video::GetComparisonFunction(description.comparisonFunction));
                    showResourceValue("Stencil Enable", "##stencilEnable", std::to_string(description.stencilEnable));
                    showResourceValue("Stencil Read Mask", "##stencilReadMask", std::to_string(description.stencilReadMask));
                    showResourceValue("Stencil Write Mask", "##stencilWriteMask", std::to_string(description.stencilWriteMask));
                    showStencilState("Stencil Front State", description.stencilFrontState);
                    showStencilState("Stencil Back State", description.stencilBackState);
                });
            }

            int currentBlendStateTarget = 0;
            void showBlendStateCache(void)
            {
                showObjectMap(blendStateCache, "Blend States"s, [&](auto& object) -> void
                {
                    auto const& description = object->getDescription();
                    showResourceValue("Alpha To Coverage", "##alphaToCoverage", std::to_string(description.alphaToCoverage));
                    showResourceValue("Independent Blend States", "##independentBlendStates", std::to_string(description.independentBlendStates));

                    ImGui::TextUnformatted("Target State");
                    ImGui::SameLine();
                    ImGui::PushItemWidth(-1.0f);
                    ImGui::SliderInt("##targetState", &currentBlendStateTarget, 0, 7);
                    ImGui::PopItemWidth();

                    auto const& targetState = description.targetStates[currentBlendStateTarget];
                    ImGui::Indent();
                    showResourceValue("enable", "##enable", std::to_string(targetState.enable));
                    showResourceValue("Color Source", "##colorSource", Video::BlendState::GetSource(targetState.colorSource));
                    showResourceValue("Color Destination", "##colorDestination", Video::BlendState::GetSource(targetState.colorDestination));
                    showResourceValue("Color Operation", "##colorOperation", Video::BlendState::GetOperation(targetState.colorOperation));
                    showResourceValue("Alpha Source", "##alphaSource", Video::BlendState::GetSource(targetState.alphaSource));
                    showResourceValue("Alpha Destination", "##alphaDestination", Video::BlendState::GetSource(targetState.alphaDestination));
                    showResourceValue("Alpha Operation", "##alphaOperation", Video::BlendState::GetOperation(targetState.alphaOperation));
                    showResourceValue("Write Mask", "##writeMask", Video::BlendState::GetMask(targetState.writeMask));
                    ImGui::Unindent();
                });
            }

            // Plugin::Processor
            void onInitialized(void)
            {
                renderer = core->getRenderer();
                renderer->onShowUserInterface.connect(this, &Resources::onShowUserInterface);
            }

            void onShutdown(void)
            {
                loadPool.drain();
                if (renderer)
                {
                    renderer->onShowUserInterface.disconnect(this, &Resources::onShowUserInterface);
                }
            }

            // Plugin::Core Slots
            void onReload(void)
            {
                programCache.clear();
                shaderCache.reload();
                filterCache.reload();
            }

            // Plugin::Resources
            VisualHandle loadVisual(std::string_view visualName)
            {
                auto hash = GetHash(visualName);
                return visualCache.getHandle(hash, [context = getContext(), videoDevice = videoDevice, resources = dynamic_cast<Engine::Resources*>(this), visualName = std::string(visualName)](VisualHandle)->Engine::VisualPtr
                {
                    return context->createClass<Engine::Visual>("Engine::Visual", videoDevice, resources, visualName);
                }).second;
            }

            MaterialHandle loadMaterial(std::string_view materialName)
            {
                auto hash = GetHash(materialName);
                return materialCache.getHandle(hash, [context = getContext(), videoDevice = videoDevice, resources = dynamic_cast<Engine::Resources*>(this), materialName = std::string(materialName)](MaterialHandle handle)->Engine::MaterialPtr
                {
                    return context->createClass<Engine::Material>("Engine::Material", resources, materialName, handle);
                }).second;
            }

            ResourceHandle loadTexture(std::string_view textureName, uint32_t flags, ResourceHandle fallback)
            {
                // iterate over formats in case the texture name has no extension
                static constexpr std::string_view formatList[] =
                {
                    "",
                    ".dds",
                    ".tga",
                    ".png",
                    ".jpg",
                    ".jpeg",
                    ".tif",
                    ".tiff",
                    ".bmp",
                };

                auto hash = GetHash(textureName);
                for (auto const& format : formatList)
                {
                    auto texturePath(getContext()->findDataPath(FileSystem::CreatePath("textures", textureName).withExtension(format)));
                    if (texturePath.isFile())
                    {
                        auto resource = dynamicCache.getHandle(hash, flags, [this, texturePath = texturePath, flags](ResourceHandle)->Video::TexturePtr
                        {
                            return videoDevice->loadTexture(texturePath, flags);
                        }, 0, &fallback);

                        if (resource.first)
                        {
                            auto description = videoDevice->loadTextureDescription(texturePath);
                            textureDescriptionMap.insert(std::make_pair(resource.second, description));
                        }

                        return resource.second;
                    }
                }

                return ResourceHandle();
            }

            ResourceHandle createPattern(std::string_view pattern, JSON::Object const& parameters)
            {
                auto lowerPattern = String::GetLower(pattern);

                std::vector<uint8_t> data;
                Video::Texture::Description description;
                std::string parameterString;
                if (lowerPattern == "color")
                {
                    description.width = 1;
                    description.height = 1;
                    if (parameters.is_array())
                    {
                        switch (parameters.size())
                        {
                        case 1:
                            data.push_back(JSON::Evaluate(parameters[0], shuntingYard, 255));
                            description.format = Video::Format::R8_UNORM;
                            parameterString = fmt::format("color[{}]", data[0]);
                            break;

                        case 2:
                            data.push_back(JSON::Evaluate(parameters[0], shuntingYard, 255));
                            data.push_back(JSON::Evaluate(parameters[1], shuntingYard, 255));
                            description.format = Video::Format::R8G8_UNORM;
                            parameterString = fmt::format("color2[{}, {}]", data[0], data[1]);
                            break;

                        case 3:
                            data.push_back(JSON::Evaluate(parameters[0], shuntingYard, 255));
                            data.push_back(JSON::Evaluate(parameters[1], shuntingYard, 255));
                            data.push_back(JSON::Evaluate(parameters[2], shuntingYard, 255));
                            data.push_back(0);
                            description.format = Video::Format::R8G8B8A8_UNORM;
                            parameterString = fmt::format("color3[{}, {}, {}]", data[0], data[1], data[2]);
                            break;

                        case 4:
                            data.push_back(JSON::Evaluate(parameters[0], shuntingYard, 255));
                            data.push_back(JSON::Evaluate(parameters[1], shuntingYard, 255));
                            data.push_back(JSON::Evaluate(parameters[2], shuntingYard, 255));
                            data.push_back(JSON::Evaluate(parameters[3], shuntingYard, 255));
                            description.format = Video::Format::R8G8B8A8_UNORM;
                            parameterString = fmt::format("color4[{}, {}, {}, {}]", data[0], data[1], data[2], data[3]);
                            break;
                        };
                    }
                    else
                    {
                        union
                        {
                            float value;
                            uint8_t quarters[4];
                        };

                        value = JSON::Evaluate(parameters, shuntingYard, 1.0f);
                        data.push_back(quarters[0]);
                        data.push_back(quarters[1]);
                        data.push_back(quarters[2]);
                        data.push_back(quarters[3]);
                        description.format = Video::Format::R32_FLOAT;
                        parameterString = fmt::format("float[{}]", value);
                    }
                }
                else if (lowerPattern == "normal")
                {
                    union
                    {
                        uint16_t halves[2];
                        uint8_t quarters[4];
                    };

                    Math::Float3 normal = JSON::Evaluate(parameters, shuntingYard, Math::Float3::Zero);
                    parameterString = fmt::format("normal[{}, {}, {}]", normal.x, normal.y, normal.z);

                    Float16Compressor compressor;
                    halves[0] = compressor.compress(normal.x);
                    halves[1] = compressor.compress(normal.y);
                    data.push_back(quarters[0]);
                    data.push_back(quarters[1]);
                    data.push_back(quarters[2]);
                    data.push_back(quarters[3]);
                    description.format = Video::Format::R16G16_FLOAT;
                }
                else if (lowerPattern == "system" && parameters.is_string())
                {
                    parameterString = String::GetLower(JSON::Value(parameters, "debug"s));
                    if (parameterString == "debug")
                    {
                        data.push_back(255);    data.push_back(0);      data.push_back(255);    data.push_back(255);
                        data.push_back(255);    data.push_back(255);    data.push_back(255);    data.push_back(255);
                        data.push_back(255);    data.push_back(255);    data.push_back(255);    data.push_back(255);
                        data.push_back(255);    data.push_back(0);      data.push_back(255);    data.push_back(255);
                        description.format = Video::Format::R8G8B8A8_UNORM;
                        description.width = 2;
                        description.height = 2;
                    }
                    else if (parameterString == "flat")
                    {
                        data.push_back(127);
                        data.push_back(127);
                        data.push_back(255);
                        data.push_back(255);
                        description.format = Video::Format::R8G8B8A8_UNORM;
                    }
                }

                description.name = fmt::format("{}:{}", lowerPattern, parameterString);
                description.flags = Video::Texture::Flags::Resource;

                auto resource = dynamicCache.getHandle(GetHash(description.name), description.getHash(), [this, description, data = move(data)](ResourceHandle)->Video::TexturePtr
                {
                    return videoDevice->createTexture(description, data.data());
                }, 0);

                if (resource.first)
                {
                    textureDescriptionMap.insert(std::make_pair(resource.second, description));
                }

                return resource.second;
            }

            ResourceHandle createTexture(Video::Texture::Description const& description, uint32_t flags)
            {
                if (description.format == Video::Format::Unknown)
                {
                    flags |= Resources::Flags::LoadFromCache;
                }

                auto resource = dynamicCache.getHandle(GetHash(description.name), description.getHash(), [this, description](ResourceHandle)->Video::TexturePtr
                {
                    return videoDevice->createTexture(description);
                }, flags);

                if (resource.first)
                {
                    textureDescriptionMap.insert(std::make_pair(resource.second, description));
                }

                return resource.second;
            }

            ResourceHandle createBuffer(Video::Buffer::Description const& description, uint32_t flags)
            {
                assert(description.count > 0);

                if (description.format == Video::Format::Unknown)
                {
                    flags |= Resources::Flags::LoadFromCache;
                }

                auto resource = dynamicCache.getHandle(GetHash(description.name), description.getHash(), [this, description](ResourceHandle)->Video::BufferPtr
                {
                    return videoDevice->createBuffer(description);
                }, flags);

                if (resource.first)
                {
                    bufferDescriptionMap.insert(std::make_pair(resource.second, description));
                }

                return resource.second;
            }

            ResourceHandle createBuffer(Video::Buffer::Description const& description, std::vector<uint8_t>&& staticData, uint32_t flags)
            {
                assert(description.count > 0);
                assert(!staticData.empty());

                if (description.format == Video::Format::Unknown)
                {
                    flags |= Resources::Flags::LoadFromCache;
                }

                auto parameters = reinterpret_cast<std::size_t>(staticData.data());
                auto resource = dynamicCache.getHandle(GetHash(description.name), parameters, [this, description, staticData = move(staticData)](ResourceHandle)->Video::BufferPtr
                {
                    return videoDevice->createBuffer(description, (void*)staticData.data());
                }, flags);

                if (resource.first)
                {
                    bufferDescriptionMap.insert(std::make_pair(resource.second, description));
                }

                return resource.second;
            }

            void setIndexBuffer(Video::Device::Context* videoContext, ResourceHandle resourceHandle, uint32_t offset)
            {
                assert(videoContext);

                if (drawPrimitiveValid)
                {
                    auto resource = getResource(resourceHandle);
                    if (drawPrimitiveValid = (resource != nullptr))
                    {
                        videoContext->setIndexBuffer(dynamic_cast<Video::Buffer*>(resource), offset);
                    }
                }
            }

            ObjectCache<Video::Buffer> vertexBufferCache;
            void setVertexBufferList(Video::Device::Context* videoContext, std::vector<ResourceHandle> const& resourceHandleList, uint32_t firstSlot, uint32_t* offsetList)
            {
                assert(videoContext);

                if (drawPrimitiveValid && (drawPrimitiveValid = vertexBufferCache.set(resourceHandleList, dynamicCache)))
                {
                    videoContext->setVertexBufferList(vertexBufferCache.get(), firstSlot, offsetList);
                }
            }

            ObjectCache<Video::Buffer> constantBufferCache;
            void setConstantBufferList(Video::Device::Context::Pipeline* videoPipeline, std::vector<ResourceHandle> const& resourceHandleList, uint32_t firstStage)
            {
                assert(videoPipeline);

                auto& valid = getValid(videoPipeline);
                if (valid && (valid = constantBufferCache.set(resourceHandleList, dynamicCache)))
                {
                    videoPipeline->setConstantBufferList(constantBufferCache.get(), firstStage);
                }
            }

            ObjectCache<Video::Object> resourceCache;
            void setResourceList(Video::Device::Context::Pipeline* videoPipeline, std::vector<ResourceHandle> const& resourceHandleList, uint32_t firstStage)
            {
                assert(videoPipeline);

                auto& valid = getValid(videoPipeline);
                if (valid && (valid = resourceCache.set(resourceHandleList, dynamicCache, videoDevice->getBackBuffer())))
                {
                    videoPipeline->setResourceList(resourceCache.get(), firstStage);
                }
            }

            ObjectCache<Video::Object> unorderedAccessCache;
            void setUnorderedAccessList(Video::Device::Context::Pipeline* videoPipeline, std::vector<ResourceHandle> const& resourceHandleList, uint32_t firstStage)
            {
                assert(videoPipeline);

                auto& valid = getValid(videoPipeline);
                if (valid && (valid = unorderedAccessCache.set(resourceHandleList, dynamicCache)))
                {
                    videoPipeline->setUnorderedAccessList(unorderedAccessCache.get(), firstStage);
                }
            }

            void clearIndexBuffer(Video::Device::Context* videoContext)
            {
                assert(videoContext);

                videoContext->clearIndexBuffer();
            }

            void clearVertexBufferList(Video::Device::Context* videoContext, uint32_t count, uint32_t firstSlot)
            {
                assert(videoContext);

                videoContext->clearVertexBufferList(count, firstSlot);
            }

            void clearConstantBufferList(Video::Device::Context::Pipeline* videoPipeline, uint32_t count, uint32_t firstStage)
            {
                assert(videoPipeline);

                videoPipeline->clearConstantBufferList(count, firstStage);
            }

            void clearResourceList(Video::Device::Context::Pipeline* videoPipeline, uint32_t count, uint32_t firstStage)
            {
                assert(videoPipeline);

                videoPipeline->clearResourceList(count, firstStage);
            }

            void clearUnorderedAccessList(Video::Device::Context::Pipeline* videoPipeline, uint32_t count, uint32_t firstStage)
            {
                assert(videoPipeline);

                videoPipeline->clearUnorderedAccessList(count, firstStage);
            }

            void drawPrimitive(Video::Device::Context* videoContext, uint32_t vertexCount, uint32_t firstVertex)
            {
                assert(videoContext);

                if (drawPrimitiveValid)
                {
                    videoContext->drawPrimitive(vertexCount, firstVertex);
                }
            }

            void drawInstancedPrimitive(Video::Device::Context* videoContext, uint32_t instanceCount, uint32_t firstInstance, uint32_t vertexCount, uint32_t firstVertex)
            {
                if (drawPrimitiveValid)
                {
                    videoContext->drawInstancedPrimitive(instanceCount, firstInstance, vertexCount, firstVertex);
                }
            }

            void drawIndexedPrimitive(Video::Device::Context* videoContext, uint32_t indexCount, uint32_t firstIndex, uint32_t firstVertex)
            {
                assert(videoContext);

                if (drawPrimitiveValid)
                {
                    videoContext->drawIndexedPrimitive(indexCount, firstIndex, firstVertex);
                }
            }

            void drawInstancedIndexedPrimitive(Video::Device::Context* videoContext, uint32_t instanceCount, uint32_t firstInstance, uint32_t indexCount, uint32_t firstIndex, uint32_t firstVertex)
            {
                assert(videoContext);

                if (drawPrimitiveValid)
                {
                    videoContext->drawInstancedIndexedPrimitive(instanceCount, firstInstance, indexCount, firstIndex, firstVertex);
                }
            }

            void dispatch(Video::Device::Context* videoContext, uint32_t threadGroupCountX, uint32_t threadGroupCountY, uint32_t threadGroupCountZ)
            {
                assert(videoContext);

                if (dispatchValid)
                {
                    videoContext->dispatch(threadGroupCountX, threadGroupCountY, threadGroupCountZ);
                }
            }

            // Engine::Resources
            void clear(void)
            {
                textureDescriptionMap.clear();
                bufferDescriptionMap.clear();
                loadPool.drain();
                materialShaderMap.clear();
                programCache.clear();
                materialCache.clear();
                shaderCache.clear();
                filterCache.clear();
                dynamicCache.clear();
                renderStateCache.clear();
                depthStateCache.clear();
                blendStateCache.clear();
            }

            void reload(void)
            {
                onReload();
            }

            ShaderHandle getMaterialShader(MaterialHandle material) const
            {
                auto shaderSearch = materialShaderMap.find(material);
                if (shaderSearch != std::end(materialShaderMap))
                {
                    return shaderSearch->second;
                }

                return ShaderHandle();
            }

            ResourceHandle getResourceHandle(std::string_view resourceName) const
            {
                return dynamicCache.getHandle(GetHash(resourceName));
            }

            Engine::Shader* const getShader(ShaderHandle handle) const
            {
                return shaderCache.getResource(handle);
            }

            ShaderHandle const getShader(std::string_view shaderName, MaterialHandle material)
            {
                std::unique_lock<std::recursive_mutex> lock(shaderMutex);

                auto hash = GetHash(shaderName);
                auto resource = shaderCache.getHandle(hash, [this, shaderName = std::string(shaderName)](ShaderHandle)->Engine::ShaderPtr
                {
                    return getContext()->createClass<Engine::Shader>("Engine::Shader", core, shaderName);
                });

                if (material && resource.second)
                {
                    materialShaderMap[material] = resource.second;
                }

                return resource.second;
            }

            Engine::Filter* const getFilter(std::string_view filterName)
            {
                auto hash = GetHash(filterName);
                auto resource = filterCache.getHandle(hash, [this, filterName = std::string(filterName)](ResourceHandle)->Engine::FilterPtr
                {
                    return getContext()->createClass<Engine::Filter>("Engine::Filter", core, filterName);
                });

                return filterCache.getResource(resource.second);
            }

            Video::Texture::Description const* const getTextureDescription(ResourceHandle resourceHandle) const
            {
                if (!resourceHandle)
                {
                    return &videoDevice->getBackBuffer()->getDescription();
                }

                auto descriptionSearch = textureDescriptionMap.find(resourceHandle);
                if (descriptionSearch != std::end(textureDescriptionMap))
                {
                    return &descriptionSearch->second;
                }
                else
                {
                    return nullptr;
                }
            }

            Video::Buffer::Description const* const getBufferDescription(ResourceHandle resourceHandle) const
            {
                auto descriptionSearch = bufferDescriptionMap.find(resourceHandle);
                if (descriptionSearch != std::end(bufferDescriptionMap))
                {
                    return &descriptionSearch->second;
                }
                else
                {
                    return nullptr;
                }
            }

            Video::Object* const getResource(ResourceHandle resourceHandle) const
            {
                if (!resourceHandle)
                {
                    return videoDevice->getBackBuffer();
                }

                return dynamicCache.getResource(resourceHandle);
            }

            Video::Program::Information getProgramInformation(Video::Program::Type type, std::string_view name, std::string_view entryFunction, std::string_view engineData)
            {
                auto programsPath(getContext()->findDataPath("programs"s, false));
                auto filePath(programsPath / name);
                auto programDirectory(filePath.getParentPath());
                std::string uncompiledData = filePath.isFile() ? FileSystem::Read(filePath) : engineData.data();

                auto hash = GetHash(name, uncompiledData, engineData);
                auto cachePath = getContext()->getCachePath(FileSystem::CreatePath("programs", name));
                auto uncompiledPath(cachePath.withExtension(fmt::format(".{}.hlsl", hash)));
                auto compiledPath(cachePath.withExtension(fmt::format(".{}.bin", hash)));
                Video::Program::Information information =
                {
                    type
                };

                if (compiledPath.isFile())
                {
                    information.compiledData = FileSystem::Load(compiledPath);
                }

                //if (information.compiledData.empty())
                {
                    std::map<std::string_view, std::string> includedMap;
                    auto onInclude = [programsPath, programDirectory, &includedMap, engineData](Video::IncludeType includeType, std::string_view fileName, void const** data, uint32_t* size) -> bool
                    {
                        std::string includeData;
                        if (String::GetLower(fileName) == "gekengine"s)
                        {
                            (*data) = engineData.data();
                            (*size) = engineData.size();
                            return true;
                        }
                        else
                        {
                            switch (includeType)
                            {
                            case Video::IncludeType::Local:
                                if (true)
                                {
                                    auto localPath(programDirectory / fileName);
                                    if (localPath.isFile())
                                    {
                                        includedMap[fileName] = FileSystem::Read(localPath);
                                        (*data) = includedMap[fileName].data();
                                        (*size) = includedMap[fileName].size();
                                        return true;
                                    }
                                }

                                break;

                            case Video::IncludeType::Global:
                                if (true)
                                {
                                    auto rootPath(programsPath / fileName);
                                    if (rootPath.isFile())
                                    {
                                        includedMap[fileName] = FileSystem::Read(rootPath);
                                        (*data) = includedMap[fileName].data();
                                        (*size) = includedMap[fileName].size();
                                        return true;
                                    }
                                }

                                break;
                            };
                        }

                        return false;
                    };

                    information = videoDevice->compileProgram(type, name, uncompiledPath, uncompiledData, entryFunction, onInclude);
                    FileSystem::Save(uncompiledPath, information.uncompiledData);
                    FileSystem::Save(compiledPath, information.compiledData);
                }
                /*else
                {
                    information.debugPath = uncompiledPath;
                    information.uncompiledData = std::move(uncompiledData);
                }*/

                return information;
            }

            Video::Program* getProgram(Video::Program::Type type, std::string_view name, std::string_view entryFunction, std::string_view engineData)
            {
                auto handle = staticProgramCache.getHandle([this, type, name = std::string(name), entryFunction = std::string(entryFunction), engineData = std::string(engineData)](ProgramHandle)->Video::ProgramPtr
                {
                    auto compiledData = getProgramInformation(type, name, entryFunction, engineData);
                    return videoDevice->createProgram(compiledData);
                });

                return staticProgramCache.getResource(handle);
            }

            ProgramHandle loadProgram(Video::Program::Type type, std::string_view name, std::string_view entryFunction, std::string_view engineData)
            {
                return programCache.getHandle([this, type, name = std::string(name), entryFunction = std::string(entryFunction), engineData = std::string(engineData)](ProgramHandle)->Video::ProgramPtr
                {
                    auto compiledData = getProgramInformation(type, name, entryFunction, engineData);
                    return videoDevice->createProgram(compiledData);
                });
            }

            RenderStateHandle createRenderState(Video::RenderState::Description const& description)
            {
                auto hash = description.getHash();
                return renderStateCache.getHandle(hash, [this, description](RenderStateHandle) -> Video::RenderStatePtr
                {
                    return videoDevice->createRenderState(description);
                }).second;
            }

            DepthStateHandle createDepthState(Video::DepthState::Description const& description)
            {
                auto hash = description.getHash();
                return depthStateCache.getHandle(hash, [this, description](DepthStateHandle) -> Video::DepthStatePtr
                {
                    return videoDevice->createDepthState(description);
                }).second;
            }

            BlendStateHandle createBlendState(Video::BlendState::Description const& description)
            {
                auto hash = description.getHash();
                return blendStateCache.getHandle(hash, [this, description](BlendStateHandle) -> Video::BlendStatePtr
                {
                    return videoDevice->createBlendState(description);
                }).second;
            }

            void generateMipMaps(Video::Device::Context* videoContext, ResourceHandle resourceHandle)
            {
                assert(videoContext);

                auto resource = getResource(resourceHandle);
                if (resource)
                {
                    videoContext->generateMipMaps(dynamic_cast<Video::Texture*>(resource));
                }
            }

            void resolveSamples(Video::Device::Context* videoContext, ResourceHandle destinationHandle, ResourceHandle sourceHandle)
            {
                auto source = getResource(sourceHandle);
                auto destination = getResource(destinationHandle);
                if (source && destination)
                {
                    videoContext->resolveSamples(dynamic_cast<Video::Texture*>(destination), dynamic_cast<Video::Texture*>(source));
                }
            }

            void copyResource(ResourceHandle destinationHandle, ResourceHandle sourceHandle)
            {
                auto source = getResource(sourceHandle);
                auto destination = getResource(destinationHandle);
                if (source && destination)
                {
                    videoDevice->copyResource(destination, source);
                }
            }

            void clearUnorderedAccess(Video::Device::Context* videoContext, ResourceHandle resourceHandle, Math::Float4 const& value)
            {
                assert(videoContext);

                auto resource = getResource(resourceHandle);
                if (resource)
                {
                    videoContext->clearUnorderedAccess(resource, value);
                }
            }

            void clearUnorderedAccess(Video::Device::Context* videoContext, ResourceHandle resourceHandle, Math::UInt4 const& value)
            {
                assert(videoContext);

                auto resource = getResource(resourceHandle);
                if (resource)
                {
                    videoContext->clearUnorderedAccess(resource, value);
                }
            }

            void clearRenderTarget(Video::Device::Context* videoContext, ResourceHandle resourceHandle, Math::Float4 const& color)
            {
                assert(videoContext);

                auto resource = getResource(resourceHandle);
                if (resource)
                {
                    videoContext->clearRenderTarget(dynamic_cast<Video::Target*>(resource), color);
                }
            }

            void clearDepthStencilTarget(Video::Device::Context* videoContext, ResourceHandle depthBufferHandle, uint32_t flags, float clearDepth, uint32_t clearStencil)
            {
                assert(videoContext);

                auto depthBuffer = getResource(depthBufferHandle);
                if (depthBuffer)
                {
                    videoContext->clearDepthStencilTarget(depthBuffer, flags, clearDepth, clearStencil);
                }
            }

            void setMaterial(Video::Device::Context* videoContext, Engine::Shader::Pass* pass, MaterialHandle handle, bool forceShader)
            {
                assert(videoContext);
                assert(pass);

                if (drawPrimitiveValid)
                {
                    auto material = materialCache.getResource(handle);
                    if (drawPrimitiveValid = (material != nullptr))
                    {
                        auto data = material->getData(pass->getMaterialHash());
                        if (drawPrimitiveValid = (data != nullptr))
                        {
                            if (!forceShader)
                            {
                                setRenderState(videoContext, material->getRenderState());
                            }

                            setResourceList(videoContext->pixelPipeline(), data->resourceList, pass->getFirstResourceStage());
                        }
                    }
                }
            }

            void setVisual(Video::Device::Context* videoContext, VisualHandle handle)
            {
                assert(videoContext);

                if (drawPrimitiveValid)
                {
                    auto visual = visualCache.getResource(handle);
                    if (drawPrimitiveValid = (visual != nullptr))
                    {
                        visual->enable(videoContext);
                    }
                }
            }

            void setRenderState(Video::Device::Context* videoContext, RenderStateHandle renderStateHandle)
            {
                assert(videoContext);

                if (drawPrimitiveValid)
                {
                    auto renderState = renderStateCache.getResource(renderStateHandle);
                    if (drawPrimitiveValid = (renderState != nullptr))
                    {
                        videoContext->setRenderState(renderState);
                    }
                }
            }

            void setDepthState(Video::Device::Context* videoContext, DepthStateHandle depthStateHandle, uint32_t stencilReference)
            {
                assert(videoContext);

                if (drawPrimitiveValid)
                {
                    auto depthState = depthStateCache.getResource(depthStateHandle);
                    if (drawPrimitiveValid = (depthState != nullptr))
                    {
                        videoContext->setDepthState(depthState, stencilReference);
                    }
                }
            }

            void setBlendState(Video::Device::Context* videoContext, BlendStateHandle blendStateHandle, Math::Float4 const& blendFactor, uint32_t sampleMask)
            {
                assert(videoContext);

                if (drawPrimitiveValid)
                {
                    auto blendState = blendStateCache.getResource(blendStateHandle);
                    if (drawPrimitiveValid = (blendState != nullptr))
                    {
                        videoContext->setBlendState(blendState, blendFactor, sampleMask);
                    }
                }
            }

            void setProgram(Video::Device::Context::Pipeline* videoPipeline, ProgramHandle programHandle)
            {
                assert(videoPipeline);

                auto& valid = getValid(videoPipeline);
                if (valid)
                {
                    auto program = programCache.getResource(programHandle);
                    if (valid && (valid = (program != nullptr)))
                    {
                        videoPipeline->setProgram(program);
                    }
                }
            }

            ObjectCache<Video::Target> renderTargetCache;
            std::vector<Video::ViewPort> viewPortCache;
            void setRenderTargetList(Video::Device::Context* videoContext, std::vector<ResourceHandle> const& renderTargetHandleList, ResourceHandle const* depthBuffer)
            {
                assert(videoContext);

                if (drawPrimitiveValid && (drawPrimitiveValid = renderTargetCache.set(renderTargetHandleList, dynamicCache, videoDevice->getBackBuffer())))
                {
                    auto& renderTargetList = renderTargetCache.get();
                    const uint32_t renderTargetCount = renderTargetList.size();
                    viewPortCache.resize(renderTargetCount);
                    for (uint32_t renderTarget = 0; renderTarget < renderTargetCount; ++renderTarget)
                    {
                        viewPortCache[renderTarget] = renderTargetList[renderTarget]->getViewPort();
                    }

                    videoContext->setRenderTargetList(renderTargetList, (depthBuffer ? getResource(*depthBuffer) : nullptr));
                    videoContext->setViewportList(viewPortCache);
                }
            }

            void clearRenderTargetList(Video::Device::Context* videoContext, int32_t count, bool depthBuffer)
            {
                assert(videoContext);

                videoContext->clearRenderTargetList(count, depthBuffer);
            }

            void startResourceBlock(void)
            {
                drawPrimitiveValid = true;
                dispatchValid = true;
            }
        };

        GEK_REGISTER_CONTEXT_USER(Resources);
    }; // namespace Implementation
}; // namespace Gek
