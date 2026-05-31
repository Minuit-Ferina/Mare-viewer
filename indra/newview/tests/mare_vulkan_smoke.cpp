#include "linden_common.h"

#include "llglslshader.h"
#include "llrender.h"
#include "llrender2dutils.h"
#include "llrenderbackend.h"
#include "llrenderstate.h"
#include "llrendertarget.h"
#include "llvertexbuffer.h"

#include "mare_vulkan_smoke_macosx.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <limits>
#include <string>
#include <thread>
#include <vector>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace
{
enum class SmokeMode
{
    DirectClear,
    OffscreenCopy,
    DeferredComposite,
    DeferredGraph,
    ViewerDeferredDirect,
    ViewerRenderTargetDirect,
    ViewerImmediateDirect,
    WorldPipelines,
};

struct SmokeOptions
{
    SmokeMode mMode = SmokeMode::DirectClear;
    int mFrameLimit = 0;
    int mLogInterval = 60;
    int mReadbackFrameLimit = 4;
    bool mStdoutReadback = true;
    bool mBufferReadback = true;
    bool mRenderUI = false;
    bool mRenderViewerUISequence = false;
    bool mHelp = false;
    std::string mVulkanSDK;
};

class SmokeMatrixScope
{
public:
    SmokeMatrixScope()
        : mMatrixMode(gGL.getMatrixMode()),
          mProjection(gGL.getProjectionMatrix()),
          mModelview(gGL.getModelviewMatrix())
    {
    }

    ~SmokeMatrixScope()
    {
        set_current_projection(mProjection);
        set_current_modelview(mModelview);
        gGL.matrixMode(LLRender::MM_PROJECTION);
        gGL.loadMatrix(glm::value_ptr(mProjection));
        gGL.matrixMode(LLRender::MM_MODELVIEW);
        gGL.loadMatrix(glm::value_ptr(mModelview));
        gGL.matrixMode(mMatrixMode);
    }

private:
    LLRender::eMatrixMode mMatrixMode;
    glm::mat4 mProjection;
    glm::mat4 mModelview;
};

bool parse_smoke_mode_value(const char* value, SmokeMode& mode)
{
    if (!value)
    {
        return false;
    }
    if (std::strcmp(value, "direct-clear") == 0)
    {
        mode = SmokeMode::DirectClear;
        return true;
    }
    if (std::strcmp(value, "offscreen-copy") == 0)
    {
        mode = SmokeMode::OffscreenCopy;
        return true;
    }
    if (std::strcmp(value, "deferred-composite") == 0)
    {
        mode = SmokeMode::DeferredComposite;
        return true;
    }
    if (std::strcmp(value, "deferred-graph") == 0)
    {
        mode = SmokeMode::DeferredGraph;
        return true;
    }
    if (std::strcmp(value, "viewer-deferred-direct") == 0)
    {
        mode = SmokeMode::ViewerDeferredDirect;
        return true;
    }
    if (std::strcmp(value, "viewer-render-target-direct") == 0)
    {
        mode = SmokeMode::ViewerRenderTargetDirect;
        return true;
    }
    if (std::strcmp(value, "viewer-immediate-direct") == 0)
    {
        mode = SmokeMode::ViewerImmediateDirect;
        return true;
    }
    if (std::strcmp(value, "world-pipelines") == 0)
    {
        mode = SmokeMode::WorldPipelines;
        return true;
    }
    return false;
}

const char* get_smoke_mode_name(SmokeMode mode)
{
    switch (mode)
    {
    case SmokeMode::WorldPipelines:
        return "world-pipelines";
    case SmokeMode::DeferredGraph:
        return "deferred-graph";
    case SmokeMode::ViewerDeferredDirect:
        return "viewer-deferred-direct";
    case SmokeMode::ViewerRenderTargetDirect:
        return "viewer-render-target-direct";
    case SmokeMode::ViewerImmediateDirect:
        return "viewer-immediate-direct";
    case SmokeMode::DeferredComposite:
        return "deferred-composite";
    case SmokeMode::OffscreenCopy:
        return "offscreen-copy";
    case SmokeMode::DirectClear:
    default:
        return "direct-clear";
    }
}

const char* get_smoke_mode_description(SmokeMode mode)
{
    switch (mode)
    {
    case SmokeMode::OffscreenCopy:
        return " copied through an offscreen render target. ";
    case SmokeMode::DeferredComposite:
        return " produced through a synthetic deferred composite. ";
    case SmokeMode::DeferredGraph:
        return " produced through a synthetic G-buffer, deferred composite target, and final composite. ";
    case SmokeMode::ViewerDeferredDirect:
        return " produced through a viewer-style G-buffer directly composited to the swapchain. ";
    case SmokeMode::ViewerRenderTargetDirect:
        return " produced through an LLRenderTarget viewer-style G-buffer directly composited to the swapchain. ";
    case SmokeMode::ViewerImmediateDirect:
        return " produced through an LLRenderTarget viewer-style G-buffer and the viewer immediate-mode composite quad. ";
    case SmokeMode::WorldPipelines:
        return " replaced by a grid of real Vulkan world shader pipelines. ";
    case SmokeMode::DirectClear:
    default:
        return ". ";
    }
}

void print_smoke_usage(const char* executable)
{
    std::cout
        << "Usage: "
        << executable
        << " [options]\n\n"
        << "Options:\n"
        << "  --mode <name>              direct-clear, offscreen-copy, deferred-composite,\n"
        << "                             deferred-graph, viewer-deferred-direct,\n"
        << "                             viewer-render-target-direct, viewer-immediate-direct,\n"
        << "                             world-pipelines\n"
        << "  --frames <count>           Number of frames to render; 0 means run until closed\n"
        << "  --log-every <count>        Frame logging interval\n"
        << "  --readback-frames <count>  Number of diagnostic readback frames\n"
        << "  --no-buffer-readbacks      Disable G-buffer/composite input readbacks\n"
        << "  --no-stdout-readback       Keep readbacks in normal logs only\n"
        << "  --ui                       Draw a synthetic UI layer after the world/deferred pass\n"
        << "  --ui-viewer-sequence       Draw a stronger viewer-style UI sequence after the world/deferred pass\n"
        << "  --vulkan-sdk <path>        Sets VULKAN_SDK before creating the Vulkan context\n"
        << "  -h, --help                 Show this help\n";
}

bool parse_nonnegative_int(
    const char* value,
    const char* option,
    int& output)
{
    if (!value || !*value)
    {
        std::cerr << option << " requires a value.\n";
        return false;
    }

    char* end = nullptr;
    const long parsed = std::strtol(value, &end, 10);
    if (*end != '\0' || parsed < 0 || parsed > std::numeric_limits<int>::max())
    {
        std::cerr << "Invalid " << option << " value '" << value << "'.\n";
        return false;
    }

    output = static_cast<int>(parsed);
    return true;
}

bool parse_smoke_options(int argc, char** argv, SmokeOptions& options)
{
    auto require_value = [&](int& index, const char* option) -> const char*
    {
        if (index + 1 >= argc)
        {
            std::cerr << option << " requires a value.\n";
            return nullptr;
        }
        ++index;
        return argv[index];
    };

    auto value_after_equals = [](const std::string& argument, const char* option) -> const char*
    {
        const std::string prefix = std::string(option) + "=";
        if (argument.rfind(prefix, 0) == 0)
        {
            return argument.c_str() + prefix.size();
        }
        return nullptr;
    };

    for (int i = 1; i < argc; ++i)
    {
        const std::string argument = argv[i] ? argv[i] : "";
        const char* value = nullptr;

        if (argument == "-h" || argument == "--help")
        {
            options.mHelp = true;
            return true;
        }

        if (argument == "--mode" ||
            (value = value_after_equals(argument, "--mode")) != nullptr)
        {
            if (!value)
            {
                value = require_value(i, "--mode");
            }
            if (!value || !parse_smoke_mode_value(value, options.mMode))
            {
                std::cerr << "Invalid --mode value '" << (value ? value : "") << "'.\n";
                return false;
            }
            continue;
        }

        if (argument == "--frames" ||
            (value = value_after_equals(argument, "--frames")) != nullptr)
        {
            if (!value)
            {
                value = require_value(i, "--frames");
            }
            if (!parse_nonnegative_int(value, "--frames", options.mFrameLimit))
            {
                return false;
            }
            continue;
        }

        if (argument == "--log-every" ||
            (value = value_after_equals(argument, "--log-every")) != nullptr)
        {
            if (!value)
            {
                value = require_value(i, "--log-every");
            }
            if (!parse_nonnegative_int(value, "--log-every", options.mLogInterval))
            {
                return false;
            }
            options.mLogInterval = std::max(1, options.mLogInterval);
            continue;
        }

        if (argument == "--readback-frames" ||
            (value = value_after_equals(argument, "--readback-frames")) != nullptr)
        {
            if (!value)
            {
                value = require_value(i, "--readback-frames");
            }
            if (!parse_nonnegative_int(value, "--readback-frames", options.mReadbackFrameLimit))
            {
                return false;
            }
            continue;
        }

        if (argument == "--no-buffer-readbacks")
        {
            options.mBufferReadback = false;
            continue;
        }

        if (argument == "--no-stdout-readback")
        {
            options.mStdoutReadback = false;
            continue;
        }

        if (argument == "--ui")
        {
            options.mRenderUI = true;
            continue;
        }

        if (argument == "--ui-viewer-sequence")
        {
            options.mRenderUI = true;
            options.mRenderViewerUISequence = true;
            continue;
        }

        if (argument == "--vulkan-sdk" ||
            (value = value_after_equals(argument, "--vulkan-sdk")) != nullptr)
        {
            if (!value)
            {
                value = require_value(i, "--vulkan-sdk");
            }
            if (!value)
            {
                return false;
            }
            options.mVulkanSDK = value;
            continue;
        }

        SmokeMode positional_mode;
        if (i == 1 && parse_smoke_mode_value(argument.c_str(), positional_mode))
        {
            options.mMode = positional_mode;
            continue;
        }

        std::cerr << "Unknown mare-vulkan-smoke option '" << argument << "'.\n";
        return false;
    }

    return true;
}

int to_color_byte(float value)
{
    return llclamp(
        static_cast<int>(std::lround(value * 255.f)),
        0,
        255);
}

template <typename T>
void append_bytes(std::vector<U8>& bytes, const T& value)
{
    const U8* begin = reinterpret_cast<const U8*>(&value);
    bytes.insert(bytes.end(), begin, begin + sizeof(T));
}

struct SmokeQuad
{
    LLRenderBufferHandle mVertexBuffer;
    U64 mPositionOffset = 0;
    U64 mNormalOffset = 0;
    U64 mTexCoordOffset = 0;
    U64 mTexCoord1Offset = 0;
    U64 mTexCoord2Offset = 0;
    U64 mColorOffset = 0;
    U64 mTangentOffset = 0;
    U64 mWeightOffset = 0;
    U64 mWeight4Offset = 0;
    U64 mTextureIndexOffset = 0;
};

bool create_smoke_quad(LLRenderBackend& backend, SmokeQuad& quad)
{
    const std::array<std::array<F32, 4>, 6> positions =
    {{
        {{ -1.f, -1.f, 0.f, 1.f }},
        {{  1.f, -1.f, 0.f, 1.f }},
        {{ -1.f,  1.f, 0.f, 1.f }},
        {{ -1.f,  1.f, 0.f, 1.f }},
        {{  1.f, -1.f, 0.f, 1.f }},
        {{  1.f,  1.f, 0.f, 1.f }},
    }};
    const std::array<std::array<F32, 2>, 6> texcoords =
    {{
        {{ 0.f, 0.f }},
        {{ 1.f, 0.f }},
        {{ 0.f, 1.f }},
        {{ 0.f, 1.f }},
        {{ 1.f, 0.f }},
        {{ 1.f, 1.f }},
    }};
    const std::array<std::array<F32, 4>, 6> normals =
    {{
        {{ 0.f, 0.f, 1.f, 0.f }},
        {{ 0.f, 0.f, 1.f, 0.f }},
        {{ 0.f, 0.f, 1.f, 0.f }},
        {{ 0.f, 0.f, 1.f, 0.f }},
        {{ 0.f, 0.f, 1.f, 0.f }},
        {{ 0.f, 0.f, 1.f, 0.f }},
    }};
    const std::array<std::array<F32, 4>, 6> tangents =
    {{
        {{ 1.f, 0.f, 0.f, 1.f }},
        {{ 1.f, 0.f, 0.f, 1.f }},
        {{ 1.f, 0.f, 0.f, 1.f }},
        {{ 1.f, 0.f, 0.f, 1.f }},
        {{ 1.f, 0.f, 0.f, 1.f }},
        {{ 1.f, 0.f, 0.f, 1.f }},
    }};
    const std::array<F32, 6> weights =
    {{
        0.f, 0.f, 0.f, 0.f, 0.f, 0.f,
    }};
    const std::array<std::array<F32, 4>, 6> weight4s =
    {{
        {{ 0.f, 1.f, 0.f, 0.f }},
        {{ 0.f, 1.f, 0.f, 0.f }},
        {{ 0.f, 1.f, 0.f, 0.f }},
        {{ 0.f, 1.f, 0.f, 0.f }},
        {{ 0.f, 1.f, 0.f, 0.f }},
        {{ 0.f, 1.f, 0.f, 0.f }},
    }};
    const std::array<std::array<U32, 4>, 6> texture_indices =
    {{
        {{ 0, 0, 0, 0 }},
        {{ 0, 0, 0, 0 }},
        {{ 0, 0, 0, 0 }},
        {{ 0, 0, 0, 0 }},
        {{ 0, 0, 0, 0 }},
        {{ 0, 0, 0, 0 }},
    }};
    const std::array<std::array<U8, 4>, 6> colors =
    {{
        {{ 255, 255, 255, 255 }},
        {{ 255, 255, 255, 255 }},
        {{ 255, 255, 255, 255 }},
        {{ 255, 255, 255, 255 }},
        {{ 255, 255, 255, 255 }},
        {{ 255, 255, 255, 255 }},
    }};

    std::vector<U8> bytes;
    bytes.reserve(
        positions.size() * sizeof(positions[0]) +
        normals.size() * sizeof(normals[0]) +
        texcoords.size() * sizeof(texcoords[0]) +
        texcoords.size() * sizeof(texcoords[0]) +
        texcoords.size() * sizeof(texcoords[0]) +
        tangents.size() * sizeof(tangents[0]) +
        weights.size() * sizeof(weights[0]) +
        weight4s.size() * sizeof(weight4s[0]) +
        texture_indices.size() * sizeof(texture_indices[0]) +
        colors.size() * sizeof(colors[0]));

    quad.mPositionOffset = bytes.size();
    for (const auto& position : positions)
    {
        append_bytes(bytes, position);
    }

    quad.mNormalOffset = bytes.size();
    for (const auto& normal : normals)
    {
        append_bytes(bytes, normal);
    }

    quad.mTexCoordOffset = bytes.size();
    for (const auto& texcoord : texcoords)
    {
        append_bytes(bytes, texcoord);
    }

    quad.mTexCoord1Offset = bytes.size();
    for (const auto& texcoord : texcoords)
    {
        append_bytes(bytes, texcoord);
    }

    quad.mTexCoord2Offset = bytes.size();
    for (const auto& texcoord : texcoords)
    {
        append_bytes(bytes, texcoord);
    }

    quad.mColorOffset = bytes.size();
    for (const auto& color : colors)
    {
        append_bytes(bytes, color);
    }

    quad.mTangentOffset = bytes.size();
    for (const auto& tangent : tangents)
    {
        append_bytes(bytes, tangent);
    }

    quad.mWeightOffset = bytes.size();
    for (const F32 weight : weights)
    {
        append_bytes(bytes, weight);
    }

    quad.mWeight4Offset = bytes.size();
    for (const auto& weight4 : weight4s)
    {
        append_bytes(bytes, weight4);
    }

    quad.mTextureIndexOffset = bytes.size();
    for (const auto& texture_index : texture_indices)
    {
        append_bytes(bytes, texture_index);
    }

    quad.mVertexBuffer = backend.createBufferHandle();
    if (!quad.mVertexBuffer)
    {
        return false;
    }

    backend.bindBuffer(LLRenderBufferTarget::Vertex, quad.mVertexBuffer);
    backend.allocateBufferStorage(
        LLRenderBufferTarget::Vertex,
        bytes.size(),
        bytes.data(),
        LLRenderBufferUsage::StaticDraw);
    return true;
}

void bind_smoke_quad(LLRenderBackend& backend, const SmokeQuad& quad)
{
    backend.bindBuffer(LLRenderBufferTarget::Vertex, quad.mVertexBuffer);
    backend.enableVertexAttributeArray(0);
    backend.setVertexAttributePointer(
        0,
        3,
        LLRenderVertexAttributeType::Float32,
        false,
        16,
        reinterpret_cast<const void*>(
            static_cast<uintptr_t>(quad.mPositionOffset)));
    backend.enableVertexAttributeArray(2);
    backend.setVertexAttributePointer(
        2,
        2,
        LLRenderVertexAttributeType::Float32,
        false,
        8,
        reinterpret_cast<const void*>(
            static_cast<uintptr_t>(quad.mTexCoordOffset)));
    backend.enableVertexAttributeArray(6);
    backend.setVertexAttributePointer(
        6,
        4,
        LLRenderVertexAttributeType::UnsignedByte,
        true,
        4,
        reinterpret_cast<const void*>(
            static_cast<uintptr_t>(quad.mColorOffset)));
}

void bind_world_smoke_quad(LLRenderBackend& backend, const SmokeQuad& quad)
{
    backend.bindBuffer(LLRenderBufferTarget::Vertex, quad.mVertexBuffer);
    backend.enableVertexAttributeArray(0);
    backend.setVertexAttributePointer(
        0,
        3,
        LLRenderVertexAttributeType::Float32,
        false,
        16,
        reinterpret_cast<const void*>(
            static_cast<uintptr_t>(quad.mPositionOffset)));
    backend.enableVertexAttributeArray(1);
    backend.setVertexAttributePointer(
        1,
        3,
        LLRenderVertexAttributeType::Float32,
        false,
        16,
        reinterpret_cast<const void*>(
            static_cast<uintptr_t>(quad.mNormalOffset)));
    backend.enableVertexAttributeArray(2);
    backend.setVertexAttributePointer(
        2,
        2,
        LLRenderVertexAttributeType::Float32,
        false,
        8,
        reinterpret_cast<const void*>(
            static_cast<uintptr_t>(quad.mTexCoordOffset)));
    backend.enableVertexAttributeArray(3);
    backend.setVertexAttributePointer(
        3,
        2,
        LLRenderVertexAttributeType::Float32,
        false,
        8,
        reinterpret_cast<const void*>(
            static_cast<uintptr_t>(quad.mTexCoord1Offset)));
    backend.enableVertexAttributeArray(4);
    backend.setVertexAttributePointer(
        4,
        2,
        LLRenderVertexAttributeType::Float32,
        false,
        8,
        reinterpret_cast<const void*>(
            static_cast<uintptr_t>(quad.mTexCoord2Offset)));
    backend.enableVertexAttributeArray(6);
    backend.setVertexAttributePointer(
        6,
        4,
        LLRenderVertexAttributeType::UnsignedByte,
        true,
        4,
        reinterpret_cast<const void*>(
            static_cast<uintptr_t>(quad.mColorOffset)));
    backend.enableVertexAttributeArray(8);
    backend.setVertexAttributePointer(
        8,
        4,
        LLRenderVertexAttributeType::Float32,
        false,
        16,
        reinterpret_cast<const void*>(
            static_cast<uintptr_t>(quad.mTangentOffset)));
    backend.enableVertexAttributeArray(9);
    backend.setVertexAttributePointer(
        9,
        1,
        LLRenderVertexAttributeType::Float32,
        false,
        4,
        reinterpret_cast<const void*>(
            static_cast<uintptr_t>(quad.mWeightOffset)));
    backend.enableVertexAttributeArray(10);
    backend.setVertexAttributePointer(
        10,
        4,
        LLRenderVertexAttributeType::Float32,
        false,
        16,
        reinterpret_cast<const void*>(
            static_cast<uintptr_t>(quad.mWeight4Offset)));
    backend.enableVertexAttributeArray(13);
    backend.setIntegerVertexAttributePointer(
        13,
        1,
        LLRenderVertexAttributeType::UnsignedInt,
        16,
        reinterpret_cast<const void*>(
            static_cast<uintptr_t>(quad.mTextureIndexOffset)));
}

struct SmokeOffscreen
{
    LLRenderTextureHandle mColorTexture;
    LLRenderFramebufferHandle mFramebuffer;
    U32 mWidth = 0;
    U32 mHeight = 0;
};

void release_smoke_offscreen(LLRenderBackend& backend, SmokeOffscreen& offscreen)
{
    if (offscreen.mFramebuffer)
    {
        backend.deleteFramebufferHandle(offscreen.mFramebuffer);
        offscreen.mFramebuffer = {};
    }
    if (offscreen.mColorTexture)
    {
        backend.deleteTextureHandle(offscreen.mColorTexture);
        offscreen.mColorTexture = {};
    }
    offscreen.mWidth = 0;
    offscreen.mHeight = 0;
}

bool ensure_smoke_offscreen(
    LLRenderBackend& backend,
    SmokeOffscreen& offscreen,
    U32 width,
    U32 height)
{
    if (offscreen.mFramebuffer &&
        offscreen.mColorTexture &&
        offscreen.mWidth == width &&
        offscreen.mHeight == height)
    {
        return true;
    }

    release_smoke_offscreen(backend, offscreen);

    offscreen.mColorTexture = backend.createTextureHandle();
    offscreen.mFramebuffer = backend.createFramebufferHandle();
    offscreen.mWidth = width;
    offscreen.mHeight = height;
    if (!offscreen.mColorTexture || !offscreen.mFramebuffer)
    {
        release_smoke_offscreen(backend, offscreen);
        return false;
    }

    backend.setActiveTextureUnit(0);
    backend.bindTexture(LLRenderTextureTarget::Texture2D, offscreen.mColorTexture);
    backend.setTextureImage2D(
        LLRenderTextureTarget::Texture2D,
        0,
        LLRenderTextureFormat::RGBA8,
        static_cast<S32>(width),
        static_cast<S32>(height),
        0,
        LLRenderPixelFormat::RGBA,
        LLRenderPixelType::UnsignedByte,
        nullptr);
    if (!backend.didLastTextureUploadSucceed())
    {
        release_smoke_offscreen(backend, offscreen);
        return false;
    }
    backend.setTextureFilter(
        LLRenderTextureTarget::Texture2D,
        LLRenderTextureFilter::Linear,
        LLRenderTextureFilter::Linear);
    backend.setTextureAddressMode(
        LLRenderTextureTarget::Texture2D,
        LLRenderTextureAddressMode::ClampToEdge);

    backend.bindReadWriteFramebuffer(offscreen.mFramebuffer);
    backend.attachFramebufferTexture2D(
        LLRenderFramebufferAttachment::Color0,
        LLRenderTextureTarget::Texture2D,
        offscreen.mColorTexture,
        0);
    backend.setFramebufferBufferRouting(1);
    const bool complete = backend.isDrawFramebufferComplete();
    backend.bindReadWriteFramebuffer(LLRenderFramebufferHandle());
    backend.restoreDefaultFramebufferBufferRouting();
    if (!complete)
    {
        release_smoke_offscreen(backend, offscreen);
        return false;
    }
    return true;
}

bool render_offscreen_copy_frame(
    LLRenderBackend& backend,
    SmokeOffscreen& offscreen,
    const SmokeQuad& quad,
    U32 width,
    U32 height,
    F32 clear_red,
    F32 clear_green,
    F32 clear_blue,
    F32 clear_alpha)
{
    if (!ensure_smoke_offscreen(backend, offscreen, width, height))
    {
        return false;
    }

    backend.bindReadWriteFramebuffer(offscreen.mFramebuffer);
    backend.setFramebufferBufferRouting(1);
    backend.setViewport(0, 0, static_cast<S32>(width), static_cast<S32>(height));
    backend.setScissor(0, 0, static_cast<S32>(width), static_cast<S32>(height));
    backend.setClearColor(clear_red, clear_green, clear_blue, clear_alpha);
    backend.clear(LL_RENDER_CLEAR_COLOR);

    backend.bindReadWriteFramebuffer(LLRenderFramebufferHandle());
    backend.restoreDefaultFramebufferBufferRouting();
    backend.setViewport(0, 0, static_cast<S32>(width), static_cast<S32>(height));
    backend.setScissor(0, 0, static_cast<S32>(width), static_cast<S32>(height));
    backend.setClearColor(0.06f, 0.f, 0.f, 1.f);
    backend.clear(LL_RENDER_CLEAR_COLOR | LL_RENDER_CLEAR_DEPTH);

    backend.setWorldDrawEnabled(false);
    backend.setActiveTextureUnit(0);
    backend.bindTexture(LLRenderTextureTarget::Texture2D, offscreen.mColorTexture);
    bind_smoke_quad(backend, quad);
    backend.drawArrays(LLRenderPrimitiveType::Triangles, 0, 6);
    return true;
}

struct SmokeDeferredTextures
{
    LLRenderTextureHandle mDiffuse;
    LLRenderTextureHandle mSpecular;
    LLRenderTextureHandle mNormal;
    LLRenderTextureHandle mEmissive;
    LLRenderTextureHandle mDepth;
};

std::vector<U8> make_solid_rgba_pixels(
    U32 width,
    U32 height,
    U8 red,
    U8 green,
    U8 blue,
    U8 alpha)
{
    std::vector<U8> pixels;
    pixels.resize(static_cast<size_t>(width) * static_cast<size_t>(height) * 4U);
    for (size_t i = 0; i < pixels.size(); i += 4)
    {
        pixels[i + 0] = red;
        pixels[i + 1] = green;
        pixels[i + 2] = blue;
        pixels[i + 3] = alpha;
    }
    return pixels;
}

bool create_smoke_texture(
    LLRenderBackend& backend,
    LLRenderTextureHandle& texture,
    U32 width,
    U32 height,
    const std::vector<U8>& pixels)
{
    texture = backend.createTextureHandle();
    if (!texture)
    {
        return false;
    }

    backend.setActiveTextureUnit(0);
    backend.bindTexture(LLRenderTextureTarget::Texture2D, texture);
    backend.setTextureImage2D(
        LLRenderTextureTarget::Texture2D,
        0,
        LLRenderTextureFormat::RGBA8,
        static_cast<S32>(width),
        static_cast<S32>(height),
        0,
        LLRenderPixelFormat::RGBA,
        LLRenderPixelType::UnsignedByte,
        pixels.data());
    if (!backend.didLastTextureUploadSucceed())
    {
        backend.deleteTextureHandle(texture);
        texture = {};
        return false;
    }

    backend.setTextureFilter(
        LLRenderTextureTarget::Texture2D,
        LLRenderTextureFilter::Linear,
        LLRenderTextureFilter::Linear);
    backend.setTextureAddressMode(
        LLRenderTextureTarget::Texture2D,
        LLRenderTextureAddressMode::ClampToEdge);
    return true;
}

bool create_empty_smoke_texture(
    LLRenderBackend& backend,
    LLRenderTextureHandle& texture,
    U32 width,
    U32 height,
    LLRenderTextureFormat internal_format,
    LLRenderPixelFormat pixel_format,
    LLRenderPixelType pixel_type)
{
    texture = backend.createTextureHandle();
    if (!texture)
    {
        return false;
    }

    backend.setActiveTextureUnit(0);
    backend.bindTexture(LLRenderTextureTarget::Texture2D, texture);
    backend.setTextureImage2D(
        LLRenderTextureTarget::Texture2D,
        0,
        internal_format,
        static_cast<S32>(width),
        static_cast<S32>(height),
        0,
        pixel_format,
        pixel_type,
        nullptr);
    if (!backend.didLastTextureUploadSucceed())
    {
        backend.deleteTextureHandle(texture);
        texture = {};
        return false;
    }

    backend.setTextureFilter(
        LLRenderTextureTarget::Texture2D,
        LLRenderTextureFilter::Linear,
        LLRenderTextureFilter::Linear);
    backend.setTextureAddressMode(
        LLRenderTextureTarget::Texture2D,
        LLRenderTextureAddressMode::ClampToEdge);
    return true;
}

void release_smoke_deferred_textures(
    LLRenderBackend& backend,
    SmokeDeferredTextures& textures)
{
    LLRenderTextureHandle* handles[] =
    {
        &textures.mDiffuse,
        &textures.mSpecular,
        &textures.mNormal,
        &textures.mEmissive,
        &textures.mDepth,
    };
    for (LLRenderTextureHandle* handle : handles)
    {
        if (*handle)
        {
            backend.deleteTextureHandle(*handle);
            *handle = {};
        }
    }
}

bool ensure_smoke_deferred_textures(
    LLRenderBackend& backend,
    SmokeDeferredTextures& textures)
{
    if (textures.mDiffuse &&
        textures.mSpecular &&
        textures.mNormal &&
        textures.mEmissive &&
        textures.mDepth)
    {
        return true;
    }

    release_smoke_deferred_textures(backend, textures);
    constexpr U32 texture_width = 4;
    constexpr U32 texture_height = 4;
    if (!create_smoke_texture(
            backend,
            textures.mDiffuse,
            texture_width,
            texture_height,
            make_solid_rgba_pixels(texture_width, texture_height, 42, 126, 230, 255)) ||
        !create_smoke_texture(
            backend,
            textures.mSpecular,
            texture_width,
            texture_height,
            make_solid_rgba_pixels(texture_width, texture_height, 0, 0, 0, 0)) ||
        !create_smoke_texture(
            backend,
            textures.mNormal,
            texture_width,
            texture_height,
            make_solid_rgba_pixels(texture_width, texture_height, 128, 128, 255, 0)) ||
        !create_smoke_texture(
            backend,
            textures.mEmissive,
            texture_width,
            texture_height,
            make_solid_rgba_pixels(texture_width, texture_height, 0, 0, 0, 255)) ||
        !create_smoke_texture(
            backend,
            textures.mDepth,
            texture_width,
            texture_height,
            make_solid_rgba_pixels(texture_width, texture_height, 255, 255, 255, 255)))
    {
        release_smoke_deferred_textures(backend, textures);
        return false;
    }
    return true;
}

bool render_deferred_composite_frame(
    LLRenderBackend& backend,
    SmokeDeferredTextures& textures,
    const SmokeQuad& quad,
    U32 width,
    U32 height)
{
    if (!ensure_smoke_deferred_textures(backend, textures))
    {
        return false;
    }

    backend.bindReadWriteFramebuffer(LLRenderFramebufferHandle());
    backend.restoreDefaultFramebufferBufferRouting();
    backend.setViewport(0, 0, static_cast<S32>(width), static_cast<S32>(height));
    backend.setScissor(0, 0, static_cast<S32>(width), static_cast<S32>(height));
    backend.setClearColor(0.06f, 0.f, 0.f, 1.f);
    backend.clear(LL_RENDER_CLEAR_COLOR | LL_RENDER_CLEAR_DEPTH);

    backend.setActiveTextureUnit(0);
    backend.bindTexture(LLRenderTextureTarget::Texture2D, textures.mDiffuse);
    backend.setActiveTextureUnit(1);
    backend.bindTexture(LLRenderTextureTarget::Texture2D, textures.mSpecular);
    backend.setActiveTextureUnit(2);
    backend.bindTexture(LLRenderTextureTarget::Texture2D, textures.mNormal);
    backend.setActiveTextureUnit(3);
    backend.bindTexture(LLRenderTextureTarget::Texture2D, textures.mEmissive);
    backend.setActiveTextureUnit(4);
    backend.bindTexture(LLRenderTextureTarget::Texture2D, textures.mDepth);
    backend.setActiveTextureUnit(0);

    LLRenderWorldMaterialParameters parameters;
    parameters.mBaseColorRed = 0.35f;
    parameters.mBaseColorGreen = 0.37f;
    parameters.mBaseColorBlue = 0.42f;
    parameters.mBaseColorAlpha = 1.f;
    parameters.mEmissiveColorRed = 0.95f;
    parameters.mEmissiveColorGreen = 0.96f;
    parameters.mEmissiveColorBlue = 1.f;
    parameters.mSpecularColorRed = 0.35f;
    parameters.mSpecularColorGreen = 0.45f;
    parameters.mSpecularColorBlue = 0.82f;
    parameters.mEnvIntensity = 0.25f;
    parameters.mSceneAmbientRed = 0.35f;
    parameters.mSceneAmbientGreen = 0.37f;
    parameters.mSceneAmbientBlue = 0.42f;
    parameters.mSceneDirectScale = 1.f;
    parameters.mSceneDirectRed = 0.95f;
    parameters.mSceneDirectGreen = 0.96f;
    parameters.mSceneDirectBlue = 1.f;
    parameters.mSceneLightingValid = 1.f;

    backend.setWorldDrawEnabled(true);
    backend.setWorldShaderClass(LLRenderWorldShaderClass::DeferredComposite);
    backend.setWorldMaterialParameters(parameters);
    backend.setWorldTextureTransform({});
    backend.setWorldTerrainParameters({});
    backend.setWorldSkinningMatrixPalette(0, nullptr);
    bind_smoke_quad(backend, quad);
    backend.drawArrays(LLRenderPrimitiveType::Triangles, 0, 6);
    backend.setWorldDrawEnabled(false);
    backend.setWorldShaderClass(LLRenderWorldShaderClass::Textured);
    backend.setWorldMaterialParameters({});
    return true;
}

void bind_world_pipeline_smoke_textures(
    LLRenderBackend& backend,
    const SmokeDeferredTextures& textures)
{
    const LLRenderTextureHandle bindings[] =
    {
        textures.mDiffuse,
        textures.mSpecular,
        textures.mNormal,
        textures.mEmissive,
        textures.mDepth,
        textures.mDepth,
    };
    const S32 binding_count =
        static_cast<S32>(sizeof(bindings) / sizeof(bindings[0]));
    for (S32 unit = 0; unit < binding_count; ++unit)
    {
        backend.setActiveTextureUnit(unit);
        backend.bindTexture(LLRenderTextureTarget::Texture2D, bindings[unit]);
    }
    for (S32 unit = 9; unit <= 12; ++unit)
    {
        backend.setActiveTextureUnit(unit);
        backend.bindTexture(LLRenderTextureTarget::Texture2D, textures.mEmissive);
    }
    backend.setActiveTextureUnit(0);
}

LLRenderWorldMaterialParameters make_world_pipeline_material(
    F32 red,
    F32 green,
    F32 blue,
    F32 alpha,
    U32 flags)
{
    LLRenderWorldMaterialParameters parameters;
    parameters.mBaseColorRed = red;
    parameters.mBaseColorGreen = green;
    parameters.mBaseColorBlue = blue;
    parameters.mBaseColorAlpha = alpha;
    parameters.mEmissiveColorRed = red * 0.18f;
    parameters.mEmissiveColorGreen = green * 0.18f;
    parameters.mEmissiveColorBlue = blue * 0.18f;
    parameters.mSpecularColorRed = 0.72f;
    parameters.mSpecularColorGreen = 0.76f;
    parameters.mSpecularColorBlue = 0.84f;
    parameters.mEnvIntensity = 0.35f;
    parameters.mRoughnessFactor = 0.58f;
    parameters.mMetallicFactor = 0.18f;
    parameters.mHasORMMap =
        (flags & LLRenderWorldMaterialParameters::HasORMMap) ? 1.f : 0.f;
    parameters.mHasEmissiveMap =
        (flags & LLRenderWorldMaterialParameters::Glow) ? 1.f : 0.f;
    parameters.mMaterialFlags = static_cast<F32>(flags);
    parameters.mSceneAmbientRed = 0.34f;
    parameters.mSceneAmbientGreen = 0.38f;
    parameters.mSceneAmbientBlue = 0.46f;
    parameters.mSceneDirectScale = 1.05f;
    parameters.mSceneDirectRed = 1.f;
    parameters.mSceneDirectGreen = 0.96f;
    parameters.mSceneDirectBlue = 0.88f;
    parameters.mSceneLightingValid = 1.f;
    parameters.mSceneLightDirectionX = 0.35f;
    parameters.mSceneLightDirectionY = 0.45f;
    parameters.mSceneLightDirectionZ = 0.82f;
    parameters.mSceneLightDirectionValid = 1.f;
    return parameters;
}

LLRenderWorldTerrainParameters make_world_pipeline_terrain_parameters()
{
    LLRenderWorldTerrainParameters parameters;
    parameters.mPlanarSampleCount = 1.f;
    parameters.mPaintType = 0.f;
    parameters.mBaseColorFactors[0] = 0.26f;
    parameters.mBaseColorFactors[1] = 0.46f;
    parameters.mBaseColorFactors[2] = 0.30f;
    parameters.mBaseColorFactors[3] = 1.f;
    parameters.mBaseColorFactors[4] = 0.48f;
    parameters.mBaseColorFactors[5] = 0.42f;
    parameters.mBaseColorFactors[6] = 0.32f;
    parameters.mBaseColorFactors[7] = 1.f;
    parameters.mBaseColorFactors[8] = 0.35f;
    parameters.mBaseColorFactors[9] = 0.42f;
    parameters.mBaseColorFactors[10] = 0.52f;
    parameters.mBaseColorFactors[11] = 1.f;
    parameters.mBaseColorFactors[12] = 0.62f;
    parameters.mBaseColorFactors[13] = 0.58f;
    parameters.mBaseColorFactors[14] = 0.44f;
    parameters.mBaseColorFactors[15] = 1.f;
    return parameters;
}

struct SmokeWorldPipelineEntry
{
    LLRenderWorldShaderClass mShaderClass;
    const char* mName;
    F32 mRed;
    F32 mGreen;
    F32 mBlue;
    U32 mFlags;
    bool mAlphaBlend = false;
    bool mAddBlend = false;
};

bool render_world_pipelines_frame(
    LLRenderBackend& backend,
    SmokeDeferredTextures& textures,
    const SmokeQuad& quad,
    U32 width,
    U32 height)
{
    if (!ensure_smoke_deferred_textures(backend, textures))
    {
        return false;
    }

    constexpr U32 fullbright =
        LLRenderWorldMaterialParameters::Fullbright;
    constexpr U32 pbr =
        LLRenderWorldMaterialParameters::GLTFPBR |
        LLRenderWorldMaterialParameters::HasNormalMap |
        LLRenderWorldMaterialParameters::HasORMMap;
    constexpr U32 material =
        LLRenderWorldMaterialParameters::HasSpecularMap |
        LLRenderWorldMaterialParameters::LegacyBump |
        LLRenderWorldMaterialParameters::LegacyShiny;
    constexpr U32 alpha =
        LLRenderWorldMaterialParameters::AlphaBlend;
    constexpr U32 glow =
        LLRenderWorldMaterialParameters::Glow |
        LLRenderWorldMaterialParameters::PostDeferred;
    constexpr U32 water =
        LLRenderWorldMaterialParameters::Water |
        LLRenderWorldMaterialParameters::PostDeferred |
        LLRenderWorldMaterialParameters::SceneDepth |
        LLRenderWorldMaterialParameters::SceneColor;
    constexpr U32 haze =
        LLRenderWorldMaterialParameters::AtmosphericHaze |
        LLRenderWorldMaterialParameters::PostDeferred |
        LLRenderWorldMaterialParameters::SceneDepth |
        LLRenderWorldMaterialParameters::SceneColor;

    const std::array<SmokeWorldPipelineEntry, 14> entries =
    {{
        { LLRenderWorldShaderClass::Sky, "Sky", 0.25f, 0.48f, 0.92f, 0 },
        { LLRenderWorldShaderClass::Terrain, "Terrain", 0.34f, 0.58f, 0.28f, 0 },
        { LLRenderWorldShaderClass::Textured, "Textured", 0.42f, 0.72f, 0.96f, 0 },
        { LLRenderWorldShaderClass::AlphaMask, "AlphaMask", 0.92f, 0.78f, 0.28f, LLRenderWorldMaterialParameters::AlphaMask },
        { LLRenderWorldShaderClass::Fullbright, "Fullbright", 0.95f, 0.42f, 0.35f, fullbright },
        { LLRenderWorldShaderClass::Material, "Material", 0.78f, 0.58f, 0.95f, material },
        { LLRenderWorldShaderClass::PBR, "PBR", 0.90f, 0.72f, 0.48f, pbr },
        { LLRenderWorldShaderClass::Avatar, "Avatar", 0.86f, 0.52f, 0.44f, 0 },
        { LLRenderWorldShaderClass::Water, "Water", 0.20f, 0.52f, 0.82f, water, true },
        { LLRenderWorldShaderClass::Haze, "Haze", 0.72f, 0.78f, 0.86f, haze, true },
        { LLRenderWorldShaderClass::Alpha, "Alpha", 0.82f, 0.34f, 0.68f, alpha, true },
        { LLRenderWorldShaderClass::Glow, "Glow", 1.00f, 0.72f, 0.22f, glow, false, true },
        { LLRenderWorldShaderClass::DeferredComposite, "DeferredComposite", 0.38f, 0.48f, 0.68f, 0 },
        { LLRenderWorldShaderClass::FinalComposite, "FinalComposite", 0.72f, 0.84f, 0.96f, 0 },
    }};

    backend.bindReadWriteFramebuffer(LLRenderFramebufferHandle());
    backend.restoreDefaultFramebufferBufferRouting();
    backend.setViewport(0, 0, static_cast<S32>(width), static_cast<S32>(height));
    backend.setScissor(0, 0, static_cast<S32>(width), static_cast<S32>(height));
    backend.setClearColor(0.015f, 0.018f, 0.024f, 1.f);
    backend.clear(LL_RENDER_CLEAR_COLOR | LL_RENDER_CLEAR_DEPTH);
    backend.setDepthWriteEnabled(false);
    backend.setCapability(LLRenderCapability::DepthTest, false);
    backend.setCapability(LLRenderCapability::CullFace, false);
    backend.setColorMask({ true, true, true, true });

    bind_world_pipeline_smoke_textures(backend, textures);
    bind_world_smoke_quad(backend, quad);
    backend.setWorldTextureTransform({});
    backend.setWorldTerrainParameters(make_world_pipeline_terrain_parameters());
    backend.setWorldSkinningMatrixPalette(0, nullptr);
    backend.setWorldDrawEnabled(true);

    constexpr S32 columns = 4;
    const S32 rows =
        static_cast<S32>((entries.size() + columns - 1) / columns);
    const S32 cell_width = llmax(1, static_cast<S32>(width) / columns);
    const S32 cell_height = llmax(1, static_cast<S32>(height) / rows);
    static bool logged_world_pipeline_entries = false;
    const bool log_world_pipeline_entries = !logged_world_pipeline_entries;

    for (U32 i = 0; i < entries.size(); ++i)
    {
        const SmokeWorldPipelineEntry& entry = entries[i];
        const S32 column = static_cast<S32>(i % columns);
        const S32 row = static_cast<S32>(i / columns);
        const S32 x = column * cell_width;
        const S32 y = row * cell_height;
        const S32 w = (column == columns - 1) ?
            static_cast<S32>(width) - x :
            cell_width;
        const S32 h = (row == rows - 1) ?
            static_cast<S32>(height) - y :
            cell_height;

        if (i == 0 && log_world_pipeline_entries)
        {
            std::cout
                << "Mare Vulkan smoke world-pipelines entries:";
        }
        if (log_world_pipeline_entries)
        {
            std::cout
                << " "
                << entry.mName;
        }

        backend.setScissor(x, y, w, h);
        backend.setWorldShaderClass(entry.mShaderClass);
        backend.setWorldMaterialParameters(
            make_world_pipeline_material(
                entry.mRed,
                entry.mGreen,
                entry.mBlue,
                entry.mAlphaBlend ? 0.72f : 1.f,
                entry.mFlags));
        backend.setCapability(LLRenderCapability::Blend, entry.mAlphaBlend || entry.mAddBlend);
        if (entry.mAddBlend)
        {
            backend.setBlendState(
                {
                    LLRenderBlendFactor::SourceAlpha,
                    LLRenderBlendFactor::One,
                    LLRenderBlendFactor::One,
                    LLRenderBlendFactor::One,
                });
        }
        else if (entry.mAlphaBlend)
        {
            backend.setBlendState(
                {
                    LLRenderBlendFactor::SourceAlpha,
                    LLRenderBlendFactor::OneMinusSourceAlpha,
                    LLRenderBlendFactor::One,
                    LLRenderBlendFactor::OneMinusSourceAlpha,
                });
        }
        backend.drawArrays(LLRenderPrimitiveType::Triangles, 0, 6);
    }
    if (log_world_pipeline_entries)
    {
        std::cout << std::endl;
        logged_world_pipeline_entries = true;
    }

    backend.setCapability(LLRenderCapability::Blend, false);
    backend.setWorldDrawEnabled(false);
    backend.setWorldShaderClass(LLRenderWorldShaderClass::Textured);
    backend.setWorldMaterialParameters({});
    backend.setWorldTerrainParameters({});
    backend.setWorldTextureTransform({});
    backend.setWorldSkinningMatrixPalette(0, nullptr);
    backend.setScissor(0, 0, static_cast<S32>(width), static_cast<S32>(height));
    return true;
}

struct SmokeDeferredGraph
{
    LLRenderTextureHandle mGBufferColor;
    LLRenderTextureHandle mGBufferSpecular;
    LLRenderTextureHandle mGBufferNormal;
    LLRenderTextureHandle mGBufferEmissive;
    LLRenderTextureHandle mGBufferDepth;
    LLRenderTextureHandle mDeferredColor;
    LLRenderFramebufferHandle mGBufferFramebuffer;
    LLRenderFramebufferHandle mDeferredFramebuffer;
    U32 mWidth = 0;
    U32 mHeight = 0;
    U32 mColorAttachmentCount = 0;
    bool mViewerFormats = false;
};

struct SmokeViewerRenderTargetGraph
{
    LLRenderTarget mDeferredScreen;
    U32 mWidth = 0;
    U32 mHeight = 0;
    U32 mColorAttachmentCount = 0;
};

void release_smoke_deferred_graph(
    LLRenderBackend& backend,
    SmokeDeferredGraph& graph)
{
    if (graph.mGBufferFramebuffer)
    {
        backend.deleteFramebufferHandle(graph.mGBufferFramebuffer);
        graph.mGBufferFramebuffer = {};
    }
    if (graph.mDeferredFramebuffer)
    {
        backend.deleteFramebufferHandle(graph.mDeferredFramebuffer);
        graph.mDeferredFramebuffer = {};
    }

    LLRenderTextureHandle* textures[] =
    {
        &graph.mGBufferColor,
        &graph.mGBufferSpecular,
        &graph.mGBufferNormal,
        &graph.mGBufferEmissive,
        &graph.mGBufferDepth,
        &graph.mDeferredColor,
    };
    for (LLRenderTextureHandle* texture : textures)
    {
        if (*texture)
        {
            backend.deleteTextureHandle(*texture);
            *texture = {};
        }
    }
    graph.mWidth = 0;
    graph.mHeight = 0;
    graph.mColorAttachmentCount = 0;
    graph.mViewerFormats = false;
}

void release_smoke_viewer_render_target_graph(
    SmokeViewerRenderTargetGraph& graph)
{
    graph.mDeferredScreen.release();
    graph.mWidth = 0;
    graph.mHeight = 0;
    graph.mColorAttachmentCount = 0;
}

bool ensure_smoke_viewer_render_target_graph(
    SmokeViewerRenderTargetGraph& graph,
    U32 width,
    U32 height,
    U32 color_attachment_count)
{
    color_attachment_count = llclamp(color_attachment_count, 3U, 4U);
    if (graph.mDeferredScreen.isComplete() &&
        graph.mDeferredScreen.getWidth() == width &&
        graph.mDeferredScreen.getHeight() == height &&
        graph.mDeferredScreen.getNumTextures() == color_attachment_count &&
        graph.mColorAttachmentCount == color_attachment_count)
    {
        return true;
    }

    release_smoke_viewer_render_target_graph(graph);
    graph.mWidth = width;
    graph.mHeight = height;
    graph.mColorAttachmentCount = color_attachment_count;

    if (!graph.mDeferredScreen.allocate(
            width,
            height,
            LLRenderTextureFormat::RGBA,
            true))
    {
        release_smoke_viewer_render_target_graph(graph);
        return false;
    }

    if (!graph.mDeferredScreen.addColorAttachment(LLRenderTextureFormat::RGBA) ||
        !graph.mDeferredScreen.addColorAttachment(LLRenderTextureFormat::RGBA16))
    {
        release_smoke_viewer_render_target_graph(graph);
        return false;
    }

    if (color_attachment_count >= 4U &&
        !graph.mDeferredScreen.addColorAttachment(LLRenderTextureFormat::RGB16F))
    {
        release_smoke_viewer_render_target_graph(graph);
        return false;
    }

    return graph.mDeferredScreen.isComplete();
}

bool ensure_smoke_deferred_graph(
    LLRenderBackend& backend,
    SmokeDeferredGraph& graph,
    U32 width,
    U32 height,
    U32 color_attachment_count,
    bool viewer_formats)
{
    color_attachment_count = llclamp(color_attachment_count, 3U, 4U);
    if (graph.mGBufferFramebuffer &&
        graph.mDeferredFramebuffer &&
        graph.mGBufferColor &&
        graph.mGBufferSpecular &&
        graph.mGBufferNormal &&
        (color_attachment_count < 4U || graph.mGBufferEmissive) &&
        graph.mGBufferDepth &&
        graph.mDeferredColor &&
        graph.mWidth == width &&
        graph.mHeight == height &&
        graph.mColorAttachmentCount == color_attachment_count &&
        graph.mViewerFormats == viewer_formats)
    {
        return true;
    }

    release_smoke_deferred_graph(backend, graph);
    graph.mWidth = width;
    graph.mHeight = height;
    graph.mColorAttachmentCount = color_attachment_count;
    graph.mViewerFormats = viewer_formats;

    const LLRenderTextureFormat color_format =
        viewer_formats ? LLRenderTextureFormat::RGBA : LLRenderTextureFormat::RGBA16F;
    const LLRenderTextureFormat specular_format =
        viewer_formats ? LLRenderTextureFormat::RGBA : LLRenderTextureFormat::RGBA16F;
    const LLRenderTextureFormat normal_format =
        viewer_formats ? LLRenderTextureFormat::RGBA16 : LLRenderTextureFormat::RGBA16F;
    const LLRenderTextureFormat emissive_format =
        viewer_formats ? LLRenderTextureFormat::RGB16F : LLRenderTextureFormat::RGBA16F;

    if (!create_empty_smoke_texture(
            backend,
            graph.mGBufferColor,
            width,
            height,
            color_format,
            LLRenderPixelFormat::RGBA,
            viewer_formats ? LLRenderPixelType::UnsignedByte : LLRenderPixelType::Float32) ||
        !create_empty_smoke_texture(
            backend,
            graph.mGBufferSpecular,
            width,
            height,
            specular_format,
            LLRenderPixelFormat::RGBA,
            viewer_formats ? LLRenderPixelType::UnsignedByte : LLRenderPixelType::Float32) ||
        !create_empty_smoke_texture(
            backend,
            graph.mGBufferNormal,
            width,
            height,
            normal_format,
            LLRenderPixelFormat::RGBA,
            viewer_formats ? LLRenderPixelType::UnsignedShort : LLRenderPixelType::Float32) ||
        (color_attachment_count >= 4U && !create_empty_smoke_texture(
            backend,
            graph.mGBufferEmissive,
            width,
            height,
            emissive_format,
            LLRenderPixelFormat::RGBA,
            LLRenderPixelType::Float32)) ||
        !create_empty_smoke_texture(
            backend,
            graph.mGBufferDepth,
            width,
            height,
            LLRenderTextureFormat::DepthComponent24,
            LLRenderPixelFormat::DepthComponent,
            LLRenderPixelType::Float32) ||
        !create_empty_smoke_texture(
            backend,
            graph.mDeferredColor,
            width,
            height,
            LLRenderTextureFormat::RGBA16F,
            LLRenderPixelFormat::RGBA,
            LLRenderPixelType::Float32))
    {
        release_smoke_deferred_graph(backend, graph);
        return false;
    }

    graph.mGBufferFramebuffer = backend.createFramebufferHandle();
    graph.mDeferredFramebuffer = backend.createFramebufferHandle();
    if (!graph.mGBufferFramebuffer || !graph.mDeferredFramebuffer)
    {
        release_smoke_deferred_graph(backend, graph);
        return false;
    }

    backend.bindReadWriteFramebuffer(graph.mGBufferFramebuffer);
    backend.attachFramebufferTexture2D(
        LLRenderFramebufferAttachment::Color0,
        LLRenderTextureTarget::Texture2D,
        graph.mGBufferColor,
        0);
    backend.attachFramebufferTexture2D(
        LLRenderFramebufferAttachment::Color1,
        LLRenderTextureTarget::Texture2D,
        graph.mGBufferSpecular,
        0);
    backend.attachFramebufferTexture2D(
        LLRenderFramebufferAttachment::Color2,
        LLRenderTextureTarget::Texture2D,
        graph.mGBufferNormal,
        0);
    if (color_attachment_count >= 4U)
    {
        backend.attachFramebufferTexture2D(
            LLRenderFramebufferAttachment::Color3,
            LLRenderTextureTarget::Texture2D,
            graph.mGBufferEmissive,
            0);
    }
    backend.attachFramebufferTexture2D(
        LLRenderFramebufferAttachment::Depth,
        LLRenderTextureTarget::Texture2D,
        graph.mGBufferDepth,
        0);
    backend.setFramebufferBufferRouting(color_attachment_count);
    if (!backend.isDrawFramebufferComplete())
    {
        release_smoke_deferred_graph(backend, graph);
        return false;
    }

    backend.bindReadWriteFramebuffer(graph.mDeferredFramebuffer);
    backend.attachFramebufferTexture2D(
        LLRenderFramebufferAttachment::Color0,
        LLRenderTextureTarget::Texture2D,
        graph.mDeferredColor,
        0);
    backend.setFramebufferBufferRouting(1);
    if (!backend.isDrawFramebufferComplete())
    {
        release_smoke_deferred_graph(backend, graph);
        return false;
    }

    backend.bindReadWriteFramebuffer(LLRenderFramebufferHandle());
    backend.restoreDefaultFramebufferBufferRouting();
    return true;
}

void bind_deferred_graph_material_textures(
    LLRenderBackend& backend,
    const SmokeDeferredTextures& textures)
{
    const LLRenderTextureHandle bindings[] =
    {
        textures.mDiffuse,
        textures.mNormal,
        textures.mSpecular,
        textures.mEmissive,
    };
    const S32 binding_count =
        static_cast<S32>(sizeof(bindings) / sizeof(bindings[0]));
    for (S32 unit = 0; unit < binding_count; ++unit)
    {
        backend.setActiveTextureUnit(unit);
        backend.bindTexture(LLRenderTextureTarget::Texture2D, bindings[unit]);
    }
    backend.setActiveTextureUnit(0);
}

void bind_deferred_graph_gbuffer_textures(
    LLRenderBackend& backend,
    const SmokeDeferredGraph& graph)
{
    const LLRenderTextureHandle bindings[] =
    {
        graph.mGBufferColor,
        graph.mGBufferSpecular,
        graph.mGBufferNormal,
        graph.mGBufferEmissive,
        graph.mGBufferDepth,
    };
    const S32 binding_count =
        static_cast<S32>(sizeof(bindings) / sizeof(bindings[0]));
    for (S32 unit = 0; unit < binding_count; ++unit)
    {
        backend.setActiveTextureUnit(unit);
        backend.bindTexture(LLRenderTextureTarget::Texture2D, bindings[unit]);
    }
    backend.setActiveTextureUnit(0);
}

void bind_deferred_graph_final_textures(
    LLRenderBackend& backend,
    const SmokeDeferredGraph& graph)
{
    const LLRenderTextureHandle bindings[] =
    {
        graph.mDeferredColor,
        graph.mGBufferColor,
        graph.mGBufferSpecular,
        graph.mGBufferNormal,
        graph.mGBufferEmissive,
        graph.mGBufferDepth,
    };
    const S32 binding_count =
        static_cast<S32>(sizeof(bindings) / sizeof(bindings[0]));
    for (S32 unit = 0; unit < binding_count; ++unit)
    {
        backend.setActiveTextureUnit(unit);
        backend.bindTexture(LLRenderTextureTarget::Texture2D, bindings[unit]);
    }
    backend.setActiveTextureUnit(0);
}

LLRenderWorldMaterialParameters make_deferred_graph_composite_parameters()
{
    LLRenderWorldMaterialParameters parameters;
    parameters.mBaseColorRed = 0.28f;
    parameters.mBaseColorGreen = 0.34f;
    parameters.mBaseColorBlue = 0.43f;
    parameters.mBaseColorAlpha = 1.f;
    parameters.mEmissiveColorRed = 1.f;
    parameters.mEmissiveColorGreen = 0.96f;
    parameters.mEmissiveColorBlue = 0.88f;
    parameters.mSpecularColorRed = 0.35f;
    parameters.mSpecularColorGreen = 0.45f;
    parameters.mSpecularColorBlue = 0.82f;
    parameters.mEnvIntensity = 1.f;
    parameters.mRoughnessFactor = 0.f;
    parameters.mMetallicFactor = 0.f;
    parameters.mMaterialFlags = 4.f;
    parameters.mNormalTextureOffsetS = 1.f;
    parameters.mNormalTextureOffsetT = 8.f;
    parameters.mORMTextureScaleS = 1.f;
    parameters.mORMTextureScaleT = 0.f;
    parameters.mSceneAmbientRed = 0.45f;
    parameters.mSceneAmbientGreen = 0.48f;
    parameters.mSceneAmbientBlue = 0.56f;
    parameters.mSceneLightingValid = 1.f;
    return parameters;
}

LLRenderWorldMaterialParameters make_deferred_graph_final_parameters()
{
    LLRenderWorldMaterialParameters parameters;
    parameters.mBaseColorRed = 1.f;
    parameters.mBaseColorGreen = 1.f / 2.2f;
    parameters.mBaseColorBlue = 1.f;
    parameters.mBaseColorAlpha = 1.f;
    parameters.mRoughnessFactor = 0.f;
    parameters.mMetallicFactor = 0.f;
    parameters.mMaterialFlags = 0.f;
    parameters.mSpecularColorRed = 0.f;
    parameters.mSpecularColorGreen = -1.f;
    parameters.mSpecularColorBlue = 4.f;
    parameters.mEnvIntensity = 0.f;
    parameters.mDiffuseAlphaMode = 0.f;
    parameters.mGLTFAlphaMode = 0.f;
    parameters.mBump = 0.f;
    parameters.mShiny = 0.f;
    return parameters;
}

void draw_deferred_graph_gbuffer_tiles(
    LLRenderBackend& backend,
    const SmokeQuad& quad,
    U32 width,
    U32 height)
{
    const std::array<SmokeWorldPipelineEntry, 6> entries =
    {{
        { LLRenderWorldShaderClass::Textured, "Textured", 0.32f, 0.62f, 0.94f, 0 },
        { LLRenderWorldShaderClass::Terrain, "Terrain", 0.34f, 0.58f, 0.28f, 0 },
        { LLRenderWorldShaderClass::AlphaMask, "AlphaMask", 0.95f, 0.82f, 0.25f, LLRenderWorldMaterialParameters::AlphaMask },
        { LLRenderWorldShaderClass::Material, "Material", 0.78f, 0.58f, 0.95f, LLRenderWorldMaterialParameters::HasSpecularMap },
        { LLRenderWorldShaderClass::PBR, "PBR", 0.90f, 0.72f, 0.48f, LLRenderWorldMaterialParameters::GLTFPBR | LLRenderWorldMaterialParameters::HasORMMap | LLRenderWorldMaterialParameters::HasNormalMap },
        { LLRenderWorldShaderClass::Avatar, "Avatar", 0.86f, 0.52f, 0.44f, 0 },
    }};

    bind_world_smoke_quad(backend, quad);
    backend.setWorldTextureTransform({});
    backend.setWorldTerrainParameters(make_world_pipeline_terrain_parameters());
    backend.setWorldSkinningMatrixPalette(0, nullptr);
    backend.setWorldDrawEnabled(true);
    backend.setCapability(LLRenderCapability::Blend, false);
    backend.setCapability(LLRenderCapability::DepthTest, true);
    backend.setDepthWriteEnabled(true);
    backend.setDepthFunction(LLRenderDepthFunction::LessEqual);
    backend.setCapability(LLRenderCapability::CullFace, false);
    backend.setColorMask({ true, true, true, true });

    constexpr S32 columns = 3;
    constexpr S32 rows = 2;
    const S32 cell_width = llmax(1, static_cast<S32>(width) / columns);
    const S32 cell_height = llmax(1, static_cast<S32>(height) / rows);
    static bool logged_entries = false;
    if (!logged_entries)
    {
        std::cout << "Mare Vulkan smoke deferred-graph G-buffer entries:";
    }
    for (U32 i = 0; i < entries.size(); ++i)
    {
        const SmokeWorldPipelineEntry& entry = entries[i];
        if (!logged_entries)
        {
            std::cout << " " << entry.mName;
        }
        const S32 column = static_cast<S32>(i % columns);
        const S32 row = static_cast<S32>(i / columns);
        const S32 x = column * cell_width;
        const S32 y = row * cell_height;
        const S32 w = (column == columns - 1) ?
            static_cast<S32>(width) - x :
            cell_width;
        const S32 h = (row == rows - 1) ?
            static_cast<S32>(height) - y :
            cell_height;

        backend.setScissor(x, y, w, h);
        backend.setWorldShaderClass(entry.mShaderClass);
        backend.setWorldMaterialParameters(
            make_world_pipeline_material(
                entry.mRed,
                entry.mGreen,
                entry.mBlue,
                1.f,
                entry.mFlags));
        backend.drawArrays(LLRenderPrimitiveType::Triangles, 0, 6);
    }
    if (!logged_entries)
    {
        std::cout << std::endl;
        logged_entries = true;
    }
}

bool render_deferred_graph_frame(
    LLRenderBackend& backend,
    SmokeDeferredTextures& material_textures,
    SmokeDeferredGraph& graph,
    const SmokeQuad& quad,
    U32 width,
    U32 height)
{
    if (!ensure_smoke_deferred_textures(backend, material_textures))
    {
        return false;
    }

    const U32 graph_width = llmax(64U, llmin(width, 960U));
    const U32 graph_height = llmax(
        64U,
        llmin(
            height,
            static_cast<U32>(
                static_cast<double>(graph_width) *
                static_cast<double>(height) /
                static_cast<double>(llmax(1U, width)))));

    if (!ensure_smoke_deferred_graph(
            backend,
            graph,
            graph_width,
            graph_height,
            4,
            false))
    {
        return false;
    }

    backend.bindReadWriteFramebuffer(graph.mGBufferFramebuffer);
    backend.setFramebufferBufferRouting(4);
    backend.setViewport(0, 0, static_cast<S32>(graph_width), static_cast<S32>(graph_height));
    backend.setScissor(0, 0, static_cast<S32>(graph_width), static_cast<S32>(graph_height));
    backend.setClearColor(0.f, 0.f, 0.f, 0.f);
    backend.clear(LL_RENDER_CLEAR_COLOR | LL_RENDER_CLEAR_DEPTH);
    bind_deferred_graph_material_textures(backend, material_textures);
    draw_deferred_graph_gbuffer_tiles(backend, quad, graph_width, graph_height);

    backend.bindReadWriteFramebuffer(graph.mDeferredFramebuffer);
    backend.setFramebufferBufferRouting(1);
    backend.setViewport(0, 0, static_cast<S32>(graph_width), static_cast<S32>(graph_height));
    backend.setScissor(0, 0, static_cast<S32>(graph_width), static_cast<S32>(graph_height));
    backend.setClearColor(0.f, 0.f, 0.f, 1.f);
    backend.clear(LL_RENDER_CLEAR_COLOR);
    bind_deferred_graph_gbuffer_textures(backend, graph);
    bind_world_smoke_quad(backend, quad);
    backend.setCapability(LLRenderCapability::Blend, false);
    backend.setCapability(LLRenderCapability::DepthTest, false);
    backend.setDepthWriteEnabled(false);
    backend.setCapability(LLRenderCapability::CullFace, false);
    backend.setColorMask({ true, true, true, true });
    backend.setWorldDrawEnabled(true);
    backend.setWorldShaderClass(LLRenderWorldShaderClass::DeferredComposite);
    backend.setWorldTerrainParameters({});
    backend.setWorldTextureTransform({});
    backend.setWorldSkinningMatrixPalette(0, nullptr);
    backend.setWorldMaterialParameters(make_deferred_graph_composite_parameters());
    backend.drawArrays(LLRenderPrimitiveType::Triangles, 0, 6);

    backend.bindReadWriteFramebuffer(LLRenderFramebufferHandle());
    backend.restoreDefaultFramebufferBufferRouting();
    backend.setViewport(0, 0, static_cast<S32>(width), static_cast<S32>(height));
    backend.setScissor(0, 0, static_cast<S32>(width), static_cast<S32>(height));
    backend.setClearColor(0.01f, 0.012f, 0.018f, 1.f);
    backend.clear(LL_RENDER_CLEAR_COLOR | LL_RENDER_CLEAR_DEPTH);
    bind_deferred_graph_final_textures(backend, graph);
    bind_world_smoke_quad(backend, quad);
    backend.setWorldShaderClass(LLRenderWorldShaderClass::FinalComposite);
    backend.setWorldMaterialParameters(make_deferred_graph_final_parameters());
    backend.drawArrays(LLRenderPrimitiveType::Triangles, 0, 6);

    backend.setWorldDrawEnabled(false);
    backend.setWorldShaderClass(LLRenderWorldShaderClass::Textured);
    backend.setWorldMaterialParameters({});
    backend.setWorldTerrainParameters({});
    backend.setWorldTextureTransform({});
    backend.setWorldSkinningMatrixPalette(0, nullptr);
    backend.setCapability(LLRenderCapability::Blend, false);
    backend.setScissor(0, 0, static_cast<S32>(width), static_cast<S32>(height));
    return true;
}

bool render_viewer_deferred_direct_frame(
    LLRenderBackend& backend,
    SmokeDeferredTextures& material_textures,
    SmokeDeferredGraph& graph,
    const SmokeQuad& quad,
    U32 width,
    U32 height)
{
    if (!ensure_smoke_deferred_textures(backend, material_textures))
    {
        return false;
    }

    const U32 graph_width = llmax(64U, llmin(width, 960U));
    const U32 graph_height = llmax(
        64U,
        llmin(
            height,
            static_cast<U32>(
                static_cast<double>(graph_width) *
                static_cast<double>(height) /
                static_cast<double>(llmax(1U, width)))));

    if (!ensure_smoke_deferred_graph(
            backend,
            graph,
            graph_width,
            graph_height,
            3,
            true))
    {
        return false;
    }

    backend.bindReadWriteFramebuffer(graph.mGBufferFramebuffer);
    backend.setFramebufferBufferRouting(graph.mColorAttachmentCount);
    backend.setViewport(0, 0, static_cast<S32>(graph_width), static_cast<S32>(graph_height));
    backend.setScissor(0, 0, static_cast<S32>(graph_width), static_cast<S32>(graph_height));
    backend.setClearColor(0.f, 0.f, 0.f, 0.f);
    backend.clear(LL_RENDER_CLEAR_COLOR | LL_RENDER_CLEAR_DEPTH);
    bind_deferred_graph_material_textures(backend, material_textures);
    draw_deferred_graph_gbuffer_tiles(backend, quad, graph_width, graph_height);

    backend.bindReadWriteFramebuffer(LLRenderFramebufferHandle());
    backend.restoreDefaultFramebufferBufferRouting();
    backend.setViewport(0, 0, static_cast<S32>(width), static_cast<S32>(height));
    backend.setScissor(0, 0, static_cast<S32>(width), static_cast<S32>(height));
    backend.setClearColor(0.01f, 0.012f, 0.018f, 1.f);
    backend.clear(LL_RENDER_CLEAR_COLOR | LL_RENDER_CLEAR_DEPTH);
    bind_deferred_graph_gbuffer_textures(backend, graph);
    bind_world_smoke_quad(backend, quad);
    backend.setCapability(LLRenderCapability::Blend, false);
    backend.setCapability(LLRenderCapability::DepthTest, false);
    backend.setDepthWriteEnabled(false);
    backend.setCapability(LLRenderCapability::CullFace, false);
    backend.setColorMask({ true, true, true, true });
    backend.setWorldDrawEnabled(true);
    backend.setWorldShaderClass(LLRenderWorldShaderClass::DeferredComposite);
    backend.setWorldTerrainParameters({});
    backend.setWorldTextureTransform({});
    backend.setWorldSkinningMatrixPalette(0, nullptr);
    LLRenderWorldMaterialParameters parameters =
        make_deferred_graph_composite_parameters();
    parameters.mRoughnessFactor = static_cast<F32>(graph.mColorAttachmentCount);
    backend.setWorldMaterialParameters(parameters);
    backend.drawArrays(LLRenderPrimitiveType::Triangles, 0, 6);

    backend.setWorldDrawEnabled(false);
    backend.setWorldShaderClass(LLRenderWorldShaderClass::Textured);
    backend.setWorldMaterialParameters({});
    backend.setWorldTerrainParameters({});
    backend.setWorldTextureTransform({});
    backend.setWorldSkinningMatrixPalette(0, nullptr);
    backend.setCapability(LLRenderCapability::Blend, false);
    backend.setScissor(0, 0, static_cast<S32>(width), static_cast<S32>(height));
    return true;
}

bool render_viewer_render_target_direct_frame(
    LLRenderBackend& backend,
    SmokeDeferredTextures& material_textures,
    SmokeViewerRenderTargetGraph& graph,
    const SmokeQuad& quad,
    U32 width,
    U32 height)
{
    if (!ensure_smoke_deferred_textures(backend, material_textures))
    {
        return false;
    }

    const U32 graph_width = llmax(64U, llmin(width, 960U));
    const U32 graph_height = llmax(
        64U,
        llmin(
            height,
            static_cast<U32>(
                static_cast<double>(graph_width) *
                static_cast<double>(height) /
                static_cast<double>(llmax(1U, width)))));

    if (!ensure_smoke_viewer_render_target_graph(
            graph,
            graph_width,
            graph_height,
            3))
    {
        return false;
    }

    graph.mDeferredScreen.bindTarget();
    backend.setClearColor(0.f, 0.f, 0.f, 0.f);
    graph.mDeferredScreen.clear(LL_RENDER_CLEAR_COLOR | LL_RENDER_CLEAR_DEPTH);
    bind_deferred_graph_material_textures(backend, material_textures);
    draw_deferred_graph_gbuffer_tiles(backend, quad, graph_width, graph_height);
    graph.mDeferredScreen.flush();

    backend.setViewport(0, 0, static_cast<S32>(width), static_cast<S32>(height));
    backend.setScissor(0, 0, static_cast<S32>(width), static_cast<S32>(height));
    backend.setClearColor(0.01f, 0.012f, 0.018f, 1.f);
    backend.clear(LL_RENDER_CLEAR_COLOR | LL_RENDER_CLEAR_DEPTH);
    for (U32 attachment = 0;
         attachment < graph.mDeferredScreen.getNumTextures();
         ++attachment)
    {
        graph.mDeferredScreen.bindTexture(
            attachment,
            static_cast<S32>(attachment),
            attachment == 0 ?
                LLTexUnit::TFO_BILINEAR :
                LLTexUnit::TFO_POINT);
    }
    backend.setActiveTextureUnit(3);
    backend.bindTexture(LLRenderTextureTarget::Texture2D, LLRenderTextureHandle());
    if (graph.mDeferredScreen.getDepthHandle())
    {
        gGL.getTexUnit(4)->bindManual(
            graph.mDeferredScreen.getUsage(),
            graph.mDeferredScreen.getDepthHandle());
        gGL.getTexUnit(4)->setTextureFilteringOption(LLTexUnit::TFO_POINT);
    }
    backend.setActiveTextureUnit(0);

    bind_world_smoke_quad(backend, quad);
    backend.setCapability(LLRenderCapability::Blend, false);
    backend.setCapability(LLRenderCapability::DepthTest, false);
    backend.setDepthWriteEnabled(false);
    backend.setCapability(LLRenderCapability::CullFace, false);
    backend.setColorMask({ true, true, true, true });
    backend.setWorldDrawEnabled(true);
    backend.setWorldShaderClass(LLRenderWorldShaderClass::DeferredComposite);
    backend.setWorldTerrainParameters({});
    backend.setWorldTextureTransform({});
    backend.setWorldSkinningMatrixPalette(0, nullptr);
    LLRenderWorldMaterialParameters parameters =
        make_deferred_graph_composite_parameters();
    parameters.mRoughnessFactor =
        static_cast<F32>(graph.mDeferredScreen.getNumTextures());
    backend.setWorldMaterialParameters(parameters);
    backend.drawArrays(LLRenderPrimitiveType::Triangles, 0, 6);

    backend.setWorldDrawEnabled(false);
    backend.setWorldShaderClass(LLRenderWorldShaderClass::Textured);
    backend.setWorldMaterialParameters({});
    backend.setWorldTerrainParameters({});
    backend.setWorldTextureTransform({});
    backend.setWorldSkinningMatrixPalette(0, nullptr);
    backend.setCapability(LLRenderCapability::Blend, false);
    backend.setScissor(0, 0, static_cast<S32>(width), static_cast<S32>(height));
    return true;
}

void draw_smoke_viewer_immediate_composite_quad(
    LLRenderBackend& backend,
    LLRenderTarget& deferred_screen,
    const SmokeQuad& quad,
    U32 width,
    U32 height)
{
    SmokeMatrixScope matrix_scope;

    const U32 attachment_count =
        llmin(deferred_screen.getNumTextures(), 4U);
    for (U32 attachment = 0; attachment < attachment_count; ++attachment)
    {
        deferred_screen.bindTexture(
            attachment,
            static_cast<S32>(attachment),
            LLTexUnit::TFO_BILINEAR);
    }

    bool depth_bound = false;
    if (deferred_screen.getDepthHandle())
    {
        depth_bound =
            gGL.getTexUnit(4)->bind(&deferred_screen, true);
    }

    backend.setWorldDrawEnabled(true);
    backend.setWorldShaderClass(LLRenderWorldShaderClass::DeferredComposite);
    backend.setWorldTextureTransform({});
    backend.setWorldTerrainParameters({});
    backend.setWorldSkinningMatrixPalette(0, nullptr);
    LLRenderWorldMaterialParameters parameters =
        make_deferred_graph_composite_parameters();
    parameters.mRoughnessFactor = static_cast<F32>(attachment_count);
    parameters.mNormalTextureOffsetS = depth_bound ? 1.f : 0.f;
    backend.setWorldMaterialParameters(parameters);

    backend.setViewport(0, 0, static_cast<S32>(width), static_cast<S32>(height));
    backend.setScissor(0, 0, static_cast<S32>(width), static_cast<S32>(height));
    backend.setCapability(LLRenderCapability::DepthTest, false);
    backend.setDepthWriteEnabled(false);
    backend.setCapability(LLRenderCapability::Blend, false);
    backend.setCapability(LLRenderCapability::CullFace, false);
    backend.setColorMask({ true, true, true, true });
    gGL.matrixMode(LLRender::MM_PROJECTION);
    gGL.loadIdentity();
    gGL.matrixMode(LLRender::MM_MODELVIEW);
    gGL.loadIdentity();
    bind_world_smoke_quad(backend, quad);
    backend.drawArrays(LLRenderPrimitiveType::Triangles, 0, 6);

    backend.setWorldDrawEnabled(false);
    backend.setWorldShaderClass(LLRenderWorldShaderClass::Textured);
    backend.setWorldMaterialParameters({});
    backend.setWorldTerrainParameters({});
    backend.setWorldTextureTransform({});
    backend.setWorldSkinningMatrixPalette(0, nullptr);

    for (U32 attachment = 0; attachment < attachment_count; ++attachment)
    {
        gGL.getTexUnit(static_cast<S32>(attachment))->unbind(LLTexUnit::TT_TEXTURE);
    }
    if (depth_bound)
    {
        gGL.getTexUnit(4)->unbind(LLTexUnit::TT_TEXTURE);
    }
}

bool render_viewer_immediate_direct_frame(
    LLRenderBackend& backend,
    SmokeDeferredTextures& material_textures,
    SmokeViewerRenderTargetGraph& graph,
    const SmokeQuad& quad,
    U32 width,
    U32 height)
{
    if (!ensure_smoke_deferred_textures(backend, material_textures))
    {
        return false;
    }

    const U32 graph_width = llmax(64U, llmin(width, 960U));
    const U32 graph_height = llmax(
        64U,
        llmin(
            height,
            static_cast<U32>(
                static_cast<double>(graph_width) *
                static_cast<double>(height) /
                static_cast<double>(llmax(1U, width)))));

    if (!ensure_smoke_viewer_render_target_graph(
            graph,
            graph_width,
            graph_height,
            3))
    {
        return false;
    }

    graph.mDeferredScreen.bindTarget();
    backend.setClearColor(0.f, 0.f, 0.f, 0.f);
    graph.mDeferredScreen.clear(LL_RENDER_CLEAR_COLOR | LL_RENDER_CLEAR_DEPTH);
    bind_deferred_graph_material_textures(backend, material_textures);
    draw_deferred_graph_gbuffer_tiles(backend, quad, graph_width, graph_height);
    graph.mDeferredScreen.flush();

    backend.setViewport(0, 0, static_cast<S32>(width), static_cast<S32>(height));
    backend.setScissor(0, 0, static_cast<S32>(width), static_cast<S32>(height));
    backend.setClearColor(0.01f, 0.012f, 0.018f, 1.f);
    backend.clear(LL_RENDER_CLEAR_COLOR | LL_RENDER_CLEAR_DEPTH);
    draw_smoke_viewer_immediate_composite_quad(
        backend,
        graph.mDeferredScreen,
        quad,
        width,
        height);
    backend.setCapability(LLRenderCapability::Blend, false);
    backend.setScissor(0, 0, static_cast<S32>(width), static_cast<S32>(height));
    return true;
}

struct SmokeUIOverlay
{
    LLRenderTextureHandle mTransparentBlack;
    LLRenderTextureHandle mDarkBar;
    LLRenderTextureHandle mPanel;
    LLRenderTextureHandle mAccent;
    LLRenderTextureHandle mWhite;
    LLRenderTextureHandle mChecker;
    LLRenderTextureHandle mLoginSurface;
};

void release_smoke_ui_overlay(
    LLRenderBackend& backend,
    SmokeUIOverlay& overlay)
{
    LLRenderTextureHandle* handles[] =
    {
        &overlay.mTransparentBlack,
        &overlay.mDarkBar,
        &overlay.mPanel,
        &overlay.mAccent,
        &overlay.mWhite,
        &overlay.mChecker,
        &overlay.mLoginSurface,
    };
    for (LLRenderTextureHandle* handle : handles)
    {
        if (*handle)
        {
            backend.deleteTextureHandle(*handle);
            *handle = {};
        }
    }
}

std::vector<U8> make_checker_rgba_pixels()
{
    constexpr U32 width = 4;
    constexpr U32 height = 4;
    std::vector<U8> pixels;
    pixels.resize(width * height * 4U);
    for (U32 y = 0; y < height; ++y)
    {
        for (U32 x = 0; x < width; ++x)
        {
            const bool bright = ((x + y) & 1U) == 0;
            const size_t offset = (static_cast<size_t>(y) * width + x) * 4U;
            pixels[offset + 0] = bright ? 240 : 70;
            pixels[offset + 1] = bright ? 248 : 120;
            pixels[offset + 2] = bright ? 255 : 210;
            pixels[offset + 3] = 255;
        }
    }
    return pixels;
}

std::vector<U8> make_login_surface_rgba_pixels()
{
    constexpr U32 width = 512;
    constexpr U32 height = 320;
    std::vector<U8> pixels(width * height * 4U, 255);

    auto fill_rect =
        [&](U32 left, U32 bottom, U32 right, U32 top, U8 red, U8 green, U8 blue, U8 alpha)
        {
            left = llmin(left, width);
            right = llmin(right, width);
            bottom = llmin(bottom, height);
            top = llmin(top, height);
            for (U32 y = bottom; y < top; ++y)
            {
                for (U32 x = left; x < right; ++x)
                {
                    const size_t offset = (static_cast<size_t>(y) * width + x) * 4U;
                    pixels[offset + 0] = red;
                    pixels[offset + 1] = green;
                    pixels[offset + 2] = blue;
                    pixels[offset + 3] = alpha;
                }
            }
        };

    fill_rect(0, 0, width, height, 238, 241, 245, 255);
    fill_rect(0, height - 58, width, height, 39, 49, 65, 255);
    fill_rect(28, height - 38, 188, height - 26, 246, 248, 252, 255);
    fill_rect(32, 72, width - 32, height - 86, 255, 255, 255, 255);
    fill_rect(58, 206, 282, 218, 70, 86, 106, 255);
    fill_rect(58, 176, 430, 186, 156, 167, 182, 255);
    fill_rect(58, 150, 396, 160, 184, 193, 204, 255);
    fill_rect(58, 106, 236, 130, 42, 139, 242, 255);
    fill_rect(284, 104, 454, 132, 232, 238, 247, 255);

    for (U32 y = 0; y < height; ++y)
    {
        for (U32 x = 0; x < width; ++x)
        {
            if (((x / 16U) + (y / 16U)) % 7U == 0U)
            {
                const size_t offset = (static_cast<size_t>(y) * width + x) * 4U;
                pixels[offset + 0] = static_cast<U8>(llmin(255, pixels[offset + 0] + 5));
                pixels[offset + 1] = static_cast<U8>(llmin(255, pixels[offset + 1] + 7));
                pixels[offset + 2] = static_cast<U8>(llmin(255, pixels[offset + 2] + 9));
            }
        }
    }

    return pixels;
}

bool ensure_smoke_ui_overlay(
    LLRenderBackend& backend,
    SmokeUIOverlay& overlay)
{
    if (overlay.mTransparentBlack &&
        overlay.mDarkBar &&
        overlay.mPanel &&
        overlay.mAccent &&
        overlay.mWhite &&
        overlay.mChecker &&
        overlay.mLoginSurface)
    {
        return true;
    }

    release_smoke_ui_overlay(backend, overlay);
    if (!create_smoke_texture(
            backend,
            overlay.mTransparentBlack,
            1,
            1,
            make_solid_rgba_pixels(1, 1, 0, 0, 0, 0)) ||
        !create_smoke_texture(
            backend,
            overlay.mDarkBar,
            1,
            1,
            make_solid_rgba_pixels(1, 1, 12, 16, 22, 226)) ||
        !create_smoke_texture(
            backend,
            overlay.mPanel,
            1,
            1,
            make_solid_rgba_pixels(1, 1, 30, 36, 46, 208)) ||
        !create_smoke_texture(
            backend,
            overlay.mAccent,
            1,
            1,
            make_solid_rgba_pixels(1, 1, 45, 139, 255, 236)) ||
        !create_smoke_texture(
            backend,
            overlay.mWhite,
            1,
            1,
            make_solid_rgba_pixels(1, 1, 255, 255, 255, 255)) ||
        !create_smoke_texture(
            backend,
            overlay.mChecker,
            4,
            4,
            make_checker_rgba_pixels()) ||
        !create_smoke_texture(
            backend,
            overlay.mLoginSurface,
            512,
            320,
            make_login_surface_rgba_pixels()))
    {
        release_smoke_ui_overlay(backend, overlay);
        return false;
    }
    return true;
}

void draw_smoke_ui_color_rect(
    LLRenderBackend& backend,
    const LLColor4& color,
    S32 x,
    S32 y,
    S32 width,
    S32 height)
{
    if (width <= 0 || height <= 0)
    {
        return;
    }

    backend.setScissor(x, y, width, height);
    gl_rect_2d(x, y + height, x + width, y, color, true);
}

void draw_smoke_ui_texture_rect(
    LLRenderBackend& backend,
    LLRenderTextureHandle texture,
    S32 x,
    S32 y,
    S32 width,
    S32 height)
{
    if (width <= 0 || height <= 0)
    {
        return;
    }

    backend.setScissor(x, y, width, height);
    gGL.getTexUnit(0)->bindManual(
        LLTexUnit::TT_TEXTURE,
        texture,
        false,
        true);

    const F32 left = static_cast<F32>(x);
    const F32 right = static_cast<F32>(x + width);
    const F32 bottom = static_cast<F32>(y);
    const F32 top = static_cast<F32>(y + height);

    gGL.begin(LLRender::TRIANGLES);
    gGL.color4f(1.f, 1.f, 1.f, 1.f);
    gGL.texCoord2f(0.f, 0.f);
    gGL.vertex2f(left, bottom);
    gGL.texCoord2f(1.f, 0.f);
    gGL.vertex2f(right, bottom);
    gGL.texCoord2f(0.f, 1.f);
    gGL.vertex2f(left, top);
    gGL.texCoord2f(0.f, 1.f);
    gGL.vertex2f(left, top);
    gGL.texCoord2f(1.f, 0.f);
    gGL.vertex2f(right, bottom);
    gGL.texCoord2f(1.f, 1.f);
    gGL.vertex2f(right, top);
    gGL.end();
}

bool render_smoke_ui_overlay(
    LLRenderBackend& backend,
    SmokeUIOverlay& overlay,
    U32 width,
    U32 height)
{
    if (!ensure_smoke_ui_overlay(backend, overlay))
    {
        return false;
    }

    const S32 screen_width = static_cast<S32>(width);
    const S32 screen_height = static_cast<S32>(height);
    const S32 menu_height = llmax(24, screen_height / 18);
    const S32 bottom_height = llmax(34, screen_height / 12);
    const S32 panel_width = llmax(170, screen_width / 4);
    const S32 panel_height = llmax(120, screen_height / 3);
    const S32 margin = llmax(12, screen_width / 48);
    const S32 checker_size = llmax(56, llmin(screen_width, screen_height) / 7);

    SmokeMatrixScope matrix_scope;
    const glm::mat4 projection = glm::ortho(
        0.f,
        static_cast<F32>(screen_width),
        0.f,
        static_cast<F32>(screen_height),
        -1.f,
        1.f);

    backend.bindReadWriteFramebuffer(LLRenderFramebufferHandle());
    backend.restoreDefaultFramebufferBufferRouting();
    backend.setViewport(0, 0, screen_width, screen_height);
    backend.setScissor(0, 0, screen_width, screen_height);
    backend.setWorldDrawEnabled(false);
    backend.setCapability(LLRenderCapability::DepthTest, false);
    backend.setDepthWriteEnabled(false);
    backend.setCapability(LLRenderCapability::CullFace, false);
    backend.setCapability(LLRenderCapability::Blend, true);
    backend.setBlendState(
        {
            LLRenderBlendFactor::SourceAlpha,
            LLRenderBlendFactor::OneMinusSourceAlpha,
            LLRenderBlendFactor::One,
            LLRenderBlendFactor::OneMinusSourceAlpha,
        });
    backend.setColorMask({ true, true, true, true });

    gGL.matrixMode(LLRender::MM_PROJECTION);
    gGL.loadMatrix(glm::value_ptr(projection));
    gGL.matrixMode(LLRender::MM_MODELVIEW);
    gGL.loadIdentity();
    gUIProgram.mAttributeMask =
        LLVertexBuffer::MAP_VERTEX |
        LLVertexBuffer::MAP_TEXCOORD0 |
        LLVertexBuffer::MAP_COLOR;
    gUIProgram.bind();

    // This probe should be invisible. If it turns the frame black, UI alpha
    // blending or state isolation after the world composite is broken.
    draw_smoke_ui_texture_rect(
        backend,
        overlay.mTransparentBlack,
        0,
        0,
        screen_width,
        screen_height);
    draw_smoke_ui_color_rect(
        backend,
        LLColor4(12.f / 255.f, 16.f / 255.f, 22.f / 255.f, 226.f / 255.f),
        0,
        screen_height - menu_height,
        screen_width,
        menu_height);
    draw_smoke_ui_color_rect(
        backend,
        LLColor4(30.f / 255.f, 36.f / 255.f, 46.f / 255.f, 208.f / 255.f),
        margin,
        screen_height - menu_height - panel_height - margin,
        panel_width,
        panel_height);
    draw_smoke_ui_color_rect(
        backend,
        LLColor4(12.f / 255.f, 16.f / 255.f, 22.f / 255.f, 226.f / 255.f),
        0,
        0,
        screen_width,
        bottom_height);
    draw_smoke_ui_color_rect(
        backend,
        LLColor4(45.f / 255.f, 139.f / 255.f, 1.f, 236.f / 255.f),
        margin,
        margin,
        llmax(80, screen_width / 6),
        llmax(8, bottom_height / 5));
    draw_smoke_ui_color_rect(
        backend,
        LLColor4::white,
        margin * 2,
        screen_height - menu_height + llmax(4, menu_height / 5),
        llmax(100, screen_width / 8),
        llmax(3, menu_height / 12));
    draw_smoke_ui_texture_rect(
        backend,
        overlay.mChecker,
        screen_width - checker_size - margin,
        bottom_height + margin,
        checker_size,
        checker_size);

    gGL.flush();
    gUIProgram.unbind();
    backend.setScissor(0, 0, screen_width, screen_height);
    backend.setCapability(LLRenderCapability::Blend, false);
    backend.setActiveTextureUnit(0);
    return true;
}

bool render_smoke_viewer_ui_sequence(
    LLRenderBackend& backend,
    SmokeUIOverlay& overlay,
    U32 width,
    U32 height)
{
    if (!ensure_smoke_ui_overlay(backend, overlay))
    {
        return false;
    }

    const S32 screen_width = static_cast<S32>(width);
    const S32 screen_height = static_cast<S32>(height);
    const S32 margin = llmax(16, screen_width / 60);
    const S32 menu_height = llmax(28, screen_height / 24);
    const S32 status_height = llmax(38, screen_height / 16);
    const S32 cef_width = llmin(screen_width - margin * 2, llmax(640, screen_width * 3 / 5));
    const S32 cef_height = llmin(screen_height - menu_height - status_height - margin * 3, llmax(360, screen_height * 3 / 5));
    const S32 cef_x = (screen_width - cef_width) / 2;
    const S32 cef_y = status_height + margin;

    SmokeMatrixScope matrix_scope;
    backend.bindReadWriteFramebuffer(LLRenderFramebufferHandle());
    backend.restoreDefaultFramebufferBufferRouting();
    backend.setViewport(0, 0, screen_width, screen_height);
    backend.setScissor(0, 0, screen_width, screen_height);
    backend.setWorldDrawEnabled(false);
    backend.setPolygonMode(
        LLRenderPolygonFace::FrontAndBack,
        LLRenderPolygonMode::Fill);
    backend.setColorMask({ true, true, true, true });
    backend.setBlendState(
        {
            LLRenderBlendFactor::SourceAlpha,
            LLRenderBlendFactor::OneMinusSourceAlpha,
            LLRenderBlendFactor::One,
            LLRenderBlendFactor::OneMinusSourceAlpha,
        });

    LLGLSUIDefault gls_ui;
    backend.setDepthWriteEnabled(false);
    gl_state_for_2d(screen_width, screen_height);

    gUIProgram.mAttributeMask =
        LLVertexBuffer::MAP_VERTEX |
        LLVertexBuffer::MAP_TEXCOORD0 |
        LLVertexBuffer::MAP_COLOR;
    gUIProgram.bind();
    gGL.color4f(1.f, 1.f, 1.f, 1.f);
    gGL.setColorMask(true, true);

    // This matches the suspicious post-world UI ordering: a full-screen,
    // zero-alpha draw, followed by opaque CEF-like content and translucent
    // chrome. If alpha/blend/state isolation regresses, the final readback
    // should go dark here instead of requiring a real login session.
    draw_smoke_ui_texture_rect(
        backend,
        overlay.mTransparentBlack,
        0,
        0,
        screen_width,
        screen_height);

    {
        LLGLDisable blend(LLRenderCapability::Blend);
        draw_smoke_ui_texture_rect(
            backend,
            overlay.mLoginSurface,
            cef_x,
            cef_y,
            cef_width,
            cef_height);
    }

    draw_smoke_ui_color_rect(
        backend,
        LLColor4(18.f / 255.f, 23.f / 255.f, 31.f / 255.f, 218.f / 255.f),
        0,
        screen_height - menu_height,
        screen_width,
        menu_height);
    draw_smoke_ui_color_rect(
        backend,
        LLColor4(10.f / 255.f, 12.f / 255.f, 16.f / 255.f, 196.f / 255.f),
        0,
        0,
        screen_width,
        status_height);

    {
        LLGLEnable scissor(LLRenderCapability::ScissorTest);
        backend.setScissor(
            cef_x + margin,
            cef_y + margin,
            llmax(1, cef_width - margin * 2),
            llmax(1, cef_height - margin * 2));
        draw_smoke_ui_color_rect(
            backend,
            LLColor4(1.f, 1.f, 1.f, 70.f / 255.f),
            cef_x + margin * 2,
            cef_y + cef_height - margin * 3,
            llmax(80, cef_width / 3),
            llmax(5, cef_height / 48));
        draw_smoke_ui_color_rect(
            backend,
            LLColor4(44.f / 255.f, 135.f / 255.f, 245.f / 255.f, 220.f / 255.f),
            cef_x + margin * 2,
            cef_y + margin * 2,
            llmax(120, cef_width / 5),
            llmax(18, status_height / 2));
    }

    draw_smoke_ui_color_rect(
        backend,
        LLColor4::white,
        margin,
        screen_height - menu_height + llmax(5, menu_height / 4),
        llmax(110, screen_width / 10),
        llmax(3, menu_height / 10));
    draw_smoke_ui_color_rect(
        backend,
        LLColor4(44.f / 255.f, 135.f / 255.f, 245.f / 255.f, 235.f / 255.f),
        margin,
        margin,
        llmax(160, screen_width / 5),
        llmax(8, status_height / 5));

    gGL.flush();
    gUIProgram.unbind();
    gGL.setColorMask(true, true);
    backend.setScissor(0, 0, screen_width, screen_height);
    backend.setCapability(LLRenderCapability::Blend, false);
    backend.setActiveTextureUnit(0);
    return true;
}
}

int main(int argc, char** argv)
{
    SmokeOptions options;
    if (!parse_smoke_options(argc, argv, options))
    {
        print_smoke_usage(argv[0] ? argv[0] : "mare-vulkan-smoke");
        return 1;
    }
    if (options.mHelp)
    {
        print_smoke_usage(argv[0] ? argv[0] : "mare-vulkan-smoke");
        return 0;
    }
    if (!options.mVulkanSDK.empty())
    {
        setenv("VULKAN_SDK", options.mVulkanSDK.c_str(), 1);
    }
    setenv("MARE_RENDER_BACKEND", "vulkan", 0);

    void* window = mare_vulkan_smoke_create_window(
        960,
        540,
        "Mare Vulkan Smoke");
    if (!window)
    {
        std::cerr << "Failed to create smoke test window.\n";
        return 1;
    }

    LLRenderBackend& backend = getRenderBackend();
    if (backend.getType() != LLRenderBackendType::Vulkan)
    {
        std::cerr << "Expected Vulkan backend, got " << backend.getName() << ".\n";
        mare_vulkan_smoke_destroy_window(window);
        return 2;
    }

    LLRenderNativeContext context;
    LLRenderNativeContextDesc desc;
    desc.mWindow = window;
    desc.mSamples = 0;
    desc.mEnableVSync = true;

    if (!backend.createNativeContext(desc, context))
    {
        std::cerr << "Failed to create Vulkan native context.\n";
        mare_vulkan_smoke_destroy_window(window);
        return 3;
    }

    if (!backend.makeNativeContextCurrent(context.mContext))
    {
        std::cerr << "Failed to make Vulkan native context current.\n";
        backend.destroyNativeContext(context);
        mare_vulkan_smoke_destroy_window(window);
        return 4;
    }

    setenv("MARE_VULKAN_SMOKE_TEST", "1", 0);
    if (options.mStdoutReadback)
    {
        setenv("MARE_VULKAN_SMOKE_STDOUT_READBACK", "1", 1);
    }
    else
    {
        unsetenv("MARE_VULKAN_SMOKE_STDOUT_READBACK");
    }
    if (options.mBufferReadback)
    {
        setenv("MARE_VULKAN_DEBUG_BUFFER_AVERAGE", "1", 1);
    }
    else
    {
        unsetenv("MARE_VULKAN_DEBUG_BUFFER_AVERAGE");
    }
    const std::string readback_frame_limit =
        std::to_string(options.mReadbackFrameLimit);
    setenv(
        "MARE_VULKAN_DEBUG_BUFFER_AVERAGE_FRAMES",
        readback_frame_limit.c_str(),
        1);

    const SmokeMode smoke_mode = options.mMode;
    bool smoke_immediate_render_initialized = false;
    if (smoke_mode == SmokeMode::ViewerImmediateDirect ||
        options.mRenderUI)
    {
        LLVertexBuffer::initClass(nullptr);
        if (!gGL.init(true))
        {
            std::cerr << "Failed to initialize LLRender immediate buffers for Vulkan smoke.\n";
            LLVertexBuffer::cleanupClass();
            backend.destroyNativeContext(context);
            mare_vulkan_smoke_destroy_window(window);
            return 5;
        }
        gUIProgram.mName = "Mare Vulkan smoke UI";
        gUIProgram.mAttributeMask =
            LLVertexBuffer::MAP_VERTEX |
            LLVertexBuffer::MAP_TEXCOORD0 |
            LLVertexBuffer::MAP_COLOR;
        smoke_immediate_render_initialized = true;
    }

    const int frame_limit = options.mFrameLimit;
    const int log_interval = options.mLogInterval;
    SmokeQuad smoke_quad;
    SmokeOffscreen smoke_offscreen;
    SmokeDeferredTextures smoke_deferred_textures;
    SmokeDeferredGraph smoke_deferred_graph;
    SmokeViewerRenderTargetGraph smoke_viewer_render_target_graph;
    SmokeUIOverlay smoke_ui_overlay;
    if ((smoke_mode == SmokeMode::OffscreenCopy ||
            smoke_mode == SmokeMode::DeferredComposite ||
            smoke_mode == SmokeMode::DeferredGraph ||
            smoke_mode == SmokeMode::ViewerDeferredDirect ||
            smoke_mode == SmokeMode::ViewerRenderTargetDirect ||
            smoke_mode == SmokeMode::ViewerImmediateDirect ||
            smoke_mode == SmokeMode::WorldPipelines) &&
        !create_smoke_quad(backend, smoke_quad))
    {
        std::cerr << "Failed to create Vulkan smoke quad.\n";
        backend.destroyNativeContext(context);
        mare_vulkan_smoke_destroy_window(window);
        return 5;
    }
    int frame = 0;
    auto start = std::chrono::steady_clock::now();

    std::cout
        << "Mare Vulkan smoke started. Mode: "
        << get_smoke_mode_name(smoke_mode)
        << ". Expected output: animated blue clear color"
        << get_smoke_mode_description(smoke_mode)
        << "Log interval: every "
        << log_interval
        << " frame(s). Synthetic UI: "
        << (options.mRenderUI ? "on" : "off")
        << ", viewer UI sequence: "
        << (options.mRenderViewerUISequence ? "on" : "off")
        << "."
        << std::endl;
    if (options.mRenderUI)
    {
        std::cout
            << "Mare Vulkan smoke UI overlay enabled: draws a zero-alpha fullscreen probe, "
            << "translucent bars/panel, opaque strokes, and a textured checker after the world/deferred pass."
            << std::endl;
    }
    if (options.mRenderViewerUISequence)
    {
        std::cout
            << "Mare Vulkan smoke viewer UI sequence enabled: replays a post-world 2D setup, "
            << "LLGLSUIDefault state, gl_rect_2d UI chrome, and a CEF-like textured surface."
            << std::endl;
    }

    while (mare_vulkan_smoke_pump_events(window) &&
           (frame_limit == 0 || frame < frame_limit))
    {
        unsigned int width = 1;
        unsigned int height = 1;
        mare_vulkan_smoke_get_view_size(context.mView, &width, &height);

        const auto now = std::chrono::steady_clock::now();
        const double seconds =
            std::chrono::duration<double>(now - start).count();
        const float pulse =
            0.5f + 0.5f * static_cast<float>(std::sin(seconds * 1.7));
        const float clear_red = 0.05f + 0.08f * pulse;
        const float clear_green = 0.32f + 0.10f * pulse;
        const float clear_blue = 0.78f + 0.16f * pulse;
        const float clear_alpha = 1.f;

        if (frame == 0 || (frame % log_interval) == 0)
        {
            std::cout
                << "Mare Vulkan smoke frame "
                << frame
                << ": submitted clear rgba float "
                << std::fixed
                << std::setprecision(4)
                << clear_red
                << ", "
                << clear_green
                << ", "
                << clear_blue
                << ", "
                << clear_alpha
                << " rgb8 "
                << to_color_byte(clear_red)
                << ","
                << to_color_byte(clear_green)
                << ","
                << to_color_byte(clear_blue)
                << " size "
                << width
                << "x"
                << height
                << std::endl;
        }

        if (smoke_mode == SmokeMode::OffscreenCopy)
        {
            if (!render_offscreen_copy_frame(
                    backend,
                    smoke_offscreen,
                    smoke_quad,
                    width,
                    height,
                    clear_red,
                    clear_green,
                    clear_blue,
                    clear_alpha))
            {
                std::cerr << "Failed to render Vulkan smoke offscreen-copy frame.\n";
                break;
            }
        }
        else if (smoke_mode == SmokeMode::DeferredComposite)
        {
            if (!render_deferred_composite_frame(
                    backend,
                    smoke_deferred_textures,
                    smoke_quad,
                    width,
                    height))
            {
                std::cerr << "Failed to render Vulkan smoke deferred-composite frame.\n";
                break;
            }
        }
        else if (smoke_mode == SmokeMode::DeferredGraph)
        {
            if (!render_deferred_graph_frame(
                    backend,
                    smoke_deferred_textures,
                    smoke_deferred_graph,
                    smoke_quad,
                    width,
                    height))
            {
                std::cerr << "Failed to render Vulkan smoke deferred-graph frame.\n";
                break;
            }
        }
        else if (smoke_mode == SmokeMode::ViewerDeferredDirect)
        {
            if (!render_viewer_deferred_direct_frame(
                    backend,
                    smoke_deferred_textures,
                    smoke_deferred_graph,
                    smoke_quad,
                    width,
                    height))
            {
                std::cerr << "Failed to render Vulkan smoke viewer-deferred-direct frame.\n";
                break;
            }
        }
        else if (smoke_mode == SmokeMode::ViewerRenderTargetDirect)
        {
            if (!render_viewer_render_target_direct_frame(
                    backend,
                    smoke_deferred_textures,
                    smoke_viewer_render_target_graph,
                    smoke_quad,
                    width,
                    height))
            {
                std::cerr << "Failed to render Vulkan smoke viewer-render-target-direct frame.\n";
                break;
            }
        }
        else if (smoke_mode == SmokeMode::ViewerImmediateDirect)
        {
            if (!render_viewer_immediate_direct_frame(
                    backend,
                    smoke_deferred_textures,
                    smoke_viewer_render_target_graph,
                    smoke_quad,
                    width,
                    height))
            {
                std::cerr << "Failed to render Vulkan smoke viewer-immediate-direct frame.\n";
                break;
            }
        }
        else if (smoke_mode == SmokeMode::WorldPipelines)
        {
            if (!render_world_pipelines_frame(
                    backend,
                    smoke_deferred_textures,
                    smoke_quad,
                    width,
                    height))
            {
                std::cerr << "Failed to render Vulkan smoke world-pipelines frame.\n";
                break;
            }
        }
        else
        {
            backend.setViewport(0, 0, static_cast<S32>(width), static_cast<S32>(height));
            backend.setScissor(0, 0, static_cast<S32>(width), static_cast<S32>(height));
            backend.setClearColor(
                clear_red,
                clear_green,
                clear_blue,
                clear_alpha);
            backend.clear(LL_RENDER_CLEAR_COLOR | LL_RENDER_CLEAR_DEPTH);
        }
        if (options.mRenderViewerUISequence)
        {
            if (!render_smoke_viewer_ui_sequence(
                    backend,
                    smoke_ui_overlay,
                    width,
                    height))
            {
                std::cerr << "Failed to render Vulkan smoke viewer UI sequence.\n";
                break;
            }
        }
        else if (options.mRenderUI &&
            !render_smoke_ui_overlay(
                backend,
                smoke_ui_overlay,
                width,
                height))
        {
            std::cerr << "Failed to render Vulkan smoke UI overlay.\n";
            break;
        }
        backend.swapNativeBuffers(context.mContext);

        std::this_thread::sleep_for(std::chrono::milliseconds(16));
        ++frame;
    }

    release_smoke_viewer_render_target_graph(smoke_viewer_render_target_graph);
    release_smoke_deferred_graph(backend, smoke_deferred_graph);
    release_smoke_deferred_textures(backend, smoke_deferred_textures);
    release_smoke_ui_overlay(backend, smoke_ui_overlay);
    if (smoke_quad.mVertexBuffer)
    {
        backend.deleteBufferHandle(smoke_quad.mVertexBuffer);
    }
    release_smoke_offscreen(backend, smoke_offscreen);
    if (smoke_immediate_render_initialized)
    {
        gGL.shutdown();
        LLVertexBuffer::cleanupClass();
    }
    backend.destroyNativeContext(context);
    mare_vulkan_smoke_destroy_window(window);
    return 0;
}
