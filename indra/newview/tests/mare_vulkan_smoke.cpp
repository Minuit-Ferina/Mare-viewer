#include "linden_common.h"

#include "llglslshader.h"
#include "llrender.h"
#include "llrender2dutils.h"
#include "llrenderbackend.h"
#include "llrenderbackendvulkan.h"
#include "llrenderstate.h"
#include "llrendertarget.h"
#include "llvertexbuffer.h"
#include "llvulkancompositeparams.h"
#include "../llworldrendercommand.h"

#include "mare_vulkan_smoke_macosx.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <sstream>
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
    ViewerStagedLightTarget,
    ViewerStagedScreenTarget,
    ViewerStagedScreenOverlays,
    ViewerStagedReusedLightTarget,
    ViewerStagedReusedLightOverlays,
    ViewerStagedPostOverlays,
    ViewerStagedPostCopy,
    ViewerStagedPostTargets,
    CopyChain,
    CopyMRTChain,
    WorldPipelines,
};

enum class SmokeViewerStagedStop
{
    DeferredLight,
    Screen,
    ReusedDeferredLight,
    FinalPostTarget,
};

enum class SmokeScene
{
    Basic,
    PostOverlaysStress,
    TwoPrims,
    ReplayCapture,
};

struct SmokeOptions
{
    SmokeMode mMode = SmokeMode::DirectClear;
    SmokeScene mScene = SmokeScene::Basic;
    int mFrameLimit = 0;
    int mLogInterval = 60;
    int mReadbackFrameLimit = 4;
    bool mStdoutReadback = true;
    bool mBufferReadback = true;
    bool mFrameDiff = false;
    bool mFrameDiffSummaryOnly = false;
    bool mRenderUI = false;
    bool mRenderViewerUISequence = false;
    bool mRenderSceneMarker = false;
    bool mHelp = false;
    std::string mCapturePath;
    std::string mScreenshotPPMPath;
    int mScreenshotMinFrame = 0;
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
    if (std::strcmp(value, "viewer-staged-light-target") == 0)
    {
        mode = SmokeMode::ViewerStagedLightTarget;
        return true;
    }
    if (std::strcmp(value, "viewer-staged-screen-target") == 0)
    {
        mode = SmokeMode::ViewerStagedScreenTarget;
        return true;
    }
    if (std::strcmp(value, "viewer-staged-screen-overlays") == 0)
    {
        mode = SmokeMode::ViewerStagedScreenOverlays;
        return true;
    }
    if (std::strcmp(value, "viewer-staged-reused-light-target") == 0)
    {
        mode = SmokeMode::ViewerStagedReusedLightTarget;
        return true;
    }
    if (std::strcmp(value, "viewer-staged-reused-light-overlays") == 0)
    {
        mode = SmokeMode::ViewerStagedReusedLightOverlays;
        return true;
    }
    if (std::strcmp(value, "viewer-staged-post-overlays") == 0)
    {
        mode = SmokeMode::ViewerStagedPostOverlays;
        return true;
    }
    if (std::strcmp(value, "viewer-staged-post-copy") == 0)
    {
        mode = SmokeMode::ViewerStagedPostCopy;
        return true;
    }
    if (std::strcmp(value, "viewer-staged-post-targets") == 0)
    {
        mode = SmokeMode::ViewerStagedPostTargets;
        return true;
    }
    if (std::strcmp(value, "copy-chain") == 0)
    {
        mode = SmokeMode::CopyChain;
        return true;
    }
    if (std::strcmp(value, "copy-mrt-chain") == 0)
    {
        mode = SmokeMode::CopyMRTChain;
        return true;
    }
    if (std::strcmp(value, "world-pipelines") == 0)
    {
        mode = SmokeMode::WorldPipelines;
        return true;
    }
    return false;
}

bool parse_smoke_scene_value(const char* value, SmokeScene& scene)
{
    if (!value)
    {
        return false;
    }
    if (std::strcmp(value, "basic") == 0)
    {
        scene = SmokeScene::Basic;
        return true;
    }
    if (std::strcmp(value, "post-overlays-stress") == 0)
    {
        scene = SmokeScene::PostOverlaysStress;
        return true;
    }
    if (std::strcmp(value, "two-prims") == 0)
    {
        scene = SmokeScene::TwoPrims;
        return true;
    }
    if (std::strcmp(value, "replay-capture") == 0)
    {
        scene = SmokeScene::ReplayCapture;
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
    case SmokeMode::ViewerStagedLightTarget:
        return "viewer-staged-light-target";
    case SmokeMode::ViewerStagedScreenTarget:
        return "viewer-staged-screen-target";
    case SmokeMode::ViewerStagedScreenOverlays:
        return "viewer-staged-screen-overlays";
    case SmokeMode::ViewerStagedReusedLightTarget:
        return "viewer-staged-reused-light-target";
    case SmokeMode::ViewerStagedReusedLightOverlays:
        return "viewer-staged-reused-light-overlays";
    case SmokeMode::ViewerStagedPostOverlays:
        return "viewer-staged-post-overlays";
    case SmokeMode::ViewerStagedPostCopy:
        return "viewer-staged-post-copy";
    case SmokeMode::ViewerStagedPostTargets:
        return "viewer-staged-post-targets";
    case SmokeMode::CopyChain:
        return "copy-chain";
    case SmokeMode::CopyMRTChain:
        return "copy-mrt-chain";
    case SmokeMode::DeferredComposite:
        return "deferred-composite";
    case SmokeMode::OffscreenCopy:
        return "offscreen-copy";
    case SmokeMode::DirectClear:
    default:
        return "direct-clear";
    }
}

const char* get_smoke_scene_name(SmokeScene scene)
{
    switch (scene)
    {
    case SmokeScene::ReplayCapture:
        return "replay-capture";
    case SmokeScene::TwoPrims:
        return "two-prims";
    case SmokeScene::PostOverlaysStress:
        return "post-overlays-stress";
    case SmokeScene::Basic:
    default:
        return "basic";
    }
}

const char* get_smoke_scene_description(SmokeScene scene)
{
    switch (scene)
    {
    case SmokeScene::ReplayCapture:
        return "captured command-shape summary, not live scene geometry/textures";
    case SmokeScene::TwoPrims:
        return "two overlapping prim-like world quads: opaque deferred depth plus post-deferred alpha";
    case SmokeScene::PostOverlaysStress:
        return "stress G-buffer and post-deferred overlay draws";
    case SmokeScene::Basic:
    default:
        return "minimal deterministic deferred scene";
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
    case SmokeMode::ViewerStagedLightTarget:
        return " produced through the viewer-style G-buffer and deferredLight target, then copied to the swapchain. ";
    case SmokeMode::ViewerStagedScreenTarget:
        return " produced through the viewer-style G-buffer, deferredLight target, and screen target, then copied to the swapchain. ";
    case SmokeMode::ViewerStagedScreenOverlays:
        return " produced through the viewer-style G-buffer, deferredLight target, and screen target with post-deferred overlays, then copied to the swapchain. ";
    case SmokeMode::ViewerStagedReusedLightTarget:
        return " produced through the viewer-style staged graph through the screen-to-deferredLight reuse hop. ";
    case SmokeMode::ViewerStagedReusedLightOverlays:
        return " produced through the viewer-style staged graph through post-deferred overlays and the screen-to-deferredLight reuse hop. ";
    case SmokeMode::ViewerStagedPostOverlays:
        return " produced through the viewer-style staged graph with synthetic post-deferred overlays drawn into the screen target. ";
    case SmokeMode::ViewerStagedPostCopy:
        return " produced through the viewer-style staged graph with post-deferred overlays, then copied through the post target without final composite. ";
    case SmokeMode::ViewerStagedPostTargets:
        return " produced through the viewer-style staged post target graph. ";
    case SmokeMode::CopyChain:
        return " rendered as a single-color world target, copied through multiple LLRenderTarget hops with the real Copy shader, then copied to the swapchain. ";
    case SmokeMode::CopyMRTChain:
        return " rendered as a multi-attachment world target, then attachment 0 is copied through multiple LLRenderTarget hops with the real Copy shader. ";
    case SmokeMode::WorldPipelines:
        return " replaced by a grid of real Vulkan world shader pipelines. ";
    case SmokeMode::DirectClear:
    default:
        return ". ";
    }
}

bool smoke_mode_uses_scene(SmokeMode mode)
{
    switch (mode)
    {
    case SmokeMode::DeferredGraph:
    case SmokeMode::ViewerDeferredDirect:
    case SmokeMode::ViewerRenderTargetDirect:
    case SmokeMode::ViewerImmediateDirect:
    case SmokeMode::ViewerStagedLightTarget:
    case SmokeMode::ViewerStagedScreenTarget:
    case SmokeMode::ViewerStagedScreenOverlays:
    case SmokeMode::ViewerStagedReusedLightTarget:
    case SmokeMode::ViewerStagedReusedLightOverlays:
    case SmokeMode::ViewerStagedPostOverlays:
    case SmokeMode::ViewerStagedPostCopy:
    case SmokeMode::ViewerStagedPostTargets:
    case SmokeMode::CopyChain:
    case SmokeMode::CopyMRTChain:
    case SmokeMode::WorldPipelines:
        return true;
    case SmokeMode::DirectClear:
    case SmokeMode::OffscreenCopy:
    case SmokeMode::DeferredComposite:
    default:
        return false;
    }
}

bool smoke_mode_replays_capture(SmokeMode mode)
{
    switch (mode)
    {
    case SmokeMode::DeferredGraph:
    case SmokeMode::ViewerDeferredDirect:
    case SmokeMode::ViewerRenderTargetDirect:
    case SmokeMode::ViewerImmediateDirect:
    case SmokeMode::ViewerStagedLightTarget:
    case SmokeMode::ViewerStagedScreenTarget:
    case SmokeMode::ViewerStagedScreenOverlays:
    case SmokeMode::ViewerStagedReusedLightTarget:
    case SmokeMode::ViewerStagedReusedLightOverlays:
    case SmokeMode::ViewerStagedPostOverlays:
    case SmokeMode::ViewerStagedPostCopy:
    case SmokeMode::ViewerStagedPostTargets:
    case SmokeMode::CopyChain:
    case SmokeMode::CopyMRTChain:
        return true;
    case SmokeMode::DirectClear:
    case SmokeMode::OffscreenCopy:
    case SmokeMode::DeferredComposite:
    case SmokeMode::WorldPipelines:
    default:
        return false;
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
        << "                             viewer-staged-light-target,\n"
        << "                             viewer-staged-screen-target,\n"
        << "                             viewer-staged-screen-overlays,\n"
        << "                             viewer-staged-reused-light-target,\n"
        << "                             viewer-staged-reused-light-overlays,\n"
        << "                             viewer-staged-post-overlays,\n"
        << "                             viewer-staged-post-copy,\n"
        << "                             viewer-staged-post-targets,\n"
        << "                             copy-chain,\n"
        << "                             copy-mrt-chain,\n"
        << "                             world-pipelines\n"
        << "  --scene <name>             basic, post-overlays-stress, two-prims,\n"
        << "                             replay-capture\n"
        << "  --capture <path>           Capture file for --scene replay-capture\n"
        << "  --frames <count>           Number of frames to render; 0 means run until closed\n"
        << "  --log-every <count>        Frame logging interval\n"
        << "  --readback-frames <count>  Number of diagnostic readback frames\n"
        << "  --frame-diff               Compare each final swapchain readback with the previous frame\n"
        << "  --frame-diff-summary-only  Compare frame diffs but print only final summaries\n"
        << "  --screenshot-ppm <path>    Write the first eligible final swapchain readback as PPM\n"
        << "  --screenshot-min-frame <n> First frame eligible for --screenshot-ppm\n"
        << "  --no-buffer-readbacks      Disable G-buffer/composite input readbacks\n"
        << "  --no-stdout-readback       Keep readbacks in normal logs only\n"
        << "  --ui                       Draw a synthetic UI layer after the world/deferred pass\n"
        << "  --ui-viewer-sequence       Draw a stronger viewer-style UI sequence after the world/deferred pass\n"
        << "  --scene-marker             Draw a small scene-identification marker\n"
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

        if (argument == "--scene" ||
            (value = value_after_equals(argument, "--scene")) != nullptr)
        {
            if (!value)
            {
                value = require_value(i, "--scene");
            }
            if (!value || !parse_smoke_scene_value(value, options.mScene))
            {
                std::cerr << "Invalid --scene value '" << (value ? value : "") << "'.\n";
                return false;
            }
            continue;
        }

        if (argument == "--capture" ||
            (value = value_after_equals(argument, "--capture")) != nullptr)
        {
            if (!value)
            {
                value = require_value(i, "--capture");
            }
            if (!value)
            {
                return false;
            }
            options.mCapturePath = value;
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

        if (argument == "--frame-diff")
        {
            options.mFrameDiff = true;
            continue;
        }

        if (argument == "--frame-diff-summary-only")
        {
            options.mFrameDiff = true;
            options.mFrameDiffSummaryOnly = true;
            continue;
        }

        if (argument == "--screenshot-ppm" ||
            (value = value_after_equals(argument, "--screenshot-ppm")) != nullptr)
        {
            if (!value)
            {
                value = require_value(i, "--screenshot-ppm");
            }
            if (!value)
            {
                return false;
            }
            options.mScreenshotPPMPath = value;
            continue;
        }

        if (argument == "--screenshot-min-frame" ||
            (value = value_after_equals(argument, "--screenshot-min-frame")) != nullptr)
        {
            if (!value)
            {
                value = require_value(i, "--screenshot-min-frame");
            }
            if (!parse_nonnegative_int(value, "--screenshot-min-frame", options.mScreenshotMinFrame))
            {
                return false;
            }
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

        if (argument == "--scene-marker")
        {
            options.mRenderSceneMarker = true;
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
    LLRenderTextureHandle mWhite;
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
        &textures.mWhite,
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
        textures.mDepth &&
        textures.mWhite)
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
            make_solid_rgba_pixels(texture_width, texture_height, 255, 255, 255, 255)) ||
        !create_smoke_texture(
            backend,
            textures.mWhite,
            1,
            1,
            make_solid_rgba_pixels(1, 1, 255, 255, 255, 255)))
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

struct SmokeCapturedMaterialTransform
{
    bool mValid = false;
    F32 mScaleS = 1.f;
    F32 mScaleT = 1.f;
    F32 mRotation = 0.f;
    F32 mOffsetS = 0.f;
    F32 mOffsetT = 0.f;
};

struct SmokeCapturedCommand
{
    LLWorldRenderMaterialClass mMaterialClass = LLWorldRenderMaterialClass::SimpleOpaque;
    LLWorldRenderPassClass mPassClass = LLWorldRenderPassClass::Deferred;
    LLWorldRenderBlendMode mBlendMode = LLWorldRenderBlendMode::None;
    LLWorldRenderDepthMode mDepthMode = LLWorldRenderDepthMode::ReadWrite;
    LLWorldRenderCullMode mCullMode = LLWorldRenderCullMode::Back;
    bool mWriteColor = true;
    bool mWriteAlpha = true;
    bool mUseTexture = true;
    bool mBatchTextures = false;
    bool mRigged = false;
    bool mFullbright = false;
    bool mHasGlow = false;
    bool mDoubleSided = false;
    U32 mSourcePass = 0;
    U32 mAttributeMask = 0;
    U32 mCount = 0;
    U32 mMode = LLRender::TRIANGLES;
    LLColor4 mBaseColor = LLColor4(1.f, 1.f, 1.f, 1.f);
    LLColor3 mEmissiveColor = LLColor3(0.f, 0.f, 0.f);
    LLVector4 mSpecColor = LLVector4(1.f, 1.f, 1.f, 0.5f);
    F32 mMetallicFactor = 1.f;
    F32 mRoughnessFactor = 1.f;
    F32 mEnvIntensity = 0.f;
    F32 mAlphaMaskCutoff = 0.5f;
    U8 mDiffuseAlphaMode = 0;
    U8 mGLTFAlphaMode = 0;
    U8 mBump = 0;
    U8 mShiny = 0;
    U32 mTerrainPaintType = 0;
    U32 mTerrainPlanarSampleCount = 1;
    SmokeCapturedMaterialTransform mBaseColorTextureTransform;
    SmokeCapturedMaterialTransform mNormalTextureTransform;
    SmokeCapturedMaterialTransform mORMTextureTransform;
    SmokeCapturedMaterialTransform mEmissiveTextureTransform;
    LLRenderWorldTextureTransform mTextureTransform;
};

std::vector<SmokeCapturedCommand> gSmokeCapturedCommands;

template <typename Enum>
Enum smoke_enum_from_u32(U32 value, Enum fallback)
{
    return static_cast<Enum>(value);
}

void skip_captured_texture_metadata(std::istringstream& stream)
{
    U32 present = 0;
    U32 tex_name = 0;
    S32 width = 0;
    S32 height = 0;
    S32 full_width = 0;
    S32 full_height = 0;
    S32 discard = -1;
    S32 raw_discard = -1;
    stream
        >> present
        >> tex_name
        >> width
        >> height
        >> full_width
        >> full_height
        >> discard
        >> raw_discard;
}

void read_captured_material_transform(
    std::istringstream& stream,
    SmokeCapturedMaterialTransform& transform)
{
    U32 valid = 0;
    stream
        >> valid
        >> transform.mScaleS
        >> transform.mScaleT
        >> transform.mRotation
        >> transform.mOffsetS
        >> transform.mOffsetT;
    transform.mValid = valid != 0;
}

bool load_smoke_capture_file(const std::string& path)
{
    gSmokeCapturedCommands.clear();
    std::ifstream input(path);
    if (!input.is_open())
    {
        std::cerr << "Unable to open smoke capture file '" << path << "'.\n";
        return false;
    }

    std::string line;
    bool saw_header = false;
    while (std::getline(input, line))
    {
        if (line.empty())
        {
            continue;
        }
        if (!saw_header)
        {
            saw_header = line == "MareVulkanWorldCommandCaptureV1";
            if (!saw_header)
            {
                std::cerr << "Invalid smoke capture header in '" << path << "'.\n";
                return false;
            }
            continue;
        }

        std::istringstream stream(line);
        std::string marker;
        stream >> marker;
        if (marker != "cmd")
        {
            continue;
        }

        SmokeCapturedCommand command;
        std::string key;
        while (stream >> key)
        {
            U32 value = 0;
            if (key == "material" && stream >> value)
            {
                command.mMaterialClass =
                    smoke_enum_from_u32(value, command.mMaterialClass);
            }
            else if (key == "pass" && stream >> value)
            {
                command.mPassClass =
                    smoke_enum_from_u32(value, command.mPassClass);
            }
            else if (key == "blend" && stream >> value)
            {
                command.mBlendMode =
                    smoke_enum_from_u32(value, command.mBlendMode);
            }
            else if (key == "depth" && stream >> value)
            {
                command.mDepthMode =
                    smoke_enum_from_u32(value, command.mDepthMode);
            }
            else if (key == "cull" && stream >> value)
            {
                command.mCullMode =
                    smoke_enum_from_u32(value, command.mCullMode);
            }
            else if (key == "write_color" && stream >> value)
            {
                command.mWriteColor = value != 0;
            }
            else if (key == "write_alpha" && stream >> value)
            {
                command.mWriteAlpha = value != 0;
            }
            else if (key == "source_pass" && stream >> command.mSourcePass)
            {
            }
            else if (key == "attributes" && stream >> command.mAttributeMask)
            {
            }
            else if (key == "count" && stream >> command.mCount)
            {
            }
            else if (key == "mode" && stream >> command.mMode)
            {
            }
            else if (key == "draw_arrays" && stream >> value)
            {
            }
            else if (key == "use_texture" && stream >> value)
            {
                command.mUseTexture = value != 0;
            }
            else if (key == "batch_textures" && stream >> value)
            {
                command.mBatchTextures = value != 0;
            }
            else if (key == "rigged" && stream >> value)
            {
                command.mRigged = value != 0;
            }
            else if (key == "fullbright" && stream >> value)
            {
                command.mFullbright = value != 0;
            }
            else if (key == "glow" && stream >> value)
            {
                command.mHasGlow = value != 0;
            }
            else if (key == "double_sided" && stream >> value)
            {
                command.mDoubleSided = value != 0;
            }
            else if (key == "base")
            {
                stream
                    >> command.mBaseColor.mV[VRED]
                    >> command.mBaseColor.mV[VGREEN]
                    >> command.mBaseColor.mV[VBLUE]
                    >> command.mBaseColor.mV[VALPHA];
            }
            else if (key == "emissive")
            {
                stream
                    >> command.mEmissiveColor.mV[VRED]
                    >> command.mEmissiveColor.mV[VGREEN]
                    >> command.mEmissiveColor.mV[VBLUE];
            }
            else if (key == "spec")
            {
                stream
                    >> command.mSpecColor.mV[VX]
                    >> command.mSpecColor.mV[VY]
                    >> command.mSpecColor.mV[VZ]
                    >> command.mSpecColor.mV[VW];
            }
            else if (key == "factors")
            {
                stream
                    >> command.mMetallicFactor
                    >> command.mRoughnessFactor
                    >> command.mEnvIntensity
                    >> command.mAlphaMaskCutoff;
            }
            else if (key == "alpha_modes")
            {
                U32 diffuse_alpha_mode = 0;
                U32 gltf_alpha_mode = 0;
                stream >> diffuse_alpha_mode >> gltf_alpha_mode;
                command.mDiffuseAlphaMode = static_cast<U8>(diffuse_alpha_mode);
                command.mGLTFAlphaMode = static_cast<U8>(gltf_alpha_mode);
            }
            else if (key == "material_modes")
            {
                U32 bump = 0;
                U32 shiny = 0;
                stream >> bump >> shiny;
                command.mBump = static_cast<U8>(bump);
                command.mShiny = static_cast<U8>(shiny);
            }
            else if (key == "terrain")
            {
                stream
                    >> command.mTerrainPaintType
                    >> command.mTerrainPlanarSampleCount;
            }
            else if (key == "primary_texture" ||
                key == "normal_texture" ||
                key == "specular_texture" ||
                key == "orm_texture" ||
                key == "emissive_texture")
            {
                skip_captured_texture_metadata(stream);
            }
            else if (key == "texture_list")
            {
                U32 total_count = 0;
                U32 valid_count = 0;
                stream >> total_count >> valid_count;
            }
            else if (key == "base_transform")
            {
                read_captured_material_transform(
                    stream,
                    command.mBaseColorTextureTransform);
            }
            else if (key == "normal_transform")
            {
                read_captured_material_transform(
                    stream,
                    command.mNormalTextureTransform);
            }
            else if (key == "orm_transform")
            {
                read_captured_material_transform(
                    stream,
                    command.mORMTextureTransform);
            }
            else if (key == "emissive_transform")
            {
                read_captured_material_transform(
                    stream,
                    command.mEmissiveTextureTransform);
            }
            else if (key == "texture_matrix")
            {
                U32 valid = 0;
                stream
                    >> valid
                    >> command.mTextureTransform.mS[0]
                    >> command.mTextureTransform.mS[1]
                    >> command.mTextureTransform.mS[2]
                    >> command.mTextureTransform.mS[3]
                    >> command.mTextureTransform.mT[0]
                    >> command.mTextureTransform.mT[1]
                    >> command.mTextureTransform.mT[2]
                    >> command.mTextureTransform.mT[3];
                if (valid == 0)
                {
                    command.mTextureTransform = {};
                }
            }
        }

        if (command.mCount > 0)
        {
            gSmokeCapturedCommands.push_back(command);
        }
    }

    std::cout
        << "Loaded "
        << gSmokeCapturedCommands.size()
        << " captured Vulkan world command(s) from "
        << path
        << "."
        << std::endl;
    return !gSmokeCapturedCommands.empty();
}

LLRenderWorldShaderClass get_capture_shader_class(LLWorldRenderMaterialClass material_class)
{
    switch (material_class)
    {
    case LLWorldRenderMaterialClass::Terrain:
        return LLRenderWorldShaderClass::Terrain;
    case LLWorldRenderMaterialClass::Sky:
        return LLRenderWorldShaderClass::Sky;
    case LLWorldRenderMaterialClass::Water:
        return LLRenderWorldShaderClass::Water;
    case LLWorldRenderMaterialClass::WaterExclusionMask:
    case LLWorldRenderMaterialClass::AtmosphericHaze:
    case LLWorldRenderMaterialClass::WaterHaze:
        return LLRenderWorldShaderClass::Haze;
    case LLWorldRenderMaterialClass::Alpha:
        return LLRenderWorldShaderClass::Alpha;
    case LLWorldRenderMaterialClass::Glow:
        return LLRenderWorldShaderClass::Glow;
    case LLWorldRenderMaterialClass::AlphaMask:
    case LLWorldRenderMaterialClass::Grass:
    case LLWorldRenderMaterialClass::Tree:
    case LLWorldRenderMaterialClass::GLTFPBRAlphaMask:
        return LLRenderWorldShaderClass::AlphaMask;
    case LLWorldRenderMaterialClass::Fullbright:
    case LLWorldRenderMaterialClass::FullbrightAlphaMask:
    case LLWorldRenderMaterialClass::FullbrightShiny:
        return LLRenderWorldShaderClass::Fullbright;
    case LLWorldRenderMaterialClass::LegacyMaterial:
    case LLWorldRenderMaterialClass::Bump:
    case LLWorldRenderMaterialClass::PostBump:
        return LLRenderWorldShaderClass::Material;
    case LLWorldRenderMaterialClass::GLTFPBR:
        return LLRenderWorldShaderClass::PBR;
    case LLWorldRenderMaterialClass::Avatar:
    case LLWorldRenderMaterialClass::AvatarImpostor:
        return LLRenderWorldShaderClass::Avatar;
    default:
        return LLRenderWorldShaderClass::Textured;
    }
}

U32 get_capture_visual_sort_key(const SmokeCapturedCommand& command)
{
    return (static_cast<U32>(command.mPassClass) << 24U) |
        (static_cast<U32>(command.mMaterialClass) << 16U) |
        (static_cast<U32>(command.mBlendMode) << 12U) |
        (static_cast<U32>(command.mDepthMode) << 8U) |
        (static_cast<U32>(command.mCullMode) << 4U);
}

U32 get_capture_material_flags(const SmokeCapturedCommand& command)
{
    U32 flags = 0;
    switch (command.mMaterialClass)
    {
    case LLWorldRenderMaterialClass::Fullbright:
    case LLWorldRenderMaterialClass::FullbrightAlphaMask:
    case LLWorldRenderMaterialClass::FullbrightShiny:
        flags |= LLRenderWorldMaterialParameters::Fullbright;
        break;
    default:
        break;
    }
    switch (command.mMaterialClass)
    {
    case LLWorldRenderMaterialClass::GLTFPBR:
    case LLWorldRenderMaterialClass::GLTFPBRAlphaMask:
        flags |= LLRenderWorldMaterialParameters::GLTFPBR |
            LLRenderWorldMaterialParameters::HasNormalMap |
            LLRenderWorldMaterialParameters::HasORMMap;
        break;
    case LLWorldRenderMaterialClass::LegacyMaterial:
    case LLWorldRenderMaterialClass::Bump:
    case LLWorldRenderMaterialClass::PostBump:
        flags |= LLRenderWorldMaterialParameters::HasSpecularMap;
        break;
    default:
        break;
    }
    if (command.mMaterialClass == LLWorldRenderMaterialClass::AlphaMask ||
        command.mMaterialClass == LLWorldRenderMaterialClass::FullbrightAlphaMask ||
        command.mMaterialClass == LLWorldRenderMaterialClass::GLTFPBRAlphaMask)
    {
        flags |= LLRenderWorldMaterialParameters::AlphaMask;
    }
    if (command.mBlendMode == LLWorldRenderBlendMode::Alpha)
    {
        flags |= LLRenderWorldMaterialParameters::AlphaBlend;
    }
    if (command.mHasGlow || command.mMaterialClass == LLWorldRenderMaterialClass::Glow)
    {
        flags |= LLRenderWorldMaterialParameters::Glow;
    }
    if (command.mPassClass == LLWorldRenderPassClass::PostDeferred)
    {
        flags |= LLRenderWorldMaterialParameters::PostDeferred;
    }
    if (command.mMaterialClass == LLWorldRenderMaterialClass::Water)
    {
        flags |= LLRenderWorldMaterialParameters::Water |
            LLRenderWorldMaterialParameters::SceneDepth |
            LLRenderWorldMaterialParameters::SceneColor;
    }
    if (command.mMaterialClass == LLWorldRenderMaterialClass::AtmosphericHaze ||
        command.mMaterialClass == LLWorldRenderMaterialClass::WaterHaze)
    {
        flags |= LLRenderWorldMaterialParameters::AtmosphericHaze |
            LLRenderWorldMaterialParameters::SceneDepth |
            LLRenderWorldMaterialParameters::SceneColor;
    }
    return flags;
}

LLColor4 make_capture_replay_color(
    const SmokeCapturedCommand& command,
    size_t command_index)
{
    LLColor4 color;
    switch (command.mMaterialClass)
    {
    case LLWorldRenderMaterialClass::Sky:
        color = LLColor4(0.18f, 0.42f, 0.95f, 1.f);
        break;
    case LLWorldRenderMaterialClass::Terrain:
    case LLWorldRenderMaterialClass::Grass:
    case LLWorldRenderMaterialClass::Tree:
        color = LLColor4(0.28f, 0.64f, 0.28f, 1.f);
        break;
    case LLWorldRenderMaterialClass::Water:
    case LLWorldRenderMaterialClass::WaterExclusionMask:
    case LLWorldRenderMaterialClass::WaterHaze:
        color = LLColor4(0.18f, 0.62f, 0.92f, 0.82f);
        break;
    case LLWorldRenderMaterialClass::AtmosphericHaze:
        color = LLColor4(0.72f, 0.78f, 0.88f, 0.74f);
        break;
    case LLWorldRenderMaterialClass::Alpha:
        color = LLColor4(0.90f, 0.32f, 0.70f, 0.70f);
        break;
    case LLWorldRenderMaterialClass::Glow:
        color = LLColor4(1.00f, 0.64f, 0.12f, 0.86f);
        break;
    case LLWorldRenderMaterialClass::AlphaMask:
    case LLWorldRenderMaterialClass::FullbrightAlphaMask:
    case LLWorldRenderMaterialClass::GLTFPBRAlphaMask:
        color = LLColor4(0.95f, 0.84f, 0.22f, 1.f);
        break;
    case LLWorldRenderMaterialClass::Fullbright:
    case LLWorldRenderMaterialClass::FullbrightShiny:
        color = LLColor4(0.95f, 0.34f, 0.28f, 1.f);
        break;
    case LLWorldRenderMaterialClass::LegacyMaterial:
    case LLWorldRenderMaterialClass::Bump:
    case LLWorldRenderMaterialClass::PostBump:
        color = LLColor4(0.70f, 0.48f, 0.95f, 1.f);
        break;
    case LLWorldRenderMaterialClass::GLTFPBR:
        color = LLColor4(0.92f, 0.68f, 0.40f, 1.f);
        break;
    case LLWorldRenderMaterialClass::Avatar:
    case LLWorldRenderMaterialClass::AvatarImpostor:
        color = LLColor4(0.88f, 0.54f, 0.44f, 1.f);
        break;
    default:
        color = LLColor4(0.34f, 0.72f, 0.96f, 1.f);
        break;
    }

    const F32 variation =
        static_cast<F32>(command_index % 7U) * 0.035f;
    color.mV[VRED] = llmin(1.f, color.mV[VRED] + variation);
    color.mV[VGREEN] = llmin(1.f, color.mV[VGREEN] + variation * 0.5f);
    color.mV[VBLUE] = llmin(1.f, color.mV[VBLUE] + variation * 0.25f);
    if (command.mPassClass == LLWorldRenderPassClass::PostDeferred)
    {
        color.mV[VALPHA] = llmin(color.mV[VALPHA], 0.82f);
    }
    return color;
}

LLRenderWorldMaterialParameters make_capture_world_material(
    const SmokeCapturedCommand& command,
    size_t command_index)
{
    const LLColor4 replay_color =
        make_capture_replay_color(command, command_index);
    LLRenderWorldMaterialParameters parameters =
        make_world_pipeline_material(
            replay_color.mV[VRED],
            replay_color.mV[VGREEN],
            replay_color.mV[VBLUE],
            replay_color.mV[VALPHA],
            get_capture_material_flags(command));
    parameters.mEmissiveColorRed = command.mEmissiveColor.mV[VRED];
    parameters.mEmissiveColorGreen = command.mEmissiveColor.mV[VGREEN];
    parameters.mEmissiveColorBlue = command.mEmissiveColor.mV[VBLUE];
    parameters.mSpecularColorRed = command.mSpecColor.mV[VX];
    parameters.mSpecularColorGreen = command.mSpecColor.mV[VY];
    parameters.mSpecularColorBlue = command.mSpecColor.mV[VZ];
    parameters.mEnvIntensity = command.mEnvIntensity;
    parameters.mMetallicFactor = command.mMetallicFactor;
    parameters.mRoughnessFactor = command.mRoughnessFactor;
    parameters.mDiffuseAlphaMode = static_cast<F32>(command.mDiffuseAlphaMode);
    parameters.mGLTFAlphaMode = static_cast<F32>(command.mGLTFAlphaMode);
    parameters.mBump = static_cast<F32>(command.mBump);
    parameters.mShiny = static_cast<F32>(command.mShiny);
    parameters.mBaseTextureScaleS = command.mBaseColorTextureTransform.mScaleS;
    parameters.mBaseTextureScaleT = command.mBaseColorTextureTransform.mScaleT;
    parameters.mBaseTextureRotation = command.mBaseColorTextureTransform.mRotation;
    parameters.mBaseTextureOffsetS = command.mBaseColorTextureTransform.mOffsetS;
    parameters.mBaseTextureOffsetT = command.mBaseColorTextureTransform.mOffsetT;
    parameters.mNormalTextureScaleS = command.mNormalTextureTransform.mScaleS;
    parameters.mNormalTextureScaleT = command.mNormalTextureTransform.mScaleT;
    parameters.mNormalTextureRotation = command.mNormalTextureTransform.mRotation;
    parameters.mNormalTextureOffsetS = command.mNormalTextureTransform.mOffsetS;
    parameters.mNormalTextureOffsetT = command.mNormalTextureTransform.mOffsetT;
    parameters.mORMTextureScaleS = command.mORMTextureTransform.mScaleS;
    parameters.mORMTextureScaleT = command.mORMTextureTransform.mScaleT;
    parameters.mORMTextureRotation = command.mORMTextureTransform.mRotation;
    parameters.mORMTextureOffsetS = command.mORMTextureTransform.mOffsetS;
    parameters.mORMTextureOffsetT = command.mORMTextureTransform.mOffsetT;
    parameters.mEmissiveTextureScaleS = command.mEmissiveTextureTransform.mScaleS;
    parameters.mEmissiveTextureScaleT = command.mEmissiveTextureTransform.mScaleT;
    parameters.mEmissiveTextureRotation = command.mEmissiveTextureTransform.mRotation;
    parameters.mEmissiveTextureOffsetS = command.mEmissiveTextureTransform.mOffsetS;
    parameters.mEmissiveTextureOffsetT = command.mEmissiveTextureTransform.mOffsetT;
    return parameters;
}

void apply_capture_command_state(
    LLRenderBackend& backend,
    const SmokeCapturedCommand& command)
{
    backend.setColorMask(
        {
            command.mWriteColor,
            command.mWriteColor,
            command.mWriteColor,
            command.mWriteAlpha
        });
    backend.setCapability(
        LLRenderCapability::Blend,
        command.mBlendMode != LLWorldRenderBlendMode::None);
    if (command.mBlendMode == LLWorldRenderBlendMode::Add)
    {
        backend.setBlendState(
            {
                LLRenderBlendFactor::SourceAlpha,
                LLRenderBlendFactor::One,
                LLRenderBlendFactor::One,
                LLRenderBlendFactor::One,
            });
    }
    else if (command.mBlendMode == LLWorldRenderBlendMode::Alpha)
    {
        backend.setBlendState(
            {
                LLRenderBlendFactor::SourceAlpha,
                LLRenderBlendFactor::OneMinusSourceAlpha,
                LLRenderBlendFactor::One,
                LLRenderBlendFactor::OneMinusSourceAlpha,
            });
    }

    const bool depth_enabled =
        command.mDepthMode != LLWorldRenderDepthMode::Disabled;
    backend.setCapability(LLRenderCapability::DepthTest, depth_enabled);
    backend.setDepthFunction(LLRenderDepthFunction::LessEqual);
    backend.setDepthWriteEnabled(command.mDepthMode == LLWorldRenderDepthMode::ReadWrite);
    backend.setCapability(
        LLRenderCapability::CullFace,
        command.mCullMode == LLWorldRenderCullMode::Back && !command.mDoubleSided);
    if (command.mCullMode == LLWorldRenderCullMode::Back && !command.mDoubleSided)
    {
        backend.setCullFace(LLRenderCullFace::Back);
    }
    backend.setAlphaMaskCutoff(command.mAlphaMaskCutoff);
}

bool draw_capture_commands_for_pass(
    LLRenderBackend& backend,
    SmokeDeferredTextures& textures,
    const SmokeQuad& quad,
    LLWorldRenderPassClass pass_class,
    U32 width,
    U32 height)
{
    std::vector<const SmokeCapturedCommand*> commands;
    for (const SmokeCapturedCommand& command : gSmokeCapturedCommands)
    {
        if (command.mPassClass == pass_class)
        {
            commands.push_back(&command);
        }
    }

    if (commands.empty())
    {
        std::cout
            << "Mare Vulkan capture replay has no "
            << (pass_class == LLWorldRenderPassClass::Deferred ? "deferred" : "post-deferred")
            << " command(s)."
            << std::endl;
        return false;
    }

    static bool logged_capture_visual_layout = false;
    if (!logged_capture_visual_layout)
    {
        std::cout
            << "Mare Vulkan capture replay visual layout: grouped by material/state. "
            << "This is command-shape replay, not captured world geometry."
            << std::endl;
        logged_capture_visual_layout = true;
    }

    std::stable_sort(
        commands.begin(),
        commands.end(),
        [](const SmokeCapturedCommand* left, const SmokeCapturedCommand* right)
        {
            const U32 left_key = get_capture_visual_sort_key(*left);
            const U32 right_key = get_capture_visual_sort_key(*right);
            if (left_key != right_key)
            {
                return left_key < right_key;
            }
            return left->mSourcePass < right->mSourcePass;
        });

    bind_world_pipeline_smoke_textures(backend, textures);
    bind_world_smoke_quad(backend, quad);
    backend.setWorldTextureTransform({});
    backend.setWorldTerrainParameters(make_world_pipeline_terrain_parameters());
    backend.setWorldSkinningMatrixPalette(0, nullptr);
    backend.setWorldDrawEnabled(true);

    const S32 columns = llmax(
        1,
        llmin(
            24,
            static_cast<S32>(
                std::ceil(
                    std::sqrt(
                        static_cast<F32>(commands.size()))))));
    const S32 rows =
        static_cast<S32>((commands.size() + static_cast<size_t>(columns) - 1) /
            static_cast<size_t>(columns));
    const S32 cell_width = llmax(1, static_cast<S32>(width) / columns);
    const S32 cell_height = llmax(1, static_cast<S32>(height) / llmax(1, rows));

    for (size_t i = 0; i < commands.size(); ++i)
    {
        const SmokeCapturedCommand& command = *commands[i];
        const S32 column = static_cast<S32>(i % static_cast<size_t>(columns));
        const S32 row = static_cast<S32>(i / static_cast<size_t>(columns));
        const S32 x = column * cell_width;
        const S32 y = row * cell_height;
        const S32 w = column == columns - 1 ?
            static_cast<S32>(width) - x :
            cell_width;
        const S32 h = row == rows - 1 ?
            static_cast<S32>(height) - y :
            cell_height;

        backend.setScissor(x, y, llmax(1, w), llmax(1, h));
        apply_capture_command_state(backend, command);
        backend.setWorldShaderClass(get_capture_shader_class(command.mMaterialClass));
        backend.setWorldTextureTransform(command.mTextureTransform);
        backend.setWorldMaterialParameters(make_capture_world_material(command, i));
        backend.drawArrays(LLRenderPrimitiveType::Triangles, 0, 6);
    }

    backend.setWorldDrawEnabled(false);
    backend.setWorldShaderClass(LLRenderWorldShaderClass::Textured);
    backend.setWorldMaterialParameters({});
    backend.setWorldTerrainParameters({});
    backend.setWorldTextureTransform({});
    backend.setWorldSkinningMatrixPalette(0, nullptr);
    backend.setAlphaMaskCutoff(-1.f);
    backend.setColorMask({ true, true, true, true });
    backend.setCapability(LLRenderCapability::Blend, false);
    backend.setScissor(0, 0, static_cast<S32>(width), static_cast<S32>(height));
    return true;
}

bool render_world_pipelines_frame(
    LLRenderBackend& backend,
    SmokeDeferredTextures& textures,
    const SmokeQuad& quad,
    SmokeScene scene,
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

    const std::array<SmokeWorldPipelineEntry, 15> entries =
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
        { LLRenderWorldShaderClass::Copy, "Copy", 0.58f, 0.68f, 0.86f, 0 },
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

    const U32 repeat_count =
        scene == SmokeScene::PostOverlaysStress ? 4U : 1U;
    const U32 tile_count =
        static_cast<U32>(entries.size()) * repeat_count;
    const S32 columns =
        scene == SmokeScene::PostOverlaysStress ? 6 : 4;
    const S32 rows =
        static_cast<S32>((tile_count + static_cast<U32>(columns) - 1U) / static_cast<U32>(columns));
    const S32 cell_width = llmax(1, static_cast<S32>(width) / columns);
    const S32 cell_height = llmax(1, static_cast<S32>(height) / rows);
    static bool logged_world_pipeline_entries = false;
    const bool log_world_pipeline_entries = !logged_world_pipeline_entries;

    for (U32 i = 0; i < tile_count; ++i)
    {
        const SmokeWorldPipelineEntry& entry = entries[i % entries.size()];
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
    LLRenderTarget mDeferredLight;
    LLRenderTarget mScreen;
    LLRenderTarget mPostPing;
    U32 mWidth = 0;
    U32 mHeight = 0;
    U32 mColorAttachmentCount = 0;
    bool mStagedPostTargets = false;
};

struct SmokeCopyChainGraph
{
    LLRenderTarget mSource;
    LLRenderTarget mCopyA;
    LLRenderTarget mCopyB;
    LLRenderTarget mCopyC;
    U32 mWidth = 0;
    U32 mHeight = 0;
    U32 mSourceColorAttachmentCount = 0;
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
    graph.mPostPing.release();
    graph.mDeferredLight.release();
    graph.mScreen.release();
    graph.mDeferredScreen.release();
    graph.mWidth = 0;
    graph.mHeight = 0;
    graph.mColorAttachmentCount = 0;
    graph.mStagedPostTargets = false;
}

void release_smoke_copy_chain_graph(SmokeCopyChainGraph& graph)
{
    graph.mCopyC.release();
    graph.mCopyB.release();
    graph.mCopyA.release();
    graph.mSource.release();
    graph.mWidth = 0;
    graph.mHeight = 0;
    graph.mSourceColorAttachmentCount = 0;
}

bool ensure_smoke_viewer_render_target_graph(
    SmokeViewerRenderTargetGraph& graph,
    U32 width,
    U32 height,
    U32 color_attachment_count,
    bool staged_post_targets = false)
{
    color_attachment_count = llclamp(color_attachment_count, 3U, 4U);
    if (graph.mDeferredScreen.isComplete() &&
        graph.mDeferredScreen.getWidth() == width &&
        graph.mDeferredScreen.getHeight() == height &&
        graph.mDeferredScreen.getNumTextures() == color_attachment_count &&
        graph.mColorAttachmentCount == color_attachment_count &&
        graph.mStagedPostTargets == staged_post_targets &&
        (!staged_post_targets ||
            (graph.mDeferredLight.isComplete() &&
             graph.mScreen.isComplete() &&
             graph.mPostPing.isComplete() &&
             graph.mDeferredLight.getWidth() == width &&
             graph.mDeferredLight.getHeight() == height &&
             graph.mScreen.getWidth() == width &&
             graph.mScreen.getHeight() == height &&
             graph.mPostPing.getWidth() == width &&
             graph.mPostPing.getHeight() == height)))
    {
        return true;
    }

    release_smoke_viewer_render_target_graph(graph);
    graph.mWidth = width;
    graph.mHeight = height;
    graph.mColorAttachmentCount = color_attachment_count;
    graph.mStagedPostTargets = staged_post_targets;

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

    if (staged_post_targets)
    {
        if (!graph.mDeferredLight.allocate(width, height, LLRenderTextureFormat::RGBA16F) ||
            !graph.mScreen.allocate(width, height, LLRenderTextureFormat::RGBA16F) ||
            !graph.mPostPing.allocate(width, height, LLRenderTextureFormat::RGBA))
        {
            release_smoke_viewer_render_target_graph(graph);
            return false;
        }

        graph.mDeferredScreen.shareDepthBuffer(graph.mScreen);
    }

    return graph.mDeferredScreen.isComplete() &&
        (!staged_post_targets ||
            (graph.mDeferredLight.isComplete() &&
             graph.mScreen.isComplete() &&
             graph.mPostPing.isComplete()));
}

bool ensure_smoke_copy_chain_graph(
    SmokeCopyChainGraph& graph,
    U32 width,
    U32 height,
    bool multi_attachment_source)
{
    const U32 source_color_attachment_count =
        multi_attachment_source ? 3U : 1U;
    if (graph.mSource.isComplete() &&
        graph.mCopyA.isComplete() &&
        graph.mCopyB.isComplete() &&
        graph.mCopyC.isComplete() &&
        graph.mWidth == width &&
        graph.mHeight == height &&
        graph.mSourceColorAttachmentCount == source_color_attachment_count &&
        graph.mSource.getNumTextures() == source_color_attachment_count &&
        graph.mSource.getWidth() == width &&
        graph.mSource.getHeight() == height &&
        graph.mCopyA.getWidth() == width &&
        graph.mCopyA.getHeight() == height &&
        graph.mCopyB.getWidth() == width &&
        graph.mCopyB.getHeight() == height &&
        graph.mCopyC.getWidth() == width &&
        graph.mCopyC.getHeight() == height)
    {
        return true;
    }

    release_smoke_copy_chain_graph(graph);
    graph.mWidth = width;
    graph.mHeight = height;
    graph.mSourceColorAttachmentCount = source_color_attachment_count;

    if (!graph.mSource.allocate(
            width,
            height,
            multi_attachment_source ? LLRenderTextureFormat::RGBA : LLRenderTextureFormat::RGBA16F,
            true) ||
        (multi_attachment_source &&
            (!graph.mSource.addColorAttachment(LLRenderTextureFormat::RGBA) ||
             !graph.mSource.addColorAttachment(LLRenderTextureFormat::RGBA16))) ||
        !graph.mCopyA.allocate(width, height, LLRenderTextureFormat::RGBA16F) ||
        !graph.mCopyB.allocate(width, height, LLRenderTextureFormat::RGBA16F) ||
        !graph.mCopyC.allocate(width, height, LLRenderTextureFormat::RGBA16F))
    {
        release_smoke_copy_chain_graph(graph);
        return false;
    }

    return graph.mSource.isComplete() &&
        graph.mCopyA.isComplete() &&
        graph.mCopyB.isComplete() &&
        graph.mCopyC.isComplete();
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

LLRenderWorldMaterialParameters make_deferred_graph_final_parameters(
    U32 deferred_attachment_count = 0)
{
    LLVulkanFinalCompositeSettings settings;
    settings.mNoPost = true;
    settings.mDeferredAttachmentCount = deferred_attachment_count;
    return make_vulkan_final_composite_material_parameters(settings);
}

void bind_two_prim_material_textures(
    LLRenderBackend& backend,
    const SmokeDeferredTextures& textures)
{
    const LLRenderTextureHandle bindings[] =
    {
        textures.mWhite,
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

void set_two_prim_world_matrix(
    F32 center_x,
    F32 center_y,
    F32 depth,
    F32 half_width,
    F32 half_height)
{
    const glm::mat4 projection(1.f);
    glm::mat4 modelview(1.f);
    modelview = glm::translate(modelview, glm::vec3(center_x, center_y, depth));
    modelview = glm::scale(modelview, glm::vec3(half_width, half_height, 1.f));

    set_current_projection(projection);
    set_current_modelview(modelview);
    gGL.matrixMode(LLRender::MM_PROJECTION);
    gGL.loadMatrix(glm::value_ptr(projection));
    gGL.matrixMode(LLRender::MM_MODELVIEW);
    gGL.loadMatrix(glm::value_ptr(modelview));
}

void draw_two_prim_deferred_gbuffer_scene(
    LLRenderBackend& backend,
    const SmokeDeferredTextures& textures,
    const SmokeQuad& quad,
    U32 width,
    U32 height)
{
    static bool logged_scene = false;
    if (!logged_scene)
    {
        std::cout
            << "Mare Vulkan smoke two-prims scene: opaque deferred prim writes "
            << "G-buffer/depth; translucent post-deferred prim overlaps it."
            << std::endl;
        logged_scene = true;
    }

    SmokeMatrixScope matrix_scope;
    bind_two_prim_material_textures(backend, textures);
    bind_world_smoke_quad(backend, quad);
    backend.setWorldDrawEnabled(true);
    backend.setWorldShaderClass(LLRenderWorldShaderClass::Textured);
    backend.setWorldTextureTransform({});
    backend.setWorldTerrainParameters({});
    backend.setWorldSkinningMatrixPalette(0, nullptr);
    backend.setCapability(LLRenderCapability::Blend, false);
    backend.setCapability(LLRenderCapability::DepthTest, true);
    backend.setDepthFunction(LLRenderDepthFunction::LessEqual);
    backend.setDepthWriteEnabled(true);
    backend.setCapability(LLRenderCapability::CullFace, false);
    backend.setColorMask({ true, true, true, true });
    backend.setScissor(0, 0, static_cast<S32>(width), static_cast<S32>(height));
    backend.setWorldMaterialParameters(
        make_world_pipeline_material(
            0.18f,
            0.68f,
            0.36f,
            1.f,
            0));
    set_two_prim_world_matrix(-0.10f, 0.02f, 0.42f, 0.48f, 0.46f);
    backend.drawArrays(LLRenderPrimitiveType::Triangles, 0, 6);

    backend.setWorldDrawEnabled(false);
    backend.setWorldShaderClass(LLRenderWorldShaderClass::Textured);
    backend.setWorldMaterialParameters({});
    backend.setCapability(LLRenderCapability::Blend, false);
    backend.setScissor(0, 0, static_cast<S32>(width), static_cast<S32>(height));
}

void draw_two_prim_post_alpha_scene(
    LLRenderBackend& backend,
    const SmokeDeferredTextures& textures,
    const SmokeQuad& quad,
    LLRenderTarget* deferred_screen,
    U32 width,
    U32 height)
{
    SmokeMatrixScope matrix_scope;
    bind_two_prim_material_textures(backend, textures);

    bool scene_depth_bound = false;
    if (deferred_screen && deferred_screen->getDepthHandle())
    {
        scene_depth_bound =
            gGL.getTexUnit(8)->bind(deferred_screen, true);
        if (scene_depth_bound)
        {
            gGL.getTexUnit(8)->setTextureFilteringOption(LLTexUnit::TFO_POINT);
        }
    }

    bind_world_smoke_quad(backend, quad);
    backend.setWorldDrawEnabled(true);
    backend.setWorldShaderClass(LLRenderWorldShaderClass::Alpha);
    backend.setWorldTextureTransform({});
    backend.setWorldTerrainParameters({});
    backend.setWorldSkinningMatrixPalette(0, nullptr);
    backend.setCapability(LLRenderCapability::Blend, true);
    backend.setBlendState(
        {
            LLRenderBlendFactor::SourceAlpha,
            LLRenderBlendFactor::OneMinusSourceAlpha,
            LLRenderBlendFactor::Zero,
            LLRenderBlendFactor::OneMinusSourceAlpha,
        });
    backend.setCapability(LLRenderCapability::DepthTest, true);
    backend.setDepthFunction(LLRenderDepthFunction::LessEqual);
    backend.setDepthWriteEnabled(false);
    backend.setCapability(LLRenderCapability::CullFace, false);
    backend.setColorMask({ true, true, true, true });
    backend.setScissor(0, 0, static_cast<S32>(width), static_cast<S32>(height));

    U32 alpha_flags =
        LLRenderWorldMaterialParameters::AlphaBlend |
        LLRenderWorldMaterialParameters::PostDeferred;
    if (scene_depth_bound)
    {
        alpha_flags |= LLRenderWorldMaterialParameters::SceneDepth;
    }
    LLRenderWorldMaterialParameters alpha_parameters =
        make_world_pipeline_material(
            0.96f,
            0.18f,
            0.76f,
            0.58f,
            alpha_flags);
    alpha_parameters.mDiffuseAlphaMode = 1.f;
    backend.setWorldMaterialParameters(alpha_parameters);
    set_two_prim_world_matrix(0.10f, -0.02f, 0.64f, 0.56f, 0.52f);
    backend.drawArrays(LLRenderPrimitiveType::Triangles, 0, 6);

    backend.setWorldDrawEnabled(false);
    backend.setWorldShaderClass(LLRenderWorldShaderClass::Textured);
    backend.setWorldMaterialParameters({});
    backend.setWorldTerrainParameters({});
    backend.setWorldTextureTransform({});
    backend.setWorldSkinningMatrixPalette(0, nullptr);
    backend.setCapability(LLRenderCapability::Blend, false);
    backend.setCapability(LLRenderCapability::DepthTest, false);
    backend.setDepthWriteEnabled(false);
    backend.setScissor(0, 0, static_cast<S32>(width), static_cast<S32>(height));
    if (scene_depth_bound)
    {
        gGL.getTexUnit(8)->unbind(LLTexUnit::TT_TEXTURE);
    }
    backend.setActiveTextureUnit(0);
}

void draw_deferred_graph_gbuffer_tiles(
    LLRenderBackend& backend,
    const SmokeDeferredTextures& textures,
    const SmokeQuad& quad,
    SmokeScene scene,
    U32 width,
    U32 height)
{
    if (scene == SmokeScene::TwoPrims)
    {
        draw_two_prim_deferred_gbuffer_scene(
            backend,
            textures,
            quad,
            width,
            height);
        return;
    }

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

    const U32 repeat_count =
        scene == SmokeScene::PostOverlaysStress ? 4U : 1U;
    const U32 tile_count =
        static_cast<U32>(entries.size()) * repeat_count;
    const S32 columns =
        scene == SmokeScene::PostOverlaysStress ? 6 : 3;
    const S32 rows =
        static_cast<S32>((tile_count + static_cast<U32>(columns) - 1U) / static_cast<U32>(columns));
    const S32 cell_width = llmax(1, static_cast<S32>(width) / columns);
    const S32 cell_height = llmax(1, static_cast<S32>(height) / rows);
    static bool logged_entries = false;
    if (!logged_entries)
    {
        std::cout << "Mare Vulkan smoke deferred-graph G-buffer entries:";
    }
    for (U32 i = 0; i < tile_count; ++i)
    {
        const SmokeWorldPipelineEntry& entry = entries[i % entries.size()];
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

void draw_deferred_graph_post_overlay_tiles(
    LLRenderBackend& backend,
    SmokeDeferredTextures& textures,
    const SmokeQuad& quad,
    LLRenderTarget* deferred_screen,
    SmokeScene scene,
    U32 width,
    U32 height)
{
    if (scene == SmokeScene::TwoPrims)
    {
        draw_two_prim_post_alpha_scene(
            backend,
            textures,
            quad,
            deferred_screen,
            width,
            height);
        return;
    }

    constexpr U32 fullbright =
        LLRenderWorldMaterialParameters::Fullbright |
        LLRenderWorldMaterialParameters::PostDeferred;
    constexpr U32 alpha =
        LLRenderWorldMaterialParameters::AlphaBlend |
        LLRenderWorldMaterialParameters::PostDeferred;
    constexpr U32 glow =
        LLRenderWorldMaterialParameters::Glow |
        LLRenderWorldMaterialParameters::PostDeferred;
    constexpr U32 pbr_alpha =
        LLRenderWorldMaterialParameters::GLTFPBR |
        LLRenderWorldMaterialParameters::HasORMMap |
        LLRenderWorldMaterialParameters::HasNormalMap |
        LLRenderWorldMaterialParameters::AlphaBlend |
        LLRenderWorldMaterialParameters::PostDeferred;

    const std::array<SmokeWorldPipelineEntry, 4> entries =
    {{
        { LLRenderWorldShaderClass::Fullbright, "PostFullbright", 0.95f, 0.38f, 0.30f, fullbright },
        { LLRenderWorldShaderClass::Alpha, "PostAlpha", 0.30f, 0.82f, 0.92f, alpha, true },
        { LLRenderWorldShaderClass::PBR, "PostPBRAlpha", 0.90f, 0.72f, 0.42f, pbr_alpha, true },
        { LLRenderWorldShaderClass::Glow, "PostGlow", 1.00f, 0.65f, 0.18f, glow, false, true },
    }};

    bind_world_pipeline_smoke_textures(backend, textures);
    bind_world_smoke_quad(backend, quad);
    backend.setWorldTextureTransform({});
    backend.setWorldTerrainParameters(make_world_pipeline_terrain_parameters());
    backend.setWorldSkinningMatrixPalette(0, nullptr);
    backend.setWorldDrawEnabled(true);
    backend.setCapability(LLRenderCapability::DepthTest, false);
    backend.setDepthWriteEnabled(false);
    backend.setCapability(LLRenderCapability::CullFace, false);
    backend.setColorMask({ true, true, true, true });

    const U32 repeat_count =
        scene == SmokeScene::PostOverlaysStress ? 12U : 1U;
    const U32 tile_count =
        static_cast<U32>(entries.size()) * repeat_count;
    const S32 columns =
        scene == SmokeScene::PostOverlaysStress ? 8 : 2;
    const S32 rows =
        static_cast<S32>((tile_count + static_cast<U32>(columns) - 1U) / static_cast<U32>(columns));
    const S32 cell_width = llmax(1, static_cast<S32>(width) / columns);
    const S32 cell_height = llmax(1, static_cast<S32>(height) / rows);
    static bool logged_entries = false;
    if (!logged_entries)
    {
        std::cout << "Mare Vulkan smoke post-deferred overlay entries:";
    }
    for (U32 i = 0; i < tile_count; ++i)
    {
        const SmokeWorldPipelineEntry& entry = entries[i % entries.size()];
        if (!logged_entries)
        {
            std::cout << " " << entry.mName;
        }

        const S32 column = static_cast<S32>(i % columns);
        const S32 row = static_cast<S32>(i / columns);
        const S32 x = column * cell_width + (cell_width / 4);
        const S32 y = row * cell_height + (cell_height / 4);
        const S32 w = llmax(1, cell_width / 2);
        const S32 h = llmax(1, cell_height / 2);

        backend.setScissor(x, y, w, h);
        backend.setWorldShaderClass(entry.mShaderClass);
        backend.setWorldMaterialParameters(
            make_world_pipeline_material(
                entry.mRed,
                entry.mGreen,
                entry.mBlue,
                entry.mAlphaBlend ? 0.62f : 1.f,
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
    if (!logged_entries)
    {
        std::cout << std::endl;
        logged_entries = true;
    }

    backend.setCapability(LLRenderCapability::Blend, false);
    backend.setWorldDrawEnabled(false);
    backend.setWorldShaderClass(LLRenderWorldShaderClass::Textured);
    backend.setWorldMaterialParameters({});
    backend.setWorldTerrainParameters({});
    backend.setWorldTextureTransform({});
    backend.setWorldSkinningMatrixPalette(0, nullptr);
    backend.setScissor(0, 0, static_cast<S32>(width), static_cast<S32>(height));
}

bool render_deferred_graph_frame(
    LLRenderBackend& backend,
    SmokeDeferredTextures& material_textures,
    SmokeDeferredGraph& graph,
    const SmokeQuad& quad,
    SmokeScene scene,
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
    if (scene == SmokeScene::ReplayCapture)
    {
        if (!draw_capture_commands_for_pass(
                backend,
                material_textures,
                quad,
                LLWorldRenderPassClass::Deferred,
                graph_width,
                graph_height))
        {
            return false;
        }
    }
    else
    {
        draw_deferred_graph_gbuffer_tiles(backend, material_textures, quad, scene, graph_width, graph_height);
    }

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
    SmokeScene scene,
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
    if (scene == SmokeScene::ReplayCapture)
    {
        if (!draw_capture_commands_for_pass(
                backend,
                material_textures,
                quad,
                LLWorldRenderPassClass::Deferred,
                graph_width,
                graph_height))
        {
            return false;
        }
    }
    else
    {
        draw_deferred_graph_gbuffer_tiles(backend, material_textures, quad, scene, graph_width, graph_height);
    }

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
    SmokeScene scene,
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
    if (scene == SmokeScene::ReplayCapture)
    {
        if (!draw_capture_commands_for_pass(
                backend,
                material_textures,
                quad,
                LLWorldRenderPassClass::Deferred,
                graph_width,
                graph_height))
        {
            return false;
        }
    }
    else
    {
        draw_deferred_graph_gbuffer_tiles(backend, material_textures, quad, scene, graph_width, graph_height);
    }
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
    SmokeScene scene,
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
    if (scene == SmokeScene::ReplayCapture)
    {
        if (!draw_capture_commands_for_pass(
                backend,
                material_textures,
                quad,
                LLWorldRenderPassClass::Deferred,
                graph_width,
                graph_height))
        {
            return false;
        }
    }
    else
    {
        draw_deferred_graph_gbuffer_tiles(backend, material_textures, quad, scene, graph_width, graph_height);
    }
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

void set_smoke_fullscreen_world_draw_state(
    LLRenderBackend& backend,
    LLRenderWorldShaderClass shader_class,
    const LLRenderWorldMaterialParameters& parameters,
    U32 width,
    U32 height)
{
    backend.setViewport(0, 0, static_cast<S32>(width), static_cast<S32>(height));
    backend.setScissor(0, 0, static_cast<S32>(width), static_cast<S32>(height));
    backend.setCapability(LLRenderCapability::DepthTest, false);
    backend.setDepthWriteEnabled(false);
    backend.setCapability(LLRenderCapability::Blend, false);
    backend.setCapability(LLRenderCapability::CullFace, false);
    backend.setColorMask({ true, true, true, true });
    backend.setWorldDrawEnabled(true);
    backend.setWorldShaderClass(shader_class);
    backend.setWorldTextureTransform({});
    backend.setWorldTerrainParameters({});
    backend.setWorldSkinningMatrixPalette(0, nullptr);
    backend.setWorldMaterialParameters(parameters);
}

void reset_smoke_world_draw_state(LLRenderBackend& backend)
{
    backend.setWorldDrawEnabled(false);
    backend.setWorldShaderClass(LLRenderWorldShaderClass::Textured);
    backend.setWorldMaterialParameters({});
    backend.setWorldTerrainParameters({});
    backend.setWorldTextureTransform({});
    backend.setWorldSkinningMatrixPalette(0, nullptr);
    backend.setCapability(LLRenderCapability::Blend, false);
}

void draw_smoke_target_copy_quad(
    LLRenderBackend& backend,
    LLRenderTarget& source,
    const SmokeQuad& quad,
    U32 width,
    U32 height)
{
    source.bindTexture(0, 0, LLTexUnit::TFO_BILINEAR);
    set_smoke_fullscreen_world_draw_state(
        backend,
        LLRenderWorldShaderClass::Copy,
        LLRenderWorldMaterialParameters(),
        width,
        height);
    bind_world_smoke_quad(backend, quad);
    backend.drawArrays(LLRenderPrimitiveType::Triangles, 0, 6);
    reset_smoke_world_draw_state(backend);
    gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);
}

void copy_smoke_target_to_target(
    LLRenderBackend& backend,
    LLRenderTarget& source,
    LLRenderTarget& destination,
    const SmokeQuad& quad,
    U32 width,
    U32 height)
{
    destination.bindTarget();
    backend.setClearColor(0.01f, 0.012f, 0.018f, 1.f);
    destination.clear(LL_RENDER_CLEAR_COLOR);
    draw_smoke_target_copy_quad(
        backend,
        source,
        quad,
        width,
        height);
    destination.flush();
}

void copy_smoke_target_to_swapchain(
    LLRenderBackend& backend,
    LLRenderTarget& source,
    const SmokeQuad& quad,
    U32 width,
    U32 height)
{
    backend.setViewport(0, 0, static_cast<S32>(width), static_cast<S32>(height));
    backend.setScissor(0, 0, static_cast<S32>(width), static_cast<S32>(height));
    backend.setClearColor(0.01f, 0.012f, 0.018f, 1.f);
    backend.clear(LL_RENDER_CLEAR_COLOR | LL_RENDER_CLEAR_DEPTH);
    draw_smoke_target_copy_quad(
        backend,
        source,
        quad,
        width,
        height);
    backend.setScissor(0, 0, static_cast<S32>(width), static_cast<S32>(height));
}

bool render_copy_chain_frame(
    LLRenderBackend& backend,
    SmokeDeferredTextures& material_textures,
    SmokeCopyChainGraph& graph,
    const SmokeQuad& quad,
    SmokeScene scene,
    U32 width,
    U32 height,
    bool multi_attachment_source)
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
    if (!ensure_smoke_copy_chain_graph(
            graph,
            graph_width,
            graph_height,
            multi_attachment_source))
    {
        return false;
    }

    graph.mSource.bindTarget();
    backend.setViewport(
        0,
        0,
        static_cast<S32>(graph_width),
        static_cast<S32>(graph_height));
    backend.setScissor(
        0,
        0,
        static_cast<S32>(graph_width),
        static_cast<S32>(graph_height));
    backend.setClearColor(0.015f, 0.018f, 0.028f, 1.f);
    graph.mSource.clear(LL_RENDER_CLEAR_COLOR | LL_RENDER_CLEAR_DEPTH);
    bind_deferred_graph_material_textures(backend, material_textures);
    if (scene == SmokeScene::ReplayCapture)
    {
        if (!draw_capture_commands_for_pass(
                backend,
                material_textures,
                quad,
                LLWorldRenderPassClass::Deferred,
                graph_width,
                graph_height))
        {
            return false;
        }
    }
    else
    {
        draw_deferred_graph_gbuffer_tiles(
            backend,
            material_textures,
            quad,
            scene,
            graph_width,
            graph_height);
    }
    graph.mSource.flush();

    copy_smoke_target_to_target(
        backend,
        graph.mSource,
        graph.mCopyA,
        quad,
        graph_width,
        graph_height);
    copy_smoke_target_to_target(
        backend,
        graph.mCopyA,
        graph.mCopyB,
        quad,
        graph_width,
        graph_height);
    copy_smoke_target_to_target(
        backend,
        graph.mCopyB,
        graph.mCopyC,
        quad,
        graph_width,
        graph_height);
    copy_smoke_target_to_swapchain(
        backend,
        graph.mCopyC,
        quad,
        width,
        height);
    return true;
}

void draw_smoke_deferred_screen_composite_quad(
    LLRenderBackend& backend,
    LLRenderTarget& deferred_screen,
    const SmokeQuad& quad,
    U32 width,
    U32 height)
{
    const U32 attachment_count =
        llmin(deferred_screen.getNumTextures(), 4U);
    for (U32 attachment = 0; attachment < attachment_count; ++attachment)
    {
        deferred_screen.bindTexture(
            attachment,
            static_cast<S32>(attachment),
            attachment == 0 ?
                LLTexUnit::TFO_BILINEAR :
                LLTexUnit::TFO_POINT);
    }

    bool depth_bound = false;
    if (deferred_screen.getDepthHandle())
    {
        depth_bound =
            gGL.getTexUnit(4)->bind(&deferred_screen, true);
    }

    LLRenderWorldMaterialParameters parameters =
        make_deferred_graph_composite_parameters();
    parameters.mRoughnessFactor = static_cast<F32>(attachment_count);
    parameters.mNormalTextureOffsetS = depth_bound ? 1.f : 0.f;
    set_smoke_fullscreen_world_draw_state(
        backend,
        LLRenderWorldShaderClass::DeferredComposite,
        parameters,
        width,
        height);
    bind_world_smoke_quad(backend, quad);
    backend.drawArrays(LLRenderPrimitiveType::Triangles, 0, 6);
    reset_smoke_world_draw_state(backend);

    for (U32 attachment = 0; attachment < attachment_count; ++attachment)
    {
        gGL.getTexUnit(static_cast<S32>(attachment))->unbind(LLTexUnit::TT_TEXTURE);
    }
    if (depth_bound)
    {
        gGL.getTexUnit(4)->unbind(LLTexUnit::TT_TEXTURE);
    }
}

void draw_smoke_final_composite_quad(
    LLRenderBackend& backend,
    LLRenderTarget& source,
    LLRenderTarget& deferred_screen,
    const SmokeQuad& quad,
    U32 width,
    U32 height)
{
    source.bindTexture(0, 0, LLTexUnit::TFO_BILINEAR);

    const U32 attachment_count =
        llmin(deferred_screen.getNumTextures(), 4U);
    for (U32 attachment = 0; attachment < attachment_count; ++attachment)
    {
        deferred_screen.bindTexture(
            attachment,
            static_cast<S32>(attachment + 1),
            LLTexUnit::TFO_BILINEAR);
    }

    bool depth_bound = false;
    if (deferred_screen.getDepthHandle())
    {
        depth_bound =
            gGL.getTexUnit(5)->bind(&deferred_screen, true);
    }

    LLRenderWorldMaterialParameters parameters =
        make_deferred_graph_final_parameters(attachment_count);
    set_smoke_fullscreen_world_draw_state(
        backend,
        LLRenderWorldShaderClass::FinalComposite,
        parameters,
        width,
        height);
    bind_world_smoke_quad(backend, quad);
    backend.drawArrays(LLRenderPrimitiveType::Triangles, 0, 6);
    reset_smoke_world_draw_state(backend);

    gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);
    for (U32 attachment = 0; attachment < attachment_count; ++attachment)
    {
        gGL.getTexUnit(static_cast<S32>(attachment + 1))->unbind(LLTexUnit::TT_TEXTURE);
    }
    if (depth_bound)
    {
        gGL.getTexUnit(5)->unbind(LLTexUnit::TT_TEXTURE);
    }
}

bool render_viewer_staged_post_targets_frame(
    LLRenderBackend& backend,
    SmokeDeferredTextures& material_textures,
    SmokeViewerRenderTargetGraph& graph,
    const SmokeQuad& quad,
    SmokeScene scene,
    U32 width,
    U32 height,
    SmokeViewerStagedStop stop_after,
    bool draw_post_overlays,
    bool use_final_composite)
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
            3,
            true))
    {
        return false;
    }

    graph.mDeferredScreen.bindTarget();
    backend.setClearColor(0.f, 0.f, 0.f, 0.f);
    graph.mDeferredScreen.clear(LL_RENDER_CLEAR_COLOR | LL_RENDER_CLEAR_DEPTH);
    bind_deferred_graph_material_textures(backend, material_textures);
    if (scene == SmokeScene::ReplayCapture)
    {
        if (!draw_capture_commands_for_pass(
                backend,
                material_textures,
                quad,
                LLWorldRenderPassClass::Deferred,
                graph_width,
                graph_height))
        {
            return false;
        }
    }
    else
    {
        draw_deferred_graph_gbuffer_tiles(backend, material_textures, quad, scene, graph_width, graph_height);
    }
    graph.mDeferredScreen.flush();

    graph.mDeferredLight.bindTarget();
    backend.setClearColor(0.01f, 0.012f, 0.018f, 1.f);
    graph.mDeferredLight.clear(LL_RENDER_CLEAR_COLOR);
    draw_smoke_deferred_screen_composite_quad(
        backend,
        graph.mDeferredScreen,
        quad,
        graph_width,
        graph_height);
    graph.mDeferredLight.flush();
    if (stop_after == SmokeViewerStagedStop::DeferredLight)
    {
        copy_smoke_target_to_swapchain(
            backend,
            graph.mDeferredLight,
            quad,
            width,
            height);
        return true;
    }

    graph.mScreen.bindTarget();
    backend.setClearColor(0.01f, 0.012f, 0.018f, 1.f);
    graph.mScreen.clear(LL_RENDER_CLEAR_COLOR);
    draw_smoke_target_copy_quad(
        backend,
        graph.mDeferredLight,
        quad,
        graph_width,
        graph_height);
    if (draw_post_overlays)
    {
        if (scene == SmokeScene::ReplayCapture)
        {
            draw_capture_commands_for_pass(
                backend,
                material_textures,
                quad,
                LLWorldRenderPassClass::PostDeferred,
                graph_width,
                graph_height);
        }
        else
        {
            draw_deferred_graph_post_overlay_tiles(
                backend,
                material_textures,
                quad,
                &graph.mDeferredScreen,
                scene,
                graph_width,
                graph_height);
        }
    }
    graph.mScreen.flush();
    if (stop_after == SmokeViewerStagedStop::Screen)
    {
        copy_smoke_target_to_swapchain(
            backend,
            graph.mScreen,
            quad,
            width,
            height);
        return true;
    }

    // Match the viewer staged path: deferredLight is sampled above, then reused
    // as the post-compose target. This catches missing sampled-read to
    // color-write synchronization without requiring login or a live region.
    copy_smoke_target_to_target(
        backend,
        graph.mScreen,
        graph.mDeferredLight,
        quad,
        graph_width,
        graph_height);
    if (stop_after == SmokeViewerStagedStop::ReusedDeferredLight)
    {
        copy_smoke_target_to_swapchain(
            backend,
            graph.mDeferredLight,
            quad,
            width,
            height);
        return true;
    }

    graph.mPostPing.bindTarget();
    backend.setClearColor(0.01f, 0.012f, 0.018f, 1.f);
    graph.mPostPing.clear(LL_RENDER_CLEAR_COLOR);
    if (use_final_composite)
    {
        draw_smoke_final_composite_quad(
            backend,
            graph.mDeferredLight,
            graph.mDeferredScreen,
            quad,
            graph_width,
            graph_height);
    }
    else
    {
        draw_smoke_target_copy_quad(
            backend,
            graph.mDeferredLight,
            quad,
            graph_width,
            graph_height);
    }
    graph.mPostPing.flush();

    copy_smoke_target_to_swapchain(
        backend,
        graph.mPostPing,
        quad,
        width,
        height);
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

bool render_smoke_scene_marker(
    LLRenderBackend& backend,
    SmokeScene scene,
    U32 width,
    U32 height)
{
    const S32 screen_width = static_cast<S32>(width);
    const S32 screen_height = static_cast<S32>(height);
    const S32 marker_width = llmax(96, screen_width / 12);
    const S32 marker_height = llmax(36, screen_height / 26);
    const S32 stripe_width = llmax(8, marker_width / 4);
    const S32 margin = llmax(8, screen_width / 120);

    LLColor4 primary;
    LLColor4 secondary;
    LLColor4 tertiary;
    switch (scene)
    {
    case SmokeScene::ReplayCapture:
        primary = LLColor4(0.10f, 0.86f, 0.52f, 0.92f);
        secondary = LLColor4(0.96f, 0.84f, 0.16f, 0.92f);
        tertiary = LLColor4(0.14f, 0.32f, 0.92f, 0.92f);
        break;
    case SmokeScene::PostOverlaysStress:
        primary = LLColor4(0.94f, 0.20f, 0.70f, 0.92f);
        secondary = LLColor4(1.00f, 0.52f, 0.12f, 0.92f);
        tertiary = LLColor4(0.34f, 0.88f, 0.96f, 0.92f);
        break;
    case SmokeScene::TwoPrims:
        primary = LLColor4(0.18f, 0.68f, 0.36f, 0.92f);
        secondary = LLColor4(0.96f, 0.18f, 0.76f, 0.92f);
        tertiary = LLColor4(0.08f, 0.10f, 0.16f, 0.92f);
        break;
    case SmokeScene::Basic:
    default:
        primary = LLColor4(0.12f, 0.58f, 0.96f, 0.92f);
        secondary = LLColor4(0.18f, 0.90f, 0.72f, 0.92f);
        tertiary = LLColor4(0.92f, 0.92f, 0.96f, 0.92f);
        break;
    }

    SmokeMatrixScope matrix_scope;
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
    gl_state_for_2d(screen_width, screen_height);

    gUIProgram.mAttributeMask =
        LLVertexBuffer::MAP_VERTEX |
        LLVertexBuffer::MAP_TEXCOORD0 |
        LLVertexBuffer::MAP_COLOR;
    gUIProgram.bind();

    const S32 x = margin;
    const S32 y = screen_height - marker_height - margin;
    draw_smoke_ui_color_rect(backend, primary, x, y, stripe_width, marker_height);
    draw_smoke_ui_color_rect(
        backend,
        secondary,
        x + stripe_width,
        y + marker_height / 4,
        marker_width - stripe_width * 2,
        llmax(4, marker_height / 2));
    draw_smoke_ui_color_rect(
        backend,
        tertiary,
        x + marker_width - stripe_width,
        y,
        stripe_width,
        marker_height);

    gGL.flush();
    gUIProgram.unbind();
    backend.setScissor(0, 0, screen_width, screen_height);
    backend.setCapability(LLRenderCapability::Blend, false);
    backend.setActiveTextureUnit(0);
    return true;
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
    if (options.mScene != SmokeScene::Basic &&
        !smoke_mode_uses_scene(options.mMode))
    {
        std::cerr
            << "--scene "
            << get_smoke_scene_name(options.mScene)
            << " is ignored by --mode "
            << get_smoke_mode_name(options.mMode)
            << ". Use a viewer/deferred mode such as viewer-staged-post-overlays, or omit --scene."
            << std::endl;
        return 1;
    }
    if (options.mScene == SmokeScene::ReplayCapture)
    {
        if (!smoke_mode_replays_capture(options.mMode))
        {
            std::cerr
                << "--scene replay-capture is not supported by --mode "
                << get_smoke_mode_name(options.mMode)
                << ". Use a viewer/deferred mode such as viewer-staged-post-overlays."
                << std::endl;
            return 1;
        }
        if (options.mCapturePath.empty())
        {
            std::cerr << "--scene replay-capture requires --capture <path>.\n";
            print_smoke_usage(argv[0] ? argv[0] : "mare-vulkan-smoke");
            return 1;
        }
        if (!load_smoke_capture_file(options.mCapturePath))
        {
            return 1;
        }
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
    if (options.mFrameDiff)
    {
        setenv("MARE_VULKAN_SMOKE_FRAME_DIFF", "1", 1);
    }
    else
    {
        unsetenv("MARE_VULKAN_SMOKE_FRAME_DIFF");
    }
    if (options.mFrameDiffSummaryOnly)
    {
        setenv("MARE_VULKAN_SMOKE_FRAME_DIFF_SUMMARY_ONLY", "1", 1);
    }
    else
    {
        unsetenv("MARE_VULKAN_SMOKE_FRAME_DIFF_SUMMARY_ONLY");
    }
    const std::string readback_frame_limit =
        std::to_string(options.mReadbackFrameLimit);
    setenv(
        "MARE_VULKAN_DEBUG_BUFFER_AVERAGE_FRAMES",
        readback_frame_limit.c_str(),
        1);
    if (!options.mScreenshotPPMPath.empty())
    {
        setenv("MARE_VULKAN_SMOKE_SCREENSHOT_PPM", options.mScreenshotPPMPath.c_str(), 1);
        const std::string screenshot_min_frame =
            std::to_string(options.mScreenshotMinFrame);
        setenv(
            "MARE_VULKAN_SMOKE_SCREENSHOT_MIN_FRAME",
            screenshot_min_frame.c_str(),
            1);
    }
    else
    {
        unsetenv("MARE_VULKAN_SMOKE_SCREENSHOT_PPM");
        unsetenv("MARE_VULKAN_SMOKE_SCREENSHOT_MIN_FRAME");
    }

    const SmokeMode smoke_mode = options.mMode;
    const SmokeScene smoke_scene = options.mScene;
    bool smoke_immediate_render_initialized = false;
    if (smoke_mode == SmokeMode::ViewerImmediateDirect ||
        smoke_mode == SmokeMode::ViewerStagedLightTarget ||
        smoke_mode == SmokeMode::ViewerStagedScreenTarget ||
        smoke_mode == SmokeMode::ViewerStagedScreenOverlays ||
        smoke_mode == SmokeMode::ViewerStagedReusedLightTarget ||
        smoke_mode == SmokeMode::ViewerStagedReusedLightOverlays ||
        smoke_mode == SmokeMode::ViewerStagedPostOverlays ||
        smoke_mode == SmokeMode::ViewerStagedPostCopy ||
        smoke_mode == SmokeMode::ViewerStagedPostTargets ||
        smoke_mode == SmokeMode::CopyChain ||
        smoke_mode == SmokeMode::CopyMRTChain ||
        options.mRenderUI ||
        options.mRenderSceneMarker)
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
    SmokeCopyChainGraph smoke_copy_chain_graph;
    SmokeUIOverlay smoke_ui_overlay;
    if ((smoke_mode == SmokeMode::OffscreenCopy ||
            smoke_mode == SmokeMode::DeferredComposite ||
            smoke_mode == SmokeMode::DeferredGraph ||
            smoke_mode == SmokeMode::ViewerDeferredDirect ||
            smoke_mode == SmokeMode::ViewerRenderTargetDirect ||
            smoke_mode == SmokeMode::ViewerImmediateDirect ||
            smoke_mode == SmokeMode::ViewerStagedLightTarget ||
            smoke_mode == SmokeMode::ViewerStagedScreenTarget ||
            smoke_mode == SmokeMode::ViewerStagedScreenOverlays ||
            smoke_mode == SmokeMode::ViewerStagedReusedLightTarget ||
            smoke_mode == SmokeMode::ViewerStagedReusedLightOverlays ||
            smoke_mode == SmokeMode::ViewerStagedPostOverlays ||
            smoke_mode == SmokeMode::ViewerStagedPostCopy ||
            smoke_mode == SmokeMode::ViewerStagedPostTargets ||
            smoke_mode == SmokeMode::CopyChain ||
            smoke_mode == SmokeMode::CopyMRTChain ||
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
        << ". Scene: "
        << get_smoke_scene_name(smoke_scene)
        << " ("
        << get_smoke_scene_description(smoke_scene)
        << ")"
        << ". Expected output: animated blue clear color"
        << get_smoke_mode_description(smoke_mode)
        << "Log interval: every "
        << log_interval
        << " frame(s). Synthetic UI: "
        << (options.mRenderUI ? "on" : "off")
        << ", viewer UI sequence: "
        << (options.mRenderViewerUISequence ? "on" : "off")
        << ", scene marker: "
        << (options.mRenderSceneMarker ? "on" : "off")
        << ", frame diff: "
        << (options.mFrameDiff ?
                (options.mFrameDiffSummaryOnly ? "summary-only" : "on") :
                "off")
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
    if (smoke_scene == SmokeScene::ReplayCapture)
    {
        std::cout
            << "Mare Vulkan replay-capture note: this mode does not display the captured region. "
            << "It replays captured world command metadata with synthetic quads/textures for renderer-state diagnostics."
            << std::endl;
        if (options.mRenderViewerUISequence)
        {
            std::cout
                << "Mare Vulkan replay-capture note: --ui-viewer-sequence draws synthetic UI over the command summary. "
                << "Omit --ui-viewer-sequence to inspect only the replay grid."
                << std::endl;
        }
    }
    else if (smoke_scene == SmokeScene::TwoPrims)
    {
        std::cout
            << "Mare Vulkan two-prims note: expected image is a green opaque deferred prim "
            << "with a magenta translucent post-deferred prim visible only where depth permits it."
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
                    smoke_scene,
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
                    smoke_scene,
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
                    smoke_scene,
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
                    smoke_scene,
                    width,
                    height))
            {
                std::cerr << "Failed to render Vulkan smoke viewer-immediate-direct frame.\n";
                break;
            }
        }
        else if (smoke_mode == SmokeMode::ViewerStagedPostTargets)
        {
            if (!render_viewer_staged_post_targets_frame(
                    backend,
                    smoke_deferred_textures,
                    smoke_viewer_render_target_graph,
                    smoke_quad,
                    smoke_scene,
                    width,
                    height,
                    SmokeViewerStagedStop::FinalPostTarget,
                    false,
                    true))
            {
                std::cerr << "Failed to render Vulkan smoke viewer-staged-post-targets frame.\n";
                break;
            }
        }
        else if (smoke_mode == SmokeMode::CopyChain ||
            smoke_mode == SmokeMode::CopyMRTChain)
        {
            if (!render_copy_chain_frame(
                    backend,
                    smoke_deferred_textures,
                    smoke_copy_chain_graph,
                    smoke_quad,
                    smoke_scene,
                    width,
                    height,
                    smoke_mode == SmokeMode::CopyMRTChain))
            {
                std::cerr << "Failed to render Vulkan smoke "
                    << get_smoke_mode_name(smoke_mode)
                    << " frame.\n";
                break;
            }
        }
        else if (smoke_mode == SmokeMode::ViewerStagedLightTarget ||
            smoke_mode == SmokeMode::ViewerStagedScreenTarget ||
            smoke_mode == SmokeMode::ViewerStagedScreenOverlays ||
            smoke_mode == SmokeMode::ViewerStagedReusedLightTarget ||
            smoke_mode == SmokeMode::ViewerStagedReusedLightOverlays ||
            smoke_mode == SmokeMode::ViewerStagedPostOverlays ||
            smoke_mode == SmokeMode::ViewerStagedPostCopy)
        {
            SmokeViewerStagedStop stop_after = SmokeViewerStagedStop::DeferredLight;
            bool draw_post_overlays = false;
            bool use_final_composite = true;
            if (smoke_mode == SmokeMode::ViewerStagedScreenTarget)
            {
                stop_after = SmokeViewerStagedStop::Screen;
            }
            else if (smoke_mode == SmokeMode::ViewerStagedScreenOverlays)
            {
                stop_after = SmokeViewerStagedStop::Screen;
                draw_post_overlays = true;
            }
            else if (smoke_mode == SmokeMode::ViewerStagedReusedLightTarget)
            {
                stop_after = SmokeViewerStagedStop::ReusedDeferredLight;
            }
            else if (smoke_mode == SmokeMode::ViewerStagedReusedLightOverlays)
            {
                stop_after = SmokeViewerStagedStop::ReusedDeferredLight;
                draw_post_overlays = true;
            }
            else if (smoke_mode == SmokeMode::ViewerStagedPostOverlays)
            {
                stop_after = SmokeViewerStagedStop::FinalPostTarget;
                draw_post_overlays = true;
            }
            else if (smoke_mode == SmokeMode::ViewerStagedPostCopy)
            {
                stop_after = SmokeViewerStagedStop::FinalPostTarget;
                draw_post_overlays = true;
                use_final_composite = false;
            }

            if (!render_viewer_staged_post_targets_frame(
                    backend,
                    smoke_deferred_textures,
                    smoke_viewer_render_target_graph,
                    smoke_quad,
                    smoke_scene,
                    width,
                    height,
                    stop_after,
                    draw_post_overlays,
                    use_final_composite))
            {
                std::cerr << "Failed to render Vulkan smoke "
                    << get_smoke_mode_name(smoke_mode)
                    << " frame.\n";
                break;
            }
        }
        else if (smoke_mode == SmokeMode::WorldPipelines)
        {
            if (!render_world_pipelines_frame(
                    backend,
                    smoke_deferred_textures,
                    smoke_quad,
                    smoke_scene,
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
        if (options.mRenderSceneMarker &&
            smoke_mode_uses_scene(smoke_mode) &&
            !render_smoke_scene_marker(
                backend,
                smoke_scene,
                width,
                height))
        {
            std::cerr << "Failed to render Vulkan smoke scene marker.\n";
            break;
        }
        backend.swapNativeBuffers(context.mContext);

        std::this_thread::sleep_for(std::chrono::milliseconds(16));
        ++frame;
    }

    flushVulkanSmokeFrameDiffSummaries();

    release_smoke_copy_chain_graph(smoke_copy_chain_graph);
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
