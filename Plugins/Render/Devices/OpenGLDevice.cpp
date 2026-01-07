#ifdef _WIN32

#include "GEK/Utility/String.hpp"
#include "GEK/Utility/ContextUser.hpp"
#include "GEK/Utility/FileSystem.hpp"
#include "GEK/Utility/Hash.hpp"
#include "GEK/Render/RenderDevice.hpp"
#include "GEK/Render/WindowDevice.hpp"
#include <Windows.h>
#include <GL/gl.h>
#include <glatter/glatter.h>
#include <memory>
#include <vector>
#include <stdexcept>
#include <mutex>
#include <string>
#include <filesystem>
#include <unordered_map>
#include <format>
#include "lodepng.h"

#pragma comment(lib, "opengl32.lib")

namespace Gek
{
    namespace Render::Implementation
    {
        // Minimal WGL helpers to create a modern context and load a few functions
        using PFNWGLCREATECONTEXTATTRIBSARBPROC = HGLRC(WINAPI*)(HDC, HGLRC, const int*);
        static PFNWGLCREATECONTEXTATTRIBSARBPROC wglCreateContextAttribsARB = nullptr;

        // GL function pointers we need (subset)
        #define DECL_GL_FN(type, name) static type name = nullptr
        DECL_GL_FN(PFNGLGENBUFFERSPROC, glGenBuffers);
        DECL_GL_FN(PFNGLBINDBUFFERPROC, glBindBuffer);
        DECL_GL_FN(PFNGLBUFFERDATAPROC, glBufferData);
        DECL_GL_FN(PFNGLMAPBUFFERRANGEPROC, glMapBufferRange);
        DECL_GL_FN(PFNGLUNMAPBUFFERPROC, glUnmapBuffer);
        DECL_GL_FN(PFNGLGENVERTEXARRAYSPROC, glGenVertexArrays);
        DECL_GL_FN(PFNGLBINDVERTEXARRAYPROC, glBindVertexArray);
        DECL_GL_FN(PFNGLCREATESHADERPROC, glCreateShader);
        DECL_GL_FN(PFNGLSHADERSOURCEPROC, glShaderSource);
        DECL_GL_FN(PFNGLCOMPILESHADERPROC, glCompileShader);
        DECL_GL_FN(PFNGLGETSHADERIVPROC, glGetShaderiv);
        DECL_GL_FN(PFNGLGETSHADERINFOLOGPROC, glGetShaderInfoLog);
        DECL_GL_FN(PFNGLCREATEPROGRAMPROC, glCreateProgram);
        DECL_GL_FN(PFNGLATTACHSHADERPROC, glAttachShader);
        DECL_GL_FN(PFNGLLINKPROGRAMPROC, glLinkProgram);
        DECL_GL_FN(PFNGLUSEPROGRAMPROC, glUseProgram);
        DECL_GL_FN(PFNGLDELETEPROGRAMPROC, glDeleteProgram);
        DECL_GL_FN(PFNGLDELETESHADERPROC, glDeleteShader);
        DECL_GL_FN(PFNGLGETPROGRAMIVPROC, glGetProgramiv);
        DECL_GL_FN(PFNGLGETPROGRAMINFOLOGPROC, glGetProgramInfoLog);
        DECL_GL_FN(PFNGLVERTEXATTRIBPOINTERPROC, glVertexAttribPointer);
        DECL_GL_FN(PFNGLENABLEVERTEXATTRIBARRAYPROC, glEnableVertexAttribArray);
        DECL_GL_FN(PFNGLDISABLEVERTEXATTRIBARRAYPROC, glDisableVertexAttribArray);
        DECL_GL_FN(PFNGLDRAWARRAYSINSTANCEDPROC, glDrawArraysInstanced);
        DECL_GL_FN(PFNGLDRAWELEMENTSINSTANCEDPROC, glDrawElementsInstanced);

        static void LoadGLFunctionPointers(void)
        {
            // wglGetProcAddress returns function pointers for modern GL functions
            auto load = [](const char* name)->void*
            {
                void* p = (void*)wglGetProcAddress(name);
                if (!p)
                {
                    // try opengl32.dll
                    static HMODULE module = LoadLibraryA("opengl32.dll");
                    if (module)
                        p = (void*)GetProcAddress(module, name);
                }
                return p;
            };

            wglCreateContextAttribsARB = (PFNWGLCREATECONTEXTATTRIBSARBPROC)load("wglCreateContextAttribsARB");

            // load a few GL fns
            glGenVertexArrays = (PFNGLGENVERTEXARRAYSPROC)load("glGenVertexArrays");
            glBindVertexArray = (PFNGLBINDVERTEXARRAYPROC)load("glBindVertexArray");
            glCreateShader = (PFNGLCREATESHADERPROC)load("glCreateShader");
            glShaderSource = (PFNGLSHADERSOURCEPROC)load("glShaderSource");
            glCompileShader = (PFNGLCOMPILESHADERPROC)load("glCompileShader");
            glGetShaderiv = (PFNGLGETSHADERIVPROC)load("glGetShaderiv");
            glGetShaderInfoLog = (PFNGLGETSHADERINFOLOGPROC)load("glGetShaderInfoLog");
            glCreateProgram = (PFNGLCREATEPROGRAMPROC)load("glCreateProgram");
            glAttachShader = (PFNGLATTACHSHADERPROC)load("glAttachShader");
            glLinkProgram = (PFNGLLINKPROGRAMPROC)load("glLinkProgram");
            glUseProgram = (PFNGLUSEPROGRAMPROC)load("glUseProgram");
            glDeleteProgram = (PFNGLDELETEPROGRAMPROC)load("glDeleteProgram");
            glDeleteShader = (PFNGLDELETESHADERPROC)load("glDeleteShader");

            glGetProgramiv = (PFNGLGETPROGRAMIVPROC)load("glGetProgramiv");
            glGetProgramInfoLog = (PFNGLGETPROGRAMINFOLOGPROC)load("glGetProgramInfoLog");

            glGenBuffers = (PFNGLGENBUFFERSPROC)load("glGenBuffers");
            glBindBuffer = (PFNGLBINDBUFFERPROC)load("glBindBuffer");
            glBufferData = (PFNGLBUFFERDATAPROC)load("glBufferData");
            glMapBufferRange = (PFNGLMAPBUFFERRANGEPROC)load("glMapBufferRange");
            glUnmapBuffer = (PFNGLUNMAPBUFFERPROC)load("glUnmapBuffer");

            glVertexAttribPointer = (PFNGLVERTEXATTRIBPOINTERPROC)load("glVertexAttribPointer");
            glEnableVertexAttribArray = (PFNGLENABLEVERTEXATTRIBARRAYPROC)load("glEnableVertexAttribArray");
            glDisableVertexAttribArray = (PFNGLDISABLEVERTEXATTRIBARRAYPROC)load("glDisableVertexAttribArray");

            glDrawArraysInstanced = (PFNGLDRAWARRAYSINSTANCEDPROC)load("glDrawArraysInstanced");
            glDrawElementsInstanced = (PFNGLDRAWELEMENTSINSTANCEDPROC)load("glDrawElementsInstanced");
        }

        // Helpers: format info and shader compile/link
        struct FormatInfo { GLint components; GLenum format; GLenum type; GLboolean normalized; };
        static FormatInfo GetFormatInfo(Render::Format f)
        {
            switch (f)
            {
            case Render::Format::R8G8B8A8_UNORM: return {4, GL_RGBA, GL_UNSIGNED_BYTE, GL_TRUE};
            case Render::Format::R32G32B32A32_FLOAT: return {4, GL_RGBA, GL_FLOAT, GL_FALSE};
            case Render::Format::R32G32B32_FLOAT: return {3, GL_RGB, GL_FLOAT, GL_FALSE};
            case Render::Format::R32G32_FLOAT: return {2, GL_RG, GL_FLOAT, GL_FALSE};
            case Render::Format::R32_FLOAT: return {1, GL_RED, GL_FLOAT, GL_FALSE};
            default: return {4, GL_RGBA, GL_UNSIGNED_BYTE, GL_TRUE};
            }
        }

        static GLuint CompileGLShader(GLenum type, const std::string &source)
        {
            if (!glCreateShader) return 0;
            GLuint shader = glCreateShader(type);
            const char* src = source.c_str();
            glShaderSource(shader, 1, &src, nullptr);
            glCompileShader(shader);
            GLint compiled = 0;
            glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
            if (!compiled)
            {
                GLint logLen = 0;
                glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLen);
                if (logLen > 0)
                {
                    std::string log(logLen, '\0');
                    glGetShaderInfoLog(shader, logLen, nullptr, log.data());
                    std::string msg = "GLSL shader compile failed: ";
                    msg += log;
                    OutputDebugStringA(msg.c_str());
                }
                glDeleteShader(shader);
                return 0;
            }
            return shader;
        }

        static GLuint LinkGLProgram(GLuint vs, GLuint fs)
        {
            if (!glCreateProgram) return 0;
            GLuint program = glCreateProgram();
            if (vs) glAttachShader(program, vs);
            if (fs) glAttachShader(program, fs);
            glLinkProgram(program);
            GLint linked = 0;
            glGetProgramiv(program, GL_LINK_STATUS, &linked);
            if (!linked)
            {
                GLint logLen = 0;
                glGetProgramiv(program, GL_INFO_LOG_LENGTH, &logLen);
                if (logLen > 0)
                {
                    std::string log(logLen, '\0');
                    glGetProgramInfoLog(program, logLen, nullptr, log.data());
                    std::string msg = "GLSL program link failed: ";
                    msg += log;
                    OutputDebugStringA(msg.c_str());
                }
                glDeleteProgram(program);
                return 0;
            }
            if (vs) glDetachShader(program, vs);
            if (fs) glDetachShader(program, fs);
            return program;
        }

        static GLenum GetGLPrimitive(Render::PrimitiveType t)
        {
            switch (t)
            {
            case Render::PrimitiveType::PointList: return GL_POINTS;
            case Render::PrimitiveType::LineList: return GL_LINES;
            case Render::PrimitiveType::LineStrip: return GL_LINE_STRIP;
            case Render::PrimitiveType::TriangleList: return GL_TRIANGLES;
            case Render::PrimitiveType::TriangleStrip: return GL_TRIANGLE_STRIP;
            default: return GL_TRIANGLES;
            }
        }

        // Forward declarations
        class Device;
        class Buffer;
        class Texture;
        class CommandList;
        class Queue;

        class Buffer : public Render::Buffer
        {
        public:
            Render::Buffer::Description description;
            GLuint buffer = 0;

            Buffer(const Render::Buffer::Description &desc, GLuint b)
                : description(desc), buffer(b) {}

            virtual ~Buffer(void)
            {
                if (buffer)
                    glDeleteBuffers(1, &buffer);
            }

            std::string_view getName(void) const override { return description.name; }
            Render::Buffer::Description const& getDescription(void) const override { return description; }
        };

        class Texture : public Render::Texture
        {
        public:
            Render::Texture::Description description;
            GLuint tex = 0;
            GLuint fbo = 0;
            GLuint depthRbo = 0;

            Texture(const Render::Texture::Description &desc, GLuint t)
                : description(desc), tex(t) {}

            virtual ~Texture(void)
            {
                if (depthRbo) glDeleteRenderbuffers(1, &depthRbo);
                if (fbo) glDeleteFramebuffers(1, &fbo);
                if (tex) glDeleteTextures(1, &tex);
            }

            std::string_view getName(void) const override { return description.name; }
            Render::Texture::Description const& getDescription(void) const override { return description; }
        };

        class PipelineState : public Render::PipelineState
        {
        public:
            Render::PipelineState::Description description;
            GLuint program = 0;
            GLenum primitive = GL_TRIANGLES;

        public:
            PipelineState(const Render::PipelineState::Description &desc, GLuint program)
                : description(desc)
                , program(program)
                , primitive(GetGLPrimitive(desc.primitiveType))
            {
            }

            virtual ~PipelineState(void)
            {
                if (program)
                    glDeleteProgram(program);
            }

            std::string_view getName(void) const override
            {
                return description.name;
            }

            Render::PipelineState::Description const& getDescription(void) const override
            {
                return description;
            }
        };

        class CommandList : public Render::CommandList
        {
        public:
            Window::Device *window = nullptr;
            PipelineState *currentPipeline = nullptr;
            GLuint vao = 0;
            std::vector<Buffer*> vBuffers;
            Buffer* iBuffer = nullptr;
            GLenum indexType = GL_UNSIGNED_INT;
            GLenum currentTopology = GL_TRIANGLES;

            CommandList(Window::Device *w) : window(w)
            {
                if (glGenVertexArrays) glGenVertexArrays(1, &vao);
            }

            virtual ~CommandList(void)
            {
                if (vao) glDeleteVertexArrays(1, &vao);
            }

            std::string_view getName(void) const override { return "OpenGL CommandList"; }

            void finish(void) override {}
            void generateMipMaps(Render::Resource* texture) override {}
            void resolveSamples(Render::Resource* destination, Render::Resource* source) override {}
            void clearUnorderedAccess(Render::Resource* object, Math::Float4 const& value) override {}
            void clearUnorderedAccess(Render::Resource* object, Math::UInt4 const& value) override {}
            void clearRenderTarget(Render::Texture* renderTarget, Math::Float4 const& clearColor) override
            {
                glClearColor(clearColor.r, clearColor.g, clearColor.b, clearColor.a);
                glClear(GL_COLOR_BUFFER_BIT);
            }
            void clearDepthStencilTarget(Render::Texture* depthBuffer, uint32_t flags, float clearDepth, uint32_t clearStencil) override {}
            void setViewportList(const std::vector<Render::ViewPort>&) override {}
            void setScissorList(const std::vector<Math::UInt4>&) override {}

            static uint32_t SemanticToLocation(Render::PipelineState::ElementDeclaration::Semantic s)
            {
                switch (s)
                {
                case Render::PipelineState::ElementDeclaration::Semantic::Position: return 0;
                case Render::PipelineState::ElementDeclaration::Semantic::TexCoord: return 1;
                case Render::PipelineState::ElementDeclaration::Semantic::Tangent: return 2;
                case Render::PipelineState::ElementDeclaration::Semantic::BiTangent: return 3;
                case Render::PipelineState::ElementDeclaration::Semantic::Normal: return 4;
                case Render::PipelineState::ElementDeclaration::Semantic::Color: return 5;
                default: return 0;
                }
            }

            void bindPipelineState(Render::PipelineState* pipelineState) override
            {
                currentPipeline = dynamic_cast<PipelineState*>(pipelineState);
                if (currentPipeline && glUseProgram)
                {
                    glUseProgram(currentPipeline->program);
                    currentTopology = currentPipeline->primitive;
                }
            }

            void bindSamplerStateList(const std::vector<Render::SamplerState*>&, uint32_t, uint8_t) override {}
            void bindConstantBufferList(const std::vector<Render::Buffer*>&, uint32_t, uint8_t) override {}

            void bindResourceList(const std::vector<Render::Resource*>&, uint32_t, uint8_t) override {}
            void bindUnorderedAccessList(const std::vector<Render::Resource*>&, uint32_t, uint32_t*) override {}

            void bindIndexBuffer(Render::Resource* indexBuffer, uint32_t offset) override
            {
                (void)offset;
                iBuffer = dynamic_cast<Buffer*>(indexBuffer);
                if (iBuffer && glBindBuffer) glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, iBuffer->buffer);

                // infer index type from format if possible
                if (iBuffer)
                {
                    switch (iBuffer->description.format)
                    {
                    case Render::Format::R16_UINT: indexType = GL_UNSIGNED_SHORT; break;
                    default: indexType = GL_UNSIGNED_INT; break;
                    }
                }
            }

            void bindVertexBufferList(const std::vector<Render::Resource*>& vertexBufferList, uint32_t firstSlot, uint32_t *offsetList) override
            {
                (void)firstSlot;
                if (vertexBufferList.empty()) return;
                vBuffers.clear();
                for (auto const &v : vertexBufferList)
                {
                    auto b = dynamic_cast<Buffer*>(v);
                    vBuffers.push_back(b);
                }

                if (!currentPipeline) return;

                // bind VAO and configure attributes
                if (glBindVertexArray) glBindVertexArray(vao);
                Buffer* vertexBuffer = vBuffers.size() ? vBuffers[0] : nullptr;
                if (!vertexBuffer) return;
                if (glBindBuffer) glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer->buffer);

                uint32_t stride = vertexBuffer->description.stride;
                uint32_t currentOffset = 0;
                for (auto const &elem : currentPipeline->description.vertexDeclaration)
                {
                    uint32_t location = SemanticToLocation(elem.semantic);
                    uint32_t aligned = elem.alignedByteOffset;
                    if (aligned == Render::PipelineState::VertexDeclaration::AppendAligned) aligned = currentOffset;

                    auto fmt = GetFormatInfo(elem.format);
                    if (glEnableVertexAttribArray) glEnableVertexAttribArray(location);
                    if (glVertexAttribPointer) glVertexAttribPointer(location, fmt.components, fmt.type, fmt.normalized, stride, (const void*)(aligned + (offsetList ? offsetList[0] : 0)));

                    uint32_t sizeBytes = fmt.components * (fmt.type == GL_FLOAT ? 4 : 1);
                    currentOffset = aligned + sizeBytes;
                }
            }

            void bindRenderTargetList(const std::vector<Render::Texture*> &renderTargetList, Render::Texture* depthBuffer) override
            {
                if (renderTargetList.empty())
                {
                    glBindFramebuffer(GL_FRAMEBUFFER, 0);
                    // reset viewport to window size if possible
                    if (window)
                    {
                        auto rect = window->getClientRectangle();
                        glViewport(rect.x, rect.y, rect.z, rect.w);
                    }
                    return;
                }

                auto rt = dynamic_cast<Texture*>(renderTargetList[0]);
                if (rt && rt->fbo)
                {
                    glBindFramebuffer(GL_FRAMEBUFFER, rt->fbo);
                    glViewport(0, 0, rt->description.width, rt->description.height);
                }
                else
                {
                    glBindFramebuffer(GL_FRAMEBUFFER, 0);
                }
            }

            void drawInstancedPrimitive(uint32_t instanceCount, uint32_t firstInstance, uint32_t vertexCount, uint32_t firstVertex) override
            {
                (void)firstInstance;
                if (glBindVertexArray) glBindVertexArray(vao);
                if (glDrawArraysInstanced) glDrawArraysInstanced(currentTopology, firstVertex, vertexCount, instanceCount);
            }

            void drawInstancedIndexedPrimitive(uint32_t instanceCount, uint32_t firstInstance, uint32_t indexCount, uint32_t firstIndex, uint32_t firstVertex) override
            {
                (void)firstInstance; (void)firstVertex;
                if (glBindVertexArray) glBindVertexArray(vao);
                if (iBuffer && glDrawElementsInstanced) glDrawElementsInstanced(currentTopology, indexCount, indexType, (const void*)(uintptr_t)(firstIndex * (indexType == GL_UNSIGNED_SHORT ? 2 : 4)), instanceCount);
            }

            void dispatch(uint32_t, uint32_t, uint32_t) override {}
            void drawInstancedPrimitive(Render::Resource*) override {}
            void drawInstancedIndexedPrimitive(Render::Resource*) override {}
            void dispatch(Render::Resource*) override {}
        };

        class Queue : public Render::Queue
        {
        public:
            Device* device = nullptr;
            Queue(Device* d) : device(d) {}
            virtual ~Queue(void) {}
            std::string_view getName(void) const override { return "OpenGL Queue"; }
            void executeCommandList(Render::CommandList* commandList) override {}
        };

        class Device : public Render::Device
        {
        public:
            Window::Device *window = nullptr;
            HGLRC glContext = nullptr;
            HDC hdc = nullptr;
            std::mutex backBufferMutex;
            std::unique_ptr<Texture> backBuffer;

            Device(Gek::Context *context, Window::Device *window, Render::Device::Description deviceDescription)
                : window(window)
            {
                assert(window);
                HWND hwnd = (HWND)window->getBaseWindow();
                hdc = GetDC(hwnd);
                if (!hdc)
                    throw std::runtime_error("Unable to get window DC");

                PIXELFORMATDESCRIPTOR pfd = {};
                pfd.nSize = sizeof(pfd);
                pfd.nVersion = 1;
                pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
                pfd.iPixelType = PFD_TYPE_RGBA;
                pfd.cColorBits = 32;
                pfd.cDepthBits = 24;
                pfd.cStencilBits = 8;
                pfd.iLayerType = PFD_MAIN_PLANE;

                int pixelFormat = ChoosePixelFormat(hdc, &pfd);
                if (pixelFormat == 0 || !SetPixelFormat(hdc, pixelFormat, &pfd))
                    throw std::runtime_error("Unable to set pixel format");

                // create temporary context to load extensions
                HGLRC tempContext = wglCreateContext(hdc);
                if (!tempContext)
                    throw std::runtime_error("Unable to create temporary GL context");

                if (!wglMakeCurrent(hdc, tempContext))
                {
                    wglDeleteContext(tempContext);
                    throw std::runtime_error("Unable to make temporary GL context current");
                }

                // load wglCreateContextAttribsARB
                wglCreateContextAttribsARB = (PFNWGLCREATECONTEXTATTRIBSARBPROC)wglGetProcAddress("wglCreateContextAttribsARB");

                // Try to create modern context if available
                if (wglCreateContextAttribsARB)
                {
                    const int attribs[] = {
                        0x2091, 3, // WGL_CONTEXT_MAJOR_VERSION_ARB
                        0x2092, 3, // WGL_CONTEXT_MINOR_VERSION_ARB
                        0x9126, 0, // WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB=0x00000002, WGL_CONTEXT_CORE_PROFILE_BIT_ARB=0x00000001
                        0
                    };
                    glContext = wglCreateContextAttribsARB(hdc, 0, attribs);
                }

                if (!glContext)
                {
                    // fallback to temp context
                    glContext = tempContext;
                }
                else
                {
                    // replace temp context with new one
                    wglMakeCurrent(nullptr, nullptr);
                    wglDeleteContext(tempContext);
                    if (!wglMakeCurrent(hdc, glContext))
                        throw std::runtime_error("Unable to make GL context current");
                }

                // load the GL function pointers we need
                LoadGLFunctionPointers();

                // Debug: report that context is created
                OutputDebugStringA("OpenGL Device: context created\n");
            }

            ~Device(void)
            {
                if (glContext)
                {
                    wglMakeCurrent(nullptr, nullptr);
                    wglDeleteContext(glContext);
                }
                if (hdc && window)
                {
                    ReleaseDC((HWND)window->getBaseWindow(), hdc);
                    hdc = nullptr;
                }
            }

            Render::DisplayModeList getDisplayModeList(Render::Format) const override
            {
                return {};
            }

            Render::Texture* getBackBuffer(void) override
            {
                std::lock_guard<std::mutex> lock(backBufferMutex);
                if (!backBuffer)
                {
                    Render::Texture::Description desc;
                    desc.name = "BackBuffer";
                    desc.width = 1;
                    desc.height = 1;
                    desc.depth = 1;
                    desc.format = Render::Format::R8G8B8A8_UNORM;
                    desc.mipMapCount = 1;
                    desc.flags = 0;

                    // default framebuffer has no GL texture, keep tex=0
                    backBuffer = std::make_unique<Texture>(desc, 0);
                }
                return backBuffer.get();
            }

            void setFullScreenState(bool) override {}
            void setDisplayMode(const Render::DisplayMode&) override {}
            void handleResize(void) override {}

            class PipelineFormatImpl : public Render::PipelineFormat
            {
            public:
                Render::PipelineFormat::Description description;

                PipelineFormatImpl(const Render::PipelineFormat::Description &d)
                    : description(d)
                {
                }

                ~PipelineFormatImpl(void) {}

                Description const& getDescription(void) const override { return description; }
            };

            Render::PipelineFormatPtr createPipelineFormat(const Render::PipelineFormat::Description &pipelineDescription) override
            {
                return std::make_unique<PipelineFormatImpl>(pipelineDescription);
            }

            Render::Program::Information compileProgram(Render::Program::Type type, std::string_view name, FileSystem::Path const &debugPath, std::string_view uncompiledProgram, std::string_view entryFunction, std::function<bool(Render::IncludeType, std::string_view, void const **data, uint32_t *size)> &&onInclude) override
            {
                Render::Program::Information information;
                information.type = type;
                information.name = std::format("{}:{}", name, entryFunction);
                information.uncompiledData = uncompiledProgram;

                if (uncompiledProgram.empty() || entryFunction.empty())
                    return information;

                // Map program type to DXC profile
                static const std::unordered_map<Render::Program::Type, std::string_view> ProfileMap = {
                    { Render::Program::Type::Compute, "cs_6_0" },
                    { Render::Program::Type::Geometry, "gs_6_0" },
                    { Render::Program::Type::Vertex, "vs_6_0" },
                    { Render::Program::Type::Pixel, "ps_6_0" },
                };

                auto profileIt = ProfileMap.find(type);
                if (profileIt == ProfileMap.end())
                    return information;

                // Create temporary directory for shader files
                std::hash<std::string> hasher;
                size_t h = hasher(std::string(name) + std::string(entryFunction));
                auto tmpDir = std::filesystem::temp_directory_path() / std::format("gek_shader_{}", h);
                std::error_code ec;
                std::filesystem::create_directories(tmpDir, ec);

                auto inPath = FileSystem::Path(tmpDir / "main.hlsl");
                auto spvPath = FileSystem::Path(tmpDir / "out.spv");
                auto outPath = FileSystem::Path(tmpDir / "out.glsl");

                // Save main HLSL
                std::vector<uint8_t> tmpBuf(uncompiledProgram.begin(), uncompiledProgram.end());
                FileSystem::Save(inPath, tmpBuf);

                // Try to resolve simple #include "..." by asking the callback and writing includes into tmpDir
                std::string s(uncompiledProgram);
                size_t pos = 0;
                while ((pos = s.find("#include", pos)) != std::string::npos)
                {
                    auto start = s.find_first_of('"', pos);
                    if (start == std::string::npos) { pos += 8; continue; }
                    auto end = s.find_first_of('"', start + 1);
                    if (end == std::string::npos) { pos += 8; continue; }
                    std::string includeName = s.substr(start + 1, end - start - 1);
                    const void *data = nullptr; uint32_t size = 0;
                    if (onInclude && onInclude(Render::IncludeType::Global, includeName, &data, &size))
                    {
                        std::vector<uint8_t> incBuf((uint8_t const*)data, (uint8_t const*)data + size);
                        auto incPath = FileSystem::Path(tmpDir / includeName);
                        FileSystem::Save(incPath, incBuf);
                    }
                    pos = end + 1;
                }

                // Build commands
                std::string dxcCmd = std::format("dxc -T {} -E {} -spirv -Fo \"{}\" \"{}\"", profileIt->second, std::string(entryFunction), spvPath.getString(), inPath.getString());
                int r = std::system(dxcCmd.c_str());
                if (r != 0)
                {
                    std::string msg = "OpenGL: dxc failed for " + std::string(name) + "\n";
                    OutputDebugStringA(msg.c_str());
                    return information;
                }

                std::string spirvCrossCmd = std::format("spirv-cross \"{}\" --version 330 --output \"{}\"", spvPath.getString(), outPath.getString());
                r = std::system(spirvCrossCmd.c_str());
                if (r != 0)
                {
                    std::string msg = "OpenGL: spirv-cross failed for " + std::string(name) + "\n";
                    OutputDebugStringA(msg.c_str());
                    return information;
                }

                // Read generated GLSL
                std::string glsl = FileSystem::Read(outPath);
                information.compiledData.assign(glsl.begin(), glsl.end());

                // cleanup (best-effort)
                std::filesystem::remove(inPath.data, ec);
                std::filesystem::remove(spvPath.data, ec);
                std::filesystem::remove(outPath.data, ec);
                std::filesystem::remove(tmpDir, ec);

                return information;
            }

            Render::ProgramPtr createProgram(Render::Program::Information const &information) override
            {
                class Program : public Render::Program
                {
                public:
                    Render::Program::Information information;
                    Program(Render::Program::Information const &info) : information(info) {}
                    std::string_view getName(void) const override { return information.name; }
                    Render::Program::Information const& getInformation(void) const override { return information; }
                };

                return std::make_unique<Program>(information);
            }

            Render::PipelineStatePtr createPipelineState(Render::PipelineFormat*, const Render::PipelineState::Description &pipelineStateDescription) override
            {
                // If the pipeline description has HLSL-style data, try to convert it to GLSL first using compileProgram
                Render::Program::Information vsInfo = compileProgram(Render::Program::Type::Vertex, pipelineStateDescription.name, FileSystem::Path(), pipelineStateDescription.vertexShader, pipelineStateDescription.vertexShaderEntryFunction, nullptr);
                Render::Program::Information psInfo = compileProgram(Render::Program::Type::Pixel, pipelineStateDescription.name, FileSystem::Path(), pipelineStateDescription.pixelShader, pipelineStateDescription.pixelShaderEntryFunction, nullptr);

                std::string vsSource;
                std::string fsSource;

                auto ensureVersion = [](const std::string &s){
                    if (s.find("#version") == std::string::npos)
                        return std::string("#version 330 core\n") + s;
                    return s;
                };

                if (!vsInfo.compiledData.empty()) vsSource = ensureVersion(std::string(vsInfo.compiledData.begin(), vsInfo.compiledData.end()));
                if (!psInfo.compiledData.empty()) fsSource = ensureVersion(std::string(psInfo.compiledData.begin(), psInfo.compiledData.end()));

                if (vsSource.empty() && !pipelineStateDescription.vertexShader.empty()) vsSource = ensureVersion(pipelineStateDescription.vertexShader);
                if (fsSource.empty() && !pipelineStateDescription.pixelShader.empty()) fsSource = ensureVersion(pipelineStateDescription.pixelShader);

                const char *defaultVS = "#version 330 core\nlayout(location=0) in vec3 POSITION; layout(location=1) in vec2 TEXCOORD; out vec2 vTex; void main(){ vTex = TEXCOORD; gl_Position = vec4(POSITION,1.0); }";
                const char *defaultFS = "#version 330 core\nin vec2 vTex; out vec4 outColor; uniform sampler2D s0; void main(){ outColor = texture(s0, vTex); }";

                if (vsSource.empty()) vsSource = defaultVS;
                if (fsSource.empty()) fsSource = defaultFS;

                GLuint vs = CompileGLShader(GL_VERTEX_SHADER, vsSource);
                GLuint fs = CompileGLShader(GL_FRAGMENT_SHADER, fsSource);
                GLuint program = 0;
                if (vs || fs) program = LinkGLProgram(vs, fs);

                if (vs) glDeleteShader(vs);
                if (fs) glDeleteShader(fs);

                if (!program)
                {
                    OutputDebugStringA("OpenGL: shader program creation failed, using fixed-function-like fallback\n");
                    // Create a trivial passthrough program
                    GLuint v = CompileGLShader(GL_VERTEX_SHADER, defaultVS);
                    GLuint f = CompileGLShader(GL_FRAGMENT_SHADER, defaultFS);
                    program = LinkGLProgram(v, f);
                    if (v) glDeleteShader(v);
                    if (f) glDeleteShader(f);
                }

                if (!program)
                    throw std::runtime_error("Unable to create GL shader program");

                {
                    std::string msg = "OpenGL: created program for "; msg += pipelineStateDescription.name; msg += "\n"; OutputDebugStringA(msg.c_str());
                }

                return std::make_unique<PipelineState>(pipelineStateDescription, program);
            }

            Render::SamplerStatePtr createSamplerState(Render::PipelineFormat*, const Render::SamplerState::Description &samplerDescription) override
            {
                return nullptr;
            }

            bool mapResource(Render::Resource* resource, void*& data, Render::Map mapping) override
            {
                auto internalBuffer = dynamic_cast<Buffer*>(resource);
                if (!internalBuffer) return false;
                if (!glBindBuffer) return false;
                glBindBuffer(GL_ARRAY_BUFFER, internalBuffer->buffer);
                // simple full mapping
                data = glMapBufferRange(GL_ARRAY_BUFFER, 0, internalBuffer->description.stride * internalBuffer->description.count, GL_MAP_WRITE_BIT | GL_MAP_READ_BIT);
                return data != nullptr;
            }

            void unmapResource(Render::Resource* resource) override
            {
                auto internalBuffer = dynamic_cast<Buffer*>(resource);
                if (!internalBuffer) return;
                glBindBuffer(GL_ARRAY_BUFFER, internalBuffer->buffer);
                glUnmapBuffer(GL_ARRAY_BUFFER);
            }

            void updateResource(Render::Resource* resource, const void* data) override
            {
                auto internalBuffer = dynamic_cast<Buffer*>(resource);
                if (!internalBuffer) return;
                glBindBuffer(GL_ARRAY_BUFFER, internalBuffer->buffer);
                glBufferData(GL_ARRAY_BUFFER, internalBuffer->description.stride * internalBuffer->description.count, data, GL_STATIC_DRAW);
            }

            void copyResource(Render::Resource* destination, Render::Resource* source) override
            {
                (void)destination; (void)source;
            }

            Render::BufferPtr createBuffer(const Render::Buffer::Description &description, const void *data) override
            {
                GLuint b = 0;
                glGenBuffers(1, &b);
                GLenum target = GL_ARRAY_BUFFER;
                switch (description.type)
                {
                case Render::Buffer::Type::Index: target = GL_ELEMENT_ARRAY_BUFFER; break;
                case Render::Buffer::Type::Vertex: target = GL_ARRAY_BUFFER; break;
                case Render::Buffer::Type::Constant: target = GL_ARRAY_BUFFER; break;
                default: target = GL_ARRAY_BUFFER; break;
                }
                glBindBuffer(target, b);
                glBufferData(target, description.stride * description.count, data, GL_STATIC_DRAW);
                return std::make_unique<Buffer>(description, b);
            }

            Render::TexturePtr createTexture(const Render::Texture::Description &description, const void *data) override
            {
                GLuint tex = 0;
                glGenTextures(1, &tex);
                glBindTexture(GL_TEXTURE_2D, tex);

                GLenum internalFormat = GL_RGBA8;
                GLenum format = GL_RGBA;
                GLenum type = GL_UNSIGNED_BYTE;

                switch (description.format)
                {
                case Render::Format::R8G8B8A8_UNORM: internalFormat = GL_RGBA8; format = GL_RGBA; type = GL_UNSIGNED_BYTE; break;
                case Render::Format::R32G32B32A32_FLOAT: internalFormat = GL_RGBA32F; format = GL_RGBA; type = GL_FLOAT; break;
                default: internalFormat = GL_RGBA8; format = GL_RGBA; type = GL_UNSIGNED_BYTE; break;
                }

                glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, description.width, description.height, 0, format, type, data);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                glGenerateMipmap(GL_TEXTURE_2D);

                auto texture = std::make_unique<Texture>(description, tex);

                // Create FBO if requested
                if (description.flags & Render::Texture::Flags::RenderTarget || description.flags & Render::Texture::Flags::DepthTarget)
                {
                    glGenFramebuffers(1, &texture->fbo);
                    glBindFramebuffer(GL_FRAMEBUFFER, texture->fbo);

                    if (description.flags & Render::Texture::Flags::RenderTarget)
                    {
                        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex, 0);
                        GLenum drawBuf = GL_COLOR_ATTACHMENT0;
                        glDrawBuffers(1, &drawBuf);
                    }

                    if (description.flags & Render::Texture::Flags::DepthTarget)
                    {
                        glGenRenderbuffers(1, &texture->depthRbo);
                        glBindRenderbuffer(GL_RENDERBUFFER, texture->depthRbo);
                        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, description.width, description.height);
                        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, texture->depthRbo);
                    }

                    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
                    if (status != GL_FRAMEBUFFER_COMPLETE)
                    {
                        OutputDebugStringA("OpenGL: framebuffer incomplete\n");
                    }

                    // unbind
                    glBindFramebuffer(GL_FRAMEBUFFER, 0);
                }

                return texture;
            }

            Render::TexturePtr loadTexture(FileSystem::Path const& filePath, uint32_t flags) override
            {
                auto buffer = FileSystem::Load(filePath);
                std::string extension = String::GetLower(filePath.getExtension());

                std::vector<unsigned char> decoded;
                unsigned width = 0, height = 0;
                if (extension == ".png")
                {
                    unsigned error = lodepng::decode(decoded, width, height, buffer);
                    if (error)
                    {
                        std::string msg = "lodepng decode failed: "; msg += std::to_string(error); msg += "\n";
                        OutputDebugStringA(msg.c_str());
                        // fallback 1x1 white
                        Render::Texture::Description description;
                        description.name = filePath.getString();
                        description.width = 1; description.height = 1; description.depth = 1;
                        description.format = Render::Format::R8G8B8A8_UNORM;
                        description.mipMapCount = 1;
                        uint32_t whitePixel = 0xFFFFFFFF;
                        return createTexture(description, &whitePixel);
                    }
                }
                else
                {
                    // not a png — fallback to 1x1
                    Render::Texture::Description description;
                    description.name = filePath.getString();
                    description.width = 1; description.height = 1; description.depth = 1;
                    description.format = Render::Format::R8G8B8A8_UNORM;
                    description.mipMapCount = 1;
                    uint32_t whitePixel = 0xFFFFFFFF;
                    return createTexture(description, &whitePixel);
                }

                Render::Texture::Description description;
                description.name = filePath.getString();
                description.width = width;
                description.height = height;
                description.depth = 1;
                description.format = Render::Format::R8G8B8A8_UNORM;
                description.mipMapCount = 1;

                GLuint tex = 0;
                glGenTextures(1, &tex);
                glBindTexture(GL_TEXTURE_2D, tex);
                GLenum internalFormat = (flags & Render::TextureLoadFlags::sRGB) ? GL_SRGB8_ALPHA8 : GL_RGBA8;
                glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, decoded.data());
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                glGenerateMipmap(GL_TEXTURE_2D);

                return std::make_unique<Texture>(description, tex);
            }

            Render::QueuePtr createQueue(Render::Queue::Type type) override
            {
                return std::make_unique<Queue>(this);
            }

            Render::CommandListPtr createCommandList(uint32_t flags) override
            {
                return std::make_unique<CommandList>(this->window);
            }

            void present(bool waitForVerticalSync) override
            {
                // glFlush/glFinish then swap
                (void)waitForVerticalSync;
                SwapBuffers(hdc);
            }
        };

        GEK_REGISTER_CONTEXT_USER(Device);
    }; // namespace Render::Implementation
}; // namespace Gek

#endif // _WIN32
