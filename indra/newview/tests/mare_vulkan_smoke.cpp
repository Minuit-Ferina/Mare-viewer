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
#include <cctype>
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
    ShaderProbe,
    ShaderSuite,
    Class1GBufferColorProbe,
    TerrainFinalProbe,
    FinalColorCompare,
    ViewerDeferredColorCompare,
    ViewerDeferredSoftenStateProbe,
    ViewerDeferredEmissiveProbe,
    ViewerDeferredReflectionProbe,
    ViewerDeferredRealReflectionProbe,
    ViewerDeferredHeroProbe,
    ViewerDeferredSSRProbe,
    ViewerDeferredLocalLightProbe,
    ViewerDeferredProjectorLightProbe,
    ViewerDeferredPointLightVolumeProbe,
    ViewerDeferredSpotLightVolumeProbe,
    ViewerDeferredLightMapBlurProbe,
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
    bool mForceVolumeLightOutput = false;
    bool mHelp = false;
    bool mListShaderCases = false;
    bool mListShaderParity = false;
    bool mListPipelineContracts = false;
    bool mShaderCaseExplicit = false;
    std::string mCapturePath;
    std::string mScreenshotPPMPath;
    std::string mReferencePPMPath;
    std::string mOpenGLReferencePPMPath;
    std::string mShaderCase = "textured";
    int mScreenshotMinFrame = 0;
    std::string mVulkanSDK;
};

bool gForceSmokeVolumeLightOutput = false;

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
    if (std::strcmp(value, "shader-probe") == 0)
    {
        mode = SmokeMode::ShaderProbe;
        return true;
    }
    if (std::strcmp(value, "shader-suite") == 0)
    {
        mode = SmokeMode::ShaderSuite;
        return true;
    }
    if (std::strcmp(value, "class1-gbuffer-color-probe") == 0)
    {
        mode = SmokeMode::Class1GBufferColorProbe;
        return true;
    }
    if (std::strcmp(value, "terrain-final-probe") == 0)
    {
        mode = SmokeMode::TerrainFinalProbe;
        return true;
    }
    if (std::strcmp(value, "final-color-compare") == 0)
    {
        mode = SmokeMode::FinalColorCompare;
        return true;
    }
    if (std::strcmp(value, "viewer-deferred-color-compare") == 0)
    {
        mode = SmokeMode::ViewerDeferredColorCompare;
        return true;
    }
    if (std::strcmp(value, "viewer-deferred-soften-state-probe") == 0)
    {
        mode = SmokeMode::ViewerDeferredSoftenStateProbe;
        return true;
    }
    if (std::strcmp(value, "viewer-deferred-emissive-probe") == 0)
    {
        mode = SmokeMode::ViewerDeferredEmissiveProbe;
        return true;
    }
    if (std::strcmp(value, "viewer-deferred-reflection-probe") == 0)
    {
        mode = SmokeMode::ViewerDeferredReflectionProbe;
        return true;
    }
    if (std::strcmp(value, "viewer-deferred-real-reflection-probe") == 0)
    {
        mode = SmokeMode::ViewerDeferredRealReflectionProbe;
        return true;
    }
    if (std::strcmp(value, "viewer-deferred-hero-probe") == 0)
    {
        mode = SmokeMode::ViewerDeferredHeroProbe;
        return true;
    }
    if (std::strcmp(value, "viewer-deferred-ssr-probe") == 0)
    {
        mode = SmokeMode::ViewerDeferredSSRProbe;
        return true;
    }
    if (std::strcmp(value, "viewer-deferred-local-light-probe") == 0)
    {
        mode = SmokeMode::ViewerDeferredLocalLightProbe;
        return true;
    }
    if (std::strcmp(value, "viewer-deferred-projector-light-probe") == 0)
    {
        mode = SmokeMode::ViewerDeferredProjectorLightProbe;
        return true;
    }
    if (std::strcmp(value, "viewer-deferred-point-light-volume-probe") == 0)
    {
        mode = SmokeMode::ViewerDeferredPointLightVolumeProbe;
        return true;
    }
    if (std::strcmp(value, "viewer-deferred-spot-light-volume-probe") == 0)
    {
        mode = SmokeMode::ViewerDeferredSpotLightVolumeProbe;
        return true;
    }
    if (std::strcmp(value, "viewer-deferred-lightmap-blur-probe") == 0)
    {
        mode = SmokeMode::ViewerDeferredLightMapBlurProbe;
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
    case SmokeMode::Class1GBufferColorProbe:
        return "class1-gbuffer-color-probe";
    case SmokeMode::TerrainFinalProbe:
        return "terrain-final-probe";
    case SmokeMode::WorldPipelines:
        return "world-pipelines";
    case SmokeMode::ShaderProbe:
        return "shader-probe";
    case SmokeMode::ShaderSuite:
        return "shader-suite";
    case SmokeMode::FinalColorCompare:
        return "final-color-compare";
    case SmokeMode::ViewerDeferredColorCompare:
        return "viewer-deferred-color-compare";
    case SmokeMode::ViewerDeferredSoftenStateProbe:
        return "viewer-deferred-soften-state-probe";
    case SmokeMode::ViewerDeferredEmissiveProbe:
        return "viewer-deferred-emissive-probe";
    case SmokeMode::ViewerDeferredReflectionProbe:
        return "viewer-deferred-reflection-probe";
    case SmokeMode::ViewerDeferredRealReflectionProbe:
        return "viewer-deferred-real-reflection-probe";
    case SmokeMode::ViewerDeferredHeroProbe:
        return "viewer-deferred-hero-probe";
    case SmokeMode::ViewerDeferredSSRProbe:
        return "viewer-deferred-ssr-probe";
    case SmokeMode::ViewerDeferredLocalLightProbe:
        return "viewer-deferred-local-light-probe";
    case SmokeMode::ViewerDeferredProjectorLightProbe:
        return "viewer-deferred-projector-light-probe";
    case SmokeMode::ViewerDeferredPointLightVolumeProbe:
        return "viewer-deferred-point-light-volume-probe";
    case SmokeMode::ViewerDeferredSpotLightVolumeProbe:
        return "viewer-deferred-spot-light-volume-probe";
    case SmokeMode::ViewerDeferredLightMapBlurProbe:
        return "viewer-deferred-lightmap-blur-probe";
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
    case SmokeMode::ShaderProbe:
        return " rendered through one selected Vulkan world shader pipeline. ";
    case SmokeMode::ShaderSuite:
        return " rendered as one fullscreen Vulkan shader-probe case per frame. ";
    case SmokeMode::Class1GBufferColorProbe:
        return " rendered as a class1-style G-buffer color grid, then copied from deferredScreen color attachment 0 to the swapchain. ";
    case SmokeMode::TerrainFinalProbe:
        return " rendered as a terrain-only viewer deferred graph with full terrain texture bindings, G-buffer, deferred composite, and final composite. ";
    case SmokeMode::FinalColorCompare:
        return " rendered through the real Vulkan final composite shader and compared against an OpenGL-style CPU color reference. ";
    case SmokeMode::ViewerDeferredColorCompare:
        return " rendered through a viewer-style legacy G-buffer, deferred composite, final composite, and compared against an OpenGL-style CPU color reference. ";
    case SmokeMode::ViewerDeferredSoftenStateProbe:
        return " rendered through the viewer-style deferred graph with the real DeferredSoften shader, lightMap input, and non-neutral sun/moon/classic/SSAO state. ";
    case SmokeMode::ViewerDeferredEmissiveProbe:
        return " rendered through the viewer-style deferred graph with a PBR emissive G-buffer band consumed by the real DeferredSoften shader before final composite. ";
    case SmokeMode::ViewerDeferredReflectionProbe:
        return " rendered through the viewer-style deferred graph with a glossy metallic PBR band consumed by the real DeferredSoften environment/probe inputs before final composite. ";
    case SmokeMode::ViewerDeferredRealReflectionProbe:
        return " rendered through the viewer-style deferred graph with a synthetic ReflectionProbes UBO and real cube-array radiance/irradiance inputs consumed by DeferredSoften before final composite. ";
    case SmokeMode::ViewerDeferredHeroProbe:
        return " rendered through the viewer-style deferred graph with a synthetic hero probe mixed by DeferredSoften over the regular reflection-probe cube arrays before final composite. ";
    case SmokeMode::ViewerDeferredSSRProbe:
        return " rendered through the viewer-style deferred graph with deterministic sceneMap/sceneDepthMap inputs consumed by DeferredSoften SSR before final composite. ";
    case SmokeMode::ViewerDeferredLocalLightProbe:
        return " rendered through the viewer-style deferred graph with a separate additive MultiPointLight pass before final composite. ";
    case SmokeMode::ViewerDeferredProjectorLightProbe:
        return " rendered through the viewer-style deferred graph with a separate additive MultiSpotLight projector pass before final composite. ";
    case SmokeMode::ViewerDeferredPointLightVolumeProbe:
        return " rendered through the viewer-style deferred graph with a separate additive PointLight cube-volume pass before final composite. ";
    case SmokeMode::ViewerDeferredSpotLightVolumeProbe:
        return " rendered through the viewer-style deferred graph with a separate additive SpotLight cube-volume projector pass before final composite. ";
    case SmokeMode::ViewerDeferredLightMapBlurProbe:
        return " rendered through the viewer-style deferred graph with two DeferredBlurLight lightMap ping-pong passes. ";
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
    case SmokeMode::Class1GBufferColorProbe:
    case SmokeMode::TerrainFinalProbe:
    case SmokeMode::FinalColorCompare:
    case SmokeMode::ViewerDeferredColorCompare:
    case SmokeMode::ViewerDeferredSoftenStateProbe:
    case SmokeMode::ViewerDeferredEmissiveProbe:
    case SmokeMode::ViewerDeferredReflectionProbe:
    case SmokeMode::ViewerDeferredRealReflectionProbe:
    case SmokeMode::ViewerDeferredHeroProbe:
    case SmokeMode::ViewerDeferredSSRProbe:
    case SmokeMode::ViewerDeferredLocalLightProbe:
    case SmokeMode::ViewerDeferredProjectorLightProbe:
    case SmokeMode::ViewerDeferredPointLightVolumeProbe:
    case SmokeMode::ViewerDeferredSpotLightVolumeProbe:
    case SmokeMode::ViewerDeferredLightMapBlurProbe:
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
    case SmokeMode::ShaderProbe:
    case SmokeMode::ShaderSuite:
    case SmokeMode::ViewerDeferredEmissiveProbe:
    case SmokeMode::ViewerDeferredReflectionProbe:
    case SmokeMode::ViewerDeferredLocalLightProbe:
    case SmokeMode::ViewerDeferredProjectorLightProbe:
    case SmokeMode::ViewerDeferredPointLightVolumeProbe:
    case SmokeMode::ViewerDeferredSpotLightVolumeProbe:
    case SmokeMode::ViewerDeferredLightMapBlurProbe:
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
        << "                             world-pipelines,\n"
        << "                             shader-probe,\n"
        << "                             shader-suite,\n"
        << "                             class1-gbuffer-color-probe,\n"
        << "                             terrain-final-probe,\n"
        << "                             final-color-compare,\n"
        << "                             viewer-deferred-color-compare,\n"
        << "                             viewer-deferred-soften-state-probe,\n"
        << "                             viewer-deferred-emissive-probe,\n"
        << "                             viewer-deferred-reflection-probe,\n"
        << "                             viewer-deferred-real-reflection-probe,\n"
        << "                             viewer-deferred-hero-probe,\n"
        << "                             viewer-deferred-ssr-probe,\n"
        << "                             viewer-deferred-local-light-probe,\n"
        << "                             viewer-deferred-projector-light-probe,\n"
        << "                             viewer-deferred-point-light-volume-probe,\n"
        << "                             viewer-deferred-spot-light-volume-probe,\n"
        << "                             viewer-deferred-lightmap-blur-probe\n"
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
        << "  --reference-ppm <path>     Write a CPU reference PPM for modes that support it\n"
        << "  --opengl-reference-ppm <path>\n"
        << "                             Write a real OpenGL reference PPM for supported shader cases\n"
        << "  --shader-case <name>       Select one shader case for --mode shader-probe\n"
        << "                             Use --mode shader-suite --frames 15 for one pass over all runtime cases\n"
        << "  --list-shader-cases        List shader-probe cases and exit\n"
        << "  --list-shader-parity       List Vulkan probe cases and their OpenGL shader references\n"
        << "  --list-pipeline-contracts  List OpenGL-derived world material pipeline contracts\n"
        << "  --no-buffer-readbacks      Disable G-buffer/composite input readbacks\n"
        << "  --no-stdout-readback       Keep readbacks in normal logs only\n"
        << "  --ui                       Draw a synthetic UI layer after the world/deferred pass\n"
        << "  --ui-viewer-sequence       Draw a stronger viewer-style UI sequence after the world/deferred pass\n"
        << "  --scene-marker             Draw a small scene-identification marker\n"
        << "  --force-volume-light-output\n"
        << "                             Force local light volume shaders to output a constant color\n"
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

        if (argument == "--reference-ppm" ||
            (value = value_after_equals(argument, "--reference-ppm")) != nullptr)
        {
            if (!value)
            {
                value = require_value(i, "--reference-ppm");
            }
            if (!value)
            {
                return false;
            }
            options.mReferencePPMPath = value;
            continue;
        }

        if (argument == "--opengl-reference-ppm" ||
            (value = value_after_equals(argument, "--opengl-reference-ppm")) != nullptr)
        {
            if (!value)
            {
                value = require_value(i, "--opengl-reference-ppm");
            }
            if (!value)
            {
                return false;
            }
            options.mOpenGLReferencePPMPath = value;
            continue;
        }

        if (argument == "--shader-case" ||
            (value = value_after_equals(argument, "--shader-case")) != nullptr)
        {
            if (!value)
            {
                value = require_value(i, "--shader-case");
            }
            if (!value || !*value)
            {
                std::cerr << "--shader-case requires a value.\n";
                return false;
            }
            options.mShaderCase = value;
            options.mShaderCaseExplicit = true;
            continue;
        }

        if (argument == "--list-shader-cases")
        {
            options.mListShaderCases = true;
            continue;
        }

        if (argument == "--list-shader-parity")
        {
            options.mListShaderParity = true;
            continue;
        }

        if (argument == "--list-pipeline-contracts")
        {
            options.mListPipelineContracts = true;
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

        if (argument == "--force-volume-light-output")
        {
            options.mForceVolumeLightOutput = true;
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

struct SmokeCube
{
    LLRenderBufferHandle mVertexBuffer;
    LLRenderBufferHandle mIndexBuffer;
};

bool create_smoke_quad(
    LLRenderBackend& backend,
    SmokeQuad& quad,
    const std::array<U8, 4>& vertex_color = {{ 255, 255, 255, 255 }},
    F32 normal_y = 0.f,
    F32 normal_z = 1.f)
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
        {{ 0.f, normal_y, normal_z, 0.f }},
        {{ 0.f, normal_y, normal_z, 0.f }},
        {{ 0.f, normal_y, normal_z, 0.f }},
        {{ 0.f, normal_y, normal_z, 0.f }},
        {{ 0.f, normal_y, normal_z, 0.f }},
        {{ 0.f, normal_y, normal_z, 0.f }},
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
        vertex_color,
        vertex_color,
        vertex_color,
        vertex_color,
        vertex_color,
        vertex_color,
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

bool create_smoke_cube(LLRenderBackend& backend, SmokeCube& cube)
{
    const std::array<std::array<F32, 4>, 8> positions =
    {{
        {{ -1.f, -1.f, -1.f, 1.f }},
        {{ -1.f, -1.f,  1.f, 1.f }},
        {{ -1.f,  1.f, -1.f, 1.f }},
        {{ -1.f,  1.f,  1.f, 1.f }},
        {{  1.f, -1.f, -1.f, 1.f }},
        {{  1.f, -1.f,  1.f, 1.f }},
        {{  1.f,  1.f, -1.f, 1.f }},
        {{  1.f,  1.f,  1.f, 1.f }},
    }};
    const std::array<U16, 64> fan_indices =
    {{
        7, 6, 2, 3, 1, 5, 4, 6,
        3, 2, 0, 1, 5, 7, 6, 2,
        5, 4, 6, 7, 3, 1, 0, 4,
        1, 0, 4, 5, 7, 3, 2, 0,
        6, 0, 2, 3, 7, 5, 4, 0,
        2, 4, 0, 1, 3, 7, 6, 4,
        4, 2, 6, 7, 5, 1, 0, 2,
        0, 6, 4, 5, 1, 3, 2, 6,
    }};

    std::vector<U8> vertex_bytes;
    vertex_bytes.reserve(positions.size() * sizeof(positions[0]));
    for (const auto& position : positions)
    {
        append_bytes(vertex_bytes, position);
    }

    std::vector<U8> index_bytes;
    index_bytes.reserve(fan_indices.size() * sizeof(fan_indices[0]));
    for (const U16 index : fan_indices)
    {
        append_bytes(index_bytes, index);
    }

    cube.mVertexBuffer = backend.createBufferHandle();
    cube.mIndexBuffer = backend.createBufferHandle();
    if (!cube.mVertexBuffer || !cube.mIndexBuffer)
    {
        return false;
    }

    backend.bindBuffer(LLRenderBufferTarget::Vertex, cube.mVertexBuffer);
    backend.allocateBufferStorage(
        LLRenderBufferTarget::Vertex,
        vertex_bytes.size(),
        vertex_bytes.data(),
        LLRenderBufferUsage::StaticDraw);
    backend.bindBuffer(LLRenderBufferTarget::Index, cube.mIndexBuffer);
    backend.allocateBufferStorage(
        LLRenderBufferTarget::Index,
        index_bytes.size(),
        index_bytes.data(),
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

void bind_smoke_cube(LLRenderBackend& backend, const SmokeCube& cube)
{
    backend.bindBuffer(LLRenderBufferTarget::Vertex, cube.mVertexBuffer);
    for (U32 location = 1; location < 16; ++location)
    {
        backend.disableVertexAttributeArray(location);
    }
    backend.enableVertexAttributeArray(0);
    backend.setVertexAttributePointer(
        0,
        3,
        LLRenderVertexAttributeType::Float32,
        false,
        16,
        nullptr);
    backend.bindBuffer(LLRenderBufferTarget::Index, cube.mIndexBuffer);
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
    LLRenderTextureHandle mCubeWhite;
};

struct SmokeReflectionProbeResources
{
    LLRenderTextureHandle mReflectionCubeArray;
    LLRenderTextureHandle mIrradianceCubeArray;
    LLRenderTextureHandle mHeroCubeArray;
    LLRenderBufferHandle mProbeUniformBuffer;
    SmokeOffscreen mCopySource;
    bool mHeroProbeEnabled = false;
};

struct SmokeSSRResources
{
    LLRenderTextureHandle mSceneColor;
    LLRenderTextureHandle mSceneDepth;
    U32 mWidth = 0;
    U32 mHeight = 0;
};

struct SmokeReflectionProbeData
{
    std::array<glm::mat4, 256> mRefBox;
    glm::mat4 mHeroBox;
    std::array<glm::vec4, 256> mRefSphere;
    std::array<glm::vec4, 256> mRefParams;
    glm::vec4 mHeroSphere;
    std::array<glm::ivec4, 256> mRefIndex;
    std::array<glm::ivec4, 1024> mRefNeighbor;
    std::array<glm::ivec4, 256> mRefBucket;
    S32 mRefmapCount;
    S32 mHeroShape;
    S32 mHeroMipCount;
    S32 mHeroProbeCount;
};

static_assert(
    sizeof(SmokeReflectionProbeData) == 49248,
    "Smoke ReflectionProbes UBO must match the shader/std140 layout");

void release_smoke_reflection_probe_resources(
    LLRenderBackend& backend,
    SmokeReflectionProbeResources& resources)
{
    if (resources.mProbeUniformBuffer)
    {
        backend.deleteBufferHandle(resources.mProbeUniformBuffer);
    resources.mProbeUniformBuffer = {};
    }
    if (resources.mReflectionCubeArray)
    {
        backend.deleteTextureHandle(resources.mReflectionCubeArray);
        resources.mReflectionCubeArray = {};
    }
    if (resources.mIrradianceCubeArray)
    {
        backend.deleteTextureHandle(resources.mIrradianceCubeArray);
        resources.mIrradianceCubeArray = {};
    }
    if (resources.mHeroCubeArray)
    {
        backend.deleteTextureHandle(resources.mHeroCubeArray);
        resources.mHeroCubeArray = {};
    }
    release_smoke_offscreen(backend, resources.mCopySource);
    resources.mHeroProbeEnabled = false;
}

bool upload_smoke_reflection_probe_source_pixel(
    LLRenderBackend& backend,
    SmokeOffscreen& source,
    const std::array<U8, 4>& color)
{
    if (!ensure_smoke_offscreen(backend, source, 1, 1))
    {
        return false;
    }

    backend.setActiveTextureUnit(0);
    backend.bindTexture(LLRenderTextureTarget::Texture2D, source.mColorTexture);
    backend.setTextureSubImage2D(
        LLRenderTextureTarget::Texture2D,
        0,
        0,
        0,
        1,
        1,
        0x1908, // GL_RGBA
        0x1401, // GL_UNSIGNED_BYTE
        color.data());
    return backend.didLastTextureUploadSucceed();
}

bool create_smoke_cube_array_texture(
    LLRenderBackend& backend,
    SmokeOffscreen& source,
    LLRenderTextureHandle& texture,
    const std::array<U8, 4>& color)
{
    texture = backend.createTextureHandle();
    if (!texture)
    {
        return false;
    }

    backend.setActiveTextureUnit(0);
    backend.bindTexture(LLRenderTextureTarget::TextureCubeMapArray, texture);
    backend.setTextureImage3D(
        LLRenderTextureTarget::TextureCubeMapArray,
        0,
        0,
        1,
        1,
        6,
        0,
        0x1908, // GL_RGBA
        0x1401, // GL_UNSIGNED_BYTE
        nullptr);
    if (!backend.didLastTextureUploadSucceed())
    {
        backend.deleteTextureHandle(texture);
        texture = {};
        return false;
    }

    backend.setTextureFilter(
        LLRenderTextureTarget::TextureCubeMapArray,
        LLRenderTextureFilter::Linear,
        LLRenderTextureFilter::Linear);
    backend.setTextureAddressMode(
        LLRenderTextureTarget::TextureCubeMapArray,
        LLRenderTextureAddressMode::ClampToEdge);

    if (!upload_smoke_reflection_probe_source_pixel(backend, source, color))
    {
        backend.deleteTextureHandle(texture);
        texture = {};
        return false;
    }

    backend.bindFramebuffer(
        LLRenderFramebufferBindPoint::Read,
        source.mFramebuffer);
    backend.setActiveTextureUnit(0);
    backend.bindTexture(LLRenderTextureTarget::TextureCubeMapArray, texture);
    for (S32 layer = 0; layer < 6; ++layer)
    {
        backend.copyTextureSubImage3D(
            LLRenderTextureTarget::TextureCubeMapArray,
            0,
            0,
            0,
            layer,
            0,
            0,
            1,
            1);
    }
    backend.bindFramebuffer(
        LLRenderFramebufferBindPoint::Read,
        LLRenderFramebufferHandle());
    return true;
}

SmokeReflectionProbeData make_smoke_reflection_probe_data(bool hero_probe)
{
    SmokeReflectionProbeData data = {};

    data.mRefSphere[0] = glm::vec4(0.f, 0.f, -1.f, 16.f);
    data.mRefIndex[0] = glm::ivec4(0, -1, -1, 0);

    data.mRefSphere[1] = glm::vec4(0.f, 0.f, -1.f, 16.f);
    data.mRefParams[1] = glm::vec4(1.f, 1.f, 1.f, 0.f);
    data.mRefIndex[1] = glm::ivec4(0, -1, -1, 1);

    for (glm::ivec4& bucket : data.mRefBucket)
    {
        bucket = glm::ivec4(1, 0, 0, 0);
    }

    data.mRefmapCount = 2;
    if (hero_probe)
    {
        data.mHeroSphere = glm::vec4(0.f, 0.f, -1.f, 16.f);
        data.mHeroShape = 1;
        data.mHeroMipCount = 1;
        data.mHeroProbeCount = 1;
    }
    else
    {
        data.mHeroShape = 0;
        data.mHeroMipCount = 0;
        data.mHeroProbeCount = 0;
    }
    return data;
}

bool ensure_smoke_reflection_probe_resources(
    LLRenderBackend& backend,
    SmokeReflectionProbeResources& resources,
    bool hero_probe = false)
{
    if (resources.mReflectionCubeArray &&
        resources.mIrradianceCubeArray &&
        resources.mHeroCubeArray &&
        resources.mProbeUniformBuffer &&
        resources.mHeroProbeEnabled == hero_probe)
    {
        return true;
    }

    release_smoke_reflection_probe_resources(backend, resources);

    if (!create_smoke_cube_array_texture(
            backend,
            resources.mCopySource,
            resources.mReflectionCubeArray,
            { 48, 132, 255, 255 }) ||
        !create_smoke_cube_array_texture(
            backend,
            resources.mCopySource,
            resources.mIrradianceCubeArray,
            { 255, 128, 42, 255 }) ||
        !create_smoke_cube_array_texture(
            backend,
            resources.mCopySource,
            resources.mHeroCubeArray,
            { 64, 255, 160, 255 }))
    {
        release_smoke_reflection_probe_resources(backend, resources);
        return false;
    }

    const SmokeReflectionProbeData data =
        make_smoke_reflection_probe_data(hero_probe);
    resources.mProbeUniformBuffer = backend.createBufferHandle();
    if (!resources.mProbeUniformBuffer)
    {
        release_smoke_reflection_probe_resources(backend, resources);
        return false;
    }

    backend.bindBuffer(
        LLRenderBufferTarget::Uniform,
        resources.mProbeUniformBuffer);
    backend.allocateBufferStorage(
        LLRenderBufferTarget::Uniform,
        sizeof(data),
        &data,
        LLRenderBufferUsage::StreamDraw);
    backend.bindBuffer(LLRenderBufferTarget::Uniform, LLRenderBufferHandle());
    resources.mHeroProbeEnabled = hero_probe;
    return true;
}

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

void release_smoke_ssr_resources(
    LLRenderBackend& backend,
    SmokeSSRResources& resources)
{
    if (resources.mSceneColor)
    {
        backend.deleteTextureHandle(resources.mSceneColor);
        resources.mSceneColor = {};
    }
    if (resources.mSceneDepth)
    {
        backend.deleteTextureHandle(resources.mSceneDepth);
        resources.mSceneDepth = {};
    }
    resources.mWidth = 0;
    resources.mHeight = 0;
}

std::vector<U8> make_smoke_ssr_scene_color_pixels(U32 width, U32 height)
{
    std::vector<U8> pixels;
    pixels.resize(static_cast<size_t>(width) * static_cast<size_t>(height) * 4U);
    for (U32 y = 0; y < height; ++y)
    {
        for (U32 x = 0; x < width; ++x)
        {
            const F32 u =
                static_cast<F32>(x) / static_cast<F32>(llmax(1U, width - 1U));
            const F32 v =
                static_cast<F32>(y) / static_cast<F32>(llmax(1U, height - 1U));
            const size_t offset =
                (static_cast<size_t>(y) * width + x) * 4U;
            pixels[offset + 0] = static_cast<U8>(llclamp(245.f - 80.f * v, 0.f, 255.f));
            pixels[offset + 1] = static_cast<U8>(llclamp(18.f + 30.f * u, 0.f, 255.f));
            pixels[offset + 2] = static_cast<U8>(llclamp(25.f + 20.f * (1.f - u), 0.f, 255.f));
            pixels[offset + 3] = 255;
        }
    }
    return pixels;
}

bool ensure_smoke_ssr_resources(
    LLRenderBackend& backend,
    SmokeSSRResources& resources,
    U32 width,
    U32 height)
{
    if (resources.mSceneColor &&
        resources.mSceneDepth &&
        resources.mWidth == width &&
        resources.mHeight == height)
    {
        return true;
    }

    release_smoke_ssr_resources(backend, resources);
    resources.mWidth = width;
    resources.mHeight = height;

    if (!create_smoke_texture(
            backend,
            resources.mSceneColor,
            width,
            height,
            make_smoke_ssr_scene_color_pixels(width, height)) ||
        !create_smoke_texture(
            backend,
            resources.mSceneDepth,
            width,
            height,
            make_solid_rgba_pixels(width, height, 13, 13, 13, 255)))
    {
        release_smoke_ssr_resources(backend, resources);
        return false;
    }
    return true;
}

bool create_smoke_cube_texture(
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

    const LLRenderTextureTarget cube_faces[] =
    {
        LLRenderTextureTarget::TextureCubeMapPositiveX,
        LLRenderTextureTarget::TextureCubeMapNegativeX,
        LLRenderTextureTarget::TextureCubeMapPositiveY,
        LLRenderTextureTarget::TextureCubeMapNegativeY,
        LLRenderTextureTarget::TextureCubeMapPositiveZ,
        LLRenderTextureTarget::TextureCubeMapNegativeZ,
    };

    backend.setActiveTextureUnit(0);
    backend.bindTexture(LLRenderTextureTarget::TextureCubeMap, texture);
    for (LLRenderTextureTarget face : cube_faces)
    {
        backend.setTextureImage2D(
            face,
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
    }

    backend.setTextureFilter(
        LLRenderTextureTarget::TextureCubeMap,
        LLRenderTextureFilter::Linear,
        LLRenderTextureFilter::Linear);
    backend.setTextureAddressMode(
        LLRenderTextureTarget::TextureCubeMap,
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
        &textures.mCubeWhite,
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
        textures.mWhite &&
        textures.mCubeWhite)
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
            make_solid_rgba_pixels(texture_width, texture_height, 204, 204, 204, 255)) ||
        !create_smoke_texture(
            backend,
            textures.mWhite,
            1,
            1,
            make_solid_rgba_pixels(1, 1, 255, 255, 255, 255)) ||
        !create_smoke_cube_texture(
            backend,
            textures.mCubeWhite,
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
    backend.setActiveTextureUnit(8);
    backend.bindTexture(LLRenderTextureTarget::Texture2D, textures.mDepth);
    backend.setActiveTextureUnit(9);
    backend.bindTexture(LLRenderTextureTarget::Texture2D, textures.mDiffuse);
    for (S32 unit = 10; unit <= 12; ++unit)
    {
        backend.setActiveTextureUnit(unit);
        backend.bindTexture(LLRenderTextureTarget::Texture2D, textures.mEmissive);
    }
    backend.setActiveTextureUnit(0);
}

void bind_terrain_final_probe_textures(
    LLRenderBackend& backend,
    const SmokeDeferredTextures& textures)
{
    const LLRenderTextureHandle bindings[] =
    {
        textures.mDiffuse,
        textures.mSpecular,
        textures.mEmissive,
        textures.mWhite,
        textures.mWhite,
        textures.mWhite,
        textures.mWhite,
        textures.mWhite,
        textures.mWhite,
        textures.mEmissive,
        textures.mEmissive,
        textures.mEmissive,
        textures.mEmissive,
        textures.mNormal,
        textures.mNormal,
        textures.mNormal,
        textures.mNormal,
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
    parameters.mUsesPBRMaterials = 1.f;
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
    LLWorldRenderMaterialClass mMaterialClass =
        LLWorldRenderMaterialClass::SimpleOpaque;
    bool mHasMaterialClass = false;
};

std::array<SmokeWorldPipelineEntry, 17> make_world_pipeline_probe_entries()
{
    constexpr U32 fullbright =
        LLRenderWorldMaterialParameters::Fullbright;
    constexpr U32 fullbright_shiny =
        LLRenderWorldMaterialParameters::Fullbright |
        LLRenderWorldMaterialParameters::LegacyShiny |
        LLRenderWorldMaterialParameters::PostDeferred;
    constexpr U32 pbr =
        LLRenderWorldMaterialParameters::GLTFPBR |
        LLRenderWorldMaterialParameters::HasNormalMap |
        LLRenderWorldMaterialParameters::HasORMMap;
    constexpr U32 material =
        LLRenderWorldMaterialParameters::HasSpecularMap |
        LLRenderWorldMaterialParameters::LegacyBump |
        LLRenderWorldMaterialParameters::LegacyShiny;
    constexpr U32 post_bump =
        LLRenderWorldMaterialParameters::LegacyBump |
        LLRenderWorldMaterialParameters::PostDeferred;
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

    return
    {{
        { LLRenderWorldShaderClass::Sky, "Sky", 0.25f, 0.48f, 0.92f, 0 },
        { LLRenderWorldShaderClass::Terrain, "Terrain", 0.34f, 0.58f, 0.28f, 0 },
        { LLRenderWorldShaderClass::Textured, "Textured", 0.42f, 0.72f, 0.96f, 0 },
        { LLRenderWorldShaderClass::AlphaMask, "AlphaMask", 0.92f, 0.78f, 0.28f, LLRenderWorldMaterialParameters::AlphaMask },
        { LLRenderWorldShaderClass::Fullbright, "Fullbright", 0.95f, 0.42f, 0.35f, fullbright },
        { LLRenderWorldShaderClass::Fullbright, "FullbrightShiny", 0.98f, 0.54f, 0.34f, fullbright_shiny, true, false, LLWorldRenderMaterialClass::FullbrightShiny, true },
        { LLRenderWorldShaderClass::Material, "Material", 0.78f, 0.58f, 0.95f, material },
        { LLRenderWorldShaderClass::Material, "PostBump", 0.72f, 0.82f, 0.96f, post_bump, false, false, LLWorldRenderMaterialClass::PostBump, true },
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
}

std::string normalize_shader_case_name(const std::string& value)
{
    std::string normalized;
    normalized.reserve(value.size());
    for (unsigned char character : value)
    {
        if (std::isalnum(character))
        {
            normalized.push_back(
                static_cast<char>(std::tolower(character)));
        }
    }
    return normalized;
}

struct SmokeShaderParityEntry
{
    const char* mCaseName;
    const char* mRuntimeVulkanVertex;
    const char* mRuntimeVulkanFragment;
    const char* mIntendedFinalVulkanVertex;
    const char* mIntendedFinalVulkanFragment;
    const char* mOpenGLVertexReference;
    const char* mOpenGLFragmentReference;
    const char* mRuntimeInterfaceContract;
    const char* mIntendedInterfaceContract;
    const char* mPipelineStateContract;
    const char* mStatus;
    const char* mNextStep;
};

std::array<SmokeShaderParityEntry, 15> make_shader_parity_entries()
{
    return
    {{
        {
            "Sky",
            "vulkan/final/active/world_textured.vert",
            "vulkan/final/class1/deferred/sky_runtime.frag",
            "vulkan/final/class1/deferred/sky.vert",
            "vulkan/final/class1/deferred/sky.frag",
            "class1/deferred/skyV.glsl",
            "class1/deferred/skyF.glsl",
            "runtime sky owner: MareWorldPushConstants plus set0 texture array",
            "pending final sky ABI: EEP/WindLight uniforms, generated varyings, and atmosphere inputs must be mapped before binding",
            "OpenGL sky pass: deferred sky draw, depth disabled/read-only by owner, blend and cull state inherited from sky owner",
            "runtime owner is no longer active/sky.frag; final class1 source exists but is not the runtime owner yet",
            "compare EEP sky uniforms and generated varyings against OpenGL class1 sky"
        },
        {
            "Terrain",
            "vulkan/final/active/terrain.vert",
            "vulkan/final/class1/deferred/terrain_runtime.frag",
            "vulkan/final/class1/deferred/terrain.vert",
            "vulkan/final/class1/deferred/terrain.frag",
            "class1/deferred/terrainV.glsl",
            "class1/deferred/terrainF.glsl",
            "runtime terrain owner: MareWorldPushConstants plus set0 terrain texture array",
            "pending final terrain ABI: terrain scale/offset, splat channels, and G-buffer outputs must match class1 terrain",
            "OpenGL terrain deferred pass: opaque blend, depth read/write, back-face cull unless owner disables it",
            "runtime owner is no longer active/terrain.frag; final class1 source exists but is not the runtime owner yet",
            "compare splat/base-color terrain inputs and G-buffer color output"
        },
        {
            "Textured",
            "vulkan/final/active/world_textured.vert",
            "vulkan/final/class1/objects/world_textured_runtime.frag",
            "vulkan/final/class1/deferred/diffuse.vert",
            "vulkan/final/class1/deferred/diffuse.frag",
            "class1/deferred/diffuseV.glsl; class1/objects/simpleNoAtmosV.glsl",
            "class1/deferred/diffuseF.glsl; class1/objects/simpleF.glsl",
            "runtime world-textured owner: MareWorldPushConstants plus set0 texture array",
            "pending split ABI: deferred diffuse and simple-object uniforms must not be collapsed unless OpenGL does",
            "LLWorldRenderCommand opaque/simple state: blend/depth/cull/color-mask translated from captured OpenGL state",
            "runtime owner is no longer active/world_textured.frag; maps to legacy diffuse/simple OpenGL families",
            "split direct textured and deferred diffuse comparisons instead of using one reference"
        },
        {
            "AlphaMask",
            "direct: vulkan/final/active/world_textured.vert; G-buffer: vulkan/final/class1/deferred/diffuse_indexed.vert",
            "direct: vulkan/final/class1/deferred/diffuse_alpha_mask_runtime.frag; G-buffer: vulkan/final/class1/deferred/diffuse_alpha_mask_indexed.frag",
            "vulkan/final/class1/deferred/diffuse_indexed.vert",
            "vulkan/final/class1/deferred/diffuse_alpha_mask_indexed.frag",
            "class1/deferred/diffuseV.glsl",
            "class1/deferred/diffuseAlphaMaskF.glsl; class1/deferred/diffuseAlphaMaskIndexedF.glsl",
            "G-buffer final ABI: MareWorldPushConstants, set0 texture array, texture index, runtime cutoff, optional skinning; direct AlphaMask still uses the active adapter",
            "same final G-buffer ABI; direct swapchain parity still needs a separate source-faithful object/simple path",
            "OpenGL alpha-mask deferred pass: alpha discard/cutoff, opaque-style depth write, cull state from captured draw owner",
            "runtime G-buffer alpha-mask now uses the indexed class1 final shader pair; direct shader-probe alpha-mask remains active adapter coverage",
            "add a controlled G-buffer alpha-mask probe and compare cutoff/discard behavior with OpenGL source output"
        },
        {
            "Fullbright",
            "vulkan/final/active/world_textured.vert",
            "vulkan/final/class1/deferred/fullbright_runtime.frag",
            "vulkan/final/class1/deferred/fullbright.vert",
            "vulkan/final/class1/deferred/fullbright.frag",
            "class1/deferred/fullbrightV.glsl; class1/deferred/fullbrightShinyV.glsl",
            "class1/deferred/fullbrightF.glsl; class3/deferred/fullbrightShinyF.glsl",
            "runtime fullbright owner: MareWorldPushConstants plus set0 texture array",
            "pending split ABI: fullbright, shiny, alpha-mask, and alpha variants need separate runtime contracts",
            "OpenGL fullbright state: owner-selected blend/depth/cull with no deferred lighting contribution",
            "runtime owner is no longer active/fullbright.frag; fullbright and shiny variants are still collapsed in the probe",
            "add separate non-shiny, alpha-mask, alpha, and shiny probe cases"
        },
        {
            "Material",
            "vulkan/final/active/world_textured.vert",
            "vulkan/final/class3/deferred/material_runtime.frag",
            "vulkan/final/class1/deferred/material.vert",
            "vulkan/final/class1/deferred/material.frag",
            "class1/deferred/materialV.glsl",
            "class1/deferred/materialF.glsl; class3/deferred/materialF.glsl",
            "runtime material owner: MareWorldPushConstants, set0 texture array, and synthetic normal/specular bindings",
            "pending material ABI: class1/class3 material uniforms, tangent basis, and map enables must match OpenGL",
            "OpenGL material deferred state: opaque G-buffer write, depth read/write, owner cull state",
            "runtime owner is no longer active/material.frag; specular/bump/shiny flags are synthetic",
            "compare legacy material permutations and normal/specular map inputs"
        },
        {
            "PBR",
            "vulkan/final/active/world_textured.vert",
            "vulkan/final/class1/deferred/pbr_runtime.frag",
            "vulkan/final/class1/deferred/pbropaque.vert",
            "vulkan/final/class1/deferred/pbropaque.frag",
            "class1/deferred/pbropaqueV.glsl; class1/gltf/pbrmetallicroughnessV.glsl",
            "class1/deferred/pbropaqueF.glsl; class1/gltf/pbrmetallicroughnessF.glsl",
            "runtime PBR owner: MareWorldPushConstants plus set0 texture array with normal/ORM/emissive slots",
            "pending PBR ABI: GLTF/PBR material uniforms, reflection probes, and lighting classes must match OpenGL settings",
            "OpenGL PBR deferred state: opaque G-buffer write with material-specific vertex attributes and tangent input",
            "runtime owner is no longer active/pbr.frag; PBR lighting and reflection probes are not a full OpenGL match yet",
            "compare base color, normal, ORM, emissive, and reflection-probe terms separately"
        },
        {
            "Avatar",
            "vulkan/final/active/world_textured.vert",
            "vulkan/final/class1/avatar/avatar_runtime.frag",
            "vulkan/final/class1/deferred/avatar.vert",
            "vulkan/final/class1/deferred/avatar.frag",
            "class1/deferred/avatarV.glsl; class1/avatar/avatarV.glsl; class1/avatar/avatarSkinV.glsl",
            "class1/deferred/avatarF.glsl; class1/avatar/avatarF.glsl",
            "runtime avatar owner: MareWorldPushConstants plus set0 avatar texture and optional skinning palette",
            "pending avatar ABI: baked textures, skinning include contract, impostor/direct/deferred variants must stay distinct",
            "OpenGL avatar state: avatar draw-pool selected depth/cull/blend with skinned vertex attributes when enabled",
            "runtime owner is no longer active/avatar.frag; smoke probe does not exercise real skinning or baked avatar textures",
            "add skinned vertex-buffer probe before judging avatar shader parity"
        },
        {
            "Water",
            "vulkan/final/active/world_textured.vert",
            "vulkan/final/class1/environment/water_runtime.frag",
            "vulkan/final/class1/environment/water.vert; vulkan/final/class3/environment/water.frag",
            "vulkan/final/class3/environment/water.frag",
            "class1/environment/waterV.glsl",
            "class1/environment/waterF.glsl; class3/environment/waterF.glsl; class3/environment/underWaterF.glsl",
            "runtime water owner: MareWorldPushConstants plus set0 texture array",
            "pending water ABI: water normals, reflection/refraction targets, fog, fresnel, and class1/class3 settings must match OpenGL",
            "OpenGL water pass: transparent water composition state with owner-specific depth/blend and reflection inputs",
            "runtime owner is no longer active/water.frag; class3 water source exists but runtime water remains approximate",
            "compare water normals, fresnel, fog, and reflection/refraction inputs"
        },
        {
            "Haze",
            "vulkan/final/active/world_textured.vert",
            "vulkan/final/class3/deferred/haze_runtime.frag",
            "vulkan/final/class2/deferred/soften_light.vert; vulkan/final/class3/deferred/haze.frag",
            "vulkan/final/class3/deferred/haze.frag",
            "class2/deferred/softenLightV.glsl",
            "class3/deferred/hazeF.glsl",
            "runtime haze owner: MareWorldPushConstants plus synthetic depth/color probe inputs",
            "pending haze ABI: class2 soften-light varyings and class3 haze atmospheric/depth uniforms must be mapped exactly",
            "OpenGL haze pass: fullscreen deferred pass using depth, color, water plane, and atmosphere uniforms",
            "runtime owner is no longer active/haze.frag; shader-probe is visible with controlled depth/color inputs, but differs strongly from the OpenGL source-level reference",
            "replace runtime haze approximation with the final class3 haze shader path and match depth/atmospheric inputs"
        },
        {
            "Alpha",
            "vulkan/final/class1/deferred/alpha.vert",
            "vulkan/final/class2/deferred/alpha.frag",
            "vulkan/final/class1/deferred/alpha.vert",
            "vulkan/final/class2/deferred/alpha.frag",
            "class1/deferred/alphaV.glsl",
            "class2/deferred/alphaF.glsl",
            "runtime alpha ABI: MareWorldPushConstants, set0 texture array, vertex color, and optional skinning palette",
            "same as runtime for the current final alpha owner; remaining work is missing OpenGL inputs, not a different ABI",
            "LLDrawPoolAlpha forward state: src-alpha/one-minus-src-alpha blend, owner-selected depth read/write, cull disabled for alpha",
            "runtime now uses class1/deferred/alpha.vert plus class2/deferred/alpha.frag; OpenGL source reference uses vertex color; strict RGB diff is 5.3333 mean, 15 max",
            "finish OpenGL local-light, reflection, fog, depth, and post-water lighting inputs"
        },
        {
            "Glow",
            "vulkan/final/active/world_textured.vert",
            "vulkan/final/class1/effects/glow_runtime.frag",
            "vulkan/final/class1/effects/glow.vert",
            "vulkan/final/class1/effects/glow.frag",
            "class1/effects/glowV.glsl",
            "class1/effects/glowF.glsl; class1/effects/glowExtractF.glsl",
            "runtime glow owner: MareWorldPushConstants plus set0 texture array",
            "pending glow ABI: extract, blur/combine, and glow target contracts must be separated like OpenGL",
            "OpenGL glow state: effect target writes and combine passes, not ordinary world textured state",
            "runtime owner is no longer active/glow.frag; glow combine/extract are not separated in the probe",
            "add extract and combine probe cases with OpenGL glow target inputs"
        },
        {
            "Copy",
            "vulkan/final/class1/interface/copy.vert",
            "vulkan/final/class1/interface/copy.frag",
            "vulkan/final/class1/interface/copy.vert",
            "vulkan/final/class1/interface/copy.frag",
            "class1/interface/copyV.glsl",
            "class1/interface/copyF.glsl",
            "runtime copy ABI: position-only fullscreen triangle, generated UV, set0 diffuseMap",
            "same as runtime; this is the final copy ABI",
            "OpenGL copy state: no blend, depth disabled, cull disabled, byte-level color copy",
            "runtime now uses the class1 interface copy vertex/fragment shaders; strict PPM diff is 0.0000 mean, 0 max",
            "keep as the byte-level copy shader harness baseline"
        },
        {
            "DeferredComposite",
            "vulkan/final/active/world_textured.vert",
            "vulkan/final/class3/deferred/deferred_composite_runtime.frag",
            "vulkan/final/class3/deferred/soften_light.vert",
            "vulkan/final/class3/deferred/soften_light.frag",
            "class2/deferred/softenLightV.glsl",
            "class3/deferred/softenLightF.glsl",
            "runtime deferred-composite owner: MareWorldPushConstants plus currently staged G-buffer inputs",
            "pending deferred ABI: class2/class3 soften-light uniforms, depth/light/SSAO/shadow/probe inputs must match OpenGL",
            "OpenGL deferred composite state: fullscreen lighting pass from G-buffer into light/composite targets",
            "runtime owner is no longer active/deferred_composite.frag; OpenGL reference is the soften/deferred lighting pass, not one simple shader",
            "compare controlled legacy/material/PBR G-buffer, depth, shadow, SSAO, and reflection-probe inputs"
        },
        {
            "FinalComposite",
            "vulkan/final/active/world_textured.vert",
            "vulkan/final/class1/deferred/final_composite_runtime.frag",
            "vulkan/final/class1/deferred/post_deferred.vert",
            "vulkan/final/class1/deferred/post_deferred.frag",
            "class1/deferred/postDeferredV.glsl",
            "class1/deferred/postDeferredF.glsl; postDeferredGammaCorrect.glsl; postDeferredTonemap.glsl",
            "runtime final-composite owner: MareWorldPushConstants plus staged color/depth/post targets",
            "pending post ABI: postDeferred variants, DoF, tonemap, gamma, AA, and glow inputs must follow user settings",
            "OpenGL final post state: fullscreen post-process pass with setting-selected shader class and no world depth writes",
            "runtime owner is no longer active/final_composite.frag; OpenGL final output depends on post-processing settings",
            "compare no-tonemap, tonemap, gamma, DoF, and FXAA/SMAA variants separately"
        },
    }};
}

bool find_shader_probe_case(
    const std::string& name,
    SmokeWorldPipelineEntry& output)
{
    const std::string requested = normalize_shader_case_name(name);
    for (const SmokeWorldPipelineEntry& entry : make_world_pipeline_probe_entries())
    {
        if (normalize_shader_case_name(entry.mName) == requested)
        {
            output = entry;
            return true;
        }
    }
    return false;
}

bool shader_parity_matches_filter(
    const SmokeShaderParityEntry& entry,
    const std::string& filter)
{
    if (filter.empty())
    {
        return true;
    }
    return normalize_shader_case_name(entry.mCaseName) ==
        normalize_shader_case_name(filter);
}

bool print_shader_parity_entries(const std::string& filter)
{
    bool found = false;
    std::cout << "Mare Vulkan/OpenGL shader parity map:\n";
    for (const SmokeShaderParityEntry& entry : make_shader_parity_entries())
    {
        if (!shader_parity_matches_filter(entry, filter))
        {
            continue;
        }
        found = true;
        std::cout
            << "\n"
            << entry.mCaseName
            << "\n"
            << "  runtime Vulkan: "
            << entry.mRuntimeVulkanVertex
            << " + "
            << entry.mRuntimeVulkanFragment
            << "\n"
            << "  intended final Vulkan: "
            << entry.mIntendedFinalVulkanVertex
            << " + "
            << entry.mIntendedFinalVulkanFragment
            << "\n"
            << "  OpenGL reference: "
            << entry.mOpenGLVertexReference
            << " + "
            << entry.mOpenGLFragmentReference
            << "\n"
            << "  runtime interface: "
            << entry.mRuntimeInterfaceContract
            << "\n"
            << "  intended interface: "
            << entry.mIntendedInterfaceContract
            << "\n"
            << "  pipeline state: "
            << entry.mPipelineStateContract
            << "\n"
            << "  status: "
            << entry.mStatus
            << "\n"
            << "  next: "
            << entry.mNextStep
            << "\n";
    }

    if (!found)
    {
        std::cerr
            << "No shader parity entry matched '"
            << filter
            << "'. Use --list-shader-cases.\n";
        return false;
    }
    return true;
}

std::array<LLWorldRenderMaterialClass, 23> make_world_render_material_contract_classes()
{
    return
    {{
        LLWorldRenderMaterialClass::Sky,
        LLWorldRenderMaterialClass::Terrain,
        LLWorldRenderMaterialClass::SimpleOpaque,
        LLWorldRenderMaterialClass::AlphaMask,
        LLWorldRenderMaterialClass::Grass,
        LLWorldRenderMaterialClass::Tree,
        LLWorldRenderMaterialClass::Fullbright,
        LLWorldRenderMaterialClass::FullbrightAlphaMask,
        LLWorldRenderMaterialClass::Bump,
        LLWorldRenderMaterialClass::LegacyMaterial,
        LLWorldRenderMaterialClass::GLTFPBR,
        LLWorldRenderMaterialClass::GLTFPBRAlphaMask,
        LLWorldRenderMaterialClass::Avatar,
        LLWorldRenderMaterialClass::AvatarImpostor,
        LLWorldRenderMaterialClass::Alpha,
        LLWorldRenderMaterialClass::Glow,
        LLWorldRenderMaterialClass::Water,
        LLWorldRenderMaterialClass::WaterExclusionMask,
        LLWorldRenderMaterialClass::WaterExclusionSurface,
        LLWorldRenderMaterialClass::AtmosphericHaze,
        LLWorldRenderMaterialClass::WaterHaze,
        LLWorldRenderMaterialClass::FullbrightShiny,
        LLWorldRenderMaterialClass::PostBump,
    }};
}

LLWorldRenderPipelineContract get_smoke_world_render_pipeline_contract(
    LLWorldRenderMaterialClass material_class)
{
    LLWorldRenderPipelineContract contract;
    contract.mMaterialClass = material_class;

    switch (material_class)
    {
        case LLWorldRenderMaterialClass::Fullbright:
        case LLWorldRenderMaterialClass::FullbrightAlphaMask:
        case LLWorldRenderMaterialClass::Alpha:
        case LLWorldRenderMaterialClass::Glow:
        case LLWorldRenderMaterialClass::Water:
        case LLWorldRenderMaterialClass::WaterExclusionMask:
        case LLWorldRenderMaterialClass::WaterExclusionSurface:
        case LLWorldRenderMaterialClass::AtmosphericHaze:
        case LLWorldRenderMaterialClass::WaterHaze:
        case LLWorldRenderMaterialClass::FullbrightShiny:
        case LLWorldRenderMaterialClass::PostBump:
            contract.mPassClass = LLWorldRenderPassClass::PostDeferred;
            break;
        case LLWorldRenderMaterialClass::Sky:
        default:
            contract.mPassClass = LLWorldRenderPassClass::Deferred;
            break;
    }

    switch (material_class)
    {
        case LLWorldRenderMaterialClass::Alpha:
            contract.mBlendMode = LLWorldRenderBlendMode::ForwardAlpha;
            break;
        case LLWorldRenderMaterialClass::Fullbright:
        case LLWorldRenderMaterialClass::FullbrightShiny:
            contract.mBlendMode = LLWorldRenderBlendMode::Alpha;
            break;
        case LLWorldRenderMaterialClass::AtmosphericHaze:
        case LLWorldRenderMaterialClass::WaterHaze:
            contract.mBlendMode = LLWorldRenderBlendMode::Haze;
            break;
        case LLWorldRenderMaterialClass::Glow:
            contract.mBlendMode = LLWorldRenderBlendMode::Add;
            break;
        case LLWorldRenderMaterialClass::PostBump:
            contract.mBlendMode = LLWorldRenderBlendMode::MultiplyX2;
            break;
        default:
            contract.mBlendMode = LLWorldRenderBlendMode::None;
            break;
    }

    switch (material_class)
    {
        case LLWorldRenderMaterialClass::Sky:
        case LLWorldRenderMaterialClass::Alpha:
        case LLWorldRenderMaterialClass::Glow:
        case LLWorldRenderMaterialClass::Water:
        case LLWorldRenderMaterialClass::AtmosphericHaze:
        case LLWorldRenderMaterialClass::WaterHaze:
        case LLWorldRenderMaterialClass::PostBump:
            contract.mDepthMode = LLWorldRenderDepthMode::ReadOnly;
            break;
        default:
            contract.mDepthMode = LLWorldRenderDepthMode::ReadWrite;
            break;
    }

    switch (material_class)
    {
        case LLWorldRenderMaterialClass::Alpha:
        case LLWorldRenderMaterialClass::AvatarImpostor:
        case LLWorldRenderMaterialClass::Sky:
        case LLWorldRenderMaterialClass::Water:
        case LLWorldRenderMaterialClass::WaterExclusionMask:
        case LLWorldRenderMaterialClass::AtmosphericHaze:
        case LLWorldRenderMaterialClass::WaterHaze:
            contract.mCullMode = LLWorldRenderCullMode::Disabled;
            break;
        default:
            contract.mCullMode = LLWorldRenderCullMode::Back;
            break;
    }

    switch (material_class)
    {
        case LLWorldRenderMaterialClass::Terrain:
            contract.mShaderClass = LLRenderWorldShaderClass::Terrain;
            break;
        case LLWorldRenderMaterialClass::Sky:
            contract.mShaderClass = LLRenderWorldShaderClass::Sky;
            break;
        case LLWorldRenderMaterialClass::Water:
            contract.mShaderClass = LLRenderWorldShaderClass::Water;
            break;
        case LLWorldRenderMaterialClass::WaterExclusionMask:
        case LLWorldRenderMaterialClass::WaterExclusionSurface:
        case LLWorldRenderMaterialClass::AtmosphericHaze:
        case LLWorldRenderMaterialClass::WaterHaze:
            contract.mShaderClass = LLRenderWorldShaderClass::Haze;
            break;
        case LLWorldRenderMaterialClass::Alpha:
            contract.mShaderClass = LLRenderWorldShaderClass::Alpha;
            break;
        case LLWorldRenderMaterialClass::Glow:
            contract.mShaderClass = LLRenderWorldShaderClass::Glow;
            break;
        case LLWorldRenderMaterialClass::AlphaMask:
        case LLWorldRenderMaterialClass::Grass:
        case LLWorldRenderMaterialClass::Tree:
        case LLWorldRenderMaterialClass::GLTFPBRAlphaMask:
            contract.mShaderClass = LLRenderWorldShaderClass::AlphaMask;
            break;
        case LLWorldRenderMaterialClass::Fullbright:
        case LLWorldRenderMaterialClass::FullbrightAlphaMask:
        case LLWorldRenderMaterialClass::FullbrightShiny:
            contract.mShaderClass = LLRenderWorldShaderClass::Fullbright;
            break;
        case LLWorldRenderMaterialClass::LegacyMaterial:
        case LLWorldRenderMaterialClass::Bump:
        case LLWorldRenderMaterialClass::PostBump:
            contract.mShaderClass = LLRenderWorldShaderClass::Material;
            break;
        case LLWorldRenderMaterialClass::GLTFPBR:
            contract.mShaderClass = LLRenderWorldShaderClass::PBR;
            break;
        case LLWorldRenderMaterialClass::Avatar:
        case LLWorldRenderMaterialClass::AvatarImpostor:
            contract.mShaderClass = LLRenderWorldShaderClass::Avatar;
            break;
        default:
            contract.mShaderClass = LLRenderWorldShaderClass::Textured;
            break;
    }

    if (material_class == LLWorldRenderMaterialClass::Glow)
    {
        contract.mWriteColor = false;
        contract.mWriteAlpha = true;
    }
    if (material_class == LLWorldRenderMaterialClass::Glow ||
        material_class == LLWorldRenderMaterialClass::PostBump)
    {
        contract.mPolygonOffsetEnabled = true;
        contract.mPolygonOffsetFactor = -1.f;
        contract.mPolygonOffsetUnits = -1.f;
    }

    return contract;
}

const char* get_smoke_world_render_material_class_name(
    LLWorldRenderMaterialClass material_class)
{
    switch (material_class)
    {
        case LLWorldRenderMaterialClass::Sky: return "Sky";
        case LLWorldRenderMaterialClass::Terrain: return "Terrain";
        case LLWorldRenderMaterialClass::SimpleOpaque: return "SimpleOpaque";
        case LLWorldRenderMaterialClass::AlphaMask: return "AlphaMask";
        case LLWorldRenderMaterialClass::Grass: return "Grass";
        case LLWorldRenderMaterialClass::Tree: return "Tree";
        case LLWorldRenderMaterialClass::Fullbright: return "Fullbright";
        case LLWorldRenderMaterialClass::FullbrightAlphaMask: return "FullbrightAlphaMask";
        case LLWorldRenderMaterialClass::Bump: return "Bump";
        case LLWorldRenderMaterialClass::LegacyMaterial: return "LegacyMaterial";
        case LLWorldRenderMaterialClass::GLTFPBR: return "GLTFPBR";
        case LLWorldRenderMaterialClass::GLTFPBRAlphaMask: return "GLTFPBRAlphaMask";
        case LLWorldRenderMaterialClass::Avatar: return "Avatar";
        case LLWorldRenderMaterialClass::AvatarImpostor: return "AvatarImpostor";
        case LLWorldRenderMaterialClass::Alpha: return "Alpha";
        case LLWorldRenderMaterialClass::Glow: return "Glow";
        case LLWorldRenderMaterialClass::Water: return "Water";
        case LLWorldRenderMaterialClass::WaterExclusionMask: return "WaterExclusionMask";
        case LLWorldRenderMaterialClass::WaterExclusionSurface: return "WaterExclusionSurface";
        case LLWorldRenderMaterialClass::AtmosphericHaze: return "AtmosphericHaze";
        case LLWorldRenderMaterialClass::WaterHaze: return "WaterHaze";
        case LLWorldRenderMaterialClass::FullbrightShiny: return "FullbrightShiny";
        case LLWorldRenderMaterialClass::PostBump: return "PostBump";
        case LLWorldRenderMaterialClass::Shadow: return "Shadow";
        case LLWorldRenderMaterialClass::ShadowAlphaMask: return "ShadowAlphaMask";
        case LLWorldRenderMaterialClass::AvatarShadow: return "AvatarShadow";
        case LLWorldRenderMaterialClass::AvatarAlphaShadow: return "AvatarAlphaShadow";
        case LLWorldRenderMaterialClass::AvatarAlphaMaskShadow: return "AvatarAlphaMaskShadow";
        case LLWorldRenderMaterialClass::TreeShadow: return "TreeShadow";
        case LLWorldRenderMaterialClass::PBRAlphaMaskShadow: return "PBRAlphaMaskShadow";
        case LLWorldRenderMaterialClass::PBRAlphaBlendShadow: return "PBRAlphaBlendShadow";
        case LLWorldRenderMaterialClass::Copy: return "Copy";
    }
    return "Unknown";
}

const char* get_smoke_world_render_pass_class_name(LLWorldRenderPassClass pass_class)
{
    switch (pass_class)
    {
        case LLWorldRenderPassClass::Deferred: return "Deferred";
        case LLWorldRenderPassClass::PostDeferred: return "PostDeferred";
    }
    return "Unknown";
}

const char* get_smoke_world_render_blend_mode_name(LLWorldRenderBlendMode blend_mode)
{
    switch (blend_mode)
    {
        case LLWorldRenderBlendMode::None: return "None";
        case LLWorldRenderBlendMode::Alpha: return "Alpha";
        case LLWorldRenderBlendMode::ForwardAlpha: return "ForwardAlpha";
        case LLWorldRenderBlendMode::Add: return "Add";
        case LLWorldRenderBlendMode::Haze: return "Haze";
        case LLWorldRenderBlendMode::MultiplyX2: return "MultiplyX2";
    }
    return "Unknown";
}

const char* get_smoke_world_render_depth_mode_name(LLWorldRenderDepthMode depth_mode)
{
    switch (depth_mode)
    {
        case LLWorldRenderDepthMode::ReadWrite: return "ReadWrite";
        case LLWorldRenderDepthMode::ReadOnly: return "ReadOnly";
        case LLWorldRenderDepthMode::Disabled: return "Disabled";
    }
    return "Unknown";
}

const char* get_smoke_world_render_cull_mode_name(LLWorldRenderCullMode cull_mode)
{
    switch (cull_mode)
    {
        case LLWorldRenderCullMode::Back: return "Back";
        case LLWorldRenderCullMode::Disabled: return "Disabled";
    }
    return "Unknown";
}

const char* get_smoke_world_render_shader_class_name(LLRenderWorldShaderClass shader_class)
{
    switch (shader_class)
    {
        case LLRenderWorldShaderClass::Textured: return "Textured";
        case LLRenderWorldShaderClass::Sky: return "Sky";
        case LLRenderWorldShaderClass::Water: return "Water";
        case LLRenderWorldShaderClass::Haze: return "Haze";
        case LLRenderWorldShaderClass::Alpha: return "Alpha";
        case LLRenderWorldShaderClass::Glow: return "Glow";
        case LLRenderWorldShaderClass::AlphaMask: return "AlphaMask";
        case LLRenderWorldShaderClass::Fullbright: return "Fullbright";
        case LLRenderWorldShaderClass::Material: return "Material";
        case LLRenderWorldShaderClass::PBR: return "PBR";
        case LLRenderWorldShaderClass::Avatar: return "Avatar";
        case LLRenderWorldShaderClass::Terrain: return "Terrain";
        case LLRenderWorldShaderClass::Shadow: return "Shadow";
        case LLRenderWorldShaderClass::ShadowAlphaMask: return "ShadowAlphaMask";
        case LLRenderWorldShaderClass::AvatarShadow: return "AvatarShadow";
        case LLRenderWorldShaderClass::AvatarAlphaShadow: return "AvatarAlphaShadow";
        case LLRenderWorldShaderClass::AvatarAlphaMaskShadow: return "AvatarAlphaMaskShadow";
        case LLRenderWorldShaderClass::TreeShadow: return "TreeShadow";
        case LLRenderWorldShaderClass::PBRAlphaMaskShadow: return "PBRAlphaMaskShadow";
        case LLRenderWorldShaderClass::PBRAlphaBlendShadow: return "PBRAlphaBlendShadow";
        case LLRenderWorldShaderClass::PointLight: return "PointLight";
        case LLRenderWorldShaderClass::MultiPointLight: return "MultiPointLight";
        case LLRenderWorldShaderClass::SpotLight: return "SpotLight";
        case LLRenderWorldShaderClass::MultiSpotLight: return "MultiSpotLight";
        case LLRenderWorldShaderClass::Copy: return "Copy";
        case LLRenderWorldShaderClass::DeferredLightMap: return "DeferredLightMap";
        case LLRenderWorldShaderClass::DeferredBlurLight: return "DeferredBlurLight";
        case LLRenderWorldShaderClass::DeferredSoften: return "DeferredSoften";
        case LLRenderWorldShaderClass::DeferredComposite: return "DeferredComposite";
        case LLRenderWorldShaderClass::FinalComposite: return "FinalComposite";
    }
    return "Unknown";
}

const char* get_world_render_blend_state_contract(LLWorldRenderBlendMode blend_mode)
{
    switch (blend_mode)
    {
        case LLWorldRenderBlendMode::None:
            return "blend disabled; Vulkan blend pipeline Opaque";
        case LLWorldRenderBlendMode::Alpha:
            return "source-alpha/one-minus-source-alpha color and alpha; Vulkan blend pipeline Alpha";
        case LLWorldRenderBlendMode::ForwardAlpha:
            return "color src-alpha/one-minus-src-alpha plus alpha zero/one-minus-src-alpha; Vulkan blend pipeline ForwardAlpha";
        case LLWorldRenderBlendMode::Add:
            return "one/one additive color and alpha; Vulkan blend pipeline Add";
        case LLWorldRenderBlendMode::Haze:
            return "one/source-alpha color plus zero/source-alpha alpha; Vulkan blend pipeline Haze";
        case LLWorldRenderBlendMode::MultiplyX2:
            return "dest-color/source-color multiply-x2; Vulkan blend pipeline MultiplyX2";
    }
    return "unknown blend contract";
}

const char* get_world_render_depth_state_contract(LLWorldRenderDepthMode depth_mode)
{
    switch (depth_mode)
    {
        case LLWorldRenderDepthMode::ReadWrite:
            return "depth test enabled, function <=, depth write enabled";
        case LLWorldRenderDepthMode::ReadOnly:
            return "depth test enabled, function <=, depth write disabled";
        case LLWorldRenderDepthMode::Disabled:
            return "depth test disabled, depth write disabled";
    }
    return "unknown depth contract";
}

const char* get_world_render_cull_state_contract(LLWorldRenderCullMode cull_mode)
{
    switch (cull_mode)
    {
        case LLWorldRenderCullMode::Back:
            return "back-face cull enabled unless the command is double-sided";
        case LLWorldRenderCullMode::Disabled:
            return "cull disabled";
    }
    return "unknown cull contract";
}

const char* get_world_render_color_pipeline_contract(
    bool write_color,
    bool write_alpha)
{
    if (write_color && write_alpha)
    {
        return "Enabled";
    }
    if (write_color)
    {
        return "ColorOnly";
    }
    if (write_alpha)
    {
        return "AlphaOnly";
    }
    return "Disabled";
}

bool print_world_pipeline_contract_entries()
{
    std::cout
        << "Mare Vulkan/OpenGL world pipeline contract map:\n"
        << "  Rule: Vulkan may use different API objects, but the pass, shader class,\n"
        << "  blend, depth, cull, and color-mask behavior must match this OpenGL-derived\n"
        << "  command contract before a final shader is considered branchable.\n";

    for (LLWorldRenderMaterialClass material_class : make_world_render_material_contract_classes())
    {
        const LLWorldRenderPipelineContract contract =
            get_smoke_world_render_pipeline_contract(material_class);
        std::cout
            << "\n"
            << get_smoke_world_render_material_class_name(material_class)
            << "\n"
            << "  pass: "
            << get_smoke_world_render_pass_class_name(contract.mPassClass)
            << "\n"
            << "  runtime shader class: "
            << get_smoke_world_render_shader_class_name(contract.mShaderClass)
            << "\n"
            << "  blend: "
            << get_smoke_world_render_blend_mode_name(contract.mBlendMode)
            << " - "
            << get_world_render_blend_state_contract(contract.mBlendMode)
            << "\n"
            << "  depth: "
            << get_smoke_world_render_depth_mode_name(contract.mDepthMode)
            << " - "
            << get_world_render_depth_state_contract(contract.mDepthMode)
            << "\n"
            << "  cull: "
            << get_smoke_world_render_cull_mode_name(contract.mCullMode)
            << " - "
            << get_world_render_cull_state_contract(contract.mCullMode)
            << "\n"
            << "  color mask: rgb="
            << (contract.mWriteColor ? "write" : "skip")
            << ", alpha="
            << (contract.mWriteAlpha ? "write" : "skip")
            << "\n"
            << "  polygon offset: "
            << (contract.mPolygonOffsetEnabled ? "enabled" : "disabled");
        if (contract.mPolygonOffsetEnabled)
        {
            std::cout
                << " factor="
                << contract.mPolygonOffsetFactor
                << " units="
                << contract.mPolygonOffsetUnits;
        }
        std::cout
            << "\n"
            << "  Vulkan color pipeline: "
            << get_world_render_color_pipeline_contract(
                contract.mWriteColor,
                contract.mWriteAlpha)
            << "\n";
    }

    return true;
}

void print_shader_probe_cases()
{
    std::cout << "Available mare-vulkan-smoke shader cases:\n";
    for (const SmokeWorldPipelineEntry& entry : make_world_pipeline_probe_entries())
    {
        std::cout << "  " << entry.mName << "\n";
    }
}

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
    bool mPolygonOffsetEnabled = false;
    F32 mPolygonOffsetFactor = 0.f;
    F32 mPolygonOffsetUnits = 0.f;
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
            else if (key == "polygon_offset" && stream >> value)
            {
                command.mPolygonOffsetEnabled = value != 0;
            }
            else if (key == "polygon_offset_factor" && stream >> command.mPolygonOffsetFactor)
            {
            }
            else if (key == "polygon_offset_units" && stream >> command.mPolygonOffsetUnits)
            {
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
    case LLWorldRenderMaterialClass::WaterExclusionSurface:
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
    if (command.mBlendMode == LLWorldRenderBlendMode::Alpha ||
        command.mBlendMode == LLWorldRenderBlendMode::ForwardAlpha)
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
    if (command.mMaterialClass == LLWorldRenderMaterialClass::WaterExclusionMask ||
        command.mMaterialClass == LLWorldRenderMaterialClass::WaterExclusionSurface)
    {
        flags |= LLRenderWorldMaterialParameters::WaterExclusionMask;
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
    case LLWorldRenderMaterialClass::WaterExclusionSurface:
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
                LLRenderBlendFactor::One,
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
                LLRenderBlendFactor::SourceAlpha,
                LLRenderBlendFactor::OneMinusSourceAlpha,
            });
    }
    else if (command.mBlendMode == LLWorldRenderBlendMode::ForwardAlpha)
    {
        backend.setBlendState(
            {
                LLRenderBlendFactor::SourceAlpha,
                LLRenderBlendFactor::OneMinusSourceAlpha,
                LLRenderBlendFactor::Zero,
                LLRenderBlendFactor::OneMinusSourceAlpha,
            });
    }
    else if (command.mBlendMode == LLWorldRenderBlendMode::MultiplyX2)
    {
        backend.setBlendState(
            {
                LLRenderBlendFactor::DestinationColor,
                LLRenderBlendFactor::SourceColor,
                LLRenderBlendFactor::DestinationColor,
                LLRenderBlendFactor::SourceColor,
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
    backend.setCapability(
        LLRenderCapability::PolygonOffsetFill,
        command.mPolygonOffsetEnabled);
    backend.setPolygonOffset(
        command.mPolygonOffsetEnabled ? command.mPolygonOffsetFactor : 0.f,
        command.mPolygonOffsetEnabled ? command.mPolygonOffsetUnits : 0.f);
    backend.setAlphaMaskCutoff(command.mAlphaMaskCutoff);
}

bool get_smoke_world_pipeline_entry_material_class(
    const SmokeWorldPipelineEntry& entry,
    LLWorldRenderMaterialClass& material_class)
{
    if (entry.mHasMaterialClass)
    {
        material_class = entry.mMaterialClass;
        return true;
    }

    switch (entry.mShaderClass)
    {
        case LLRenderWorldShaderClass::Sky:
            material_class = LLWorldRenderMaterialClass::Sky;
            return true;
        case LLRenderWorldShaderClass::Terrain:
            material_class = LLWorldRenderMaterialClass::Terrain;
            return true;
        case LLRenderWorldShaderClass::Textured:
            material_class = LLWorldRenderMaterialClass::SimpleOpaque;
            return true;
        case LLRenderWorldShaderClass::AlphaMask:
            material_class = LLWorldRenderMaterialClass::AlphaMask;
            return true;
        case LLRenderWorldShaderClass::Fullbright:
            material_class = LLWorldRenderMaterialClass::Fullbright;
            return true;
        case LLRenderWorldShaderClass::Material:
            material_class = LLWorldRenderMaterialClass::LegacyMaterial;
            return true;
        case LLRenderWorldShaderClass::PBR:
            material_class = LLWorldRenderMaterialClass::GLTFPBR;
            return true;
        case LLRenderWorldShaderClass::Avatar:
            material_class = LLWorldRenderMaterialClass::Avatar;
            return true;
        case LLRenderWorldShaderClass::Water:
            material_class = LLWorldRenderMaterialClass::Water;
            return true;
        case LLRenderWorldShaderClass::Haze:
            material_class = LLWorldRenderMaterialClass::AtmosphericHaze;
            return true;
        case LLRenderWorldShaderClass::Alpha:
            material_class = LLWorldRenderMaterialClass::Alpha;
            return true;
        case LLRenderWorldShaderClass::Glow:
            material_class = LLWorldRenderMaterialClass::Glow;
            return true;
        default:
            return false;
    }
}

F32 get_smoke_world_pipeline_entry_alpha_cutoff(
    LLWorldRenderMaterialClass material_class)
{
    switch (material_class)
    {
        case LLWorldRenderMaterialClass::AlphaMask:
        case LLWorldRenderMaterialClass::Grass:
        case LLWorldRenderMaterialClass::Tree:
        case LLWorldRenderMaterialClass::FullbrightAlphaMask:
        case LLWorldRenderMaterialClass::GLTFPBRAlphaMask:
        case LLWorldRenderMaterialClass::Avatar:
        case LLWorldRenderMaterialClass::AvatarImpostor:
            return 0.5f;
        default:
            return -1.f;
    }
}

void apply_smoke_world_pipeline_entry_state(
    LLRenderBackend& backend,
    const SmokeWorldPipelineEntry& entry)
{
    LLWorldRenderMaterialClass material_class =
        LLWorldRenderMaterialClass::SimpleOpaque;
    if (!get_smoke_world_pipeline_entry_material_class(entry, material_class))
    {
        backend.setColorMask({ true, true, true, true });
        backend.setCapability(LLRenderCapability::Blend, false);
        backend.setCapability(LLRenderCapability::DepthTest, false);
        backend.setDepthWriteEnabled(false);
        backend.setCapability(LLRenderCapability::CullFace, false);
        backend.setCapability(LLRenderCapability::PolygonOffsetFill, false);
        backend.setPolygonOffset(0.f, 0.f);
        backend.setAlphaMaskCutoff(-1.f);
        return;
    }

    const LLWorldRenderPipelineContract contract =
        get_smoke_world_render_pipeline_contract(material_class);
    backend.setColorMask(
        {
            contract.mWriteColor,
            contract.mWriteColor,
            contract.mWriteColor,
            contract.mWriteAlpha
        });
    backend.setCapability(
        LLRenderCapability::Blend,
        contract.mBlendMode != LLWorldRenderBlendMode::None);
    if (contract.mBlendMode == LLWorldRenderBlendMode::Add)
    {
        backend.setBlendState(
            {
                LLRenderBlendFactor::One,
                LLRenderBlendFactor::One,
                LLRenderBlendFactor::One,
                LLRenderBlendFactor::One,
            });
    }
    else if (contract.mBlendMode == LLWorldRenderBlendMode::Haze)
    {
        backend.setBlendState(
            {
                LLRenderBlendFactor::One,
                LLRenderBlendFactor::SourceAlpha,
                LLRenderBlendFactor::Zero,
                LLRenderBlendFactor::SourceAlpha,
            });
    }
    else if (contract.mBlendMode == LLWorldRenderBlendMode::Alpha)
    {
        backend.setBlendState(
            {
                LLRenderBlendFactor::SourceAlpha,
                LLRenderBlendFactor::OneMinusSourceAlpha,
                LLRenderBlendFactor::SourceAlpha,
                LLRenderBlendFactor::OneMinusSourceAlpha,
            });
    }
    else if (contract.mBlendMode == LLWorldRenderBlendMode::ForwardAlpha)
    {
        backend.setBlendState(
            {
                LLRenderBlendFactor::SourceAlpha,
                LLRenderBlendFactor::OneMinusSourceAlpha,
                LLRenderBlendFactor::Zero,
                LLRenderBlendFactor::OneMinusSourceAlpha,
            });
    }
    else if (contract.mBlendMode == LLWorldRenderBlendMode::MultiplyX2)
    {
        backend.setBlendState(
            {
                LLRenderBlendFactor::DestinationColor,
                LLRenderBlendFactor::SourceColor,
                LLRenderBlendFactor::DestinationColor,
                LLRenderBlendFactor::SourceColor,
            });
    }

    const bool depth_enabled =
        contract.mDepthMode != LLWorldRenderDepthMode::Disabled;
    backend.setCapability(LLRenderCapability::DepthTest, depth_enabled);
    backend.setDepthFunction(LLRenderDepthFunction::LessEqual);
    backend.setDepthWriteEnabled(contract.mDepthMode == LLWorldRenderDepthMode::ReadWrite);
    backend.setCapability(
        LLRenderCapability::CullFace,
        contract.mCullMode == LLWorldRenderCullMode::Back);
    if (contract.mCullMode == LLWorldRenderCullMode::Back)
    {
        backend.setCullFace(LLRenderCullFace::Back);
    }
    backend.setCapability(
        LLRenderCapability::PolygonOffsetFill,
        contract.mPolygonOffsetEnabled);
    backend.setPolygonOffset(
        contract.mPolygonOffsetEnabled ? contract.mPolygonOffsetFactor : 0.f,
        contract.mPolygonOffsetEnabled ? contract.mPolygonOffsetUnits : 0.f);
    backend.setAlphaMaskCutoff(
        get_smoke_world_pipeline_entry_alpha_cutoff(material_class));
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

    const std::array<SmokeWorldPipelineEntry, 17> entries =
        make_world_pipeline_probe_entries();

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
        apply_smoke_world_pipeline_entry_state(backend, entry);
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
    backend.setAlphaMaskCutoff(-1.f);
    backend.setColorMask({ true, true, true, true });
    backend.setScissor(0, 0, static_cast<S32>(width), static_cast<S32>(height));
    return true;
}

bool render_shader_probe_entry_frame(
    LLRenderBackend& backend,
    SmokeDeferredTextures& textures,
    const SmokeQuad& quad,
    const SmokeWorldPipelineEntry& entry,
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
    backend.setClearColor(0.015f, 0.018f, 0.024f, 1.f);
    backend.clear(LL_RENDER_CLEAR_COLOR | LL_RENDER_CLEAR_DEPTH);
    backend.setCapability(LLRenderCapability::DepthTest, false);
    backend.setDepthWriteEnabled(false);
    backend.setCapability(LLRenderCapability::CullFace, false);
    backend.setColorMask({ true, true, true, true });

    bind_world_pipeline_smoke_textures(backend, textures);
    bind_world_smoke_quad(backend, quad);
    backend.setWorldTextureTransform({});
    backend.setWorldTerrainParameters(make_world_pipeline_terrain_parameters());
    backend.setWorldSkinningMatrixPalette(0, nullptr);
    backend.setWorldDrawEnabled(true);
    backend.setWorldShaderClass(entry.mShaderClass);
    backend.setWorldMaterialParameters(
        make_world_pipeline_material(
            entry.mRed,
            entry.mGreen,
            entry.mBlue,
            entry.mAlphaBlend ? 0.72f : 1.f,
            entry.mFlags));
    apply_smoke_world_pipeline_entry_state(backend, entry);
    backend.drawArrays(LLRenderPrimitiveType::Triangles, 0, 6);

    backend.setCapability(LLRenderCapability::Blend, false);
    backend.setWorldDrawEnabled(false);
    backend.setWorldShaderClass(LLRenderWorldShaderClass::Textured);
    backend.setWorldMaterialParameters({});
    backend.setWorldTerrainParameters({});
    backend.setWorldTextureTransform({});
    backend.setWorldSkinningMatrixPalette(0, nullptr);
    backend.setAlphaMaskCutoff(-1.f);
    backend.setColorMask({ true, true, true, true });
    backend.setScissor(0, 0, static_cast<S32>(width), static_cast<S32>(height));
    return true;
}

bool render_shader_probe_frame(
    LLRenderBackend& backend,
    SmokeDeferredTextures& textures,
    const SmokeQuad& quad,
    const std::string& shader_case,
    U32 width,
    U32 height)
{
    SmokeWorldPipelineEntry entry;
    if (!find_shader_probe_case(shader_case, entry))
    {
        std::cerr
            << "Unknown shader-probe case '"
            << shader_case
            << "'. Use --list-shader-cases.\n";
        return false;
    }

    static std::string logged_case;
    if (logged_case != entry.mName)
    {
        std::cout
            << "Mare Vulkan smoke shader-probe case: "
            << entry.mName
            << ". This tests one runtime Vulkan shader pipeline with fixed synthetic inputs."
            << std::endl;
        logged_case = entry.mName;
    }

    return render_shader_probe_entry_frame(
        backend,
        textures,
        quad,
        entry,
        width,
        height);
}

bool render_shader_suite_frame(
    LLRenderBackend& backend,
    SmokeDeferredTextures& textures,
    const SmokeQuad& quad,
    int frame,
    U32 width,
    U32 height)
{
    const auto entries = make_world_pipeline_probe_entries();
    const size_t entry_index =
        static_cast<size_t>(frame) % entries.size();
    const SmokeWorldPipelineEntry& entry = entries[entry_index];

    if (frame == 0)
    {
        std::cout
            << "Mare Vulkan smoke shader-suite: "
            << entries.size()
            << " runtime shader cases; one fullscreen case per frame. "
            << "Use --frames "
            << entries.size()
            << " for one full pass."
            << std::endl;
    }
    if (frame < static_cast<int>(entries.size()))
    {
        std::cout
            << "Mare Vulkan smoke shader-suite frame "
            << frame
            << " case "
            << entry_index
            << ": "
            << entry.mName
            << std::endl;
    }

    return render_shader_probe_entry_frame(
        backend,
        textures,
        quad,
        entry,
        width,
        height);
}

struct SmokeDeferredGraph
{
    LLRenderTextureHandle mGBufferColor;
    LLRenderTextureHandle mGBufferSpecular;
    LLRenderTextureHandle mGBufferNormal;
    LLRenderTextureHandle mGBufferEmissive;
    LLRenderTextureHandle mGBufferDepth;
    LLRenderTextureHandle mDeferredColor;
    LLRenderTextureHandle mExposureMap;
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
    LLRenderTarget mExposureMap;
    LLRenderTextureHandle mLightMapSource;
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
        &graph.mExposureMap,
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
    LLRenderBackend& backend,
    SmokeViewerRenderTargetGraph& graph)
{
    if (graph.mLightMapSource)
    {
        backend.deleteTextureHandle(graph.mLightMapSource);
        graph.mLightMapSource = {};
    }
    graph.mExposureMap.release();
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
    LLRenderBackend& backend,
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
        graph.mExposureMap.isComplete() &&
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

    release_smoke_viewer_render_target_graph(backend, graph);
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
        release_smoke_viewer_render_target_graph(backend, graph);
        return false;
    }

    if (!graph.mDeferredScreen.addColorAttachment(LLRenderTextureFormat::RGBA) ||
        !graph.mDeferredScreen.addColorAttachment(LLRenderTextureFormat::RGBA16))
    {
        release_smoke_viewer_render_target_graph(backend, graph);
        return false;
    }

    if (color_attachment_count >= 4U &&
        !graph.mDeferredScreen.addColorAttachment(LLRenderTextureFormat::RGB16F))
    {
        release_smoke_viewer_render_target_graph(backend, graph);
        return false;
    }

    if (staged_post_targets)
    {
        if (!graph.mDeferredLight.allocate(width, height, LLRenderTextureFormat::RGBA16F) ||
            !graph.mScreen.allocate(width, height, LLRenderTextureFormat::RGBA16F) ||
            !graph.mPostPing.allocate(width, height, LLRenderTextureFormat::RGBA))
        {
            release_smoke_viewer_render_target_graph(backend, graph);
            return false;
        }

        graph.mDeferredScreen.shareDepthBuffer(graph.mScreen);
    }

    if (!graph.mExposureMap.allocate(1, 1, LLRenderTextureFormat::RGBA))
    {
        release_smoke_viewer_render_target_graph(backend, graph);
        return false;
    }
    graph.mExposureMap.bindTarget();
    backend.setViewport(0, 0, 1, 1);
    backend.setScissor(0, 0, 1, 1);
    backend.setClearColor(1.f, 1.f, 1.f, 1.f);
    graph.mExposureMap.clear(LL_RENDER_CLEAR_COLOR);
    graph.mExposureMap.flush();

    return graph.mDeferredScreen.isComplete() &&
        graph.mExposureMap.isComplete() &&
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
        graph.mExposureMap &&
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
            LLRenderPixelType::Float32) ||
        !create_smoke_texture(
            backend,
            graph.mExposureMap,
            1,
            1,
            make_solid_rgba_pixels(1, 1, 255, 255, 255, 255)))
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
        graph.mExposureMap,
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

struct SmokeRGB
{
    F32 mRed = 0.f;
    F32 mGreen = 0.f;
    F32 mBlue = 0.f;
};

F32 smoke_linear_to_srgb_component(F32 value)
{
    value = llclamp(value, 0.f, 1.f);
    if (value < 0.0031308f)
    {
        return value * 12.92f;
    }
    return 1.055f * std::pow(value, 0.41666f) - 0.055f;
}

F32 smoke_srgb_to_linear_component(F32 value)
{
    value = llclamp(value, 0.f, 1.f);
    if (value <= 0.04045f)
    {
        return value / 12.92f;
    }
    return std::pow((value + 0.055f) / 1.055f, 2.4f);
}

SmokeRGB smoke_linear_to_srgb(SmokeRGB color)
{
    return
    {
        smoke_linear_to_srgb_component(color.mRed),
        smoke_linear_to_srgb_component(color.mGreen),
        smoke_linear_to_srgb_component(color.mBlue)
    };
}

SmokeRGB smoke_srgb_to_linear(SmokeRGB color)
{
    return
    {
        smoke_srgb_to_linear_component(color.mRed),
        smoke_srgb_to_linear_component(color.mGreen),
        smoke_srgb_to_linear_component(color.mBlue)
    };
}

SmokeRGB get_deferred_color_compare_srgb_input()
{
    return { 0.50f, 0.32f, 0.18f };
}

SmokeRGB get_deferred_color_compare_material_srgb_input()
{
    return { 0.68f, 0.45f, 0.82f };
}

SmokeRGB get_deferred_color_compare_pbr_srgb_input()
{
    return { 0.90f, 0.72f, 0.48f };
}

LLRenderWorldMaterialParameters make_final_color_compare_parameters()
{
    LLVulkanFinalCompositeSettings settings;
    settings.mNoPost = false;
    settings.mExposure = 1.f;
    settings.mGamma = 2.2f;
    settings.mTonemapMix = 0.f;
    settings.mTonemapType = 0;
    settings.mCASSharpness = 0.f;
    settings.mRenderGlow = false;
    settings.mDeferredAttachmentCount = 0;
    return make_vulkan_final_composite_material_parameters(settings);
}

void log_final_color_compare_reference(SmokeRGB linear_input)
{
    static bool logged_reference = false;
    if (logged_reference)
    {
        return;
    }

    const SmokeRGB opengl_reference = smoke_linear_to_srgb(linear_input);
    std::cout
        << "Mare Vulkan final-color-compare OpenGL-style reference: linear input "
        << std::fixed
        << std::setprecision(4)
        << linear_input.mRed
        << ", "
        << linear_input.mGreen
        << ", "
        << linear_input.mBlue
        << " should read back approximately sRGB "
        << opengl_reference.mRed
        << ", "
        << opengl_reference.mGreen
        << ", "
        << opengl_reference.mBlue
        << " rgb8 "
        << to_color_byte(opengl_reference.mRed)
        << ","
        << to_color_byte(opengl_reference.mGreen)
        << ","
        << to_color_byte(opengl_reference.mBlue)
        << ". If Vulkan readback is much brighter, suspect double sRGB/gamma conversion."
        << std::endl;
    logged_reference = true;
}

void log_deferred_color_compare_reference()
{
    static bool logged_reference = false;
    if (logged_reference)
    {
        return;
    }

    const SmokeRGB srgb_input = get_deferred_color_compare_srgb_input();
    const SmokeRGB material_input =
        get_deferred_color_compare_material_srgb_input();
    const SmokeRGB pbr_input =
        get_deferred_color_compare_pbr_srgb_input();
    const SmokeRGB opengl_linear = smoke_srgb_to_linear(srgb_input);
    const SmokeRGB opengl_final = smoke_linear_to_srgb(opengl_linear);
    const SmokeRGB missing_conversion_final = smoke_linear_to_srgb(srgb_input);
    const SmokeRGB material_linear = smoke_srgb_to_linear(material_input);
    const SmokeRGB pbr_linear = smoke_srgb_to_linear(pbr_input);

    std::cout
        << "Mare Vulkan viewer-deferred-color-compare OpenGL-style reference: "
        << "legacy G-buffer sRGB input "
        << std::fixed
        << std::setprecision(4)
        << srgb_input.mRed
        << ", "
        << srgb_input.mGreen
        << ", "
        << srgb_input.mBlue
        << " converts to linear "
        << opengl_linear.mRed
        << ", "
        << opengl_linear.mGreen
        << ", "
        << opengl_linear.mBlue
        << " and should read back near final sRGB "
        << opengl_final.mRed
        << ", "
        << opengl_final.mGreen
        << ", "
        << opengl_final.mBlue
        << " rgb8 "
        << to_color_byte(opengl_final.mRed)
        << ","
        << to_color_byte(opengl_final.mGreen)
        << ","
        << to_color_byte(opengl_final.mBlue)
        << ". If the deferred pass skips the legacy sRGB-to-linear conversion, "
        << "the final readback drifts toward pale sRGB "
        << missing_conversion_final.mRed
        << ", "
        << missing_conversion_final.mGreen
        << ", "
        << missing_conversion_final.mBlue
        << " rgb8 "
        << to_color_byte(missing_conversion_final.mRed)
        << ","
        << to_color_byte(missing_conversion_final.mGreen)
        << ","
        << to_color_byte(missing_conversion_final.mBlue)
        << "."
        << " The synthetic scene is split into vertical bands: left legacy diffuse, "
        << "middle legacy material, right PBR. Middle legacy material sRGB "
        << material_input.mRed
        << ", "
        << material_input.mGreen
        << ", "
        << material_input.mBlue
        << " converts to linear "
        << material_linear.mRed
        << ", "
        << material_linear.mGreen
        << ", "
        << material_linear.mBlue
        << "; right PBR sRGB "
        << pbr_input.mRed
        << ", "
        << pbr_input.mGreen
        << ", "
        << pbr_input.mBlue
        << " is written linear "
        << pbr_linear.mRed
        << ", "
        << pbr_linear.mGreen
        << ", "
        << pbr_linear.mBlue
        << " by the PBR G-buffer shader."
        << std::endl;
    logged_reference = true;
}

void log_deferred_soften_state_probe_reference()
{
    static bool logged_reference = false;
    if (logged_reference)
    {
        return;
    }

    std::cout
        << "Mare Vulkan viewer-deferred-soften-state-probe: using the same "
        << "three-band synthetic G-buffer as viewer-deferred-color-compare, "
        << "then executing the real DeferredSoften shader with a synthetic "
        << "lightMap, fallback reflection-probe cube arrays, moon-selected "
        << "light, classic mode enabled, direct light enabled, sky HDR scale "
        << "> 1, and non-identity environment/SSAO matrices. This is a "
        << "guardrail for the live DeferredSoften owner and final composite "
        << "handoff."
        << std::endl;
    logged_reference = true;
}

void log_deferred_emissive_probe_reference()
{
    static bool logged_reference = false;
    if (logged_reference)
    {
        return;
    }

    std::cout
        << "Mare Vulkan viewer-deferred-emissive-probe: using the same "
        << "three-band synthetic G-buffer as viewer-deferred-color-compare, "
        << "but the PBR band writes a bright emissive value into attachment 3. "
        << "This validates the G-buffer emissive variant, DeferredSoften "
        << "emissiveMap binding, and final composite handoff."
        << std::endl;
    logged_reference = true;
}

void log_deferred_reflection_probe_reference()
{
    static bool logged_reference = false;
    if (logged_reference)
    {
        return;
    }

    std::cout
        << "Mare Vulkan viewer-deferred-reflection-probe: using the same "
        << "three-band synthetic G-buffer as viewer-deferred-color-compare, "
        << "but the PBR band is glossy and metallic so DeferredSoften must use "
        << "its environment/probe, BRDF LUT, and scene reflection parameters. "
        << "This validates the DeferredSoften reflection input bindings and "
        << "final composite handoff without requiring a live region."
        << std::endl;
    logged_reference = true;
}

void log_deferred_real_reflection_probe_reference()
{
    static bool logged_reference = false;
    if (logged_reference)
    {
        return;
    }

    std::cout
        << "Mare Vulkan viewer-deferred-real-reflection-probe: using the same "
        << "glossy metallic PBR band as viewer-deferred-reflection-probe, "
        << "but binding a synthetic ReflectionProbes UBO plus real Vulkan "
        << "TextureCubeMapArray radiance and irradiance inputs. This validates "
        << "the DeferredSoften cube-array/probe descriptor path without "
        << "requiring a live region."
        << std::endl;
    logged_reference = true;
}

void log_deferred_hero_probe_reference()
{
    static bool logged_reference = false;
    if (logged_reference)
    {
        return;
    }

    std::cout
        << "Mare Vulkan viewer-deferred-hero-probe: using a synthetic "
        << "ReflectionProbes UBO with heroProbeCount=1 and a distinct hero "
        << "TextureCubeMapArray color. This validates the OpenGL-style "
        << "high-gloss hero probe mix inside DeferredSoften without requiring "
        << "a live hero-probe render pass."
        << std::endl;
    logged_reference = true;
}

void log_deferred_ssr_probe_reference()
{
    static bool logged_reference = false;
    if (logged_reference)
    {
        return;
    }

    std::cout
        << "Mare Vulkan viewer-deferred-ssr-probe: using a glossy metallic "
        << "PBR G-buffer band with a camera-facing normal, real reflection "
        << "probe inputs, and deterministic sceneMap/sceneDepthMap textures. "
        << "This validates that DeferredSoften can mix screen-space "
        << "reflections through the OpenGL-derived SSR path without requiring "
        << "a live region or moving camera."
        << std::endl;
    logged_reference = true;
}

void log_terrain_final_probe_reference()
{
    static bool logged_reference = false;
    if (logged_reference)
    {
        return;
    }

    std::cout
        << "Mare Vulkan terrain-final-probe: rendering a terrain-only "
        << "viewer-style deferred graph. Texture slots are bound like the "
        << "runtime terrain path: base-color 0..3, composition 4, ORM 5..8, "
        << "emissive 9..12, and normal 13..16. This isolates terrain G-buffer "
        << "and final-composite behavior from other world draw pools."
        << std::endl;
    logged_reference = true;
}

LLRenderWorldMaterialParameters make_deferred_color_compare_gbuffer_material()
{
    const SmokeRGB srgb_input = get_deferred_color_compare_srgb_input();

    LLRenderWorldMaterialParameters parameters;
    parameters.mBaseColorRed = srgb_input.mRed;
    parameters.mBaseColorGreen = srgb_input.mGreen;
    parameters.mBaseColorBlue = srgb_input.mBlue;
    parameters.mBaseColorAlpha = 1.f;
    parameters.mSpecularColorRed = 0.f;
    parameters.mSpecularColorGreen = 0.f;
    parameters.mSpecularColorBlue = 0.f;
    parameters.mEnvIntensity = 0.f;
    parameters.mDiffuseAlphaMode = 0.f;
    parameters.mGLTFAlphaMode = 0.f;
    parameters.mShiny = 0.f;
    return parameters;
}

LLRenderWorldMaterialParameters make_deferred_color_compare_legacy_material()
{
    const SmokeRGB srgb_input =
        get_deferred_color_compare_material_srgb_input();

    LLRenderWorldMaterialParameters parameters;
    parameters.mBaseColorRed = srgb_input.mRed;
    parameters.mBaseColorGreen = srgb_input.mGreen;
    parameters.mBaseColorBlue = srgb_input.mBlue;
    parameters.mBaseColorAlpha = 1.f;
    parameters.mSpecularColorRed = 0.24f;
    parameters.mSpecularColorGreen = 0.22f;
    parameters.mSpecularColorBlue = 0.18f;
    parameters.mEnvIntensity = 0.15f;
    parameters.mRoughnessFactor = 0.72f;
    parameters.mMetallicFactor = 0.f;
    parameters.mMaterialFlags =
        static_cast<F32>(LLRenderWorldMaterialParameters::LegacyShiny);
    parameters.mDiffuseAlphaMode = 0.f;
    parameters.mGLTFAlphaMode = 0.f;
    parameters.mShiny = 3.f;
    return parameters;
}

LLRenderWorldMaterialParameters make_deferred_color_compare_pbr_material()
{
    const SmokeRGB srgb_input =
        get_deferred_color_compare_pbr_srgb_input();

    LLRenderWorldMaterialParameters parameters;
    parameters.mBaseColorRed = srgb_input.mRed;
    parameters.mBaseColorGreen = srgb_input.mGreen;
    parameters.mBaseColorBlue = srgb_input.mBlue;
    parameters.mBaseColorAlpha = 1.f;
    parameters.mSpecularColorRed = 0.04f;
    parameters.mSpecularColorGreen = 0.04f;
    parameters.mSpecularColorBlue = 0.04f;
    parameters.mEnvIntensity = 0.30f;
    parameters.mRoughnessFactor = 0.58f;
    parameters.mMetallicFactor = 0.18f;
    parameters.mMaterialFlags =
        static_cast<F32>(
            LLRenderWorldMaterialParameters::GLTFPBR |
            LLRenderWorldMaterialParameters::HasNormalMap);
    parameters.mHasORMMap = 0.f;
    parameters.mHasEmissiveMap = 0.f;
    parameters.mDiffuseAlphaMode = 0.f;
    parameters.mGLTFAlphaMode = 0.f;
    return parameters;
}

LLRenderWorldMaterialParameters make_deferred_color_compare_pbr_emissive_material()
{
    LLRenderWorldMaterialParameters parameters =
        make_deferred_color_compare_pbr_material();
    parameters.mEmissiveColorRed = 0.25f;
    parameters.mEmissiveColorGreen = 1.f;
    parameters.mEmissiveColorBlue = 0.35f;
    parameters.mHasEmissiveMap = 0.f;
    return parameters;
}

LLRenderWorldMaterialParameters make_deferred_color_compare_pbr_reflective_material()
{
    LLRenderWorldMaterialParameters parameters =
        make_deferred_color_compare_pbr_material();
    parameters.mBaseColorRed = 0.78f;
    parameters.mBaseColorGreen = 0.82f;
    parameters.mBaseColorBlue = 0.92f;
    parameters.mSpecularColorRed = 0.9f;
    parameters.mSpecularColorGreen = 0.9f;
    parameters.mSpecularColorBlue = 0.9f;
    parameters.mEnvIntensity = 1.f;
    parameters.mRoughnessFactor = 0.08f;
    parameters.mMetallicFactor = 1.f;
    parameters.mMaterialFlags =
        static_cast<F32>(LLRenderWorldMaterialParameters::GLTFPBR);
    parameters.mHasORMMap = 0.f;
    parameters.mHasEmissiveMap = 0.f;
    return parameters;
}

LLRenderWorldMaterialParameters make_deferred_color_compare_composite_parameters(
    U32 width,
    U32 height)
{
    LLVulkanDeferredCompositeSettings settings;
    settings.mAmbientRed = 1.f;
    settings.mAmbientGreen = 1.f;
    settings.mAmbientBlue = 1.f;
    settings.mDirectLightRed = 0.f;
    settings.mDirectLightGreen = 0.f;
    settings.mDirectLightBlue = 0.f;
    settings.mLightDirectionX = 0.f;
    settings.mLightDirectionY = 0.f;
    settings.mLightDirectionZ = 1.f;
    settings.mDirectLightScale = 0.f;
    settings.mDeferredAttachmentCount = 4;
    settings.mDominantLightScreenX = -1.f;
    settings.mDominantLightScreenY = 1.f;
    settings.mDominantLightRadius = 0.f;
    settings.mReflectionProbeAmbiance = 0.f;
    settings.mTonemapMix = 0.f;
    settings.mSkyLightingValid = 0.f;
    settings.mScreenWidth = static_cast<F32>(llmax(1U, width));
    settings.mScreenHeight = static_cast<F32>(llmax(1U, height));
    settings.mSunDirectionX = 0.35f;
    settings.mSunDirectionY = 0.45f;
    settings.mSunDirectionZ = 0.82f;
    settings.mSunUpFactor = 1.f;
    settings.mMoonDirectionX = -0.25f;
    settings.mMoonDirectionY = -0.15f;
    settings.mMoonDirectionZ = 0.95f;
    settings.mClassicMode = 0.f;
    settings.mCubeSnapshot = 0.f;
    settings.mSkyHDRScale = 1.f;
    settings.mBlurSize = 1.4f;
    settings.mBlurFidelity = 4.f;
    settings.mSSAOIrradianceScale = 0.6f;
    settings.mSSAOIrradianceMax = 0.18f;
    return make_vulkan_deferred_composite_material_parameters(settings);
}

LLRenderWorldMaterialParameters make_deferred_soften_state_probe_parameters(
    U32 width,
    U32 height)
{
    LLVulkanDeferredCompositeSettings settings;
    settings.mAmbientRed = 0.32f;
    settings.mAmbientGreen = 0.34f;
    settings.mAmbientBlue = 0.38f;
    settings.mDirectLightRed = 0.88f;
    settings.mDirectLightGreen = 0.78f;
    settings.mDirectLightBlue = 0.58f;
    settings.mLightDirectionX = 0.f;
    settings.mLightDirectionY = 0.f;
    settings.mLightDirectionZ = 1.f;
    settings.mDirectLightScale = 1.f;
    settings.mDeferredAttachmentCount = 4;
    settings.mSSAOEnabled = true;
    settings.mSSAOScale = 3.f;
    settings.mSSAOMaxScale = 8.f;
    settings.mSSAOFactor = 1.5f;
    settings.mSSAOEffect = 1.f;
    settings.mDominantLightScreenX = -1.f;
    settings.mDominantLightScreenY = 1.f;
    settings.mDominantLightRadius = 0.f;
    settings.mReflectionProbeAmbiance = 0.35f;
    settings.mReflectionInputsValid = true;
    settings.mTonemapMix = 0.25f;
    settings.mSkyLightingValid = 1.f;
    settings.mScreenWidth = static_cast<F32>(llmax(1U, width));
    settings.mScreenHeight = static_cast<F32>(llmax(1U, height));
    settings.mSunDirectionX = -0.75f;
    settings.mSunDirectionY = 0.10f;
    settings.mSunDirectionZ = 0.35f;
    settings.mSunUpFactor = 0.f;
    settings.mMoonDirectionX = 0.f;
    settings.mMoonDirectionY = 0.f;
    settings.mMoonDirectionZ = 1.f;
    settings.mClassicMode = 1.f;
    settings.mCubeSnapshot = 0.f;
    settings.mSkyHDRScale = 1.45f;
    settings.mBlurSize = 2.25f;
    settings.mBlurFidelity = 6.f;
    settings.mSSAOIrradianceScale = 0.45f;
    settings.mSSAOIrradianceMax = 0.22f;
    settings.mEnvironmentMatrix[0] = 1.f;
    settings.mEnvironmentMatrix[1] = 0.f;
    settings.mEnvironmentMatrix[2] = 0.f;
    settings.mEnvironmentMatrix[3] = 0.f;
    settings.mEnvironmentMatrix[4] = 0.72f;
    settings.mEnvironmentMatrix[5] = 0.69f;
    settings.mEnvironmentMatrix[6] = 0.f;
    settings.mEnvironmentMatrix[7] = -0.69f;
    settings.mEnvironmentMatrix[8] = 0.72f;
    settings.mSSAOEffectMatrix[0] = 0.72f;
    settings.mSSAOEffectMatrix[1] = 0.08f;
    settings.mSSAOEffectMatrix[2] = 0.05f;
    settings.mSSAOEffectMatrix[3] = 0.08f;
    settings.mSSAOEffectMatrix[4] = 0.64f;
    settings.mSSAOEffectMatrix[5] = 0.05f;
    settings.mSSAOEffectMatrix[6] = 0.05f;
    settings.mSSAOEffectMatrix[7] = 0.05f;
    settings.mSSAOEffectMatrix[8] = 0.58f;
    return make_vulkan_deferred_composite_material_parameters(settings);
}

LLRenderWorldMaterialParameters make_deferred_local_light_probe_parameters(
    U32 width,
    U32 height)
{
    LLRenderWorldMaterialParameters parameters;
    parameters.mLocalLightScreenSettings[0] =
        static_cast<F32>(llmax(1U, width));
    parameters.mLocalLightScreenSettings[1] =
        static_cast<F32>(llmax(1U, height));
    parameters.mLocalLightScreenSettings[2] = -1.f;
    parameters.mLocalLightScreenSettings[3] = 0.f;
    parameters.mLocalLightSunWashAndCount[0] = 0.f;
    parameters.mLocalLightSunWashAndCount[1] = 1.f;
    parameters.mLocalLightSunWashAndCount[2] = 0.f;
    parameters.mLocalLightSunWashAndCount[3] = 0.f;

    for (U32 i = 0; i < LLRenderWorldMaterialParameters::MaxDeferredMultiLightCount; ++i)
    {
        const U32 offset = i * 4;
        parameters.mLocalLight[offset] = 0.f;
        parameters.mLocalLight[offset + 1] = 0.f;
        parameters.mLocalLight[offset + 2] = 1.25f;
        parameters.mLocalLight[offset + 3] = 1.f;
        parameters.mLocalLightColor[offset] = 0.f;
        parameters.mLocalLightColor[offset + 1] = 0.f;
        parameters.mLocalLightColor[offset + 2] = 0.f;
        parameters.mLocalLightColor[offset + 3] = 0.f;
    }

    parameters.mLocalLight[0] = 0.f;
    parameters.mLocalLight[1] = 0.f;
    parameters.mLocalLight[2] = 1.25f;
    parameters.mLocalLight[3] = 4.f;
    parameters.mLocalLightColor[0] = 0.60f;
    parameters.mLocalLightColor[1] = 0.38f;
    parameters.mLocalLightColor[2] = 0.16f;
    parameters.mLocalLightColor[3] = 0.f;
    return parameters;
}

bool force_smoke_volume_light_output()
{
    return gForceSmokeVolumeLightOutput;
}

LLRenderWorldMaterialParameters make_deferred_projector_light_probe_parameters(
    U32 width,
    U32 height)
{
    LLRenderWorldMaterialParameters parameters;
    parameters.mCompositeInverseProjection[0] = 1.f;
    parameters.mLocalLightScreenSettings[0] =
        static_cast<F32>(llmax(1U, width));
    parameters.mLocalLightScreenSettings[1] =
        static_cast<F32>(llmax(1U, height));
    parameters.mLocalLightScreenSettings[2] = -1.f;
    parameters.mLocalLightScreenSettings[3] = 0.f;
    parameters.mLocalLightSunWashAndCount[3] =
        force_smoke_volume_light_output() ? 1.f : 0.f;
    parameters.mLocalLight[0] = 0.f;
    parameters.mLocalLight[1] = 0.f;
    parameters.mLocalLight[2] = 1.25f;
    parameters.mLocalLight[3] = 0.f;
    parameters.mLocalLightColor[0] = 0.18f;
    parameters.mLocalLightColor[1] = 0.46f;
    parameters.mLocalLightColor[2] = 0.70f;
    parameters.mLocalLightColor[3] = 0.f;
    parameters.mLocalLightProjectionMatrix[0] = 0.5f;
    parameters.mLocalLightProjectionMatrix[1] = 0.f;
    parameters.mLocalLightProjectionMatrix[2] = 0.f;
    parameters.mLocalLightProjectionMatrix[3] = 0.f;
    parameters.mLocalLightProjectionMatrix[4] = 0.f;
    parameters.mLocalLightProjectionMatrix[5] = 0.5f;
    parameters.mLocalLightProjectionMatrix[6] = 0.f;
    parameters.mLocalLightProjectionMatrix[7] = 0.f;
    parameters.mLocalLightProjectionMatrix[8] = 0.f;
    parameters.mLocalLightProjectionMatrix[9] = 0.f;
    parameters.mLocalLightProjectionMatrix[10] = 0.f;
    parameters.mLocalLightProjectionMatrix[11] = 0.f;
    parameters.mLocalLightProjectionMatrix[12] = 0.5f;
    parameters.mLocalLightProjectionMatrix[13] = 0.5f;
    parameters.mLocalLightProjectionMatrix[14] = 0.5f;
    parameters.mLocalLightProjectionMatrix[15] = 1.f;
    parameters.mLocalLightProjectionPAndNear[0] = 0.f;
    parameters.mLocalLightProjectionPAndNear[1] = 0.f;
    parameters.mLocalLightProjectionPAndNear[2] = 1.25f;
    parameters.mLocalLightProjectionPAndNear[3] = 0.f;
    parameters.mLocalLightProjectionNAndFocus[0] = 0.f;
    parameters.mLocalLightProjectionNAndFocus[1] = 0.f;
    parameters.mLocalLightProjectionNAndFocus[2] = -1.f;
    parameters.mLocalLightProjectionNAndFocus[3] = 0.f;
    parameters.mLocalLightProjectionLodRangeAmbiance[0] = 1.f;
    parameters.mLocalLightProjectionLodRangeAmbiance[1] = 4.f;
    parameters.mLocalLightProjectionLodRangeAmbiance[2] = 0.f;
    parameters.mLocalLightProjectionLodRangeAmbiance[3] = 0.18f;
    parameters.mLocalLightNearFarSunShadow[0] = 0.f;
    parameters.mLocalLightNearFarSunShadow[1] = 4.f;
    parameters.mLocalLightNearFarSunShadow[2] = 0.f;
    parameters.mLocalLightNearFarSunShadow[3] = 1.f;
    parameters.mLocalLightProjectionOriginSize[0] = 0.f;
    parameters.mLocalLightProjectionOriginSize[1] = 0.f;
    parameters.mLocalLightProjectionOriginSize[2] = 1.25f;
    parameters.mLocalLightProjectionOriginSize[3] = 4.f;
    return parameters;
}

LLRenderWorldMaterialParameters make_deferred_point_light_volume_probe_parameters(
    U32 width,
    U32 height)
{
    LLRenderWorldMaterialParameters parameters;
    parameters.mLocalLightScreenSettings[0] =
        static_cast<F32>(llmax(1U, width));
    parameters.mLocalLightScreenSettings[1] =
        static_cast<F32>(llmax(1U, height));
    parameters.mLocalLightScreenSettings[3] = 0.f;
    parameters.mLocalLightSunWashAndCount[3] =
        force_smoke_volume_light_output() ? 1.f : 0.f;
    parameters.mLocalLightCenterSize[0] = 0.f;
    parameters.mLocalLightCenterSize[1] = 0.f;
    parameters.mLocalLightCenterSize[2] = 0.5f;
    parameters.mLocalLightCenterSize[3] = 0.5f;
    parameters.mLocalLightColor[0] = 0.52f;
    parameters.mLocalLightColor[1] = 0.30f;
    parameters.mLocalLightColor[2] = 0.10f;
    parameters.mLocalLightColor[3] = 0.f;
    parameters.mLocalLightViewport[0] = 0.f;
    parameters.mLocalLightViewport[1] = 0.f;
    parameters.mLocalLightViewport[2] =
        static_cast<F32>(llmax(1U, width));
    parameters.mLocalLightViewport[3] =
        static_cast<F32>(llmax(1U, height));
    return parameters;
}

LLRenderWorldMaterialParameters make_deferred_spot_light_volume_probe_parameters(
    U32 width,
    U32 height)
{
    LLRenderWorldMaterialParameters parameters =
        make_deferred_projector_light_probe_parameters(width, height);
    parameters.mLocalLightCenterSize[0] = 0.f;
    parameters.mLocalLightCenterSize[1] = 0.f;
    parameters.mLocalLightCenterSize[2] = 0.5f;
    parameters.mLocalLightCenterSize[3] = 0.5f;
    parameters.mLocalLightColor[0] = 0.16f;
    parameters.mLocalLightColor[1] = 0.40f;
    parameters.mLocalLightColor[2] = 0.68f;
    parameters.mLocalLightColor[3] = 0.f;
    parameters.mLocalLightProjectionOriginSize[0] = 0.f;
    parameters.mLocalLightProjectionOriginSize[1] = 0.f;
    parameters.mLocalLightProjectionOriginSize[2] = 0.f;
    parameters.mLocalLightProjectionOriginSize[3] = 1.5f;
    return parameters;
}

void set_smoke_deferred_blur_parameters(
    LLRenderWorldMaterialParameters& parameters,
    const LLVector2& delta,
    U32 width,
    U32 height)
{
    constexpr U32 kern_length = 4;
    const LLVector3 gaussian(3.f, 2.f, 0.f);
    F32 x = 0.f;

    parameters.mCompositeBlurSettings[0] = delta.mV[VX];
    parameters.mCompositeBlurSettings[1] = delta.mV[VY];
    parameters.mCompositeBlurSettings[2] = 0.f;
    parameters.mCompositeBlurSettings[3] = 1.4f * (kern_length / 2.f - 0.5f);
    parameters.mCompositeBlurScreen[0] = static_cast<F32>(llmax(1U, width));
    parameters.mCompositeBlurScreen[1] = static_cast<F32>(llmax(1U, height));
    parameters.mCompositeBlurScreen[2] = 1.4f;
    parameters.mCompositeBlurScreen[3] = static_cast<F32>(kern_length);

    for (U32 i = 0; i < kern_length; ++i)
    {
        parameters.mCompositeBlurKernel[i * 4 + 0] =
            llgaussian(x, gaussian.mV[VX]);
        parameters.mCompositeBlurKernel[i * 4 + 1] =
            llgaussian(x, gaussian.mV[VY]);
        parameters.mCompositeBlurKernel[i * 4 + 2] = x;
        parameters.mCompositeBlurKernel[i * 4 + 3] = 0.f;
        x += 1.f;
    }
}

void log_deferred_local_light_probe_reference()
{
    static bool logged_reference = false;
    if (logged_reference)
    {
        return;
    }

    std::cout
        << "Mare Vulkan viewer-deferred-local-light-probe: using the "
        << "three-band synthetic G-buffer, then adding one fullscreen "
        << "MultiPointLight pass into the deferred light target before final "
        << "composite. This guards the separate local-light owner, blend state, "
        << "G-buffer/depth/lightFunc bindings, and final handoff."
        << std::endl;
    logged_reference = true;
}

void log_deferred_projector_light_probe_reference()
{
    static bool logged_reference = false;
    if (logged_reference)
    {
        return;
    }

    std::cout
        << "Mare Vulkan viewer-deferred-projector-light-probe: using the "
        << "three-band synthetic G-buffer, then adding one fullscreen "
        << "MultiSpotLight projector pass into the deferred light target before "
        << "final composite. This guards the separate projector owner, cube/"
        << "projection/noise/lightFunc bindings, additive blend state, and final "
        << "handoff."
        << std::endl;
    logged_reference = true;
}

void log_deferred_point_light_volume_probe_reference()
{
    static bool logged_reference = false;
    if (logged_reference)
    {
        return;
    }

    std::cout
        << "Mare Vulkan viewer-deferred-point-light-volume-probe: using the "
        << "three-band synthetic G-buffer, then adding one indexed cube "
        << "PointLight pass into the deferred light target before final "
        << "composite. This guards the outside-camera local light volume owner, "
        << "point_light.vert ABI, indexed TRIANGLE_FAN draw path, and final "
        << "handoff."
        << std::endl;
    logged_reference = true;
}

void log_deferred_spot_light_volume_probe_reference()
{
    static bool logged_reference = false;
    if (logged_reference)
    {
        return;
    }

    std::cout
        << "Mare Vulkan viewer-deferred-spot-light-volume-probe: using the "
        << "three-band synthetic G-buffer, then adding one indexed cube "
        << "SpotLight projector pass into the deferred light target before "
        << "final composite. This guards the outside-camera projector volume "
        << "owner, point_light.vert ABI, cube/projection/noise/lightFunc "
        << "bindings, indexed TRIANGLE_FAN draw path, and final handoff."
        << std::endl;
    logged_reference = true;
}

void log_deferred_lightmap_blur_probe_reference()
{
    static bool logged_reference = false;
    if (logged_reference)
    {
        return;
    }

    std::cout
        << "Mare Vulkan viewer-deferred-lightmap-blur-probe: seeding a "
        << "three-band synthetic lightMap, then running DeferredBlurLight "
        << "horizontal and vertical ping-pong passes with the viewer default "
        << "RenderShadowGaussian/RenderShadowBlurSize values. This guards the "
        << "OpenGL-style blurLightF lightMap owner, normal/depth edge inputs, "
        << "and post-blur handoff before DeferredSoften consumes lightMap."
        << std::endl;
    logged_reference = true;
}

void bind_deferred_color_compare_material_textures(
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

void draw_deferred_color_compare_gbuffer_scene(
    LLRenderBackend& backend,
    const SmokeDeferredTextures& textures,
    const SmokeQuad& quad,
    U32 width,
    U32 height,
    F32 depth = 0.f,
    bool pbr_emissive = false,
    bool pbr_reflective = false)
{
    log_deferred_color_compare_reference();

    SmokeMatrixScope matrix_scope;
    bind_deferred_color_compare_material_textures(backend, textures);
    bind_world_smoke_quad(backend, quad);
    backend.setWorldDrawEnabled(true);
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

    backend.setWorldShaderClass(LLRenderWorldShaderClass::Textured);
    backend.setWorldMaterialParameters(
        make_deferred_color_compare_gbuffer_material());
    set_two_prim_world_matrix(-0.67f, 0.f, depth, 0.34f, 1.f);
    backend.drawArrays(LLRenderPrimitiveType::Triangles, 0, 6);

    backend.setWorldShaderClass(LLRenderWorldShaderClass::Material);
    backend.setWorldMaterialParameters(
        make_deferred_color_compare_legacy_material());
    set_two_prim_world_matrix(0.f, 0.f, depth, 0.34f, 1.f);
    backend.drawArrays(LLRenderPrimitiveType::Triangles, 0, 6);

    backend.setWorldShaderClass(LLRenderWorldShaderClass::PBR);
    LLRenderWorldMaterialParameters pbr_parameters =
        make_deferred_color_compare_pbr_material();
    if (pbr_emissive)
    {
        pbr_parameters =
            make_deferred_color_compare_pbr_emissive_material();
    }
    else if (pbr_reflective)
    {
        pbr_parameters =
            make_deferred_color_compare_pbr_reflective_material();
    }
    backend.setWorldMaterialParameters(
        pbr_parameters);
    set_two_prim_world_matrix(0.67f, 0.f, depth, 0.34f, 1.f);
    backend.drawArrays(LLRenderPrimitiveType::Triangles, 0, 6);

    backend.setWorldDrawEnabled(false);
    backend.setWorldShaderClass(LLRenderWorldShaderClass::Textured);
    backend.setWorldMaterialParameters({});
    backend.setCapability(LLRenderCapability::DepthTest, false);
    backend.setDepthWriteEnabled(false);
    backend.setScissor(0, 0, static_cast<S32>(width), static_cast<S32>(height));
}

void draw_deferred_ssr_probe_gbuffer_scene(
    LLRenderBackend& backend,
    const SmokeDeferredTextures& textures,
    const SmokeQuad& quad,
    U32 width,
    U32 height)
{
    SmokeMatrixScope matrix_scope;
    bind_deferred_color_compare_material_textures(backend, textures);
    bind_world_smoke_quad(backend, quad);
    backend.setWorldDrawEnabled(true);
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

    backend.setWorldShaderClass(LLRenderWorldShaderClass::PBR);
    backend.setWorldMaterialParameters(
        make_deferred_color_compare_pbr_reflective_material());
    set_two_prim_world_matrix(0.f, 0.f, 0.f, 1.f, 1.f);
    backend.drawArrays(LLRenderPrimitiveType::Triangles, 0, 6);

    backend.setWorldDrawEnabled(false);
    backend.setWorldShaderClass(LLRenderWorldShaderClass::Textured);
    backend.setWorldMaterialParameters({});
    backend.setCapability(LLRenderCapability::DepthTest, false);
    backend.setDepthWriteEnabled(false);
    backend.setScissor(0, 0, static_cast<S32>(width), static_cast<S32>(height));
}

void draw_terrain_final_probe_gbuffer_scene(
    LLRenderBackend& backend,
    const SmokeDeferredTextures& textures,
    const SmokeQuad& quad,
    U32 width,
    U32 height)
{
    log_terrain_final_probe_reference();

    SmokeMatrixScope matrix_scope;
    bind_terrain_final_probe_textures(backend, textures);
    bind_world_smoke_quad(backend, quad);
    backend.setWorldDrawEnabled(true);
    backend.setWorldTextureTransform({});
    backend.setWorldTerrainParameters(make_world_pipeline_terrain_parameters());
    backend.setWorldSkinningMatrixPalette(0, nullptr);
    backend.setCapability(LLRenderCapability::Blend, false);
    backend.setCapability(LLRenderCapability::DepthTest, true);
    backend.setDepthFunction(LLRenderDepthFunction::LessEqual);
    backend.setDepthWriteEnabled(true);
    backend.setCapability(LLRenderCapability::CullFace, false);
    backend.setColorMask({ true, true, true, true });
    backend.setScissor(0, 0, static_cast<S32>(width), static_cast<S32>(height));
    backend.setWorldShaderClass(LLRenderWorldShaderClass::Terrain);
    backend.setWorldMaterialParameters(
        make_world_pipeline_material(
            0.34f,
            0.58f,
            0.28f,
            1.f,
            0));
    set_two_prim_world_matrix(0.f, 0.f, 0.f, 1.f, 1.f);
    backend.drawArrays(LLRenderPrimitiveType::Triangles, 0, 6);

    backend.setWorldDrawEnabled(false);
    backend.setWorldShaderClass(LLRenderWorldShaderClass::Textured);
    backend.setWorldMaterialParameters({});
    backend.setWorldTerrainParameters({});
    backend.setWorldTextureTransform({});
    backend.setWorldSkinningMatrixPalette(0, nullptr);
    backend.setCapability(LLRenderCapability::DepthTest, false);
    backend.setDepthWriteEnabled(false);
    backend.setScissor(0, 0, static_cast<S32>(width), static_cast<S32>(height));
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

std::array<SmokeWorldPipelineEntry, 6> make_class1_gbuffer_probe_entries()
{
    return
    {{
        { LLRenderWorldShaderClass::Textured, "Textured", 0.32f, 0.62f, 0.94f, 0 },
        { LLRenderWorldShaderClass::Terrain, "Terrain", 0.34f, 0.58f, 0.28f, 0 },
        { LLRenderWorldShaderClass::AlphaMask, "AlphaMask", 0.95f, 0.82f, 0.25f, LLRenderWorldMaterialParameters::AlphaMask },
        { LLRenderWorldShaderClass::Material, "Material", 0.78f, 0.58f, 0.95f, LLRenderWorldMaterialParameters::HasSpecularMap },
        { LLRenderWorldShaderClass::PBR, "PBR", 0.90f, 0.72f, 0.48f, LLRenderWorldMaterialParameters::GLTFPBR | LLRenderWorldMaterialParameters::HasORMMap | LLRenderWorldMaterialParameters::HasNormalMap },
        { LLRenderWorldShaderClass::Avatar, "Avatar", 0.86f, 0.52f, 0.44f, 0 },
    }};
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
        make_class1_gbuffer_probe_entries();

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
            backend,
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
            backend,
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

bool write_rgb_ppm_file(
    const std::string& path,
    U32 width,
    U32 height,
    const std::vector<U8>& rgb_pixels)
{
    if (path.empty())
    {
        return true;
    }

    const size_t expected_size =
        static_cast<size_t>(width) * static_cast<size_t>(height) * 3U;
    if (rgb_pixels.size() != expected_size)
    {
        std::cerr
            << "Invalid PPM pixel buffer for "
            << path
            << ": expected "
            << expected_size
            << " bytes, got "
            << rgb_pixels.size()
            << ".\n";
        return false;
    }

    std::ofstream output(path, std::ios::binary);
    if (!output.is_open())
    {
        std::cerr << "Unable to open reference PPM file: " << path << ".\n";
        return false;
    }

    output
        << "P6\n"
        << width
        << " "
        << height
        << "\n255\n";
    output.write(
        reinterpret_cast<const char*>(rgb_pixels.data()),
        static_cast<std::streamsize>(rgb_pixels.size()));
    if (!output.good())
    {
        std::cerr << "Failed while writing reference PPM file: " << path << ".\n";
        return false;
    }

    std::cout << "Wrote Mare smoke RGB PPM to " << path << ".\n";
    return true;
}

std::string read_smoke_text_file(const std::string& path)
{
    std::ifstream input(path, std::ios::binary);
    if (!input.is_open())
    {
        return {};
    }

    std::ostringstream output;
    output << input.rdbuf();
    return output.str();
}

bool log_shader_compile_failure(
    LLRenderBackend& backend,
    U32 shader,
    const char* label)
{
    S32 status = 0;
    backend.getShaderInteger(
        shader,
        LLRenderShaderParameter::CompileStatus,
        &status);
    if (status)
    {
        return false;
    }

    S32 log_length = 0;
    backend.getShaderInteger(
        shader,
        LLRenderShaderParameter::InfoLogLength,
        &log_length);
    std::vector<char> info_log(static_cast<size_t>(llmax(1, log_length)), '\0');
    S32 written = 0;
    backend.getShaderInfoLog(
        shader,
        static_cast<S32>(info_log.size()),
        &written,
        info_log.data());
    std::cerr
        << "OpenGL reference shader compile failed for "
        << label
        << ": "
        << info_log.data()
        << "\n";
    return true;
}

bool log_program_link_failure(
    LLRenderBackend& backend,
    U32 program,
    const char* label)
{
    S32 status = 0;
    backend.getProgramInteger(
        program,
        LLRenderProgramParameter::LinkStatus,
        &status);
    if (status)
    {
        return false;
    }

    S32 log_length = 0;
    backend.getProgramInteger(
        program,
        LLRenderProgramParameter::InfoLogLength,
        &log_length);
    std::vector<char> info_log(static_cast<size_t>(llmax(1, log_length)), '\0');
    S32 written = 0;
    backend.getProgramInfoLog(
        program,
        static_cast<S32>(info_log.size()),
        &written,
        info_log.data());
    std::cerr
        << "OpenGL reference program link failed for "
        << label
        << ": "
        << info_log.data()
        << "\n";
    return true;
}

bool compile_opengl_reference_shader(
    LLRenderBackend& backend,
    const std::string& path,
    LLRenderShaderStage stage,
    U32& shader,
    const std::string& prefix = std::string())
{
    const std::string source = read_smoke_text_file(path);
    if (source.empty())
    {
        std::cerr << "Unable to read OpenGL reference shader: " << path << ".\n";
        return false;
    }

    shader = backend.createShader(stage);
    if (!shader)
    {
        std::cerr << "Unable to create OpenGL reference shader: " << path << ".\n";
        return false;
    }

    const std::string version = "#version 410 core\n";
    const char* sources[] =
    {
        version.c_str(),
        prefix.c_str(),
        source.c_str(),
    };
    backend.setShaderSource(shader, 3, sources);
    backend.compileShader(shader);
    if (log_shader_compile_failure(backend, shader, path.c_str()))
    {
        backend.deleteShader(shader);
        shader = 0;
        return false;
    }
    return true;
}

bool write_rgba_readback_as_rgb_ppm(
    const std::string& path,
    U32 width,
    U32 height,
    const std::vector<U8>& rgba_pixels)
{
    const size_t expected_size =
        static_cast<size_t>(width) * static_cast<size_t>(height) * 4U;
    if (rgba_pixels.size() != expected_size)
    {
        std::cerr
            << "Invalid RGBA readback size for "
            << path
            << ": expected "
            << expected_size
            << " bytes, got "
            << rgba_pixels.size()
            << ".\n";
        return false;
    }

    std::vector<U8> rgb_pixels;
    rgb_pixels.resize(static_cast<size_t>(width) * static_cast<size_t>(height) * 3U);
    for (size_t rgba = 0, rgb = 0; rgba < rgba_pixels.size(); rgba += 4, rgb += 3)
    {
        rgb_pixels[rgb + 0] = rgba_pixels[rgba + 0];
        rgb_pixels[rgb + 1] = rgba_pixels[rgba + 1];
        rgb_pixels[rgb + 2] = rgba_pixels[rgba + 2];
    }
    return write_rgb_ppm_file(path, width, height, rgb_pixels);
}

bool render_opengl_copy_reference_ppm(
    LLRenderBackend& backend,
    const std::string& path,
    U32 width,
    U32 height)
{
    U32 vertex_shader = 0;
    U32 fragment_shader = 0;
    U32 program = 0;
    LLRenderTextureHandle diffuse_texture;
    LLRenderVertexArrayHandle vertex_array;
    SmokeQuad quad;

    auto cleanup = [&]()
    {
        backend.useProgram(0);
        if (quad.mVertexBuffer)
        {
            backend.deleteBufferHandle(quad.mVertexBuffer);
            quad.mVertexBuffer = {};
        }
        if (vertex_array)
        {
            backend.bindVertexArray(0);
            vertex_array = {};
        }
        if (diffuse_texture)
        {
            backend.deleteTextureHandle(diffuse_texture);
            diffuse_texture = {};
        }
        if (program)
        {
            if (vertex_shader)
            {
                backend.detachShader(program, vertex_shader);
            }
            if (fragment_shader)
            {
                backend.detachShader(program, fragment_shader);
            }
            backend.deleteProgram(program);
            program = 0;
        }
        if (vertex_shader)
        {
            backend.deleteShader(vertex_shader);
            vertex_shader = 0;
        }
        if (fragment_shader)
        {
            backend.deleteShader(fragment_shader);
            fragment_shader = 0;
        }
    };

    if (!compile_opengl_reference_shader(
            backend,
            "indra/newview/app_settings/shaders/class1/interface/copyV.glsl",
            LLRenderShaderStage::Vertex,
            vertex_shader) ||
        !compile_opengl_reference_shader(
            backend,
            "indra/newview/app_settings/shaders/class1/interface/copyF.glsl",
            LLRenderShaderStage::Fragment,
            fragment_shader))
    {
        cleanup();
        return false;
    }

    program = backend.createProgram();
    if (!program)
    {
        cleanup();
        return false;
    }
    backend.attachShader(program, vertex_shader);
    backend.attachShader(program, fragment_shader);
    backend.bindAttributeLocation(program, 0, "position");
    backend.linkProgram(program);
    if (log_program_link_failure(backend, program, "OpenGL copy reference"))
    {
        cleanup();
        return false;
    }

    if (!create_smoke_texture(
            backend,
            diffuse_texture,
            4,
            4,
            make_solid_rgba_pixels(4, 4, 42, 126, 230, 255)) ||
        !create_smoke_quad(backend, quad))
    {
        cleanup();
        return false;
    }

    vertex_array = backend.createVertexArrayHandle();
    if (!vertex_array)
    {
        cleanup();
        return false;
    }
    backend.bindVertexArray(vertex_array);

    backend.bindReadWriteFramebuffer(LLRenderFramebufferHandle());
    backend.restoreDefaultFramebufferBufferRouting();
    backend.setViewport(0, 0, static_cast<S32>(width), static_cast<S32>(height));
    backend.setScissor(0, 0, static_cast<S32>(width), static_cast<S32>(height));
    backend.setClearColor(0.f, 0.f, 0.f, 1.f);
    backend.clear(LL_RENDER_CLEAR_COLOR | LL_RENDER_CLEAR_DEPTH);
    backend.setCapability(LLRenderCapability::DepthTest, false);
    backend.setDepthWriteEnabled(false);
    backend.setCapability(LLRenderCapability::Blend, false);
    backend.setCapability(LLRenderCapability::CullFace, false);
    backend.setColorMask({ true, true, true, true });

    backend.useProgram(program);
    const S32 diffuse_location = backend.getUniformLocation(program, "diffuseMap");
    if (diffuse_location >= 0)
    {
        backend.setUniformInteger(diffuse_location, 0);
    }
    backend.setActiveTextureUnit(0);
    backend.bindTexture(LLRenderTextureTarget::Texture2D, diffuse_texture);
    backend.setTextureFilter(
        LLRenderTextureTarget::Texture2D,
        LLRenderTextureFilter::Linear,
        LLRenderTextureFilter::Linear);
    bind_smoke_quad(backend, quad);
    backend.drawArrays(LLRenderPrimitiveType::Triangles, 0, 6);

    std::vector<U8> rgba_pixels;
    rgba_pixels.resize(static_cast<size_t>(width) * static_cast<size_t>(height) * 4U);
    backend.readPixels(
        0,
        0,
        static_cast<S32>(width),
        static_cast<S32>(height),
        LLRenderPixelFormat::RGBA,
        LLRenderPixelType::UnsignedByte,
        rgba_pixels.data());

    bool result = write_rgba_readback_as_rgb_ppm(path, width, height, rgba_pixels);
    if (result)
    {
        std::cout
            << "Wrote Mare smoke real OpenGL copy reference PPM to "
            << path
            << " at "
            << width
            << "x"
            << height
            << ".\n";
    }
    cleanup();
    return result;
}

std::string get_opengl_haze_vertex_prefix()
{
    return
        "out vec3 mare_smoke_atmos_attenuation;\n"
        "out vec3 mare_smoke_additive_color;\n"
        "void setAtmosAttenuation(vec3 c) { mare_smoke_atmos_attenuation = c; }\n"
        "void setAdditiveColor(vec3 c) { mare_smoke_additive_color = c; }\n";
}

std::string get_opengl_haze_fragment_prefix()
{
    return
        "uniform sampler2D depthMap;\n"
        "uniform sampler2D normalMap;\n"
        "vec3 linear_to_srgb(vec3 c)\n"
        "{\n"
        "    bvec3 cutoff = lessThanEqual(c, vec3(0.0031308));\n"
        "    vec3 low = c * 12.92;\n"
        "    vec3 high = 1.055 * pow(max(c, vec3(0.0)), vec3(1.0 / 2.4)) - 0.055;\n"
        "    return mix(high, low, cutoff);\n"
        "}\n"
        "vec3 srgb_to_linear(vec3 c)\n"
        "{\n"
        "    bvec3 cutoff = lessThanEqual(c, vec3(0.04045));\n"
        "    vec3 low = c / 12.92;\n"
        "    vec3 high = pow((c + vec3(0.055)) / 1.055, vec3(2.4));\n"
        "    return mix(high, low, cutoff);\n"
        "}\n"
        "float getDepth(vec2 pos_screen)\n"
        "{\n"
        "    return texture(depthMap, pos_screen).r;\n"
        "}\n"
        "vec4 getNorm(vec2 pos_screen)\n"
        "{\n"
        "    vec3 encoded = texture(normalMap, pos_screen).rgb;\n"
        "    vec3 normal = normalize(encoded * 2.0 - 1.0);\n"
        "    return vec4(normal, 0.0);\n"
        "}\n"
        "vec4 getPositionWithDepth(vec2 pos_screen, float depth)\n"
        "{\n"
        "    vec2 ndc = pos_screen * 2.0 - 1.0;\n"
        "    float eye_depth = mix(2.0, 80.0, clamp(depth, 0.0, 1.0));\n"
        "    return vec4(ndc.x * 24.0, ndc.y * 14.0, -eye_depth, 1.0);\n"
        "}\n"
        "void calcAtmosphericVarsLinear(\n"
        "    vec3 inPositionEye,\n"
        "    vec3 norm,\n"
        "    vec3 light_dir,\n"
        "    out vec3 sunlit,\n"
        "    out vec3 amblit,\n"
        "    out vec3 atten,\n"
        "    out vec3 additive)\n"
        "{\n"
        "    float distance_factor = clamp(length(inPositionEye) / 80.0, 0.0, 1.0);\n"
        "    float light_factor = clamp(dot(normalize(norm), normalize(light_dir)) * 0.5 + 0.5, 0.0, 1.0);\n"
        "    sunlit = vec3(0.95, 0.92, 0.84) * light_factor;\n"
        "    amblit = vec3(0.30, 0.36, 0.48);\n"
        "    atten = vec3(mix(0.88, 0.32, smoothstep(0.1, 1.0, distance_factor)));\n"
        "    additive = vec3(0.22, 0.34, 0.58) * (1.0 - atten) * (0.72 + light_factor * 0.28);\n"
        "}\n";
}

bool render_opengl_haze_reference_ppm(
    LLRenderBackend& backend,
    const std::string& path,
    U32 width,
    U32 height)
{
    U32 vertex_shader = 0;
    U32 fragment_shader = 0;
    U32 program = 0;
    LLRenderTextureHandle depth_texture;
    LLRenderTextureHandle normal_texture;
    LLRenderVertexArrayHandle vertex_array;
    SmokeQuad quad;

    auto cleanup = [&]()
    {
        backend.useProgram(0);
        if (quad.mVertexBuffer)
        {
            backend.deleteBufferHandle(quad.mVertexBuffer);
            quad.mVertexBuffer = {};
        }
        if (vertex_array)
        {
            backend.bindVertexArray(0);
            vertex_array = {};
        }
        if (depth_texture)
        {
            backend.deleteTextureHandle(depth_texture);
            depth_texture = {};
        }
        if (normal_texture)
        {
            backend.deleteTextureHandle(normal_texture);
            normal_texture = {};
        }
        if (program)
        {
            if (vertex_shader)
            {
                backend.detachShader(program, vertex_shader);
            }
            if (fragment_shader)
            {
                backend.detachShader(program, fragment_shader);
            }
            backend.deleteProgram(program);
            program = 0;
        }
        if (vertex_shader)
        {
            backend.deleteShader(vertex_shader);
            vertex_shader = 0;
        }
        if (fragment_shader)
        {
            backend.deleteShader(fragment_shader);
            fragment_shader = 0;
        }
    };

    if (!compile_opengl_reference_shader(
            backend,
            "indra/newview/app_settings/shaders/class2/deferred/softenLightV.glsl",
            LLRenderShaderStage::Vertex,
            vertex_shader,
            get_opengl_haze_vertex_prefix()) ||
        !compile_opengl_reference_shader(
            backend,
            "indra/newview/app_settings/shaders/class3/deferred/hazeF.glsl",
            LLRenderShaderStage::Fragment,
            fragment_shader,
            get_opengl_haze_fragment_prefix()))
    {
        cleanup();
        return false;
    }

    program = backend.createProgram();
    if (!program)
    {
        cleanup();
        return false;
    }
    backend.attachShader(program, vertex_shader);
    backend.attachShader(program, fragment_shader);
    backend.bindAttributeLocation(program, 0, "position");
    backend.linkProgram(program);
    if (log_program_link_failure(backend, program, "OpenGL haze reference"))
    {
        cleanup();
        return false;
    }

    if (!create_smoke_texture(
            backend,
            depth_texture,
            4,
            4,
            make_solid_rgba_pixels(4, 4, 204, 204, 204, 255)) ||
        !create_smoke_texture(
            backend,
            normal_texture,
            4,
            4,
            make_solid_rgba_pixels(4, 4, 128, 128, 255, 0)) ||
        !create_smoke_quad(backend, quad))
    {
        cleanup();
        return false;
    }

    vertex_array = backend.createVertexArrayHandle();
    if (!vertex_array)
    {
        cleanup();
        return false;
    }
    backend.bindVertexArray(vertex_array);

    backend.bindReadWriteFramebuffer(LLRenderFramebufferHandle());
    backend.restoreDefaultFramebufferBufferRouting();
    backend.setViewport(0, 0, static_cast<S32>(width), static_cast<S32>(height));
    backend.setScissor(0, 0, static_cast<S32>(width), static_cast<S32>(height));
    backend.setClearColor(0.f, 0.f, 0.f, 1.f);
    backend.clear(LL_RENDER_CLEAR_COLOR | LL_RENDER_CLEAR_DEPTH);
    backend.setCapability(LLRenderCapability::DepthTest, false);
    backend.setDepthWriteEnabled(false);
    backend.setCapability(LLRenderCapability::Blend, false);
    backend.setCapability(LLRenderCapability::CullFace, false);
    backend.setColorMask({ true, true, true, true });

    backend.useProgram(program);
    const auto set_int_uniform = [&](const char* name, S32 value)
    {
        const S32 location = backend.getUniformLocation(program, name);
        if (location >= 0)
        {
            backend.setUniformInteger(location, value);
        }
    };
    const auto set_float_uniform = [&](const char* name, F32 value)
    {
        const S32 location = backend.getUniformLocation(program, name);
        if (location >= 0)
        {
            backend.setUniformFloat(location, value);
        }
    };
    const auto set_vec3_uniform = [&](const char* name, F32 x, F32 y, F32 z)
    {
        const S32 location = backend.getUniformLocation(program, name);
        if (location >= 0)
        {
            backend.setUniformFloat3(location, x, y, z);
        }
    };
    const auto set_vec4_uniform = [&](const char* name, F32 x, F32 y, F32 z, F32 w)
    {
        const S32 location = backend.getUniformLocation(program, name);
        if (location >= 0)
        {
            backend.setUniformFloat4(location, x, y, z, w);
        }
    };

    set_int_uniform("depthMap", 0);
    set_int_uniform("normalMap", 1);
    set_int_uniform("sun_up_factor", 1);
    set_int_uniform("cube_snapshot", 0);
    set_float_uniform("sky_hdr_scale", 1.f);
    set_vec3_uniform("sun_dir", 0.35f, 0.45f, 0.82f);
    set_vec3_uniform("moon_dir", -0.25f, -0.15f, 0.95f);
    set_vec4_uniform("waterPlane", 0.f, 0.f, 1.f, 1.f);

    backend.setActiveTextureUnit(0);
    backend.bindTexture(LLRenderTextureTarget::Texture2D, depth_texture);
    backend.setActiveTextureUnit(1);
    backend.bindTexture(LLRenderTextureTarget::Texture2D, normal_texture);
    backend.setActiveTextureUnit(0);
    bind_smoke_quad(backend, quad);
    backend.drawArrays(LLRenderPrimitiveType::Triangles, 0, 6);

    std::vector<U8> rgba_pixels;
    rgba_pixels.resize(static_cast<size_t>(width) * static_cast<size_t>(height) * 4U);
    backend.readPixels(
        0,
        0,
        static_cast<S32>(width),
        static_cast<S32>(height),
        LLRenderPixelFormat::RGBA,
        LLRenderPixelType::UnsignedByte,
        rgba_pixels.data());

    bool result = write_rgba_readback_as_rgb_ppm(path, width, height, rgba_pixels);
    if (result)
    {
        std::cout
            << "Wrote Mare smoke OpenGL haze source-reference PPM to "
            << path
            << " at "
            << width
            << "x"
            << height
            << ".\n";
    }
    cleanup();
    return result;
}

std::string get_opengl_alpha_fragment_prefix()
{
    return
        "#define USE_DIFFUSE_TEX 1\n"
        "#define USE_VERTEX_COLOR 1\n"
        "#define HAS_ALPHA_MASK 1\n"
        "vec3 linear_to_srgb(vec3 c)\n"
        "{\n"
        "    bvec3 cutoff = lessThanEqual(c, vec3(0.0031308));\n"
        "    vec3 low = c * 12.92;\n"
        "    vec3 high = 1.055 * pow(max(c, vec3(0.0)), vec3(1.0 / 2.4)) - 0.055;\n"
        "    return mix(high, low, cutoff);\n"
        "}\n"
        "vec3 srgb_to_linear(vec3 c)\n"
        "{\n"
        "    bvec3 cutoff = lessThanEqual(c, vec3(0.04045));\n"
        "    vec3 low = c / 12.92;\n"
        "    vec3 high = pow((c + vec3(0.055)) / 1.055, vec3(2.4));\n"
        "    return mix(high, low, cutoff);\n"
        "}\n"
        "void waterClip(vec3 pos) {}\n"
        "void mirrorClip(vec3 pos) {}\n"
        "float getAmbientClamp() { return 1.0; }\n"
        "vec4 applySkyAndWaterFog(vec3 pos, vec3 additive, vec3 atten, vec4 color)\n"
        "{\n"
        "    return color;\n"
        "}\n"
        "void calcAtmosphericVarsLinear(\n"
        "    vec3 inPositionEye,\n"
        "    vec3 norm,\n"
        "    vec3 light_dir,\n"
        "    out vec3 sunlit,\n"
        "    out vec3 amblit,\n"
        "    out vec3 atten,\n"
        "    out vec3 additive)\n"
        "{\n"
        "    float light_factor = clamp(dot(normalize(norm), normalize(light_dir)) * 0.5 + 0.5, 0.0, 1.0);\n"
        "    sunlit = vec3(1.0, 0.96, 0.88) * light_factor;\n"
        "    amblit = vec3(0.34, 0.38, 0.46);\n"
        "    atten = vec3(1.0);\n"
        "    additive = vec3(0.0);\n"
        "}\n"
        "void sampleReflectionProbesLegacy(\n"
        "    inout vec3 ambenv,\n"
        "    inout vec3 glossenv,\n"
        "    inout vec3 legacyenv,\n"
        "    vec2 tc,\n"
        "    vec3 pos,\n"
        "    vec3 norm,\n"
        "    float glossiness,\n"
        "    float envIntensity,\n"
        "    bool transparent,\n"
        "    vec3 amblit_linear)\n"
        "{\n"
        "    ambenv = amblit_linear;\n"
        "    glossenv = vec3(0.0);\n"
        "    legacyenv = vec3(0.0);\n"
        "}\n";
}

bool render_opengl_alpha_reference_ppm(
    LLRenderBackend& backend,
    const std::string& path,
    U32 width,
    U32 height)
{
    U32 vertex_shader = 0;
    U32 fragment_shader = 0;
    U32 program = 0;
    LLRenderTextureHandle diffuse_texture;
    LLRenderVertexArrayHandle vertex_array;
    SmokeQuad quad;

    auto cleanup = [&]()
    {
        backend.useProgram(0);
        if (quad.mVertexBuffer)
        {
            backend.deleteBufferHandle(quad.mVertexBuffer);
            quad.mVertexBuffer = {};
        }
        if (vertex_array)
        {
            backend.bindVertexArray(0);
            vertex_array = {};
        }
        if (diffuse_texture)
        {
            backend.deleteTextureHandle(diffuse_texture);
            diffuse_texture = {};
        }
        if (program)
        {
            if (vertex_shader)
            {
                backend.detachShader(program, vertex_shader);
            }
            if (fragment_shader)
            {
                backend.detachShader(program, fragment_shader);
            }
            backend.deleteProgram(program);
            program = 0;
        }
        if (vertex_shader)
        {
            backend.deleteShader(vertex_shader);
            vertex_shader = 0;
        }
        if (fragment_shader)
        {
            backend.deleteShader(fragment_shader);
            fragment_shader = 0;
        }
    };

    if (!compile_opengl_reference_shader(
            backend,
            "indra/newview/app_settings/shaders/class1/deferred/alphaV.glsl",
            LLRenderShaderStage::Vertex,
            vertex_shader,
            get_opengl_alpha_fragment_prefix()) ||
        !compile_opengl_reference_shader(
            backend,
            "indra/newview/app_settings/shaders/class2/deferred/alphaF.glsl",
            LLRenderShaderStage::Fragment,
            fragment_shader,
            get_opengl_alpha_fragment_prefix()))
    {
        cleanup();
        return false;
    }

    program = backend.createProgram();
    if (!program)
    {
        cleanup();
        return false;
    }
    backend.attachShader(program, vertex_shader);
    backend.attachShader(program, fragment_shader);
    backend.bindAttributeLocation(program, 0, "position");
    backend.bindAttributeLocation(program, 1, "normal");
    backend.bindAttributeLocation(program, 2, "texcoord0");
    backend.bindAttributeLocation(program, 6, "diffuse_color");
    backend.linkProgram(program);
    if (log_program_link_failure(backend, program, "OpenGL alpha reference"))
    {
        cleanup();
        return false;
    }

    if (!create_smoke_texture(
            backend,
            diffuse_texture,
            4,
            4,
            make_solid_rgba_pixels(4, 4, 42, 126, 230, 255)) ||
        !create_smoke_quad(
            backend,
            quad,
            {{
                static_cast<U8>(to_color_byte(0.82f)),
                static_cast<U8>(to_color_byte(0.34f)),
                static_cast<U8>(to_color_byte(0.68f)),
                static_cast<U8>(to_color_byte(0.72f)),
            }}))
    {
        cleanup();
        return false;
    }

    vertex_array = backend.createVertexArrayHandle();
    if (!vertex_array)
    {
        cleanup();
        return false;
    }
    backend.bindVertexArray(vertex_array);

    backend.bindReadWriteFramebuffer(LLRenderFramebufferHandle());
    backend.restoreDefaultFramebufferBufferRouting();
    backend.setViewport(0, 0, static_cast<S32>(width), static_cast<S32>(height));
    backend.setScissor(0, 0, static_cast<S32>(width), static_cast<S32>(height));
    backend.setClearColor(0.015f, 0.018f, 0.024f, 1.f);
    backend.clear(LL_RENDER_CLEAR_COLOR | LL_RENDER_CLEAR_DEPTH);
    backend.setCapability(LLRenderCapability::DepthTest, false);
    backend.setDepthWriteEnabled(false);
    backend.setCapability(LLRenderCapability::Blend, true);
    backend.setBlendState(
        {
            LLRenderBlendFactor::SourceAlpha,
            LLRenderBlendFactor::OneMinusSourceAlpha,
            LLRenderBlendFactor::One,
            LLRenderBlendFactor::OneMinusSourceAlpha,
        });
    backend.setCapability(LLRenderCapability::CullFace, false);
    backend.setColorMask({ true, true, true, true });

    backend.useProgram(program);
    const auto set_int_uniform = [&](const char* name, S32 value)
    {
        const S32 location = backend.getUniformLocation(program, name);
        if (location >= 0)
        {
            backend.setUniformInteger(location, value);
        }
    };
    const auto set_float_uniform = [&](const char* name, F32 value)
    {
        const S32 location = backend.getUniformLocation(program, name);
        if (location >= 0)
        {
            backend.setUniformFloat(location, value);
        }
    };
    const auto set_vec2_uniform = [&](const char* name, F32 x, F32 y)
    {
        const S32 location = backend.getUniformLocation(program, name);
        if (location >= 0)
        {
            backend.setUniformFloat2(location, x, y);
        }
    };
    const auto set_vec3_uniform = [&](const char* name, F32 x, F32 y, F32 z)
    {
        const S32 location = backend.getUniformLocation(program, name);
        if (location >= 0)
        {
            backend.setUniformFloat3(location, x, y, z);
        }
    };
    const auto set_vec4_uniform = [&](const char* name, F32 x, F32 y, F32 z, F32 w)
    {
        const S32 location = backend.getUniformLocation(program, name);
        if (location >= 0)
        {
            backend.setUniformFloat4(location, x, y, z, w);
        }
    };
    const auto set_mat3_uniform = [&](const char* name, const glm::mat3& matrix)
    {
        const S32 location = backend.getUniformLocation(program, name);
        if (location >= 0)
        {
            backend.setUniformMatrix3(location, 1, false, glm::value_ptr(matrix));
        }
    };
    const auto set_mat4_uniform = [&](const char* name, const glm::mat4& matrix)
    {
        const S32 location = backend.getUniformLocation(program, name);
        if (location >= 0)
        {
            backend.setUniformMatrix4(location, 1, false, glm::value_ptr(matrix));
        }
    };

    const glm::mat4 identity4(1.f);
    const glm::mat3 identity3(1.f);
    set_mat3_uniform("normal_matrix", identity3);
    set_mat3_uniform("env_mat", identity3);
    set_mat4_uniform("texture_matrix0", identity4);
    set_mat4_uniform("projection_matrix", identity4);
    set_mat4_uniform("modelview_matrix", identity4);
    set_mat4_uniform("modelview_projection_matrix", identity4);
    set_mat4_uniform("proj_mat", identity4);
    set_mat4_uniform("inv_proj", identity4);
    set_int_uniform("diffuseMap", 0);
    set_int_uniform("sun_up_factor", 1);
    set_int_uniform("classic_mode", 0);
    set_float_uniform("near_clip", 1.f);
    set_float_uniform("minimum_alpha", -1.f);
    set_vec2_uniform("screen_res", static_cast<F32>(width), static_cast<F32>(height));
    set_vec3_uniform("sun_dir", 0.35f, 0.45f, 0.82f);
    set_vec3_uniform("moon_dir", -0.25f, -0.15f, 0.95f);

    for (S32 i = 0; i < 8; ++i)
    {
        const std::string index = std::to_string(i);
        set_vec4_uniform(("light_position[" + index + "]").c_str(), 0.f, 0.f, 10.f, 1.f);
        set_vec3_uniform(("light_direction[" + index + "]").c_str(), 0.f, 0.f, -1.f);
        set_vec4_uniform(("light_attenuation[" + index + "]").c_str(), 12.f, 1.f, 1.f, 0.f);
        set_vec3_uniform(("light_diffuse[" + index + "]").c_str(), 0.f, 0.f, 0.f);
    }

    backend.setActiveTextureUnit(0);
    backend.bindTexture(LLRenderTextureTarget::Texture2D, diffuse_texture);
    backend.setTextureFilter(
        LLRenderTextureTarget::Texture2D,
        LLRenderTextureFilter::Linear,
        LLRenderTextureFilter::Linear);
    bind_world_smoke_quad(backend, quad);
    backend.drawArrays(LLRenderPrimitiveType::Triangles, 0, 6);

    std::vector<U8> rgba_pixels;
    rgba_pixels.resize(static_cast<size_t>(width) * static_cast<size_t>(height) * 4U);
    backend.readPixels(
        0,
        0,
        static_cast<S32>(width),
        static_cast<S32>(height),
        LLRenderPixelFormat::RGBA,
        LLRenderPixelType::UnsignedByte,
        rgba_pixels.data());

    bool result = write_rgba_readback_as_rgb_ppm(path, width, height, rgba_pixels);
    if (result)
    {
        std::cout
            << "Wrote Mare smoke OpenGL alpha source-reference PPM to "
            << path
            << " at "
            << width
            << "x"
            << height
            << ".\n";
    }
    cleanup();
    return result;
}

bool write_class1_gbuffer_probe_reference_ppm(
    const std::string& path,
    SmokeScene scene,
    U32 width,
    U32 height)
{
    if (path.empty())
    {
        return true;
    }

    const std::array<SmokeWorldPipelineEntry, 6> entries =
        make_class1_gbuffer_probe_entries();
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

    std::vector<U8> pixels;
    pixels.resize(static_cast<size_t>(width) * static_cast<size_t>(height) * 3U);
    for (U32 y = 0; y < height; ++y)
    {
        const S32 row =
            llclamp(static_cast<S32>(y) / cell_height, 0, rows - 1);
        for (U32 x = 0; x < width; ++x)
        {
            const S32 column =
                llclamp(static_cast<S32>(x) / cell_width, 0, columns - 1);
            const U32 tile_index =
                static_cast<U32>(row * columns + column);
            const SmokeWorldPipelineEntry& entry =
                entries[tile_index < tile_count ? tile_index % entries.size() : 0U];
            const size_t offset =
                (static_cast<size_t>(y) * static_cast<size_t>(width) +
                    static_cast<size_t>(x)) * 3U;
            pixels[offset + 0] = static_cast<U8>(to_color_byte(entry.mRed));
            pixels[offset + 1] = static_cast<U8>(to_color_byte(entry.mGreen));
            pixels[offset + 2] = static_cast<U8>(to_color_byte(entry.mBlue));
        }
    }

    std::cout
        << "Mare Vulkan class1-gbuffer-color-probe CPU reference: flat "
        << columns
        << "x"
        << rows
        << " tile grid for Textured, Terrain, AlphaMask, Material, PBR, Avatar. "
        << "This is a first image probe for G-buffer color output; it is not a full OpenGL runtime reference yet."
        << std::endl;
    return write_rgb_ppm_file(path, width, height, pixels);
}

bool render_class1_gbuffer_color_probe_frame(
    LLRenderBackend& backend,
    SmokeDeferredTextures& material_textures,
    SmokeViewerRenderTargetGraph& graph,
    const SmokeQuad& quad,
    SmokeScene scene,
    U32 width,
    U32 height,
    const std::string& reference_path)
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
            backend,
            graph,
            graph_width,
            graph_height,
            4))
    {
        return false;
    }

    static bool wrote_reference = false;
    if (!wrote_reference)
    {
        if (!write_class1_gbuffer_probe_reference_ppm(
                reference_path,
                scene,
                width,
                height))
        {
            return false;
        }
        wrote_reference = true;
    }

    graph.mDeferredScreen.bindTarget();
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
    backend.setClearColor(0.f, 0.f, 0.f, 0.f);
    graph.mDeferredScreen.clear(LL_RENDER_CLEAR_COLOR | LL_RENDER_CLEAR_DEPTH);
    bind_deferred_graph_material_textures(backend, material_textures);
    draw_deferred_graph_gbuffer_tiles(
        backend,
        material_textures,
        quad,
        scene,
        graph_width,
        graph_height);
    graph.mDeferredScreen.flush();

    copy_smoke_target_to_swapchain(
        backend,
        graph.mDeferredScreen,
        quad,
        width,
        height);
    return true;
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
    U32 height,
    const LLRenderWorldMaterialParameters* override_parameters = nullptr)
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
        override_parameters ?
            *override_parameters :
            make_deferred_graph_composite_parameters();
    if (!override_parameters)
    {
        parameters.mRoughnessFactor = static_cast<F32>(attachment_count);
        parameters.mNormalTextureOffsetS = depth_bound ? 1.f : 0.f;
    }
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

void draw_smoke_deferred_soften_quad(
    LLRenderBackend& backend,
    LLRenderTarget& deferred_screen,
    LLRenderTextureHandle light_map_texture,
    const SmokeDeferredTextures& material_textures,
    const SmokeQuad& quad,
    U32 width,
    U32 height,
    const LLRenderWorldMaterialParameters& parameters,
    const SmokeReflectionProbeResources* reflection_probe_resources = nullptr,
    const SmokeSSRResources* ssr_resources = nullptr)
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
    bool scene_depth_bound = false;
    if (deferred_screen.getDepthHandle())
    {
        depth_bound =
            gGL.getTexUnit(4)->bind(&deferred_screen, true);
        scene_depth_bound =
            gGL.getTexUnit(13)->bind(&deferred_screen, true);
    }

    backend.setActiveTextureUnit(5);
    backend.bindTexture(
        LLRenderTextureTarget::Texture2D,
        light_map_texture ? light_map_texture : material_textures.mWhite);
    backend.setActiveTextureUnit(6);
    backend.bindTexture(
        LLRenderTextureTarget::TextureCubeMap,
        material_textures.mCubeWhite);

    if (reflection_probe_resources)
    {
        backend.setActiveTextureUnit(7);
        backend.bindTexture(
            LLRenderTextureTarget::TextureCubeMapArray,
            reflection_probe_resources->mReflectionCubeArray);
        backend.setActiveTextureUnit(8);
        backend.bindTexture(
            LLRenderTextureTarget::TextureCubeMapArray,
            reflection_probe_resources->mIrradianceCubeArray);
        backend.setActiveTextureUnit(9);
        backend.bindTexture(
            LLRenderTextureTarget::TextureCubeMapArray,
            reflection_probe_resources->mHeroCubeArray);
        backend.bindBufferBase(
            LLRenderBufferTarget::Uniform,
            LLGLSLShader::UB_REFLECTION_PROBES,
            reflection_probe_resources->mProbeUniformBuffer);
    }
    else
    {
        for (S32 unit = 7; unit <= 9; ++unit)
        {
            gGL.getTexUnit(unit)->unbind(LLTexUnit::TT_TEXTURE);
            gGL.getTexUnit(unit)->unbind(LLTexUnit::TT_CUBE_MAP);
        }
    }

    backend.setActiveTextureUnit(10);
    backend.bindTexture(LLRenderTextureTarget::Texture2D, material_textures.mWhite);
    backend.setActiveTextureUnit(11);
    backend.bindTexture(LLRenderTextureTarget::Texture2D, material_textures.mWhite);
    if (ssr_resources && ssr_resources->mSceneColor)
    {
        backend.setActiveTextureUnit(12);
        backend.bindTexture(
            LLRenderTextureTarget::Texture2D,
            ssr_resources->mSceneColor);
    }
    else if (attachment_count > 0)
    {
        deferred_screen.bindTexture(0, 12, LLTexUnit::TFO_BILINEAR);
    }
    else
    {
        backend.setActiveTextureUnit(12);
        backend.bindTexture(LLRenderTextureTarget::Texture2D, material_textures.mWhite);
    }
    if (ssr_resources && ssr_resources->mSceneDepth)
    {
        backend.setActiveTextureUnit(13);
        backend.bindTexture(
            LLRenderTextureTarget::Texture2D,
            ssr_resources->mSceneDepth);
    }
    backend.setActiveTextureUnit(0);

    set_smoke_fullscreen_world_draw_state(
        backend,
        LLRenderWorldShaderClass::DeferredSoften,
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
    gGL.getTexUnit(5)->unbind(LLTexUnit::TT_TEXTURE);
    gGL.getTexUnit(6)->unbind(LLTexUnit::TT_CUBE_MAP);
    for (S32 unit = 7; unit <= 9; ++unit)
    {
        gGL.getTexUnit(unit)->unbind(LLTexUnit::TT_TEXTURE);
        gGL.getTexUnit(unit)->unbind(LLTexUnit::TT_CUBE_MAP);
    }
    if (reflection_probe_resources)
    {
        backend.bindBufferBase(
            LLRenderBufferTarget::Uniform,
            LLGLSLShader::UB_REFLECTION_PROBES,
            LLRenderBufferHandle());
    }
    gGL.getTexUnit(10)->unbind(LLTexUnit::TT_TEXTURE);
    gGL.getTexUnit(11)->unbind(LLTexUnit::TT_TEXTURE);
    gGL.getTexUnit(12)->unbind(LLTexUnit::TT_TEXTURE);
    if (scene_depth_bound || (ssr_resources && ssr_resources->mSceneDepth))
    {
        gGL.getTexUnit(13)->unbind(LLTexUnit::TT_TEXTURE);
    }
}

std::vector<U8> make_smoke_deferred_lightmap_band_pixels(
    U32 width,
    U32 height)
{
    struct Band
    {
        U8 mRed;
        U8 mGreen;
        U8 mBlue;
        U8 mAlpha;
    };
    const Band bands[] =
    {
        { 20, 46, 209, 255 },
        { 235, 204, 51, 255 },
        { 51, 242, 115, 255 },
    };

    std::vector<U8> pixels;
    pixels.resize(static_cast<size_t>(width) * static_cast<size_t>(height) * 4U);
    for (U32 y = 0; y < height; ++y)
    {
        for (U32 x = 0; x < width; ++x)
        {
            const U32 band_index = llmin(2U, (x * 3U) / llmax(1U, width));
            const Band& band = bands[band_index];
            const size_t offset =
                (static_cast<size_t>(y) * width + x) * 4U;
            pixels[offset + 0] = band.mRed;
            pixels[offset + 1] = band.mGreen;
            pixels[offset + 2] = band.mBlue;
            pixels[offset + 3] = band.mAlpha;
        }
    }
    return pixels;
}

bool ensure_smoke_deferred_lightmap_source(
    LLRenderBackend& backend,
    SmokeViewerRenderTargetGraph& graph,
    U32 width,
    U32 height)
{
    if (graph.mLightMapSource)
    {
        return true;
    }

    return create_smoke_texture(
        backend,
        graph.mLightMapSource,
        width,
        height,
        make_smoke_deferred_lightmap_band_pixels(width, height));
}

bool draw_smoke_deferred_soften_stage(
    LLRenderBackend& backend,
    SmokeViewerRenderTargetGraph& graph,
    const SmokeDeferredTextures& material_textures,
    const SmokeQuad& quad,
    U32 width,
    U32 height,
    const LLRenderWorldMaterialParameters& parameters,
    const SmokeReflectionProbeResources* reflection_probe_resources = nullptr,
    const SmokeSSRResources* ssr_resources = nullptr)
{
    if (!ensure_smoke_deferred_lightmap_source(
            backend,
            graph,
            width,
            height))
    {
        return false;
    }

    draw_smoke_deferred_soften_quad(
        backend,
        graph.mDeferredScreen,
        graph.mLightMapSource,
        material_textures,
        quad,
        width,
        height,
        parameters,
        reflection_probe_resources,
        ssr_resources);
    return true;
}

void draw_smoke_deferred_lightmap_blur_quad(
    LLRenderBackend& backend,
    LLRenderTextureHandle source_texture,
    LLRenderTarget& destination,
    LLRenderTarget& deferred_screen,
    const SmokeQuad& quad,
    const LLVector2& delta,
    U32 width,
    U32 height)
{
    destination.bindTarget();
    backend.setClearColor(1.f, 1.f, 1.f, 1.f);
    destination.clear(LL_RENDER_CLEAR_COLOR);

    backend.setActiveTextureUnit(0);
    backend.bindTexture(LLRenderTextureTarget::Texture2D, source_texture);
    backend.setTextureFilter(
        LLRenderTextureTarget::Texture2D,
        LLRenderTextureFilter::Linear,
        LLRenderTextureFilter::Linear);
    if (deferred_screen.getNumTextures() > 2)
    {
        deferred_screen.bindTexture(2, 2, LLTexUnit::TFO_BILINEAR);
    }

    bool depth_bound = false;
    if (deferred_screen.getDepthHandle())
    {
        depth_bound =
            gGL.getTexUnit(4)->bind(&deferred_screen, true);
    }

    LLRenderWorldMaterialParameters parameters;
    set_smoke_deferred_blur_parameters(parameters, delta, width, height);

    set_smoke_fullscreen_world_draw_state(
        backend,
        LLRenderWorldShaderClass::DeferredBlurLight,
        parameters,
        width,
        height);
    bind_world_smoke_quad(backend, quad);
    backend.drawArrays(LLRenderPrimitiveType::Triangles, 0, 6);
    reset_smoke_world_draw_state(backend);
    destination.flush();

    gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);
    if (deferred_screen.getNumTextures() > 2)
    {
        gGL.getTexUnit(2)->unbind(LLTexUnit::TT_TEXTURE);
    }
    if (depth_bound)
    {
        gGL.getTexUnit(4)->unbind(LLTexUnit::TT_TEXTURE);
    }
}

void draw_smoke_deferred_local_light_quad(
    LLRenderBackend& backend,
    LLRenderTarget& deferred_screen,
    const SmokeDeferredTextures& material_textures,
    const SmokeQuad& quad,
    U32 width,
    U32 height)
{
    const U32 attachment_count =
        llmin(deferred_screen.getNumTextures(), 3U);
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
            gGL.getTexUnit(3)->bind(&deferred_screen, true);
    }
    backend.setActiveTextureUnit(4);
    backend.bindTexture(LLRenderTextureTarget::Texture2D, material_textures.mWhite);
    backend.setActiveTextureUnit(0);

    set_smoke_fullscreen_world_draw_state(
        backend,
        LLRenderWorldShaderClass::MultiPointLight,
        make_deferred_local_light_probe_parameters(width, height),
        width,
        height);
    backend.setCapability(LLRenderCapability::Blend, true);
    backend.setBlendState(
        {
            LLRenderBlendFactor::One,
            LLRenderBlendFactor::One,
            LLRenderBlendFactor::One,
            LLRenderBlendFactor::One,
        });
    bind_world_smoke_quad(backend, quad);
    backend.drawArrays(LLRenderPrimitiveType::Triangles, 0, 6);
    reset_smoke_world_draw_state(backend);

    for (U32 attachment = 0; attachment < attachment_count; ++attachment)
    {
        gGL.getTexUnit(static_cast<S32>(attachment))->unbind(LLTexUnit::TT_TEXTURE);
    }
    if (depth_bound)
    {
        gGL.getTexUnit(3)->unbind(LLTexUnit::TT_TEXTURE);
    }
    gGL.getTexUnit(4)->unbind(LLTexUnit::TT_TEXTURE);
}

void draw_smoke_deferred_projector_light_quad(
    LLRenderBackend& backend,
    LLRenderTarget& deferred_screen,
    const SmokeDeferredTextures& material_textures,
    const SmokeQuad& quad,
    U32 width,
    U32 height)
{
    const U32 attachment_count =
        llmin(deferred_screen.getNumTextures(), 3U);
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
            gGL.getTexUnit(3)->bind(&deferred_screen, true);
    }
    backend.setActiveTextureUnit(4);
    backend.bindTexture(LLRenderTextureTarget::TextureCubeMap, material_textures.mCubeWhite);
    backend.setActiveTextureUnit(5);
    backend.bindTexture(LLRenderTextureTarget::Texture2D, material_textures.mWhite);
    backend.setActiveTextureUnit(6);
    backend.bindTexture(LLRenderTextureTarget::Texture2D, material_textures.mWhite);
    backend.setActiveTextureUnit(7);
    backend.bindTexture(LLRenderTextureTarget::Texture2D, material_textures.mWhite);
    backend.setActiveTextureUnit(0);

    backend.setWorldDeferredShaderLevel(1);
    set_smoke_fullscreen_world_draw_state(
        backend,
        LLRenderWorldShaderClass::MultiSpotLight,
        make_deferred_projector_light_probe_parameters(width, height),
        width,
        height);
    backend.setCapability(LLRenderCapability::Blend, true);
    backend.setBlendState(
        {
            LLRenderBlendFactor::One,
            LLRenderBlendFactor::One,
            LLRenderBlendFactor::One,
            LLRenderBlendFactor::One,
        });
    bind_world_smoke_quad(backend, quad);
    backend.drawArrays(LLRenderPrimitiveType::Triangles, 0, 6);
    reset_smoke_world_draw_state(backend);
    backend.setWorldDeferredShaderLevel(1);

    for (U32 attachment = 0; attachment < attachment_count; ++attachment)
    {
        gGL.getTexUnit(static_cast<S32>(attachment))->unbind(LLTexUnit::TT_TEXTURE);
    }
    if (depth_bound)
    {
        gGL.getTexUnit(3)->unbind(LLTexUnit::TT_TEXTURE);
    }
    gGL.getTexUnit(4)->unbind(LLTexUnit::TT_TEXTURE);
    gGL.getTexUnit(5)->unbind(LLTexUnit::TT_TEXTURE);
    gGL.getTexUnit(6)->unbind(LLTexUnit::TT_TEXTURE);
    gGL.getTexUnit(7)->unbind(LLTexUnit::TT_TEXTURE);
}

void draw_smoke_deferred_point_light_volume(
    LLRenderBackend& backend,
    LLRenderTarget& deferred_screen,
    const SmokeDeferredTextures& material_textures,
    const SmokeCube& cube,
    U32 width,
    U32 height)
{
    const U32 attachment_count =
        llmin(deferred_screen.getNumTextures(), 3U);
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
            gGL.getTexUnit(3)->bind(&deferred_screen, true);
    }
    backend.setActiveTextureUnit(4);
    backend.bindTexture(LLRenderTextureTarget::Texture2D, material_textures.mWhite);
    backend.setActiveTextureUnit(0);

    set_smoke_fullscreen_world_draw_state(
        backend,
        LLRenderWorldShaderClass::PointLight,
        make_deferred_point_light_volume_probe_parameters(width, height),
        width,
        height);
    backend.setWorldDeferredShaderLevel(3);
    backend.setCapability(LLRenderCapability::DepthTest, false);
    backend.setDepthWriteEnabled(false);
    backend.setCapability(LLRenderCapability::CullFace, false);
    backend.setCapability(LLRenderCapability::Blend, true);
    backend.setBlendState(
        {
            LLRenderBlendFactor::One,
            LLRenderBlendFactor::One,
            LLRenderBlendFactor::One,
            LLRenderBlendFactor::One,
        });
    bind_smoke_cube(backend, cube);
    for (U32 fan = 0; fan < 8; ++fan)
    {
        backend.drawElements(
            LLRenderPrimitiveType::TriangleFan,
            8,
            LLRenderIndexType::UnsignedShort,
            reinterpret_cast<const void*>(
                static_cast<uintptr_t>(fan * 8 * sizeof(U16))));
    }
    reset_smoke_world_draw_state(backend);
    backend.setWorldDeferredShaderLevel(1);

    for (U32 attachment = 0; attachment < attachment_count; ++attachment)
    {
        gGL.getTexUnit(static_cast<S32>(attachment))->unbind(LLTexUnit::TT_TEXTURE);
    }
    if (depth_bound)
    {
        gGL.getTexUnit(3)->unbind(LLTexUnit::TT_TEXTURE);
    }
    gGL.getTexUnit(4)->unbind(LLTexUnit::TT_TEXTURE);
}

void draw_smoke_deferred_spot_light_volume(
    LLRenderBackend& backend,
    LLRenderTarget& deferred_screen,
    const SmokeDeferredTextures& material_textures,
    const SmokeCube& cube,
    U32 width,
    U32 height)
{
    const U32 attachment_count =
        llmin(deferred_screen.getNumTextures(), 3U);
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
            gGL.getTexUnit(3)->bind(&deferred_screen, true);
    }
    backend.setActiveTextureUnit(4);
    backend.bindTexture(LLRenderTextureTarget::TextureCubeMap, material_textures.mCubeWhite);
    backend.setActiveTextureUnit(5);
    backend.bindTexture(LLRenderTextureTarget::Texture2D, material_textures.mWhite);
    backend.setActiveTextureUnit(6);
    backend.bindTexture(LLRenderTextureTarget::Texture2D, material_textures.mWhite);
    backend.setActiveTextureUnit(7);
    backend.bindTexture(LLRenderTextureTarget::Texture2D, material_textures.mWhite);
    backend.setActiveTextureUnit(0);

    set_smoke_fullscreen_world_draw_state(
        backend,
        LLRenderWorldShaderClass::SpotLight,
        make_deferred_spot_light_volume_probe_parameters(width, height),
        width,
        height);
    backend.setWorldDeferredShaderLevel(1);
    backend.setCapability(LLRenderCapability::DepthTest, false);
    backend.setDepthWriteEnabled(false);
    backend.setCapability(LLRenderCapability::CullFace, false);
    backend.setCapability(LLRenderCapability::Blend, true);
    backend.setBlendState(
        {
            LLRenderBlendFactor::One,
            LLRenderBlendFactor::One,
            LLRenderBlendFactor::One,
            LLRenderBlendFactor::One,
        });
    bind_smoke_cube(backend, cube);
    for (U32 fan = 0; fan < 8; ++fan)
    {
        backend.drawElements(
            LLRenderPrimitiveType::TriangleFan,
            8,
            LLRenderIndexType::UnsignedShort,
            reinterpret_cast<const void*>(
                static_cast<uintptr_t>(fan * 8 * sizeof(U16))));
    }
    reset_smoke_world_draw_state(backend);
    backend.setWorldDeferredShaderLevel(1);

    for (U32 attachment = 0; attachment < attachment_count; ++attachment)
    {
        gGL.getTexUnit(static_cast<S32>(attachment))->unbind(LLTexUnit::TT_TEXTURE);
    }
    if (depth_bound)
    {
        gGL.getTexUnit(3)->unbind(LLTexUnit::TT_TEXTURE);
    }
    gGL.getTexUnit(4)->unbind(LLTexUnit::TT_TEXTURE);
    gGL.getTexUnit(5)->unbind(LLTexUnit::TT_TEXTURE);
    gGL.getTexUnit(6)->unbind(LLTexUnit::TT_TEXTURE);
    gGL.getTexUnit(7)->unbind(LLTexUnit::TT_TEXTURE);
}

void draw_smoke_final_composite_quad(
    LLRenderBackend& backend,
    LLRenderTarget& source,
    LLRenderTarget& deferred_screen,
    const SmokeQuad& quad,
    U32 width,
    U32 height,
    const LLRenderWorldMaterialParameters* override_parameters = nullptr,
    LLRenderTarget* exposure_map = nullptr)
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
    const bool exposure_bound =
        exposure_map &&
        exposure_map->isComplete() &&
        (exposure_map->bindTexture(0, 6, LLTexUnit::TFO_POINT), true);

    LLRenderWorldMaterialParameters parameters =
        override_parameters ?
            *override_parameters :
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
    if (exposure_bound)
    {
        gGL.getTexUnit(6)->unbind(LLTexUnit::TT_TEXTURE);
    }
}

bool render_final_color_compare_frame(
    LLRenderBackend& backend,
    SmokeViewerRenderTargetGraph& graph,
    const SmokeQuad& quad,
    U32 width,
    U32 height)
{
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
            backend,
            graph,
            graph_width,
            graph_height,
            3,
            true))
    {
        return false;
    }

    const SmokeRGB linear_input { 0.18f, 0.36f, 0.72f };
    log_final_color_compare_reference(linear_input);

    graph.mDeferredLight.bindTarget();
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
    backend.setClearColor(
        linear_input.mRed,
        linear_input.mGreen,
        linear_input.mBlue,
        1.f);
    graph.mDeferredLight.clear(LL_RENDER_CLEAR_COLOR);
    graph.mDeferredLight.flush();

    backend.bindReadWriteFramebuffer(LLRenderFramebufferHandle());
    backend.restoreDefaultFramebufferBufferRouting();
    backend.setViewport(0, 0, static_cast<S32>(width), static_cast<S32>(height));
    backend.setScissor(0, 0, static_cast<S32>(width), static_cast<S32>(height));
    backend.setClearColor(0.f, 0.f, 0.f, 1.f);
    backend.clear(LL_RENDER_CLEAR_COLOR | LL_RENDER_CLEAR_DEPTH);

    LLRenderWorldMaterialParameters final_parameters =
        make_final_color_compare_parameters();
    draw_smoke_final_composite_quad(
        backend,
        graph.mDeferredLight,
        graph.mDeferredScreen,
        quad,
        width,
        height,
        &final_parameters,
        &graph.mExposureMap);
    return true;
}

bool render_viewer_deferred_color_compare_frame(
    LLRenderBackend& backend,
    SmokeDeferredTextures& material_textures,
    SmokeViewerRenderTargetGraph& graph,
    const SmokeQuad& quad,
    U32 width,
    U32 height,
    bool soften_state_probe = false,
    bool emissive_probe = false,
    bool reflection_probe = false,
    SmokeReflectionProbeResources* reflection_probe_resources = nullptr,
    bool hero_probe = false,
    SmokeSSRResources* ssr_resources = nullptr,
    bool ssr_probe = false)
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
            backend,
            graph,
            graph_width,
            graph_height,
            4,
            true))
    {
        return false;
    }

    graph.mDeferredScreen.bindTarget();
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
    backend.setClearColor(0.f, 0.f, 0.f, 0.f);
    graph.mDeferredScreen.clear(LL_RENDER_CLEAR_COLOR | LL_RENDER_CLEAR_DEPTH);
    if (ssr_probe)
    {
        draw_deferred_ssr_probe_gbuffer_scene(
            backend,
            material_textures,
            quad,
            graph_width,
            graph_height);
    }
    else
    {
        draw_deferred_color_compare_gbuffer_scene(
            backend,
            material_textures,
            quad,
            graph_width,
            graph_height,
            0.f,
            emissive_probe,
            reflection_probe);
    }
    if (soften_state_probe)
    {
        log_deferred_soften_state_probe_reference();
    }
    if (emissive_probe)
    {
        log_deferred_emissive_probe_reference();
    }
    if (reflection_probe)
    {
        if (reflection_probe_resources)
        {
            if (hero_probe)
            {
                log_deferred_hero_probe_reference();
            }
            else if (ssr_probe)
            {
                log_deferred_ssr_probe_reference();
            }
            else
            {
                log_deferred_real_reflection_probe_reference();
            }
        }
        else
        {
            log_deferred_reflection_probe_reference();
        }
    }
    graph.mDeferredScreen.flush();

    graph.mDeferredLight.bindTarget();
    backend.setClearColor(0.f, 0.f, 0.f, 1.f);
    graph.mDeferredLight.clear(LL_RENDER_CLEAR_COLOR);
    LLRenderWorldMaterialParameters deferred_parameters =
        (soften_state_probe || emissive_probe || reflection_probe) ?
            make_deferred_soften_state_probe_parameters(
                graph_width,
                graph_height) :
            make_deferred_color_compare_composite_parameters(
                graph_width,
                graph_height);
    if (hero_probe)
    {
        deferred_parameters.mCompositeClipPlane[3] = 1.f;
    }
    if (ssr_probe)
    {
        deferred_parameters.mCompositeSSR0[0] = 1.f;
        deferred_parameters.mCompositeSSR0[1] = 8.f;
        deferred_parameters.mCompositeSSR0[2] = 0.1f;
        deferred_parameters.mCompositeSSR0[3] = 10.f;
        deferred_parameters.mCompositeSSR1[0] = 0.f;
        deferred_parameters.mCompositeSSR1[1] = 1.f;
        deferred_parameters.mCompositeSSR1[2] = 1.25f;
        deferred_parameters.mCompositeSSR1[3] = 0.f;
        deferred_parameters.mCompositeInverseProjection[0] = 0.5f;
        deferred_parameters.mCompositeInverseProjection[5] = 0.5f;
    }
    if (soften_state_probe || emissive_probe || reflection_probe)
    {
        if (reflection_probe_resources &&
            !ensure_smoke_reflection_probe_resources(
                backend,
                *reflection_probe_resources,
                hero_probe))
        {
            return false;
        }
        if (ssr_probe &&
            (!ssr_resources ||
                !ensure_smoke_ssr_resources(
                    backend,
                    *ssr_resources,
                    graph_width,
                    graph_height)))
        {
            return false;
        }
        if (!draw_smoke_deferred_soften_stage(
            backend,
            graph,
            material_textures,
            quad,
            graph_width,
            graph_height,
            deferred_parameters,
            reflection_probe_resources,
            ssr_resources))
        {
            return false;
        }
    }
    else
    {
        draw_smoke_deferred_screen_composite_quad(
            backend,
            graph.mDeferredScreen,
            quad,
            graph_width,
            graph_height,
            &deferred_parameters);
    }
    graph.mDeferredLight.flush();

    copy_smoke_target_to_target(
        backend,
        graph.mDeferredLight,
        graph.mScreen,
        quad,
        graph_width,
        graph_height);
    copy_smoke_target_to_target(
        backend,
        graph.mScreen,
        graph.mDeferredLight,
        quad,
        graph_width,
        graph_height);

    graph.mPostPing.bindTarget();
    backend.setClearColor(0.f, 0.f, 0.f, 1.f);
    graph.mPostPing.clear(LL_RENDER_CLEAR_COLOR);
    LLRenderWorldMaterialParameters final_parameters =
        make_final_color_compare_parameters();
    draw_smoke_final_composite_quad(
        backend,
        graph.mDeferredLight,
        graph.mDeferredScreen,
        quad,
        graph_width,
        graph_height,
        &final_parameters,
        &graph.mExposureMap);
    graph.mPostPing.flush();

    copy_smoke_target_to_swapchain(
        backend,
        graph.mPostPing,
        quad,
        width,
        height);
    return true;
}

bool render_viewer_deferred_local_light_probe_frame(
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
            backend,
            graph,
            graph_width,
            graph_height,
            4,
            true))
    {
        return false;
    }

    graph.mDeferredScreen.bindTarget();
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
    backend.setClearColor(0.f, 0.f, 0.f, 0.f);
    graph.mDeferredScreen.clear(LL_RENDER_CLEAR_COLOR | LL_RENDER_CLEAR_DEPTH);
    draw_deferred_color_compare_gbuffer_scene(
        backend,
        material_textures,
        quad,
        graph_width,
        graph_height);
    log_deferred_local_light_probe_reference();
    graph.mDeferredScreen.flush();

    graph.mDeferredLight.bindTarget();
    backend.setClearColor(0.f, 0.f, 0.f, 1.f);
    graph.mDeferredLight.clear(LL_RENDER_CLEAR_COLOR);
    LLRenderWorldMaterialParameters deferred_parameters =
        make_deferred_soften_state_probe_parameters(
            graph_width,
            graph_height);
    if (!draw_smoke_deferred_soften_stage(
        backend,
        graph,
        material_textures,
        quad,
        graph_width,
        graph_height,
        deferred_parameters))
    {
        return false;
    }
    draw_smoke_deferred_local_light_quad(
        backend,
        graph.mDeferredScreen,
        material_textures,
        quad,
        graph_width,
        graph_height);
    graph.mDeferredLight.flush();

    copy_smoke_target_to_target(
        backend,
        graph.mDeferredLight,
        graph.mScreen,
        quad,
        graph_width,
        graph_height);
    copy_smoke_target_to_target(
        backend,
        graph.mScreen,
        graph.mDeferredLight,
        quad,
        graph_width,
        graph_height);

    graph.mPostPing.bindTarget();
    backend.setClearColor(0.f, 0.f, 0.f, 1.f);
    graph.mPostPing.clear(LL_RENDER_CLEAR_COLOR);
    LLRenderWorldMaterialParameters final_parameters =
        make_final_color_compare_parameters();
    draw_smoke_final_composite_quad(
        backend,
        graph.mDeferredLight,
        graph.mDeferredScreen,
        quad,
        graph_width,
        graph_height,
        &final_parameters,
        &graph.mExposureMap);
    graph.mPostPing.flush();

    copy_smoke_target_to_swapchain(
        backend,
        graph.mPostPing,
        quad,
        width,
        height);
    return true;
}

bool render_viewer_deferred_projector_light_probe_frame(
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
            backend,
            graph,
            graph_width,
            graph_height,
            4,
            true))
    {
        return false;
    }

    graph.mDeferredScreen.bindTarget();
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
    backend.setClearColor(0.f, 0.f, 0.f, 0.f);
    graph.mDeferredScreen.clear(LL_RENDER_CLEAR_COLOR | LL_RENDER_CLEAR_DEPTH);
    draw_deferred_color_compare_gbuffer_scene(
        backend,
        material_textures,
        quad,
        graph_width,
        graph_height);
    log_deferred_projector_light_probe_reference();
    graph.mDeferredScreen.flush();

    graph.mDeferredLight.bindTarget();
    backend.setClearColor(0.f, 0.f, 0.f, 1.f);
    graph.mDeferredLight.clear(LL_RENDER_CLEAR_COLOR);
    LLRenderWorldMaterialParameters deferred_parameters =
        make_deferred_soften_state_probe_parameters(
            graph_width,
            graph_height);
    if (!draw_smoke_deferred_soften_stage(
        backend,
        graph,
        material_textures,
        quad,
        graph_width,
        graph_height,
        deferred_parameters))
    {
        return false;
    }
    draw_smoke_deferred_projector_light_quad(
        backend,
        graph.mDeferredScreen,
        material_textures,
        quad,
        graph_width,
        graph_height);
    graph.mDeferredLight.flush();

    copy_smoke_target_to_target(
        backend,
        graph.mDeferredLight,
        graph.mScreen,
        quad,
        graph_width,
        graph_height);
    copy_smoke_target_to_target(
        backend,
        graph.mScreen,
        graph.mDeferredLight,
        quad,
        graph_width,
        graph_height);

    graph.mPostPing.bindTarget();
    backend.setClearColor(0.f, 0.f, 0.f, 1.f);
    graph.mPostPing.clear(LL_RENDER_CLEAR_COLOR);
    LLRenderWorldMaterialParameters final_parameters =
        make_final_color_compare_parameters();
    draw_smoke_final_composite_quad(
        backend,
        graph.mDeferredLight,
        graph.mDeferredScreen,
        quad,
        graph_width,
        graph_height,
        &final_parameters,
        &graph.mExposureMap);
    graph.mPostPing.flush();

    copy_smoke_target_to_swapchain(
        backend,
        graph.mPostPing,
        quad,
        width,
        height);
    return true;
}

bool render_viewer_deferred_volume_light_probe_frame(
    LLRenderBackend& backend,
    SmokeDeferredTextures& material_textures,
    SmokeViewerRenderTargetGraph& graph,
    const SmokeQuad& quad,
    const SmokeCube& cube,
    U32 width,
    U32 height,
    bool spot_volume)
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
            backend,
            graph,
            graph_width,
            graph_height,
            4,
            true))
    {
        return false;
    }

    graph.mDeferredScreen.bindTarget();
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
    backend.setClearColor(0.f, 0.f, 0.f, 0.f);
    graph.mDeferredScreen.clear(LL_RENDER_CLEAR_COLOR | LL_RENDER_CLEAR_DEPTH);
    draw_deferred_color_compare_gbuffer_scene(
        backend,
        material_textures,
        quad,
        graph_width,
        graph_height,
        0.7f);
    if (spot_volume)
    {
        log_deferred_spot_light_volume_probe_reference();
    }
    else
    {
        log_deferred_point_light_volume_probe_reference();
    }
    graph.mDeferredScreen.flush();

    graph.mDeferredLight.bindTarget();
    backend.setClearColor(0.f, 0.f, 0.f, 1.f);
    graph.mDeferredLight.clear(LL_RENDER_CLEAR_COLOR);
    LLRenderWorldMaterialParameters deferred_parameters =
        make_deferred_soften_state_probe_parameters(
            graph_width,
            graph_height);
    if (!draw_smoke_deferred_soften_stage(
        backend,
        graph,
        material_textures,
        quad,
        graph_width,
        graph_height,
        deferred_parameters))
    {
        return false;
    }
    if (spot_volume)
    {
        draw_smoke_deferred_spot_light_volume(
            backend,
            graph.mDeferredScreen,
            material_textures,
            cube,
            graph_width,
            graph_height);
    }
    else
    {
        draw_smoke_deferred_point_light_volume(
            backend,
            graph.mDeferredScreen,
            material_textures,
            cube,
            graph_width,
            graph_height);
    }
    graph.mDeferredLight.flush();

    copy_smoke_target_to_target(
        backend,
        graph.mDeferredLight,
        graph.mScreen,
        quad,
        graph_width,
        graph_height);
    copy_smoke_target_to_target(
        backend,
        graph.mScreen,
        graph.mDeferredLight,
        quad,
        graph_width,
        graph_height);

    graph.mPostPing.bindTarget();
    backend.setClearColor(0.f, 0.f, 0.f, 1.f);
    graph.mPostPing.clear(LL_RENDER_CLEAR_COLOR);
    LLRenderWorldMaterialParameters final_parameters =
        make_final_color_compare_parameters();
    draw_smoke_final_composite_quad(
        backend,
        graph.mDeferredLight,
        graph.mDeferredScreen,
        quad,
        graph_width,
        graph_height,
        &final_parameters,
        &graph.mExposureMap);
    graph.mPostPing.flush();

    copy_smoke_target_to_swapchain(
        backend,
        graph.mPostPing,
        quad,
        width,
        height);
    return true;
}

bool render_viewer_deferred_lightmap_blur_probe_frame(
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
            backend,
            graph,
            graph_width,
            graph_height,
            4,
            true))
    {
        return false;
    }

    graph.mDeferredScreen.bindTarget();
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
    backend.setClearColor(0.f, 0.f, 0.f, 0.f);
    graph.mDeferredScreen.clear(LL_RENDER_CLEAR_COLOR | LL_RENDER_CLEAR_DEPTH);
    draw_deferred_color_compare_gbuffer_scene(
        backend,
        material_textures,
        quad,
        graph_width,
        graph_height);
    log_deferred_lightmap_blur_probe_reference();
    graph.mDeferredScreen.flush();

    if (!ensure_smoke_deferred_lightmap_source(
            backend,
            graph,
            graph_width,
            graph_height))
    {
        return false;
    }

    draw_smoke_deferred_lightmap_blur_quad(
        backend,
        graph.mLightMapSource,
        graph.mScreen,
        graph.mDeferredScreen,
        quad,
        LLVector2(1.f, 0.f),
        graph_width,
        graph_height);
    draw_smoke_deferred_lightmap_blur_quad(
        backend,
        LLRenderTextureHandle(graph.mScreen.getTexture(0)),
        graph.mDeferredLight,
        graph.mDeferredScreen,
        quad,
        LLVector2(0.f, 1.f),
        graph_width,
        graph_height);

    copy_smoke_target_to_swapchain(
        backend,
        graph.mDeferredLight,
        quad,
        width,
        height);
    return true;
}

bool render_terrain_final_probe_frame(
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
            backend,
            graph,
            graph_width,
            graph_height,
            4,
            true))
    {
        return false;
    }

    graph.mDeferredScreen.bindTarget();
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
    backend.setClearColor(0.f, 0.f, 0.f, 0.f);
    graph.mDeferredScreen.clear(LL_RENDER_CLEAR_COLOR | LL_RENDER_CLEAR_DEPTH);
    draw_terrain_final_probe_gbuffer_scene(
        backend,
        material_textures,
        quad,
        graph_width,
        graph_height);
    graph.mDeferredScreen.flush();

    graph.mDeferredLight.bindTarget();
    backend.setClearColor(0.f, 0.f, 0.f, 1.f);
    graph.mDeferredLight.clear(LL_RENDER_CLEAR_COLOR);
    LLRenderWorldMaterialParameters deferred_parameters =
        make_deferred_color_compare_composite_parameters(
            graph_width,
            graph_height);
    draw_smoke_deferred_screen_composite_quad(
        backend,
        graph.mDeferredScreen,
        quad,
        graph_width,
        graph_height,
        &deferred_parameters);
    graph.mDeferredLight.flush();

    copy_smoke_target_to_target(
        backend,
        graph.mDeferredLight,
        graph.mScreen,
        quad,
        graph_width,
        graph_height);
    copy_smoke_target_to_target(
        backend,
        graph.mScreen,
        graph.mDeferredLight,
        quad,
        graph_width,
        graph_height);

    graph.mPostPing.bindTarget();
    backend.setClearColor(0.f, 0.f, 0.f, 1.f);
    graph.mPostPing.clear(LL_RENDER_CLEAR_COLOR);
    LLRenderWorldMaterialParameters final_parameters =
        make_final_color_compare_parameters();
    draw_smoke_final_composite_quad(
        backend,
        graph.mDeferredLight,
        graph.mDeferredScreen,
        quad,
        graph_width,
        graph_height,
        &final_parameters,
        &graph.mExposureMap);
    graph.mPostPing.flush();

    copy_smoke_target_to_swapchain(
        backend,
        graph.mPostPing,
        quad,
        width,
        height);
    return true;
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
            backend,
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
            graph_height,
            nullptr,
            &graph.mExposureMap);
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

int run_opengl_shader_reference_ppm(const SmokeOptions& options)
{
    const std::string shader_case =
        normalize_shader_case_name(options.mShaderCase);
    if (!options.mShaderCaseExplicit ||
        (shader_case != "copy" &&
            shader_case != "haze" &&
            shader_case != "alpha"))
    {
        std::cerr
            << "--opengl-reference-ppm currently supports only "
            << "--shader-case copy, --shader-case haze, or --shader-case alpha.\n";
        return 1;
    }

    setenv("MARE_RENDER_BACKEND", "opengl", 1);

    void* window = mare_vulkan_smoke_create_window(
        960,
        540,
        "Mare OpenGL Shader Reference");
    if (!window)
    {
        std::cerr << "Failed to create OpenGL reference window.\n";
        return 1;
    }

    LLRenderBackend& backend = getRenderBackend();
    if (backend.getType() != LLRenderBackendType::OpenGL)
    {
        std::cerr
            << "Expected OpenGL backend, got "
            << backend.getName()
            << ".\n";
        mare_vulkan_smoke_destroy_window(window);
        return 2;
    }

    LLRenderNativeContext context;
    LLRenderNativeContextDesc desc;
    desc.mWindow = window;
    desc.mSamples = 0;
    desc.mEnableVSync = false;

    if (!backend.createNativeContext(desc, context))
    {
        std::cerr << "Failed to create OpenGL native context.\n";
        mare_vulkan_smoke_destroy_window(window);
        return 3;
    }

    if (!backend.makeNativeContextCurrent(context.mContext))
    {
        std::cerr << "Failed to make OpenGL native context current.\n";
        backend.destroyNativeContext(context);
        mare_vulkan_smoke_destroy_window(window);
        return 4;
    }

    backend.initPlatformContextExtensions();
    if (!backend.initContextCapabilities())
    {
        std::cerr << "Failed to initialize OpenGL context capabilities.\n";
        backend.destroyNativeContext(context);
        mare_vulkan_smoke_destroy_window(window);
        return 5;
    }

    unsigned int width = 1;
    unsigned int height = 1;
    mare_vulkan_smoke_get_view_size(context.mView, &width, &height);
    bool rendered = false;
    if (shader_case == "copy")
    {
        rendered =
            render_opengl_copy_reference_ppm(
                backend,
                options.mOpenGLReferencePPMPath,
                width,
                height);
    }
    else if (shader_case == "haze")
    {
        rendered =
            render_opengl_haze_reference_ppm(
                backend,
                options.mOpenGLReferencePPMPath,
                width,
                height);
    }
    else
    {
        rendered =
            render_opengl_alpha_reference_ppm(
                backend,
                options.mOpenGLReferencePPMPath,
                width,
                height);
    }

    backend.shutdownContextCapabilities();
    backend.destroyNativeContext(context);
    mare_vulkan_smoke_destroy_window(window);
    return rendered ? 0 : 6;
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
    gForceSmokeVolumeLightOutput = options.mForceVolumeLightOutput;
    if (options.mHelp)
    {
        print_smoke_usage(argv[0] ? argv[0] : "mare-vulkan-smoke");
        return 0;
    }
    if (options.mListShaderCases)
    {
        print_shader_probe_cases();
        return 0;
    }
    if (options.mListShaderParity)
    {
        return print_shader_parity_entries(
            options.mShaderCaseExplicit ? options.mShaderCase : std::string()) ? 0 : 1;
    }
    if (options.mListPipelineContracts)
    {
        return print_world_pipeline_contract_entries() ? 0 : 1;
    }
    if (!options.mOpenGLReferencePPMPath.empty())
    {
        return run_opengl_shader_reference_ppm(options);
    }
    if (options.mShaderCaseExplicit &&
        options.mOpenGLReferencePPMPath.empty() &&
        options.mMode != SmokeMode::ShaderProbe)
    {
        std::cerr << "--shader-case requires --mode shader-probe.\n";
        return 1;
    }
    if (options.mMode == SmokeMode::ShaderProbe)
    {
        SmokeWorldPipelineEntry selected_case;
        if (!find_shader_probe_case(options.mShaderCase, selected_case))
        {
            std::cerr
                << "Unknown shader-probe case '"
                << options.mShaderCase
                << "'.\n";
            print_shader_probe_cases();
            return 1;
        }
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
    if (options.mMode == SmokeMode::Class1GBufferColorProbe &&
        options.mScene == SmokeScene::TwoPrims)
    {
        std::cerr
            << "--mode class1-gbuffer-color-probe supports --scene basic or "
            << "post-overlays-stress. Use a viewer-staged mode for two-prims."
            << std::endl;
        return 1;
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
    unsetenv("MARE_VULKAN_SMOKE_VALIDATION_FAILED");
    unsetenv("MARE_VULKAN_SMOKE_EXPECT_FINAL_RGB");
    unsetenv("MARE_VULKAN_SMOKE_EXPECT_FINAL_RGB_TOLERANCE");
    unsetenv("MARE_VULKAN_SMOKE_EXPECT_DEFERRED_COMPOSITE_RGB");
    unsetenv("MARE_VULKAN_SMOKE_EXPECT_DEFERRED_COMPOSITE_RGB_TOLERANCE");
    auto set_expected_rgb = [](const char* key, SmokeRGB rgb)
    {
        std::ostringstream expected_rgb;
        expected_rgb
            << std::fixed
            << std::setprecision(6)
            << rgb.mRed
            << ","
            << rgb.mGreen
            << ","
            << rgb.mBlue;
        setenv(key, expected_rgb.str().c_str(), 1);
    };
    if (options.mMode == SmokeMode::FinalColorCompare)
    {
        set_expected_rgb(
            "MARE_VULKAN_SMOKE_EXPECT_FINAL_RGB",
            smoke_linear_to_srgb({ 0.18f, 0.36f, 0.72f }));
        setenv("MARE_VULKAN_SMOKE_EXPECT_FINAL_RGB_TOLERANCE", "0.02", 1);
    }
    else if (options.mMode == SmokeMode::ViewerDeferredSoftenStateProbe)
    {
        set_expected_rgb(
            "MARE_VULKAN_SMOKE_EXPECT_DEFERRED_COMPOSITE_RGB",
            { 0.6978f, 0.5708f, 0.4602f });
        setenv(
            "MARE_VULKAN_SMOKE_EXPECT_DEFERRED_COMPOSITE_RGB_TOLERANCE",
            "0.03",
            1);
        set_expected_rgb(
            "MARE_VULKAN_SMOKE_EXPECT_FINAL_RGB",
            { 0.8549f, 0.7804f, 0.7098f });
        setenv("MARE_VULKAN_SMOKE_EXPECT_FINAL_RGB_TOLERANCE", "0.03", 1);
    }
    else if (options.mMode == SmokeMode::ViewerDeferredEmissiveProbe)
    {
        set_expected_rgb(
            "MARE_VULKAN_SMOKE_EXPECT_DEFERRED_COMPOSITE_RGB",
            { 0.7534f, 1.6709f, 0.5703f });
        setenv(
            "MARE_VULKAN_SMOKE_EXPECT_DEFERRED_COMPOSITE_RGB_TOLERANCE",
            "0.05",
            1);
        set_expected_rgb(
            "MARE_VULKAN_SMOKE_EXPECT_FINAL_RGB",
            { 0.8824f, 1.0000f, 0.7804f });
        setenv("MARE_VULKAN_SMOKE_EXPECT_FINAL_RGB_TOLERANCE", "0.03", 1);
    }
    else if (options.mMode == SmokeMode::ViewerDeferredReflectionProbe)
    {
        set_expected_rgb(
            "MARE_VULKAN_SMOKE_EXPECT_DEFERRED_COMPOSITE_RGB",
            { 0.6040f, 0.6313f, 0.7041f });
        setenv(
            "MARE_VULKAN_SMOKE_EXPECT_DEFERRED_COMPOSITE_RGB_TOLERANCE",
            "0.05",
            1);
        set_expected_rgb(
            "MARE_VULKAN_SMOKE_EXPECT_FINAL_RGB",
            { 0.8000f, 0.8157f, 0.8549f });
        setenv("MARE_VULKAN_SMOKE_EXPECT_FINAL_RGB_TOLERANCE", "0.03", 1);
    }
    else if (options.mMode == SmokeMode::ViewerDeferredRealReflectionProbe)
    {
        set_expected_rgb(
            "MARE_VULKAN_SMOKE_EXPECT_DEFERRED_COMPOSITE_RGB",
            { 0.3252f, 0.9336f, 2.0098f });
        setenv(
            "MARE_VULKAN_SMOKE_EXPECT_DEFERRED_COMPOSITE_RGB_TOLERANCE",
            "0.05",
            1);
        set_expected_rgb(
            "MARE_VULKAN_SMOKE_EXPECT_FINAL_RGB",
            { 0.6039f, 0.9686f, 1.0000f });
        setenv("MARE_VULKAN_SMOKE_EXPECT_FINAL_RGB_TOLERANCE", "0.03", 1);
    }
    else if (options.mMode == SmokeMode::ViewerDeferredHeroProbe)
    {
        set_expected_rgb(
            "MARE_VULKAN_SMOKE_EXPECT_DEFERRED_COMPOSITE_RGB",
            { 0.3994f, 1.5303f, 1.4961f });
        setenv(
            "MARE_VULKAN_SMOKE_EXPECT_DEFERRED_COMPOSITE_RGB_TOLERANCE",
            "0.05",
            1);
        set_expected_rgb(
            "MARE_VULKAN_SMOKE_EXPECT_FINAL_RGB",
            { 0.6667f, 1.0000f, 1.0000f });
        setenv("MARE_VULKAN_SMOKE_EXPECT_FINAL_RGB_TOLERANCE", "0.03", 1);
    }
    else if (options.mMode == SmokeMode::ViewerDeferredSSRProbe)
    {
        unsetenv("MARE_VULKAN_SMOKE_EXPECT_DEFERRED_COMPOSITE_RGB");
        unsetenv("MARE_VULKAN_SMOKE_EXPECT_DEFERRED_COMPOSITE_RGB_TOLERANCE");
        unsetenv("MARE_VULKAN_SMOKE_EXPECT_FINAL_RGB");
        unsetenv("MARE_VULKAN_SMOKE_EXPECT_FINAL_RGB_TOLERANCE");
    }
    else if (options.mMode == SmokeMode::ViewerDeferredLocalLightProbe)
    {
        set_expected_rgb(
            "MARE_VULKAN_SMOKE_EXPECT_DEFERRED_COMPOSITE_RGB",
            { 0.8725f, 0.6628f, 0.5037f });
        setenv(
            "MARE_VULKAN_SMOKE_EXPECT_DEFERRED_COMPOSITE_RGB_TOLERANCE",
            "0.05",
            1);
        set_expected_rgb(
            "MARE_VULKAN_SMOKE_EXPECT_FINAL_RGB",
            { 0.9347f, 0.8322f, 0.7374f });
        setenv("MARE_VULKAN_SMOKE_EXPECT_FINAL_RGB_TOLERANCE", "0.03", 1);
    }
    else if (options.mMode == SmokeMode::ViewerDeferredProjectorLightProbe)
    {
        set_expected_rgb(
            "MARE_VULKAN_SMOKE_EXPECT_DEFERRED_COMPOSITE_RGB",
            { 0.7443f, 0.6649f, 0.6097f });
        setenv(
            "MARE_VULKAN_SMOKE_EXPECT_DEFERRED_COMPOSITE_RGB_TOLERANCE",
            "0.05",
            1);
        set_expected_rgb(
            "MARE_VULKAN_SMOKE_EXPECT_FINAL_RGB",
            { 0.8777f, 0.8324f, 0.7935f });
        setenv("MARE_VULKAN_SMOKE_EXPECT_FINAL_RGB_TOLERANCE", "0.03", 1);
    }
    else if (options.mMode == SmokeMode::ViewerDeferredPointLightVolumeProbe)
    {
        set_expected_rgb(
            "MARE_VULKAN_SMOKE_EXPECT_DEFERRED_COMPOSITE_RGB",
            { 0.7240f, 0.5697f, 0.4683f });
        setenv(
            "MARE_VULKAN_SMOKE_EXPECT_DEFERRED_COMPOSITE_RGB_TOLERANCE",
            "0.05",
            1);
        set_expected_rgb(
            "MARE_VULKAN_SMOKE_EXPECT_FINAL_RGB",
            { 0.8554f, 0.7788f, 0.7139f });
        setenv("MARE_VULKAN_SMOKE_EXPECT_FINAL_RGB_TOLERANCE", "0.03", 1);
    }
    else if (options.mMode == SmokeMode::ViewerDeferredSpotLightVolumeProbe)
    {
        set_expected_rgb(
            "MARE_VULKAN_SMOKE_EXPECT_DEFERRED_COMPOSITE_RGB",
            { 0.7217f, 0.6177f, 0.6861f });
        setenv(
            "MARE_VULKAN_SMOKE_EXPECT_DEFERRED_COMPOSITE_RGB_TOLERANCE",
            "0.05",
            1);
        set_expected_rgb(
            "MARE_VULKAN_SMOKE_EXPECT_FINAL_RGB",
            { 0.8653f, 0.8027f, 0.7667f });
        setenv("MARE_VULKAN_SMOKE_EXPECT_FINAL_RGB_TOLERANCE", "0.03", 1);
    }
    else if (options.mMode == SmokeMode::ViewerDeferredLightMapBlurProbe)
    {
        set_expected_rgb(
            "MARE_VULKAN_SMOKE_EXPECT_DEFERRED_COMPOSITE_RGB",
            { 0.4000f, 0.6431f, 0.4902f });
        setenv(
            "MARE_VULKAN_SMOKE_EXPECT_DEFERRED_COMPOSITE_RGB_TOLERANCE",
            "0.02",
            1);
        set_expected_rgb(
            "MARE_VULKAN_SMOKE_EXPECT_FINAL_RGB",
            { 0.4000f, 0.6431f, 0.4902f });
        setenv("MARE_VULKAN_SMOKE_EXPECT_FINAL_RGB_TOLERANCE", "0.02", 1);
    }
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
        smoke_mode == SmokeMode::Class1GBufferColorProbe ||
        smoke_mode == SmokeMode::TerrainFinalProbe ||
        smoke_mode == SmokeMode::FinalColorCompare ||
        smoke_mode == SmokeMode::ViewerDeferredColorCompare ||
        smoke_mode == SmokeMode::ViewerDeferredSoftenStateProbe ||
        smoke_mode == SmokeMode::ViewerDeferredEmissiveProbe ||
        smoke_mode == SmokeMode::ViewerDeferredReflectionProbe ||
        smoke_mode == SmokeMode::ViewerDeferredRealReflectionProbe ||
        smoke_mode == SmokeMode::ViewerDeferredHeroProbe ||
        smoke_mode == SmokeMode::ViewerDeferredSSRProbe ||
        smoke_mode == SmokeMode::ViewerDeferredLocalLightProbe ||
        smoke_mode == SmokeMode::ViewerDeferredProjectorLightProbe ||
        smoke_mode == SmokeMode::ViewerDeferredPointLightVolumeProbe ||
        smoke_mode == SmokeMode::ViewerDeferredSpotLightVolumeProbe ||
        smoke_mode == SmokeMode::ViewerDeferredLightMapBlurProbe ||
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
    SmokeCube smoke_cube;
    SmokeOffscreen smoke_offscreen;
    SmokeDeferredTextures smoke_deferred_textures;
    SmokeDeferredGraph smoke_deferred_graph;
    SmokeViewerRenderTargetGraph smoke_viewer_render_target_graph;
    SmokeReflectionProbeResources smoke_reflection_probe_resources;
    SmokeSSRResources smoke_ssr_resources;
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
            smoke_mode == SmokeMode::WorldPipelines ||
            smoke_mode == SmokeMode::ShaderProbe ||
            smoke_mode == SmokeMode::ShaderSuite ||
            smoke_mode == SmokeMode::Class1GBufferColorProbe ||
            smoke_mode == SmokeMode::TerrainFinalProbe ||
            smoke_mode == SmokeMode::FinalColorCompare ||
            smoke_mode == SmokeMode::ViewerDeferredColorCompare ||
            smoke_mode == SmokeMode::ViewerDeferredSoftenStateProbe ||
            smoke_mode == SmokeMode::ViewerDeferredEmissiveProbe ||
            smoke_mode == SmokeMode::ViewerDeferredReflectionProbe ||
            smoke_mode == SmokeMode::ViewerDeferredRealReflectionProbe ||
            smoke_mode == SmokeMode::ViewerDeferredHeroProbe ||
            smoke_mode == SmokeMode::ViewerDeferredSSRProbe ||
            smoke_mode == SmokeMode::ViewerDeferredLocalLightProbe ||
            smoke_mode == SmokeMode::ViewerDeferredProjectorLightProbe ||
            smoke_mode == SmokeMode::ViewerDeferredPointLightVolumeProbe ||
            smoke_mode == SmokeMode::ViewerDeferredSpotLightVolumeProbe ||
            smoke_mode == SmokeMode::ViewerDeferredLightMapBlurProbe) &&
        !create_smoke_quad(
            backend,
            smoke_quad,
            {{ 255, 255, 255, 255 }},
            0.f,
            smoke_mode == SmokeMode::ViewerDeferredSSRProbe ? -1.f : 1.f))
    {
        std::cerr << "Failed to create Vulkan smoke quad.\n";
        backend.destroyNativeContext(context);
        mare_vulkan_smoke_destroy_window(window);
        return 5;
    }
    if ((smoke_mode == SmokeMode::ViewerDeferredPointLightVolumeProbe ||
            smoke_mode == SmokeMode::ViewerDeferredSpotLightVolumeProbe) &&
        !create_smoke_cube(backend, smoke_cube))
    {
        std::cerr << "Failed to create Vulkan smoke cube.\n";
        backend.destroyNativeContext(context);
        mare_vulkan_smoke_destroy_window(window);
        return 5;
    }
    if ((smoke_mode == SmokeMode::ViewerDeferredRealReflectionProbe ||
            smoke_mode == SmokeMode::ViewerDeferredHeroProbe ||
            smoke_mode == SmokeMode::ViewerDeferredSSRProbe) &&
        !ensure_smoke_reflection_probe_resources(
            backend,
            smoke_reflection_probe_resources,
            smoke_mode == SmokeMode::ViewerDeferredHeroProbe))
    {
        std::cerr << "Failed to create Vulkan smoke reflection-probe resources.\n";
        release_smoke_reflection_probe_resources(
            backend,
            smoke_reflection_probe_resources);
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
    if (smoke_mode == SmokeMode::ShaderProbe)
    {
        std::cout
            << "Mare Vulkan smoke shader case selected: "
            << options.mShaderCase
            << std::endl;
    }
    else if (smoke_mode == SmokeMode::ShaderSuite)
    {
        std::cout
            << "Mare Vulkan smoke shader suite selected: runtime shader cases "
            << "will be rendered fullscreen, one case per frame."
            << std::endl;
    }
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
        else if (smoke_mode == SmokeMode::FinalColorCompare)
        {
            if (!render_final_color_compare_frame(
                    backend,
                    smoke_viewer_render_target_graph,
                    smoke_quad,
                    width,
                    height))
            {
                std::cerr << "Failed to render Vulkan smoke final-color-compare frame.\n";
                break;
            }
        }
        else if (smoke_mode == SmokeMode::Class1GBufferColorProbe)
        {
            if (!render_class1_gbuffer_color_probe_frame(
                    backend,
                    smoke_deferred_textures,
                    smoke_viewer_render_target_graph,
                    smoke_quad,
                    smoke_scene,
                    width,
                    height,
                    options.mReferencePPMPath))
            {
                std::cerr << "Failed to render Vulkan smoke class1-gbuffer-color-probe frame.\n";
                break;
            }
        }
        else if (smoke_mode == SmokeMode::TerrainFinalProbe)
        {
            if (!render_terrain_final_probe_frame(
                    backend,
                    smoke_deferred_textures,
                    smoke_viewer_render_target_graph,
                    smoke_quad,
                    width,
                    height))
            {
                std::cerr << "Failed to render Vulkan smoke terrain-final-probe frame.\n";
                break;
            }
        }
        else if (smoke_mode == SmokeMode::ViewerDeferredColorCompare ||
            smoke_mode == SmokeMode::ViewerDeferredSoftenStateProbe ||
            smoke_mode == SmokeMode::ViewerDeferredEmissiveProbe ||
            smoke_mode == SmokeMode::ViewerDeferredReflectionProbe ||
            smoke_mode == SmokeMode::ViewerDeferredRealReflectionProbe ||
            smoke_mode == SmokeMode::ViewerDeferredHeroProbe ||
            smoke_mode == SmokeMode::ViewerDeferredSSRProbe)
        {
            const bool reflection_probe_mode =
                smoke_mode == SmokeMode::ViewerDeferredReflectionProbe ||
                smoke_mode == SmokeMode::ViewerDeferredRealReflectionProbe ||
                smoke_mode == SmokeMode::ViewerDeferredHeroProbe ||
                smoke_mode == SmokeMode::ViewerDeferredSSRProbe;
            const bool real_probe_resources =
                smoke_mode == SmokeMode::ViewerDeferredRealReflectionProbe ||
                smoke_mode == SmokeMode::ViewerDeferredHeroProbe ||
                smoke_mode == SmokeMode::ViewerDeferredSSRProbe;
            const bool hero_probe =
                smoke_mode == SmokeMode::ViewerDeferredHeroProbe;
            const bool ssr_probe =
                smoke_mode == SmokeMode::ViewerDeferredSSRProbe;
            if (!render_viewer_deferred_color_compare_frame(
                    backend,
                    smoke_deferred_textures,
                    smoke_viewer_render_target_graph,
                    smoke_quad,
                    width,
                    height,
                    smoke_mode == SmokeMode::ViewerDeferredSoftenStateProbe,
                    smoke_mode == SmokeMode::ViewerDeferredEmissiveProbe,
                    reflection_probe_mode,
                    real_probe_resources ?
                        &smoke_reflection_probe_resources :
                        nullptr,
                    hero_probe,
                    ssr_probe ? &smoke_ssr_resources : nullptr,
                    ssr_probe))
            {
                std::cerr
                    << "Failed to render Vulkan smoke "
                    << get_smoke_mode_name(smoke_mode)
                    << " frame.\n";
                break;
            }
        }
        else if (smoke_mode == SmokeMode::ViewerDeferredLocalLightProbe)
        {
            if (!render_viewer_deferred_local_light_probe_frame(
                    backend,
                    smoke_deferred_textures,
                    smoke_viewer_render_target_graph,
                    smoke_quad,
                    width,
                    height))
            {
                std::cerr
                    << "Failed to render Vulkan smoke "
                    << get_smoke_mode_name(smoke_mode)
                    << " frame.\n";
                break;
            }
        }
        else if (smoke_mode == SmokeMode::ViewerDeferredProjectorLightProbe)
        {
            if (!render_viewer_deferred_projector_light_probe_frame(
                    backend,
                    smoke_deferred_textures,
                    smoke_viewer_render_target_graph,
                    smoke_quad,
                    width,
                    height))
            {
                std::cerr
                    << "Failed to render Vulkan smoke "
                    << get_smoke_mode_name(smoke_mode)
                    << " frame.\n";
                break;
            }
        }
        else if (smoke_mode == SmokeMode::ViewerDeferredPointLightVolumeProbe ||
            smoke_mode == SmokeMode::ViewerDeferredSpotLightVolumeProbe)
        {
            if (!render_viewer_deferred_volume_light_probe_frame(
                    backend,
                    smoke_deferred_textures,
                    smoke_viewer_render_target_graph,
                    smoke_quad,
                    smoke_cube,
                    width,
                    height,
                    smoke_mode == SmokeMode::ViewerDeferredSpotLightVolumeProbe))
            {
                std::cerr
                    << "Failed to render Vulkan smoke "
                    << get_smoke_mode_name(smoke_mode)
                    << " frame.\n";
                break;
            }
        }
        else if (smoke_mode == SmokeMode::ViewerDeferredLightMapBlurProbe)
        {
            if (!render_viewer_deferred_lightmap_blur_probe_frame(
                    backend,
                    smoke_deferred_textures,
                    smoke_viewer_render_target_graph,
                    smoke_quad,
                    width,
                    height))
            {
                std::cerr
                    << "Failed to render Vulkan smoke "
                    << get_smoke_mode_name(smoke_mode)
                    << " frame.\n";
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
        else if (smoke_mode == SmokeMode::ShaderProbe)
        {
            if (!render_shader_probe_frame(
                    backend,
                    smoke_deferred_textures,
                    smoke_quad,
                    options.mShaderCase,
                    width,
                    height))
            {
                std::cerr << "Failed to render Vulkan smoke shader-probe frame.\n";
                break;
            }
        }
        else if (smoke_mode == SmokeMode::ShaderSuite)
        {
            if (!render_shader_suite_frame(
                    backend,
                    smoke_deferred_textures,
                    smoke_quad,
                    frame,
                    width,
                    height))
            {
                std::cerr << "Failed to render Vulkan smoke shader-suite frame.\n";
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
    release_smoke_reflection_probe_resources(backend, smoke_reflection_probe_resources);
    release_smoke_ssr_resources(backend, smoke_ssr_resources);
    release_smoke_viewer_render_target_graph(backend, smoke_viewer_render_target_graph);
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
    return std::getenv("MARE_VULKAN_SMOKE_VALIDATION_FAILED") ? 6 : 0;
}
