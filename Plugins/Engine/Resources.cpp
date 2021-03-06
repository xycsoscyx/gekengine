﻿#define _ENABLE_ATOMIC_ALIGNMENT_FIX

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
#include <concurrent_unordered_map.h>
#include <concurrent_unordered_set.h>
#include <concurrent_queue.h>
#include <concurrent_vector.h>
#include <imgui_internal.h>
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
			ThreadPool<5> &loadPool;
			ResourceHandleMap resourceHandleMap;
            ResourceMap resourceMap;

        public:
            ResourceCache(ThreadPool<5> &loadPool)
                : loadPool(loadPool)
            {
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

			void setResource(HANDLE handle, TypePtr const &data)
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
            GeneralResourceCache(ThreadPool<5> &loadPool)
                : ResourceCache(loadPool)
            {
            }

            void clear(void)
            {
                requestedLoadSet.clear();
                ResourceCache::clear();
            }

			template <typename FUNCTOR>
            std::pair<bool, HANDLE> getHandle(std::size_t hash, FUNCTOR &&load)
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
					loadPool.enqueueAndDetach([this, handle, load = std::move(load)](void) -> void
                    {
                        setResource(handle, load(handle));
                    }, __FILE__, __LINE__);

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
            DynamicResourceCache(ThreadPool<5> &loadPool)
                : ResourceCache(loadPool)
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

			template <typename FUNCTOR>
			std::pair<bool, HANDLE> getHandle(std::size_t hash, std::size_t parameters, FUNCTOR &&load, uint32_t flags)
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
									loadPool.enqueueAndDetach([this, handle, load](void) -> void
                                    {
										setResource(handle, load(handle));
                                    }, __FILE__, __LINE__);
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
						loadPool.enqueueAndDetach([this, handle, load](void) -> void
                        {
							setResource(handle, load(handle));
                        }, __FILE__, __LINE__);
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
            ProgramResourceCache(ThreadPool<5> &loadPool)
                : ResourceCache(loadPool)
            {
            }

			template <typename FUNCTOR>
            HANDLE getHandle(FUNCTOR &&load)
            {
                HANDLE handle;
                handle = getNextHandle();
				loadPool.enqueueAndDetach([this, handle, load](void) -> void
                {
                    setResource(handle, load(handle));
                }, __FILE__, __LINE__);

                return handle;
            }
        };

		template <class HANDLE, typename TYPE>
		class StaticProgramResourceCache
			: public ResourceCache<HANDLE, TYPE>
		{
		public:
			StaticProgramResourceCache(ThreadPool<5> &loadPool)
				: ResourceCache(loadPool)
			{
			}

			template <typename FUNCTOR>
			HANDLE getHandle(FUNCTOR &&load)
			{
				HANDLE handle;
				handle = getNextHandle();
				setResource(handle, load(handle));
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
            ReloadResourceCache(ThreadPool<5> &loadPool)
                : ResourceCache(loadPool)
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

			template <typename FUNCTOR>
            std::pair<bool, HANDLE> getHandle(std::size_t hash, FUNCTOR &&load)
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
            bool set(std::vector<INPUT>const  &inputList, ResourceCache<HANDLE, SOURCE> &cache, TYPE *defaultObject = nullptr)
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
            bool set(std::vector<INPUT> const &inputList, ResourceCache<HANDLE, TYPE> &cache)
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

        GEK_CONTEXT_USER(Resources, Engine::Core *)
            , public Engine::Resources
        {
        private:
            Engine::Core *core = nullptr;
            Video::Device *videoDevice = nullptr;
            Plugin::Renderer *renderer = nullptr;

            ThreadPool<5> loadPool;
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
            Resources(Context *context, Engine::Core *core)
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

                return (videoPipeline->getType() == Video::Device::Context::Pipeline::Type::Compute ? dispatchValid : drawPrimitiveValid);
            }

            Video::TexturePtr loadTextureData(FileSystem::Path const &filePath, std::string_view textureName, uint32_t flags)
            {
                auto texture = videoDevice->loadTexture(filePath, flags);
                if (texture)
                {
                    texture->setName(textureName);
                }

                return texture;
            }

            // Renderer
            bool showResources = false;
            void onShowUserInterface(void)
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
                        ImGui::End();
                    }
                }
            }

			template <typename CACHE, typename FUNCTOR>
			void showObjectMap(CACHE &cache, std::string_view name, FUNCTOR &&onObject)
			{
				auto lowerName = String::GetLower(name);
				if (ImGui::TreeNodeEx(name.data(), ImGuiTreeNodeFlags_Framed))
				{
					auto &resourceMap = cache.getResourceMap();
					for (auto &resourcePair : resourceMap)
					{
						auto handle = resourcePair.first;
						auto &object = resourcePair.second;
						std::string nodeName(object->getName());
						if (nodeName.empty())
						{
							nodeName = String::Format("{}_{}", lowerName, static_cast<uint64_t>(handle.identifier));
						}

						if (ImGui::TreeNodeEx(nodeName.data(), ImGuiTreeNodeFlags_Framed))
						{
							onObject(object);
							ImGui::TreePop();
						}
					}

					ImGui::TreePop();
				}
			}

			template <typename CACHE, typename FUNCTOR>
			void showVideoResourceMap(CACHE &cache, std::string_view name, FUNCTOR &&onObject)
            {
                if (ImGui::TreeNodeEx(name.data(), ImGuiTreeNodeFlags_Framed))
                {
                    auto lowerName = String::GetLower(name);
                    std::unordered_map<std::string_view, std::vector<CACHE::ResourceMap::value_type>> typeDataMap;
                    auto &resourceMap = cache.getResourceMap();
                    for (auto &resourcePair : resourceMap)
                    {
                        auto &object = resourcePair.second;
						if (dynamic_cast<Video::Target *>(object.get()))
						{
							typeDataMap["Target"sv].push_back(resourcePair);
						}
						else if (dynamic_cast<Video::Texture *>(object.get()))
						{
							typeDataMap["Texture"sv].push_back(resourcePair);
						}
						else if (dynamic_cast<Video::Buffer *>(object.get()))
						{
							typeDataMap["Buffer"sv].push_back(resourcePair);
						}
						else
						{
							auto program = dynamic_cast<Video::Program *>(object.get());
							if (program)
							{
								static const std::unordered_map<Video::Program::Type, std::string_view> ProgramTypeMap = 
								{
									{ Video::Program::Type::Compute, "Compute"sv },
									{ Video::Program::Type::Geometry, "Geometry"sv },
									{ Video::Program::Type::Vertex, "Vertex"sv },
									{ Video::Program::Type::Pixel, "Pixel"sv },
								};

								auto typeSearch = ProgramTypeMap.find(program->getInformation().type);
								if (typeSearch != std::end(ProgramTypeMap))
								{
									typeDataMap[typeSearch->second].push_back(resourcePair);
								}
							}
						}
					}

                    for (auto &typePair : typeDataMap)
                    {
                        if (ImGui::TreeNodeEx(typePair.first.data(), ImGuiTreeNodeFlags_Framed))
                        {
                            for (auto &resourcePair : typePair.second)
                            {
                                auto handle = resourcePair.first;
                                auto &object = resourcePair.second;
                                auto nodeName = String::Format("{} - {}", static_cast<uint64_t>(handle.identifier), (object->getName().empty() ? lowerName : object->getName()));
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
				showObjectMap(visualCache, "Visuals"s, [&](auto &object) -> void
				{
				});
			}

            void showShaderCache(void)
            {
				showObjectMap(shaderCache, "Shaders"s, [&](auto &object) -> void
				{
				});
			}

            void showFilterCache(void)
            {
				showObjectMap(filterCache, "Filters"s, [&](auto &object) -> void
				{
				});
			}

            void showProgramCache(void)
            {
				if (ImGui::TreeNodeEx("Programs", ImGuiTreeNodeFlags_Framed))
				{
					showVideoResourceMap(staticProgramCache, "Global"s, [&](auto &object) -> void
					{
						auto &information = object->getInformation();
						auto &pathString = information.debugPath.getString();
						ImGui::PushItemWidth(-1.0f);
						ImGui::InputText("##path", const_cast<char *>(pathString.data()), pathString.size(), ImGuiInputTextFlags_ReadOnly);
						ImGui::InputTextMultiline("##data", const_cast<char *>(information.uncompiledData.data()), information.uncompiledData.size(), ImVec2(-1.0f, 500.0f), ImGuiInputTextFlags_ReadOnly);
						ImGui::PopItemWidth();
					});

					showVideoResourceMap(programCache, "Local"s, [&](auto &object) -> void
					{
						auto &information = object->getInformation();
						auto &pathString = information.debugPath.getString();
						ImGui::PushItemWidth(-1.0f);
						ImGui::InputText("##path", const_cast<char *>(pathString.data()), pathString.size(), ImGuiInputTextFlags_ReadOnly);
						ImGui::InputTextMultiline("##data", const_cast<char *>(information.uncompiledData.data()), information.uncompiledData.size(), ImVec2(-1.0f, 500.0f), ImGuiInputTextFlags_ReadOnly);
						ImGui::PopItemWidth();
					});

					ImGui::TreePop();
				}
			}

            void showMaterialCache(void)
            {
                showObjectMap(materialCache, "Materials"s, [&](auto &object) -> void
				{
				});
            }

            void showResourceValue(std::string_view text, std::string_view label, std::string &value)
            {
                ImGui::AlignTextToFramePadding();
                ImGui::Text(text.data());
                ImGui::SameLine();
                ImGui::PushItemWidth(-1.0f);
                UI::InputString(label.data(), value, ImGuiInputTextFlags_ReadOnly);
                ImGui::PopItemWidth();
            }

            void showDynamicCache(void)
            {
                showVideoResourceMap(dynamicCache, "Resources"s, [&](auto &object) -> void
                {
					if (dynamic_cast<Video::Texture *>(object.get()))
                    {
                        auto texture = dynamic_cast<Video::Texture *>(object.get());
                        auto const &description = texture->getDescription();
                        float width = ImGui::GetContentRegionAvailWidth();
                        float ratio = (width / float(description.width));
                        float height = (float(description.height) * ratio);
                        showResourceValue("Format", "##format", Video::GetFormat(description.format));
                        showResourceValue("Width", "##width", std::to_string(description.width));
                        showResourceValue("Height", "##height", std::to_string(description.height));
                        showResourceValue("Depth", "##depth", std::to_string(description.depth));
                        showResourceValue("MipMap Levels", "##mipMapCount", std::to_string(description.mipMapCount));
                        showResourceValue("MultiSample Count", "##sampleCount", std::to_string(description.sampleCount));
                        showResourceValue("MultiSample Quality", "##sampleQuality", std::to_string(description.sampleQuality));
                        showResourceValue("Flags", "##flags", std::to_string(description.flags));
						ImGui::Image(reinterpret_cast<ImTextureID>(texture), ImVec2(width, height));
					}
					else if (dynamic_cast<Video::Buffer *>(object.get()))
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
                showObjectMap(renderStateCache, "Render States"s, [&](auto &object) -> void
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

            void showStencilState(std::string_view text, Video::DepthState::Description::StencilState const &stencilState)
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
                showObjectMap(depthStateCache, "Depth States"s, [&](auto &object) -> void
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
                showObjectMap(blendStateCache, "Blend States"s, [&](auto &object) -> void
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

            // Plugin::Resources
            VisualHandle loadVisual(std::string_view visualName)
            {
                auto hash = GetHash(visualName);
                return visualCache.getHandle(hash, [this, visualName = std::string(visualName)](VisualHandle)->Engine::VisualPtr
				{
					return getContext()->createClass<Engine::Visual>("Engine::Visual", videoDevice, (Engine::Resources *)this, visualName);
				}).second;
            }

            MaterialHandle loadMaterial(std::string_view materialName)
            {
                auto hash = GetHash(materialName);
                return materialCache.getHandle(hash, [this, materialName = std::string(materialName)](MaterialHandle handle)->Engine::MaterialPtr
				{
					return getContext()->createClass<Engine::Material>("Engine::Material", (Engine::Resources *)this, materialName, handle);
				}).second;
            }

            ResourceHandle loadTexture(std::string_view textureName, uint32_t flags)
            {
                // iterate over formats in case the texture name has no extension
				static constexpr std::string_view formatList[] =
                {
                    ""sv,
                    ".dds"sv,
                    ".tga"sv,
                    ".png"sv,
                    ".jpg"sv,
                    ".jpeg"sv,
                    ".tif"sv,
                    ".tiff"sv,
                    ".bmp"sv,
                };

				auto hash = GetHash(textureName);
				for (auto const &format : formatList)
                {
                    auto texturePath(getContext()->findDataPath(FileSystem::CombinePaths("textures", textureName).withExtension(format)));
                    if (texturePath.isFile())
                    {
                        auto resource = dynamicCache.getHandle(hash, flags, [this, filePath = FileSystem::Path(texturePath), textureName = std::string(textureName), flags](ResourceHandle)->Video::TexturePtr
						{
							return loadTextureData(filePath, textureName, flags);
						}, false);

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

            ResourceHandle createPattern(std::string_view pattern, JSON const &parameters)
            {
                auto lowerPattern = String::GetLower(pattern);

                std::vector<uint8_t> data;
                Video::Texture::Description description;
                if (lowerPattern == "color")
                {
                    auto parametersArray = parameters.asType(JSON::EmptyArray);
                    switch (parametersArray.size())
                    {
                    case 1:
                        data.push_back(parametersArray.at(0).convert(255));
                        description.format = Video::Format::R8_UNORM;
                        break;

                    case 2:
                        data.push_back(parametersArray.at(0).convert(255));
                        data.push_back(parametersArray.at(1).convert(255));
                        description.format = Video::Format::R8G8_UNORM;
                        break;

                    case 3:
                        data.push_back(parametersArray.at(0).convert(255));
                        data.push_back(parametersArray.at(1).convert(255));
                        data.push_back(parametersArray.at(2).convert(255));
                        data.push_back(0);
                        description.format = Video::Format::R8G8B8A8_UNORM;
                        break;

                    case 4:
                        data.push_back(parametersArray.at(0).convert(255));
                        data.push_back(parametersArray.at(1).convert(255));
                        data.push_back(parametersArray.at(2).convert(255));
                        data.push_back(parametersArray.at(3).convert(255));
                        description.format = Video::Format::R8G8B8A8_UNORM;
                        break;

                    default:
                        if (true)
                        {
                            union
                            {
                                float value;
                                uint8_t quarters[4];
                            };

                            value = parameters.evaluate(shuntingYard, 1.0f);
                            data.push_back(quarters[0]);
                            data.push_back(quarters[1]);
                            data.push_back(quarters[2]);
                            data.push_back(quarters[3]);
                            description.format = Video::Format::R32_FLOAT;
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

                    Math::Float3 normal = parameters.evaluate(shuntingYard, Math::Float3::Zero);

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
                std::string name(String::Format("{}:{}", pattern, parameters.convert(String::Empty)));
                auto hash = GetHash(name);

                auto resource = dynamicCache.getHandle(hash, 0, [this, name, description, data = move(data)](ResourceHandle)->Video::TexturePtr
				{
					auto texture = videoDevice->createTexture(description, data.data());
                    if (texture)
                    {
                        texture->setName(name);
                    }

                    return texture;
				}, false);

                if (resource.first)
                {
                    textureDescriptionMap.insert(std::make_pair(resource.second, description));
                }

                return resource.second;
            }

            ResourceHandle createTexture(std::string_view textureName, Video::Texture::Description const &description, uint32_t flags)
            {
                auto hash = GetHash(textureName);
                auto parameters = description.getHash();
                if (description.format == Video::Format::Unknown)
                {
                    flags |= Resources::Flags::LoadFromCache;
                }

                auto resource = dynamicCache.getHandle(hash, parameters, [this, textureName = std::string(textureName), description](ResourceHandle)->Video::TexturePtr
				{
					auto texture = videoDevice->createTexture(description);
                    if (texture)
                    {
                        texture->setName(textureName);
                    }

                    return texture;
				}, flags);

                if (resource.first)
                {
                    textureDescriptionMap.insert(std::make_pair(resource.second, description));
                }

                return resource.second;
            }

            ResourceHandle createBuffer(std::string_view bufferName, Video::Buffer::Description const &description, uint32_t flags)
            {
                assert(description.count > 0);

                auto hash = GetHash(bufferName);
                auto parameters = description.getHash();
                if (description.format == Video::Format::Unknown)
                {
                    flags |= Resources::Flags::LoadFromCache;
                }

                auto resource = dynamicCache.getHandle(hash, parameters, [this, bufferName = std::string(bufferName), description](ResourceHandle)->Video::BufferPtr
				{
					auto buffer = videoDevice->createBuffer(description);
                    if (buffer)
                    {
                        buffer->setName(bufferName);
                    }

                    return buffer;
				}, flags);

				if (resource.first)
                {
                    bufferDescriptionMap.insert(std::make_pair(resource.second, description));
                }

                return resource.second;
            }

            ResourceHandle createBuffer(std::string_view bufferName, Video::Buffer::Description const &description, std::vector<uint8_t> &&staticData, uint32_t flags)
            {
                assert(description.count > 0);
                assert(!staticData.empty());

                auto hash = GetHash(bufferName);
                auto parameters = reinterpret_cast<std::size_t>(staticData.data());
                if (description.format == Video::Format::Unknown)
                {
                    flags |= Resources::Flags::LoadFromCache;
                }

                auto resource = dynamicCache.getHandle(hash, parameters, [this, bufferName = std::string(bufferName), description, staticData = move(staticData)](ResourceHandle)->Video::BufferPtr
                {
                    auto buffer = videoDevice->createBuffer(description, (void *)staticData.data());
                    if (buffer)
                    {
                        buffer->setName(bufferName);
                    }

                    return buffer;
                }, flags);

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
            void setVertexBufferList(Video::Device::Context *videoContext, std::vector<ResourceHandle> const &resourceHandleList, uint32_t firstSlot, uint32_t *offsetList)
            {
                assert(videoContext);

                if (drawPrimitiveValid && (drawPrimitiveValid = vertexBufferCache.set(resourceHandleList, dynamicCache)))
                {
                    videoContext->setVertexBufferList(vertexBufferCache.get(), firstSlot, offsetList);
                }
            }

            ObjectCache<Video::Buffer> constantBufferCache;
            void setConstantBufferList(Video::Device::Context::Pipeline *videoPipeline, std::vector<ResourceHandle> const &resourceHandleList, uint32_t firstStage)
            {
                assert(videoPipeline);

                auto &valid = getValid(videoPipeline);
                if (valid && (valid = constantBufferCache.set(resourceHandleList, dynamicCache)))
                {
                    videoPipeline->setConstantBufferList(constantBufferCache.get(), firstStage);
                }
            }

            ObjectCache<Video::Object> resourceCache;
            void setResourceList(Video::Device::Context::Pipeline *videoPipeline, std::vector<ResourceHandle> const &resourceHandleList, uint32_t firstStage)
            {
                assert(videoPipeline);

                auto &valid = getValid(videoPipeline);
                if (valid && (valid = resourceCache.set(resourceHandleList, dynamicCache, videoDevice->getBackBuffer())))
                {
                    videoPipeline->setResourceList(resourceCache.get(), firstStage);
                }
            }

            ObjectCache<Video::Object> unorderedAccessCache;
            void setUnorderedAccessList(Video::Device::Context::Pipeline *videoPipeline, std::vector<ResourceHandle> const &resourceHandleList, uint32_t firstStage)
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

            Engine::Shader * const getShader(ShaderHandle handle) const
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

            Engine::Filter * const getFilter(std::string_view filterName)
            {
                auto hash = GetHash(filterName);
                auto resource = filterCache.getHandle(hash, [this, filterName = std::string(filterName)](ResourceHandle)->Engine::FilterPtr
				{
					return getContext()->createClass<Engine::Filter>("Engine::Filter", core, filterName);
				});

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

			Video::Program::Information getProgramInformation(Video::Program::Type type, std::string_view name, std::string_view entryFunction, std::string_view engineData)
            {
				auto programsPath(getContext()->findDataPath("programs"s, false));
				auto filePath(FileSystem::CombinePaths(programsPath, name));
				auto programDirectory(filePath.getParentPath());
				std::string uncompiledData(FileSystem::Load(filePath, std::string(engineData)));

				auto hash = GetHash(name, uncompiledData, engineData);
				auto cachePath = getContext()->getCachePath(FileSystem::CombinePaths("programs", name));
				auto uncompiledPath(cachePath.withExtension(String::Format(".{}.hlsl", hash)));
				auto compiledPath(cachePath.withExtension(String::Format(".{}.bin", hash)));
				Video::Program::Information information =
				{
					type
				};

                if (compiledPath.isFile())
                {
					static const std::vector<uint8_t> EmptyBuffer;
					information.compiledData = FileSystem::Load(compiledPath, EmptyBuffer);
				}
                
                if (information.compiledData.empty())
                {
					std::map<std::string_view, std::string> includedMap;
					auto onInclude = [programsPath, programDirectory, &includedMap, engineData](Video::IncludeType includeType, std::string_view fileName, void const **data, uint32_t *size) -> bool
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
									auto localPath(FileSystem::CombinePaths(programDirectory, fileName));
									if (localPath.isFile())
									{
										includedMap[fileName] = FileSystem::Load(localPath, String::Empty);
										(*data) = includedMap[fileName].data();
										(*size) = includedMap[fileName].size();
										return true;
									}
								}

								break;

							case Video::IncludeType::Global:
								if (true)
								{
									auto rootPath(FileSystem::CombinePaths(programsPath, fileName));
									if (rootPath.isFile())
									{
										includedMap[fileName] = FileSystem::Load(rootPath, String::Empty);
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
				else
				{
					information.debugPath = uncompiledPath;
					information.uncompiledData = std::move(uncompiledData);
				}

                return information;
            }

			Video::Program * getProgram(Video::Program::Type type, std::string_view name, std::string_view entryFunction, std::string_view engineData)
			{
				auto handle = staticProgramCache.getHandle([this, type, name = std::string(name), entryFunction = std::string(entryFunction), engineData = std::string(engineData)](ProgramHandle)->Video::ProgramPtr
				{
					auto compiledData = getProgramInformation(type, name, entryFunction, engineData);
					auto program = videoDevice->createProgram(compiledData);
                    if (program)
                    {
                        program->setName(String::Format("{}:{}", name, entryFunction));
                    }

                    return program;
				});

				return staticProgramCache.getResource(handle);
			}

			ProgramHandle loadProgram(Video::Program::Type type, std::string_view name, std::string_view entryFunction, std::string_view engineData)
			{
				return programCache.getHandle([this, type, name = std::string(name), entryFunction = std::string(entryFunction), engineData = std::string(engineData)](ProgramHandle)->Video::ProgramPtr
				{
					auto compiledData = getProgramInformation(type, name, entryFunction, engineData);
					auto program = videoDevice->createProgram(compiledData);
                    if (program)
                    {
                        program->setName(String::Format("{}:{}", name, entryFunction));
                    }

                    return program;
				});
			}

			RenderStateHandle createRenderState(Video::RenderState::Description const &description)
            {
                auto hash = description.getHash();
                return renderStateCache.getHandle(hash, [this, description](RenderStateHandle) -> Video::RenderStatePtr
				{
					auto state = videoDevice->createRenderState(description);
					//state->setName(stateName);
					return state;
				}).second;
            }

            DepthStateHandle createDepthState(Video::DepthState::Description const &description)
            {
                auto hash = description.getHash();
                return depthStateCache.getHandle(hash, [this, description](DepthStateHandle) -> Video::DepthStatePtr
				{
					auto state = videoDevice->createDepthState(description);
					//state->setName(stateName);
					return state;
				}).second;
            }

            BlendStateHandle createBlendState(Video::BlendState::Description const &description)
            {
                auto hash = description.getHash();
                return blendStateCache.getHandle(hash, [this, description](BlendStateHandle) -> Video::BlendStatePtr
				{
					auto state = videoDevice->createBlendState(description);
					//state->setName(stateName);
					return state;
				}).second;
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
            void setRenderTargetList(Video::Device::Context *videoContext, std::vector<ResourceHandle> const &renderTargetHandleList, ResourceHandle const *depthBuffer)
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
