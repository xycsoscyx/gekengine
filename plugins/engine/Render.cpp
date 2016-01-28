#include "GEK\Engine\Render.h"
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
#include "GEK\Context\Common.h"
#include "GEK\Context\ContextUserMixin.h"
#include "GEK\Context\ObservableMixin.h"
#include "GEK\Utility\Evaluator.h"
#include "GEK\Utility\FileSystem.h"
#include "GEK\Utility\String.h"
#include "GEK\Utility\XML.h"
#include "GEK\Shape\Sphere.h"
#include <set>
#include <ppl.h>

namespace Gek
{
    static const UINT32 MaxLightCount = 255;

    class RenderImplementation : public ContextUserMixin
        , public ObservableMixin
        , public PopulationObserver
        , public Render
    {
    public:
        struct CameraConstantData
        {
            Math::Float2 fieldOfView;
            float minimumDistance;
            float maximumDistance;
            Math::Float4x4 viewMatrix;
            Math::Float4x4 projectionMatrix;
            Math::Float4x4 inverseProjectionMatrix;
        };

        struct LightConstants
        {
            UINT32 count;
            UINT32 padding[3];
        };

        struct LightData
        {
            Math::Float3 position;
            float range;
            float radius;
            Math::Float3 color;
            LightData(const Math::Float3 &position, float range, float radius, const Math::Float3 &color)
                : position(position)
                , range(range)
                , radius(radius)
                , color(color)
            {
            }
        };

        typedef std::function<void(VideoContext *)> DrawCall;
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

            DrawCallValue(PluginHandle plugin, MaterialHandle material, DrawCall onDraw)
                : plugin(plugin)
                , shader(material.identifier.shader)
                , properties(material.identifier.properties)
                , onDraw(onDraw)
            {
            }
        };

    private:
        IUnknown *initializerContext;
        VideoSystem *video;
        Resources *resources;
        Population *population;
        UINT32 updateHandle;

        CComPtr<IUnknown> pointSamplerStates;
        CComPtr<IUnknown> linearSamplerStates;
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
            REQUIRE_RETURN(initializerContext, E_INVALIDARG);

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
                samplerStates.addressModeU = Video::AddressMode::Wrap;
                samplerStates.addressModeV = Video::AddressMode::Wrap;
                resultValue = video->createSamplerStates(&linearSamplerStates, samplerStates);
            }

            if (SUCCEEDED(resultValue))
            {
                resultValue = video->createBuffer(&cameraConstantBuffer, sizeof(CameraConstantData), 1, Video::BufferType::Constant, 0);
            }

            if (SUCCEEDED(resultValue))
            {
                resultValue = video->createBuffer(&lightConstantBuffer, sizeof(LightConstants), 1, Video::BufferType::Constant, Video::BufferFlags::Mappable);
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
        STDMETHODIMP_(void) queueDrawCall(PluginHandle plugin, MaterialHandle material, std::function<void(VideoContext *)> draw)
        {
            REQUIRE_VOID_RETURN(resources);

            if (plugin.isValid() && material.isValid() && draw)
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
            REQUIRE_VOID_RETURN(resources);

            resources->clearLocal();
        }

        STDMETHODIMP_(void) onUpdate(float frameTime)
        {
            REQUIRE_VOID_RETURN(population);
            REQUIRE_VOID_RETURN(resources);
            REQUIRE_VOID_RETURN(video);

            population->listEntities<TransformComponent, CameraComponent>([&](Entity *cameraEntity) -> void
            {
                auto &cameraTransform = cameraEntity->getComponent<TransformComponent>();
                Math::Float4x4 cameraMatrix(cameraTransform.getMatrix());

                auto &cameraData = cameraEntity->getComponent<CameraComponent>();

                CameraConstantData cameraConstantData;
                float displayAspectRatio = (1280.0f / 800.0f);
                float fieldOfView = Math::convertDegreesToRadians(cameraData.fieldOfView);
                cameraConstantData.fieldOfView.x = tan(fieldOfView * 0.5f);
                cameraConstantData.fieldOfView.y = (cameraConstantData.fieldOfView.x / displayAspectRatio);
                cameraConstantData.minimumDistance = cameraData.minimumDistance;
                cameraConstantData.maximumDistance = cameraData.maximumDistance;
                cameraConstantData.viewMatrix = cameraMatrix.getInverse();
                cameraConstantData.projectionMatrix.setPerspective(fieldOfView, displayAspectRatio, cameraData.minimumDistance, cameraData.maximumDistance);
                cameraConstantData.inverseProjectionMatrix = cameraConstantData.projectionMatrix.getInverse();
                video->updateBuffer(cameraConstantBuffer, &cameraConstantData);

                VideoContext *videoContext = video->getDefaultContext();

                videoContext->geometryPipeline()->setConstantBuffer(cameraConstantBuffer, 0);
                videoContext->vertexPipeline()->setConstantBuffer(cameraConstantBuffer, 0);
                videoContext->pixelPipeline()->setConstantBuffer(cameraConstantBuffer, 0);
                videoContext->computePipeline()->setConstantBuffer(cameraConstantBuffer, 0);

                const Shape::Frustum viewFrustum(cameraConstantData.viewMatrix * cameraConstantData.projectionMatrix);

                lightList.clear();
                population->listEntities<TransformComponent, LightComponent, ColorComponent>([&](Entity *lightEntity) -> void
                {
                    auto &lightTransformComponent = lightEntity->getComponent<TransformComponent>();
                    auto &lightComponent = lightEntity->getComponent<LightComponent>();
                    if (viewFrustum.isVisible(Shape::Sphere(lightTransformComponent.position, lightComponent.range)))
                    {
                        auto &lightColorComponent = lightEntity->getComponent<ColorComponent>();
                        lightList.emplace_back((cameraConstantData.viewMatrix * lightTransformComponent.position.w(1.0f)).xyz, lightComponent.range, lightComponent.radius, lightColorComponent.value.xyz);
                    }
                });

                drawCallList.clear();
                ObservableMixin::sendEvent(Event<RenderObserver>(std::bind(&RenderObserver::onRenderScene, std::placeholders::_1, cameraEntity, &viewFrustum)));
                concurrency::parallel_sort(drawCallList.begin(), drawCallList.end(), [](const DrawCallValue &leftValue, const DrawCallValue &rightValue) -> bool
                {
                    return (leftValue.value < rightValue.value);
                });

                UINT32 lightListCount = lightList.size();
                videoContext->pixelPipeline()->setSamplerStates(pointSamplerStates, 0);
                videoContext->pixelPipeline()->setSamplerStates(linearSamplerStates, 1);
                videoContext->setPrimitiveType(Video::PrimitiveType::TriangleList);

                ShaderHandle currentShader;
                PluginHandle currentPlugin;
                PropertiesHandle currentProperties;
                UINT32 drawCallCount = drawCallList.size();
                for (UINT32 drawCallIndex = 0; drawCallIndex < drawCallCount; )
                {
                    DrawCallValue &drawCall = drawCallList[drawCallIndex];
                    Shader *shader = resources->getResource<Shader, ShaderHandle>(currentShader = drawCall.shader);
                    auto drawLights = [&](std::function<void(void)> drawPasses) -> void
                    {
                        for (UINT32 lightBase = 0; lightBase < lightListCount; lightBase += MaxLightCount)
                        {
                            UINT32 lightCount = std::min((lightListCount - lightBase), MaxLightCount);

                            LightConstants *lightConstants = nullptr;
                            if (SUCCEEDED(video->mapBuffer(lightConstantBuffer, (LPVOID *)&lightConstants)))
                            {
                                memcpy(lightConstants, &lightCount, sizeof(UINT32));
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

                    auto enableLights = [&](VideoPipeline *videoPipeline) -> void
                    {
                        videoPipeline->setResource(lightDataBuffer, 0);
                        videoPipeline->setConstantBuffer(lightConstantBuffer, 1);
                    };

                    auto drawForward = [&](void) -> void
                    {
                        while (drawCallIndex < drawCallCount)
                        {
                            drawCall = drawCallList[drawCallIndex++];
                            if (currentPlugin != drawCall.plugin)
                            {
                                Plugin *plugin = resources->getResource<Plugin, PluginHandle>(currentPlugin = drawCall.plugin);
                                if (plugin)
                                {
                                    plugin->enable(videoContext);
                                }
                            }

                            if (currentProperties != drawCall.properties)
                            {
                                Material *material = resources->getResource<Material, PropertiesHandle>(currentProperties = drawCall.properties);
                                if (material)
                                {
                                    shader->setResourceList(videoContext, material->getResourceList());
                                }
                            }

                            drawCall.onDraw(videoContext);
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

                    shader->draw(videoContext, drawLights, enableLights, drawForward, drawDeferred, runCompute);
                }
            });

            ObservableMixin::sendEvent(Event<RenderObserver>(std::bind(&RenderObserver::onRenderOverlay, std::placeholders::_1)));

            video->present(true);
        }
    };

    REGISTER_CLASS(RenderImplementation)
}; // namespace Gek
