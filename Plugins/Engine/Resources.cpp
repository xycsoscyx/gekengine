#define _ENABLE_ATOMIC_ALIGNMENT_FIX

#include "GEK/Utility/String.hpp"
#include "GEK/Utility/ThreadPool.hpp"
#include "GEK/Utility/ShuntingYard.hpp"
#include "GEK/Utility/FileSystem.hpp"
#include "GEK/Utility/JSON.hpp"
#include "GEK/Shapes/Sphere.hpp"
#include "GEK/Utility/ContextUser.hpp"
#include "GEK/Engine/Core.hpp"
#include "GEK/Engine/Visual.hpp"
#include "GEK/Engine/Renderer.hpp"
#include "GEK/Engine/Resources.hpp"
#include "GEK/Engine/Shader.hpp"
#include "GEK/Engine/Filter.hpp"
#include "GEK/Engine/Material.hpp"
#include "GEK/Engine/Population.hpp"
#include "GEK/Engine/Entity.hpp"
#include "GEK/Engine/Component.hpp"
#include "GEK/Components/Transform.hpp"
#include "GEK/Components/Light.hpp"
#include "GEK/Components/Color.hpp"
#include "GEK/Utility/ContextUser.hpp"
#include <concurrent_unordered_map.h>
#include <concurrent_unordered_set.h>
#include <concurrent_queue.h>
#include <concurrent_vector.h>
#include <ppl.h>

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

        GEK_INTERFACE(ResourceRequester)
        {
            virtual ~ResourceRequester(void) = default;

            virtual void addRequest(std::function<void(void)> &&load) = 0;
        };

        template <class HANDLE, typename TYPE>
        class ResourceCache
        {
        public:
            using TypePtr = std::shared_ptr<TYPE>;
            using ResourceHandleMap = concurrency::concurrent_unordered_map<std::size_t, HANDLE>;
            using ResourceMap = concurrency::concurrent_unordered_map<HANDLE, TypePtr>;

        private:
            uint32_t validationIdentifier = 0;
            uint32_t nextIdentifier = 0;

        protected:
            ResourceRequester *resources = nullptr;
            ResourceHandleMap resourceHandleMap;
            ResourceMap resourceMap;

        public:
            ResourceCache(ResourceRequester *resources)
                : resources(resources)
            {
                assert(resources);
            }

            virtual ~ResourceCache(void) = default;

            ResourceMap &getResourceMap(void)
            {
                return resourceMap;
            }

            virtual void clear(void)
            {
                validationIdentifier = nextIdentifier;
                resourceHandleMap.clear();
                resourceMap.clear();
            }

            void setResource(HANDLE handle, const TypePtr &data)
            {
                auto &resourceSearch = resourceMap.insert(std::make_pair(handle, nullptr));
                auto &atomicResource = resourceSearch.first->second;
                std::atomic_exchange(&atomicResource, data);
            }

            virtual TYPE * const getResource(HANDLE handle) const
            {
                if (handle.identifier >= validationIdentifier)
                {
                    auto resourceSearch = resourceMap.find(handle);
                    if (resourceSearch != std::end(resourceMap))
                    {
                        return resourceSearch->second.get();
                    }
                }

                return nullptr;
            }

            uint32_t getNextHandle(void)
            {
                return InterlockedIncrement(&nextIdentifier);
            }
        };

        template <class HANDLE, typename TYPE>
        class GeneralResourceCache
            : public ResourceCache<HANDLE, TYPE>
        {
        private:
            concurrency::concurrent_unordered_set<std::size_t> requestedLoadSet;

        public:
            GeneralResourceCache(ResourceRequester *resources)
                : ResourceCache(resources)
            {
            }

            void clear(void)
            {
                requestedLoadSet.clear();
                ResourceCache::clear();
            }

            std::pair<bool, HANDLE> getHandle(std::size_t hash, std::function<TypePtr(HANDLE)> &&load)
            {
                if (requestedLoadSet.count(hash) > 0)
                {
                    auto resourceSearch = resourceHandleMap.find(hash);
                    if (resourceSearch != std::end(resourceHandleMap))
                    {
                        return std::make_pair(false, resourceSearch->second);
                    }
                }
                else
                {
                    requestedLoadSet.insert(hash);
                    HANDLE handle = getNextHandle();
                    resourceHandleMap[hash] = handle;
                    resources->addRequest([this, handle, load = move(load)](void) -> void
                    {
                        setResource(handle, load(handle));
                    });

                    return std::make_pair(true, handle);
                }

                return std::make_pair(false, HANDLE());
            }

            HANDLE getHandle(std::size_t hash) const
            {
                if (requestedLoadSet.count(hash) > 0)
                {
                    auto resourceSearch = resourceHandleMap.find(hash);
                    if (resourceSearch != std::end(resourceHandleMap))
                    {
                        return resourceSearch->second;
                    }
                }

                return HANDLE();
            }
        };

        template <class HANDLE, typename TYPE>
        class DynamicResourceCache
            : public ResourceCache<HANDLE, TYPE>
        {
        private:
            concurrency::concurrent_unordered_set<std::size_t> requestedLoadSet;
            concurrency::concurrent_unordered_map<HANDLE, std::size_t> loadParameters;

        public:
            DynamicResourceCache(ResourceRequester *resources)
                : ResourceCache(resources)
            {
            }

            void clear(void)
            {
                loadParameters.clear();
                requestedLoadSet.clear();
                ResourceCache::clear();
            }

            void setHandle(std::size_t hash, HANDLE handle, TypePtr data)
            {
                requestedLoadSet.insert(hash);
                resourceHandleMap[hash] = handle;
                setResource(handle, data);
            }

            std::pair<bool, HANDLE> getHandle(std::size_t hash, std::size_t parameters, std::function<TypePtr(HANDLE)> &&load, uint32_t flags)
            {
                HANDLE handle;
                if (requestedLoadSet.count(hash) > 0)
                {
                    auto resourceSearch = resourceHandleMap.find(hash);
                    if (resourceSearch != std::end(resourceHandleMap))
                    {
                        HANDLE handle = resourceSearch->second;
                        if (!(flags & Resources::Flags::LoadFromCache))
                        {
                            auto loadParametersSearch = loadParameters.find(handle);
                            if (loadParametersSearch == std::end(loadParameters) || loadParametersSearch->second != parameters)
                            {
                                loadParameters[handle] = parameters;
                                if (flags & Resources::Flags::LoadImmediately)
                                {
                                    setResource(handle, load(handle));
                                }
                                else
                                {
                                    resources->addRequest([this, handle, load = move(load)](void) -> void
                                    {
                                        setResource(handle, load(handle));
                                    });
                                }

                                return std::make_pair(true, handle);
                            }
                        }

                        return std::make_pair(false, handle);
                    }
                }
                else
                {
                    requestedLoadSet.insert(hash);
                    HANDLE handle = getNextHandle();
                    resourceHandleMap[hash] = handle;
                    loadParameters[handle] = parameters;
                    if (flags & Resources::Flags::LoadImmediately)
                    {
                        setResource(handle, load(handle));
                    }
                    else
                    {
                        resources->addRequest([this, handle, load = move(load)](void) -> void
                        {
                            setResource(handle, load(handle));
                        });
                    }

                    return std::make_pair(true, handle);
                }

                return std::make_pair(false, HANDLE());
            }

            HANDLE getHandle(std::size_t hash) const
            {
                if (requestedLoadSet.count(hash) > 0)
                {
                    auto resourceSearch = resourceHandleMap.find(hash);
                    if (resourceSearch != std::end(resourceHandleMap))
                    {
                        return resourceSearch->second;
                    }
                }

                return HANDLE();
            }
        };

        template <class HANDLE, typename TYPE>
        class ProgramResourceCache
            : public ResourceCache<HANDLE, TYPE>
        {
        public:
            ProgramResourceCache(ResourceRequester *resources)
                : ResourceCache(resources)
            {
            }

            HANDLE getHandle(std::function<TypePtr(HANDLE)> &&load)
            {
                HANDLE handle;
                handle = getNextHandle();
                resources->addRequest([this, handle, load = move(load)](void) -> void
                {
                    setResource(handle, load(handle));
                });

                return handle;
            }
        };

        template <class HANDLE, typename TYPE>
        class ReloadResourceCache
            : public ResourceCache<HANDLE, TYPE>
        {
        private:
            concurrency::concurrent_unordered_set<std::size_t> requestedLoadSet;

        public:
            ReloadResourceCache(ResourceRequester *resources)
                : ResourceCache(resources)
            {
            }

            void reload(void)
            {
                for (auto &resourceSearch : resourceMap)
                {
                    auto &resource = std::atomic_load(&resourceSearch.second);
                    if (resource)
                    {
                        resource->reload();
                    }
                }
            }

            void clear(void)
            {
                requestedLoadSet.clear();
                ResourceCache::clear();
            }

            std::pair<bool, HANDLE> getHandle(std::size_t hash, std::function<TypePtr(HANDLE)> &&load)
            {
                HANDLE handle;
                if (requestedLoadSet.count(hash) > 0)
                {
                    auto resourceSearch = resourceHandleMap.find(hash);
                    if (resourceSearch != std::end(resourceHandleMap))
                    {
                        return std::make_pair(false, resourceSearch->second);
                    }
                }
                else
                {
                    requestedLoadSet.insert(hash);
                    HANDLE handle = getNextHandle();
                    resourceHandleMap[hash] = handle;
                    setResource(handle, load(handle));
                    return std::make_pair(true, handle);
                }

                return std::make_pair(false, HANDLE());
            }

            HANDLE getHandle(std::size_t hash) const
            {
                if (requestedLoadSet.count(hash) > 0)
                {
                    auto resourceSearch = resourceHandleMap.find(hash);
                    if (resourceSearch != std::end(resourceHandleMap))
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
            std::vector<TYPE *> objectList;

            template <typename INPUT, typename HANDLE, typename SOURCE>
            bool set(const std::vector<INPUT> &inputList, ResourceCache<HANDLE, SOURCE> &cache, TYPE *defaultObject = nullptr)
            {
                const auto listCount = inputList.size();
                objectList.reserve(std::max(listCount, objectList.size()));
                objectList.resize(listCount);

                for (uint32_t object = 0; object < listCount; ++object)
                {
                    auto &input = inputList[object];
                    if (input)
                    {
                        auto resource = cache.getResource(inputList[object]);
                        if (!resource)
                        {
                            return false;
                        }

                        objectList[object] = dynamic_cast<TYPE *>(resource);
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
            bool set(const std::vector<INPUT> &inputList, ResourceCache<HANDLE, TYPE> &cache)
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

            std::vector<TYPE *> &get(void)
            {
                return objectList;
            }

            const std::vector<TYPE *> &get(void) const
            {
                return objectList;
            }
        };

        GEK_CONTEXT_USER(Resources, Plugin::Core *)
            , public Engine::Resources
            , public ResourceRequester
        {
        private:
            Plugin::Core *core = nullptr;
            Video::Device *videoDevice = nullptr;
            Plugin::Renderer *renderer = nullptr;

            ThreadPool loadPool;
            std::recursive_mutex shaderMutex;

            ProgramResourceCache<ProgramHandle, Video::Object> programCache;
            GeneralResourceCache<VisualHandle, Plugin::Visual> visualCache;
            GeneralResourceCache<MaterialHandle, Engine::Material> materialCache;
            ReloadResourceCache<ShaderHandle, Engine::Shader> shaderCache;
            ReloadResourceCache<ResourceHandle, Engine::Filter> filterCache;
            DynamicResourceCache<ResourceHandle, Video::Object> dynamicCache;
            GeneralResourceCache<RenderStateHandle, Video::RenderState> renderStateCache;
            GeneralResourceCache<DepthStateHandle, Video::DepthState> depthStateCache;
            GeneralResourceCache<BlendStateHandle, Video::BlendState> blendStateCache;

            concurrency::concurrent_unordered_map<MaterialHandle, ShaderHandle> materialShaderMap;
            concurrency::concurrent_unordered_map<ResourceHandle, Video::Texture::Description> textureDescriptionMap;
            concurrency::concurrent_unordered_map<ResourceHandle, Video::Buffer::Description> bufferDescriptionMap;

            struct Validate
            {
                bool state;
                Validate(void)
                    : state(false)
                {
                }

                operator bool ()
                {
                    return state;
                }

                bool operator = (bool state)
                {
                    this->state = state;
                    return state;
                }
            };

            Validate drawPrimitiveValid;
            Validate dispatchValid;

        public:
            Resources(Context *context, Plugin::Core *core)
                : ContextRegistration(context)
                , core(core)
                , videoDevice(core->getVideoDevice())
                , programCache(this)
                , visualCache(this)
                , materialCache(this)
                , shaderCache(this)
                , filterCache(this)
                , dynamicCache(this)
                , renderStateCache(this)
                , depthStateCache(this)
                , blendStateCache(this)
                , loadPool(2)
            {
                assert(core);
                assert(videoDevice);

                core->onChangedDisplay.connect(this, &Resources::onReload);
                core->onChangedSettings.connect(this, &Resources::onReload);
                core->onInitialized.connect(this, &Resources::onInitialized);
                core->onShutdown.connect(this, &Resources::onShutdown);
            }

            Validate &getValid(Video::Device::Context::Pipeline *videoPipeline)
            {
                assert(videoPipeline);

                return (videoPipeline->getType() == Video::PipelineType::Compute ? dispatchValid : drawPrimitiveValid);
            }

            Video::TexturePtr loadTextureData(FileSystem::Path const &filePath, std::string const &textureName, uint32_t flags)
            {
                auto texture = videoDevice->loadTexture(filePath, flags);
                texture->setName(textureName);
                return texture;
            }

            std::string getFullProgram(std::string const &name, std::string const &engineData)
            {
                auto programsPath(getContext()->getRootFileName("data", "programs"));
                auto filePath(FileSystem::GetFileName(programsPath, name));
                if (filePath.isFile())
                {
                    auto programDirectory(filePath.getParentPath());
					std::string baseProgram(FileSystem::Load(filePath, String::Empty));
                    String::Replace(baseProgram, "\r", "$");
					String::Replace(baseProgram, "\n", "$");
					String::Replace(baseProgram, "$$", "$");
                    auto programLines = String::Split(baseProgram, '$', false);

                    std::string uncompiledProgram;
                    for (auto const &line : programLines)
                    {
                        if (line.empty())
                        {
                            uncompiledProgram.append("\r\n");
                        }
                        else if (line.find("#include") == 0)
                        {
							std::string includeName(String::GetLower(line.substr(8)));
							String::Trim(includeName);

                            std::string includeData;
                            if (includeName == "gekengine")
                            {
                                includeData = engineData;
                            }
                            else
                            {
                                auto includeType = includeName.at(0);
                                includeName = includeName.substr(1, includeName.length() - 2);
                                if (includeType == '\"')
                                {
                                    auto localPath(FileSystem::GetFileName(programDirectory, includeName));
                                    if (localPath.isFile())
                                    {
										includeData = FileSystem::Load(localPath, String::Empty);
                                    }
                                }
                                else if (includeType == '<')
                                {
                                    auto rootPath(FileSystem::GetFileName(programsPath, includeName));
                                    if (rootPath.isFile())
                                    {
										includeData = FileSystem::Load(rootPath, String::Empty);
                                    }
                                }
                            }

                            uncompiledProgram.append(includeData);
                        }
                        else
                        {
                            uncompiledProgram.append(line);
                        }

                        uncompiledProgram.append("\r\n");
                    }

                    return uncompiledProgram;
                }
                else
                {
                    return engineData;
                }
            }

            // Renderer
            bool showResources = false;
            void onShowUserInterface(ImGuiContext * const guiContext)
            {
                ImGuiIO &imGuiIo = ImGui::GetIO();
                auto mainMenu = ImGui::FindWindowByName("##MainMenuBar");
                auto mainMenuShowing = (mainMenu ? mainMenu->Active : false);
                if (mainMenuShowing)
                {
                    ImGui::BeginMainMenuBar();
                    ImGui::PushStyleVar(ImGuiStyleVar_ItemInnerSpacing, ImVec2(5.0f, 10.0f));
                    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(5.0f, 10.0f));
                    if (ImGui::BeginMenu("Resources"))
                    {
                        ImGui::MenuItem("Show", "CTRL+S", &showResources);
                        if (ImGui::MenuItem("Reload", "CTRL+R"))
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
                    ImGui::SetNextWindowSize(ImVec2(500.0f, 350.0f), ImGuiSetCond_Once);
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
                        ImGui::End();
                    }
                }
            }

            template <typename CACHE>
            void showVideoObjectMap(CACHE &cache, std::string const &name, std::function<void(typename CACHE::TypePtr &)> onObject)
            {
                auto lowerName = String::GetLower(name);
                if (ImGui::TreeNodeEx(name.c_str(), ImGuiTreeNodeFlags_Framed))
                {
                    auto &resourceMap = cache.getResourceMap();
                    for (auto &resourcePair : resourceMap)
                    {
                        auto handle = resourcePair.first;
                        auto &object = resourcePair.second;
                        std::string nodeName(object->getName());
                        if (nodeName.empty())
                        {
                            nodeName = String::Format("%v_%v", lowerName, static_cast<uint64_t>(handle.identifier));
                        }

                        if (ImGui::TreeNodeEx(nodeName.c_str(), ImGuiTreeNodeFlags_Framed))
                        {
                            onObject(object);
                            ImGui::TreePop();
                        }
                    }

                    ImGui::TreePop();
                }
            }

            template <typename CACHE>
            void showVideoResourceMap(CACHE &cache, std::string const &name, std::function<void(typename CACHE::TypePtr &)> onObject)
            {
                if (ImGui::TreeNodeEx(name.c_str(), ImGuiTreeNodeFlags_Framed))
                {
                    auto lowerName = String::GetLower(name);
                    std::unordered_map<std::type_index, std::vector<CACHE::ResourceMap::value_type>> typeDataMap;
                    auto &resourceMap = cache.getResourceMap();
                    for (auto &resourcePair : resourceMap)
                    {
                        auto &object = resourcePair.second;
                        typeDataMap[object->getTypeInfo()].push_back(resourcePair);
                    }

                    for (auto &typePair : typeDataMap)
                    {
                        auto name = (std::strrchr(typePair.first.name(), ':') + 1);
                        if (ImGui::TreeNodeEx(name, ImGuiTreeNodeFlags_Framed))
                        {
                            for (auto &resourcePair : typePair.second)
                            {
                                auto handle = resourcePair.first;
                                auto &object = resourcePair.second;
                                std::string nodeName(object->getName());
                                if (nodeName.empty())
                                {
                                    nodeName = String::Format("%v_%v", lowerName, static_cast<uint64_t>(handle.identifier));
                                }

                                if (ImGui::TreeNodeEx(nodeName.c_str(), ImGuiTreeNodeFlags_Framed))
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
                //showResourceMap(visualCache, "Visuals"s);
            }

            void showShaderCache(void)
            {
                //showResourceMap(visualCache, "Shaders"s);
            }

            void showFilterCache(void)
            {
                //showResourceMap(visualCache, "Filters"s);
            }

            void showProgramCache(void)
            {
                showVideoObjectMap(programCache, "Programs"s, [&](auto &object) -> void
                {
                });
            }

            void showMaterialCache(void)
            {
                //showResourceMap(visualCache, "Materials"s);
            }

            bool showResourceValue(char const *text, const char *label, std::string &value)
            {
                ImGui::AlignFirstTextHeightToWidgets();
                ImGui::Text(text);
                ImGui::SameLine();
                ImGui::PushItemWidth(-1.0f);
                bool changed = UI::InputString(label, value, ImGuiInputTextFlags_ReadOnly);
                ImGui::PopItemWidth();
                return changed;
            }

            void showDynamicCache(void)
            {
                showVideoResourceMap(dynamicCache, "Resources"s, [&](auto &object) -> void
                {
                    if (object->getTypeInfo() == typeid(Video::Texture) || object->getTypeInfo() == typeid(Video::Target))
                    {
                        auto texture = dynamic_cast<Video::Texture *>(object.get());
                        Video::Object *object = texture;

                        auto const &description = texture->getDescription();
                        showResourceValue("Format", "##format", Video::GetFormat(description.format));
                        showResourceValue("Width", "##width", std::to_string(description.width));
                        showResourceValue("Height", "##height", std::to_string(description.height));
                        showResourceValue("Depth", "##depth", std::to_string(description.depth));
                        showResourceValue("MipMap Levels", "##mipMapCount", std::to_string(description.mipMapCount));
                        showResourceValue("MultiSample Count", "##sampleCount", std::to_string(description.sampleCount));
                        showResourceValue("MultiSample Quality", "##sampleQuality", std::to_string(description.sampleQuality));
                        showResourceValue("Flags", "##flags", std::to_string(description.flags));
                        ImGui::Image(reinterpret_cast<ImTextureID>(object), ImVec2(ImGui::GetContentRegionAvailWidth(), ImGui::GetContentRegionAvailWidth()));
                    }
                    else if (object->getTypeInfo() == typeid(Video::Buffer))
                    {
                        auto buffer = dynamic_cast<Video::Buffer *>(object.get());
                        auto const &description = buffer->getDescription();
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
                showVideoObjectMap(renderStateCache, "Render States"s, [&](auto &object) -> void
                {
                    auto const &description = object->getDescription();
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

            void showStencilState(char const *text, Video::DepthState::Description::StencilState const &stencilState)
            {
                if (ImGui::TreeNodeEx(text, ImGuiTreeNodeFlags_Framed))
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
                showVideoObjectMap(depthStateCache, "Depth States"s, [&](auto &object) -> void
                {
                    auto const &description = object->getDescription();
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
                showVideoObjectMap(blendStateCache, "Blend States"s, [&](auto &object) -> void
                {
                    auto const &description = object->getDescription();
                    showResourceValue("Alpha To Coverage", "##alphaToCoverage", std::to_string(description.alphaToCoverage));
                    showResourceValue("Independent Blend States", "##independentBlendStates", std::to_string(description.independentBlendStates));

                    ImGui::Text("Target State");
                    ImGui::SameLine();
                    ImGui::PushItemWidth(-1.0f);
                    ImGui::SliderInt("##targetState", &currentBlendStateTarget, 0, 7);
                    ImGui::PopItemWidth();

                    auto const &targetState = description.targetStates[currentBlendStateTarget];
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

            // ResourceRequester
            void addRequest(std::function<void(void)> &&load)
            {
                loadPool.enqueue([this, load = move(load)](void) -> void
                {
                    load();
                });
            }

            // Plugin::Resources
            VisualHandle loadVisual(std::string const &visualName)
            {
                auto load = [this, visualName](VisualHandle)->Plugin::VisualPtr
                {
                    return getContext()->createClass<Plugin::Visual>("Engine::Visual", videoDevice, (Engine::Resources *)this, visualName);
                };

                auto hash = GetHash(visualName);
                return visualCache.getHandle(hash, std::move(load)).second;
            }

            MaterialHandle loadMaterial(std::string const &materialName)
            {
                auto load = [this, materialName](MaterialHandle handle)->Engine::MaterialPtr
                {
                    return getContext()->createClass<Engine::Material>("Engine::Material", (Engine::Resources *)this, materialName, handle);
                };

                auto hash = GetHash(materialName);
                return materialCache.getHandle(hash, std::move(load)).second;
            }

            ResourceHandle loadTexture(std::string const &textureName, uint32_t flags)
            {
                // iterate over formats in case the texture name has no extension
                static const std::string formatList[] =
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

                auto texturePath(getContext()->getRootFileName("data", "textures", textureName));
                for (auto const &format : formatList)
                {
                    auto filePath(texturePath.withExtension(format));
                    if (filePath.isFile())
                    {
                        auto load = [this, filePath = FileSystem::Path(filePath), textureName = std::string(textureName), flags](ResourceHandle)->Video::TexturePtr
                        {
                            return loadTextureData(filePath, textureName, flags);
                        };

                        auto hash = GetHash(textureName);
                        auto resource = dynamicCache.getHandle(hash, flags, std::move(load), false);
                        if (resource.first)
                        {
                            auto description = videoDevice->loadTextureDescription(filePath);
                            textureDescriptionMap.insert(std::make_pair(resource.second, description));
                        }

                        return resource.second;
                    }
                }

                return ResourceHandle();
            }

            ResourceHandle createPattern(std::string const &pattern, JSON::Reference parameters)
            {
                auto lowerPattern = String::GetLower(pattern);

                std::vector<uint8_t> data;
                Video::Texture::Description description;
                if (lowerPattern == "color")
                {
                    switch (parameters.getArray().size())
                    {
                    case 1:
                        data.push_back(parameters.at(0).convert(255));
                        description.format = Video::Format::R8_UNORM;
                        break;

                    case 2:
                        data.push_back(parameters.at(0).convert(255));
                        data.push_back(parameters.at(1).convert(255));
                        description.format = Video::Format::R8G8_UNORM;
                        break;

                    case 3:
                        data.push_back(parameters.at(0).convert(255));
                        data.push_back(parameters.at(1).convert(255));
                        data.push_back(parameters.at(2).convert(255));
                        data.push_back(0);
                        description.format = Video::Format::R8G8B8A8_UNORM;
                        break;

                    case 4:
                        data.push_back(parameters.at(0).convert(255));
                        data.push_back(parameters.at(1).convert(255));
                        data.push_back(parameters.at(2).convert(255));
                        data.push_back(parameters.at(3).convert(255));
                        description.format = Video::Format::R8G8B8A8_UNORM;
                        break;

                    default:
                        if (true)
                        {
                            if (parameters.isFloat())
                            {
                                union
                                {
                                    float value;
                                    uint8_t quarters[4];
                                };

                                value = parameters.convert(1.0f);
                                data.push_back(quarters[0]);
                                data.push_back(quarters[1]);
                                data.push_back(quarters[2]);
                                data.push_back(quarters[3]);
                                description.format = Video::Format::R32_FLOAT;
                            }
                            else
                            {
                                data.push_back(parameters.convert(255));
                                description.format = Video::Format::R8_UNORM;
                            }
                        }
                    };
                }
                else if (lowerPattern == "normal")
                {
                    union
                    {
                        uint16_t halves[2];
                        uint8_t quarters[4];
                    };

                    Math::Float3 normal = parameters.convert(Math::Float3::Zero);

                    Float16Compressor compressor;
                    halves[0] = compressor.compress(normal.x);
                    halves[1] = compressor.compress(normal.y);
                    data.push_back(quarters[0]);
                    data.push_back(quarters[1]);
                    data.push_back(quarters[2]);
                    data.push_back(quarters[3]);

                    description.format = Video::Format::R16G16_FLOAT;
                }
                else if (lowerPattern == "system")
                {
                    std::string type(String::GetLower(parameters.convert(String::Empty)));
                    if (type == "debug")
                    {
                        data.push_back(255);    data.push_back(0);      data.push_back(255);    data.push_back(255);
                        data.push_back(255);    data.push_back(255);    data.push_back(255);    data.push_back(255);
                        data.push_back(255);    data.push_back(255);    data.push_back(255);    data.push_back(255);
                        data.push_back(255);    data.push_back(0);      data.push_back(255);    data.push_back(255);
                        description.format = Video::Format::R8G8B8A8_UNORM;
                        description.width = 2;
                        description.height = 2;
                    }
                    else if (type == "flat")
                    {
                        data.push_back(127);
                        data.push_back(127);
                        data.push_back(255);
                        data.push_back(255);
                        description.format = Video::Format::R8G8B8A8_UNORM;
                    }
                }

                description.flags = Video::Texture::Flags::Resource;
                std::string name(String::Format("%v:%v", pattern, parameters.convert(String::Empty)));
                auto load = [this, name, description, data = move(data)](ResourceHandle) mutable -> Video::TexturePtr
                {
                    auto texture = videoDevice->createTexture(description, data.data());
                    texture->setName(name);
                    return texture;
                };

                auto hash = GetHash(name);
                auto resource = dynamicCache.getHandle(hash, 0, std::move(load), false);
                if (resource.first)
                {
                    textureDescriptionMap.insert(std::make_pair(resource.second, description));
                }

                return resource.second;
            }

            ResourceHandle createTexture(std::string const &textureName, const Video::Texture::Description &description, uint32_t flags)
            {
                auto load = [this, textureName, description](ResourceHandle)->Video::TexturePtr
                {
                    auto texture = videoDevice->createTexture(description);
                    texture->setName(textureName);
                    return texture;
                };

                auto hash = GetHash(textureName);
                auto parameters = description.getHash();
                if (description.format == Video::Format::Unknown)
                {
                    flags |= Resources::Flags::LoadFromCache;
                }

                auto resource = dynamicCache.getHandle(hash, parameters, std::move(load), flags);
                if (resource.first)
                {
                    textureDescriptionMap.insert(std::make_pair(resource.second, description));
                }

                return resource.second;
            }

            ResourceHandle createBuffer(std::string const &bufferName, const Video::Buffer::Description &description, uint32_t flags)
            {
                assert(description.count > 0);

                auto load = [this, bufferName, description](ResourceHandle)->Video::BufferPtr
                {
                    auto buffer = videoDevice->createBuffer(description);
                    buffer->setName(bufferName);
                    return buffer;
                };

                auto hash = GetHash(bufferName);
                auto parameters = description.getHash();
                if (description.format == Video::Format::Unknown)
                {
                    flags |= Resources::Flags::LoadFromCache;
                }

                auto resource = dynamicCache.getHandle(hash, parameters, std::move(load), flags);
                if (resource.first)
                {
                    bufferDescriptionMap.insert(std::make_pair(resource.second, description));
                }

                return resource.second;
            }

            ResourceHandle createBuffer(std::string const &bufferName, const Video::Buffer::Description &description, std::vector<uint8_t> &&staticData, uint32_t flags)
            {
                assert(description.count > 0);
                assert(!staticData.empty());

                auto load = [this, bufferName, description, staticData = move(staticData)](ResourceHandle)->Video::BufferPtr
                {
                    auto buffer = videoDevice->createBuffer(description, (void *)staticData.data());
                    buffer->setName(bufferName);
                    return buffer;
                };

                auto hash = GetHash(bufferName);
                auto parameters = reinterpret_cast<std::size_t>(staticData.data());
                if (description.format == Video::Format::Unknown)
                {
                    flags |= Resources::Flags::LoadFromCache;
                }

                auto resource = dynamicCache.getHandle(hash, parameters, std::move(load), flags);
                if (resource.first)
                {
                    bufferDescriptionMap.insert(std::make_pair(resource.second, description));
                }

                return resource.second;
            }

            void setIndexBuffer(Video::Device::Context *videoContext, ResourceHandle resourceHandle, uint32_t offset)
            {
                assert(videoContext);

                if (drawPrimitiveValid)
                {
                    auto resource = getResource(resourceHandle);
                    if (drawPrimitiveValid = (resource != nullptr))
                    {
                        videoContext->setIndexBuffer(dynamic_cast<Video::Buffer *>(resource), offset);
                    }
                }
            }

            ObjectCache<Video::Buffer> vertexBufferCache;
            void setVertexBufferList(Video::Device::Context *videoContext, const std::vector<ResourceHandle> &resourceHandleList, uint32_t firstSlot, uint32_t *offsetList)
            {
                assert(videoContext);

                if (drawPrimitiveValid && (drawPrimitiveValid = vertexBufferCache.set(resourceHandleList, dynamicCache)))
                {
                    videoContext->setVertexBufferList(vertexBufferCache.get(), firstSlot, offsetList);
                }
            }

            ObjectCache<Video::Buffer> constantBufferCache;
            void setConstantBufferList(Video::Device::Context::Pipeline *videoPipeline, const std::vector<ResourceHandle> &resourceHandleList, uint32_t firstStage)
            {
                assert(videoPipeline);

                auto &valid = getValid(videoPipeline);
                if (valid && (valid = constantBufferCache.set(resourceHandleList, dynamicCache)))
                {
                    videoPipeline->setConstantBufferList(constantBufferCache.get(), firstStage);
                }
            }

            ObjectCache<Video::Object> resourceCache;
            void setResourceList(Video::Device::Context::Pipeline *videoPipeline, const std::vector<ResourceHandle> &resourceHandleList, uint32_t firstStage)
            {
                assert(videoPipeline);

                auto &valid = getValid(videoPipeline);
                if (valid && (valid = resourceCache.set(resourceHandleList, dynamicCache, videoDevice->getBackBuffer())))
                {
                    videoPipeline->setResourceList(resourceCache.get(), firstStage);
                }
            }

            ObjectCache<Video::Object> unorderedAccessCache;
            void setUnorderedAccessList(Video::Device::Context::Pipeline *videoPipeline, const std::vector<ResourceHandle> &resourceHandleList, uint32_t firstStage)
            {
                assert(videoPipeline);

                auto &valid = getValid(videoPipeline);
                if (valid && (valid = unorderedAccessCache.set(resourceHandleList, dynamicCache)))
                {
                    videoPipeline->setUnorderedAccessList(unorderedAccessCache.get(), firstStage);
                }
            }

            void clearIndexBuffer(Video::Device::Context *videoContext)
            {
                assert(videoContext);

                videoContext->clearIndexBuffer();
            }

            void clearVertexBufferList(Video::Device::Context *videoContext, uint32_t count, uint32_t firstSlot)
            {
                assert(videoContext);

                videoContext->clearVertexBufferList(count, firstSlot);
            }

            void clearConstantBufferList(Video::Device::Context::Pipeline *videoPipeline, uint32_t count, uint32_t firstStage)
            {
                assert(videoPipeline);

                videoPipeline->clearConstantBufferList(count, firstStage);
            }

            void clearResourceList(Video::Device::Context::Pipeline *videoPipeline, uint32_t count, uint32_t firstStage)
            {
                assert(videoPipeline);

                videoPipeline->clearResourceList(count, firstStage);
            }

            void clearUnorderedAccessList(Video::Device::Context::Pipeline *videoPipeline, uint32_t count, uint32_t firstStage)
            {
                assert(videoPipeline);

                videoPipeline->clearUnorderedAccessList(count, firstStage);
            }

            void drawPrimitive(Video::Device::Context *videoContext, uint32_t vertexCount, uint32_t firstVertex)
            {
                assert(videoContext);

                if (drawPrimitiveValid)
                {
                    videoContext->drawPrimitive(vertexCount, firstVertex);
                }
            }

            void drawInstancedPrimitive(Video::Device::Context *videoContext, uint32_t instanceCount, uint32_t firstInstance, uint32_t vertexCount, uint32_t firstVertex)
            {
                if (drawPrimitiveValid)
                {
                    videoContext->drawInstancedPrimitive(instanceCount, firstInstance, vertexCount, firstVertex);
                }
            }

            void drawIndexedPrimitive(Video::Device::Context *videoContext, uint32_t indexCount, uint32_t firstIndex, uint32_t firstVertex)
            {
                assert(videoContext);

                if (drawPrimitiveValid)
                {
                    videoContext->drawIndexedPrimitive(indexCount, firstIndex, firstVertex);
                }
            }

            void drawInstancedIndexedPrimitive(Video::Device::Context *videoContext, uint32_t instanceCount, uint32_t firstInstance, uint32_t indexCount, uint32_t firstIndex, uint32_t firstVertex)
            {
                assert(videoContext);

                if (drawPrimitiveValid)
                {
                    videoContext->drawInstancedIndexedPrimitive(instanceCount, firstInstance, indexCount, firstIndex, firstVertex);
                }
            }

            void dispatch(Video::Device::Context *videoContext, uint32_t threadGroupCountX, uint32_t threadGroupCountY, uint32_t threadGroupCountZ)
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
                loadPool.reset();
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

            ShaderHandle getMaterialShader(MaterialHandle material) const
            {
                auto shaderSearch = materialShaderMap.find(material);
                if (shaderSearch != std::end(materialShaderMap))
                {
                    return shaderSearch->second;
                }

                return ShaderHandle();
            }

            ResourceHandle getResourceHandle(std::string const &resourceName) const
            {
                return dynamicCache.getHandle(GetHash(resourceName));
            }

            Engine::Shader * const getShader(ShaderHandle handle) const
            {
                return shaderCache.getResource(handle);
            }

            ShaderHandle const getShader(std::string const &shaderName, MaterialHandle material)
            {
                std::unique_lock<std::recursive_mutex> lock(shaderMutex);
                auto load = [this, shaderName](ShaderHandle) -> Engine::ShaderPtr
                {
                    return getContext()->createClass<Engine::Shader>("Engine::Shader", core, shaderName);
                };

                auto hash = GetHash(shaderName);
                auto resource = shaderCache.getHandle(hash, std::move(load));
                if (material && resource.second)
                {
                    materialShaderMap[material] = resource.second;
                }

                return resource.second;
            }

            Engine::Filter * const getFilter(std::string const &filterName)
            {
                auto load = [this, filterName](ResourceHandle)->Engine::FilterPtr
                {
                    return getContext()->createClass<Engine::Filter>("Engine::Filter", core, filterName);
                };

                auto hash = GetHash(filterName);
                auto resource = filterCache.getHandle(hash, std::move(load));
                return filterCache.getResource(resource.second);
            }

            Video::Texture::Description const * const getTextureDescription(ResourceHandle resourceHandle) const
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

            Video::Buffer::Description const * const getBufferDescription(ResourceHandle resourceHandle) const
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

            Video::Object * const getResource(ResourceHandle resourceHandle) const
            {
                if (!resourceHandle)
                {
                    return videoDevice->getBackBuffer();
                }

                return dynamicCache.getResource(resourceHandle);
            }

            std::vector<uint8_t> compileProgram(Video::PipelineType pipelineType, std::string const &name, std::string const &entryFunction, std::string const &engineData)
            {
                auto uncompiledProgram = getFullProgram(name, engineData);

                auto hash = GetHash(uncompiledProgram);
                auto cacheExtension = String::Format(".%v.bin", hash);
                auto cachePath(getContext()->getRootFileName("data", "cache", name).withExtension(cacheExtension));

				std::vector<uint8_t> compiledProgram;
                if (cachePath.isFile())
                {
					static const std::vector<uint8_t> EmptyBuffer;
					compiledProgram = FileSystem::Load(cachePath, EmptyBuffer);
                }
                
                if (compiledProgram.empty())
                {
#ifdef _DEBUG
					auto debugExtension = String::Format(".%v.hlsl", hash);
					auto debugPath(getContext()->getRootFileName("data", "cache", name).withExtension(debugExtension));
					FileSystem::Save(debugPath, uncompiledProgram);
#endif
					compiledProgram = videoDevice->compileProgram(pipelineType, name, uncompiledProgram, entryFunction);
                    FileSystem::Save(cachePath, compiledProgram);
                }

                return compiledProgram;
            }

            ProgramHandle loadProgram(Video::PipelineType pipelineType, std::string const &name, std::string const &entryFunction, std::string const &engineData)
            {
                auto load = [this, pipelineType, name, entryFunction, engineData](ProgramHandle)->Video::ObjectPtr
                {
                    auto compiledProgram = compileProgram(pipelineType, name, entryFunction, engineData);
                    auto program = videoDevice->createProgram(pipelineType, compiledProgram.data(), compiledProgram.size());
                    program->setName(String::Format("%v:%v", name, entryFunction));
                    return program;
                };

                return programCache.getHandle(std::move(load));
            }

            RenderStateHandle createRenderState(Video::RenderState::Description const &description)
            {
                auto load = [this, description](RenderStateHandle) -> Video::RenderStatePtr
                {
                    auto state = videoDevice->createRenderState(description);
					//state->setName(stateName);
					return state;
                };

                auto hash = description.getHash();
                return renderStateCache.getHandle(hash, std::move(load)).second;
            }

            DepthStateHandle createDepthState(Video::DepthState::Description const &description)
            {
                auto load = [this, description](DepthStateHandle) -> Video::DepthStatePtr
                {
					auto state = videoDevice->createDepthState(description);
					//state->setName(stateName);
					return state;
				};

                auto hash = description.getHash();
                return depthStateCache.getHandle(hash, std::move(load)).second;
            }

            BlendStateHandle createBlendState(Video::BlendState::Description const &description)
            {
                auto load = [this, description](BlendStateHandle) -> Video::BlendStatePtr
                {
					auto state = videoDevice->createBlendState(description);
					//state->setName(stateName);
					return state;
				};

                auto hash = description.getHash();
                return blendStateCache.getHandle(hash, std::move(load)).second;
            }

            void generateMipMaps(Video::Device::Context *videoContext, ResourceHandle resourceHandle)
            {
                assert(videoContext);

                auto resource = getResource(resourceHandle);
                if (resource)
                {
                    videoContext->generateMipMaps(dynamic_cast<Video::Texture *>(resource));
                }
            }

            void resolveSamples(Video::Device::Context *videoContext, ResourceHandle destinationHandle, ResourceHandle sourceHandle)
            {
                auto source = getResource(sourceHandle);
                auto destination = getResource(destinationHandle);
                if (source && destination)
                {
                    videoContext->resolveSamples(dynamic_cast<Video::Texture *>(destination), dynamic_cast<Video::Texture *>(source));
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

            void clearUnorderedAccess(Video::Device::Context *videoContext, ResourceHandle resourceHandle, Math::Float4 const &value)
            {
                assert(videoContext);

                auto resource = getResource(resourceHandle);
                if (resource)
                {
                    videoContext->clearUnorderedAccess(resource, value);
                }
            }

            void clearUnorderedAccess(Video::Device::Context *videoContext, ResourceHandle resourceHandle, Math::UInt4 const &value)
            {
                assert(videoContext);

                auto resource = getResource(resourceHandle);
                if (resource)
                {
                    videoContext->clearUnorderedAccess(resource, value);
                }
            }

            void clearRenderTarget(Video::Device::Context *videoContext, ResourceHandle resourceHandle, Math::Float4 const &color)
            {
                assert(videoContext);

                auto resource = getResource(resourceHandle);
                if (resource)
                {
                    videoContext->clearRenderTarget(dynamic_cast<Video::Target *>(resource), color);
                }
            }

            void clearDepthStencilTarget(Video::Device::Context *videoContext, ResourceHandle depthBufferHandle, uint32_t flags, float clearDepth, uint32_t clearStencil)
            {
                assert(videoContext);

                auto depthBuffer = getResource(depthBufferHandle);
                if (depthBuffer)
                {
                    videoContext->clearDepthStencilTarget(depthBuffer, flags, clearDepth, clearStencil);
                }
            }

            void setMaterial(Video::Device::Context *videoContext, Engine::Shader::Pass *pass, MaterialHandle handle, bool forceShader)
            {
                assert(videoContext);
                assert(pass);

                if (drawPrimitiveValid)
                {
                    auto material = materialCache.getResource(handle);
                    if (drawPrimitiveValid =( material != nullptr))
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

            void setVisual(Video::Device::Context *videoContext, VisualHandle handle)
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

            void setRenderState(Video::Device::Context *videoContext, RenderStateHandle renderStateHandle)
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

            void setDepthState(Video::Device::Context *videoContext, DepthStateHandle depthStateHandle, uint32_t stencilReference)
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

            void setBlendState(Video::Device::Context *videoContext, BlendStateHandle blendStateHandle, Math::Float4 const &blendFactor, uint32_t sampleMask)
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

            void setProgram(Video::Device::Context::Pipeline *videoPipeline, ProgramHandle programHandle)
            {
                assert(videoPipeline);

                auto &valid = getValid(videoPipeline);
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
            void setRenderTargetList(Video::Device::Context *videoContext, const std::vector<ResourceHandle> &renderTargetHandleList, ResourceHandle const *depthBuffer)
            {
                assert(videoContext);

                if (drawPrimitiveValid && (drawPrimitiveValid = renderTargetCache.set(renderTargetHandleList, dynamicCache, videoDevice->getBackBuffer())))
                {
                    auto &renderTargetList = renderTargetCache.get();
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

            void clearRenderTargetList(Video::Device::Context *videoContext, int32_t count, bool depthBuffer)
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
