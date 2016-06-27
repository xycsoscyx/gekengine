#pragma once

#include "GEK\Context\Context.h"
#include "GEK\Context\Observable.h"
#include "GEK\Engine\Resources.h"
#include "GEK\Shapes\Frustum.h"

namespace Gek
{
    GEK_PREDECLARE(VideoSystem);
    GEK_PREDECLARE(VideoPipeline);
    GEK_PREDECLARE(VideoContext);
    GEK_PREDECLARE(Entity);

    GEK_INTERFACE(RenderPipeline)
    {
        virtual void setProgram(VideoObject *program) = 0;
        virtual void setConstantBuffer(VideoBuffer *constantBuffer, uint32_t stage) = 0;
        virtual void setSamplerState(VideoObject *samplerState, uint32_t stage) = 0;
        virtual void setResource(VideoObject *resource, uint32_t stage) = 0;
        virtual void setUnorderedAccess(VideoObject *unorderedAccess, uint32_t stage) = 0;
    };

    GEK_INTERFACE(RenderContext)
    {
        virtual RenderPipeline * const computePipeline(void) = 0;
        virtual RenderPipeline * const vertexPipeline(void) = 0;
        virtual RenderPipeline * const geometryPipeline(void) = 0;
        virtual RenderPipeline * const pixelPipeline(void) = 0;

        virtual void generateMipMaps(VideoTexture *texture) = 0;

        virtual void clearResources(void) = 0;

        virtual void setViewports(Video::ViewPort *viewPortList, uint32_t viewPortCount) = 0;
        virtual void setScissorRect(Shapes::Rectangle<uint32_t> *rectangleList, uint32_t rectangleCount) = 0;

        virtual void clearRenderTarget(VideoTarget *renderTarget, const Math::Color &clearColor) = 0;
        virtual void clearDepthStencilTarget(VideoObject *depthBuffer, uint32_t flags, float clearDepth, uint32_t clearStencil) = 0;
        virtual void setRenderTargets(VideoTarget **renderTargetList, uint32_t renderTargetCount, VideoObject *depthBuffer) = 0;

        virtual void setRenderState(VideoObject *renderState) = 0;
        virtual void setDepthState(VideoObject *depthState, uint32_t stencilReference) = 0;
        virtual void setBlendState(VideoObject *blendState, const Math::Color &blendFactor, uint32_t sampleMask) = 0;

        virtual void setVertexBuffer(uint32_t slot, VideoBuffer *vertexBuffer, uint32_t offset) = 0;
        virtual void setIndexBuffer(VideoBuffer *indexBuffer, uint32_t offset) = 0;
        virtual void setPrimitiveType(Video::PrimitiveType type) = 0;

        virtual void drawPrimitive(uint32_t vertexCount, uint32_t firstVertex) = 0;
        virtual void drawInstancedPrimitive(uint32_t instanceCount, uint32_t firstInstance, uint32_t vertexCount, uint32_t firstVertex) = 0;

        virtual void drawIndexedPrimitive(uint32_t indexCount, uint32_t firstIndex, uint32_t firstVertex) = 0;
        virtual void drawInstancedIndexedPrimitive(uint32_t instanceCount, uint32_t firstInstance, uint32_t indexCount, uint32_t firstIndex, uint32_t firstVertex) = 0;

        virtual void dispatch(uint32_t threadGroupCountX, uint32_t threadGroupCountY, uint32_t threadGroupCountZ) = 0;
    };

    GEK_INTERFACE(Render)
        : virtual public Observable
    {
        virtual VideoSystem * getVideoSystem(void) const = 0;

        virtual void render(Entity *cameraEntity, const Math::Float4x4 &projectionMatrix, float nearClip, float farClip, ResourceHandle cameraTarget) = 0;
        virtual void queueDrawCall(PluginHandle plugin, MaterialHandle material, std::function<void(RenderContext *renderContext)> draw) = 0;
    };

    GEK_INTERFACE(RenderObserver)
        : public Observer
    {
        virtual void onRenderBackground(void) { };
        virtual void onRenderScene(Entity *cameraEntity, const Math::Float4x4 *viewMatrix, const Shapes::Frustum *viewFrustum) { };
        virtual void onRenderForeground(void) { };
    };
}; // namespace Gek
