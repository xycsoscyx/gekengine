#include "GEK\Utility\String.h"
#include "GEK\Utility\FileSystem.h"
#include "GEK\Utility\XML.h"
#include "GEK\Context\ContextUser.h"
#include "GEK\Context\ObservableMixin.h"
#include "GEK\Engine\Engine.h"
#include "GEK\Engine\Render.h"
#include "GEK\Engine\Resources.h"
#include "GEK\Engine\Plugin.h"
#include "GEK\Engine\Shader.h"
#include "GEK\Engine\Material.h"
#include "GEK\Engine\Population.h"
#include "GEK\Engine\Entity.h"
#include "GEK\Engine\Component.h"
#include "GEK\Components\Transform.h"
#include "GEK\Shapes\Sphere.h"
#include <set>
#include <ppl.h>
#include <concurrent_vector.h>

namespace Gek
{
    class RenderPipelineImplementation
        : public RenderPipeline
    {
    private:
        VideoPipeline *videoPipeline;

    public:
        RenderPipelineImplementation(VideoPipeline *videoPipeline)
            : videoPipeline(videoPipeline)
        {
        }

        // RenderPipeline
        void setProgram(VideoObject *program)
        {
            videoPipeline->setProgram(program);
        }

        void setConstantBuffer(VideoBuffer *constantBuffer, uint32_t stage)
        {
            videoPipeline->setConstantBuffer(constantBuffer, stage);
        }

        void setSamplerState(VideoObject *samplerState, uint32_t stage)
        {
            videoPipeline->setSamplerState(samplerState, stage);
        }

        void setResource(VideoObject *resource, uint32_t stage)
        {
            videoPipeline->setResource(resource, stage);
        }

        void setUnorderedAccess(VideoObject *unorderedAccess, uint32_t stage)
        {
            videoPipeline->setUnorderedAccess(unorderedAccess, stage);
        }
    };

    class RenderContextImplementation
        : public RenderContext
    {
    private:
        VideoContext *videoContext;
        RenderPipelinePtr computePipelineHandler;
        RenderPipelinePtr vertexPipelineHandler;
        RenderPipelinePtr geometryPipelineHandler;
        RenderPipelinePtr pixelPipelineHandler;

    public:
        RenderContextImplementation(VideoContext *videoContext)
            : videoContext(videoContext)
            , computePipelineHandler(makeShared<RenderPipeline, RenderPipelineImplementation>(videoContext->computePipeline()))
            , vertexPipelineHandler(makeShared<RenderPipeline, RenderPipelineImplementation>(videoContext->vertexPipeline()))
            , geometryPipelineHandler(makeShared<RenderPipeline, RenderPipelineImplementation>(videoContext->geometryPipeline()))
            , pixelPipelineHandler(makeShared<RenderPipeline, RenderPipelineImplementation>(videoContext->pixelPipeline()))
        {
        }

        // RenderContext
        RenderPipeline * const computePipeline(void) { return computePipelineHandler.get(); };
        RenderPipeline * const vertexPipeline(void) { return vertexPipelineHandler.get(); };
        RenderPipeline * const geometryPipeline(void) { return geometryPipelineHandler.get(); };
        RenderPipeline * const pixelPipeline(void) { return pixelPipelineHandler.get(); };

        void generateMipMaps(VideoTexture *texture)
        {
            videoContext->generateMipMaps(texture);
        }

        void clearResources(void)
        {
            videoContext->clearResources();
        }

        void setViewports(Video::ViewPort *viewPortList, uint32_t viewPortCount)
        {
            videoContext->setViewports(viewPortList, viewPortCount);
        }

        void setScissorRect(Shapes::Rectangle<uint32_t> *rectangleList, uint32_t rectangleCount)
        {
            videoContext->setScissorRect(rectangleList, rectangleCount);
        }

        void clearRenderTarget(VideoTarget *renderTarget, const Math::Color &clearColor)
        {
            videoContext->clearRenderTarget(renderTarget, clearColor);
        }

        void clearDepthStencilTarget(VideoObject *depthBuffer, uint32_t flags, float clearDepth, uint32_t clearStencil)
        {
            videoContext->clearDepthStencilTarget(depthBuffer, flags, clearDepth, clearStencil);
        }

        void setRenderTargets(VideoTarget **renderTargetList, uint32_t renderTargetCount, VideoObject *depthBuffer)
        {
            videoContext->setRenderTargets(renderTargetList, renderTargetCount, depthBuffer);
        }

        void setRenderState(VideoObject *renderState)
        {
            videoContext->setRenderState(renderState);
        }

        void setDepthState(VideoObject *depthState, uint32_t stencilReference)
        {
            videoContext->setDepthState(depthState, stencilReference);
        }

        void setBlendState(VideoObject *blendState, const Math::Color &blendFactor, uint32_t sampleMask)
        {
            videoContext->setBlendState(blendState, blendFactor, sampleMask);
        }

        void setVertexBuffer(uint32_t slot, VideoBuffer *vertexBuffer, uint32_t offset)
        {
            videoContext->setVertexBuffer(slot, vertexBuffer, offset);
        }

        void setIndexBuffer(VideoBuffer *indexBuffer, uint32_t offset)
        {
            videoContext->setIndexBuffer(indexBuffer, offset);
        }

        void setPrimitiveType(Video::PrimitiveType type)
        {
            videoContext->setPrimitiveType(type);
        }

        void drawPrimitive(uint32_t vertexCount, uint32_t firstVertex)
        {
            videoContext->drawPrimitive(vertexCount, firstVertex);
        }

        void drawInstancedPrimitive(uint32_t instanceCount, uint32_t firstInstance, uint32_t vertexCount, uint32_t firstVertex)
        {
            videoContext->drawInstancedPrimitive(instanceCount, firstInstance, vertexCount, firstVertex);
        }

        void drawIndexedPrimitive(uint32_t indexCount, uint32_t firstIndex, uint32_t firstVertex)
        {
            videoContext->drawIndexedPrimitive(indexCount, firstIndex, firstVertex);
        }

        void drawInstancedIndexedPrimitive(uint32_t instanceCount, uint32_t firstInstance, uint32_t indexCount, uint32_t firstIndex, uint32_t firstVertex)
        {
            videoContext->drawInstancedIndexedPrimitive(instanceCount, firstInstance, indexCount, firstIndex, firstVertex);
        }

        void dispatch(uint32_t threadGroupCountX, uint32_t threadGroupCountY, uint32_t threadGroupCountZ)
        {
            videoContext->dispatch(threadGroupCountX, threadGroupCountY, threadGroupCountZ);
        }
    };

    class RenderImplementation
        : public ContextRegistration<RenderImplementation, VideoSystem *, Population *, Resources *>
        , public ObservableMixin<RenderObserver>
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
                uint32_t value;
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
        VideoSystem *video;
        Population *population;
        uint32_t backgroundUpdateHandle;
        uint32_t foregroundUpdateHandle;
        Resources *resources;

        VideoObjectPtr pointSamplerState;
        VideoObjectPtr linearClampSamplerState;
        VideoObjectPtr linearWrapSamplerState;
        VideoBufferPtr engineConstantBuffer;
        VideoBufferPtr cameraConstantBuffer;

        VideoObjectPtr deferredVertexProgram;

        DrawCallList drawCallList;

    public:
        RenderImplementation(Context *context, VideoSystem *video, Population *population, Resources *resources)
            : ContextRegistration(context)
            , video(video)
            , population(population)
            , backgroundUpdateHandle(0)
            , foregroundUpdateHandle(0)
            , resources(resources)
        {
            GEK_TRACE_SCOPE();

            population->addObserver((PopulationObserver *)this);
            backgroundUpdateHandle = population->setUpdatePriority(this, 10);
            foregroundUpdateHandle = population->setUpdatePriority(this, 100);

            Video::SamplerState pointSamplerStateData;
            pointSamplerStateData.filterMode = Video::FilterMode::AllPoint;
            pointSamplerStateData.addressModeU = Video::AddressMode::Clamp;
            pointSamplerStateData.addressModeV = Video::AddressMode::Clamp;
            pointSamplerState = video->createSamplerState(pointSamplerStateData);

            Video::SamplerState linearClampSamplerStateData;
            linearClampSamplerStateData.maximumAnisotropy = 8;
            linearClampSamplerStateData.filterMode = Video::FilterMode::Anisotropic;
            linearClampSamplerStateData.addressModeU = Video::AddressMode::Clamp;
            linearClampSamplerStateData.addressModeV = Video::AddressMode::Clamp;
            linearClampSamplerState = video->createSamplerState(linearClampSamplerStateData);

            Video::SamplerState linearWrapSamplerStateData;
            linearWrapSamplerStateData.maximumAnisotropy = 8;
            linearWrapSamplerStateData.filterMode = Video::FilterMode::Anisotropic;
            linearWrapSamplerStateData.addressModeU = Video::AddressMode::Wrap;
            linearWrapSamplerStateData.addressModeV = Video::AddressMode::Wrap;
            linearWrapSamplerState = video->createSamplerState(linearWrapSamplerStateData);

            engineConstantBuffer = video->createBuffer(sizeof(EngineConstantData), 1, Video::BufferType::Constant, 0);

            cameraConstantBuffer = video->createBuffer(sizeof(CameraConstantData), 1, Video::BufferType::Constant, 0);

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

            deferredVertexProgram = video->compileVertexProgram(program, "mainVertexProgram");
        }

        ~RenderImplementation(void)
        {
            if (population)
            {
                population->removeUpdatePriority(foregroundUpdateHandle);
                population->removeUpdatePriority(backgroundUpdateHandle);
            }

            population->removeObserver((PopulationObserver *)this);
        }

        // Render
        VideoSystem * getVideoSystem(void) const
        {
            return video;
        }

        void queueDrawCall(PluginHandle plugin, MaterialHandle material, std::function<void(RenderContext *renderContext)> draw)
        {
            if (plugin && material && draw)
            {
                ShaderHandle shader = resources->getMaterialShader(material);
                if (shader)
                {
                    drawCallList.push_back(DrawCallValue(material, plugin, shader, draw));
                }
            }
        }

        void render(Entity *cameraEntity, const Math::Float4x4 &projectionMatrix, float minimumDistance, float maximumDistance)
        {
            GEK_TRACE_SCOPE();
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
            sendEvent(Event(std::bind(&RenderObserver::onRenderScene, std::placeholders::_1, cameraEntity, &cameraConstantData.viewMatrix, &viewFrustum)));
            if (!drawCallList.empty())
            {
                VideoContext *videoContext = video->getDefaultContext();
                RenderContextPtr renderContext(makeShared<RenderContext, RenderContextImplementation>(videoContext));

                video->updateResource(engineConstantBuffer.get(), &engineConstantData);
                videoContext->geometryPipeline()->setConstantBuffer(engineConstantBuffer.get(), 0);
                videoContext->vertexPipeline()->setConstantBuffer(engineConstantBuffer.get(), 0);
                videoContext->pixelPipeline()->setConstantBuffer(engineConstantBuffer.get(), 0);
                videoContext->computePipeline()->setConstantBuffer(engineConstantBuffer.get(), 0);

                video->updateResource(cameraConstantBuffer.get(), &cameraConstantData);
                videoContext->geometryPipeline()->setConstantBuffer(cameraConstantBuffer.get(), 1);
                videoContext->vertexPipeline()->setConstantBuffer(cameraConstantBuffer.get(), 1);
                videoContext->pixelPipeline()->setConstantBuffer(cameraConstantBuffer.get(), 1);
                videoContext->computePipeline()->setConstantBuffer(cameraConstantBuffer.get(), 1);

                videoContext->pixelPipeline()->setSamplerState(pointSamplerState.get(), 0);
                videoContext->pixelPipeline()->setSamplerState(linearClampSamplerState.get(), 1);
                videoContext->pixelPipeline()->setSamplerState(linearWrapSamplerState.get(), 2);

                videoContext->vertexPipeline()->setSamplerState(pointSamplerState.get(), 0);
                videoContext->vertexPipeline()->setSamplerState(linearClampSamplerState.get(), 1);
                videoContext->vertexPipeline()->setSamplerState(linearWrapSamplerState.get(), 2);

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
                    Shader *shader = resources->getShader(currentShader);
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

                for (auto &drawCallSet : drawCallSetList)
                {
                    auto &shader = drawCallSet.shader;
                    for (auto block = shader->begin(renderContext.get(), cameraConstantData.viewMatrix, viewFrustum); block; block = block->next())
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
                                                Plugin *plugin = resources->getPlugin(currentPlugin);
                                                if (!plugin)
                                                {
                                                    continue;
                                                }

                                                plugin->enable(videoContext);
                                            }

                                            if (currentMaterial != (*shaderDrawCall).material)
                                            {
                                                currentMaterial = (*shaderDrawCall).material;
                                                Material *material = resources->getMaterial(currentMaterial);
                                                if (!material)
                                                {
                                                    continue;
                                                }

                                                shader->setResourceList(renderContext.get(), block.get(), pass.get(), material->getResourceList());
                                            }

                                            (*shaderDrawCall).onDraw(renderContext.get());
                                        }
                                    }

                                    break;

                                case Shader::Pass::Mode::Deferred:
                                    videoContext->vertexPipeline()->setProgram(deferredVertexProgram.get());
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
        void onLoadBegin(void)
        {
        }

        void onLoadSucceeded(void)
        {
        }

        void onLoadFailed(void)
        {
            onFree();
        }

        void onFree(void)
        {
            GEK_REQUIRE(resources);

            resources->clearLocal();
        }

        void onUpdate(uint32_t handle, bool isIdle)
        {
            if (handle == backgroundUpdateHandle)
            {
                sendEvent(Event(std::bind(&RenderObserver::onRenderBackground, std::placeholders::_1)));
            }
            else if (handle == foregroundUpdateHandle)
            {
                sendEvent(Event(std::bind(&RenderObserver::onRenderForeground, std::placeholders::_1)));
                video->present(false);
            }
        }
    };

    GEK_REGISTER_CONTEXT_USER(RenderImplementation);
}; // namespace Gek
