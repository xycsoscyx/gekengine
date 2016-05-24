#include "GEK\Engine\Render.h"
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
#include "GEK\Context\Plugin.h"
#include "GEK\Context\ObservableMixin.h"
#include "GEK\Shapes\Sphere.h"
#include <set>
#include <ppl.h>
#include <concurrent_vector.h>

namespace Gek
{
    class RenderPipelineImplementation
        : public UnknownMixin
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

    class RenderContextImplementation
        : public UnknownMixin
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

    class RenderImplementation
        : public ContextUserMixin
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

        struct DrawCallValue
        {
            union
            {
                UINT32 value;
                struct
                {
                    MaterialHandle material;
                    PluginHandle plugin;
                    ShaderHandle shader;
                };
            };

            std::function<void(RenderContext *renderContext)> onDraw;

            DrawCallValue(const DrawCallValue &drawCallValue)
                : value(drawCallValue.value)
                , onDraw(drawCallValue.onDraw)
            {
            }

            DrawCallValue(DrawCallValue &&drawCallValue)
                : value(drawCallValue.value)
                , onDraw(std::move(drawCallValue.onDraw))
            {
            }

            DrawCallValue(MaterialHandle material, PluginHandle plugin, ShaderHandle shader, std::function<void(RenderContext *renderContext)> onDraw)
                : material(material)
                , plugin(plugin)
                , shader(shader)
                , onDraw(onDraw)
            {
            }

            void operator = (const DrawCallValue &drawCallValue)
            {
                value = drawCallValue.value;
                onDraw = drawCallValue.onDraw;
            }
        };

        typedef concurrency::concurrent_vector<DrawCallValue> DrawCallList;

        struct DrawCallSet
        {
            Shader *shader;
            DrawCallList::iterator begin;
            DrawCallList::iterator end;

            DrawCallSet(Shader *shader, DrawCallList::iterator begin, DrawCallList::iterator end)
                : shader(shader)
                , begin(begin)
                , end(end)
            {
            }

            DrawCallSet(const DrawCallSet &drawCallSet)
                : shader(drawCallSet.shader)
                , begin(drawCallSet.begin)
                , end(drawCallSet.end)
            {
            }

            DrawCallSet(DrawCallSet &&drawCallSet)
                : shader(std::move(drawCallSet.shader))
                , begin(std::move(drawCallSet.begin))
                , end(std::move(drawCallSet.end))
            {
            }
        };

    private:
        IUnknown *initializerContext;
        VideoSystem *video;
        Resources *resources;
        Population *population;
        UINT32 backgroundUpdateHandle;
        UINT32 foregroundUpdateHandle;

        CComPtr<IUnknown> pointSamplerState;
        CComPtr<IUnknown> linearClampSamplerState;
        CComPtr<IUnknown> linearWrapSamplerState;
        CComPtr<VideoBuffer> engineConstantBuffer;
        CComPtr<VideoBuffer> cameraConstantBuffer;

        CComPtr<IUnknown> deferredVertexProgram;

        DrawCallList drawCallList;

    public:
        RenderImplementation(void)
            : initializerContext(nullptr)
            , video(nullptr)
            , resources(nullptr)
            , population(nullptr)
            , backgroundUpdateHandle(0)
            , foregroundUpdateHandle(0)
        {
        }

        ~RenderImplementation(void)
        {
            if (population)
            {
                population->removeUpdatePriority(foregroundUpdateHandle);
                population->removeUpdatePriority(backgroundUpdateHandle);
            }

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
            GEK_REQUIRE(initializerContext);

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
                backgroundUpdateHandle = population->setUpdatePriority(this, 10);
                foregroundUpdateHandle = population->setUpdatePriority(this, 100);
            }

            if (SUCCEEDED(resultValue))
            {
                Video::SamplerState samplerState;
                samplerState.filterMode = Video::FilterMode::AllPoint;
                samplerState.addressModeU = Video::AddressMode::Clamp;
                samplerState.addressModeV = Video::AddressMode::Clamp;
                resultValue = video->createSamplerState(&pointSamplerState, samplerState);
            }

            if (SUCCEEDED(resultValue))
            {
                Video::SamplerState samplerState;
                samplerState.maximumAnisotropy = 8;
                samplerState.filterMode = Video::FilterMode::Anisotropic;
                samplerState.addressModeU = Video::AddressMode::Clamp;
                samplerState.addressModeV = Video::AddressMode::Clamp;
                resultValue = video->createSamplerState(&linearClampSamplerState, samplerState);
            }

            if (SUCCEEDED(resultValue))
            {
                Video::SamplerState samplerState;
                samplerState.maximumAnisotropy = 8;
                samplerState.filterMode = Video::FilterMode::Anisotropic;
                samplerState.addressModeU = Video::AddressMode::Wrap;
                samplerState.addressModeV = Video::AddressMode::Wrap;
                resultValue = video->createSamplerState(&linearWrapSamplerState, samplerState);
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
                    "    pixel.position = float4(pixel.texCoord * float2(2.0f, -2.0f)                   \r\n" \
                    "                                           + float2(-1.0f, 1.0f), 0.0f, 1.0f);     \r\n" \
                    "    return pixel;                                                                  \r\n" \
                    "}                                                                                  \r\n" \
                    "                                                                                   \r\n";

                resultValue = video->compileVertexProgram(&deferredVertexProgram, program, "mainVertexProgram");
            }

            return resultValue;
        }

        // Render
        STDMETHODIMP_(void) queueDrawCall(PluginHandle plugin, MaterialHandle material, std::function<void(RenderContext *renderContext)> draw)
        {
            if (plugin && material && draw)
            {
                ShaderHandle shader = resources->getShader(material);
                if (shader)
                {
                    drawCallList.push_back(DrawCallValue(material, plugin, shader, draw));
                }
            }
        }

        STDMETHODIMP_(void) render(Entity *cameraEntity, const Math::Float4x4 &projectionMatrix, float minimumDistance, float maximumDistance)
        {
            GEK_REQUIRE(population);
            GEK_REQUIRE(cameraEntity);

            auto &cameraTransform = cameraEntity->getComponent<TransformComponent>();
            Math::Float4x4 cameraMatrix(cameraTransform.getMatrix());

            EngineConstantData engineConstantData;
            engineConstantData.frameTime = population->getFrameTime();
            engineConstantData.worldTime = population->getWorldTime();

            CameraConstantData cameraConstantData;
            cameraConstantData.fieldOfView.x = (1.0f / projectionMatrix._11);
            cameraConstantData.fieldOfView.y = (1.0f / projectionMatrix._22);
            cameraConstantData.minimumDistance = minimumDistance;
            cameraConstantData.maximumDistance = maximumDistance;
            cameraConstantData.viewMatrix = cameraMatrix.getInverse();
            cameraConstantData.projectionMatrix = projectionMatrix;
            cameraConstantData.inverseProjectionMatrix = projectionMatrix.getInverse();

            const Shapes::Frustum viewFrustum(cameraConstantData.viewMatrix * cameraConstantData.projectionMatrix);

            drawCallList.clear();
            ObservableMixin::sendEvent(Event<RenderObserver>(std::bind(&RenderObserver::onRenderScene, std::placeholders::_1, cameraEntity, &cameraConstantData.viewMatrix, &viewFrustum)));
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

                videoContext->pixelPipeline()->setSamplerState(pointSamplerState, 0);
                videoContext->pixelPipeline()->setSamplerState(linearClampSamplerState, 1);
                videoContext->pixelPipeline()->setSamplerState(linearWrapSamplerState, 2);
                videoContext->vertexPipeline()->setSamplerState(pointSamplerState, 0);
                videoContext->vertexPipeline()->setSamplerState(linearClampSamplerState, 1);
                videoContext->vertexPipeline()->setSamplerState(linearWrapSamplerState, 2);
                videoContext->setPrimitiveType(Video::PrimitiveType::TriangleList);

                concurrency::parallel_sort(drawCallList.begin(), drawCallList.end(), [](const DrawCallValue &leftValue, const DrawCallValue &rightValue) -> bool
                {
                    return (leftValue.value < rightValue.value);
                });

                ShaderHandle currentShader;
                std::list<DrawCallSet> drawCallSetList;
                for (auto &drawCall = drawCallList.begin(); drawCall != drawCallList.end(); )
                {
                    currentShader = (*drawCall).shader;

                    auto beginShaderList = drawCall;
                    while (drawCall != drawCallList.end() && (*drawCall).shader == currentShader)
                    {
                        ++drawCall;
                    };

                    auto endShaderList = drawCall;
                    Shader *shader = resources->getResource<Shader, ShaderHandle>(currentShader);
                    if (!shader)
                    {
                        continue;
                    }

                    drawCallSetList.push_back(DrawCallSet(shader, beginShaderList, endShaderList));
                }

                drawCallSetList.sort([](const DrawCallSet &leftValue, const DrawCallSet &rightValue) -> bool
                {
                    return (leftValue.shader->getPriority() < rightValue.shader->getPriority());
                });

                for(auto &drawCallSet : drawCallSetList)
                {
                    auto &shader = drawCallSet.shader;
                    for (auto block = shader->begin(renderContext, cameraConstantData.viewMatrix, viewFrustum); block; block = block->next())
                    {
                        while (block->prepare())
                        {
                            for (auto pass = block->begin(); pass; pass = pass->next())
                            {
                                switch (pass->prepare())
                                {
                                case Shader::Pass::Mode::Forward:
                                    if (true)
                                    {
                                        PluginHandle currentPlugin;
                                        MaterialHandle currentMaterial;
                                        for (auto &shaderDrawCall = drawCallSet.begin; shaderDrawCall != drawCallSet.end; ++shaderDrawCall)
                                        {
                                            if (currentPlugin != (*shaderDrawCall).plugin)
                                            {
                                                currentPlugin = (*shaderDrawCall).plugin;
                                                Plugin *plugin = resources->getResource<Plugin, PluginHandle>(currentPlugin);
                                                if (!plugin)
                                                {
                                                    continue;
                                                }

                                                plugin->enable(videoContext);
                                            }

                                            if (currentMaterial != (*shaderDrawCall).material)
                                            {
                                                currentMaterial = (*shaderDrawCall).material;
                                                Material *material = resources->getResource<Material, MaterialHandle>(currentMaterial);
                                                if (!material)
                                                {
                                                    continue;
                                                }

                                                shader->setResourceList(renderContext, block.get(), pass.get(), material->getResourceList());
                                            }

                                            (*shaderDrawCall).onDraw(renderContext);
                                        }
                                    }

                                    break;

                                case Shader::Pass::Mode::Deferred:
                                    videoContext->vertexPipeline()->setProgram(deferredVertexProgram);
                                    videoContext->drawPrimitive(3, 0);
                                    break;

                                case Shader::Pass::Mode::Compute:
                                    break;
                                };
                            }
                        };
                    }
                }
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
            GEK_REQUIRE(resources);

            resources->clearLocal();
        }

        STDMETHODIMP_(void) onUpdate(UINT32 handle, bool isIdle)
        {
            if (handle == backgroundUpdateHandle)
            {
                ObservableMixin::sendEvent(Event<RenderObserver>(std::bind(&RenderObserver::onRenderBackground, std::placeholders::_1)));
            }
            else if (handle == foregroundUpdateHandle)
            {
                ObservableMixin::sendEvent(Event<RenderObserver>(std::bind(&RenderObserver::onRenderForeground, std::placeholders::_1)));
                video->present(false);
            }
        }
    };

    REGISTER_CLASS(RenderImplementation)
}; // namespace Gek
