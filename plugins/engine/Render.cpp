#include "GEK\Engine\Render.h"
#include "GEK\Utility\Trace.h"
#include "GEK\Utility\String.h"
#include "GEK\Utility\Evaluator.h"
#include "GEK\Utility\FileSystem.h"
#include "GEK\Utility\XML.h"
#include "GEK\Engine\Resources.h"
#include "GEK\Engine\Plugin.h"
#include "GEK\Engine\Shader.h"
#include "GEK\Engine\Material.h"
#include "GEK\Engine\Population.h"
#include "GEK\Engine\Entity.h"
#include "GEK\Engine\Component.h"
#include "GEK\Components\Transform.h"
#include "GEK\Components\Camera.h"
#include "GEK\Components\Light.h"
#include "GEK\Components\Color.h"
#include "GEK\Context\COM.h"
#include "GEK\Context\ContextUserMixin.h"
#include "GEK\Context\ObservableMixin.h"
#include "GEK\Shapes\Sphere.h"
#include <set>
#include <ppl.h>

namespace Gek
{
    static const UINT32 MaxLightCount = 255;

    class RenderPipelineImplementation : public UnknownMixin
        , public RenderPipeline
    {
    private:
        VideoPipeline *videoPipeline;

    public:
        RenderPipelineImplementation(VideoPipeline *videoPipeline)
            : videoPipeline(videoPipeline)
        {
        }

        BEGIN_INTERFACE_LIST(RenderPipelineImplementation)
            INTERFACE_LIST_ENTRY_COM(RenderPipeline)
        END_INTERFACE_LIST_UNKNOWN

        // RenderPipeline
        STDMETHODIMP_(VideoPipeline *) getPipeline(void) { return videoPipeline; };
    };

    class RenderContextImplementation : public UnknownMixin
        , public RenderContext
    {
    private:
        VideoContext *videoContext;
        CComPtr<RenderPipeline> computePipelineHandler;
        CComPtr<RenderPipeline> vertexPipelineHandler;
        CComPtr<RenderPipeline> geometryPipelineHandler;
        CComPtr<RenderPipeline> pixelPipelineHandler;

    public:
        RenderContextImplementation(VideoContext *videoContext)
            : videoContext(videoContext)
            , computePipelineHandler(new RenderPipelineImplementation(videoContext->computePipeline()))
            , vertexPipelineHandler(new RenderPipelineImplementation(videoContext->vertexPipeline()))
            , geometryPipelineHandler(new RenderPipelineImplementation(videoContext->geometryPipeline()))
            , pixelPipelineHandler(new RenderPipelineImplementation(videoContext->pixelPipeline()))
        {
        }

        BEGIN_INTERFACE_LIST(RenderContextImplementation)
            INTERFACE_LIST_ENTRY_COM(RenderContext)
        END_INTERFACE_LIST_UNKNOWN

        // RenderContext
        STDMETHODIMP_(VideoContext *) getContext(void) { return videoContext; };
        STDMETHODIMP_(RenderPipeline *) computePipeline(void) { return computePipelineHandler.p; };
        STDMETHODIMP_(RenderPipeline *) vertexPipeline(void) { return vertexPipelineHandler.p; };
        STDMETHODIMP_(RenderPipeline *) geometryPipeline(void) { return geometryPipelineHandler.p; };
        STDMETHODIMP_(RenderPipeline *) pixelPipeline(void) { return pixelPipelineHandler.p; };
    };

    class RenderImplementation : public ContextUserMixin
        , public ObservableMixin
        , public PopulationObserver
        , public Render
    {
    public:
        __declspec(align(16))
        struct EngineConstantData
        {
            float worldTime;
            float frameTime;
            float buffer[2];
        };

        __declspec(align(16))
        struct CameraConstantData
        {
            Math::Float2 fieldOfView;
            float minimumDistance;
            float maximumDistance;
            Math::Float4x4 viewMatrix;
            Math::Float4x4 projectionMatrix;
            Math::Float4x4 inverseProjectionMatrix;
        };

        __declspec(align(16))
        struct LightConstantData
        {
            UINT32 count;
            UINT32 padding[3];
        };

        struct LightData
        {
            UINT32 type;
            Math::Float3 position;
            Math::Float3 direction;
            float range;
            float radius;
            float innerAngle;
            float outerAngle;
            Math::Float3 color;

            LightData(const PointLightComponent &light, const ColorComponent &color, const Math::Float4x4 &matrix)
                : type(0)
                , position(matrix.translation)
                , direction(matrix.rz)
                , range(light.range)
                , radius(light.radius)
                , color(color.value)
            {
            }

            LightData(const SpotLightComponent &light, const ColorComponent &color, const Math::Float4x4 &matrix)
                : type(1)
                , position(matrix.translation)
                , direction(matrix.rz)
                , range(light.range)
                , radius(light.radius)
                , innerAngle(light.innerAngle)
                , outerAngle(light.outerAngle)
                , color(color.value)
            {
            }

            LightData(const DirectionalLightComponent &light, const ColorComponent &color, const Math::Float4x4 &matrix)
                : type(2)
                , direction(matrix.rz)
                , color(color.value)
            {
            }
        };

        typedef std::function<void(RenderContext *renderContext)> DrawCall;
        struct DrawCallValue
        {
            union
            {
                UINT32 value;
                struct
                {
                    ShaderHandle shader;
                    PluginHandle plugin;
                    PropertiesHandle properties;
                };
            };

            DrawCall onDraw;

            DrawCallValue(const DrawCallValue &drawCallValue)
                : value(drawCallValue.value)
                , onDraw(drawCallValue.onDraw)
            {
            }

            DrawCallValue(PluginHandle plugin, MaterialHandle material, DrawCall onDraw)
                : plugin(plugin)
                , shader(material.shader)
                , properties(material.properties)
                , onDraw(onDraw)
            {
            }

            void operator = (const DrawCallValue &drawCallValue)
            {
                value = drawCallValue.value;
                onDraw = drawCallValue.onDraw;
            }
        };

    private:
        IUnknown *initializerContext;
        VideoSystem *video;
        Resources *resources;
        Population *population;
        UINT32 updateHandle;

        CComPtr<IUnknown> pointSamplerStates;
        CComPtr<IUnknown> linearClampSamplerStates;
        CComPtr<IUnknown> linearWrapSamplerStates;
        CComPtr<VideoBuffer> engineConstantBuffer;
        CComPtr<VideoBuffer> cameraConstantBuffer;
        CComPtr<VideoBuffer> lightConstantBuffer;
        CComPtr<VideoBuffer> lightDataBuffer;

        CComPtr<IUnknown> deferredVertexProgram;

        std::vector<LightData> lightList;
        std::vector<DrawCallValue> drawCallList;

    public:
        RenderImplementation(void)
            : initializerContext(nullptr)
            , video(nullptr)
            , resources(nullptr)
            , population(nullptr)
            , updateHandle(0)
        {
        }

        ~RenderImplementation(void)
        {
            population->removeUpdatePriority(updateHandle);
            ObservableMixin::removeObserver(population, getClass<PopulationObserver>());
        }

        BEGIN_INTERFACE_LIST(RenderImplementation)
            INTERFACE_LIST_ENTRY_COM(Observable)
            INTERFACE_LIST_ENTRY_COM(PopulationObserver)
            INTERFACE_LIST_ENTRY_COM(Render)
        END_INTERFACE_LIST_USER

        // Render/Resources
        STDMETHODIMP initialize(IUnknown *initializerContext)
        {
            GEK_TRACE_FUNCTION(Render);

            GEK_REQUIRE_RETURN(initializerContext, E_INVALIDARG);

            HRESULT resultValue = E_FAIL;
            CComQIPtr<VideoSystem> video(initializerContext);
            CComQIPtr<Resources> resources(initializerContext);
            CComQIPtr<Population> population(initializerContext);
            if (video && resources && population)
            {
                this->video = video;
                this->resources = resources;
                this->population = population;
                this->initializerContext = initializerContext;
                resultValue = ObservableMixin::addObserver(population, getClass<PopulationObserver>());
                updateHandle = population->setUpdatePriority(this, 100);
            }

            if (SUCCEEDED(resultValue))
            {
                Video::SamplerStates samplerStates;
                samplerStates.filterMode = Video::FilterMode::AllPoint;
                samplerStates.addressModeU = Video::AddressMode::Clamp;
                samplerStates.addressModeV = Video::AddressMode::Clamp;
                resultValue = video->createSamplerStates(&pointSamplerStates, samplerStates);
            }

            if (SUCCEEDED(resultValue))
            {
                Video::SamplerStates samplerStates;
                samplerStates.maximumAnisotropy = 8;
                samplerStates.filterMode = Video::FilterMode::Anisotropic;
                samplerStates.addressModeU = Video::AddressMode::Clamp;
                samplerStates.addressModeV = Video::AddressMode::Clamp;
                resultValue = video->createSamplerStates(&linearClampSamplerStates, samplerStates);
            }

            if (SUCCEEDED(resultValue))
            {
                Video::SamplerStates samplerStates;
                samplerStates.maximumAnisotropy = 8;
                samplerStates.filterMode = Video::FilterMode::Anisotropic;
                samplerStates.addressModeU = Video::AddressMode::Wrap;
                samplerStates.addressModeV = Video::AddressMode::Wrap;
                resultValue = video->createSamplerStates(&linearWrapSamplerStates, samplerStates);
            }

            if (SUCCEEDED(resultValue))
            {
                resultValue = video->createBuffer(&engineConstantBuffer, sizeof(EngineConstantData), 1, Video::BufferType::Constant, 0);
            }

            if (SUCCEEDED(resultValue))
            {
                resultValue = video->createBuffer(&cameraConstantBuffer, sizeof(CameraConstantData), 1, Video::BufferType::Constant, 0);
            }

            if (SUCCEEDED(resultValue))
            {
                resultValue = video->createBuffer(&lightConstantBuffer, sizeof(LightConstantData), 1, Video::BufferType::Constant, Video::BufferFlags::Mappable);
            }

            if (SUCCEEDED(resultValue))
            {
                resultValue = video->createBuffer(&lightDataBuffer, sizeof(LightData), MaxLightCount, Video::BufferType::Structured, Video::BufferFlags::Mappable | Video::BufferFlags::Resource);
            }

            if (SUCCEEDED(resultValue))
            {
                static const char program[] =
                    "struct Pixel                                                                       \r\n" \
                    "{                                                                                  \r\n" \
                    "    float4 position : SV_POSITION;                                                 \r\n" \
                    "    float2 texCoord : TEXCOORD0;                                                   \r\n" \
                    "};                                                                                 \r\n" \
                    "                                                                                   \r\n" \
                    "Pixel mainVertexProgram(in uint vertexID : SV_VertexID)                            \r\n" \
                    "{                                                                                  \r\n" \
                    "    Pixel pixel;                                                                   \r\n" \
                    "    pixel.texCoord = float2((vertexID << 1) & 2, vertexID & 2);                    \r\n" \
                    "    pixel.position = float4(pixel.texCoord * float2(2.0f, -2.0f) + float2(-1.0f, 1.0f), 0.0f, 1.0f); \r\n" \
                    "    return pixel;                                                                  \r\n" \
                    "}                                                                                  \r\n" \
                    "                                                                                   \r\n";

                resultValue = video->compileVertexProgram(&deferredVertexProgram, program, "mainVertexProgram", nullptr);
            }

            return resultValue;
        }

        // Render
        STDMETHODIMP_(void) queueDrawCall(PluginHandle plugin, MaterialHandle material, std::function<void(RenderContext *renderContext)> draw)
        {
            if (plugin && material && draw)
            {
                drawCallList.emplace_back(plugin, material, draw);
            }
        }

        // PopulationObserver
        STDMETHODIMP_(void) onLoadBegin(void)
        {
        }

        STDMETHODIMP_(void) onLoadEnd(HRESULT resultValue)
        {
            if (FAILED(resultValue))
            {
                onFree();
            }
        }

        STDMETHODIMP_(void) onFree(void)
        {
            GEK_REQUIRE_VOID_RETURN(resources);

            resources->clearLocal();
        }

        STDMETHODIMP_(void) render(Entity *cameraEntity, const Math::Float4x4 &projectionMatrix)
        {
            GEK_TRACE_FUNCTION(Render);

            GEK_REQUIRE_VOID_RETURN(population);
            GEK_REQUIRE_VOID_RETURN(cameraEntity);

            auto &cameraTransform = cameraEntity->getComponent<TransformComponent>();
            Math::Float4x4 cameraMatrix(cameraTransform.getMatrix());

            auto &cameraData = cameraEntity->getComponent<CameraComponent>();

            EngineConstantData engineConstantData;
            engineConstantData.frameTime = population->getFrameTime();
            engineConstantData.worldTime = population->getWorldTime();

            CameraConstantData cameraConstantData;
            float displayAspectRatio = (float(video->getWidth()) / float(video->getHeight()));
            float fieldOfView = Math::convertDegreesToRadians(cameraData.fieldOfView);
            cameraConstantData.fieldOfView.x = tan(fieldOfView * 0.5f);
            cameraConstantData.fieldOfView.y = (cameraConstantData.fieldOfView.x / displayAspectRatio);
            cameraConstantData.minimumDistance = cameraData.minimumDistance;
            cameraConstantData.maximumDistance = cameraData.maximumDistance;
            cameraConstantData.viewMatrix = cameraMatrix.getInverse();
            cameraConstantData.projectionMatrix = projectionMatrix;
            cameraConstantData.inverseProjectionMatrix = projectionMatrix.getInverse();

            const Shapes::Frustum viewFrustum(cameraConstantData.viewMatrix * cameraConstantData.projectionMatrix);

            drawCallList.clear();
            ObservableMixin::sendEvent(Event<RenderObserver>(std::bind(&RenderObserver::onRenderScene, std::placeholders::_1, cameraEntity, &viewFrustum)));
            if (!drawCallList.empty())
            {
                VideoContext *videoContext = video->getDefaultContext();
                CComPtr<RenderContext> renderContext(new RenderContextImplementation(videoContext));

                video->updateBuffer(engineConstantBuffer, &engineConstantData);
                videoContext->geometryPipeline()->setConstantBuffer(engineConstantBuffer, 0);
                videoContext->vertexPipeline()->setConstantBuffer(engineConstantBuffer, 0);
                videoContext->pixelPipeline()->setConstantBuffer(engineConstantBuffer, 0);
                videoContext->computePipeline()->setConstantBuffer(engineConstantBuffer, 0);

                video->updateBuffer(cameraConstantBuffer, &cameraConstantData);
                videoContext->geometryPipeline()->setConstantBuffer(cameraConstantBuffer, 1);
                videoContext->vertexPipeline()->setConstantBuffer(cameraConstantBuffer, 1);
                videoContext->pixelPipeline()->setConstantBuffer(cameraConstantBuffer, 1);
                videoContext->computePipeline()->setConstantBuffer(cameraConstantBuffer, 1);

                concurrency::parallel_sort(drawCallList.begin(), drawCallList.end(), [](const DrawCallValue &leftValue, const DrawCallValue &rightValue) -> bool
                {
                    return (leftValue.value < rightValue.value);
                });

                lightList.clear();
                population->listEntities<TransformComponent, PointLightComponent, ColorComponent>([&](Entity *lightEntity) -> void
                {
                    auto &lightTransformComponent = lightEntity->getComponent<TransformComponent>();
                    auto &lightComponent = lightEntity->getComponent<PointLightComponent>();
                    if (viewFrustum.isVisible(Shapes::Sphere(lightTransformComponent.position, lightComponent.range)))
                    {
                        auto &lightColorComponent = lightEntity->getComponent<ColorComponent>();
                        Math::Float4x4 lightMatrix(cameraConstantData.viewMatrix * lightTransformComponent.getMatrix());
                        lightList.emplace_back(lightComponent, lightColorComponent, lightMatrix);
                    }
                });

                population->listEntities<TransformComponent, SpotLightComponent, ColorComponent>([&](Entity *lightEntity) -> void
                {
                    auto &lightTransformComponent = lightEntity->getComponent<TransformComponent>();
                    auto &lightComponent = lightEntity->getComponent<SpotLightComponent>();
                    if (viewFrustum.isVisible(Shapes::Sphere(lightTransformComponent.position, lightComponent.range)))
                    {
                        auto &lightColorComponent = lightEntity->getComponent<ColorComponent>();
                        Math::Float4x4 lightMatrix(cameraConstantData.viewMatrix * lightTransformComponent.getMatrix());
                        lightList.emplace_back(lightComponent, lightColorComponent, lightMatrix);
                    }
                });

                population->listEntities<TransformComponent, DirectionalLightComponent, ColorComponent>([&](Entity *lightEntity) -> void
                {
                    auto &lightTransformComponent = lightEntity->getComponent<TransformComponent>();
                    auto &lightComponent = lightEntity->getComponent<DirectionalLightComponent>();
                    auto &lightColorComponent = lightEntity->getComponent<ColorComponent>();
                    Math::Float4x4 lightMatrix(cameraConstantData.viewMatrix * lightTransformComponent.getMatrix());
                    lightList.emplace_back(lightComponent, lightColorComponent, lightMatrix);
                });

                UINT32 lightListCount = lightList.size();
                videoContext->pixelPipeline()->setSamplerStates(pointSamplerStates, 0);
                videoContext->pixelPipeline()->setSamplerStates(linearClampSamplerStates, 1);
                videoContext->pixelPipeline()->setSamplerStates(linearWrapSamplerStates, 2);
                videoContext->setPrimitiveType(Video::PrimitiveType::TriangleList);

                ShaderHandle currentShader;
                PluginHandle currentPlugin;
                PropertiesHandle currentProperties;
                UINT32 drawCallCount = drawCallList.size();
                for (UINT32 drawCallIndex = 0; drawCallIndex < drawCallCount; )
                {
                    DrawCallValue &drawCall = drawCallList[drawCallIndex];

                    currentShader = drawCall.shader;
                    Shader *shader = resources->getResource<Shader, ShaderHandle>(currentShader);
                    if (!shader)
                    {
                        drawCallIndex++;
                        continue;
                    }

                    auto drawLights = [&](std::function<void(void)> drawPasses) -> void
                    {
                        for (UINT32 lightBase = 0; lightBase < lightListCount; lightBase += MaxLightCount)
                        {
                            UINT32 lightCount = std::min((lightListCount - lightBase), MaxLightCount);

                            LightConstantData *lightConstants = nullptr;
                            if (SUCCEEDED(video->mapBuffer(lightConstantBuffer, (LPVOID *)&lightConstants)))
                            {
                                lightConstants->count = lightCount;
                                video->unmapBuffer(lightConstantBuffer);
                            }

                            LightData *lightingData = nullptr;
                            if (SUCCEEDED(video->mapBuffer(lightDataBuffer, (LPVOID *)&lightingData)))
                            {
                                memcpy(lightingData, &lightList[lightBase], (sizeof(LightData) * lightCount));
                                video->unmapBuffer(lightDataBuffer);
                            }

                            drawPasses();
                        }
                    };

                    auto enableLights = [&](RenderPipeline *renderPipeline) -> void
                    {
                        renderPipeline->getPipeline()->setResource(lightDataBuffer, 0);
                        renderPipeline->getPipeline()->setConstantBuffer(lightConstantBuffer, 3);
                    };

                    auto drawForward = [&](void) -> void
                    {
                        while (drawCallIndex < drawCallCount)
                        {
                            drawCall = drawCallList[drawCallIndex++];
                            if (currentPlugin != drawCall.plugin)
                            {
                                currentPlugin = drawCall.plugin;
                                Plugin *plugin = resources->getResource<Plugin, PluginHandle>(currentPlugin);
                                if (!plugin)
                                {
                                    continue;
                                }

                                plugin->enable(videoContext);
                            }

                            if (currentProperties != drawCall.properties)
                            {
                                currentProperties = drawCall.properties;
                                Material *material = resources->getResource<Material, PropertiesHandle>(currentProperties);
                                if (!material)
                                {
                                    continue;
                                }

                                shader->setResourceList(renderContext, material->getResourceList());
                            }

                            drawCall.onDraw(renderContext);
                        };
                    };

                    auto drawDeferred = [&](void) -> void
                    {
                        videoContext->vertexPipeline()->setProgram(deferredVertexProgram);
                        videoContext->drawPrimitive(3, 0);
                    };

                    auto runCompute = [&](UINT32 dispatchWidth, UINT32 dispatchHeight, UINT32 dispatchDepth) -> void
                    {
                        videoContext->dispatch(dispatchWidth, dispatchHeight, dispatchDepth);
                    };

                    shader->draw(renderContext, drawLights, enableLights, drawForward, drawDeferred, runCompute);
                }
            }
        }

        void render(void)
        {
            GEK_TRACE_FUNCTION(Render);

            GEK_REQUIRE_VOID_RETURN(population);
            GEK_REQUIRE_VOID_RETURN(resources);
            GEK_REQUIRE_VOID_RETURN(video);

            population->listEntities<TransformComponent, FirstPersonCameraComponent>([&](Entity *cameraEntity) -> void
            {
                auto &cameraData = cameraEntity->getComponent<FirstPersonCameraComponent>();

                float displayAspectRatio = (1280.0f / 800.0f);
                float fieldOfView = Math::convertDegreesToRadians(cameraData.fieldOfView);

                Math::Float4x4 projectionMatrix;
                projectionMatrix.setPerspective(fieldOfView, displayAspectRatio, cameraData.minimumDistance, cameraData.maximumDistance);

                render(cameraEntity, projectionMatrix);
            });

            ObservableMixin::sendEvent(Event<RenderObserver>(std::bind(&RenderObserver::onRenderOverlay, std::placeholders::_1)));

            video->present(true);
        }

        STDMETHODIMP_(void) onUpdate(void)
        {
            render();
        }

        STDMETHODIMP_(void) onIdle(void)
        {
            render();
        }
    };

    REGISTER_CLASS(RenderImplementation)
}; // namespace Gek
