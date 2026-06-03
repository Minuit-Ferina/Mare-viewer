#include "linden_common.h"

#include "llglslshader.h"
#include "llrender.h"
#include "llrender2dutils.h"
#include "llrenderbackend.h"
#include "llvertexbuffer.h"
#include "llvulkancompositeparams.h"

#include "mare_vulkan_test_support.h"

#include <array>
#include <chrono>
#include <cstddef>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <limits>
#include <string>
#include <thread>
#include <vector>

namespace
{
template <typename T>
void append_scene_bytes(std::vector<U8>& bytes, const T& value)
{
    const U8* begin = reinterpret_cast<const U8*>(&value);
    bytes.insert(bytes.end(), begin, begin + sizeof(T));
}

struct SceneWorldQuad
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

struct SceneTestOptions
{
    std::string mBackend = "vulkan";
    std::string mScene = "clear";
    std::string mRenderPath = "immediate";
    std::string mQuality = "class1";
    std::string mVulkanSDK;
    std::string mScreenshotPPMPath;
    int mScreenshotMinFrame = 0;
    int mWidth = 960;
    int mHeight = 540;
    int mFrameLimit = 1;
    bool mHelp = false;
};

struct SceneTestResources
{
    SceneWorldQuad mWorldQuad;
    LLRenderBufferHandle mDirectVertexBuffer;
    LLRenderProgramHandle mDirectProgram;
    LLRenderShaderHandle mDirectVertexShader;
    LLRenderShaderHandle mDirectFragmentShader;
    LLRenderTextureHandle mWhiteTexture;
    LLRenderTextureHandle mCheckerTexture;
    LLRenderTextureHandle mRoadTexture;
    LLRenderTextureHandle mAlphaTexture;
    LLRenderTextureHandle mGBufferColor;
    LLRenderTextureHandle mGBufferSpecular;
    LLRenderTextureHandle mGBufferNormal;
    LLRenderTextureHandle mGBufferEmissive;
    LLRenderTextureHandle mGBufferDepth;
    LLRenderTextureHandle mDeferredColor;
    LLRenderTextureHandle mExposureMap;
    LLRenderFramebufferHandle mGBufferFramebuffer;
    LLRenderFramebufferHandle mDeferredFramebuffer;
    U32 mDeferredWidth = 0;
    U32 mDeferredHeight = 0;
};

struct SceneDirectVertex
{
    F32 mPosition[2];
    F32 mTexCoord[2];
    U8 mColor[4];
};

void print_usage(const char* executable)
{
    std::cerr
        << "Usage: "
        << executable
        << " [--backend vulkan|opengl] [--scene clear|primitives|complete] "
        << "[--render-path immediate|deferred] "
        << "[--quality class1|class2|class3] [--frames count] "
        << "[--width pixels] [--height pixels] [--vulkan-sdk path] "
        << "[--screenshot-ppm path] [--screenshot-min-frame count]\n";
}

S32 scene_test_quality_level(const std::string& quality)
{
    if (quality == "class3" || quality == "3")
    {
        return 3;
    }
    if (quality == "class2" || quality == "2")
    {
        return 2;
    }
    return 1;
}

const char* scene_test_quality_name(S32 level)
{
    switch (level)
    {
        case 3:
            return "class3";
        case 2:
            return "class2";
        default:
            return "class1";
    }
}

bool parse_positive_int(const char* value, const char* option, int& output)
{
    if (!value || !*value)
    {
        std::cerr << option << " requires a positive integer.\n";
        return false;
    }

    char* end = nullptr;
    const long parsed = std::strtol(value, &end, 10);
    if (*end != '\0' || parsed <= 0 || parsed > std::numeric_limits<int>::max())
    {
        std::cerr << "Invalid " << option << " value: " << value << ".\n";
        return false;
    }

    output = static_cast<int>(parsed);
    return true;
}

bool parse_nonnegative_int(const char* value, const char* option, int& output)
{
    if (!value || !*value)
    {
        std::cerr << option << " requires a nonnegative integer.\n";
        return false;
    }

    char* end = nullptr;
    const long parsed = std::strtol(value, &end, 10);
    if (*end != '\0' || parsed < 0 || parsed > std::numeric_limits<int>::max())
    {
        std::cerr << "Invalid " << option << " value: " << value << ".\n";
        return false;
    }

    output = static_cast<int>(parsed);
    return true;
}

bool parse_scene_test_options(int argc, char** argv, SceneTestOptions& options)
{
    for (int i = 1; i < argc; ++i)
    {
        const char* argument = argv[i];
        const auto require_value = [&](const char* option) -> const char*
        {
            if (i + 1 >= argc)
            {
                std::cerr << option << " requires a value.\n";
                return nullptr;
            }
            return argv[++i];
        };

        if (std::strcmp(argument, "--help") == 0 ||
            std::strcmp(argument, "-h") == 0)
        {
            options.mHelp = true;
            return true;
        }
        if (std::strcmp(argument, "--backend") == 0)
        {
            const char* value = require_value("--backend");
            if (!value)
            {
                return false;
            }
            if (std::strcmp(value, "vulkan") != 0 &&
                std::strcmp(value, "opengl") != 0)
            {
                std::cerr << "--backend must be vulkan or opengl.\n";
                return false;
            }
            options.mBackend = value;
            continue;
        }
        if (std::strcmp(argument, "--vulkan-sdk") == 0)
        {
            const char* value = require_value("--vulkan-sdk");
            if (!value)
            {
                return false;
            }
            options.mVulkanSDK = value;
            continue;
        }
        if (std::strcmp(argument, "--scene") == 0)
        {
            const char* value = require_value("--scene");
            if (!value)
            {
                return false;
            }
            if (std::strcmp(value, "clear") != 0 &&
                std::strcmp(value, "primitives") != 0 &&
                std::strcmp(value, "complete") != 0)
            {
                std::cerr << "--scene must be clear, primitives, or complete.\n";
                return false;
            }
            options.mScene = value;
            continue;
        }
        if (std::strcmp(argument, "--render-path") == 0)
        {
            const char* value = require_value("--render-path");
            if (!value)
            {
                return false;
            }
            if (std::strcmp(value, "immediate") != 0 &&
                std::strcmp(value, "deferred") != 0)
            {
                std::cerr << "--render-path must be immediate or deferred.\n";
                return false;
            }
            options.mRenderPath = value;
            continue;
        }
        if (std::strcmp(argument, "--quality") == 0)
        {
            const char* value = require_value("--quality");
            if (!value)
            {
                return false;
            }
            if (std::strcmp(value, "class1") != 0 &&
                std::strcmp(value, "class2") != 0 &&
                std::strcmp(value, "class3") != 0 &&
                std::strcmp(value, "1") != 0 &&
                std::strcmp(value, "2") != 0 &&
                std::strcmp(value, "3") != 0)
            {
                std::cerr << "--quality must be class1, class2, or class3.\n";
                return false;
            }
            options.mQuality = scene_test_quality_name(
                scene_test_quality_level(value));
            continue;
        }
        if (std::strcmp(argument, "--screenshot-ppm") == 0)
        {
            const char* value = require_value("--screenshot-ppm");
            if (!value)
            {
                return false;
            }
            options.mScreenshotPPMPath = value;
            continue;
        }
        if (std::strcmp(argument, "--screenshot-min-frame") == 0)
        {
            const char* value = require_value("--screenshot-min-frame");
            if (!parse_nonnegative_int(
                    value,
                    "--screenshot-min-frame",
                    options.mScreenshotMinFrame))
            {
                return false;
            }
            continue;
        }
        if (std::strcmp(argument, "--frames") == 0)
        {
            const char* value = require_value("--frames");
            if (!parse_nonnegative_int(value, "--frames", options.mFrameLimit))
            {
                return false;
            }
            continue;
        }
        if (std::strcmp(argument, "--width") == 0)
        {
            const char* value = require_value("--width");
            if (!parse_positive_int(value, "--width", options.mWidth))
            {
                return false;
            }
            continue;
        }
        if (std::strcmp(argument, "--height") == 0)
        {
            const char* value = require_value("--height");
            if (!parse_positive_int(value, "--height", options.mHeight))
            {
                return false;
            }
            continue;
        }

        std::cerr << "Unknown option: " << argument << ".\n";
        return false;
    }

    return true;
}

LLRenderBackendType expected_backend_type(const std::string& backend)
{
    return backend == "opengl" ?
        LLRenderBackendType::OpenGL :
        LLRenderBackendType::Vulkan;
}

bool scene_uses_immediate_render(const std::string& scene)
{
    return scene == "primitives" || scene == "complete";
}

bool scene_uses_deferred_render(
    const std::string& scene,
    const std::string& render_path)
{
    return scene == "complete" && render_path == "deferred";
}

bool scene_uses_llrender(
    const std::string& scene,
    const std::string& render_path)
{
    return scene_uses_immediate_render(scene) ||
        scene_uses_deferred_render(scene, render_path);
}

bool create_scene_world_quad(
    LLRenderBackend& backend,
    SceneWorldQuad& quad)
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
    const std::array<std::array<U8, 4>, 6> colors =
    {{
        {{ 255, 255, 255, 255 }},
        {{ 255, 255, 255, 255 }},
        {{ 255, 255, 255, 255 }},
        {{ 255, 255, 255, 255 }},
        {{ 255, 255, 255, 255 }},
        {{ 255, 255, 255, 255 }},
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

    std::vector<U8> bytes;
    quad.mPositionOffset = bytes.size();
    for (const auto& value : positions)
    {
        append_scene_bytes(bytes, value);
    }
    quad.mNormalOffset = bytes.size();
    for (const auto& value : normals)
    {
        append_scene_bytes(bytes, value);
    }
    quad.mTexCoordOffset = bytes.size();
    for (const auto& value : texcoords)
    {
        append_scene_bytes(bytes, value);
    }
    quad.mTexCoord1Offset = bytes.size();
    for (const auto& value : texcoords)
    {
        append_scene_bytes(bytes, value);
    }
    quad.mTexCoord2Offset = bytes.size();
    for (const auto& value : texcoords)
    {
        append_scene_bytes(bytes, value);
    }
    quad.mColorOffset = bytes.size();
    for (const auto& value : colors)
    {
        append_scene_bytes(bytes, value);
    }
    quad.mTangentOffset = bytes.size();
    for (const auto& value : tangents)
    {
        append_scene_bytes(bytes, value);
    }
    quad.mWeightOffset = bytes.size();
    for (const F32 value : weights)
    {
        append_scene_bytes(bytes, value);
    }
    quad.mWeight4Offset = bytes.size();
    for (const auto& value : weight4s)
    {
        append_scene_bytes(bytes, value);
    }
    quad.mTextureIndexOffset = bytes.size();
    for (const auto& value : texture_indices)
    {
        append_scene_bytes(bytes, value);
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

void bind_scene_world_quad(
    LLRenderBackend& backend,
    const SceneWorldQuad& quad)
{
    backend.bindBuffer(LLRenderBufferTarget::Vertex, quad.mVertexBuffer);
    backend.enableVertexAttributeArray(0);
    backend.setVertexAttributePointer(
        0,
        3,
        LLRenderVertexAttributeType::Float32,
        false,
        16,
        reinterpret_cast<const void*>(static_cast<uintptr_t>(quad.mPositionOffset)));
    backend.enableVertexAttributeArray(1);
    backend.setVertexAttributePointer(
        1,
        3,
        LLRenderVertexAttributeType::Float32,
        false,
        16,
        reinterpret_cast<const void*>(static_cast<uintptr_t>(quad.mNormalOffset)));
    backend.enableVertexAttributeArray(2);
    backend.setVertexAttributePointer(
        2,
        2,
        LLRenderVertexAttributeType::Float32,
        false,
        8,
        reinterpret_cast<const void*>(static_cast<uintptr_t>(quad.mTexCoordOffset)));
    backend.enableVertexAttributeArray(3);
    backend.setVertexAttributePointer(
        3,
        2,
        LLRenderVertexAttributeType::Float32,
        false,
        8,
        reinterpret_cast<const void*>(static_cast<uintptr_t>(quad.mTexCoord1Offset)));
    backend.enableVertexAttributeArray(4);
    backend.setVertexAttributePointer(
        4,
        2,
        LLRenderVertexAttributeType::Float32,
        false,
        8,
        reinterpret_cast<const void*>(static_cast<uintptr_t>(quad.mTexCoord2Offset)));
    backend.enableVertexAttributeArray(6);
    backend.setVertexAttributePointer(
        6,
        4,
        LLRenderVertexAttributeType::UnsignedByte,
        true,
        4,
        reinterpret_cast<const void*>(static_cast<uintptr_t>(quad.mColorOffset)));
    backend.enableVertexAttributeArray(8);
    backend.setVertexAttributePointer(
        8,
        4,
        LLRenderVertexAttributeType::Float32,
        false,
        16,
        reinterpret_cast<const void*>(static_cast<uintptr_t>(quad.mTangentOffset)));
    backend.enableVertexAttributeArray(9);
    backend.setVertexAttributePointer(
        9,
        1,
        LLRenderVertexAttributeType::Float32,
        false,
        4,
        reinterpret_cast<const void*>(static_cast<uintptr_t>(quad.mWeightOffset)));
    backend.enableVertexAttributeArray(10);
    backend.setVertexAttributePointer(
        10,
        4,
        LLRenderVertexAttributeType::Float32,
        false,
        16,
        reinterpret_cast<const void*>(static_cast<uintptr_t>(quad.mWeight4Offset)));
    backend.enableVertexAttributeArray(13);
    backend.setIntegerVertexAttributePointer(
        13,
        1,
        LLRenderVertexAttributeType::UnsignedInt,
        16,
        reinterpret_cast<const void*>(static_cast<uintptr_t>(quad.mTextureIndexOffset)));
}

U8 scene_test_color_byte(F32 value)
{
    const F32 clamped = llclamp(value, 0.f, 1.f);
    return static_cast<U8>(std::round(clamped * 255.f));
}

SceneDirectVertex make_scene_direct_vertex(
    F32 x,
    F32 y,
    F32 u,
    F32 v,
    const LLColor4& color,
    S32 screen_width,
    S32 screen_height)
{
    SceneDirectVertex vertex;
    vertex.mPosition[0] =
        (x / static_cast<F32>(screen_width)) * 2.f - 1.f;
    vertex.mPosition[1] =
        (y / static_cast<F32>(screen_height)) * 2.f - 1.f;
    vertex.mTexCoord[0] = u;
    vertex.mTexCoord[1] = v;
    vertex.mColor[0] = scene_test_color_byte(color.mV[VRED]);
    vertex.mColor[1] = scene_test_color_byte(color.mV[VGREEN]);
    vertex.mColor[2] = scene_test_color_byte(color.mV[VBLUE]);
    vertex.mColor[3] = scene_test_color_byte(color.mV[VALPHA]);
    return vertex;
}

bool get_scene_shader_status_log(
    LLRenderBackend& backend,
    LLRenderShaderHandle shader,
    std::string& log)
{
    S32 status = 0;
    backend.getShaderInteger(
        shader,
        LLRenderShaderParameter::CompileStatus,
        &status);
    S32 log_length = 0;
    backend.getShaderInteger(
        shader,
        LLRenderShaderParameter::InfoLogLength,
        &log_length);
    if (log_length > 1)
    {
        std::vector<char> buffer(static_cast<size_t>(log_length) + 1U, '\0');
        S32 actual_length = 0;
        backend.getShaderInfoLog(
            shader,
            log_length,
            &actual_length,
            buffer.data());
        log.assign(buffer.data(), static_cast<size_t>(llmax(actual_length, 0)));
    }
    return status != 0;
}

bool get_scene_program_status_log(
    LLRenderBackend& backend,
    LLRenderProgramHandle program,
    std::string& log)
{
    S32 status = 0;
    backend.getProgramInteger(
        program,
        LLRenderProgramParameter::LinkStatus,
        &status);
    S32 log_length = 0;
    backend.getProgramInteger(
        program,
        LLRenderProgramParameter::InfoLogLength,
        &log_length);
    if (log_length > 1)
    {
        std::vector<char> buffer(static_cast<size_t>(log_length) + 1U, '\0');
        S32 actual_length = 0;
        backend.getProgramInfoLog(
            program,
            log_length,
            &actual_length,
            buffer.data());
        log.assign(buffer.data(), static_cast<size_t>(llmax(actual_length, 0)));
    }
    return status != 0;
}

void flip_scene_rgba_rows(
    std::vector<U8>& pixels,
    U32 width,
    U32 height)
{
    const size_t row_size = static_cast<size_t>(width) * 4U;
    if (row_size == 0 || height <= 1)
    {
        return;
    }

    std::vector<U8> row(row_size);
    for (U32 y = 0; y < height / 2U; ++y)
    {
        U8* bottom =
            pixels.data() + static_cast<size_t>(y) * row_size;
        U8* top =
            pixels.data() + static_cast<size_t>(height - 1U - y) * row_size;
        std::memcpy(row.data(), bottom, row_size);
        std::memcpy(bottom, top, row_size);
        std::memcpy(top, row.data(), row_size);
    }
}

bool ensure_scene_direct_renderer(
    LLRenderBackend& backend,
    SceneTestResources& resources)
{
    if (resources.mDirectProgram && resources.mDirectVertexBuffer)
    {
        return true;
    }

    if (backend.getType() != LLRenderBackendType::OpenGL)
    {
        std::cerr << "Direct scene renderer is currently only used for OpenGL.\n";
        return false;
    }

    static const char* vertex_shader_source =
        "#version 150\n"
        "in vec2 position;\n"
        "in vec2 texcoord0;\n"
        "in vec4 diffuse_color;\n"
        "out vec2 vary_texcoord0;\n"
        "out vec4 vertex_color;\n"
        "void main()\n"
        "{\n"
        "    vary_texcoord0 = texcoord0;\n"
        "    vertex_color = diffuse_color;\n"
        "    gl_Position = vec4(position, 0.0, 1.0);\n"
        "}\n";
    static const char* fragment_shader_source =
        "#version 150\n"
        "uniform sampler2D diffuseMap;\n"
        "in vec2 vary_texcoord0;\n"
        "in vec4 vertex_color;\n"
        "out vec4 frag_color;\n"
        "void main()\n"
        "{\n"
        "    frag_color = texture(diffuseMap, vary_texcoord0) * vertex_color;\n"
        "}\n";

    resources.mDirectVertexShader =
        backend.createShaderHandle(LLRenderShaderStage::Vertex);
    resources.mDirectFragmentShader =
        backend.createShaderHandle(LLRenderShaderStage::Fragment);
    resources.mDirectProgram = backend.createProgramHandle();
    resources.mDirectVertexBuffer = backend.createBufferHandle();
    if (!resources.mDirectVertexShader ||
        !resources.mDirectFragmentShader ||
        !resources.mDirectProgram ||
        !resources.mDirectVertexBuffer)
    {
        return false;
    }

    const char* vertex_shader_sources[] = { vertex_shader_source };
    backend.setShaderSource(
        resources.mDirectVertexShader,
        1,
        vertex_shader_sources);
    backend.compileShader(resources.mDirectVertexShader);
    std::string log;
    if (!get_scene_shader_status_log(backend, resources.mDirectVertexShader, log))
    {
        std::cerr << "Failed to compile scene OpenGL vertex shader: " << log << "\n";
        return false;
    }

    const char* fragment_shader_sources[] = { fragment_shader_source };
    backend.setShaderSource(
        resources.mDirectFragmentShader,
        1,
        fragment_shader_sources);
    backend.compileShader(resources.mDirectFragmentShader);
    log.clear();
    if (!get_scene_shader_status_log(backend, resources.mDirectFragmentShader, log))
    {
        std::cerr << "Failed to compile scene OpenGL fragment shader: " << log << "\n";
        return false;
    }

    backend.attachShader(resources.mDirectProgram, resources.mDirectVertexShader);
    backend.attachShader(resources.mDirectProgram, resources.mDirectFragmentShader);
    backend.bindAttributeLocation(resources.mDirectProgram, 0, "position");
    backend.bindAttributeLocation(resources.mDirectProgram, 2, "texcoord0");
    backend.bindAttributeLocation(resources.mDirectProgram, 6, "diffuse_color");
    backend.linkProgram(resources.mDirectProgram);
    log.clear();
    if (!get_scene_program_status_log(backend, resources.mDirectProgram, log))
    {
        std::cerr << "Failed to link scene OpenGL shader program: " << log << "\n";
        return false;
    }

    backend.useProgram(resources.mDirectProgram);
    const S32 diffuse_location =
        backend.getUniformLocation(resources.mDirectProgram, "diffuseMap");
    if (diffuse_location >= 0)
    {
        backend.setUniformInteger(diffuse_location, 0);
    }
    backend.useProgram(LLRenderProgramHandle());
    return true;
}

void draw_scene_direct_triangles(
    LLRenderBackend& backend,
    SceneTestResources& resources,
    LLRenderTextureHandle texture,
    const std::vector<SceneDirectVertex>& vertices)
{
    if (vertices.empty())
    {
        return;
    }

    backend.useProgram(resources.mDirectProgram);
    backend.setActiveTextureUnit(0);
    backend.bindTexture(LLRenderTextureTarget::Texture2D, texture);
    backend.bindBuffer(LLRenderBufferTarget::Vertex, resources.mDirectVertexBuffer);
    backend.allocateBufferStorage(
        LLRenderBufferTarget::Vertex,
        vertices.size() * sizeof(SceneDirectVertex),
        vertices.data(),
        LLRenderBufferUsage::StreamDraw);
    backend.enableVertexAttributeArray(0);
    backend.setVertexAttributePointer(
        0,
        2,
        LLRenderVertexAttributeType::Float32,
        false,
        sizeof(SceneDirectVertex),
        reinterpret_cast<const void*>(offsetof(SceneDirectVertex, mPosition)));
    backend.enableVertexAttributeArray(2);
    backend.setVertexAttributePointer(
        2,
        2,
        LLRenderVertexAttributeType::Float32,
        false,
        sizeof(SceneDirectVertex),
        reinterpret_cast<const void*>(offsetof(SceneDirectVertex, mTexCoord)));
    backend.enableVertexAttributeArray(6);
    backend.setVertexAttributePointer(
        6,
        4,
        LLRenderVertexAttributeType::UnsignedByte,
        true,
        sizeof(SceneDirectVertex),
        reinterpret_cast<const void*>(offsetof(SceneDirectVertex, mColor)));
    backend.drawArrays(
        LLRenderPrimitiveType::Triangles,
        0,
        static_cast<S32>(vertices.size()));
}

void draw_scene_direct_color_rect(
    LLRenderBackend& backend,
    SceneTestResources& resources,
    const LLColor4& color,
    S32 x,
    S32 y,
    S32 width,
    S32 height,
    S32 screen_width,
    S32 screen_height)
{
    if (width <= 0 || height <= 0)
    {
        return;
    }

    const F32 left = static_cast<F32>(x);
    const F32 right = static_cast<F32>(x + width);
    const F32 bottom = static_cast<F32>(y);
    const F32 top = static_cast<F32>(y + height);
    std::vector<SceneDirectVertex> vertices =
    {
        make_scene_direct_vertex(left, bottom, 0.f, 0.f, color, screen_width, screen_height),
        make_scene_direct_vertex(right, bottom, 1.f, 0.f, color, screen_width, screen_height),
        make_scene_direct_vertex(left, top, 0.f, 1.f, color, screen_width, screen_height),
        make_scene_direct_vertex(left, top, 0.f, 1.f, color, screen_width, screen_height),
        make_scene_direct_vertex(right, bottom, 1.f, 0.f, color, screen_width, screen_height),
        make_scene_direct_vertex(right, top, 1.f, 1.f, color, screen_width, screen_height),
    };
    draw_scene_direct_triangles(backend, resources, resources.mWhiteTexture, vertices);
}

void draw_scene_direct_texture_quad(
    LLRenderBackend& backend,
    SceneTestResources& resources,
    LLRenderTextureHandle texture,
    F32 x0,
    F32 y0,
    F32 x1,
    F32 y1,
    F32 x2,
    F32 y2,
    F32 x3,
    F32 y3,
    F32 repeat_x,
    F32 repeat_y,
    const LLColor4& color,
    S32 screen_width,
    S32 screen_height)
{
    std::vector<SceneDirectVertex> vertices =
    {
        make_scene_direct_vertex(x0, y0, 0.f, 0.f, color, screen_width, screen_height),
        make_scene_direct_vertex(x1, y1, repeat_x, 0.f, color, screen_width, screen_height),
        make_scene_direct_vertex(x2, y2, 0.f, repeat_y, color, screen_width, screen_height),
        make_scene_direct_vertex(x2, y2, 0.f, repeat_y, color, screen_width, screen_height),
        make_scene_direct_vertex(x1, y1, repeat_x, 0.f, color, screen_width, screen_height),
        make_scene_direct_vertex(x3, y3, repeat_x, repeat_y, color, screen_width, screen_height),
    };
    draw_scene_direct_triangles(backend, resources, texture, vertices);
}

void draw_scene_direct_texture_rect(
    LLRenderBackend& backend,
    SceneTestResources& resources,
    LLRenderTextureHandle texture,
    S32 x,
    S32 y,
    S32 width,
    S32 height,
    F32 repeat_x,
    F32 repeat_y,
    const LLColor4& color,
    S32 screen_width,
    S32 screen_height)
{
    if (width <= 0 || height <= 0)
    {
        return;
    }

    draw_scene_direct_texture_quad(
        backend,
        resources,
        texture,
        static_cast<F32>(x),
        static_cast<F32>(y),
        static_cast<F32>(x + width),
        static_cast<F32>(y),
        static_cast<F32>(x),
        static_cast<F32>(y + height),
        static_cast<F32>(x + width),
        static_cast<F32>(y + height),
        repeat_x,
        repeat_y,
        color,
        screen_width,
        screen_height);
}

void draw_scene_direct_color_triangle(
    LLRenderBackend& backend,
    SceneTestResources& resources,
    const LLColor4& color,
    F32 x0,
    F32 y0,
    F32 x1,
    F32 y1,
    F32 x2,
    F32 y2,
    S32 screen_width,
    S32 screen_height)
{
    std::vector<SceneDirectVertex> vertices =
    {
        make_scene_direct_vertex(x0, y0, 0.f, 0.f, color, screen_width, screen_height),
        make_scene_direct_vertex(x1, y1, 1.f, 0.f, color, screen_width, screen_height),
        make_scene_direct_vertex(x2, y2, 0.5f, 1.f, color, screen_width, screen_height),
    };
    draw_scene_direct_triangles(backend, resources, resources.mWhiteTexture, vertices);
}

void draw_scene_direct_disc(
    LLRenderBackend& backend,
    SceneTestResources& resources,
    const LLColor4& color,
    F32 center_x,
    F32 center_y,
    F32 radius,
    S32 segments,
    S32 screen_width,
    S32 screen_height)
{
    constexpr F32 two_pi = 6.28318530718f;
    const S32 safe_segments = llmax(segments, 8);
    std::vector<SceneDirectVertex> vertices;
    vertices.reserve(static_cast<size_t>(safe_segments) * 3U);
    for (S32 i = 0; i < safe_segments; ++i)
    {
        const F32 angle0 = two_pi * static_cast<F32>(i) / static_cast<F32>(safe_segments);
        const F32 angle1 = two_pi * static_cast<F32>(i + 1) / static_cast<F32>(safe_segments);
        vertices.push_back(make_scene_direct_vertex(center_x, center_y, 0.5f, 0.5f, color, screen_width, screen_height));
        vertices.push_back(make_scene_direct_vertex(center_x + std::cos(angle0) * radius, center_y + std::sin(angle0) * radius, 0.f, 0.f, color, screen_width, screen_height));
        vertices.push_back(make_scene_direct_vertex(center_x + std::cos(angle1) * radius, center_y + std::sin(angle1) * radius, 1.f, 1.f, color, screen_width, screen_height));
    }
    draw_scene_direct_triangles(backend, resources, resources.mWhiteTexture, vertices);
}

bool render_scene_test_frame(
    LLRenderBackend& backend,
    U32 width,
    U32 height)
{
    backend.setViewport(0, 0, static_cast<S32>(width), static_cast<S32>(height));
    backend.setScissor(0, 0, static_cast<S32>(width), static_cast<S32>(height));
    backend.setClearColor(0.07f, 0.16f, 0.31f, 1.f);
    backend.clear(LL_RENDER_CLEAR_COLOR | LL_RENDER_CLEAR_DEPTH);
    return true;
}

std::vector<U8> make_scene_checker_rgba_pixels()
{
    constexpr U32 width = 16;
    constexpr U32 height = 16;
    std::vector<U8> pixels(width * height * 4U);
    for (U32 y = 0; y < height; ++y)
    {
        for (U32 x = 0; x < width; ++x)
        {
            const bool bright = ((x / 4U) + (y / 4U)) % 2U == 0U;
            const size_t offset = (static_cast<size_t>(y) * width + x) * 4U;
            pixels[offset + 0] = bright ? 235 : 32;
            pixels[offset + 1] = bright ? 245 : 84;
            pixels[offset + 2] = bright ? 255 : 170;
            pixels[offset + 3] = 255;
        }
    }
    return pixels;
}

std::vector<U8> make_scene_solid_rgba_pixels(
    U32 width,
    U32 height,
    U8 red,
    U8 green,
    U8 blue,
    U8 alpha)
{
    std::vector<U8> pixels(width * height * 4U);
    for (size_t i = 0; i < pixels.size(); i += 4)
    {
        pixels[i + 0] = red;
        pixels[i + 1] = green;
        pixels[i + 2] = blue;
        pixels[i + 3] = alpha;
    }
    return pixels;
}

std::vector<U8> make_scene_road_rgba_pixels()
{
    constexpr U32 width = 64;
    constexpr U32 height = 16;
    std::vector<U8> pixels(width * height * 4U);
    for (U32 y = 0; y < height; ++y)
    {
        for (U32 x = 0; x < width; ++x)
        {
            const size_t offset = (static_cast<size_t>(y) * width + x) * 4U;
            const bool edge = y < 2U || y >= height - 2U;
            const bool center_line = (y == 7U || y == 8U) && ((x / 8U) % 2U == 0U);
            const bool seam = x % 16U == 0U;
            pixels[offset + 0] = edge ? 235 : center_line ? 255 : seam ? 95 : 54;
            pixels[offset + 1] = edge ? 228 : center_line ? 242 : seam ? 95 : 58;
            pixels[offset + 2] = edge ? 190 : center_line ? 118 : seam ? 100 : 64;
            pixels[offset + 3] = 255;
        }
    }
    return pixels;
}

std::vector<U8> make_scene_alpha_rgba_pixels()
{
    constexpr U32 width = 32;
    constexpr U32 height = 32;
    std::vector<U8> pixels(width * height * 4U);
    for (U32 y = 0; y < height; ++y)
    {
        for (U32 x = 0; x < width; ++x)
        {
            const size_t offset = (static_cast<size_t>(y) * width + x) * 4U;
            const S32 dx = static_cast<S32>(x) - 16;
            const S32 dy = static_cast<S32>(y) - 16;
            const S32 distance_sq = dx * dx + dy * dy;
            const bool ring = distance_sq > 36 && distance_sq < 180;
            const bool diagonal = ((x + y) / 4U) % 2U == 0U;
            pixels[offset + 0] = diagonal ? 210 : 52;
            pixels[offset + 1] = diagonal ? 255 : 190;
            pixels[offset + 2] = diagonal ? 118 : 255;
            pixels[offset + 3] = ring ? 190 : 0;
        }
    }
    return pixels;
}

bool create_scene_texture(
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
        LLRenderTextureFilter::Nearest,
        LLRenderTextureFilter::Nearest);
    backend.setTextureAddressMode(
        LLRenderTextureTarget::Texture2D,
        LLRenderTextureAddressMode::Repeat);
    return true;
}

bool create_empty_scene_texture(
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

void release_scene_test_resources(
    LLRenderBackend& backend,
    SceneTestResources& resources)
{
    if (resources.mDirectVertexBuffer)
    {
        backend.deleteBufferHandle(resources.mDirectVertexBuffer);
        resources.mDirectVertexBuffer = {};
    }
    if (resources.mDirectProgram)
    {
        backend.deleteProgram(resources.mDirectProgram);
        resources.mDirectProgram = {};
    }
    if (resources.mDirectVertexShader)
    {
        backend.deleteShader(resources.mDirectVertexShader);
        resources.mDirectVertexShader = {};
    }
    if (resources.mDirectFragmentShader)
    {
        backend.deleteShader(resources.mDirectFragmentShader);
        resources.mDirectFragmentShader = {};
    }
    if (resources.mGBufferFramebuffer)
    {
        backend.deleteFramebufferHandle(resources.mGBufferFramebuffer);
        resources.mGBufferFramebuffer = {};
    }
    if (resources.mDeferredFramebuffer)
    {
        backend.deleteFramebufferHandle(resources.mDeferredFramebuffer);
        resources.mDeferredFramebuffer = {};
    }
    if (resources.mWorldQuad.mVertexBuffer)
    {
        backend.deleteBufferHandle(resources.mWorldQuad.mVertexBuffer);
        resources.mWorldQuad = {};
    }
    if (resources.mWhiteTexture)
    {
        backend.deleteTextureHandle(resources.mWhiteTexture);
        resources.mWhiteTexture = {};
    }
    if (resources.mCheckerTexture)
    {
        backend.deleteTextureHandle(resources.mCheckerTexture);
        resources.mCheckerTexture = {};
    }
    if (resources.mRoadTexture)
    {
        backend.deleteTextureHandle(resources.mRoadTexture);
        resources.mRoadTexture = {};
    }
    if (resources.mAlphaTexture)
    {
        backend.deleteTextureHandle(resources.mAlphaTexture);
        resources.mAlphaTexture = {};
    }
    LLRenderTextureHandle* deferred_textures[] =
    {
        &resources.mGBufferColor,
        &resources.mGBufferSpecular,
        &resources.mGBufferNormal,
        &resources.mGBufferEmissive,
        &resources.mGBufferDepth,
        &resources.mDeferredColor,
        &resources.mExposureMap,
    };
    for (LLRenderTextureHandle* texture : deferred_textures)
    {
        if (*texture)
        {
            backend.deleteTextureHandle(*texture);
            *texture = {};
        }
    }
    resources.mDeferredWidth = 0;
    resources.mDeferredHeight = 0;
}

bool ensure_scene_test_resources(
    LLRenderBackend& backend,
    SceneTestResources& resources)
{
    if (resources.mWorldQuad.mVertexBuffer &&
        resources.mWhiteTexture &&
        resources.mCheckerTexture &&
        resources.mRoadTexture &&
        resources.mAlphaTexture)
    {
        return true;
    }

    if (!create_scene_world_quad(backend, resources.mWorldQuad) ||
        !create_scene_texture(
            backend,
            resources.mWhiteTexture,
            1,
            1,
            make_scene_solid_rgba_pixels(1, 1, 255, 255, 255, 255)) ||
        !create_scene_texture(
            backend,
            resources.mCheckerTexture,
            16,
            16,
            make_scene_checker_rgba_pixels()))
    {
        release_scene_test_resources(backend, resources);
        return false;
    }
    if (!create_scene_texture(
            backend,
            resources.mRoadTexture,
            64,
            16,
            make_scene_road_rgba_pixels()))
    {
        release_scene_test_resources(backend, resources);
        return false;
    }
    if (!create_scene_texture(
            backend,
            resources.mAlphaTexture,
            32,
            32,
            make_scene_alpha_rgba_pixels()))
    {
        release_scene_test_resources(backend, resources);
        return false;
    }
    return true;
}

bool ensure_scene_deferred_graph(
    LLRenderBackend& backend,
    SceneTestResources& resources,
    U32 width,
    U32 height)
{
    if (resources.mGBufferFramebuffer &&
        resources.mDeferredFramebuffer &&
        resources.mGBufferColor &&
        resources.mGBufferSpecular &&
        resources.mGBufferNormal &&
        resources.mGBufferEmissive &&
        resources.mGBufferDepth &&
        resources.mDeferredColor &&
        resources.mExposureMap &&
        resources.mDeferredWidth == width &&
        resources.mDeferredHeight == height)
    {
        return true;
    }

    if (resources.mGBufferFramebuffer)
    {
        backend.deleteFramebufferHandle(resources.mGBufferFramebuffer);
        resources.mGBufferFramebuffer = {};
    }
    if (resources.mDeferredFramebuffer)
    {
        backend.deleteFramebufferHandle(resources.mDeferredFramebuffer);
        resources.mDeferredFramebuffer = {};
    }
    LLRenderTextureHandle* deferred_textures[] =
    {
        &resources.mGBufferColor,
        &resources.mGBufferSpecular,
        &resources.mGBufferNormal,
        &resources.mGBufferEmissive,
        &resources.mGBufferDepth,
        &resources.mDeferredColor,
        &resources.mExposureMap,
    };
    for (LLRenderTextureHandle* texture : deferred_textures)
    {
        if (*texture)
        {
            backend.deleteTextureHandle(*texture);
            *texture = {};
        }
    }

    resources.mDeferredWidth = width;
    resources.mDeferredHeight = height;
    if (!create_empty_scene_texture(
            backend,
            resources.mGBufferColor,
            width,
            height,
            LLRenderTextureFormat::RGBA,
            LLRenderPixelFormat::RGBA,
            LLRenderPixelType::UnsignedByte) ||
        !create_empty_scene_texture(
            backend,
            resources.mGBufferSpecular,
            width,
            height,
            LLRenderTextureFormat::RGBA,
            LLRenderPixelFormat::RGBA,
            LLRenderPixelType::UnsignedByte) ||
        !create_empty_scene_texture(
            backend,
            resources.mGBufferNormal,
            width,
            height,
            LLRenderTextureFormat::RGBA16,
            LLRenderPixelFormat::RGBA,
            LLRenderPixelType::UnsignedShort) ||
        !create_empty_scene_texture(
            backend,
            resources.mGBufferEmissive,
            width,
            height,
            LLRenderTextureFormat::RGB16F,
            LLRenderPixelFormat::RGBA,
            LLRenderPixelType::Float32) ||
        !create_empty_scene_texture(
            backend,
            resources.mGBufferDepth,
            width,
            height,
            LLRenderTextureFormat::DepthComponent24,
            LLRenderPixelFormat::DepthComponent,
            LLRenderPixelType::Float32) ||
        !create_empty_scene_texture(
            backend,
            resources.mDeferredColor,
            width,
            height,
            LLRenderTextureFormat::RGBA16F,
            LLRenderPixelFormat::RGBA,
            LLRenderPixelType::Float32) ||
        !create_scene_texture(
            backend,
            resources.mExposureMap,
            1,
            1,
            make_scene_solid_rgba_pixels(1, 1, 255, 255, 255, 255)))
    {
        return false;
    }

    resources.mGBufferFramebuffer = backend.createFramebufferHandle();
    resources.mDeferredFramebuffer = backend.createFramebufferHandle();
    if (!resources.mGBufferFramebuffer || !resources.mDeferredFramebuffer)
    {
        return false;
    }

    backend.bindReadWriteFramebuffer(resources.mGBufferFramebuffer);
    backend.attachFramebufferTexture2D(
        LLRenderFramebufferAttachment::Color0,
        LLRenderTextureTarget::Texture2D,
        resources.mGBufferColor,
        0);
    backend.attachFramebufferTexture2D(
        LLRenderFramebufferAttachment::Color1,
        LLRenderTextureTarget::Texture2D,
        resources.mGBufferSpecular,
        0);
    backend.attachFramebufferTexture2D(
        LLRenderFramebufferAttachment::Color2,
        LLRenderTextureTarget::Texture2D,
        resources.mGBufferNormal,
        0);
    backend.attachFramebufferTexture2D(
        LLRenderFramebufferAttachment::Color3,
        LLRenderTextureTarget::Texture2D,
        resources.mGBufferEmissive,
        0);
    backend.attachFramebufferTexture2D(
        LLRenderFramebufferAttachment::Depth,
        LLRenderTextureTarget::Texture2D,
        resources.mGBufferDepth,
        0);
    backend.setFramebufferBufferRouting(4);
    if (!backend.isDrawFramebufferComplete())
    {
        return false;
    }

    backend.bindReadWriteFramebuffer(resources.mDeferredFramebuffer);
    backend.attachFramebufferTexture2D(
        LLRenderFramebufferAttachment::Color0,
        LLRenderTextureTarget::Texture2D,
        resources.mDeferredColor,
        0);
    backend.setFramebufferBufferRouting(1);
    if (!backend.isDrawFramebufferComplete())
    {
        return false;
    }

    backend.bindReadWriteFramebuffer(LLRenderFramebufferHandle());
    backend.restoreDefaultFramebufferBufferRouting();
    return true;
}

void draw_scene_test_color_rect(
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

    gl_rect_2d(x, y + height, x + width, y, color, true);
}

void draw_scene_test_texture_quad(
    LLRenderTextureHandle texture,
    F32 x0,
    F32 y0,
    F32 x1,
    F32 y1,
    F32 x2,
    F32 y2,
    F32 x3,
    F32 y3,
    F32 repeat_x,
    F32 repeat_y,
    const LLColor4& color = LLColor4::white);

void draw_scene_test_texture_rect(
    LLRenderBackend& backend,
    LLRenderTextureHandle texture,
    S32 x,
    S32 y,
    S32 width,
    S32 height,
    F32 repeat_x,
    F32 repeat_y,
    const LLColor4& color = LLColor4::white)
{
    if (width <= 0 || height <= 0)
    {
        return;
    }

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
    gGL.color4fv(color.mV);
    gGL.texCoord2f(0.f, 0.f);
    gGL.vertex2f(left, bottom);
    gGL.texCoord2f(repeat_x, 0.f);
    gGL.vertex2f(right, bottom);
    gGL.texCoord2f(0.f, repeat_y);
    gGL.vertex2f(left, top);
    gGL.texCoord2f(0.f, repeat_y);
    gGL.vertex2f(left, top);
    gGL.texCoord2f(repeat_x, 0.f);
    gGL.vertex2f(right, bottom);
    gGL.texCoord2f(repeat_x, repeat_y);
    gGL.vertex2f(right, top);
    gGL.end();
}

LLRenderWorldMaterialParameters make_scene_deferred_material(
    const LLColor4& color,
    U32 flags = 0)
{
    LLRenderWorldMaterialParameters parameters;
    parameters.mBaseColorRed = color.mV[VRED];
    parameters.mBaseColorGreen = color.mV[VGREEN];
    parameters.mBaseColorBlue = color.mV[VBLUE];
    parameters.mBaseColorAlpha = color.mV[VALPHA];
    parameters.mSpecularColorRed = 0.25f;
    parameters.mSpecularColorGreen = 0.28f;
    parameters.mSpecularColorBlue = 0.34f;
    parameters.mRoughnessFactor = 0.72f;
    parameters.mMetallicFactor = 0.f;
    parameters.mMaterialFlags = static_cast<F32>(flags);
    parameters.mSceneAmbientRed = 0.50f;
    parameters.mSceneAmbientGreen = 0.53f;
    parameters.mSceneAmbientBlue = 0.58f;
    parameters.mSceneDirectScale = 0.85f;
    parameters.mSceneDirectRed = 0.95f;
    parameters.mSceneDirectGreen = 0.92f;
    parameters.mSceneDirectBlue = 0.86f;
    parameters.mSceneLightingValid = 1.f;
    parameters.mSceneLightDirectionX = 0.32f;
    parameters.mSceneLightDirectionY = 0.42f;
    parameters.mSceneLightDirectionZ = 0.85f;
    parameters.mSceneLightDirectionValid = 1.f;
    return parameters;
}

LLRenderWorldMaterialParameters make_scene_deferred_composite_parameters()
{
    LLRenderWorldMaterialParameters parameters =
        make_scene_deferred_material(LLColor4(0.40f, 0.46f, 0.55f, 1.f));
    parameters.mEmissiveColorRed = 0.86f;
    parameters.mEmissiveColorGreen = 0.92f;
    parameters.mEmissiveColorBlue = 1.f;
    parameters.mEnvIntensity = 0.35f;
    parameters.mMaterialFlags =
        static_cast<F32>(LLRenderWorldMaterialParameters::Fullbright);
    return parameters;
}

LLRenderWorldMaterialParameters make_scene_final_composite_parameters()
{
    LLVulkanFinalCompositeSettings settings;
    settings.mNoPost = true;
    settings.mDeferredAttachmentCount = 4;
    return make_vulkan_final_composite_material_parameters(settings);
}

void bind_scene_gbuffer_textures(
    LLRenderBackend& backend,
    const SceneTestResources& resources)
{
    const LLRenderTextureHandle bindings[] =
    {
        resources.mGBufferColor,
        resources.mGBufferSpecular,
        resources.mGBufferNormal,
        resources.mGBufferEmissive,
        resources.mGBufferDepth,
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

void bind_scene_final_textures(
    LLRenderBackend& backend,
    const SceneTestResources& resources)
{
    const LLRenderTextureHandle bindings[] =
    {
        resources.mDeferredColor,
        resources.mGBufferColor,
        resources.mGBufferSpecular,
        resources.mGBufferNormal,
        resources.mGBufferEmissive,
        resources.mGBufferDepth,
        resources.mExposureMap,
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

void draw_scene_test_texture_quad(
    LLRenderTextureHandle texture,
    F32 x0,
    F32 y0,
    F32 x1,
    F32 y1,
    F32 x2,
    F32 y2,
    F32 x3,
    F32 y3,
    F32 repeat_x,
    F32 repeat_y,
    const LLColor4& color)
{
    gGL.getTexUnit(0)->bindManual(
        LLTexUnit::TT_TEXTURE,
        texture,
        false,
        true);

    gGL.begin(LLRender::TRIANGLES);
    gGL.color4fv(color.mV);
    gGL.texCoord2f(0.f, 0.f);
    gGL.vertex2f(x0, y0);
    gGL.texCoord2f(repeat_x, 0.f);
    gGL.vertex2f(x1, y1);
    gGL.texCoord2f(0.f, repeat_y);
    gGL.vertex2f(x2, y2);
    gGL.texCoord2f(0.f, repeat_y);
    gGL.vertex2f(x2, y2);
    gGL.texCoord2f(repeat_x, 0.f);
    gGL.vertex2f(x1, y1);
    gGL.texCoord2f(repeat_x, repeat_y);
    gGL.vertex2f(x3, y3);
    gGL.end();
}

void draw_scene_test_color_triangle(
    const LLColor4& color,
    F32 x0,
    F32 y0,
    F32 x1,
    F32 y1,
    F32 x2,
    F32 y2)
{
    gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);
    gGL.begin(LLRender::TRIANGLES);
    gGL.color4fv(color.mV);
    gGL.vertex2f(x0, y0);
    gGL.vertex2f(x1, y1);
    gGL.vertex2f(x2, y2);
    gGL.end();
}

void draw_scene_test_disc(
    const LLColor4& color,
    F32 center_x,
    F32 center_y,
    F32 radius,
    S32 segments)
{
    constexpr F32 two_pi = 6.28318530718f;
    const S32 safe_segments = llmax(segments, 8);
    gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);
    gGL.begin(LLRender::TRIANGLES);
    gGL.color4fv(color.mV);
    for (S32 i = 0; i < safe_segments; ++i)
    {
        const F32 angle0 = two_pi * static_cast<F32>(i) / static_cast<F32>(safe_segments);
        const F32 angle1 = two_pi * static_cast<F32>(i + 1) / static_cast<F32>(safe_segments);
        gGL.vertex2f(center_x, center_y);
        gGL.vertex2f(center_x + std::cos(angle0) * radius, center_y + std::sin(angle0) * radius);
        gGL.vertex2f(center_x + std::cos(angle1) * radius, center_y + std::sin(angle1) * radius);
    }
    gGL.end();
}

void prepare_scene_test_immediate_state(
    LLRenderBackend& backend,
    S32 screen_width,
    S32 screen_height)
{
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
    backend.setClearColor(0.06f, 0.12f, 0.24f, 1.f);
    backend.clear(LL_RENDER_CLEAR_COLOR | LL_RENDER_CLEAR_DEPTH);

    gl_state_for_2d(screen_width, screen_height);
    gUIProgram.mAttributeMask =
        LLVertexBuffer::MAP_VERTEX |
        LLVertexBuffer::MAP_TEXCOORD0 |
        LLVertexBuffer::MAP_COLOR;
    gUIProgram.bind();
}

void finish_scene_test_immediate_state(
    LLRenderBackend& backend,
    S32 screen_width,
    S32 screen_height)
{
    gGL.flush();
    gUIProgram.unbind();
    backend.setCapability(LLRenderCapability::Blend, false);
    backend.setScissor(0, 0, screen_width, screen_height);
    backend.setActiveTextureUnit(0);
}

void prepare_scene_direct_state(
    LLRenderBackend& backend,
    S32 screen_width,
    S32 screen_height)
{
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
    backend.setClearColor(0.06f, 0.12f, 0.24f, 1.f);
    backend.clear(LL_RENDER_CLEAR_COLOR | LL_RENDER_CLEAR_DEPTH);
}

void finish_scene_direct_state(
    LLRenderBackend& backend,
    S32 screen_width,
    S32 screen_height)
{
    backend.useProgram(LLRenderProgramHandle());
    backend.disableVertexAttributeArray(0);
    backend.disableVertexAttributeArray(2);
    backend.disableVertexAttributeArray(6);
    backend.setCapability(LLRenderCapability::Blend, false);
    backend.setScissor(0, 0, screen_width, screen_height);
    backend.setActiveTextureUnit(0);
}

bool render_scene_test_primitives_direct_frame(
    LLRenderBackend& backend,
    SceneTestResources& resources,
    U32 width,
    U32 height)
{
    if (!ensure_scene_test_resources(backend, resources) ||
        !ensure_scene_direct_renderer(backend, resources))
    {
        std::cerr << "Failed to create direct primitive scene resources.\n";
        return false;
    }

    const S32 screen_width = static_cast<S32>(width);
    const S32 screen_height = static_cast<S32>(height);
    const S32 margin = llmax(16, screen_width / 32);
    const S32 ground_top = llmax(screen_height / 3, 1);
    const S32 grid_step = llmax(18, screen_width / 28);

    prepare_scene_direct_state(backend, screen_width, screen_height);
    draw_scene_direct_color_rect(
        backend,
        resources,
        LLColor4(0.12f, 0.42f, 0.20f, 1.f),
        0,
        0,
        screen_width,
        ground_top,
        screen_width,
        screen_height);

    for (S32 x = 0; x < screen_width; x += grid_step)
    {
        draw_scene_direct_color_rect(
            backend,
            resources,
            LLColor4(0.20f, 0.58f, 0.28f, 0.70f),
            x,
            0,
            llmax(2, grid_step / 12),
            ground_top,
            screen_width,
            screen_height);
    }
    for (S32 y = 0; y < ground_top; y += llmax(12, grid_step / 2))
    {
        draw_scene_direct_color_rect(
            backend,
            resources,
            LLColor4(0.05f, 0.22f, 0.10f, 0.55f),
            0,
            y,
            screen_width,
            2,
            screen_width,
            screen_height);
    }

    draw_scene_direct_color_rect(
        backend,
        resources,
        LLColor4(0.96f, 0.78f, 0.16f, 1.f),
        margin * 2,
        ground_top - margin / 2,
        screen_width / 3,
        screen_height / 3,
        screen_width,
        screen_height);
    draw_scene_direct_color_rect(
        backend,
        resources,
        LLColor4(0.95f, 0.12f, 0.80f, 0.48f),
        screen_width / 3,
        ground_top + margin / 2,
        screen_width / 3,
        screen_height / 3,
        screen_width,
        screen_height);
    draw_scene_direct_texture_rect(
        backend,
        resources,
        resources.mCheckerTexture,
        screen_width / 2,
        margin,
        screen_width / 3,
        ground_top - margin * 2,
        6.f,
        3.f,
        LLColor4::white,
        screen_width,
        screen_height);
    draw_scene_direct_color_rect(
        backend,
        resources,
        LLColor4(0.10f, 0.72f, 0.96f, 0.82f),
        screen_width - margin * 5,
        screen_height - margin * 4,
        margin * 3,
        margin * 2,
        screen_width,
        screen_height);

    finish_scene_direct_state(backend, screen_width, screen_height);
    return true;
}

bool render_scene_test_primitives_frame(
    LLRenderBackend& backend,
    SceneTestResources& resources,
    U32 width,
    U32 height)
{
    if (!ensure_scene_test_resources(backend, resources))
    {
        std::cerr << "Failed to create scene test texture resources.\n";
        return false;
    }

    const S32 screen_width = static_cast<S32>(width);
    const S32 screen_height = static_cast<S32>(height);
    const S32 margin = llmax(16, screen_width / 32);
    const S32 ground_top = llmax(screen_height / 3, 1);
    const S32 grid_step = llmax(18, screen_width / 28);

    prepare_scene_test_immediate_state(backend, screen_width, screen_height);

    draw_scene_test_color_rect(
        backend,
        LLColor4(0.12f, 0.42f, 0.20f, 1.f),
        0,
        0,
        screen_width,
        ground_top);

    for (S32 x = 0; x < screen_width; x += grid_step)
    {
        draw_scene_test_color_rect(
            backend,
            LLColor4(0.20f, 0.58f, 0.28f, 0.70f),
            x,
            0,
            llmax(2, grid_step / 12),
            ground_top);
    }
    for (S32 y = 0; y < ground_top; y += llmax(12, grid_step / 2))
    {
        draw_scene_test_color_rect(
            backend,
            LLColor4(0.05f, 0.22f, 0.10f, 0.55f),
            0,
            y,
            screen_width,
            2);
    }

    draw_scene_test_color_rect(
        backend,
        LLColor4(0.96f, 0.78f, 0.16f, 1.f),
        margin * 2,
        ground_top - margin / 2,
        screen_width / 3,
        screen_height / 3);
    draw_scene_test_color_rect(
        backend,
        LLColor4(0.95f, 0.12f, 0.80f, 0.48f),
        screen_width / 3,
        ground_top + margin / 2,
        screen_width / 3,
        screen_height / 3);
    draw_scene_test_texture_rect(
        backend,
        resources.mCheckerTexture,
        screen_width / 2,
        margin,
        screen_width / 3,
        ground_top - margin * 2,
        6.f,
        3.f);
    draw_scene_test_color_rect(
        backend,
        LLColor4(0.10f, 0.72f, 0.96f, 0.82f),
        screen_width - margin * 5,
        screen_height - margin * 4,
        margin * 3,
        margin * 2);

    finish_scene_test_immediate_state(backend, screen_width, screen_height);
    return true;
}

bool render_scene_test_complete_frame(
    LLRenderBackend& backend,
    SceneTestResources& resources,
    U32 width,
    U32 height)
{
    if (!ensure_scene_test_resources(backend, resources))
    {
        std::cerr << "Failed to create complete scene test texture resources.\n";
        return false;
    }

    const S32 screen_width = static_cast<S32>(width);
    const S32 screen_height = static_cast<S32>(height);
    const S32 margin = llmax(18, screen_width / 42);
    const S32 horizon = (screen_height * 53) / 100;
    const S32 water_top = (screen_height * 44) / 100;
    const S32 ground_top = (screen_height * 36) / 100;

    prepare_scene_test_immediate_state(backend, screen_width, screen_height);

    draw_scene_test_color_rect(
        backend,
        LLColor4(0.10f, 0.22f, 0.43f, 1.f),
        0,
        horizon,
        screen_width,
        screen_height - horizon);
    draw_scene_test_color_rect(
        backend,
        LLColor4(0.34f, 0.58f, 0.78f, 0.92f),
        0,
        water_top,
        screen_width,
        horizon - water_top);
    draw_scene_test_disc(
        LLColor4(1.f, 0.85f, 0.28f, 0.88f),
        static_cast<F32>((screen_width * 78) / 100),
        static_cast<F32>((screen_height * 82) / 100),
        static_cast<F32>(llmax(18, screen_width / 28)),
        32);

    draw_scene_test_color_rect(
        backend,
        LLColor4(0.09f, 0.33f, 0.16f, 1.f),
        0,
        0,
        screen_width,
        ground_top);
    draw_scene_test_color_rect(
        backend,
        LLColor4(0.06f, 0.24f, 0.14f, 0.80f),
        0,
        ground_top - margin,
        screen_width,
        margin * 2);
    draw_scene_test_color_rect(
        backend,
        LLColor4(0.10f, 0.38f, 0.62f, 0.62f),
        0,
        ground_top,
        screen_width,
        water_top - ground_top);
    draw_scene_test_color_rect(
        backend,
        LLColor4(0.80f, 0.95f, 1.f, 0.28f),
        0,
        water_top - margin,
        screen_width,
        llmax(2, margin / 5));

    draw_scene_test_texture_quad(
        resources.mRoadTexture,
        static_cast<F32>((screen_width * 18) / 100),
        0.f,
        static_cast<F32>((screen_width * 32) / 100),
        0.f,
        static_cast<F32>((screen_width * 45) / 100),
        static_cast<F32>(ground_top),
        static_cast<F32>((screen_width * 55) / 100),
        static_cast<F32>(ground_top),
        5.f,
        1.f);

    draw_scene_test_texture_quad(
        resources.mRoadTexture,
        static_cast<F32>((screen_width * 54) / 100),
        static_cast<F32>(ground_top - margin),
        static_cast<F32>((screen_width * 97) / 100),
        static_cast<F32>(ground_top + margin),
        static_cast<F32>((screen_width * 51) / 100),
        static_cast<F32>(ground_top + margin * 3),
        static_cast<F32>((screen_width * 98) / 100),
        static_cast<F32>(ground_top + margin * 5),
        7.f,
        1.f);

    draw_scene_test_texture_rect(
        backend,
        resources.mCheckerTexture,
        margin,
        ground_top + margin,
        screen_width / 5,
        screen_height / 5,
        4.f,
        3.f);
    draw_scene_test_color_rect(
        backend,
        LLColor4(0.82f, 0.28f, 0.18f, 1.f),
        margin * 4,
        ground_top + margin * 2,
        screen_width / 6,
        screen_height / 4);
    draw_scene_test_color_triangle(
        LLColor4(0.98f, 0.72f, 0.22f, 1.f),
        static_cast<F32>(margin * 4),
        static_cast<F32>(ground_top + margin * 2 + screen_height / 4),
        static_cast<F32>(margin * 4 + screen_width / 6),
        static_cast<F32>(ground_top + margin * 2 + screen_height / 4),
        static_cast<F32>(margin * 4 + screen_width / 12),
        static_cast<F32>(ground_top + margin * 5 + screen_height / 4));

    draw_scene_test_color_rect(
        backend,
        LLColor4(0.20f, 0.22f, 0.27f, 1.f),
        (screen_width * 54) / 100,
        ground_top + margin,
        screen_width / 7,
        screen_height / 3);
    draw_scene_test_color_rect(
        backend,
        LLColor4(0.72f, 0.86f, 1.f, 0.42f),
        (screen_width * 56) / 100,
        ground_top + margin * 3,
        screen_width / 12,
        screen_height / 7);

    draw_scene_test_color_rect(
        backend,
        LLColor4(0.13f, 0.10f, 0.08f, 1.f),
        (screen_width * 75) / 100,
        ground_top + margin,
        screen_width / 18,
        screen_height / 5);
    draw_scene_test_color_rect(
        backend,
        LLColor4(0.76f, 0.52f, 0.38f, 1.f),
        (screen_width * 74) / 100,
        ground_top + margin * 3,
        screen_width / 14,
        screen_height / 6);
    draw_scene_test_disc(
        LLColor4(0.82f, 0.62f, 0.48f, 1.f),
        static_cast<F32>((screen_width * 775) / 1000),
        static_cast<F32>(ground_top + margin * 3 + screen_height / 6),
        static_cast<F32>(llmax(12, screen_width / 58)),
        18);
    draw_scene_test_texture_rect(
        backend,
        resources.mAlphaTexture,
        (screen_width * 72) / 100,
        ground_top + margin * 2,
        screen_width / 9,
        screen_height / 4,
        2.f,
        2.f);

    draw_scene_test_texture_quad(
        resources.mCheckerTexture,
        static_cast<F32>((screen_width * 82) / 100),
        static_cast<F32>(ground_top + margin * 2),
        static_cast<F32>((screen_width * 94) / 100),
        static_cast<F32>(ground_top + margin * 3),
        static_cast<F32>((screen_width * 80) / 100),
        static_cast<F32>(ground_top + margin * 7),
        static_cast<F32>((screen_width * 92) / 100),
        static_cast<F32>(ground_top + margin * 8),
        3.f,
        2.f,
        LLColor4(1.f, 1.f, 1.f, 0.88f));

    draw_scene_test_color_rect(
        backend,
        LLColor4(0.02f, 0.03f, 0.04f, 0.72f),
        margin,
        screen_height - margin * 5,
        screen_width / 4,
        margin * 4);
    draw_scene_test_color_rect(
        backend,
        LLColor4(0.08f, 0.52f, 0.94f, 0.90f),
        margin * 2,
        screen_height - margin * 4,
        margin * 3,
        margin);
    draw_scene_test_color_rect(
        backend,
        LLColor4(0.94f, 0.18f, 0.62f, 0.80f),
        margin * 2,
        screen_height - margin * 25 / 10,
        margin * 5,
        margin);
    draw_scene_test_color_rect(
        backend,
        LLColor4(0.08f, 0.10f, 0.12f, 0.64f),
        screen_width - margin * 9,
        margin,
        margin * 8,
        margin * 4);
    draw_scene_test_texture_rect(
        backend,
        resources.mAlphaTexture,
        screen_width - margin * 8,
        margin * 2,
        margin * 2,
        margin * 2,
        1.f,
        1.f);
    draw_scene_test_color_rect(
        backend,
        LLColor4(0.10f, 0.72f, 0.96f, 0.82f),
        screen_width - margin * 5,
        margin * 2,
        margin * 3,
        margin);

    finish_scene_test_immediate_state(backend, screen_width, screen_height);
    return true;
}

bool render_scene_test_complete_direct_frame(
    LLRenderBackend& backend,
    SceneTestResources& resources,
    U32 width,
    U32 height)
{
    if (!ensure_scene_test_resources(backend, resources) ||
        !ensure_scene_direct_renderer(backend, resources))
    {
        std::cerr << "Failed to create direct complete scene resources.\n";
        return false;
    }

    const S32 screen_width = static_cast<S32>(width);
    const S32 screen_height = static_cast<S32>(height);
    const S32 margin = llmax(18, screen_width / 42);
    const S32 horizon = (screen_height * 53) / 100;
    const S32 water_top = (screen_height * 44) / 100;
    const S32 ground_top = (screen_height * 36) / 100;

    prepare_scene_direct_state(backend, screen_width, screen_height);

    draw_scene_direct_color_rect(
        backend,
        resources,
        LLColor4(0.10f, 0.22f, 0.43f, 1.f),
        0,
        horizon,
        screen_width,
        screen_height - horizon,
        screen_width,
        screen_height);
    draw_scene_direct_color_rect(
        backend,
        resources,
        LLColor4(0.34f, 0.58f, 0.78f, 0.92f),
        0,
        water_top,
        screen_width,
        horizon - water_top,
        screen_width,
        screen_height);
    draw_scene_direct_disc(
        backend,
        resources,
        LLColor4(1.f, 0.85f, 0.28f, 0.88f),
        static_cast<F32>((screen_width * 78) / 100),
        static_cast<F32>((screen_height * 82) / 100),
        static_cast<F32>(llmax(18, screen_width / 28)),
        32,
        screen_width,
        screen_height);

    draw_scene_direct_color_rect(
        backend,
        resources,
        LLColor4(0.09f, 0.33f, 0.16f, 1.f),
        0,
        0,
        screen_width,
        ground_top,
        screen_width,
        screen_height);
    draw_scene_direct_color_rect(
        backend,
        resources,
        LLColor4(0.06f, 0.24f, 0.14f, 0.80f),
        0,
        ground_top - margin,
        screen_width,
        margin * 2,
        screen_width,
        screen_height);
    draw_scene_direct_color_rect(
        backend,
        resources,
        LLColor4(0.10f, 0.38f, 0.62f, 0.62f),
        0,
        ground_top,
        screen_width,
        water_top - ground_top,
        screen_width,
        screen_height);
    draw_scene_direct_color_rect(
        backend,
        resources,
        LLColor4(0.80f, 0.95f, 1.f, 0.28f),
        0,
        water_top - margin,
        screen_width,
        llmax(2, margin / 5),
        screen_width,
        screen_height);

    draw_scene_direct_texture_quad(
        backend,
        resources,
        resources.mRoadTexture,
        static_cast<F32>((screen_width * 18) / 100),
        0.f,
        static_cast<F32>((screen_width * 32) / 100),
        0.f,
        static_cast<F32>((screen_width * 45) / 100),
        static_cast<F32>(ground_top),
        static_cast<F32>((screen_width * 55) / 100),
        static_cast<F32>(ground_top),
        5.f,
        1.f,
        LLColor4::white,
        screen_width,
        screen_height);

    draw_scene_direct_texture_quad(
        backend,
        resources,
        resources.mRoadTexture,
        static_cast<F32>((screen_width * 54) / 100),
        static_cast<F32>(ground_top - margin),
        static_cast<F32>((screen_width * 97) / 100),
        static_cast<F32>(ground_top + margin),
        static_cast<F32>((screen_width * 51) / 100),
        static_cast<F32>(ground_top + margin * 3),
        static_cast<F32>((screen_width * 98) / 100),
        static_cast<F32>(ground_top + margin * 5),
        7.f,
        1.f,
        LLColor4::white,
        screen_width,
        screen_height);

    draw_scene_direct_texture_rect(
        backend,
        resources,
        resources.mCheckerTexture,
        margin,
        ground_top + margin,
        screen_width / 5,
        screen_height / 5,
        4.f,
        3.f,
        LLColor4::white,
        screen_width,
        screen_height);
    draw_scene_direct_color_rect(
        backend,
        resources,
        LLColor4(0.82f, 0.28f, 0.18f, 1.f),
        margin * 4,
        ground_top + margin * 2,
        screen_width / 6,
        screen_height / 4,
        screen_width,
        screen_height);
    draw_scene_direct_color_triangle(
        backend,
        resources,
        LLColor4(0.98f, 0.72f, 0.22f, 1.f),
        static_cast<F32>(margin * 4),
        static_cast<F32>(ground_top + margin * 2 + screen_height / 4),
        static_cast<F32>(margin * 4 + screen_width / 6),
        static_cast<F32>(ground_top + margin * 2 + screen_height / 4),
        static_cast<F32>(margin * 4 + screen_width / 12),
        static_cast<F32>(ground_top + margin * 5 + screen_height / 4),
        screen_width,
        screen_height);

    draw_scene_direct_color_rect(
        backend,
        resources,
        LLColor4(0.20f, 0.22f, 0.27f, 1.f),
        (screen_width * 54) / 100,
        ground_top + margin,
        screen_width / 7,
        screen_height / 3,
        screen_width,
        screen_height);
    draw_scene_direct_color_rect(
        backend,
        resources,
        LLColor4(0.72f, 0.86f, 1.f, 0.42f),
        (screen_width * 56) / 100,
        ground_top + margin * 3,
        screen_width / 12,
        screen_height / 7,
        screen_width,
        screen_height);

    draw_scene_direct_color_rect(
        backend,
        resources,
        LLColor4(0.13f, 0.10f, 0.08f, 1.f),
        (screen_width * 75) / 100,
        ground_top + margin,
        screen_width / 18,
        screen_height / 5,
        screen_width,
        screen_height);
    draw_scene_direct_color_rect(
        backend,
        resources,
        LLColor4(0.76f, 0.52f, 0.38f, 1.f),
        (screen_width * 74) / 100,
        ground_top + margin * 3,
        screen_width / 14,
        screen_height / 6,
        screen_width,
        screen_height);
    draw_scene_direct_disc(
        backend,
        resources,
        LLColor4(0.82f, 0.62f, 0.48f, 1.f),
        static_cast<F32>((screen_width * 775) / 1000),
        static_cast<F32>(ground_top + margin * 3 + screen_height / 6),
        static_cast<F32>(llmax(12, screen_width / 58)),
        18,
        screen_width,
        screen_height);
    draw_scene_direct_texture_rect(
        backend,
        resources,
        resources.mAlphaTexture,
        (screen_width * 72) / 100,
        ground_top + margin * 2,
        screen_width / 9,
        screen_height / 4,
        2.f,
        2.f,
        LLColor4::white,
        screen_width,
        screen_height);

    draw_scene_direct_texture_quad(
        backend,
        resources,
        resources.mCheckerTexture,
        static_cast<F32>((screen_width * 82) / 100),
        static_cast<F32>(ground_top + margin * 2),
        static_cast<F32>((screen_width * 94) / 100),
        static_cast<F32>(ground_top + margin * 3),
        static_cast<F32>((screen_width * 80) / 100),
        static_cast<F32>(ground_top + margin * 7),
        static_cast<F32>((screen_width * 92) / 100),
        static_cast<F32>(ground_top + margin * 8),
        3.f,
        2.f,
        LLColor4(1.f, 1.f, 1.f, 0.88f),
        screen_width,
        screen_height);

    draw_scene_direct_color_rect(
        backend,
        resources,
        LLColor4(0.02f, 0.03f, 0.04f, 0.72f),
        margin,
        screen_height - margin * 5,
        screen_width / 4,
        margin * 4,
        screen_width,
        screen_height);
    draw_scene_direct_color_rect(
        backend,
        resources,
        LLColor4(0.08f, 0.52f, 0.94f, 0.90f),
        margin * 2,
        screen_height - margin * 4,
        margin * 3,
        margin,
        screen_width,
        screen_height);
    draw_scene_direct_color_rect(
        backend,
        resources,
        LLColor4(0.94f, 0.18f, 0.62f, 0.80f),
        margin * 2,
        screen_height - margin * 25 / 10,
        margin * 5,
        margin,
        screen_width,
        screen_height);
    draw_scene_direct_color_rect(
        backend,
        resources,
        LLColor4(0.08f, 0.10f, 0.12f, 0.64f),
        screen_width - margin * 9,
        margin,
        margin * 8,
        margin * 4,
        screen_width,
        screen_height);
    draw_scene_direct_texture_rect(
        backend,
        resources,
        resources.mAlphaTexture,
        screen_width - margin * 8,
        margin * 2,
        margin * 2,
        margin * 2,
        1.f,
        1.f,
        LLColor4::white,
        screen_width,
        screen_height);
    draw_scene_direct_color_rect(
        backend,
        resources,
        LLColor4(0.10f, 0.72f, 0.96f, 0.82f),
        screen_width - margin * 5,
        margin * 2,
        margin * 3,
        margin,
        screen_width,
        screen_height);

    finish_scene_direct_state(backend, screen_width, screen_height);
    return true;
}

bool render_scene_test_complete_deferred_frame(
    LLRenderBackend& backend,
    SceneTestResources& resources,
    U32 width,
    U32 height)
{
    if (!ensure_scene_test_resources(backend, resources) ||
        !ensure_scene_deferred_graph(backend, resources, width, height))
    {
        std::cerr << "Failed to create complete deferred scene resources.\n";
        return false;
    }

    const S32 screen_width = static_cast<S32>(width);
    const S32 screen_height = static_cast<S32>(height);
    const S32 margin = llmax(18, screen_width / 42);
    const S32 horizon = (screen_height * 53) / 100;
    const S32 water_top = (screen_height * 44) / 100;
    const S32 ground_top = (screen_height * 36) / 100;

    backend.bindReadWriteFramebuffer(resources.mGBufferFramebuffer);
    backend.setFramebufferBufferRouting(4);
    backend.setViewport(0, 0, screen_width, screen_height);
    backend.setScissor(0, 0, screen_width, screen_height);
    backend.setClearColor(0.f, 0.f, 0.f, 0.f);
    backend.clear(LL_RENDER_CLEAR_COLOR | LL_RENDER_CLEAR_DEPTH);
    backend.setCapability(LLRenderCapability::DepthTest, false);
    backend.setDepthWriteEnabled(false);
    backend.setCapability(LLRenderCapability::CullFace, false);
    backend.setCapability(LLRenderCapability::Blend, false);
    backend.setColorMask({ true, true, true, true });
    backend.setWorldDrawEnabled(true);
    backend.setWorldTerrainParameters({});
    backend.setWorldTextureTransform({});
    backend.setWorldSkinningMatrixPalette(0, nullptr);
    bind_scene_world_quad(backend, resources.mWorldQuad);
    const auto bind_world_scene_texture =
        [&](LLRenderTextureHandle texture)
        {
            const LLRenderTextureHandle bindings[] =
            {
                texture,
                resources.mWhiteTexture,
                resources.mWhiteTexture,
                resources.mWhiteTexture,
            };
            for (S32 unit = 0; unit < 4; ++unit)
            {
                backend.setActiveTextureUnit(unit);
                backend.bindTexture(LLRenderTextureTarget::Texture2D, bindings[unit]);
            }
            backend.setActiveTextureUnit(0);
        };
    const auto draw_deferred_tile =
        [&](LLRenderTextureHandle texture,
            const LLColor4& color,
            S32 x,
            S32 y,
            S32 tile_width,
            S32 tile_height,
            LLRenderWorldShaderClass shader_class = LLRenderWorldShaderClass::Textured,
            U32 flags = 0)
        {
            if (tile_width <= 0 || tile_height <= 0)
            {
                return;
            }
            backend.setScissor(x, y, tile_width, tile_height);
            bind_world_scene_texture(texture);
            backend.setWorldShaderClass(shader_class);
            backend.setWorldMaterialParameters(make_scene_deferred_material(color, flags));
            backend.drawArrays(LLRenderPrimitiveType::Triangles, 0, 6);
        };

    draw_deferred_tile(
        resources.mWhiteTexture,
        LLColor4(0.10f, 0.22f, 0.43f, 1.f),
        0,
        horizon,
        screen_width,
        screen_height - horizon);
    draw_deferred_tile(
        resources.mWhiteTexture,
        LLColor4(0.34f, 0.58f, 0.78f, 1.f),
        0,
        water_top,
        screen_width,
        horizon - water_top);
    draw_deferred_tile(
        resources.mWhiteTexture,
        LLColor4(0.09f, 0.33f, 0.16f, 1.f),
        0,
        0,
        screen_width,
        ground_top);
    draw_deferred_tile(
        resources.mWhiteTexture,
        LLColor4(0.10f, 0.38f, 0.62f, 1.f),
        0,
        ground_top,
        screen_width,
        water_top - ground_top);
    draw_deferred_tile(
        resources.mRoadTexture,
        LLColor4::white,
        (screen_width * 18) / 100,
        0,
        (screen_width * 37) / 100,
        ground_top);
    draw_deferred_tile(
        resources.mRoadTexture,
        LLColor4::white,
        (screen_width * 54) / 100,
        ground_top - margin,
        (screen_width * 44) / 100,
        margin * 6);
    draw_deferred_tile(
        resources.mCheckerTexture,
        LLColor4::white,
        margin,
        ground_top + margin,
        screen_width / 5,
        screen_height / 5);
    draw_deferred_tile(
        resources.mWhiteTexture,
        LLColor4(0.82f, 0.28f, 0.18f, 1.f),
        margin * 4,
        ground_top + margin * 2,
        screen_width / 6,
        screen_height / 4,
        LLRenderWorldShaderClass::Material,
        LLRenderWorldMaterialParameters::LegacyShiny);
    draw_deferred_tile(
        resources.mWhiteTexture,
        LLColor4(0.20f, 0.22f, 0.27f, 1.f),
        (screen_width * 54) / 100,
        ground_top + margin,
        screen_width / 7,
        screen_height / 3,
        LLRenderWorldShaderClass::Material);
    draw_deferred_tile(
        resources.mWhiteTexture,
        LLColor4(0.76f, 0.52f, 0.38f, 1.f),
        (screen_width * 74) / 100,
        ground_top + margin * 3,
        screen_width / 14,
        screen_height / 6,
        LLRenderWorldShaderClass::Avatar);
    draw_deferred_tile(
        resources.mAlphaTexture,
        LLColor4(1.f, 1.f, 1.f, 1.f),
        (screen_width * 72) / 100,
        ground_top + margin * 2,
        screen_width / 9,
        screen_height / 4,
        LLRenderWorldShaderClass::AlphaMask,
        LLRenderWorldMaterialParameters::AlphaMask);
    draw_deferred_tile(
        resources.mCheckerTexture,
        LLColor4(1.f, 1.f, 1.f, 1.f),
        (screen_width * 82) / 100,
        ground_top + margin * 2,
        screen_width / 8,
        margin * 6,
        LLRenderWorldShaderClass::PBR,
        LLRenderWorldMaterialParameters::GLTFPBR);

    backend.bindReadWriteFramebuffer(resources.mDeferredFramebuffer);
    backend.setFramebufferBufferRouting(1);
    backend.setViewport(0, 0, screen_width, screen_height);
    backend.setScissor(0, 0, screen_width, screen_height);
    backend.setClearColor(0.f, 0.f, 0.f, 1.f);
    backend.clear(LL_RENDER_CLEAR_COLOR);
    bind_scene_gbuffer_textures(backend, resources);
    backend.setCapability(LLRenderCapability::Blend, false);
    backend.setCapability(LLRenderCapability::DepthTest, false);
    backend.setDepthWriteEnabled(false);
    backend.setWorldDrawEnabled(true);
    backend.setWorldShaderClass(LLRenderWorldShaderClass::DeferredComposite);
    backend.setWorldMaterialParameters(make_scene_deferred_composite_parameters());
    bind_scene_world_quad(backend, resources.mWorldQuad);
    backend.drawArrays(LLRenderPrimitiveType::Triangles, 0, 6);

    backend.bindReadWriteFramebuffer(LLRenderFramebufferHandle());
    backend.restoreDefaultFramebufferBufferRouting();
    backend.setViewport(0, 0, screen_width, screen_height);
    backend.setScissor(0, 0, screen_width, screen_height);
    backend.setClearColor(0.01f, 0.012f, 0.018f, 1.f);
    backend.clear(LL_RENDER_CLEAR_COLOR | LL_RENDER_CLEAR_DEPTH);
    bind_scene_final_textures(backend, resources);
    backend.setWorldShaderClass(LLRenderWorldShaderClass::FinalComposite);
    backend.setWorldMaterialParameters(make_scene_final_composite_parameters());
    bind_scene_world_quad(backend, resources.mWorldQuad);
    backend.drawArrays(LLRenderPrimitiveType::Triangles, 0, 6);

    backend.setWorldDrawEnabled(false);
    backend.setWorldShaderClass(LLRenderWorldShaderClass::Textured);
    backend.setWorldMaterialParameters({});
    backend.setWorldTerrainParameters({});
    backend.setWorldTextureTransform({});
    backend.setWorldSkinningMatrixPalette(0, nullptr);
    backend.setCapability(LLRenderCapability::Blend, true);
    backend.setBlendState(
        {
            LLRenderBlendFactor::SourceAlpha,
            LLRenderBlendFactor::OneMinusSourceAlpha,
            LLRenderBlendFactor::One,
            LLRenderBlendFactor::OneMinusSourceAlpha,
        });
    gl_state_for_2d(screen_width, screen_height);
    gUIProgram.bind();
    draw_scene_test_color_rect(
        backend,
        LLColor4(0.02f, 0.03f, 0.04f, 0.72f),
        margin,
        screen_height - margin * 5,
        screen_width / 4,
        margin * 4);
    draw_scene_test_color_rect(
        backend,
        LLColor4(0.08f, 0.52f, 0.94f, 0.90f),
        margin * 2,
        screen_height - margin * 4,
        margin * 3,
        margin);
    draw_scene_test_color_rect(
        backend,
        LLColor4(0.08f, 0.10f, 0.12f, 0.64f),
        screen_width - margin * 9,
        margin,
        margin * 8,
        margin * 4);
    draw_scene_test_texture_rect(
        backend,
        resources.mAlphaTexture,
        screen_width - margin * 8,
        margin * 2,
        margin * 2,
        margin * 2,
        1.f,
        1.f);
    draw_scene_test_color_rect(
        backend,
        LLColor4(0.10f, 0.72f, 0.96f, 0.82f),
        screen_width - margin * 5,
        margin * 2,
        margin * 3,
        margin);
    gGL.flush();
    gUIProgram.unbind();
    backend.setCapability(LLRenderCapability::Blend, false);
    backend.setScissor(0, 0, screen_width, screen_height);
    backend.setActiveTextureUnit(0);
    return true;
}
}

int main(int argc, char** argv)
{
    SceneTestOptions options;
    if (!parse_scene_test_options(argc, argv, options))
    {
        print_usage(argv[0] ? argv[0] : "mare-vulkan-scene-test");
        return 1;
    }
    if (options.mHelp)
    {
        print_usage(argv[0] ? argv[0] : "mare-vulkan-scene-test");
        return 0;
    }
    if (options.mRenderPath == "deferred" && options.mScene != "complete")
    {
        std::cerr << "--render-path deferred is currently supported only with --scene complete.\n";
        return 1;
    }

    if (!options.mVulkanSDK.empty())
    {
        setenv("VULKAN_SDK", options.mVulkanSDK.c_str(), 1);
    }
    setenv("MARE_RENDER_BACKEND", options.mBackend.c_str(), 1);
    if (options.mBackend == "vulkan")
    {
        setenv("MARE_VULKAN_SMOKE_TEST", "1", 0);
    }
    if (!options.mScreenshotPPMPath.empty())
    {
        setenv("MARE_VULKAN_SMOKE_SCREENSHOT_PPM", options.mScreenshotPPMPath.c_str(), 1);
        const std::string min_frame =
            std::to_string(options.mScreenshotMinFrame);
        setenv("MARE_VULKAN_SMOKE_SCREENSHOT_MIN_FRAME", min_frame.c_str(), 1);
    }
    else
    {
        unsetenv("MARE_VULKAN_SMOKE_SCREENSHOT_PPM");
        unsetenv("MARE_VULKAN_SMOKE_SCREENSHOT_MIN_FRAME");
    }

    void* window = mare_vulkan_test::create_window(
        options.mWidth,
        options.mHeight,
        "Mare Vulkan Scene Test");
    if (!window)
    {
        std::cerr << "Failed to create scene test window.\n";
        return 1;
    }

    LLRenderBackend& backend = getRenderBackend();
    if (backend.getType() != expected_backend_type(options.mBackend))
    {
        std::cerr
            << "Expected "
            << options.mBackend
            << " backend, got "
            << backend.getName()
            << ".\n";
        mare_vulkan_test::destroy_window(window);
        return 2;
    }

    LLRenderNativeContext context;
    LLRenderNativeContextDesc desc;
    desc.mWindow = window;
    desc.mSamples = 0;
    desc.mEnableVSync = false;

    if (!backend.createNativeContext(desc, context))
    {
        std::cerr << "Failed to create native render context.\n";
        mare_vulkan_test::destroy_window(window);
        return 3;
    }
    if (!backend.makeNativeContextCurrent(context.mContext))
    {
        std::cerr << "Failed to make native render context current.\n";
        backend.destroyNativeContext(context);
        mare_vulkan_test::destroy_window(window);
        return 4;
    }

    backend.initPlatformContextExtensions();
    if (!backend.initContextCapabilities())
    {
        std::cerr << "Failed to initialize render context capabilities.\n";
        backend.destroyNativeContext(context);
        mare_vulkan_test::destroy_window(window);
        return 5;
    }

    const S32 quality_level = scene_test_quality_level(options.mQuality);
    backend.setWorldDeferredShaderLevel(quality_level);

    bool render_state_initialized = false;
    bool vertex_buffer_class_initialized = false;
    if (scene_uses_llrender(options.mScene, options.mRenderPath))
    {
        if (backend.getType() == LLRenderBackendType::OpenGL &&
            options.mRenderPath == "deferred")
        {
            std::cerr
                << "--backend opengl currently supports --render-path immediate "
                << "for synthetic scenes. Deferred comparison still needs the "
                << "viewer deferred graph reference path.\n";
            backend.shutdownContextCapabilities();
            backend.destroyNativeContext(context);
            mare_vulkan_test::destroy_window(window);
            return 6;
        }

        const bool needs_vulkan_ui_bridge =
            backend.getType() == LLRenderBackendType::Vulkan;
        if (needs_vulkan_ui_bridge)
        {
            LLVertexBuffer::initClass(nullptr);
            vertex_buffer_class_initialized = true;
        }
        if (!gGL.init(needs_vulkan_ui_bridge))
        {
            std::cerr << "Failed to initialize LLRender scene test state.\n";
            if (vertex_buffer_class_initialized)
            {
                LLVertexBuffer::cleanupClass();
            }
            backend.shutdownContextCapabilities();
            backend.destroyNativeContext(context);
            mare_vulkan_test::destroy_window(window);
            return 6;
        }
        render_state_initialized = true;
        if (needs_vulkan_ui_bridge)
        {
            gUIProgram.mName = "Mare Vulkan scene test UI";
            gUIProgram.mAttributeMask =
                LLVertexBuffer::MAP_VERTEX |
                LLVertexBuffer::MAP_TEXCOORD0 |
                LLVertexBuffer::MAP_COLOR;
        }
    }

    std::cout
        << "Mare Vulkan scene test started. Backend: "
        << backend.getName()
        << ", scene: "
        << options.mScene
        << ", render path: "
        << options.mRenderPath
        << ", quality: "
        << scene_test_quality_name(quality_level)
        << " deferred shader level "
        << quality_level
        << ", frames: "
        << options.mFrameLimit
        << ", screenshot: "
        << (options.mScreenshotPPMPath.empty() ? "off" : options.mScreenshotPPMPath)
        << ", clear rgba: 0.0700, 0.1600, 0.3100, 1.0000."
        << std::endl;

    int frame = 0;
    bool render_failed = false;
    bool screenshot_written = false;
    SceneTestResources resources;
    while (mare_vulkan_test::pump_events(window) &&
        (options.mFrameLimit == 0 || frame < options.mFrameLimit))
    {
        std::uint32_t width = 1;
        std::uint32_t height = 1;
        mare_vulkan_test::get_view_size(context.mView, &width, &height);
        bool rendered = false;
        if (options.mScene == "primitives")
        {
            rendered = backend.getType() == LLRenderBackendType::OpenGL ?
                render_scene_test_primitives_direct_frame(
                    backend,
                    resources,
                    width,
                    height) :
                render_scene_test_primitives_frame(
                    backend,
                    resources,
                    width,
                    height);
        }
        else if (options.mScene == "complete")
        {
            rendered =
                backend.getType() == LLRenderBackendType::OpenGL ?
                render_scene_test_complete_direct_frame(
                    backend,
                    resources,
                    width,
                    height) :
                options.mRenderPath == "deferred" ?
                render_scene_test_complete_deferred_frame(
                    backend,
                    resources,
                    width,
                    height) :
                render_scene_test_complete_frame(
                    backend,
                    resources,
                    width,
                    height);
        }
        else
        {
            rendered = render_scene_test_frame(backend, width, height);
        }
        if (!rendered)
        {
            std::cerr << "Failed to render scene test frame.\n";
            render_failed = true;
            break;
        }

        if (!options.mScreenshotPPMPath.empty() &&
            backend.getType() == LLRenderBackendType::OpenGL &&
            !screenshot_written &&
            frame >= options.mScreenshotMinFrame)
        {
            std::vector<U8> rgba_pixels(
                static_cast<size_t>(width) *
                static_cast<size_t>(height) *
                4U);
            backend.readPixels(
                0,
                0,
                static_cast<S32>(width),
                static_cast<S32>(height),
                LLRenderPixelFormat::RGBA,
                LLRenderPixelType::UnsignedByte,
                rgba_pixels.data());
            flip_scene_rgba_rows(rgba_pixels, width, height);
            if (!mare_vulkan_test::write_rgba_readback_as_rgb_ppm(
                    options.mScreenshotPPMPath,
                    width,
                    height,
                    rgba_pixels))
            {
                std::cerr << "Failed to write scene test screenshot.\n";
                render_failed = true;
                break;
            }
            screenshot_written = true;
        }
        backend.swapNativeBuffers(context.mContext);

        std::cout
            << "Mare Vulkan scene test frame "
            << frame
            << ": "
            << width
            << "x"
            << height
            << " submitted.\n";
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
        ++frame;
    }

    release_scene_test_resources(backend, resources);
    if (render_state_initialized)
    {
        gGL.shutdown();
        if (vertex_buffer_class_initialized)
        {
            LLVertexBuffer::cleanupClass();
        }
    }
    backend.shutdownContextCapabilities();
    backend.destroyNativeContext(context);
    mare_vulkan_test::destroy_window(window);
    return render_failed ? 7 : 0;
}
