#include "GEK\Utility\String.h"
#include "GEK\Utility\Evaluator.h"
#include "GEK\Utility\FileSystem.h"
#include "GEK\Utility\XML.h"
#include "GEK\Shapes\Sphere.h"
#include "GEK\Context\ContextUser.h"
#include "GEK\Engine\Engine.h"
#include "GEK\Engine\Plugin.h"
#include "GEK\Engine\Render.h"
#include "GEK\Engine\Resources.h"
#include "GEK\Engine\Shader.h"
#include "GEK\Engine\Material.h"
#include "GEK\Engine\Population.h"
#include "GEK\Engine\Entity.h"
#include "GEK\Engine\Component.h"
#include "GEK\Components\Transform.h"
#include "GEK\Components\Camera.h"
#include "GEK\Components\Light.h"
#include "GEK\Components\Color.h"
#include "GEK\Context\ContextUser.h"
#include <concurrent_unordered_map.h>
#include <concurrent_unordered_set.h>
#include <concurrent_queue.h>
#include <ppl.h>
#include <future>
#include <set>

namespace Gek
{
    namespace String
    {
        template <typename CHAR>
        CStringT<CHAR, StrTraitATL<CHAR, ChTraitsCRT<CHAR>>> from(Video::Format value)
        {
            std::basic_stringstream<CHAR, std::char_traits<CHAR>, std::allocator<CHAR>> stream;
            stream << static_cast<UINT8>(value);
            return stream.str().c_str();
        }

        template <typename CHAR>
        CStringT<CHAR, StrTraitATL<CHAR, ChTraitsCRT<CHAR>>> from(Video::BufferType value)
        {
            std::basic_stringstream<CHAR, std::char_traits<CHAR>, std::allocator<CHAR>> stream;
            stream << static_cast<UINT8>(value);
            return stream.str().c_str();
        }
    }; // namespace String

    template <class HANDLE, typename TYPE>
    class ResourceManager
    {
    public:
        typedef std::shared_ptr<TYPE> TypePtr;

        struct AtomicResource
        {
            enum class State : UINT8
            {
                Empty = 0,
                Loading,
                Loaded,
            };

            std::atomic<State> state;
            TypePtr data;

            AtomicResource(void)
                : state(State::Empty)
            {
            }

            AtomicResource(const AtomicResource &resource)
                : state(resource.state == State::Empty ? State::Empty : resource.state == State::Loading ? State::Loading : State::Loaded)
                , data(resource.data)
            {
            }

            void set(TypePtr data)
            {
                if (state == State::Empty)
                {
                    state = State::Loading;
                    this->data = data;
                    state = State::Loaded;
                }
            }

            TYPE * const get(void) const
            {
                return (state == State::Loaded ? data.get() : nullptr);
            }
        };

        struct ReadWriteResource
        {
            AtomicResource read;
            AtomicResource write;

            ReadWriteResource(void)
            {
            }

            void setRead(TypePtr data)
            {
                this->read.set(data);
            }

            void setWrite(TypePtr data)
            {
                this->write.set(data);
            }
        };

    private:
        UINT64 nextIdentifier;
        concurrency::concurrent_unordered_set<std::size_t> requestedLoadSet;
        concurrency::concurrent_unordered_map<std::size_t, HANDLE> resourceHandleMap;
        concurrency::concurrent_unordered_map<HANDLE, AtomicResource> localResourceMap;
        concurrency::concurrent_unordered_map<HANDLE, ReadWriteResource> readWriteResourceMap;
        concurrency::concurrent_unordered_map<HANDLE, AtomicResource> globalResourceMap;

    public:
        ResourceManager(void)
            : nextIdentifier(0)
        {
        }

        ~ResourceManager(void)
        {
        }

        void clear(void)
        {
            requestedLoadSet.clear();
            resourceHandleMap.clear();
            localResourceMap.clear();
            readWriteResourceMap.clear();
        }

        HANDLE getGlobalHandle(std::function<void(HANDLE, std::function<void(TypePtr)>)> requestLoad)
        {
            HANDLE handle;
            handle.assign(InterlockedIncrement(&nextIdentifier));
            requestLoad(handle, [this, handle](TypePtr data) -> void
            {
                auto &resource = globalResourceMap[handle];
                resource.set(data);
            });
            return handle;
        }

        HANDLE getUniqueHandle(std::function<void(HANDLE, std::function<void(TypePtr)>)> requestLoad)
        {
            HANDLE handle;
            handle.assign(InterlockedIncrement(&nextIdentifier));
            requestLoad(handle, [this, handle](TypePtr data) -> void
            {
                auto &resource = localResourceMap[handle];
                resource.set(data);
            });
            return handle;
        }

        HANDLE getUniqueReadWriteHandle(std::function<void(HANDLE, std::function<void(TypePtr)>)> requestLoad)
        {
            HANDLE handle;
            handle.assign(InterlockedIncrement(&nextIdentifier));
            requestLoad(handle, [this, handle](TypePtr data) -> void
            {
                auto &resource = readWriteResourceMap[handle];
                resource.setRead(data);
            });
            requestLoad(handle, [this, handle](TypePtr data) -> void
            {
                auto &resource = readWriteResourceMap[handle];
                resource.setWrite(data);
            });
            return handle;
        }

        HANDLE getHandle(std::size_t hash, std::function<void(HANDLE, std::function<void(TypePtr)>)> requestLoad)
        {
            HANDLE handle;
            if (requestedLoadSet.count(hash) > 0)
            {
                auto resourceIterator = resourceHandleMap.find(hash);
                if (resourceIterator != resourceHandleMap.end())
                {
                    handle = resourceIterator->second;
                }
            }
            else
            {
                requestedLoadSet.insert(hash);
                handle.assign(InterlockedIncrement(&nextIdentifier));
                resourceHandleMap[hash] = handle;
                requestLoad(handle, [this, handle](TypePtr data) -> void
                {
                    auto &resource = localResourceMap[handle];
                    resource.set(data);
                });
            }

            return handle;
        }

        HANDLE getReadWriteHandle(std::size_t hash, std::function<void(HANDLE, std::function<void(TypePtr)>)> requestLoad)
        {
            HANDLE handle;
            if (requestedLoadSet.count(hash) > 0)
            {
                auto resourceIterator = resourceHandleMap.find(hash);
                if (resourceIterator != resourceHandleMap.end())
                {
                    handle = resourceIterator->second;
                }
            }
            else
            {
                requestedLoadSet.insert(hash);
                handle.assign(InterlockedIncrement(&nextIdentifier));
                resourceHandleMap[hash] = handle;
                requestLoad(handle, [this, handle](TypePtr data) -> void
                {
                    auto &resource = readWriteResourceMap[handle];
                    resource.setRead(data);
                });
                requestLoad(handle, [this, handle](TypePtr data) -> void
                {
                    auto &resource = readWriteResourceMap[handle];
                    resource.setWrite(data);
                });
            }

            return handle;
        }

        HANDLE getHandle(std::size_t hash) const
        {
            if (requestedLoadSet.count(hash) > 0)
            {
                auto resourceIterator = resourceHandleMap.find(hash);
                if (resourceIterator != resourceHandleMap.end())
                {
                    return resourceIterator->second;
                }
            }

            return HANDLE();
        }

        TYPE * const getResource(HANDLE handle, bool writable = false) const
        {
            auto globalIterator = globalResourceMap.find(handle);
            if (globalIterator != globalResourceMap.end())
            {
                return globalIterator->second.get();
            }

            auto localIterator = localResourceMap.find(handle);
            if (localIterator != localResourceMap.end())
            {
                return localIterator->second.get();
            }

            auto readWriteIterator = readWriteResourceMap.find(handle);
            if (readWriteIterator != readWriteResourceMap.end())
            {
                auto &resource = readWriteIterator->second;
                return (writable ? resource.write.get() : resource.read.get());
            }

            return nullptr;
        }

        void flipResource(HANDLE handle)
        {
            auto readWriteIterator = readWriteResourceMap.find(handle);
            if (readWriteIterator != readWriteResourceMap.end())
            {
                auto &resource = readWriteIterator->second;
                if (resource.read.state == AtomicResource::State::Loaded &&
                    resource.write.state == AtomicResource::State::Loaded)
                {
                    std::swap(resource.read.data, resource.write.data);
                }
            }
        }
    };

    class ResourcesImplementation 
        : public ContextRegistration<ResourcesImplementation, EngineContext *>
        , public Resources
    {
    private:
        VideoSystem *video;

        ResourceManager<ProgramHandle, VideoObject> programManager;
        ResourceManager<PluginHandle, Plugin> pluginManager;
        ResourceManager<MaterialHandle, Material> materialManager;
        ResourceManager<ShaderHandle, Shader> shaderManager;
        ResourceManager<ResourceHandle, VideoObject> resourceManager;
        ResourceManager<RenderStateHandle, VideoObject> renderStateManager;
        ResourceManager<DepthStateHandle, VideoObject> depthStateManager;
        ResourceManager<BlendStateHandle, VideoObject> blendStateManager;

        concurrency::concurrent_unordered_map<MaterialHandle, ShaderHandle> materialShaderMap;

        std::future<void> loadResourceRunning;
        concurrency::concurrent_queue<std::function<void(void)>> loadResourceQueue;

    public:
        ResourcesImplementation(Context *context, EngineContext *engine)
            : ContextRegistration(context)
            , video(engine->getRender()->getVideoSystem())
        {
        }

        void requestLoad(std::function<void(void)> load)
        {
            loadResourceQueue.push(load);
            if (!loadResourceRunning.valid() || (loadResourceRunning.wait_for(std::chrono::milliseconds(0)) == std::future_status::ready))
            {
                loadResourceRunning = std::async(std::launch::async, [this](void) -> void
                {
                    CoInitialize(nullptr);
                    std::function<void(void)> function;
                    while (loadResourceQueue.try_pop(function))
                    {
                        function();
                    };

                    CoUninitialize();
                });
            }
        }

        // Resources
        void clearLocal(void)
        {
            loadResourceQueue.clear();
            if (loadResourceRunning.valid() && (loadResourceRunning.wait_for(std::chrono::milliseconds(0)) != std::future_status::ready))
            {
                loadResourceRunning.get();
            }

            materialShaderMap.clear();
            programManager.clear();
            materialManager.clear();
            shaderManager.clear();
            resourceManager.clear();
            renderStateManager.clear();
            depthStateManager.clear();
            blendStateManager.clear();
        }

        ShaderHandle getMaterialShader(MaterialHandle material) const
        {
            auto shader = materialShaderMap.find(material);
            if (shader != materialShaderMap.end())
            {
                return (*shader).second;
            }

            return ShaderHandle();
        }

        ResourceHandle getResourceHandle(const wchar_t *name) const
        {
            std::size_t hash = std::hash<wstring>()(name);
            return resourceManager.getHandle(hash);
        }

        Shader * const getShader(ShaderHandle handle) const
        {
            return shaderManager.getResource(handle, false);
        }

        Plugin * const getPlugin(PluginHandle handle) const
        {
            return pluginManager.getResource(handle, false);
        }

        Material * const getMaterial(MaterialHandle handle) const
        {
            return materialManager.getResource(handle, false);
        }

        PluginHandle loadPlugin(const wstring &fileName)
        {
            auto load = [this, fileName](PluginHandle handle) -> PluginPtr
            {
                return getContext()->createClass<Plugin>(L"PluginSystem");
            };

            auto request = [this, load](PluginHandle handle, std::function<void(PluginPtr)> set) -> void
            {
                set(load(handle));
            };

            return pluginManager.getGlobalHandle(request);
        }

        MaterialHandle loadMaterial(const wstring &fileName)
        {
            auto load = [this, fileName](MaterialHandle handle) -> MaterialPtr
            {
                return getContext()->createClass<Material>(L"MaterialSystem");
            };

            auto request = [this, load](MaterialHandle handle, std::function<void(MaterialPtr)> set) -> void
            {
                MaterialPtr material = load(handle);
                materialShaderMap[handle] = material->getShader();
                set(load(handle));
            };

            std::size_t hash = std::hash<wstring>()(fileName);
            return materialManager.getHandle(hash, request);
        }

        ShaderHandle loadShader(const wstring &fileName)
        {
            auto load = [this, fileName](ShaderHandle handle) -> ShaderPtr
            {
                return getContext()->createClass<Shader>(L"ShaderSystem");
            };

            auto request = [this, load](ShaderHandle handle, std::function<void(ShaderPtr)> set) -> void
            {
                set(load(handle));
            };

            std::size_t hash = std::hash<wstring>()(fileName);
            return shaderManager.getHandle(hash, request);
        }

        void loadResourceList(ShaderHandle shaderHandle, const wchar_t *materialName, std::unordered_map<wstring, wstring> &resourceMap, std::list<ResourceHandle> &resourceList)
        {
            Shader *shader = shaderManager.getResource(shaderHandle);
            if (shader)
            {
                shader->loadResourceList(materialName, resourceMap, resourceList);
            }
        }

        RenderStateHandle createRenderState(const Video::RenderState &renderState)
        {
            auto load = [this, renderState](RenderStateHandle handle) -> VideoObjectPtr
            {
                return video->createRenderState(renderState);
            };

            auto request = [this, load](RenderStateHandle handle, std::function<void(VideoObjectPtr)> set) -> void
            {
                set(load(handle));
            };

            std::size_t hash = std::hash_combine(static_cast<UINT8>(renderState.fillMode),
                static_cast<UINT8>(renderState.cullMode),
                renderState.frontCounterClockwise,
                renderState.depthBias,
                renderState.depthBiasClamp,
                renderState.slopeScaledDepthBias,
                renderState.depthClipEnable,
                renderState.scissorEnable,
                renderState.multisampleEnable,
                renderState.antialiasedLineEnable);
            return renderStateManager.getHandle(hash, request);
        }

        DepthStateHandle createDepthState(const Video::DepthState &depthState)
        {
            auto load = [this, depthState](DepthStateHandle handle) -> VideoObjectPtr
            {
                return video->createDepthState(depthState);
            };

            auto request = [this, load](DepthStateHandle handle, std::function<void(VideoObjectPtr)> set) -> void
            {
                set(load(handle));
            };

            std::size_t hash = std::hash_combine(depthState.enable,
                static_cast<UINT8>(depthState.writeMask),
                static_cast<UINT8>(depthState.comparisonFunction),
                depthState.stencilEnable,
                depthState.stencilReadMask,
                depthState.stencilWriteMask,
                static_cast<UINT8>(depthState.stencilFrontState.failOperation),
                static_cast<UINT8>(depthState.stencilFrontState.depthFailOperation),
                static_cast<UINT8>(depthState.stencilFrontState.passOperation),
                static_cast<UINT8>(depthState.stencilFrontState.comparisonFunction),
                static_cast<UINT8>(depthState.stencilBackState.failOperation),
                static_cast<UINT8>(depthState.stencilBackState.depthFailOperation),
                static_cast<UINT8>(depthState.stencilBackState.passOperation),
                static_cast<UINT8>(depthState.stencilBackState.comparisonFunction));
            return depthStateManager.getHandle(hash, request);
        }

        BlendStateHandle createBlendState(const Video::UnifiedBlendState &blendState)
        {
            auto load = [this, blendState](BlendStateHandle handle) -> VideoObjectPtr
            {
                return video->createBlendState(blendState);
            };

            auto request = [this, load](BlendStateHandle handle, std::function<void(VideoObjectPtr)> set) -> void
            {
                set(load(handle));
            };

            std::size_t hash = std::hash_combine(blendState.enable,
                static_cast<UINT8>(blendState.colorSource),
                static_cast<UINT8>(blendState.colorDestination),
                static_cast<UINT8>(blendState.colorOperation),
                static_cast<UINT8>(blendState.alphaSource),
                static_cast<UINT8>(blendState.alphaDestination),
                static_cast<UINT8>(blendState.alphaOperation),
                blendState.writeMask);
            return blendStateManager.getHandle(hash, request);
        }

        BlendStateHandle createBlendState(const Video::IndependentBlendState &blendState)
        {
            auto load = [this, blendState](BlendStateHandle handle) -> VideoObjectPtr
            {
                return video->createBlendState(blendState);
            };

            auto request = [this, load](BlendStateHandle handle, std::function<void(VideoObjectPtr)> set) -> void
            {
                set(load(handle));
            };

            std::size_t hash = 0;
            for (UINT32 renderTarget = 0; renderTarget < 8; ++renderTarget)
            {
                if (blendState.targetStates[renderTarget].enable)
                {
                    std::hash_combine(hash, std::hash_combine(renderTarget,
                        static_cast<UINT8>(blendState.targetStates[renderTarget].colorSource),
                        static_cast<UINT8>(blendState.targetStates[renderTarget].colorDestination),
                        static_cast<UINT8>(blendState.targetStates[renderTarget].colorOperation),
                        static_cast<UINT8>(blendState.targetStates[renderTarget].alphaSource),
                        static_cast<UINT8>(blendState.targetStates[renderTarget].alphaDestination),
                        static_cast<UINT8>(blendState.targetStates[renderTarget].alphaOperation),
                        blendState.targetStates[renderTarget].writeMask));
                }
            }

            return blendStateManager.getHandle(hash, request);
        }

        ResourceHandle createTexture(const wchar_t *name, Video::Format format, UINT32 width, UINT32 height, UINT32 depth, DWORD flags, UINT32 mipmaps)
        {
            auto load = [this, format, width, height, depth, flags, mipmaps](ResourceHandle handle) -> VideoTexturePtr
            {
                return video->createTexture(format, width, height, depth, flags, mipmaps);
            };

            auto request = [this, load](ResourceHandle handle, std::function<void(VideoTexturePtr)> set) -> void
            {
                set(load(handle));
            };

            if (name)
            {
                std::size_t hash = std::hash<const wchar_t *>()(name);
                if (flags & TextureFlags::ReadWrite ? true : false)
                {
                    return resourceManager.getReadWriteHandle(hash, request);
                }
                else
                {
                    return resourceManager.getHandle(hash, request);
                }
            }
            else
            {
                return resourceManager.getGlobalHandle(request);
            }
        }

        ResourceHandle createBuffer(const wchar_t *name, UINT32 stride, UINT32 count, Video::BufferType type, DWORD flags, const void *staticData)
        {
            auto load = [this, stride, count, type, flags, staticData](ResourceHandle handle) -> VideoBufferPtr
            {
                return video->createBuffer(stride, count, type, flags, staticData);
            };

            auto request = [this, load](ResourceHandle handle, std::function<void(VideoBufferPtr)> set) -> void
            {
                set(load(handle));
            };

            if (name)
            {
                std::size_t hash = std::hash<const wchar_t *>()(name);
                if (flags & TextureFlags::ReadWrite ? true : false)
                {
                    return resourceManager.getReadWriteHandle(hash, request);
                }
                else
                {
                    return resourceManager.getHandle(hash, request);
                }
            }
            else
            {
                return resourceManager.getGlobalHandle(request);
            }
        }

        ResourceHandle createBuffer(const wchar_t *name, Video::Format format, UINT32 count, Video::BufferType type, DWORD flags, const void *staticData)
        {
            auto load = [this, format, count, type, flags, staticData](ResourceHandle handle) -> VideoBufferPtr
            {
                return video->createBuffer(format, count, type, flags, staticData);
            };

            auto request = [this, load](ResourceHandle handle, std::function<void(VideoBufferPtr)> set) -> void
            {
                set(load(handle));
            };

            if (name)
            {
                std::size_t hash = std::hash<const wchar_t *>()(name);
                if (flags & TextureFlags::ReadWrite ? true : false)
                {
                    return resourceManager.getReadWriteHandle(hash, request);
                }
                else
                {
                    return resourceManager.getHandle(hash, request);
                }
            }
            else
            {
                return resourceManager.getGlobalHandle(request);
            }
        }

        VideoTexturePtr createTexture(wstring parameters, UINT32 flags)
        {
            VideoTexturePtr texture;
            std::vector<wstring> tokenList(parameters.getLower().split(L':'));
            GEK_THROW_ERROR(tokenList.size() != 2, BaseException, "Invalid number of parameters passed to create texture");
            if (tokenList[0].compare(L"color") == 0)
            {
                UINT32 colorPitch = 0;
                UINT8 colorData[4] = { 0, 0, 0, 0 };
                try
                {
                    float color1;
                    Evaluator::get(tokenList[1], color1);
                    texture = video->createTexture(Video::Format::Byte, 1, 1, 1, Video::TextureFlags::Resource);
                    colorData[0] = UINT8(color1 * 255.0f);
                    colorPitch = 1;
                }
                catch (Evaluator::Exception exception)
                {
                    try
                    {
                        Math::Float2 color2;
                        Evaluator::get(tokenList[1], color2);
                        texture = video->createTexture(Video::Format::Byte2, 1, 1, 1, Video::TextureFlags::Resource);
                        colorData[0] = UINT8(color2.x * 255.0f);
                        colorData[1] = UINT8(color2.y * 255.0f);
                        colorPitch = 2;
                    }
                    catch (Evaluator::Exception exception)
                    {
                        try
                        {
                            Math::Float3 color3;
                            Evaluator::get(tokenList[1], color3);
                            texture = video->createTexture(Video::Format::Byte4, 1, 1, 1, Video::TextureFlags::Resource);
                            colorData[0] = UINT8(color3.x * 255.0f);
                            colorData[1] = UINT8(color3.y * 255.0f);
                            colorData[2] = UINT8(color3.z * 255.0f);
                            colorData[3] = 255;
                            colorPitch = 4;
                        }
                        catch (Evaluator::Exception exception)
                        {
                            try
                            {
                                Math::Float4 color4;
                                Evaluator::get(tokenList[1], color4);
                                texture = video->createTexture(Video::Format::Byte4, 1, 1, 1, Video::TextureFlags::Resource);
                                colorData[0] = UINT8(color4.x * 255.0f);
                                colorData[1] = UINT8(color4.y * 255.0f);
                                colorData[2] = UINT8(color4.z * 255.0f);
                                colorData[3] = UINT8(color4.w * 255.0f);
                                colorPitch = 4;
                            }
                            catch (Evaluator::Exception exception)
                            {
                            };
                        };
                    };
                };

                GEK_THROW_ERROR(!texture, BaseException, "Unable to create texture");
                GEK_THROW_ERROR(colorPitch == 0, BaseException, "Unable to evaluate color format");
                video->updateTexture(texture.get(), colorData, colorPitch);
            }
            else if (tokenList[0].compare(L"normal") == 0)
            {
                Math::Float3 normal((Evaluator::get<Math::Float3>(tokenList[1]).getNormal() + 1.0f) * 0.5f);
                UINT8 normalData[4] = 
                {
                    UINT8(normal.x * 255.0f),
                    UINT8(normal.y * 255.0f),
                    UINT8(normal.z * 255.0f),
                    255,
                };

                texture = video->createTexture(Video::Format::Byte4, 1, 1, 1, Video::TextureFlags::Resource);
                video->updateTexture(texture.get(), normalData, 4);
            }
            else if (tokenList[0].compare(L"pattern") == 0)
            {
                if (tokenList[1].compare(L"debug") == 0)
                {
                    UINT8 data[] =
                    {
                        255, 0, 255, 255,
                    };

                    texture = video->createTexture(Video::Format::Byte4, 1, 1, 1, Video::TextureFlags::Resource);
                    video->updateTexture(texture.get(), data, 4);
                }
                else if (tokenList[1].compare(L"flat") == 0)
                {
                    UINT8 normalData[4] =
                    {
                        127,
                        127,
                        255,
                        255,
                    };

                    texture = video->createTexture(Video::Format::Byte4, 1, 1, 1, Video::TextureFlags::Resource);
                    video->updateTexture(texture.get(), normalData, 4);
                }
            }

            return texture;
        }

        VideoTexturePtr loadTexture(const wchar_t *fileName, UINT32 flags)
        {
            GEK_REQUIRE(fileName);

            if (*fileName == L'*')
            {
                return createTexture(&fileName[1], flags);
            }
            else
            {
                // iterate over formats in case the texture name has no extension
                static const wchar_t *formatList[] =
                {
                    L"",
                    L".dds",
                    L".tga",
                    L".png",
                    L".jpg",
                    L".bmp",
                };

                for (auto &format : formatList)
                {
                    wstring fullFileName(FileSystem::expandPath(String::format(L"$root\\data\\textures\\%v%v", fileName, format)));
                    if (PathFileExists(fullFileName))
                    {
                        return video->loadTexture(fullFileName, flags);
                    }
                }

                return nullptr;
            }
        }

        ResourceHandle loadTexture(const wstring &fileName, const wstring &fallback, UINT32 flags)
        {
            auto load = [this, fileName, fallback, flags](ResourceHandle handle) -> VideoTexturePtr
            {
                VideoTexturePtr texture = loadTexture(fileName, flags);
                if (!texture)
                {
                    texture = loadTexture(fallback, flags);
                }

                return texture;
            };

            auto request = [this, load](ResourceHandle handle, std::function<void(VideoTexturePtr)> set) -> void
            {
                set(load(handle));
            };

            std::size_t hash = std::hash<wstring>()(fileName);
            return resourceManager.getHandle(hash, request);
        }

        ProgramHandle loadComputeProgram(const wstring &fileName, const string &entryFunction, std::function<HRESULT(const char *, std::vector<UINT8> &)> onInclude, const std::unordered_map<string, string> &defineList)
        {
            auto load = [this, fileName, entryFunction, onInclude, defineList](ProgramHandle handle) -> VideoObjectPtr
            {
                return video->loadComputeProgram(fileName, entryFunction, onInclude, defineList);
            };

            auto request = [this, load](ProgramHandle handle, std::function<void(VideoObjectPtr)> set) -> void
            {
                set(load(handle));
            };

            return programManager.getUniqueHandle(request);
        }

        ProgramHandle loadPixelProgram(const wstring &fileName, const string &entryFunction, std::function<HRESULT(const char *, std::vector<UINT8> &)> onInclude, const std::unordered_map<string, string> &defineList)
        {
            auto load = [this, fileName, entryFunction, onInclude, defineList](ProgramHandle handle) -> VideoObjectPtr
            {
                return video->loadPixelProgram(fileName, entryFunction, onInclude, defineList);
            };

            auto request = [this, load](ProgramHandle handle, std::function<void(VideoObjectPtr)> set) -> void
            {
                set(load(handle));
            };

            return programManager.getUniqueHandle(request);
        }

        void mapBuffer(ResourceHandle buffer, void **data)
        {
            video->mapBuffer(reinterpret_cast<VideoBuffer *>(resourceManager.getResource(buffer)), data);
        }

        void unmapBuffer(ResourceHandle buffer)
        {
            video->unmapBuffer(reinterpret_cast<VideoBuffer *>(resourceManager.getResource(buffer)));
        }

        void flip(ResourceHandle resourceHandle)
        {
            resourceManager.flipResource(resourceHandle);
        }

        void generateMipMaps(RenderContext *renderContext, ResourceHandle resourceHandle)
        {
            renderContext->getContext()->generateMipMaps(reinterpret_cast<VideoTexture *>(resourceManager.getResource(resourceHandle)));
        }

        void copyResource(ResourceHandle destinationHandle, ResourceHandle sourceHandle)
        {
            video->copyResource(resourceManager.getResource(destinationHandle), resourceManager.getResource(sourceHandle));
        }

        void setRenderState(RenderContext *renderContext, RenderStateHandle renderStateHandle)
        {
            renderContext->getContext()->setRenderState(renderStateManager.getResource(renderStateHandle));
        }

        void setDepthState(RenderContext *renderContext, DepthStateHandle depthStateHandle, UINT32 stencilReference)
        {
            renderContext->getContext()->setDepthState(depthStateManager.getResource(depthStateHandle), stencilReference);
        }

        void setBlendState(RenderContext *renderContext, BlendStateHandle blendStateHandle, const Math::Color &blendFactor, UINT32 sampleMask)
        {
            renderContext->getContext()->setBlendState(blendStateManager.getResource(blendStateHandle), blendFactor, sampleMask);
        }

        void setResource(RenderPipeline *renderPipeline, ResourceHandle resourceHandle, UINT32 stage)
        {
            renderPipeline->getPipeline()->setResource(resourceManager.getResource(resourceHandle), stage);
        }

        void setUnorderedAccess(RenderPipeline *renderPipeline, ResourceHandle resourceHandle, UINT32 stage)
        {
            renderPipeline->getPipeline()->setUnorderedAccess(resourceManager.getResource(resourceHandle, true), stage);
        }

        void setConstantBuffer(RenderPipeline *renderPipeline, ResourceHandle resourceHandle, UINT32 stage)
        {
            renderPipeline->getPipeline()->setConstantBuffer(reinterpret_cast<VideoBuffer *>(resourceManager.getResource(resourceHandle)), stage);
        }

        void setProgram(RenderPipeline *renderPipeline, ProgramHandle programHandle)
        {
            renderPipeline->getPipeline()->setProgram(programManager.getResource(programHandle));
        }

        void setVertexBuffer(RenderContext *renderContext, UINT32 slot, ResourceHandle resourceHandle, UINT32 offset)
        {
            renderContext->getContext()->setVertexBuffer(slot, reinterpret_cast<VideoBuffer *>(resourceManager.getResource(resourceHandle)), offset);
        }

        void setIndexBuffer(RenderContext *renderContext, ResourceHandle resourceHandle, UINT32 offset)
        {
            renderContext->getContext()->setIndexBuffer(reinterpret_cast<VideoBuffer *>(resourceManager.getResource(resourceHandle)), offset);
        }

        void clearRenderTarget(RenderContext *renderContext, ResourceHandle resourceHandle, const Math::Color &color)
        {
            renderContext->getContext()->clearRenderTarget(reinterpret_cast<VideoTarget *>(resourceManager.getResource(resourceHandle, true)), color);
        }

        void clearDepthStencilTarget(RenderContext *renderContext, ResourceHandle depthBuffer, DWORD flags, float depthClear, UINT32 stencilClear)
        {
            renderContext->getContext()->clearDepthStencilTarget(resourceManager.getResource(depthBuffer), flags, depthClear, stencilClear);
        }

        Video::ViewPort viewPortList[8];
        VideoTarget *renderTargetList[8];
        void setRenderTargets(RenderContext *renderContext, ResourceHandle *renderTargetHandleList, UINT32 renderTargetHandleCount, ResourceHandle *depthBuffer)
        {
            for (UINT32 renderTarget = 0; renderTarget < renderTargetHandleCount; renderTarget++)
            {
                renderTargetList[renderTarget] = reinterpret_cast<VideoTarget *>(resourceManager.getResource(renderTargetHandleList[renderTarget], true));
                if (renderTargetList[renderTarget])
                {
                    viewPortList[renderTarget] = renderTargetList[renderTarget]->getViewPort();
                }
            }

            renderContext->getContext()->setRenderTargets(renderTargetList, renderTargetHandleCount, (depthBuffer ? resourceManager.getResource(*depthBuffer) : nullptr));
            renderContext->getContext()->setViewports(viewPortList, renderTargetHandleCount);
        }

        void setBackBuffer(RenderContext *renderContext, ResourceHandle *depthBuffer)
        {
            auto backBuffer = video->getBackBuffer();
            renderTargetList[0] = backBuffer;
            if (renderTargetList[0])
            {
                viewPortList[0] = renderTargetList[0]->getViewPort();
            }

            renderContext->getContext()->setRenderTargets(renderTargetList, 1, (depthBuffer ? resourceManager.getResource(*depthBuffer) : nullptr));
            renderContext->getContext()->setViewports(viewPortList, 1);
        }
    };
    
    GEK_REGISTER_CONTEXT_USER(ResourcesImplementation);
}; // namespace Gek
