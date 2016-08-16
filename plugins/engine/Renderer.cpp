#include "GEK\Utility\String.h"
#include "GEK\Utility\FileSystem.h"
#include "GEK\Utility\XML.h"
#include "GEK\Context\ContextUser.h"
#include "GEK\Engine\Core.h"
#include "GEK\Engine\Renderer.h"
#include "GEK\Engine\Resources.h"
#include "GEK\Engine\Visual.h"
#include "GEK\Engine\Shader.h"
#include "GEK\Engine\Filter.h"
#include "GEK\Engine\Material.h"
#include "GEK\Engine\Population.h"
#include "GEK\Engine\Entity.h"
#include "GEK\Engine\Component.h"
#include "GEK\Components\Transform.h"
#include "GEK\Components\Filter.h"
#include "GEK\Shapes\Sphere.h"
#include <set>
#include <ppl.h>
#include <concurrent_vector.h>

// Function Traits
// http://stackoverflow.com/questions/2562320/specializing-a-template-on-a-lambda-in-c0x

namespace Gek
{
    namespace Video
    {
        ElementType getElementType(const String &elementClassString)
        {
            if (elementClassString.compareNoCase(L"instance") == 0) return ElementType::Instance;
            /*else if (elementClassString.compareNoCase(L"vertex") == 0) */ return ElementType::Vertex;
        }

        Format getFormat(const String &formatString)
        {
            if (formatString.compareNoCase(L"Unknown") == 0) return Format::Unknown;
            else if (formatString.compareNoCase(L"R32G32B32A32_FLOAT") == 0) return Format::R32G32B32A32_FLOAT;
            else if (formatString.compareNoCase(L"R16G16B16A16_FLOAT") == 0) return Format::R16G16B16A16_FLOAT;
            else if (formatString.compareNoCase(L"R32G32B32_FLOAT") == 0) return Format::R32G32B32_FLOAT;
            else if (formatString.compareNoCase(L"R11G11B10_FLOAT") == 0) return Format::R11G11B10_FLOAT;
            else if (formatString.compareNoCase(L"R32G32_FLOAT") == 0) return Format::R32G32_FLOAT;
            else if (formatString.compareNoCase(L"R16G16_FLOAT") == 0) return Format::R16G16_FLOAT;
            else if (formatString.compareNoCase(L"R32_FLOAT") == 0) return Format::R32_FLOAT;
            else if (formatString.compareNoCase(L"R16_FLOAT") == 0) return Format::R16_FLOAT;

            else if (formatString.compareNoCase(L"R32G32B32A32_UINT") == 0) return Format::R32G32B32A32_UINT;
            else if (formatString.compareNoCase(L"R16G16B16A16_UINT") == 0) return Format::R16G16B16A16_UINT;
            else if (formatString.compareNoCase(L"R10G10B10A2_UINT") == 0) return Format::R10G10B10A2_UINT;
            else if (formatString.compareNoCase(L"R8G8B8A8_UINT") == 0) return Format::R8G8B8A8_UINT;
            else if (formatString.compareNoCase(L"R32G32B32_UINT") == 0) return Format::R32G32B32_UINT;
            else if (formatString.compareNoCase(L"R32G32_UINT") == 0) return Format::R32G32_UINT;
            else if (formatString.compareNoCase(L"R16G16_UINT") == 0) return Format::R16G16_UINT;
            else if (formatString.compareNoCase(L"R8G8_UINT") == 0) return Format::R8G8_UINT;
            else if (formatString.compareNoCase(L"R32_UINT") == 0) return Format::R32_UINT;
            else if (formatString.compareNoCase(L"R16_UINT") == 0) return Format::R16_UINT;
            else if (formatString.compareNoCase(L"R8_UINT") == 0) return Format::R8_UINT;

            else if (formatString.compareNoCase(L"R32G32B32A32_INT") == 0) return Format::R32G32B32A32_INT;
            else if (formatString.compareNoCase(L"R16G16B16A16_INT") == 0) return Format::R16G16B16A16_INT;
            else if (formatString.compareNoCase(L"R8G8B8A8_INT") == 0) return Format::R8G8B8A8_INT;
            else if (formatString.compareNoCase(L"R32G32B32_INT") == 0) return Format::R32G32B32_INT;
            else if (formatString.compareNoCase(L"R32G32_INT") == 0) return Format::R32G32_INT;
            else if (formatString.compareNoCase(L"R16G16_INT") == 0) return Format::R16G16_INT;
            else if (formatString.compareNoCase(L"R8G8_INT") == 0) return Format::R8G8_INT;
            else if (formatString.compareNoCase(L"R32_INT") == 0) return Format::R32_INT;
            else if (formatString.compareNoCase(L"R16_INT") == 0) return Format::R16_INT;
            else if (formatString.compareNoCase(L"R8_INT") == 0) return Format::R8_INT;

            else if (formatString.compareNoCase(L"R16G16B16A16_UNORM") == 0) return Format::R16G16B16A16_UNORM;
            else if (formatString.compareNoCase(L"R10G10B10A2_UNORM") == 0) return Format::R10G10B10A2_UNORM;
            else if (formatString.compareNoCase(L"R8G8B8A8_UNORM") == 0) return Format::R8G8B8A8_UNORM;
            else if (formatString.compareNoCase(L"R8G8B8A8_UNORM_SRGB") == 0) return Format::R8G8B8A8_UNORM_SRGB;
            else if (formatString.compareNoCase(L"R16G16_UNORM") == 0) return Format::R16G16_UNORM;
            else if (formatString.compareNoCase(L"R8G8_UNORM") == 0) return Format::R8G8_UNORM;
            else if (formatString.compareNoCase(L"R16_UNORM") == 0) return Format::R16_UNORM;
            else if (formatString.compareNoCase(L"R8_UNORM") == 0) return Format::R8_UNORM;

            else if (formatString.compareNoCase(L"R16G16B16A16_NORM") == 0) return Format::R16G16B16A16_NORM;
            else if (formatString.compareNoCase(L"R8G8B8A8_NORM") == 0) return Format::R8G8B8A8_NORM;
            else if (formatString.compareNoCase(L"R16G16_NORM") == 0) return Format::R16G16_NORM;
            else if (formatString.compareNoCase(L"R8G8_NORM") == 0) return Format::R8G8_NORM;
            else if (formatString.compareNoCase(L"R16_NORM") == 0) return Format::R16_NORM;
            else if (formatString.compareNoCase(L"R8_NORM") == 0) return Format::R8_NORM;

            else if (formatString.compareNoCase(L"D32_FLOAT_S8X24_UINT") == 0) return Format::D32_FLOAT_S8X24_UINT;
            else if (formatString.compareNoCase(L"D24_UNORM_S8_UINT") == 0) return Format::D24_UNORM_S8_UINT;

            else if (formatString.compareNoCase(L"D32_FLOAT") == 0) return Format::D32_FLOAT;
            else if (formatString.compareNoCase(L"D16_UNORM") == 0) return Format::D16_UNORM;

            return Format::Unknown;
        }

        DepthWrite getDepthWriteMask(const String &depthWrite)
        {
            if (depthWrite.compareNoCase(L"zero") == 0) return DepthWrite::Zero;
            else if (depthWrite.compareNoCase(L"all") == 0) return DepthWrite::All;
            else return DepthWrite::Zero;
        }

        ComparisonFunction getComparisonFunction(const String &comparisonFunction)
        {
            if (comparisonFunction.compareNoCase(L"always") == 0) return ComparisonFunction::Always;
            else if (comparisonFunction.compareNoCase(L"never") == 0) return ComparisonFunction::Never;
            else if (comparisonFunction.compareNoCase(L"equal") == 0) return ComparisonFunction::Equal;
            else if (comparisonFunction.compareNoCase(L"not_equal") == 0) return ComparisonFunction::NotEqual;
            else if (comparisonFunction.compareNoCase(L"less") == 0) return ComparisonFunction::Less;
            else if (comparisonFunction.compareNoCase(L"less_equal") == 0) return ComparisonFunction::LessEqual;
            else if (comparisonFunction.compareNoCase(L"greater") == 0) return ComparisonFunction::Greater;
            else if (comparisonFunction.compareNoCase(L"greater_equal") == 0) return ComparisonFunction::GreaterEqual;
            else return ComparisonFunction::Always;
        }

        StencilOperation getStencilOperation(const String &stencilOperation)
        {
            if (stencilOperation.compareNoCase(L"zero") == 0) return StencilOperation::Zero;
            else if (stencilOperation.compareNoCase(L"keep") == 0) return StencilOperation::Keep;
            else if (stencilOperation.compareNoCase(L"replace") == 0) return StencilOperation::Replace;
            else if (stencilOperation.compareNoCase(L"invert") == 0) return StencilOperation::Invert;
            else if (stencilOperation.compareNoCase(L"increase") == 0) return StencilOperation::Increase;
            else if (stencilOperation.compareNoCase(L"increase_saturated") == 0) return StencilOperation::IncreaseSaturated;
            else if (stencilOperation.compareNoCase(L"decrease") == 0) return StencilOperation::Decrease;
            else if (stencilOperation.compareNoCase(L"decrease_saturated") == 0) return StencilOperation::DecreaseSaturated;
            else return StencilOperation::Keep;
        }

        FillMode getFillMode(const String &fillMode)
        {
            if (fillMode.compareNoCase(L"solid") == 0) return FillMode::Solid;
            else if (fillMode.compareNoCase(L"wire") == 0) return FillMode::WireFrame;
            else return FillMode::Solid;
        }

        CullMode getCullMode(const String &cullMode)
        {
            if (cullMode.compareNoCase(L"none") == 0) return CullMode::None;
            else if (cullMode.compareNoCase(L"front") == 0) return CullMode::Front;
            else if (cullMode.compareNoCase(L"back") == 0) return CullMode::Back;
            else return CullMode::None;
        }

        BlendSource getBlendSource(const String &blendSource)
        {
            if (blendSource.compareNoCase(L"zero") == 0) return BlendSource::Zero;
            else if (blendSource.compareNoCase(L"one") == 0) return BlendSource::One;
            else if (blendSource.compareNoCase(L"blend_factor") == 0) return BlendSource::BlendFactor;
            else if (blendSource.compareNoCase(L"inverse_blend_factor") == 0) return BlendSource::InverseBlendFactor;
            else if (blendSource.compareNoCase(L"source_color") == 0) return BlendSource::SourceColor;
            else if (blendSource.compareNoCase(L"inverse_source_color") == 0) return BlendSource::InverseSourceColor;
            else if (blendSource.compareNoCase(L"source_alpha") == 0) return BlendSource::SourceAlpha;
            else if (blendSource.compareNoCase(L"inverse_source_alpha") == 0) return BlendSource::InverseSourceAlpha;
            else if (blendSource.compareNoCase(L"source_alpha_saturate") == 0) return BlendSource::SourceAlphaSaturated;
            else if (blendSource.compareNoCase(L"destination_color") == 0) return BlendSource::DestinationColor;
            else if (blendSource.compareNoCase(L"inverse_destination_color") == 0) return BlendSource::InverseDestinationColor;
            else if (blendSource.compareNoCase(L"destination_alpha") == 0) return BlendSource::DestinationAlpha;
            else if (blendSource.compareNoCase(L"inverse_destination_alpha") == 0) return BlendSource::InverseDestinationAlpha;
            else if (blendSource.compareNoCase(L"secondary_source_color") == 0) return BlendSource::SecondarySourceColor;
            else if (blendSource.compareNoCase(L"inverse_secondary_source_color") == 0) return BlendSource::InverseSecondarySourceColor;
            else if (blendSource.compareNoCase(L"secondary_source_alpha") == 0) return BlendSource::SecondarySourceAlpha;
            else if (blendSource.compareNoCase(L"inverse_secondary_source_alpha") == 0) return BlendSource::InverseSecondarySourceAlpha;
            else return BlendSource::One;
        }

        BlendOperation getBlendOperation(const String &blendOperation)
        {
            if (blendOperation.compareNoCase(L"add") == 0) return BlendOperation::Add;
            else if (blendOperation.compareNoCase(L"subtract") == 0) return BlendOperation::Subtract;
            else if (blendOperation.compareNoCase(L"reverse_subtract") == 0) return BlendOperation::ReverseSubtract;
            else if (blendOperation.compareNoCase(L"minimum") == 0) return BlendOperation::Minimum;
            else if (blendOperation.compareNoCase(L"maximum") == 0) return BlendOperation::Maximum;
            else return BlendOperation::Add;
        }
    }; // namespace Video

    namespace Implementation
    {
        GEK_CONTEXT_USER(Renderer, Video::Device *, Plugin::Population *, Engine::Resources *)
            , public Plugin::PopulationListener
            , public Plugin::PopulationStep
            , public Plugin::Renderer
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
                float nearClip;
                float farClip;
                Math::Float4x4 viewMatrix;
                Math::Float4x4 projectionMatrix;
            };

            struct DrawCallValue
            {
                union
                {
                    uint32_t value;
                    struct
                    {
                        MaterialHandle material;
                        VisualHandle plugin;
                        ShaderHandle shader;
                    };
                };

                std::function<void(Video::Device::Context *)> onDraw;

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

                DrawCallValue(MaterialHandle material, VisualHandle plugin, ShaderHandle shader, std::function<void(Video::Device::Context *)> onDraw)
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

            using DrawCallList = concurrency::concurrent_vector<DrawCallValue>;

            struct DrawCallSet
            {
                Engine::Shader *shader;
                DrawCallList::iterator begin;
                DrawCallList::iterator end;

                DrawCallSet(Engine::Shader *shader, DrawCallList::iterator begin, DrawCallList::iterator end)
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

                DrawCallSet &operator = (const DrawCallSet &drawCallSet)
                {
                    shader = drawCallSet.shader;
                    begin = drawCallSet.begin;
                    end = drawCallSet.end;
                    return (*this);
                }

                DrawCallSet &operator = (DrawCallSet &&drawCallSet)
                {
                    shader = std::move(drawCallSet.shader);
                    begin = std::move(drawCallSet.begin);
                    end = std::move(drawCallSet.end);
                    return (*this);
                }
            };

        private:
            Video::Device *device;
            Plugin::Population *population;
            Engine::Resources *resources;

            Video::ObjectPtr pointSamplerState;
            Video::ObjectPtr linearClampSamplerState;
            Video::ObjectPtr linearWrapSamplerState;
            Video::BufferPtr engineConstantBuffer;
            Video::BufferPtr cameraConstantBuffer;

            Video::ObjectPtr deferredVertexProgram;
            Video::ObjectPtr deferredPixelProgram;

            DrawCallList drawCallList;

        public:
            Renderer(Context *context, Video::Device *device, Plugin::Population *population, Engine::Resources *resources)
                : ContextRegistration(context)
                , device(device)
                , population(population)
                , resources(resources)
            {
                population->addListener(this);
                population->addStep(this, 10, 100);

                Video::SamplerStateInformation pointSamplerStateData;
                pointSamplerStateData.filterMode = Video::FilterMode::AllPoint;
                pointSamplerStateData.addressModeU = Video::AddressMode::Clamp;
                pointSamplerStateData.addressModeV = Video::AddressMode::Clamp;
                pointSamplerState = device->createSamplerState(pointSamplerStateData);

                Video::SamplerStateInformation linearClampSamplerStateData;
                linearClampSamplerStateData.maximumAnisotropy = 8;
                linearClampSamplerStateData.filterMode = Video::FilterMode::Anisotropic;
                linearClampSamplerStateData.addressModeU = Video::AddressMode::Clamp;
                linearClampSamplerStateData.addressModeV = Video::AddressMode::Clamp;
                linearClampSamplerState = device->createSamplerState(linearClampSamplerStateData);

                Video::SamplerStateInformation linearWrapSamplerStateData;
                linearWrapSamplerStateData.maximumAnisotropy = 8;
                linearWrapSamplerStateData.filterMode = Video::FilterMode::Anisotropic;
                linearWrapSamplerStateData.addressModeU = Video::AddressMode::Wrap;
                linearWrapSamplerStateData.addressModeV = Video::AddressMode::Wrap;
                linearWrapSamplerState = device->createSamplerState(linearWrapSamplerStateData);

                engineConstantBuffer = device->createBuffer(sizeof(EngineConstantData), 1, Video::BufferType::Constant, 0);
                engineConstantBuffer->setName(L"engineConstantBuffer");

                cameraConstantBuffer = device->createBuffer(sizeof(CameraConstantData), 1, Video::BufferType::Constant, 0);
                cameraConstantBuffer->setName(L"cameraConstantBuffer");

                static const char program[] =
                    "struct Pixel                                                                       \r\n" \
                    "{                                                                                  \r\n" \
                    "    float4 screen : SV_POSITION;                                                   \r\n" \
                    "    float2 texCoord : TEXCOORD0;                                                   \r\n" \
                    "};                                                                                 \r\n" \
                    "                                                                                   \r\n" \
                    "Pixel mainVertexProgram(in uint vertexID : SV_VertexID)                            \r\n" \
                    "{                                                                                  \r\n" \
                    "    Pixel pixel;                                                                   \r\n" \
                    "    pixel.texCoord = float2((vertexID << 1) & 2, vertexID & 2);                    \r\n" \
                    "    pixel.screen = float4(pixel.texCoord * float2(2.0f, -2.0f)                     \r\n" \
                    "                                           + float2(-1.0f, 1.0f), 0.0f, 1.0f);     \r\n" \
                    "    return pixel;                                                                  \r\n" \
                    "}                                                                                  \r\n" \
                    "                                                                                   \r\n" \
                    "Texture2D<float3> screenBuffer : register(t0);                                     \r\n" \
                    "float4 mainPixelProgram(Pixel inputPixel) : SV_TARGET0                             \r\n" \
                    "{                                                                                  \r\n" \
                    "    return float4(screenBuffer[inputPixel.screen.xy], 1.0);                        \r\n" \
                    "}                                                                                  \r\n" \
                    "                                                                                   \r\n";

                deferredVertexProgram = device->compileVertexProgram(program, "mainVertexProgram");
                deferredVertexProgram->setName(L"deferredVertexProgram");

                deferredPixelProgram = device->compilePixelProgram(program, "mainPixelProgram");
                deferredPixelProgram->setName(L"deferredPixelProgram");
            }

            ~Renderer(void)
            {
                population->removeStep(this);
                population->removeListener(this);
            }

            // Renderer
            Video::Device * getDevice(void) const
            {
                return device;
            }

            void queueDrawCall(VisualHandle plugin, MaterialHandle material, std::function<void(Video::Device::Context *deviceContext)> draw)
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

            void render(Plugin::Entity *cameraEntity, const Math::Float4x4 &projectionMatrix, float nearClip, float farClip, ResourceHandle cameraTarget)
            {
                GEK_REQUIRE(device);
                GEK_REQUIRE(population);
                GEK_REQUIRE(cameraEntity);

                auto &cameraTransform = cameraEntity->getComponent<Components::Transform>();
                Math::Float4x4 cameraMatrix(cameraTransform.getMatrix());
                Math::Float4x4 viewMatrix(cameraMatrix.getInverse());

                const Shapes::Frustum viewFrustum(viewMatrix * projectionMatrix);

                EngineConstantData engineConstantData;
                engineConstantData.frameTime = population->getFrameTime();
                engineConstantData.worldTime = population->getWorldTime();

                CameraConstantData cameraConstantData;
                cameraConstantData.fieldOfView.x = (1.0f / projectionMatrix._11);
                cameraConstantData.fieldOfView.y = (1.0f / projectionMatrix._22);
                cameraConstantData.nearClip = nearClip;
                cameraConstantData.farClip = farClip;
                cameraConstantData.viewMatrix = viewMatrix;
                cameraConstantData.projectionMatrix = projectionMatrix;

                drawCallList.clear();
                sendShout(&Plugin::RendererListener::onRenderScene, cameraEntity, cameraConstantData.viewMatrix, viewFrustum);
                if (!drawCallList.empty())
                {
                    Video::Device::Context *deviceContext = device->getDefaultContext();
                    deviceContext->clearState();

                    device->updateResource(engineConstantBuffer.get(), &engineConstantData);
                    deviceContext->geometryPipeline()->setConstantBuffer(engineConstantBuffer.get(), 0);
                    deviceContext->vertexPipeline()->setConstantBuffer(engineConstantBuffer.get(), 0);
                    deviceContext->pixelPipeline()->setConstantBuffer(engineConstantBuffer.get(), 0);
                    deviceContext->computePipeline()->setConstantBuffer(engineConstantBuffer.get(), 0);

                    device->updateResource(cameraConstantBuffer.get(), &cameraConstantData);
                    deviceContext->geometryPipeline()->setConstantBuffer(cameraConstantBuffer.get(), 1);
                    deviceContext->vertexPipeline()->setConstantBuffer(cameraConstantBuffer.get(), 1);
                    deviceContext->pixelPipeline()->setConstantBuffer(cameraConstantBuffer.get(), 1);
                    deviceContext->computePipeline()->setConstantBuffer(cameraConstantBuffer.get(), 1);

                    deviceContext->pixelPipeline()->setSamplerState(pointSamplerState.get(), 0);
                    deviceContext->pixelPipeline()->setSamplerState(linearClampSamplerState.get(), 1);
                    deviceContext->pixelPipeline()->setSamplerState(linearWrapSamplerState.get(), 2);

                    deviceContext->vertexPipeline()->setSamplerState(pointSamplerState.get(), 0);
                    deviceContext->vertexPipeline()->setSamplerState(linearClampSamplerState.get(), 1);
                    deviceContext->vertexPipeline()->setSamplerState(linearWrapSamplerState.get(), 2);

                    deviceContext->setPrimitiveType(Video::PrimitiveType::TriangleList);

                    concurrency::parallel_sort(drawCallList.begin(), drawCallList.end(), [](const DrawCallValue &leftValue, const DrawCallValue &rightValue) -> bool
                    {
                        return (leftValue.value < rightValue.value);
                    });

                    ShaderHandle currentShader;
                    std::list<DrawCallSet> drawCallSetList;
                    for (auto &drawCall = drawCallList.begin(); drawCall != drawCallList.end(); )
                    {
                        currentShader = drawCall->shader;

                        auto beginShaderList = drawCall;
                        while (drawCall != drawCallList.end() && drawCall->shader == currentShader)
                        {
                            ++drawCall;
                        };

                        auto endShaderList = drawCall;
                        Engine::Shader *shader = resources->getShader(currentShader);
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
                        bool visualEnabled = false;
                        bool materialEnabled = false;
                        auto &shader = drawCallSet.shader;
                        for (auto block = shader->begin(deviceContext, cameraConstantData.viewMatrix, viewFrustum); block; block = block->next())
                        {
                            while (block->prepare())
                            {
                                for (auto pass = block->begin(); pass; pass = pass->next())
                                {
                                    switch (pass->prepare())
                                    {
                                    case Engine::Shader::Pass::Mode::Forward:
                                        if(true)
                                        {
                                            VisualHandle currentVisual;
                                            MaterialHandle currentMaterial;
                                            for (auto shaderDrawCall = drawCallSet.begin; shaderDrawCall != drawCallSet.end; ++shaderDrawCall)
                                            {
                                                if (currentVisual != shaderDrawCall->plugin)
                                                {
                                                    visualEnabled = false;
                                                    currentVisual = shaderDrawCall->plugin;
                                                    Plugin::Visual *visual = resources->getVisual(currentVisual);
                                                    if (!visual)
                                                    {
                                                        continue;
                                                    }

                                                    visualEnabled = true;
                                                    visual->enable(deviceContext);
                                                }

                                                if (currentMaterial != shaderDrawCall->material)
                                                {
                                                    materialEnabled = false;
                                                    currentMaterial = shaderDrawCall->material;
                                                    Engine::Material *material = resources->getMaterial(currentMaterial);
                                                    if (!material)
                                                    {
                                                        continue;
                                                    }

                                                    materialEnabled = pass->enableMaterial(material);
                                                }

                                                if (visualEnabled && materialEnabled)
                                                {
                                                    try
                                                    {
                                                        shaderDrawCall->onDraw(deviceContext);
                                                    }
                                                    catch (const Plugin::Resources::ResourceNotLoaded &)
                                                    {
                                                    };
                                                }
                                            }
                                        }

                                        break;

                                    case Engine::Shader::Pass::Mode::Deferred:
                                        deviceContext->vertexPipeline()->setProgram(deferredVertexProgram.get());
                                        deviceContext->drawPrimitive(3, 0);
                                        break;

                                    case Engine::Shader::Pass::Mode::Compute:
                                        break;
                                    };

                                    pass->clear();
                                }
                            };
                        }
                    }

                    deviceContext->vertexPipeline()->setProgram(deferredVertexProgram.get());
                    if (cameraEntity->hasComponent<Components::Filter>())
                    {
                        auto &filterList = cameraEntity->getComponent<Components::Filter>().list;
                        for (auto &filterName : filterList)
                        {
                            Engine::Filter * const filter = resources->getFilter(filterName);
                            if (filter)
                            {
                                for (auto pass = filter->begin(deviceContext); pass; pass = pass->next())
                                {
                                    switch (pass->prepare())
                                    {
                                    case Engine::Filter::Pass::Mode::Deferred:
                                        deviceContext->drawPrimitive(3, 0);
                                        break;

                                    case Engine::Filter::Pass::Mode::Compute:
                                        break;
                                    };

                                    pass->clear();
                                }
                            }
                        }
                    }

                    deviceContext->pixelPipeline()->setProgram(deferredPixelProgram.get());
                    resources->setResource(deviceContext->pixelPipeline(), resources->getResourceHandle(L"screen"), 0);
                    auto backBuffer = device->getBackBuffer();
                    if (cameraTarget)
                    {
                        resources->setRenderTargets(deviceContext, &cameraTarget, 1, nullptr);
                    }
                    else
                    {
                        deviceContext->setRenderTargets(&backBuffer, 1, nullptr);
                        deviceContext->setViewports(&backBuffer->getViewPort(), 1);
                    }

                    deviceContext->drawPrimitive(3, 0);

                    deviceContext->geometryPipeline()->setConstantBuffer(nullptr, 0);
                    deviceContext->vertexPipeline()->setConstantBuffer(nullptr, 0);
                    deviceContext->pixelPipeline()->setConstantBuffer(nullptr, 0);
                    deviceContext->computePipeline()->setConstantBuffer(nullptr, 0);

                    deviceContext->geometryPipeline()->setConstantBuffer(nullptr, 1);
                    deviceContext->vertexPipeline()->setConstantBuffer(nullptr, 1);
                    deviceContext->pixelPipeline()->setConstantBuffer(nullptr, 1);
                    deviceContext->computePipeline()->setConstantBuffer(nullptr, 1);
                }
            }

            // Plugin::PopulationListener
            void onLoadBegin(void)
            {
                GEK_REQUIRE(resources);

                resources->clearLocal();
                resources->createTexture(L"screen", Video::Format::R11G11B10_FLOAT, device->getBackBuffer()->getWidth(), device->getBackBuffer()->getHeight(), 1, 1, Video::TextureFlags::RenderTarget | Video::TextureFlags::Resource);
                resources->createTexture(L"screenBuffer", Video::Format::R11G11B10_FLOAT, device->getBackBuffer()->getWidth(), device->getBackBuffer()->getHeight(), 1, 1, Video::TextureFlags::RenderTarget | Video::TextureFlags::Resource);
            }

            void onLoadSucceeded(void)
            {
            }

            void onLoadFailed(void)
            {
            }

            // Plugin::PopulationStep
            void onUpdate(uint32_t order, State state)
            {
                GEK_REQUIRE(device);

                if (order == 10)
                {
                    sendShout(&Plugin::RendererListener::onRenderBackground);
                }
                else if (order == 100)
                {
                    sendShout(&Plugin::RendererListener::onRenderForeground);
                    device->present(false);
                }
            }
        };

        GEK_REGISTER_CONTEXT_USER(Renderer);
    }; // namespace Implementation
}; // namespace Gek
