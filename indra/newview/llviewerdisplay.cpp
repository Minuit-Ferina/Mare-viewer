/**
 * @file llviewerdisplay.cpp
 * @brief LLViewerDisplay class implementation
 *
 * $LicenseInfo:firstyear=2004&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2010, Linden Research, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation;
 * version 2.1 of the License only.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * Linden Research, Inc., 945 Battery Street, San Francisco, CA  94111  USA
 * $/LicenseInfo$
 */

#include "llviewerprecompiledheaders.h"

#include "llviewerdisplay.h"

#include "fsyspath.h"
#include "hexdump.h"
#include "llagent.h"
#include "llagentcamera.h"
#include "llappviewer.h"
#include "llcoord.h"
#include "llcriticaldamp.h"
#include "llcubemap.h"
#include "lldir.h"
#include "lldrawpoolalpha.h"
#include "lldrawpoolbump.h"
#include "lldrawpoolwater.h"
#include "lldynamictexture.h"
#include "llenvironment.h"
#include "llfasttimer.h"
#include "llfeaturemanager.h"
#include "llfloatertools.h"
#include "llfocusmgr.h"

#include "llrenderbackend.h"

#include "llgltfmateriallist.h"
#include "llhudmanager.h"
#include "llimagepng.h"
#include "llmachineid.h"
#include "llmemory.h"
#include "llparcel.h"
#include "llperfstats.h"
#include "llpostprocess.h"
#include "llrender.h"
#include "llscenemonitor.h"
#include "llsdjson.h"
#include "llselectmgr.h"
#include "llsky.h"
#include "llspatialpartition.h"
#include "llstartup.h"
#include "llstartup.h"
#include "lltooldraganddrop.h"
#include "lltoolfocus.h"
#include "lltoolmgr.h"
#include "lltoolpie.h"
#include "lltracker.h"
#include "lltrans.h"
#include "llui.h"
#include "lluuid.h"
#include "llversioninfo.h"
#include "llviewercamera.h"
#include "llviewercontrol.h"
#include "llviewernetwork.h"
#include "llviewerobjectlist.h"
#include "llviewerparcelmgr.h"
#include "llviewerregion.h"
#include "llviewershadermgr.h"
#include "llviewertexturelist.h"
#include "llviewerwindow.h"
#include "llvoavatarself.h"
#include "llvograss.h"
#include "llvertexbuffer.h"
#include "llworld.h"
#include "pipeline.h"

#include <boost/json.hpp>

#include <array>
#include <filesystem>
#include <iomanip>
#include <set>
#include <sstream>
#include <vector>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "kokuarlvextras.h"
#include "llrenderstate.h"
#include "llrendercontext.h"

extern LLPointer<LLViewerTexture> gStartTexture;
extern bool gShiftFrame;

LLPointer<LLViewerTexture> gDisconnectedImagep = nullptr;

// used to toggle renderer back on after teleport
bool         gTeleportDisplay = false;
LLFrameTimer gTeleportDisplayTimer;
LLFrameTimer gTeleportArrivalTimer;
constexpr F32 RESTORE_GL_TIME = 5.f;  // Wait this long while reloading textures before we raise the curtain
// <FS:Ansariel> FIRE-12004: Attachments getting lost on TP
LLFrameTimer gPostTeleportFinishKillObjectDelayTimer;

bool gForceRenderLandFence = false;
bool gDisplaySwapBuffers = false;
bool gDepthDirty = false;
bool gResizeScreenTexture = false;
bool gResizeShadowTexture = false;
bool gWindowResized = false;
bool gSnapshot = false;
bool gCubeSnapshot = false;
bool gSnapshotNoPost = false;
bool gShaderProfileFrame = false;

// This is how long the sim will try to teleport you before giving up.
constexpr F32 TELEPORT_EXPIRY = 15.0f;
// Additional time (in seconds) to wait per attachment
constexpr F32 TELEPORT_EXPIRY_PER_ATTACHMENT = 3.f;

U32 gRecentFrameCount = 0; // number of 'recent' frames
LLFrameTimer gRecentFPSTime;
LLFrameTimer gRecentMemoryTime;
LLFrameTimer gAssetStorageLogTime;
LLFrameTimer gRecentRLVLogTime;

// Rendering stuff
void render_ui(F32 zoom_factor = 1.f, int subfield = 0);
void swap();
void render_hud_attachments();
void render_ui_3d();
void render_ui_2d();
void render_disconnected_background();
static void render_ui_internal(F32 zoom_factor, int subfield, bool finalize_scene);

void getProfileStatsContext(boost::json::object& stats);
std::string getProfileStatsFilename();

static bool use_vulkan_world_path()
{
    return getRenderBackend().getType() == LLRenderBackendType::Vulkan &&
        getRenderBackend().isReady();
}

static bool use_vulkan_smoke_sky_scene()
{
    static const bool enabled =
        !LLStringUtil::getenv("MARE_VULKAN_SMOKE_SCENE").empty();
    return enabled;
}

static bool get_vulkan_boolean_env(const char* name)
{
    std::string value = LLStringUtil::getenv(name);
    LLStringUtil::toLower(value);
    return value == "1" || value == "true" || value == "yes" || value == "on";
}

static bool use_vulkan_debug_copy_deferred_color_to_swapchain()
{
    static const bool enabled =
        get_vulkan_boolean_env("MARE_VULKAN_DEBUG_COPY_DEFERRED_COLOR_TO_SWAPCHAIN");
    return enabled;
}

static bool use_vulkan_debug_skip_ui_after_world()
{
    static const bool enabled =
        get_vulkan_boolean_env("MARE_VULKAN_DEBUG_SKIP_UI_AFTER_WORLD");
    return enabled;
}

static bool use_vulkan_debug_ui_stage_logs()
{
    static const bool enabled =
        get_vulkan_boolean_env("MARE_VULKAN_DEBUG_UI_STAGE_LOGS");
    return enabled;
}

static bool use_vulkan_debug_render_legacy_hud_attachments()
{
    static const bool enabled =
        get_vulkan_boolean_env("MARE_VULKAN_DEBUG_RENDER_LEGACY_HUD_ATTACHMENTS");
    return enabled;
}

static void log_vulkan_ui_stage_once(
    const char* prefix,
    const char* stage,
    const char* env_name = nullptr)
{
    if (!use_vulkan_world_path())
    {
        return;
    }

    static std::set<std::string> logged;
    std::string key = prefix;
    key += ":";
    key += stage;
    if (env_name)
    {
        key += ":";
        key += env_name;
    }

    if (!logged.insert(key).second)
    {
        return;
    }

    LL_INFOS("RenderBackend")
        << prefix
        << " Vulkan UI stage: "
        << stage;
    if (env_name)
    {
        LL_CONT << " via " << env_name;
    }
    LL_CONT << LL_ENDL;
}

static void trace_vulkan_ui_stage(const char* stage)
{
    if (use_vulkan_debug_ui_stage_logs())
    {
        log_vulkan_ui_stage_once("Reached", stage);
    }
}

static bool should_skip_vulkan_ui_stage(
    const char* env_name,
    const char* stage)
{
    if (!use_vulkan_world_path() ||
        !get_vulkan_boolean_env(env_name))
    {
        return false;
    }

    log_vulkan_ui_stage_once("Skipping", stage, env_name);
    return true;
}

static void render_vulkan_existing_world_geometry()
{
    LL_PROFILE_ZONE_NAMED_CATEGORY_DISPLAY("Vulkan existing world draw pools");

    static bool logged = false;
    if (!logged)
    {
        LL_INFOS("RenderBackend")
            << "Vulkan world path is invoking supported world draw pools through command emission."
            << LL_ENDL;
        logged = true;
    }

    gPipeline.pushRenderTypeMask();
    gPipeline.andRenderTypeMask(
        LLPipeline::RENDER_TYPE_SKY,
        LLPipeline::RENDER_TYPE_WL_SKY,
        LLPipeline::RENDER_TYPE_TERRAIN,
        LLPipeline::RENDER_TYPE_SIMPLE,
        LLPipeline::RENDER_TYPE_ALPHA_MASK,
        LLPipeline::RENDER_TYPE_GRASS,
        LLPipeline::RENDER_TYPE_TREE,
        LLPipeline::RENDER_TYPE_AVATAR,
        LLPipeline::RENDER_TYPE_CONTROL_AV,
        LLPipeline::RENDER_TYPE_BUMP,
        LLPipeline::RENDER_TYPE_MATERIALS,
        LLPipeline::RENDER_TYPE_GLTF_PBR,
        LLPipeline::RENDER_TYPE_GLTF_PBR_ALPHA_MASK,
        LLPipeline::END_RENDER_TYPES);
    gPipeline.renderGeomDeferred(*LLViewerCamera::getInstance(), false);
    gPipeline.popRenderTypeMask();
    gGL.setColorMask(true, true);
}

static void render_vulkan_existing_world_post_geometry()
{
    LL_PROFILE_ZONE_NAMED_CATEGORY_DISPLAY("Vulkan existing post-deferred world draw pools");

    gPipeline.pushRenderTypeMask();
    gPipeline.andRenderTypeMask(
        LLPipeline::RENDER_TYPE_FULLBRIGHT,
        LLPipeline::RENDER_TYPE_FULLBRIGHT_ALPHA_MASK,
        LLPipeline::RENDER_TYPE_BUMP,
        LLPipeline::RENDER_TYPE_ALPHA_PRE_WATER,
        LLPipeline::RENDER_TYPE_ALPHA_POST_WATER,
        LLPipeline::RENDER_TYPE_GLOW,
        LLPipeline::RENDER_TYPE_GLTF_PBR,
        LLPipeline::RENDER_TYPE_WATER,
        LLPipeline::RENDER_TYPE_VOIDWATER,
        LLPipeline::END_RENDER_TYPES);
    gPipeline.renderGeomPostDeferred(*LLViewerCamera::getInstance());
    gPipeline.popRenderTypeMask();
    gGL.setColorMask(true, true);
}

static bool render_vulkan_world_to_deferred_screen(const LLColor4& clear_color)
{
    if (!gPipeline.mRT ||
        !gPipeline.mRT->deferredScreen.isComplete())
    {
        LL_WARNS_ONCE("RenderBackend")
            << "Vulkan deferredScreen is not available; falling back to direct swapchain world rendering."
            << LL_ENDL;
        return false;
    }

    gPipeline.mRT->deferredScreen.bindTarget();
    if (!getRenderBackend().isDrawFramebufferComplete())
    {
        LL_WARNS_ONCE("RenderBackend")
            << "Vulkan deferredScreen backend framebuffer is incomplete; main render-target world pass cannot be recorded this frame."
            << LL_ENDL;
        gPipeline.mRT->deferredScreen.flush();
        return false;
    }

    const LLColor4 smoke_sky_color(0.23f, 0.46f, 0.86f, 1.f);
    const LLColor4& target_clear_color =
        use_vulkan_smoke_sky_scene() ? smoke_sky_color : clear_color;

    getRenderBackend().setClearColor(
        target_clear_color.mV[VRED],
        target_clear_color.mV[VGREEN],
        target_clear_color.mV[VBLUE],
        use_vulkan_smoke_sky_scene() ? 1.f : 0.f);
    gPipeline.mRT->deferredScreen.clear();

    if (use_vulkan_smoke_sky_scene())
    {
        LL_WARNS_ONCE("RenderBackend")
            << "Vulkan smoke scene is active: deferredScreen is filled with a synthetic sky color and normal world geometry is skipped."
            << LL_ENDL;
        gPipeline.mRT->deferredScreen.flush();
        return true;
    }

    LLViewerCamera::sCurCameraID = LLViewerCamera::CAMERA_WORLD;
    LLViewerCamera::getInstance()->setPerspective(
        NOT_FOR_SELECTION,
        0,
        0,
        gPipeline.mRT->deferredScreen.getWidth(),
        gPipeline.mRT->deferredScreen.getHeight(),
        false,
        LLViewerCamera::getInstance()->getNear(),
        MAX_FAR_CLIP * 2.f);

    LLGLSPipeline gls_pipeline;
    LLGLDisable blend(LLRenderCapability::Blend);
    gPipeline.disableLights();
    render_vulkan_existing_world_geometry();

    gPipeline.mRT->deferredScreen.flush();
    return true;
}

static LLRenderWorldMaterialParameters get_vulkan_deferred_composite_parameters(
    U32 attachment_count,
    bool deferred_depth_bound)
{
    LLRenderWorldMaterialParameters parameters;

    LLEnvironment& environment = LLEnvironment::instance();
    LLSettingsSky::ptr_t sky = environment.getCurrentSky();
    static LLCachedControl<bool> should_auto_adjust(gSavedSettings, "RenderSkyAutoAdjustLegacy", false);

    LLColor4 ambient(0.28f, 0.28f, 0.28f, 1.f);
    LLColor3 diffuse_light(0.85f, 0.85f, 0.85f);
    F32 direct_light_scale = 1.f;
    F32 reflection_probe_ambiance = 0.f;
    F32 tonemap_mix = 0.f;
    if (sky)
    {
        ambient = sky->getTotalAmbient();
        const F32 cloud_shadow = llclamp(sky->getCloudShadow(), 0.f, 1.f);
        ambient += (LLColor4::white - ambient) * cloud_shadow * 0.5f;
        direct_light_scale = 1.f - cloud_shadow;
        reflection_probe_ambiance =
            llclamp(sky->getReflectionProbeAmbiance(should_auto_adjust()), 0.f, 1.f);
        tonemap_mix =
            llclamp(sky->getTonemapMix(should_auto_adjust()), 0.f, 1.f);
        gPipeline.setupHWLights();
        const LLColor4& selected_diffuse =
            environment.getIsSunUp() ? gPipeline.mSunDiffuse : gPipeline.mMoonDiffuse;
        diffuse_light = LLColor3(selected_diffuse);
    }

    LLVector4 light_norm = environment.getClampedLightNorm();
    if (sky)
    {
        light_norm = environment.getIsSunUp() ? gPipeline.mSunDir : gPipeline.mMoonDir;
    }
    glm::vec4 transformed_light =
        get_current_modelview() *
        glm::vec4(
            light_norm.mV[VX],
            light_norm.mV[VY],
            light_norm.mV[VZ],
            0.f);
    parameters.mBaseColorRed = llclamp(ambient.mV[VRED], 0.f, 2.f);
    parameters.mBaseColorGreen = llclamp(ambient.mV[VGREEN], 0.f, 2.f);
    parameters.mBaseColorBlue = llclamp(ambient.mV[VBLUE], 0.f, 2.f);
    parameters.mBaseColorAlpha = 1.f;
    parameters.mEmissiveColorRed = llclamp(diffuse_light.mV[VRED], 0.f, 2.f);
    parameters.mEmissiveColorGreen = llclamp(diffuse_light.mV[VGREEN], 0.f, 2.f);
    parameters.mEmissiveColorBlue = llclamp(diffuse_light.mV[VBLUE], 0.f, 2.f);
    parameters.mHasEmissiveMap = 0.f;
    parameters.mSpecularColorRed = transformed_light.x;
    parameters.mSpecularColorGreen = transformed_light.y;
    parameters.mSpecularColorBlue = transformed_light.z;
    parameters.mEnvIntensity = direct_light_scale;
    parameters.mRoughnessFactor = static_cast<F32>(attachment_count);
    const bool ssao_enabled =
        deferred_depth_bound &&
        LLPipeline::RenderDeferredSSAO &&
        !gCubeSnapshot;
    parameters.mMetallicFactor = ssao_enabled ? 1.f : 0.f;
    parameters.mNormalTextureOffsetS =
        ssao_enabled ? llclamp(LLPipeline::RenderSSAOScale, 0.f, 32.f) : 0.f;
    parameters.mNormalTextureOffsetT =
        ssao_enabled ? static_cast<F32>(llmin(LLPipeline::RenderSSAOMaxScale, 32U)) : 0.f;
    parameters.mORMTextureScaleS =
        ssao_enabled ? llclamp(LLPipeline::RenderSSAOFactor, 0.1f, 8.f) : 0.f;
    parameters.mORMTextureScaleT =
        ssao_enabled ? llclamp(LLPipeline::RenderSSAOEffect.mV[VX], 0.f, 2.f) : 0.f;

    LLColor3 local_light_color = LLColor3::black;
    F32 local_light_strength = 0.f;
    U32 visible_light_count = 0;
    LLVector3 dominant_light_screen(-1.f, -1.f, 0.f);
    LLViewerCamera* camera = LLViewerCamera::getInstance();
    if (camera)
    {
        gPipeline.getVulkanDeferredLightSummary(
            *camera,
            local_light_color,
            local_light_strength,
            visible_light_count,
            dominant_light_screen);
    }
    parameters.mHasEmissiveMap = dominant_light_screen.mV[VZ];
    parameters.mMaterialFlags = dominant_light_screen.mV[VX];
    parameters.mBaseColorAlpha = dominant_light_screen.mV[VY];
    parameters.mDiffuseAlphaMode = local_light_color.mV[VRED];
    parameters.mGLTFAlphaMode = local_light_color.mV[VGREEN];
    parameters.mBump = local_light_color.mV[VBLUE];
    parameters.mShiny = local_light_strength;
    parameters.mSceneAmbientRed = reflection_probe_ambiance;
    parameters.mSceneAmbientGreen = tonemap_mix;
    parameters.mSceneAmbientBlue = direct_light_scale;
    parameters.mSceneDirectScale = sky ? 1.f : 0.f;

    return parameters;
}

static LLRenderWorldMaterialParameters get_vulkan_final_composite_parameters(
    U32 deferred_attachment_count,
    bool deferred_depth_bound)
{
    LLRenderWorldMaterialParameters parameters;

    static LLCachedControl<bool> build_no_post(gSavedSettings, "RenderDisablePostProcessing", false);
    static LLCachedControl<bool> should_auto_adjust(gSavedSettings, "RenderSkyAutoAdjustLegacy", false);
    static LLCachedControl<F32> exposure(gSavedSettings, "RenderExposure", 1.f);
    static LLCachedControl<F32> display_gamma(gSavedSettings, "RenderDeferredDisplayGamma", 2.2f);
    static LLCachedControl<U32> tonemap_type(gSavedSettings, "RenderTonemapType", 0U);
    static LLCachedControl<F32> cas_sharpness(gSavedSettings, "RenderCASSharpness", 0.4f);

    LLSettingsSky::ptr_t sky = LLEnvironment::instance().getCurrentSky();
    bool no_post = true;
    F32 tonemap_mix = 0.f;
    if (sky)
    {
        no_post = gSnapshotNoPost ||
                  sky->getReflectionProbeAmbiance(should_auto_adjust()) == 0.f ||
                  (build_no_post && gFloaterTools && gFloaterTools->isAvailable());
        if (!no_post)
        {
            tonemap_mix = llclamp(sky->getTonemapMix(should_auto_adjust()), 0.f, 1.f);
        }
    }

    const F32 gamma = llclamp(display_gamma(), 0.1f, 8.f);
    parameters.mBaseColorRed = no_post ? 1.f : llclamp(exposure(), 0.5f, 4.f);
    parameters.mBaseColorGreen = no_post ? 1.f : 1.f / gamma;
    parameters.mBaseColorBlue = no_post ? 0.f : 1.f;
    parameters.mBaseColorAlpha = no_post ? 0.f : llclamp(cas_sharpness(), 0.f, 1.f);
    parameters.mRoughnessFactor = tonemap_mix;
    parameters.mMetallicFactor = no_post ? 0.f : static_cast<F32>(tonemap_type());
    parameters.mMaterialFlags =
        no_post || !LLPipeline::sRenderGlow ? 0.f : llclamp(LLPipeline::RenderGlowWarmthAmount, 0.f, 1.f);
    parameters.mSpecularColorRed = no_post ? 0.f : static_cast<F32>(LLPipeline::RenderFSAAType);
    parameters.mSpecularColorGreen =
        LLPipeline::RenderBufferVisualization >= 0 && LLPipeline::RenderBufferVisualization <= 6 ?
            static_cast<F32>(LLPipeline::RenderBufferVisualization) :
            -1.f;
    parameters.mSpecularColorBlue = static_cast<F32>(deferred_attachment_count);
    const bool dof_enabled =
        !no_post &&
        deferred_depth_bound &&
        (LLPipeline::RenderDepthOfFieldInEditMode || !LLToolMgr::getInstance()->inBuildMode()) &&
        LLPipeline::RenderDepthOfField &&
        !gCubeSnapshot;
    parameters.mEnvIntensity = dof_enabled ? llclamp(LLPipeline::CameraMaxCoF, 0.f, 10.f) : 0.f;
    parameters.mDiffuseAlphaMode =
        no_post || !LLPipeline::sRenderGlow ? 0.f : llclamp(LLPipeline::RenderGlowStrength, 0.f, 2.f);
    parameters.mGLTFAlphaMode =
        no_post || !LLPipeline::sRenderGlow ? 0.f : llclamp(LLPipeline::RenderGlowMaxExtractAlpha, 0.f, 1.f);
    parameters.mBump =
        no_post || !LLPipeline::sRenderGlow ? 0.f : llclamp(LLPipeline::RenderGlowWidth, 0.f, 8.f);
    parameters.mShiny =
        no_post || !LLPipeline::sRenderGlow ? 0.f : static_cast<F32>(llclamp(LLPipeline::RenderGlowIterations, 0, 8));

    return parameters;
}

static S32 get_vulkan_debug_deferred_attachment()
{
    static bool initialized = false;
    static S32 debug_attachment = -1;
    if (initialized)
    {
        return debug_attachment;
    }
    initialized = true;

    const std::string value =
        LLStringUtil::getenv("MARE_VULKAN_DEBUG_DEFERRED_ATTACHMENT");
    if (value.empty())
    {
        return debug_attachment;
    }

    S32 parsed_attachment = -1;
    if (!LLStringUtil::convertToS32(value, parsed_attachment))
    {
        LL_WARNS("RenderBackend")
            << "Ignoring invalid MARE_VULKAN_DEBUG_DEFERRED_ATTACHMENT value '"
            << value
            << "'. Use 0, 1, 2, 3, or 4 for depth."
            << LL_ENDL;
        return debug_attachment;
    }

    debug_attachment = llclamp(parsed_attachment, -1, 4);
    if (debug_attachment >= 0)
    {
        LL_WARNS("RenderBackend")
            << "Vulkan deferred composite debug is showing attachment "
            << debug_attachment
            << " directly; normal deferred lighting/composite is bypassed for diagnosis."
            << LL_ENDL;
    }
    return debug_attachment;
}

class LLVulkanCompositeMatrixScope
{
public:
    LLVulkanCompositeMatrixScope()
        : mMatrixMode(gGL.getMatrixMode()),
          mProjection(gGL.getProjectionMatrix()),
          mModelview(gGL.getModelviewMatrix())
    {
    }

    ~LLVulkanCompositeMatrixScope()
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

template <typename T>
static void append_vulkan_fullscreen_quad_bytes(std::vector<U8>& bytes, const T& value)
{
    const U8* begin = reinterpret_cast<const U8*>(&value);
    bytes.insert(bytes.end(), begin, begin + sizeof(T));
}

struct LLVulkanFullscreenQuad
{
    LLRenderBufferHandle mVertexBuffer;
    U64 mPositionOffset = 0;
    U64 mTexCoordOffset = 0;
    U64 mColorOffset = 0;
};

static LLVulkanFullscreenQuad sVulkanFullscreenQuad;

static bool ensure_vulkan_fullscreen_quad()
{
    if (sVulkanFullscreenQuad.mVertexBuffer)
    {
        return true;
    }

    // World Vulkan pipelines use a fixed 16-byte position binding stride.
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
        {{ 0.f, 1.f }},
        {{ 1.f, 1.f }},
        {{ 0.f, 0.f }},
        {{ 0.f, 0.f }},
        {{ 1.f, 1.f }},
        {{ 1.f, 0.f }},
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
        texcoords.size() * sizeof(texcoords[0]) +
        colors.size() * sizeof(colors[0]));

    sVulkanFullscreenQuad.mPositionOffset = bytes.size();
    for (const auto& position : positions)
    {
        append_vulkan_fullscreen_quad_bytes(bytes, position);
    }

    sVulkanFullscreenQuad.mTexCoordOffset = bytes.size();
    for (const auto& texcoord : texcoords)
    {
        append_vulkan_fullscreen_quad_bytes(bytes, texcoord);
    }

    sVulkanFullscreenQuad.mColorOffset = bytes.size();
    for (const auto& color : colors)
    {
        append_vulkan_fullscreen_quad_bytes(bytes, color);
    }

    LLRenderBackend& backend = getRenderBackend();
    sVulkanFullscreenQuad.mVertexBuffer = backend.createBufferHandle();
    if (!sVulkanFullscreenQuad.mVertexBuffer)
    {
        return false;
    }

    backend.bindBuffer(
        LLRenderBufferTarget::Vertex,
        sVulkanFullscreenQuad.mVertexBuffer);
    backend.allocateBufferStorage(
        LLRenderBufferTarget::Vertex,
        bytes.size(),
        bytes.data(),
        LLRenderBufferUsage::StaticDraw);
    return true;
}

static bool draw_vulkan_fullscreen_quad()
{
    if (!ensure_vulkan_fullscreen_quad())
    {
        LL_WARNS_ONCE("RenderBackend")
            << "Unable to create Vulkan fullscreen composite quad."
            << LL_ENDL;
        return false;
    }

    LLRenderBackend& backend = getRenderBackend();
    for (U32 type = 0; type < LLVertexBuffer::TYPE_MAX; ++type)
    {
        backend.disableVertexAttributeArray(type);
    }

    backend.bindBuffer(
        LLRenderBufferTarget::Vertex,
        sVulkanFullscreenQuad.mVertexBuffer);
    backend.enableVertexAttributeArray(LLVertexBuffer::TYPE_VERTEX);
    backend.setVertexAttributePointer(
        LLVertexBuffer::TYPE_VERTEX,
        3,
        LLRenderVertexAttributeType::Float32,
        false,
        16,
        reinterpret_cast<const void*>(
            static_cast<uintptr_t>(sVulkanFullscreenQuad.mPositionOffset)));
    backend.enableVertexAttributeArray(LLVertexBuffer::TYPE_TEXCOORD0);
    backend.setVertexAttributePointer(
        LLVertexBuffer::TYPE_TEXCOORD0,
        2,
        LLRenderVertexAttributeType::Float32,
        false,
        8,
        reinterpret_cast<const void*>(
            static_cast<uintptr_t>(sVulkanFullscreenQuad.mTexCoordOffset)));
    backend.enableVertexAttributeArray(LLVertexBuffer::TYPE_COLOR);
    backend.setVertexAttributePointer(
        LLVertexBuffer::TYPE_COLOR,
        4,
        LLRenderVertexAttributeType::UnsignedByte,
        true,
        4,
        reinterpret_cast<const void*>(
            static_cast<uintptr_t>(sVulkanFullscreenQuad.mColorOffset)));

    gGL.matrixMode(LLRender::MM_PROJECTION);
    gGL.loadIdentity();
    gGL.matrixMode(LLRender::MM_MODELVIEW);
    gGL.loadIdentity();
    backend.drawArrays(LLRenderPrimitiveType::Triangles, 0, 6);
    return true;
}

static LLRect make_vulkan_target_rect(S32 width, S32 height)
{
    return LLRect(0, height, width, 0);
}

static LLRect get_vulkan_world_view_rect()
{
    return gViewerWindow ?
        gViewerWindow->getWorldViewRectRaw() :
        LLRect(0, 0, 0, 0);
}

static void set_vulkan_composite_viewport(
    const LLRect& rect,
    bool swapchain_viewport = false)
{
    S32 viewport_y = rect.mBottom;
    if (swapchain_viewport && gViewerWindow)
    {
        viewport_y = gViewerWindow->getWindowRectRaw().getHeight() - rect.mTop;
    }

    getRenderBackend().setViewport(
        rect.mLeft,
        viewport_y,
        rect.getWidth(),
        rect.getHeight());
    getRenderBackend().setScissor(
        rect.mLeft,
        rect.mBottom,
        rect.getWidth(),
        rect.getHeight());
}

static void render_vulkan_deferred_screen_composite_quad(
    const LLRect& viewport_rect,
    bool swapchain_viewport = false)
{
    LL_PROFILE_ZONE_NAMED_CATEGORY_DISPLAY("Vulkan deferredScreen composite");
    LLVulkanCompositeMatrixScope matrix_scope;

    const U32 deferred_attachment_count =
        llmin(gPipeline.mRT->deferredScreen.getNumTextures(), 4U);
    const bool use_deferred_composite = deferred_attachment_count >= 3U;
    const bool has_deferred_depth = gPipeline.mRT->deferredScreen.getDepth() != 0;
    LLRenderWorldMaterialParameters deferred_composite_parameters;
    if (use_deferred_composite)
    {
        deferred_composite_parameters =
            get_vulkan_deferred_composite_parameters(
                deferred_attachment_count,
                has_deferred_depth);
    }

    LLGLSUIDefault gls_ui;
    LLGLDepthTest depth(false);
    LLGLDisable blend(LLRenderCapability::Blend);
    LLGLDisable cull(LLRenderCapability::CullFace);

    gViewerWindow->setup2DRender();
    gGL.pushMatrix();
    {
        const LLVector2& display_scale = gViewerWindow->getDisplayScale();
        gGL.scalef(display_scale.mV[VX], display_scale.mV[VY], 1.f);

        const S32 debug_attachment = get_vulkan_debug_deferred_attachment();
        bool debug_attachment_rendered = false;
        if (debug_attachment >= 0)
        {
            if (use_deferred_composite)
            {
                gPipeline.mRT->deferredScreen.bindTexture(0, 0, LLTexUnit::TFO_BILINEAR);
                for (U32 attachment = 0; attachment < deferred_attachment_count; ++attachment)
                {
                    gPipeline.mRT->deferredScreen.bindTexture(
                        attachment,
                        static_cast<S32>(attachment + 1),
                        LLTexUnit::TFO_BILINEAR);
                }

                bool deferred_debug_depth_bound = false;
                if (gPipeline.mRT->deferredScreen.getDepth() != 0)
                {
                    deferred_debug_depth_bound =
                        gGL.getTexUnit(5)->bind(&gPipeline.mRT->deferredScreen, true);
                }

                LLRenderWorldMaterialParameters debug_composite_parameters =
                    get_vulkan_final_composite_parameters(
                        deferred_attachment_count,
                        deferred_debug_depth_bound);
                debug_composite_parameters.mSpecularColorGreen =
                    static_cast<F32>(debug_attachment);
                debug_composite_parameters.mSpecularColorBlue =
                    static_cast<F32>(deferred_attachment_count);

                getRenderBackend().setWorldDrawEnabled(true);
                getRenderBackend().setWorldShaderClass(LLRenderWorldShaderClass::FinalComposite);
                getRenderBackend().setWorldMaterialParameters(debug_composite_parameters);
                getRenderBackend().setWorldTextureTransform({});
                getRenderBackend().setWorldTerrainParameters({});
                getRenderBackend().setWorldSkinningMatrixPalette(0, nullptr);
                set_vulkan_composite_viewport(
                    viewport_rect,
                    swapchain_viewport);
                getRenderBackend().setCapability(LLRenderCapability::DepthTest, false);
                getRenderBackend().setDepthWriteEnabled(false);
                getRenderBackend().setCapability(LLRenderCapability::Blend, false);
                getRenderBackend().setCapability(LLRenderCapability::CullFace, false);
                getRenderBackend().setColorMask({ true, true, true, true });
                draw_vulkan_fullscreen_quad();
                getRenderBackend().setWorldDrawEnabled(false);
                getRenderBackend().setWorldShaderClass(LLRenderWorldShaderClass::Textured);
                getRenderBackend().setWorldMaterialParameters({});
                gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);
                for (U32 attachment = 0; attachment < deferred_attachment_count; ++attachment)
                {
                    gGL.getTexUnit(static_cast<S32>(attachment + 1))->unbind(LLTexUnit::TT_TEXTURE);
                }
                if (deferred_debug_depth_bound)
                {
                    gGL.getTexUnit(5)->unbind(LLTexUnit::TT_TEXTURE);
                }
                debug_attachment_rendered = true;
            }
            else
            {
                LL_WARNS_ONCE("RenderBackend")
                    << "Vulkan deferred composite debug attachment "
                    << debug_attachment
                    << " is unavailable."
                    << LL_ENDL;
            }
        }

        bool deferred_depth_bound = false;
        if (!debug_attachment_rendered)
        {
            for (U32 attachment = 0; attachment < deferred_attachment_count; ++attachment)
            {
                gPipeline.mRT->deferredScreen.bindTexture(
                    attachment,
                    static_cast<S32>(attachment),
                    LLTexUnit::TFO_BILINEAR);
            }
            if (gPipeline.mRT->deferredScreen.getDepth() != 0)
            {
                deferred_depth_bound =
                    gGL.getTexUnit(4)->bind(&gPipeline.mRT->deferredScreen, true);
            }

            if (use_deferred_composite)
            {
                getRenderBackend().setWorldDrawEnabled(true);
                getRenderBackend().setWorldShaderClass(LLRenderWorldShaderClass::DeferredComposite);
                getRenderBackend().setWorldTextureTransform({});
                getRenderBackend().setWorldTerrainParameters({});
                getRenderBackend().setWorldSkinningMatrixPalette(0, nullptr);
                getRenderBackend().setWorldMaterialParameters(
                    deferred_composite_parameters);
            }
            else
            {
                LL_WARNS_ONCE("RenderBackend")
                    << "Vulkan deferredScreen has fewer than three G-buffer attachments; using color-only swapchain composite."
                    << LL_ENDL;
            }

            set_vulkan_composite_viewport(
                viewport_rect,
                swapchain_viewport);
            getRenderBackend().setCapability(LLRenderCapability::DepthTest, false);
            getRenderBackend().setDepthWriteEnabled(false);
            getRenderBackend().setCapability(LLRenderCapability::Blend, false);
            getRenderBackend().setCapability(LLRenderCapability::CullFace, false);
            getRenderBackend().setColorMask({ true, true, true, true });
            draw_vulkan_fullscreen_quad();
            if (use_deferred_composite)
            {
                getRenderBackend().setWorldDrawEnabled(false);
                getRenderBackend().setWorldShaderClass(LLRenderWorldShaderClass::Textured);
                getRenderBackend().setWorldMaterialParameters({});
            }
            for (U32 attachment = 0; attachment < deferred_attachment_count; ++attachment)
            {
                gGL.getTexUnit(static_cast<S32>(attachment))->unbind(LLTexUnit::TT_TEXTURE);
            }
            if (deferred_depth_bound)
            {
                gGL.getTexUnit(4)->unbind(LLTexUnit::TT_TEXTURE);
            }
        }
    }
    gGL.popMatrix();
}

static void render_vulkan_final_composite_quad(
    LLRenderTarget& source,
    const LLRect& viewport_rect,
    bool swapchain_viewport = false)
{
    LL_PROFILE_ZONE_NAMED_CATEGORY_DISPLAY("Vulkan final composite quad");
    LLVulkanCompositeMatrixScope matrix_scope;

    LLGLSUIDefault gls_ui;
    LLGLDepthTest depth(false);
    LLGLDisable blend(LLRenderCapability::Blend);
    LLGLDisable cull(LLRenderCapability::CullFace);

    gViewerWindow->setup2DRender();
    gGL.pushMatrix();
    {
        const LLVector2& display_scale = gViewerWindow->getDisplayScale();
        gGL.scalef(display_scale.mV[VX], display_scale.mV[VY], 1.f);

        source.bindTexture(0, 0, LLTexUnit::TFO_BILINEAR);
        U32 deferred_attachment_count = 0;
        bool deferred_depth_bound = false;
        if (gPipeline.mRT &&
            gPipeline.mRT->deferredScreen.isComplete())
        {
            deferred_attachment_count =
                llmin(gPipeline.mRT->deferredScreen.getNumTextures(), 4U);
            for (U32 attachment = 0; attachment < deferred_attachment_count; ++attachment)
            {
                gPipeline.mRT->deferredScreen.bindTexture(
                    attachment,
                    static_cast<S32>(attachment + 1),
                    LLTexUnit::TFO_BILINEAR);
            }
            if (gPipeline.mRT->deferredScreen.getDepth() != 0)
            {
                deferred_depth_bound =
                    gGL.getTexUnit(5)->bind(&gPipeline.mRT->deferredScreen, true);
            }
        }

        getRenderBackend().setWorldDrawEnabled(true);
        getRenderBackend().setWorldShaderClass(LLRenderWorldShaderClass::FinalComposite);
        getRenderBackend().setWorldTextureTransform({});
        getRenderBackend().setWorldTerrainParameters({});
        getRenderBackend().setWorldSkinningMatrixPalette(0, nullptr);
        getRenderBackend().setWorldMaterialParameters(
            get_vulkan_final_composite_parameters(deferred_attachment_count, deferred_depth_bound));
        set_vulkan_composite_viewport(
            viewport_rect,
            swapchain_viewport);
        getRenderBackend().setCapability(LLRenderCapability::DepthTest, false);
        getRenderBackend().setDepthWriteEnabled(false);
        getRenderBackend().setCapability(LLRenderCapability::Blend, false);
        getRenderBackend().setCapability(LLRenderCapability::CullFace, false);
        getRenderBackend().setColorMask({ true, true, true, true });
        draw_vulkan_fullscreen_quad();
        getRenderBackend().setWorldDrawEnabled(false);
        getRenderBackend().setWorldShaderClass(LLRenderWorldShaderClass::Textured);
        getRenderBackend().setWorldMaterialParameters({});
        gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);
        for (U32 attachment = 0; attachment < deferred_attachment_count; ++attachment)
        {
            gGL.getTexUnit(static_cast<S32>(attachment + 1))->unbind(LLTexUnit::TT_TEXTURE);
        }
        if (deferred_depth_bound)
        {
            gGL.getTexUnit(5)->unbind(LLTexUnit::TT_TEXTURE);
        }
    }
    gGL.popMatrix();
}

static void render_vulkan_screen_target_to_swapchain(
    LLRenderTarget& target,
    const LLColor4& clear_color)
{
    LL_PROFILE_ZONE_NAMED_CATEGORY_DISPLAY("Vulkan final composite to swapchain");

    getRenderBackend().setClearColor(
        clear_color.mV[VRED],
        clear_color.mV[VGREEN],
        clear_color.mV[VBLUE],
        1.f);
    getRenderBackend().clear(LL_RENDER_CLEAR_COLOR | LL_RENDER_CLEAR_DEPTH);

    render_vulkan_final_composite_quad(
        target,
        get_vulkan_world_view_rect(),
        true);
}

static void render_vulkan_copy_target_to_target(
    LLRenderTarget& source,
    LLRenderTarget& destination,
    const LLColor4& clear_color)
{
    LL_PROFILE_ZONE_NAMED_CATEGORY_DISPLAY("Vulkan render target copy");
    LLVulkanCompositeMatrixScope matrix_scope;

    destination.bindTarget();
    getRenderBackend().setClearColor(
        clear_color.mV[VRED],
        clear_color.mV[VGREEN],
        clear_color.mV[VBLUE],
        1.f);
    destination.clear(LL_RENDER_CLEAR_COLOR);

    LLGLSUIDefault gls_ui;
    LLGLDepthTest depth(false);
    LLGLDisable blend(LLRenderCapability::Blend);
    LLGLDisable cull(LLRenderCapability::CullFace);

    gViewerWindow->setup2DRender();
    gGL.pushMatrix();
    {
        const LLVector2& display_scale = gViewerWindow->getDisplayScale();
        gGL.scalef(display_scale.mV[VX], display_scale.mV[VY], 1.f);

        source.bindTexture(0, 0, LLTexUnit::TFO_BILINEAR);
        getRenderBackend().setWorldDrawEnabled(true);
        getRenderBackend().setWorldShaderClass(LLRenderWorldShaderClass::Textured);
        getRenderBackend().setWorldTextureTransform({});
        getRenderBackend().setWorldTerrainParameters({});
        getRenderBackend().setWorldSkinningMatrixPalette(0, nullptr);
        getRenderBackend().setWorldMaterialParameters({});
        getRenderBackend().setViewport(
            0,
            0,
            destination.getWidth(),
            destination.getHeight());
        getRenderBackend().setScissor(
            0,
            0,
            destination.getWidth(),
            destination.getHeight());
        getRenderBackend().setCapability(LLRenderCapability::DepthTest, false);
        getRenderBackend().setDepthWriteEnabled(false);
        getRenderBackend().setCapability(LLRenderCapability::Blend, false);
        getRenderBackend().setCapability(LLRenderCapability::CullFace, false);
        getRenderBackend().setColorMask({ true, true, true, true });
        draw_vulkan_fullscreen_quad();
        getRenderBackend().setWorldDrawEnabled(false);
        gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);
    }
    gGL.popMatrix();

    destination.flush();
}

static void render_vulkan_copy_target_to_swapchain(
    LLRenderTarget& source,
    const LLColor4& clear_color)
{
    LL_PROFILE_ZONE_NAMED_CATEGORY_DISPLAY("Vulkan render target copy to swapchain");
    LLVulkanCompositeMatrixScope matrix_scope;

    getRenderBackend().setClearColor(
        clear_color.mV[VRED],
        clear_color.mV[VGREEN],
        clear_color.mV[VBLUE],
        1.f);
    getRenderBackend().clear(LL_RENDER_CLEAR_COLOR | LL_RENDER_CLEAR_DEPTH);

    LLGLSUIDefault gls_ui;
    LLGLDepthTest depth(false);
    LLGLDisable blend(LLRenderCapability::Blend);
    LLGLDisable cull(LLRenderCapability::CullFace);

    gViewerWindow->setup2DRender();
    gGL.pushMatrix();
    {
        const LLVector2& display_scale = gViewerWindow->getDisplayScale();
        gGL.scalef(display_scale.mV[VX], display_scale.mV[VY], 1.f);

        source.bindTexture(0, 0, LLTexUnit::TFO_BILINEAR);
        getRenderBackend().setWorldDrawEnabled(true);
        getRenderBackend().setWorldShaderClass(LLRenderWorldShaderClass::Textured);
        getRenderBackend().setWorldTextureTransform({});
        getRenderBackend().setWorldTerrainParameters({});
        getRenderBackend().setWorldSkinningMatrixPalette(0, nullptr);
        getRenderBackend().setWorldMaterialParameters({});
        set_vulkan_composite_viewport(
            get_vulkan_world_view_rect(),
            true);
        getRenderBackend().setCapability(LLRenderCapability::DepthTest, false);
        getRenderBackend().setDepthWriteEnabled(false);
        getRenderBackend().setCapability(LLRenderCapability::Blend, false);
        getRenderBackend().setCapability(LLRenderCapability::CullFace, false);
        getRenderBackend().setColorMask({ true, true, true, true });
        draw_vulkan_fullscreen_quad();
        getRenderBackend().setWorldDrawEnabled(false);
        gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);
    }
    gGL.popMatrix();
}

static void render_vulkan_final_composite_target_to_target(
    LLRenderTarget& source,
    LLRenderTarget& destination,
    const LLColor4& clear_color)
{
    LL_PROFILE_ZONE_NAMED_CATEGORY_DISPLAY("Vulkan final composite to render target");

    destination.bindTarget();
    getRenderBackend().setClearColor(
        clear_color.mV[VRED],
        clear_color.mV[VGREEN],
        clear_color.mV[VBLUE],
        1.f);
    destination.clear(LL_RENDER_CLEAR_COLOR);

    render_vulkan_final_composite_quad(
        source,
        make_vulkan_target_rect(destination.getWidth(), destination.getHeight()));

    destination.flush();
}

static LLRenderTarget* render_vulkan_deferred_screen_to_light_target(
    const LLColor4& clear_color)
{
    if (!gPipeline.mRT ||
        !gPipeline.mRT->deferredLight.isComplete())
    {
        LL_WARNS_ONCE("RenderBackend")
            << "Vulkan deferredLight target is not available; compositing deferredScreen directly to the screen target."
            << LL_ENDL;
        return nullptr;
    }

    LL_PROFILE_ZONE_NAMED_CATEGORY_DISPLAY("Vulkan deferred light composite");

    LLRenderTarget& light_target = gPipeline.mRT->deferredLight;
    light_target.bindTarget();
    getRenderBackend().setClearColor(
        clear_color.mV[VRED],
        clear_color.mV[VGREEN],
        clear_color.mV[VBLUE],
        1.f);
    light_target.clear(LL_RENDER_CLEAR_COLOR);

    render_vulkan_deferred_screen_composite_quad(
        make_vulkan_target_rect(light_target.getWidth(), light_target.getHeight()));

    light_target.flush();
    return &light_target;
}

static bool render_vulkan_lit_world_to_screen_target(
    LLRenderTarget* lit_world_target,
    const LLColor4& clear_color)
{
    if (!gPipeline.mRT ||
        !gPipeline.mRT->screen.isComplete())
    {
        LL_WARNS_ONCE("RenderBackend")
            << "Vulkan screen render target is not available; compositing directly to the swapchain."
            << LL_ENDL;
        return false;
    }

    LLRenderTarget& screen_target = gPipeline.mRT->screen;
    if (lit_world_target)
    {
        render_vulkan_copy_target_to_target(*lit_world_target, screen_target, clear_color);
    }
    else
    {
        screen_target.bindTarget();
        getRenderBackend().setClearColor(
            clear_color.mV[VRED],
            clear_color.mV[VGREEN],
            clear_color.mV[VBLUE],
            1.f);
        // Preserve the shared deferredScreen depth attachment for post-deferred
        // overlays; the Vulkan offscreen render pass loads depth for color-only
        // clears.
        screen_target.clear(LL_RENDER_CLEAR_COLOR);

        render_vulkan_deferred_screen_composite_quad(
            make_vulkan_target_rect(screen_target.getWidth(), screen_target.getHeight()));
        screen_target.flush();
    }

    screen_target.bindTarget();
    gViewerWindow->setup3DRender();
    gPipeline.disableLights();
    render_vulkan_existing_world_post_geometry();

    screen_target.flush();
    return true;
}

static LLRenderTarget& render_vulkan_screen_target_to_post_target(
    LLRenderTarget& screen_target,
    const LLColor4& clear_color)
{
    if (!gPipeline.mRT ||
        !gPipeline.mRT->deferredLight.isComplete())
    {
        LL_WARNS_ONCE("RenderBackend")
            << "Vulkan deferredLight target is not available; copying screen target directly to the swapchain."
            << LL_ENDL;
        return screen_target;
    }

    LL_PROFILE_ZONE_NAMED_CATEGORY_DISPLAY("Vulkan screen target post-compose");

    LLRenderTarget& post_target = gPipeline.mRT->deferredLight;
    render_vulkan_copy_target_to_target(screen_target, post_target, clear_color);
    return post_target;
}

static LLRenderTarget& render_vulkan_post_target_to_final_target(
    LLRenderTarget& post_target,
    const LLColor4& clear_color,
    bool& final_composite_applied)
{
    final_composite_applied = false;
    if (!gPipeline.mPostPingMap.isComplete())
    {
        LL_WARNS_ONCE("RenderBackend")
            << "Vulkan final post-process target is not available; applying final composite directly to the swapchain."
            << LL_ENDL;
        return post_target;
    }

    render_vulkan_final_composite_target_to_target(
        post_target,
        gPipeline.mPostPingMap,
        clear_color);
    final_composite_applied = true;
    return gPipeline.mPostPingMap;
}

static void render_vulkan_deferred_screen_to_swapchain(const LLColor4& clear_color)
{
    getRenderBackend().setClearColor(
        clear_color.mV[VRED],
        clear_color.mV[VGREEN],
        clear_color.mV[VBLUE],
        1.f);
    getRenderBackend().clear(LL_RENDER_CLEAR_COLOR | LL_RENDER_CLEAR_DEPTH);

    render_vulkan_deferred_screen_composite_quad(
        get_vulkan_world_view_rect(),
        true);
}

static bool use_vulkan_staged_post_targets()
{
    return false;
}

static bool use_vulkan_post_deferred_overlays_after_composite()
{
    return false;
}

static void render_vulkan_depth_prepass()
{
    LL_PROFILE_ZONE_NAMED_CATEGORY_DISPLAY("Vulkan depth prepass");

    LL_WARNS_ONCE("RenderBackend")
        << "Vulkan is replaying deferred geometry as a depth prepass until deferred depth copy support exists."
        << LL_ENDL;

    LLGLSUIDefault gls_ui;
    LLGLDepthTest depth(true, true, LLRenderDepthFunction::LessEqual);
    LLGLDisable blend(LLRenderCapability::Blend);

    gGL.setColorMask(false, false);
    gPipeline.disableLights();
    gViewerWindow->setup3DRender();
    render_vulkan_existing_world_geometry();
    gGL.setColorMask(true, true);
}

void display_startup()
{
    if (   !gViewerWindow
        || !gViewerWindow->getActive()
        || !gViewerWindow->getWindow()->getVisible()
        || gViewerWindow->getWindow()->getMinimized()
        || gNonInteractive)
    {
        return;
    }

    gPipeline.updateGL();

    // Written as branch to appease GCC which doesn't like different
    // pointer types across ternary ops
    //
    if (!LLViewerFetchedTexture::sWhiteImagep.isNull())
    {
    LLTexUnit::sWhiteTexture = LLViewerFetchedTexture::sWhiteImagep->getTexName();
    }

    LLGLSDefault gls_default;

    // Required for HTML update in login screen
    static S32 frame_count = 0;

    LLGLState::checkStates();

    if (frame_count++ > 1) // make sure we have rendered a frame first
    {
        LLViewerDynamicTexture::updateAllInstances();
    }
    else
    {
        LL_DEBUGS("Window") << "First display_startup frame" << LL_ENDL;
    }

    LLGLState::checkStates();

    getRenderBackend().clear(LL_RENDER_CLEAR_DEPTH | LL_RENDER_CLEAR_COLOR); // | LL_RENDER_CLEAR_STENCIL);
    LLGLSUIDefault gls_ui;
    gPipeline.disableLights();

    if (gViewerWindow)
    gViewerWindow->setup2DRender();
    if (gViewerWindow)
    gViewerWindow->draw();
    gGL.flush();

    LLVertexBuffer::unbind();

    LLGLState::checkStates();

    if (gViewerWindow && gViewerWindow->getWindow())
    gViewerWindow->getWindow()->swapBuffers();

    getRenderBackend().clear(LL_RENDER_CLEAR_DEPTH);
}

void display_update_camera()
{
    LL_PROFILE_ZONE_NAMED_CATEGORY_DISPLAY("Update Camera");
    // TODO: cut draw distance down if customizing avatar?
    // TODO: cut draw distance on per-parcel basis?

    // Cut draw distance in half when customizing avatar,
    // but on the viewer only.
    F32 final_far = gAgentCamera.mDrawDistance;
    if (gCubeSnapshot)
    {
        static LLCachedControl<F32> reflection_probe_draw_distance(gSavedSettings, "RenderReflectionProbeDrawDistance", 64.f);
        final_far = reflection_probe_draw_distance();
    }
    else if (CAMERA_MODE_CUSTOMIZE_AVATAR == gAgentCamera.getCameraMode())
    {
        final_far *= 0.5f;
    }
    else if (LLViewerTexture::sDesiredDiscardBias > 2.f)
    {
        final_far = llmax(32.f, final_far / (LLViewerTexture::sDesiredDiscardBias - 1.f));
    }
    LLViewerCamera::getInstance()->setFar(final_far);
    LLVOAvatar::sRenderDistance = llclamp(final_far, 16.f, 256.f);
    gViewerWindow->setup3DRender();

    if (!gCubeSnapshot)
    {
        // Update land visibility too
        LLWorld::getInstance()->setLandFarClip(final_far);
    }
}

// Write some stats to LL_INFOS()
void display_stats()
{
    if (gSavedSettings.getBOOL("KokuaSuppressPeriodicLogging")) return;

    LL_PROFILE_ZONE_SCOPED;
    constexpr F32 FPS_LOG_FREQUENCY = 10.f;
    if (gRecentFPSTime.getElapsedTimeF32() >= FPS_LOG_FREQUENCY)
    {
        LL_PROFILE_ZONE_NAMED_CATEGORY_DISPLAY("DS - FPS");
        LLTrace::Recording& recording = LLTrace::get_frame_recording().getLastRecording();
        F64 normalized_session_jitter = recording.getLastValue(LLStatViewer::NOTRMALIZED_FRAMETIME_JITTER_SESSION);
        F64 normalized_period_jitter = recording.getLastValue(LLStatViewer::NORMALIZED_FRAMTIME_JITTER_PERIOD);
        F32 fps = gRecentFrameCount / FPS_LOG_FREQUENCY;
        LL_INFOS() << llformat("FPS: %.02f SESSION JITTER: %.4f PERIOD JITTER: %.4f", fps, normalized_session_jitter, normalized_period_jitter) << LL_ENDL;
        gRecentFrameCount = 0;
        gRecentFPSTime.reset();
    }
    static LLCachedControl<F32> mem_log_freq(gSavedSettings, "MemoryLogFrequency", 600.f);
    if (mem_log_freq > 0.f && gRecentMemoryTime.getElapsedTimeF32() >= mem_log_freq)
    {
        LL_PROFILE_ZONE_NAMED_CATEGORY_DISPLAY("DS - Memory");
        gMemoryAllocated = U64Bytes(LLMemory::getCurrentRSS());
        U32Megabytes memory = gMemoryAllocated;
        LL_INFOS() << "MEMORY: " << memory << LL_ENDL;
        LLMemory::logMemoryInfo(true) ;
        gRecentMemoryTime.reset();
    }
    constexpr F32 ASSET_STORAGE_LOG_FREQUENCY = 60.f;
    if (gAssetStorageLogTime.getElapsedTimeF32() >= ASSET_STORAGE_LOG_FREQUENCY)
    {
        LL_PROFILE_ZONE_NAMED_CATEGORY_DISPLAY("DS - Asset Storage");
        gAssetStorageLogTime.reset();
        gAssetStorage->logAssetStorageInfo();
    }
    F32 rlv_log_freq = gSavedSettings.getF32("KokuaRLVLogSummaryFrequency");
    if (rlv_log_freq > 0.f && gRecentRLVLogTime.getElapsedTimeF32() >= rlv_log_freq)
    {
        if (gRRenabled)
        {
            KokuaRLVExtras::writeLogSummary();
        }
        else
        {
            LL_INFOS_ONCE() << "RLV is disabled" << LL_ENDL;
        }
        gRecentRLVLogTime.reset();
    }
}

static void update_tp_display(bool minimized)
{
    static LLCachedControl<F32> teleport_arrival_delay(gSavedSettings, "TeleportArrivalDelay");
    static LLCachedControl<F32> teleport_local_delay(gSavedSettings, "TeleportLocalDelay");

    LLViewerCamera& camera = LLViewerCamera::instance(); // <FS:Ansariel> Factor out calls to getInstance

    S32 attach_count = 0;
    if (isAgentAvatarValid())
    {
        attach_count = gAgentAvatarp->getAttachmentCount();
    }
    F32 teleport_save_time = TELEPORT_EXPIRY + TELEPORT_EXPIRY_PER_ATTACHMENT * attach_count;
    F32 teleport_elapsed = gTeleportDisplayTimer.getElapsedTimeF32();
    F32 teleport_percent = teleport_elapsed * (100.f / teleport_save_time);
    if (gAgent.getTeleportState() != LLAgent::TELEPORT_START && teleport_percent > 100.f)
    {
        // Give up.  Don't keep the UI locked forever.
        LL_WARNS("Teleport") << "Giving up on teleport. elapsed time " << teleport_elapsed << " exceeds max time " << teleport_save_time << LL_ENDL;
        gAgent.setTeleportState(LLAgent::TELEPORT_NONE);
        gAgent.setTeleportMessage(std::string());
    }

    // Make sure the TP progress panel gets hidden in case the viewer window
    // is minimized *during* a TP. HB
    if (minimized)
    {
        gViewerWindow->setShowProgress(false);
    }

    const std::string& message = gAgent.getTeleportMessage();
    switch (gAgent.getTeleportState())
    {
        case LLAgent::TELEPORT_PENDING:
        {
            gTeleportDisplayTimer.reset();
            const std::string& msg = LLAgent::sTeleportProgressMessages["pending"];
            if (!minimized)
            {
                gViewerWindow->setShowProgress(true);
                gViewerWindow->setProgressPercent(llmin(teleport_percent, 0.0f));
                gViewerWindow->setProgressString(msg);
            }
            gAgent.setTeleportMessage(msg);
            break;
        }

        case LLAgent::TELEPORT_START:
        {
            // Transition to REQUESTED.  Viewer has sent some kind
            // of TeleportRequest to the source simulator

            // Reset view angle if in mouselook. Fixes camera angle getting stuck on teleport. -Zi
            if(gAgentCamera.cameraMouselook())
            {
                // If someone knows how to call "View.ZoomDefault" by hand, we should do that instead of
                // replicating the behavior here. -Zi
                camera.setDefaultFOV(DEFAULT_FIELD_OF_VIEW);
                if(gSavedSettings.getBOOL("FSResetCameraOnTP"))
                {
                    gSavedSettings.setF32("CameraAngle", camera.getView()); // FS:LO Dont reset rightclick zoom when we teleport however. Fixes FIRE-6246.
                    // KKA-1117 I can't see anything wrong in the merge around the handling of the signal
                    // connected to this control setting, however the signal isn't firing.
                    // Work around it by still setting the control variable but also calling
                    // setDefaultFOV directly to do what the signal handler should be doing.
                    LLViewerCamera::getInstance()->setDefaultFOV(camera.getView());
                }
                // also, reset the marker for "currently zooming" in the mouselook zoom settings. -Zi
                LLVector3 vTemp=gSavedSettings.getVector3("_NACL_MLFovValues");
                vTemp.mV[2]=0.0f;
                gSavedSettings.setVector3("_NACL_MLFovValues",vTemp);
            }

            gTeleportDisplayTimer.reset();
            const std::string& msg = LLAgent::sTeleportProgressMessages["requesting"];
            LL_INFOS("Teleport") << "A teleport request has been sent, setting state to TELEPORT_REQUESTED" << LL_ENDL;
            gAgent.setTeleportState(LLAgent::TELEPORT_REQUESTED);
            gAgent.setTeleportMessage(msg);
            if (!minimized)
            {
                gViewerWindow->setShowProgress(true);
                gViewerWindow->setProgressPercent(llmin(teleport_percent, 0.0f));
                gViewerWindow->setProgressString(msg);
                gViewerWindow->setProgressMessage(gAgent.mMOTD);
            }
            break;
        }

        case LLAgent::TELEPORT_REQUESTED:
            // Waiting for source simulator to respond
            if (!minimized)
            {
                gViewerWindow->setProgressPercent(llmin(teleport_percent, 37.5f));
                gViewerWindow->setProgressString(message);
            }
            break;

        case LLAgent::TELEPORT_MOVING:
            // Viewer has received destination location from source simulator
            if (!minimized)
            {
                gViewerWindow->setProgressPercent(llmin(teleport_percent, 75.f));
                gViewerWindow->setProgressString(message);
            }
            break;

        case LLAgent::TELEPORT_START_ARRIVAL:
            // Transition to ARRIVING.  Viewer has received avatar update, etc.,
            // from destination simulator
            gTeleportArrivalTimer.reset();
            LL_INFOS("Teleport") << "Changing state to TELEPORT_ARRIVING" << LL_ENDL;
            gAgent.setTeleportState(LLAgent::TELEPORT_ARRIVING);
            gAgent.setTeleportMessage(LLAgent::sTeleportProgressMessages["arriving"]);
            gAgent.sheduleTeleportIM();
            gTextureList.mForceResetTextureStats = true;
//MK
            // Let's not reset the view, we could be stuck in mouselook with @camdistmax set to 0
////            gAgentCamera.resetView(true, true);
//mk
            if (!minimized)
            {
                gViewerWindow->setProgressCancelButtonVisible(false, LLTrans::getString("Cancel"));
                gViewerWindow->setProgressPercent(75.f);
            }
            // <FS:Ansariel> FIRE-12004: Attachments getting lost on TP
            gPostTeleportFinishKillObjectDelayTimer.reset();
            break;

        case LLAgent::TELEPORT_ARRIVING:
        // Make the user wait while content "pre-caches"
        {
            F32 arrival_fraction = (gTeleportArrivalTimer.getElapsedTimeF32() / teleport_arrival_delay());
            if (arrival_fraction > 1.f)
            {
                arrival_fraction = 1.f;
                //LLFirstUse::useTeleport();
                LL_INFOS("Teleport") << "arrival_fraction is " << arrival_fraction << " changing state to TELEPORT_NONE" << LL_ENDL;
                gAgent.setTeleportState(LLAgent::TELEPORT_NONE);
            }
            if (!minimized)
            {
                gViewerWindow->setProgressCancelButtonVisible(false, LLTrans::getString("Cancel"));
                gViewerWindow->setProgressPercent(arrival_fraction * 25.f + 75.f);
                gViewerWindow->setProgressString(message);
            }
            break;
        }

        case LLAgent::TELEPORT_LOCAL:
        // Short delay when teleporting in the same sim (progress screen active but not shown - did not
        // fall-through from TELEPORT_START)
        {
            if (gTeleportDisplayTimer.getElapsedTimeF32() > teleport_local_delay())
            {
                //LLFirstUse::useTeleport();
                LL_INFOS("Teleport") << "State is local and gTeleportDisplayTimer " << gTeleportDisplayTimer.getElapsedTimeF32()
                                     << " exceeds teleport_local_delete " << teleport_local_delay
                                     << "; setting state to TELEPORT_NONE"
                                     << LL_ENDL;
                gAgent.setTeleportState(LLAgent::TELEPORT_NONE);
            }
            break;
        }

        case LLAgent::TELEPORT_NONE:
            // No teleport in progress
            gViewerWindow->setShowProgress(false);
            gTeleportDisplay = false;
    }
}

static void render_vulkan_world_frame()
{
    LL_PROFILE_ZONE_NAMED_CATEGORY_DISPLAY("Vulkan world frame");
    static bool logged = false;
    if (!logged)
    {
        LL_WARNS("RenderBackend")
            << "Vulkan world path is active. Scene updates continue and supported draw pools use the world command bridge."
            << LL_ENDL;
        logged = true;
    }

    LLColor4 clear_color = gSky.mVOSkyp ?
        gSky.getSkyFogColor() :
        LLColor4(0.025f, 0.03f, 0.04f, 1.f);

    gGL.setColorMask(true, true);
    getRenderBackend().setClearColor(
        clear_color.mV[VRED],
        clear_color.mV[VGREEN],
        clear_color.mV[VBLUE],
        1.f);
    const bool rendered_deferred_screen =
        render_vulkan_world_to_deferred_screen(clear_color);
    if (!rendered_deferred_screen)
    {
        LL_WARNS_ONCE("RenderBackend")
            << "Vulkan deferredScreen did not render this frame; skipping direct world fallback so the main render-target path failure stays visible."
            << LL_ENDL;
    }

    if (rendered_deferred_screen)
    {
        if (use_vulkan_smoke_sky_scene())
        {
            LL_WARNS_ONCE("RenderBackend")
                << "Vulkan smoke scene is copying the synthetic deferredScreen color target directly to the swapchain; deferred lighting/composite is bypassed for this test."
                << LL_ENDL;
            render_vulkan_copy_target_to_swapchain(
                gPipeline.mRT->deferredScreen,
                clear_color);
        }
        else if (use_vulkan_debug_copy_deferred_color_to_swapchain())
        {
            LL_WARNS_ONCE("RenderBackend")
                << "Vulkan debug is copying deferredScreen color directly to the swapchain; deferred lighting/composite is bypassed for diagnosis."
                << LL_ENDL;
            render_vulkan_copy_target_to_swapchain(
                gPipeline.mRT->deferredScreen,
                clear_color);
        }
        else if (use_vulkan_staged_post_targets())
        {
            LLRenderTarget* lit_world_target =
                render_vulkan_deferred_screen_to_light_target(clear_color);
            if (render_vulkan_lit_world_to_screen_target(lit_world_target, clear_color))
            {
                LLRenderTarget& post_target =
                    render_vulkan_screen_target_to_post_target(gPipeline.mRT->screen, clear_color);
                bool final_composite_applied = false;
                LLRenderTarget& final_world_target =
                    render_vulkan_post_target_to_final_target(
                        post_target,
                        clear_color,
                        final_composite_applied);
                if (final_composite_applied)
                {
                    render_vulkan_copy_target_to_swapchain(final_world_target, clear_color);
                }
                else
                {
                    render_vulkan_screen_target_to_swapchain(final_world_target, clear_color);
                }
            }
            else
            {
                render_vulkan_deferred_screen_to_swapchain(clear_color);
                render_vulkan_depth_prepass();
                LLGLSUIDefault gls_ui;
                gViewerWindow->setup3DRender();
                gPipeline.disableLights();
                render_vulkan_existing_world_post_geometry();
            }
        }
        else
        {
            LL_WARNS_ONCE("RenderBackend")
                << "Vulkan staged post targets are bypassed after a black-frame regression; compositing deferredScreen directly to the swapchain until the offscreen hops are revalidated."
                << LL_ENDL;
            render_vulkan_deferred_screen_to_swapchain(clear_color);
            if (use_vulkan_post_deferred_overlays_after_composite())
            {
                render_vulkan_depth_prepass();
                LLGLSUIDefault gls_ui;
                gViewerWindow->setup3DRender();
                gPipeline.disableLights();
                render_vulkan_existing_world_post_geometry();
            }
            else
            {
                LL_WARNS_ONCE("RenderBackend")
                    << "Vulkan post-deferred overlays are skipped while diagnosing the deferredScreen black-frame path."
                    << LL_ENDL;
            }
        }
    }
    else
    {
        getRenderBackend().clear(LL_RENDER_CLEAR_COLOR | LL_RENDER_CLEAR_DEPTH);
    }

    // UI is submitted by render_ui_internal(..., false) after the Vulkan world
    // frame so it uses the normal UI shader setup without re-entering
    // renderFinalize().
}

// Paint the display!
void display(bool rebuild, F32 zoom_factor, int subfield, bool for_snapshot)
{
    LL_PROFILE_ZONE_NAMED_CATEGORY_DISPLAY("Render");
    LL_PROFILE_GPU_ZONE("Render");

    LLPerfStats::RecordSceneTime T (LLPerfStats::StatType_t::RENDER_DISPLAY); // render time capture - This is the main stat for overall rendering.

    const bool vulkan_backend_ready = use_vulkan_world_path();

    if (gWindowResized)
    { //skip render on frames where window has been resized
        LL_DEBUGS("Window") << "Resizing window" << LL_ENDL;
        LL_PROFILE_ZONE_NAMED_CATEGORY_DISPLAY("Resize Window");
        gGL.flush();
        getRenderBackend().clear(LL_RENDER_CLEAR_COLOR);
        gViewerWindow->getWindow()->swapBuffers();
        LLPipeline::refreshCachedSettings();
        gPipeline.resizeScreenTexture();
        gResizeScreenTexture = false;
        gWindowResized = false;
        return;
    }

    if (gResizeShadowTexture)
    { //skip render on frames where window has been resized
        if (!vulkan_backend_ready)
        {
            gPipeline.resizeShadowTexture();
        }
        gResizeShadowTexture = false;
    }

    gSnapshot = for_snapshot;

    if (LLPipeline::sRenderDeferred)
    { //hack to make sky show up in deferred snapshots
        for_snapshot = false;
    }

    LLGLSDefault gls_default;
    LLGLDepthTest gls_depth(true, true, LLRenderDepthFunction::LessEqual);

    LLVertexBuffer::unbind();

    LLGLState::checkStates();

    gPipeline.disableLights();

    // Don't draw if the window is hidden or minimized.
    // In fact, must explicitly check the minimized state before drawing.
    // Attempting to draw into a minimized window causes a GL error. JC
    if (   !gViewerWindow->getActive()
        || !gViewerWindow->getWindow()->getVisible()
        || gViewerWindow->getWindow()->getMinimized()
        || gNonInteractive)
    {
        // Clean up memory the pools may have allocated
        if (rebuild)
        {
            stop_glerror();
            gPipeline.rebuildPools();
            stop_glerror();
        }

        stop_glerror();
        gViewerWindow->returnEmptyPicks();
        stop_glerror();

        // We still need to update the teleport progress (to get changes done
        // in TP states, else the sim does not get the messages signaling the
        // agent's arrival). This fixes BUG-230616. HB
        if (gTeleportDisplay)
        {
            // true = minimized, do not show/update the TP screen. HB
            update_tp_display(true);
        }

        // Run texture subsystem to discard memory while backgrounded
        if (!gNonInteractive)
        {
            LL_PROFILE_ZONE_NAMED("Update Images");

            {
                LL_PROFILE_ZONE_NAMED_CATEGORY_DISPLAY("Class");
                LLViewerTexture::updateClass();
            }

            {
                LL_PROFILE_ZONE_NAMED_CATEGORY_DISPLAY("Image Update Bump");
                gBumpImageList.updateImages();  // must be called before gTextureList version so that it's textures are thrown out first.
            }

            {
                LL_PROFILE_ZONE_NAMED_CATEGORY_DISPLAY("List");
                F32 max_image_decode_time = 0.050f * gFrameIntervalSeconds.value();          // 50 ms/second decode time
                max_image_decode_time     = llclamp(max_image_decode_time, 0.002f, 0.005f);  // min 2ms/frame, max 5ms/frame)
                gTextureList.updateImages(max_image_decode_time);
            }

            {
                LL_PROFILE_ZONE_NAMED_CATEGORY_DISPLAY("GLTF Materials Cleanup");
                // remove dead gltf materials
                gGLTFMaterialList.flushMaterials();
            }
        }
        return;
    }

    gViewerWindow->checkSettings();

    {
        LL_PROFILE_ZONE_NAMED_CATEGORY_DISPLAY("Picking");
        gViewerWindow->performPick();
    }

    LLAppViewer::instance()->pingMainloopTimeout("Display:CheckStates");
    LLGLState::checkStates();

    //////////////////////////////////////////////////////////
    //
    // Logic for forcing window updates if we're in drone mode.
    //

    // *TODO: Investigate running display() during gHeadlessClient.  See if this early exit is needed DK 2011-02-18
    if (gHeadlessClient)
    {
#if LL_WINDOWS
        static F32 last_update_time = 0.f;
        if ((gFrameTimeSeconds - last_update_time) > 1.f)
        {
            InvalidateRect((HWND)gViewerWindow->getPlatformWindow(), NULL, false);
            last_update_time = gFrameTimeSeconds;
        }
#elif LL_DARWIN
        // MBW -- Do something clever here.
#endif
        // Not actually rendering, don't bother.
        return;
    }

    //
    // Bail out if we're in the startup state and don't want to try to
    // render the world.
    //
    if (LLStartUp::getStartupState() < STATE_PRECACHE)
    {
        LLAppViewer::instance()->pingMainloopTimeout("Display:Startup");
        display_startup();
        return;
    }

    const bool vulkan_world_path = vulkan_backend_ready;

    if (gShaderProfileFrame)
    {
        LLGLSLShader::initProfile();
    }

    //LLGLState::verify(false);

    /////////////////////////////////////////////////
    //
    // Update GL Texture statistics (used for discard logic?)
    //

    LLAppViewer::instance()->pingMainloopTimeout("Display:TextureStats");
    stop_glerror();

    LLImageGL::updateStats(gFrameTimeSeconds);

    static LLCachedControl<S32> avatar_name_tag_mode(gSavedSettings, "AvatarNameTagMode", 1);
    static LLCachedControl<bool> name_tag_show_group_titles(gSavedSettings, "NameTagShowGroupTitles", true);
    LLVOAvatar::sRenderName = avatar_name_tag_mode;
    LLVOAvatar::sRenderGroupTitles = name_tag_show_group_titles && avatar_name_tag_mode > 0;

    gPipeline.mBackfaceCull = true;
    gFrameCount++;
    gRecentFrameCount++;
    if (gFocusMgr.getAppHasFocus())
    {
        gForegroundFrameCount++;
    }

    //////////////////////////////////////////////////////////
    //
    // Display start screen if we're teleporting, and skip render
    //

    if (gTeleportDisplay)
    {
        LL_PROFILE_ZONE_NAMED_CATEGORY_DISPLAY("Teleport Display");
        LLAppViewer::instance()->pingMainloopTimeout("Display:Teleport");
        // Note: false = not minimized, do update the TP screen. HB
        update_tp_display(false);
    }
    else if(LLAppViewer::instance()->logoutRequestSent())
    {
        LLAppViewer::instance()->pingMainloopTimeout("Display:Logout");
        F32 percent_done = gLogoutTimer.getElapsedTimeF32() * 100.f / gLogoutMaxTime;
        if (percent_done > 100.f)
        {
            percent_done = 100.f;
        }

        if( LLApp::isExiting() )
        {
            percent_done = 100.f;
        }

        gViewerWindow->setProgressPercent( percent_done );
        gViewerWindow->setProgressMessage(std::string());
    }
    else
    if (gRestoreGL)
    {
        LLAppViewer::instance()->pingMainloopTimeout("Display:RestoreGL");
        F32 percent_done = gRestoreGLTimer.getElapsedTimeF32() * 100.f / RESTORE_GL_TIME;
        if( percent_done > 100.f )
        {
            gViewerWindow->setShowProgress(false);
            gRestoreGL = false;
        }
        else
        {

            if( LLApp::isExiting() )
            {
                percent_done = 100.f;
            }

            gViewerWindow->setProgressPercent( percent_done );
        }
        gViewerWindow->setProgressMessage(std::string());
    }

    //////////////////////////
    //
    // Prepare for the next frame
    //

    /////////////////////////////
    //
    // Update the camera
    //
    //

    LLAppViewer::instance()->pingMainloopTimeout("Display:Camera");
    if (LLViewerCamera::instanceExists())
    {
        LLViewerCamera::getInstance()->setZoomParameters(zoom_factor, subfield);
        LLViewerCamera::getInstance()->setNear(MIN_NEAR_PLANE);
    }

    //////////////////////////
    //
    // clear the next buffer
    // (must follow dynamic texture writing since that uses the frame buffer)
    //

    if (gDisconnected)
    {
        LLAppViewer::instance()->pingMainloopTimeout("Display:Disconnected");
        render_ui();
        swap();
    }

    //////////////////////////
    //
    // Set rendering options
    //
    //
    LLAppViewer::instance()->pingMainloopTimeout("Display:RenderSetup");
    stop_glerror();

    ///////////////////////////////////////
    //
    // Slam lighting parameters back to our defaults.
    // Note that these are not the same as GL defaults...

    stop_glerror();
    gGL.setAmbientLightColor(LLColor4::white);
    stop_glerror();

    /////////////////////////////////////
    //
    // Render
    //
    // Actually push all of our triangles to the screen.
    //

    // do render-to-texture stuff here
//MK
    //if (gRRenabled && gAgent.mRRInterface.mContainsCamFocus)
    //{
//mk
    if (!vulkan_world_path &&
        gPipeline.hasRenderDebugFeatureMask(LLPipeline::RENDER_DEBUG_FEATURE_DYNAMIC_TEXTURES))
    {
        LLAppViewer::instance()->pingMainloopTimeout("Display:DynamicTextures");
        LL_PROFILE_ZONE_NAMED_CATEGORY_DISPLAY("Update Dynamic Textures");
        if (LLViewerDynamicTexture::updateAllInstances())
        {
            gGL.setColorMask(true, true);
            getRenderBackend().clear(LL_RENDER_CLEAR_DEPTH);
        }
    }
    else if (vulkan_world_path &&
        gPipeline.hasRenderDebugFeatureMask(LLPipeline::RENDER_DEBUG_FEATURE_DYNAMIC_TEXTURES))
    {
        LL_WARNS_ONCE("RenderBackend")
            << "Vulkan skipped legacy OpenGL stage: dynamic texture update. Dynamic texture rendering still needs Vulkan render-target ownership."
            << LL_ENDL;
    }
//MK
    //}
//mk

    gViewerWindow->setup3DViewport();

    gPipeline.resetFrameStats();    // Reset per-frame statistics.

    if (!gDisconnected && !LLApp::isExiting())
    {
        // Render mirrors and associated hero probes before we render the rest of the scene.
        // This ensures the scene state in the hero probes are exactly the same as the rest of the scene before we render it.
        if (!vulkan_world_path && gPipeline.RenderMirrors && !gSnapshot)
        {
            LL_PROFILE_ZONE_NAMED_CATEGORY_DISPLAY("Update hero probes");
            LL_PROFILE_GPU_ZONE("hero manager")
            gPipeline.mHeroProbeManager.update();
            gPipeline.mHeroProbeManager.renderProbes();
        }
        else if (vulkan_world_path && gPipeline.RenderMirrors && !gSnapshot)
        {
            LL_WARNS_ONCE("RenderBackend")
                << "Vulkan skipped legacy OpenGL stage: hero/reflection probes. Probe rendering still needs Vulkan cubemap/render-target ownership."
                << LL_ENDL;
        }

        LL_PROFILE_ZONE_NAMED_CATEGORY_DISPLAY("display - 1");
        LLAppViewer::instance()->pingMainloopTimeout("Display:Update");
        if (gPipeline.hasRenderType(LLPipeline::RENDER_TYPE_HUD))
        { //don't draw hud objects in this frame
            gPipeline.toggleRenderType(LLPipeline::RENDER_TYPE_HUD);
        }

        if (gPipeline.hasRenderType(LLPipeline::RENDER_TYPE_HUD_PARTICLES))
        { //don't draw hud particles in this frame
            gPipeline.toggleRenderType(LLPipeline::RENDER_TYPE_HUD_PARTICLES);
        }

        stop_glerror();
        display_update_camera();
        stop_glerror();

        {
            LL_PROFILE_ZONE_NAMED_CATEGORY_DISPLAY("Env Update");
            // update all the sky/atmospheric/water settings
            LLEnvironment::instance().update(LLViewerCamera::getInstance());
        }

        // *TODO: merge these two methods
        {
            LL_PROFILE_ZONE_NAMED_CATEGORY_DISPLAY("HUD Update");
            LLHUDManager::getInstance()->updateEffects();
            LLHUDObject::updateAll();
            stop_glerror();
        }

        {
            LL_PROFILE_ZONE_NAMED_CATEGORY_DISPLAY("Update Geom");
            const F32 max_geom_update_time = 0.005f*10.f*gFrameIntervalSeconds.value(); // 50 ms/second update time
            gPipeline.createObjects(max_geom_update_time);
            gPipeline.processPartitionQ();
            gPipeline.updateGeom(max_geom_update_time);
            stop_glerror();
        }

        gPipeline.updateGL();

        stop_glerror();

        LLAppViewer::instance()->pingMainloopTimeout("Display:Cull");

        //Increment drawable frame counter
        LLDrawable::incrementVisible();

        LLSpatialGroup::sNoDelete = true;
        LLTexUnit::sWhiteTexture = LLViewerFetchedTexture::sWhiteImagep->getTexName();

        S32 occlusion = LLPipeline::sUseOcclusion;
        if (gDepthDirty)
        { //depth buffer is invalid, don't overwrite occlusion state
            LLPipeline::sUseOcclusion = llmin(occlusion, 1);
        }
        gDepthDirty = false;

        LLGLState::checkStates();

        static LLCullResult result;
        LLViewerCamera::sCurCameraID = LLViewerCamera::CAMERA_WORLD;
        LLPipeline::sUnderWaterRender = LLViewerCamera::getInstance()->cameraUnderWater();
        gPipeline.updateCull(*LLViewerCamera::getInstance(), result);
        stop_glerror();

        LLGLState::checkStates();

        LLAppViewer::instance()->pingMainloopTimeout("Display:Swap");

        {
            LL_PROFILE_ZONE_NAMED_CATEGORY_DISPLAY("display - 2")
            if (gResizeScreenTexture)
            {
                gPipeline.resizeScreenTexture();
                gResizeScreenTexture = false;
            }

            gGL.setColorMask(true, true);
            getRenderBackend().setClearColor(0.f, 0.f, 0.f, 0.f);

            LLGLState::checkStates();

            if (!vulkan_world_path && !for_snapshot)
            {
                if (gFrameCount > 1 && !for_snapshot)
                { //for some reason, ATI 4800 series will error out if you
                  //try to generate a shadow before the first frame is through
                    gPipeline.generateSunShadow(*LLViewerCamera::getInstance());
                }

                LLVertexBuffer::unbind();

                LLGLState::checkStates();

                glm::mat4 proj = get_current_projection();
                glm::mat4 mod = get_current_modelview();
                getRenderBackend().setViewport(0, 0, 512, 512);

                LLVOAvatar::updateImpostors();

                set_current_projection(proj);
                set_current_modelview(mod);
                gGL.matrixMode(LLRender::MM_PROJECTION);
                gGL.loadMatrix(glm::value_ptr(proj));
                gGL.matrixMode(LLRender::MM_MODELVIEW);
                gGL.loadMatrix(glm::value_ptr(mod));
                gViewerWindow->setup3DViewport();

                LLGLState::checkStates();
            }
            else if (vulkan_world_path && !for_snapshot)
            {
                LL_WARNS_ONCE("RenderBackend")
                    << "Vulkan skipped legacy OpenGL stage: sun shadow generation. Shadow maps still need Vulkan render-pass and shadow pipeline ownership."
                    << LL_ENDL;
            }
            getRenderBackend().clear(LL_RENDER_CLEAR_DEPTH);
        }

        //////////////////////////////////////
        //
        // Update images, using the image stats generated during object update/culling
        //
        // Can put objects onto the retextured list.
        //
        // Doing this here gives hardware occlusion queries extra time to complete
        LLAppViewer::instance()->pingMainloopTimeout("Display:UpdateImages");

        {
            LL_PROFILE_ZONE_NAMED("Update Images");

            {
                LL_PROFILE_ZONE_NAMED_CATEGORY_DISPLAY("Class");
                LLViewerTexture::updateClass();
            }

            {
                LL_PROFILE_ZONE_NAMED_CATEGORY_DISPLAY("Image Update Bump");
                gBumpImageList.updateImages();  // must be called before gTextureList version so that it's textures are thrown out first.
            }

            {
                LL_PROFILE_ZONE_NAMED_CATEGORY_DISPLAY("List");
                F32 max_image_decode_time = 0.050f*gFrameIntervalSeconds.value(); // 50 ms/second decode time
                max_image_decode_time = llclamp(max_image_decode_time, 0.002f, 0.005f ); // min 2ms/frame, max 5ms/frame)
                gTextureList.updateImages(max_image_decode_time);
            }

            {
                LL_PROFILE_ZONE_NAMED_CATEGORY_DISPLAY("GLTF Materials Cleanup");
                //remove dead gltf materials
                gGLTFMaterialList.flushMaterials();
            }
        }

        LLGLState::checkStates();

        ///////////////////////////////////
        //
        // StateSort
        //
        // Responsible for taking visible objects, and adding them to the appropriate draw orders.
        // In the case of alpha objects, z-sorts them first.
        // Also creates special lists for outlines and selected face rendering.
        //
        LLAppViewer::instance()->pingMainloopTimeout("Display:StateSort");
        {
            LL_PROFILE_ZONE_NAMED_CATEGORY_DISPLAY("display - 4")
            LLViewerCamera::sCurCameraID = LLViewerCamera::CAMERA_WORLD;
            gPipeline.stateSort(*LLViewerCamera::getInstance(), result);
            stop_glerror();

            if (rebuild)
            {
                //////////////////////////////////////
                //
                // rebuildPools
                //
                //
                gPipeline.rebuildPools();
                stop_glerror();
            }
        }

        LLSceneMonitor::getInstance()->fetchQueryResult();

        LLGLState::checkStates();

        LLPipeline::sUseOcclusion = occlusion;

        {
            LLAppViewer::instance()->pingMainloopTimeout("Display:Sky");
            LL_PROFILE_ZONE_NAMED_CATEGORY_ENVIRONMENT("update sky"); //LL_RECORD_BLOCK_TIME(FTM_UPDATE_SKY);
            gSky.updateSky();
        }

        if (vulkan_world_path)
        {
            LLAppViewer::instance()->pingMainloopTimeout("Display:VulkanWorld");
            render_vulkan_world_frame();
            if (!for_snapshot)
            {
                if (use_vulkan_debug_skip_ui_after_world())
                {
                    LL_WARNS_ONCE("RenderBackend")
                        << "Vulkan debug is skipping UI after the world frame; this isolates world composite visibility from progress/UI overlays."
                        << LL_ENDL;
                }
                else
                {
                    LLAppViewer::instance()->pingMainloopTimeout("Display:RenderUI");
                    render_ui_internal(1.f, 0, false);
                }
                swap();
            }
        }
        else
        {
        if(gUseWireframe)
        {
            getRenderBackend().setClearColor(0.5f, 0.5f, 0.5f, 0.f);
            getRenderBackend().clear(LL_RENDER_CLEAR_COLOR);
        }

        LLAppViewer::instance()->pingMainloopTimeout("Display:RenderStart");

        //// render frontmost floater opaque for occlusion culling purposes
        //LLFloater* frontmost_floaterp = gFloaterView->getFrontmost();
        //// assumes frontmost floater with focus is opaque
        //if (frontmost_floaterp && gFocusMgr.childHasKeyboardFocus(frontmost_floaterp))
        //{
        //  gGL.matrixMode(LLRender::MM_MODELVIEW);
        //  gGL.pushMatrix();
        //  {
        //      gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);

        //      Configure the color mask for alpha-only writes here.
        //      gGL.loadIdentity();

        //      LLRect floater_rect = frontmost_floaterp->calcScreenRect();
        //      // deflate by one pixel so rounding errors don't occlude outside of floater extents
        //      floater_rect.stretch(-1);
        //      LLRectf floater_3d_rect((F32)floater_rect.mLeft / (F32)gViewerWindow->getWindowWidthScaled(),
        //                              (F32)floater_rect.mTop / (F32)gViewerWindow->getWindowHeightScaled(),
        //                              (F32)floater_rect.mRight / (F32)gViewerWindow->getWindowWidthScaled(),
        //                              (F32)floater_rect.mBottom / (F32)gViewerWindow->getWindowHeightScaled());
        //      floater_3d_rect.translate(-0.5f, -0.5f);
        //      gGL.translatef(0.f, 0.f, -LLViewerCamera::getInstance()->getNear());
        //      gGL.scalef(LLViewerCamera::getInstance()->getNear() * LLViewerCamera::getInstance()->getAspect() / sinf(LLViewerCamera::getInstance()->getView()), LLViewerCamera::getInstance()->getNear() / sinf(LLViewerCamera::getInstance()->getView()), 1.f);
        //      gGL.color4fv(LLColor4::white.mV);
        //      gGL.begin(LLVertexBuffer::QUADS);
        //      {
        //          gGL.vertex3f(floater_3d_rect.mLeft, floater_3d_rect.mBottom, 0.f);
        //          gGL.vertex3f(floater_3d_rect.mLeft, floater_3d_rect.mTop, 0.f);
        //          gGL.vertex3f(floater_3d_rect.mRight, floater_3d_rect.mTop, 0.f);
        //          gGL.vertex3f(floater_3d_rect.mRight, floater_3d_rect.mBottom, 0.f);
        //      }
        //      gGL.end();
        //      Restore full color mask writes here.
        //  }
        //  gGL.popMatrix();
        //}

        LLPipeline::sUnderWaterRender = LLViewerCamera::getInstance()->cameraUnderWater();

        LLGLState::checkStates();

        stop_glerror();

        gGL.setColorMask(true, true);

        gPipeline.mRT->deferredScreen.bindTarget();
        if (gUseWireframe)
        {
            constexpr F32 g = 0.5f;
            getRenderBackend().setClearColor(g, g, g, 1.f);
        }
        else
        {
            getRenderBackend().setClearColor(1, 0, 1, 1);
        }
        gPipeline.mRT->deferredScreen.clear();

        gGL.setColorMask(true, false);

        LLAppViewer::instance()->pingMainloopTimeout("Display:RenderGeom");

        if (!(LLAppViewer::instance()->logoutRequestSent() && LLAppViewer::instance()->hasSavedFinalSnapshot())
                && !gRestoreGL)
        {
            LL_PROFILE_ZONE_NAMED_CATEGORY_DISPLAY("display - 5")
            LLViewerCamera::sCurCameraID = LLViewerCamera::CAMERA_WORLD;

            static LLCachedControl<bool> render_depth_pre_pass(gSavedSettings, "RenderDepthPrePass", false);
            if (render_depth_pre_pass)
            {
                gGL.setColorMask(false, false);

                constexpr U32 types[] = {
                    LLRenderPass::PASS_SIMPLE,
                    LLRenderPass::PASS_FULLBRIGHT,
                    LLRenderPass::PASS_SHINY
                };

                U32 num_types = LL_ARRAY_SIZE(types);
                gOcclusionProgram.bind();
                for (U32 i = 0; i < num_types; i++)
                {
                    gPipeline.renderObjects(types[i], LLVertexBuffer::MAP_VERTEX, false);
                }

                gOcclusionProgram.unbind();

            }

            gGL.setColorMask(true, true);
            gPipeline.renderGeomDeferred(*LLViewerCamera::getInstance(), true);
        }

        {
            LL_PROFILE_ZONE_NAMED_CATEGORY_DISPLAY("Texture Unbind");
            for (S32 i = 0; i < gGLManager.mNumTextureImageUnits; i++)
            { //dummy cleanup of any currently bound textures
                if (gGL.getTexUnit(i)->getCurrType() != LLTexUnit::TT_NONE)
                {
                    gGL.getTexUnit(i)->unbind(gGL.getTexUnit(i)->getCurrType());
                    gGL.getTexUnit(i)->disable();
                }
            }
        }

        LLAppViewer::instance()->pingMainloopTimeout("Display:RenderFlush");

        LLRenderTarget &rt = (gPipeline.sRenderDeferred ? gPipeline.mRT->deferredScreen : gPipeline.mRT->screen);
        rt.flush();

        if (LLPipeline::sRenderDeferred)
        {
            gPipeline.renderDeferredLighting();
        }

        LLPipeline::sUnderWaterRender = false;

        {
            //capture the frame buffer.
            LLSceneMonitor::getInstance()->capture();
        }

        LLAppViewer::instance()->pingMainloopTimeout("Display:RenderUI");
        if (!for_snapshot)
        {
            render_ui();
            swap();
        }
        }

        LLPipeline::sUnderWaterRender = false;
        LLSpatialGroup::sNoDelete = false;
        gPipeline.clearReferences();
    }

    LLAppViewer::instance()->pingMainloopTimeout("Display:FrameStats");

    stop_glerror();

    display_stats();

    LLAppViewer::instance()->pingMainloopTimeout("Display:Done");

    gShiftFrame = false;

    if (gShaderProfileFrame)
    {
        gShaderProfileFrame = false;
        boost::json::value stats{ boost::json::object_kind };
        getProfileStatsContext(stats.as_object());
        LLGLSLShader::finishProfile(stats);

        auto report_name = getProfileStatsFilename();
        std::ofstream outf(report_name);
        if (! outf)
        {
            LL_WARNS() << "Couldn't write to " << std::quoted(report_name) << LL_ENDL;
        }
        else
        {
            outf << stats;
            LL_INFOS() << "(also dumped to " << std::quoted(report_name) << ")" << LL_ENDL;
        }
    }
}

void getProfileStatsContext(boost::json::object& stats)
{
    // populate the context with info from LLFloaterAbout
    auto contextit = stats.emplace("context",
                                   LlsdToJson(LLAppViewer::instance()->getViewerInfo())).first;
    auto& context = contextit->value().as_object();

    // then add a few more things
    unsigned char unique_id[MAC_ADDRESS_BYTES]{};
    LLMachineID::getUniqueID(unique_id, sizeof(unique_id));
    context.emplace("machine", stringize(LL::hexdump(unique_id, sizeof(unique_id))));
    context.emplace("grid", LLGridManager::instance().getGrid());
    LLViewerRegion* region = gAgent.getRegion();
    if (region)
    {
        context.emplace("regionid", stringize(region->getRegionID()));
    }
    LLParcel* parcel = LLViewerParcelMgr::instance().getAgentParcel();
    if (parcel)
    {
        context.emplace("parcel", parcel->getName());
        context.emplace("parcelid", parcel->getLocalID());
    }
    context.emplace("time", LLDate::now().toHTTPDateString("%Y-%m-%dT%H:%M:%S"));
}

std::string getProfileStatsFilename()
{
    std::ostringstream basebuff;
    // viewer build
    basebuff << "profile.v" << LLVersionInfo::instance().getBuild();
    // machine ID: zero-initialize unique_id in case LLMachineID fails
    unsigned char unique_id[MAC_ADDRESS_BYTES]{};
    LLMachineID::getUniqueID(unique_id, sizeof(unique_id));
    basebuff << ".m" << LL::hexdump(unique_id, sizeof(unique_id));
    // region ID
    LLViewerRegion *region = gAgent.getRegion();
    basebuff << ".r" << (region? region->getRegionID() : LLUUID());
    // local parcel ID
    LLParcel* parcel = LLViewerParcelMgr::instance().getAgentParcel();
    basebuff << ".p" << (parcel? parcel->getLocalID() : 0);
    // date/time -- omit seconds for now
    auto now = LLDate::now();
    basebuff << ".t" << LLDate::now().toHTTPDateString("%Y-%m-%dT%H-%M-");
    // put this candidate file in our logs directory
    auto base = gDirUtilp->getExpandedFilename(LL_PATH_LOGS, basebuff.str());
    S32 sec;
    now.split(nullptr, nullptr, nullptr, nullptr, nullptr, &sec);
    // Loop over finished filename, incrementing sec until we find one that
    // doesn't yet exist. Should rarely loop (only if successive calls within
    // same second), may produce (e.g.) sec==61, but avoids collisions and
    // preserves chronological filename sort order.
    std::string name;
    std::error_code ec;
    do
    {
        // base + missing 2-digit seconds, append ".json"
        // post-increment sec in case we have to try again
        name = stringize(base, std::setw(2), std::setfill('0'), sec++, ".json");
    } while (std::filesystem::exists(fsyspath(name), ec));
    // Ignoring ec means we might potentially return a name that does already
    // exist -- but if we can't check its existence, what more can we do?
    return name;
}

// WIP simplified copy of display() that does minimal work
void display_cube_face()
{
    LL_PROFILE_ZONE_NAMED_CATEGORY_DISPLAY("Render Cube Face");
    LL_PROFILE_GPU_ZONE("display cube face");

    llassert(!gSnapshot);
    llassert(!gTeleportDisplay);
    llassert(LLStartUp::getStartupState() >= STATE_PRECACHE);
    llassert(!LLAppViewer::instance()->logoutRequestSent());
    llassert(!gRestoreGL);

    bool rebuild = false;

    LLGLSDefault gls_default;
    LLGLDepthTest gls_depth(true, true, LLRenderDepthFunction::LessEqual);

    LLVertexBuffer::unbind();

    gPipeline.disableLights();

    gPipeline.mBackfaceCull = true;

    gViewerWindow->setup3DViewport();

    if (gPipeline.hasRenderType(LLPipeline::RENDER_TYPE_HUD))
    { //don't draw hud objects in this frame
        gPipeline.toggleRenderType(LLPipeline::RENDER_TYPE_HUD);
    }

    if (gPipeline.hasRenderType(LLPipeline::RENDER_TYPE_HUD_PARTICLES))
    { //don't draw hud particles in this frame
        gPipeline.toggleRenderType(LLPipeline::RENDER_TYPE_HUD_PARTICLES);
    }

    display_update_camera();

    {
        LL_PROFILE_ZONE_NAMED_CATEGORY_DISPLAY("Env Update");
        // update all the sky/atmospheric/water settings
        LLEnvironment::instance().update(LLViewerCamera::getInstance());
    }

    LLSpatialGroup::sNoDelete = true;

    S32 occlusion = LLPipeline::sUseOcclusion;
    LLPipeline::sUseOcclusion = 0; // occlusion data is from main camera point of view, don't read or write it during cube snapshots
    //gDepthDirty = true; //let "real" render pipe know it can't trust the depth buffer for occlusion data

    static LLCullResult result;
    LLViewerCamera::sCurCameraID = LLViewerCamera::CAMERA_WORLD;
    LLPipeline::sUnderWaterRender = LLViewerCamera::getInstance()->cameraUnderWater();
    gPipeline.updateCull(*LLViewerCamera::getInstance(), result);

    gGL.setColorMask(true, true);

    getRenderBackend().setClearColor(0.f, 0.f, 0.f, 0.f);
    gPipeline.generateSunShadow(*LLViewerCamera::getInstance());

    getRenderBackend().clear(LL_RENDER_CLEAR_DEPTH); // | LL_RENDER_CLEAR_STENCIL);

    {
        LLViewerCamera::sCurCameraID = LLViewerCamera::CAMERA_WORLD;
        gPipeline.stateSort(*LLViewerCamera::getInstance(), result);

        if (rebuild)
        {
            //////////////////////////////////////
            //
            // rebuildPools
            //
            //
            gPipeline.rebuildPools();
            stop_glerror();
        }
    }

    LLPipeline::sUseOcclusion = occlusion;

    LLAppViewer::instance()->pingMainloopTimeout("Display:RenderStart");

    LLPipeline::sUnderWaterRender = LLViewerCamera::getInstance()->cameraUnderWater();

    gGL.setColorMask(true, true);

    gPipeline.mRT->deferredScreen.bindTarget();
    if (gUseWireframe)
    {
        getRenderBackend().setClearColor(0.5f, 0.5f, 0.5f, 1.f);
    }
    else
    {
        getRenderBackend().setClearColor(1.f, 0.f, 1.f, 1.f);
    }
    gPipeline.mRT->deferredScreen.clear();

    LLViewerCamera::sCurCameraID = LLViewerCamera::CAMERA_WORLD;

    gPipeline.renderGeomDeferred(*LLViewerCamera::getInstance());

    gPipeline.mRT->deferredScreen.flush();

    gPipeline.renderDeferredLighting();

    LLPipeline::sUnderWaterRender = false;

    // Finalize scene
    //gPipeline.renderFinalize();

    LLSpatialGroup::sNoDelete = false;
    gPipeline.clearReferences();
}

void render_hud_attachments()
{
    LLPerfStats::RecordSceneTime T ( LLPerfStats::StatType_t::RENDER_HUDS); // render time capture - Primary contributor to HUDs (though these end up in render batches)
    gGL.matrixMode(LLRender::MM_PROJECTION);
    gGL.pushMatrix();
    gGL.matrixMode(LLRender::MM_MODELVIEW);
    gGL.pushMatrix();

    glm::mat4 current_proj = get_current_projection();
    glm::mat4 current_mod = get_current_modelview();

    // clamp target zoom level to reasonable values
//MK
    if (gRRenabled && gAgent.mRRInterface.mHasLockedHuds)
    {
        gAgentCamera.mHUDTargetZoom = llclamp(gAgentCamera.mHUDTargetZoom, 0.85f, 1.f);
    }
    else
//mk
        gAgentCamera.mHUDTargetZoom = llclamp(gAgentCamera.mHUDTargetZoom, 0.1f, 1.f);
    // smoothly interpolate current zoom level
    gAgentCamera.mHUDCurZoom = lerp(gAgentCamera.mHUDCurZoom, gAgentCamera.getAgentHUDTargetZoom(), LLSmoothInterpolation::getInterpolant(0.03f));

    if (use_vulkan_world_path() &&
        !use_vulkan_debug_render_legacy_hud_attachments())
    {
        LL_WARNS_ONCE("RenderBackend")
            << "Vulkan skipped legacy HUD attachment geometry. "
            << "render_hud_attachments() uses the legacy post-deferred HUD pipeline and can cover the Vulkan world frame. "
            << "Set MARE_VULKAN_DEBUG_RENDER_LEGACY_HUD_ATTACHMENTS=1 to re-enable it for isolation."
            << LL_ENDL;
        gGL.matrixMode(LLRender::MM_PROJECTION);
        gGL.popMatrix();
        gGL.matrixMode(LLRender::MM_MODELVIEW);
        gGL.popMatrix();
        set_current_projection(current_proj);
        set_current_modelview(current_mod);
        return;
    }

    if (LLPipeline::sShowHUDAttachments && !gDisconnected && setup_hud_matrices())
    {
        LLPipeline::sRenderingHUDs = true;
        LLCamera hud_cam = *LLViewerCamera::getInstance();
        hud_cam.setOrigin(-1.f, 0.f, 0.f);
        hud_cam.setAxes(LLVector3(1.f, 0.f, 0.f), LLVector3(0.f, 1.f, 0.f), LLVector3(0.f, 0.f, 1.f));
        LLViewerCamera::updateFrustumPlanes(hud_cam, true);

        static LLCachedControl<bool> render_hud_particles(gSavedSettings, "RenderHUDParticles", false);
        bool render_particles = gPipeline.hasRenderType(LLPipeline::RENDER_TYPE_PARTICLES) && render_hud_particles;

        //only render hud objects
        gPipeline.pushRenderTypeMask();

        // turn off everything
        gPipeline.andRenderTypeMask(LLPipeline::END_RENDER_TYPES);
        // turn on HUD
        gPipeline.toggleRenderType(LLPipeline::RENDER_TYPE_HUD);
        // turn on HUD particles
        gPipeline.toggleRenderType(LLPipeline::RENDER_TYPE_HUD_PARTICLES);

        // if particles are off, turn off hud-particles as well
        if (!render_particles)
        {
            // turn back off HUD particles
            gPipeline.toggleRenderType(LLPipeline::RENDER_TYPE_HUD_PARTICLES);
        }

        bool has_ui = gPipeline.hasRenderDebugFeatureMask(LLPipeline::RENDER_DEBUG_FEATURE_UI);
        if (has_ui)
        {
            gPipeline.toggleRenderDebugFeature(LLPipeline::RENDER_DEBUG_FEATURE_UI);
        }

        S32 use_occlusion = LLPipeline::sUseOcclusion;
        LLPipeline::sUseOcclusion = 0;

        //cull, sort, and render hud objects
        static LLCullResult result;
        LLSpatialGroup::sNoDelete = true;

        LLViewerCamera::sCurCameraID = LLViewerCamera::CAMERA_WORLD;
        gPipeline.updateCull(hud_cam, result);

        // Toggle render types
        gPipeline.toggleRenderType(LLPipeline::RENDER_TYPE_BUMP);
        gPipeline.toggleRenderType(LLPipeline::RENDER_TYPE_SIMPLE);
        gPipeline.toggleRenderType(LLPipeline::RENDER_TYPE_VOLUME);
        gPipeline.toggleRenderType(LLPipeline::RENDER_TYPE_ALPHA);
        gPipeline.toggleRenderType(LLPipeline::RENDER_TYPE_ALPHA_PRE_WATER);
        gPipeline.toggleRenderType(LLPipeline::RENDER_TYPE_ALPHA_MASK);
        gPipeline.toggleRenderType(LLPipeline::RENDER_TYPE_FULLBRIGHT_ALPHA_MASK);
        gPipeline.toggleRenderType(LLPipeline::RENDER_TYPE_FULLBRIGHT);
        gPipeline.toggleRenderType(LLPipeline::RENDER_TYPE_GLTF_PBR);
        gPipeline.toggleRenderType(LLPipeline::RENDER_TYPE_GLTF_PBR_ALPHA_MASK);

        // Toggle render passes
        gPipeline.toggleRenderType(LLPipeline::RENDER_TYPE_PASS_ALPHA);
        gPipeline.toggleRenderType(LLPipeline::RENDER_TYPE_PASS_ALPHA_MASK);
        gPipeline.toggleRenderType(LLPipeline::RENDER_TYPE_PASS_BUMP);
        gPipeline.toggleRenderType(LLPipeline::RENDER_TYPE_PASS_MATERIAL);
        gPipeline.toggleRenderType(LLPipeline::RENDER_TYPE_PASS_FULLBRIGHT);
        gPipeline.toggleRenderType(LLPipeline::RENDER_TYPE_PASS_FULLBRIGHT_ALPHA_MASK);
        gPipeline.toggleRenderType(LLPipeline::RENDER_TYPE_PASS_FULLBRIGHT_SHINY);
        gPipeline.toggleRenderType(LLPipeline::RENDER_TYPE_PASS_SHINY);
        gPipeline.toggleRenderType(LLPipeline::RENDER_TYPE_PASS_INVISIBLE);
        gPipeline.toggleRenderType(LLPipeline::RENDER_TYPE_PASS_INVISI_SHINY);
        gPipeline.toggleRenderType(LLPipeline::RENDER_TYPE_PASS_GLTF_PBR);
        gPipeline.toggleRenderType(LLPipeline::RENDER_TYPE_PASS_GLTF_PBR_ALPHA_MASK);

        gPipeline.stateSort(hud_cam, result);

        gPipeline.renderGeomPostDeferred(hud_cam);

        LLSpatialGroup::sNoDelete = false;
        //gPipeline.clearReferences();

        render_hud_elements();

        //restore type mask
        gPipeline.popRenderTypeMask();

        if (has_ui)
        {
            gPipeline.toggleRenderDebugFeature(LLPipeline::RENDER_DEBUG_FEATURE_UI);
        }
        LLPipeline::sUseOcclusion = use_occlusion;
        LLPipeline::sRenderingHUDs = false;
    }
    gGL.matrixMode(LLRender::MM_PROJECTION);
    gGL.popMatrix();
    gGL.matrixMode(LLRender::MM_MODELVIEW);
    gGL.popMatrix();

    set_current_projection(current_proj);
    set_current_modelview(current_mod);
}

LLRect get_whole_screen_region()
{
    LLRect whole_screen = gViewerWindow->getWorldViewRectScaled();

    // apply camera zoom transform (for high res screenshots)
    F32 zoom_factor = LLViewerCamera::getInstance()->getZoomFactor();
    S16 sub_region = LLViewerCamera::getInstance()->getZoomSubRegion();
    if (zoom_factor > 1.f)
    {
        S32 num_horizontal_tiles = llceil(zoom_factor);
        S32 tile_width = ll_round((F32)gViewerWindow->getWorldViewWidthScaled() / zoom_factor);
        S32 tile_height = ll_round((F32)gViewerWindow->getWorldViewHeightScaled() / zoom_factor);
        int tile_y = sub_region / num_horizontal_tiles;
        int tile_x = sub_region - (tile_y * num_horizontal_tiles);

        whole_screen.setLeftTopAndSize(tile_x * tile_width, gViewerWindow->getWorldViewHeightScaled() - (tile_y * tile_height), tile_width, tile_height);
    }
    return whole_screen;
}

bool get_hud_matrices(const LLRect& screen_region, glm::mat4 &proj, glm::mat4&model)
{
    if (isAgentAvatarValid() && gAgentAvatarp->hasHUDAttachment())
    {
        F32 zoom_level = gAgentCamera.mHUDCurZoom;
        LLBBox hud_bbox = gAgentAvatarp->getHUDBBox();

        F32 hud_depth = llmax(1.f, hud_bbox.getExtentLocal().mV[VX] * 1.1f);
        proj = glm::ortho(-0.5f * LLViewerCamera::getInstance()->getAspect(), 0.5f * LLViewerCamera::getInstance()->getAspect(), -0.5f, 0.5f, 0.f, hud_depth);
        proj[2][2] = -0.01f;

        F32 aspect_ratio = LLViewerCamera::getInstance()->getAspect();

        F32 scale_x = (F32)gViewerWindow->getWorldViewWidthScaled() / (F32)screen_region.getWidth();
        F32 scale_y = (F32)gViewerWindow->getWorldViewHeightScaled() / (F32)screen_region.getHeight();

        glm::mat4 mat = glm::identity<glm::mat4>();
        mat = glm::translate(mat,
            glm::vec3(clamp_rescale((F32)(screen_region.getCenterX() - screen_region.mLeft), 0.f, (F32)gViewerWindow->getWorldViewWidthScaled(), 0.5f * scale_x * aspect_ratio, -0.5f * scale_x * aspect_ratio),
                clamp_rescale((F32)(screen_region.getCenterY() - screen_region.mBottom), 0.f, (F32)gViewerWindow->getWorldViewHeightScaled(), 0.5f * scale_y, -0.5f * scale_y),
                0.f));
        mat = glm::scale(mat, glm::vec3(scale_x, scale_y, 1.f));
        proj *= mat;

        glm::mat4 tmp_model = glm::make_mat4(OGL_TO_CFR_ROTATION);
        mat = glm::identity<glm::mat4>();
        mat = glm::translate(mat, glm::vec3(-hud_bbox.getCenterLocal().mV[VX] + (hud_depth * 0.5f), 0.f, 0.f));
        mat = glm::scale(mat, glm::vec3(zoom_level));
        tmp_model *= mat;
        model = tmp_model;

        return true;
    }
    else
    {
        return false;
    }
}

bool get_hud_matrices(glm::mat4 &proj, glm::mat4&model)
{
    LLRect whole_screen = get_whole_screen_region();
    return get_hud_matrices(whole_screen, proj, model);
}

bool setup_hud_matrices()
{
    LLRect whole_screen = get_whole_screen_region();
    return setup_hud_matrices(whole_screen);
}

bool setup_hud_matrices(const LLRect& screen_region)
{
    glm::mat4 proj, model;
    bool result = get_hud_matrices(screen_region, proj, model);
    if (!result) return result;

    // set up transform to keep HUD objects in front of camera
    gGL.matrixMode(LLRender::MM_PROJECTION);
    gGL.loadMatrix(glm::value_ptr(proj));
    set_current_projection(proj);

    gGL.matrixMode(LLRender::MM_MODELVIEW);
    gGL.loadMatrix(glm::value_ptr(model));
    set_current_modelview(model);
    return true;
}

void render_ui(F32 zoom_factor, int subfield)
{
    render_ui_internal(zoom_factor, subfield, true);
}

static void render_ui_internal(F32 zoom_factor, int subfield, bool finalize_scene)
{
    LLPerfStats::RecordSceneTime T ( LLPerfStats::StatType_t::RENDER_UI ); // render time capture - Primary UI stat can have HUD time overlap (TODO)
    LL_PROFILE_ZONE_SCOPED_CATEGORY_UI; //LL_RECORD_BLOCK_TIME(FTM_RENDER_UI);
    LL_PROFILE_GPU_ZONE("ui");
    LLGLState::checkStates();

    glm::mat4 saved_view = get_current_modelview();

    if (!gSnapshot)
    {
        gGL.pushMatrix();
        gGL.loadMatrix(gGLLastModelView);
        set_current_modelview(glm::make_mat4(gGLLastModelView));
    }

    if(LLSceneMonitor::getInstance()->needsUpdate())
    {
        gGL.pushMatrix();
        gViewerWindow->setup2DRender();
        LLSceneMonitor::getInstance()->compare();
        gViewerWindow->setup3DRender();
        gGL.popMatrix();
    }

    if (finalize_scene)
    {
        // apply gamma correction and post effects
        gPipeline.renderFinalize();
    }

//MK
        // Draw a big black sphere around our avatar if the camera render is limited by RLV
        // This call happens only while the avatar is a cloud. This is a crutch while we wait
        // for the real call in lldrawpoolavatar.cpp to be possible.
        if (gRRenabled && (!gAgentAvatarp || !gAgentAvatarp->isFullyLoaded() || !gAgent.mRRInterface.sRenderLimitRenderedThisFrame))
        {
            gAgent.mRRInterface.drawRenderLimit (TRUE); // force opaque because in this degraded case, it is possible to cheat if the outer sphere is not fully opaque because it will be rendered differently (probably the OpenGL engine is not configured for this at this stage)
        }
//mk
    {
        LLGLState::checkStates();

        LL_PROFILE_ZONE_NAMED_CATEGORY_UI("HUD");
        trace_vulkan_ui_stage("render_hud_elements");
        if (!should_skip_vulkan_ui_stage(
                "MARE_VULKAN_DEBUG_SKIP_UI_HUD_ELEMENTS",
                "render_hud_elements"))
        {
            render_hud_elements();
        }
        LLGLState::checkStates();
        trace_vulkan_ui_stage("render_hud_attachments");
        if (!should_skip_vulkan_ui_stage(
                "MARE_VULKAN_DEBUG_SKIP_UI_HUD_ATTACHMENTS",
                "render_hud_attachments"))
        {
            render_hud_attachments();
        }

        LLGLState::checkStates();

        LLGLSDefault gls_default;
        LLGLSUIDefault gls_ui;
        {
            gPipeline.disableLights();
        }

        bool render_ui = gPipeline.hasRenderDebugFeatureMask(LLPipeline::RENDER_DEBUG_FEATURE_UI);
        if (render_ui)
        {
            if (!gDisconnected)
            {
                LL_PROFILE_ZONE_NAMED_CATEGORY_UI("UI 3D"); //LL_RECORD_BLOCK_TIME(FTM_RENDER_UI_3D);
                LLGLState::checkStates();
                trace_vulkan_ui_stage("render_ui_3d");
                if (!should_skip_vulkan_ui_stage(
                        "MARE_VULKAN_DEBUG_SKIP_UI_3D",
                        "render_ui_3d"))
                {
                    render_ui_3d();
                }
                LLGLState::checkStates();
            }
            else
            {
                render_disconnected_background();
            }
        }
        else
        {
            // Make sure particle effects disappear
            LLHUDObject::renderAllForTimer();
        }

        if (render_ui)
        {
            LL_PROFILE_ZONE_NAMED_CATEGORY_UI("UI 2D"); //LL_RECORD_BLOCK_TIME(FTM_RENDER_UI_2D);
            trace_vulkan_ui_stage("LLHUDObject::renderAll");
            if (!should_skip_vulkan_ui_stage(
                    "MARE_VULKAN_DEBUG_SKIP_UI_HUD_OBJECTS",
                    "LLHUDObject::renderAll"))
            {
                LLHUDObject::renderAll();
            }
            trace_vulkan_ui_stage("render_ui_2d");
            if (!should_skip_vulkan_ui_stage(
                    "MARE_VULKAN_DEBUG_SKIP_UI_2D",
                    "render_ui_2d"))
            {
                render_ui_2d();
            }
        }

        trace_vulkan_ui_stage("debug_text");
        if (!should_skip_vulkan_ui_stage(
                "MARE_VULKAN_DEBUG_SKIP_UI_DEBUG_TEXT",
                "debug_text"))
        {
            gViewerWindow->setup2DRender();
            gViewerWindow->updateDebugText();
            gViewerWindow->drawDebugText();
        }
    }

    if (!gSnapshot)
    {
        set_current_modelview(saved_view);
        gGL.popMatrix();
    }
}

void swap()
{
    LLPerfStats::RecordSceneTime T ( LLPerfStats::StatType_t::RENDER_SWAP ); // render time capture - Swap buffer time - can signify excessive data transfer to/from GPU
    LL_PROFILE_ZONE_NAMED_CATEGORY_DISPLAY("Swap");
    LL_PROFILE_GPU_ZONE("swap");
    if (gDisplaySwapBuffers)
    {
        gViewerWindow->getWindow()->swapBuffers();
    }
    gDisplaySwapBuffers = true;
}

void renderCoordinateAxes()
{
    gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);
    gGL.begin(LLRender::LINES);
        gGL.color3f(1.0f, 0.0f, 0.0f);   // i direction = X-Axis = red
        gGL.vertex3f(0.0f, 0.0f, 0.0f);
        gGL.vertex3f(2.0f, 0.0f, 0.0f);
        gGL.vertex3f(3.0f, 0.0f, 0.0f);
        gGL.vertex3f(5.0f, 0.0f, 0.0f);
        gGL.vertex3f(6.0f, 0.0f, 0.0f);
        gGL.vertex3f(8.0f, 0.0f, 0.0f);
        // Make an X
        gGL.vertex3f(11.0f, 1.0f, 1.0f);
        gGL.vertex3f(11.0f, -1.0f, -1.0f);
        gGL.vertex3f(11.0f, 1.0f, -1.0f);
        gGL.vertex3f(11.0f, -1.0f, 1.0f);

        gGL.color3f(0.0f, 1.0f, 0.0f);   // j direction = Y-Axis = green
        gGL.vertex3f(0.0f, 0.0f, 0.0f);
        gGL.vertex3f(0.0f, 2.0f, 0.0f);
        gGL.vertex3f(0.0f, 3.0f, 0.0f);
        gGL.vertex3f(0.0f, 5.0f, 0.0f);
        gGL.vertex3f(0.0f, 6.0f, 0.0f);
        gGL.vertex3f(0.0f, 8.0f, 0.0f);
        // Make a Y
        gGL.vertex3f(1.0f, 11.0f, 1.0f);
        gGL.vertex3f(0.0f, 11.0f, 0.0f);
        gGL.vertex3f(-1.0f, 11.0f, 1.0f);
        gGL.vertex3f(0.0f, 11.0f, 0.0f);
        gGL.vertex3f(0.0f, 11.0f, 0.0f);
        gGL.vertex3f(0.0f, 11.0f, -1.0f);

        gGL.color3f(0.0f, 0.0f, 1.0f);   // Z-Axis = blue
        gGL.vertex3f(0.0f, 0.0f, 0.0f);
        gGL.vertex3f(0.0f, 0.0f, 2.0f);
        gGL.vertex3f(0.0f, 0.0f, 3.0f);
        gGL.vertex3f(0.0f, 0.0f, 5.0f);
        gGL.vertex3f(0.0f, 0.0f, 6.0f);
        gGL.vertex3f(0.0f, 0.0f, 8.0f);
        // Make a Z
        gGL.vertex3f(-1.0f, 1.0f, 11.0f);
        gGL.vertex3f(1.0f, 1.0f, 11.0f);
        gGL.vertex3f(1.0f, 1.0f, 11.0f);
        gGL.vertex3f(-1.0f, -1.0f, 11.0f);
        gGL.vertex3f(-1.0f, -1.0f, 11.0f);
        gGL.vertex3f(1.0f, -1.0f, 11.0f);
    gGL.end();
}

void draw_axes()
{
    LLGLSUIDefault gls_ui;
    gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);
    // A vertical white line at origin
    LLVector3 v = gAgent.getPositionAgent();
    gGL.begin(LLRender::LINES);
        gGL.color3f(1.0f, 1.0f, 1.0f);
        gGL.vertex3f(0.0f, 0.0f, 0.0f);
        gGL.vertex3f(0.0f, 0.0f, 40.0f);
    gGL.end();
    // Some coordinate axes
    gGL.pushMatrix();
        gGL.translatef( v.mV[VX], v.mV[VY], v.mV[VZ] );
        renderCoordinateAxes();
    gGL.popMatrix();
}

void render_ui_3d()
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_UI;
    LLGLSPipeline gls_pipeline;

    //////////////////////////////////////
    //
    // Render 3D UI elements
    // NOTE: zbuffer is cleared before we get here by LLDrawPoolHUD,
    //       so 3d elements requiring Z buffer are moved to LLDrawPoolHUD
    //

    /////////////////////////////////////////////////////////////
    //
    // Render 2.5D elements (2D elements in the world)
    // Stuff without z writes
    //

    // Debugging stuff goes before the UI.

    stop_glerror();

    gUIProgram.bind();
    gGL.color4f(1.f, 1.f, 1.f, 1.f);

    // Coordinate axes
    static LLCachedControl<bool> show_axes(gSavedSettings, "ShowAxes");
    if (show_axes())
    {
        draw_axes();
    }

    gViewerWindow->renderSelections(false, false, true); // Non HUD call in render_hud_elements

    if (gPipeline.hasRenderDebugFeatureMask(LLPipeline::RENDER_DEBUG_FEATURE_UI))
    {
        // Render debugging beacons.
        gObjectList.renderObjectBeacons();
        gObjectList.resetObjectBeacons();
        gSky.addSunMoonBeacons();
    }
    else
    {
        // Make sure particle effects disappear
        LLHUDObject::renderAllForTimer();
    }

    stop_glerror();
}

void render_ui_2d()
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_UI;
    LLGLSUIDefault gls_ui;

    /////////////////////////////////////////////////////////////
    //
    // Render 2D UI elements that overlay the world (no z compare)

    //  Disable wireframe mode below here, as this is HUD/menus
    getRenderBackend().setPolygonMode(
        LLRenderPolygonFace::FrontAndBack,
        LLRenderPolygonMode::Fill);

    //  Menu overlays, HUD, etc
    gViewerWindow->setup2DRender();

    F32 zoom_factor = LLViewerCamera::getInstance()->getZoomFactor();
    S16 sub_region = LLViewerCamera::getInstance()->getZoomSubRegion();

    if (zoom_factor > 1.f)
    {
        //decompose subregion number to x and y values
        int pos_y = sub_region / llceil(zoom_factor);
        int pos_x = sub_region - (pos_y*llceil(zoom_factor));
        // offset for this tile
        LLFontGL::sCurOrigin.mX -= ll_round((F32)gViewerWindow->getWindowWidthScaled() * (F32)pos_x / zoom_factor);
        LLFontGL::sCurOrigin.mY -= ll_round((F32)gViewerWindow->getWindowHeightScaled() * (F32)pos_y / zoom_factor);
    }

    stop_glerror();

    // render outline for HUD
    trace_vulkan_ui_stage("render_ui_2d HUD outline");
    if (isAgentAvatarValid() &&
        gAgentCamera.mHUDCurZoom < 0.98f &&
        !should_skip_vulkan_ui_stage(
            "MARE_VULKAN_DEBUG_SKIP_UI_HUD_OUTLINE",
            "render_ui_2d HUD outline"))
    {
        gUIProgram.bind();
        gGL.pushMatrix();
        S32 half_width = (gViewerWindow->getWorldViewWidthScaled() / 2);
        S32 half_height = (gViewerWindow->getWorldViewHeightScaled() / 2);
        gGL.scalef(LLUI::getScaleFactor().mV[VX], LLUI::getScaleFactor().mV[VY], 1.f);
        gGL.translatef((F32)half_width, (F32)half_height, 0.f);
        F32 zoom = gAgentCamera.mHUDCurZoom;
        gGL.scalef(zoom,zoom,1.f);
        gGL.color4fv(LLColor4::white.mV);
        gl_rect_2d(-half_width, half_height, half_width, -half_height, false);
        gGL.popMatrix();
        gUIProgram.unbind();
        stop_glerror();
    }

    gUIProgram.bind();
    gGL.color4f(1.f, 1.f, 1.f, 1.f);

    const bool vulkan_world_path = use_vulkan_world_path();
    const bool render_ui_buffer =
        LLPipeline::RenderUIBuffer &&
        !vulkan_world_path;
    if (LLPipeline::RenderUIBuffer && vulkan_world_path)
    {
        LL_WARNS_ONCE("RenderBackend")
            << "Vulkan skipped RenderUIBuffer; compositing the UI offscreen with opaque copy can cover the world with the UI buffer clear color."
            << LL_ENDL;
    }

    if (render_ui_buffer)
    {
        if (LLView::sIsRectDirty)
        {
            LLView::sIsRectDirty = false;
            LLRect t_rect;

            gPipeline.mUIScreen.bindTarget();
            gGL.setColorMask(true, true);
            {
                constexpr S32 pad = 8;

                LLView::sDirtyRect.mLeft -= pad;
                LLView::sDirtyRect.mRight += pad;
                LLView::sDirtyRect.mBottom -= pad;
                LLView::sDirtyRect.mTop += pad;

                LLGLEnable scissor(LLRenderCapability::ScissorTest);
                static LLRect last_rect = LLView::sDirtyRect;

                //union with last rect to avoid mouse poop
                last_rect.unionWith(LLView::sDirtyRect);

                t_rect = LLView::sDirtyRect;
                LLView::sDirtyRect = last_rect;
                last_rect = t_rect;

                last_rect.mLeft = LLRect::tCoordType(last_rect.mLeft / LLUI::getScaleFactor().mV[0]);
                last_rect.mRight = LLRect::tCoordType(last_rect.mRight / LLUI::getScaleFactor().mV[0]);
                last_rect.mTop = LLRect::tCoordType(last_rect.mTop / LLUI::getScaleFactor().mV[1]);
                last_rect.mBottom = LLRect::tCoordType(last_rect.mBottom / LLUI::getScaleFactor().mV[1]);

                LLRect clip_rect(last_rect);

                getRenderBackend().clear(LL_RENDER_CLEAR_COLOR);

                gUIProgram.bind();
                gViewerWindow->draw();
            }

            gPipeline.mUIScreen.flush();
            gGL.setColorMask(true, false);

            LLView::sDirtyRect = t_rect;
        }

        LLGLDisable cull(LLRenderCapability::CullFace);
        LLGLDisable blend(LLRenderCapability::Blend);
        S32 width = gViewerWindow->getWindowWidthScaled();
        S32 height = gViewerWindow->getWindowHeightScaled();
        gGL.getTexUnit(0)->bind(&gPipeline.mUIScreen);
        gGL.begin(LLRender::TRIANGLE_STRIP);
        gGL.color4f(1.f,1.f,1.f,1.f);
        gGL.texCoord2f(0.f, 0.f);                 gGL.vertex2i(0, 0);
        gGL.texCoord2f((F32)width, 0.f);          gGL.vertex2i(width, 0);
        gGL.texCoord2f(0.f, (F32)height);         gGL.vertex2i(0, height);
        gGL.texCoord2f((F32)width, (F32)height);  gGL.vertex2i(width, height);
        gGL.end();
    }
    else
    {
        trace_vulkan_ui_stage("gViewerWindow->draw");
        if (!should_skip_vulkan_ui_stage(
                "MARE_VULKAN_DEBUG_SKIP_UI_VIEWER_WINDOW_DRAW",
                "gViewerWindow->draw"))
        {
            gUIProgram.bind();
            gViewerWindow->draw();
        }
    }

    // reset current origin for font rendering, in case of tiling render
    LLFontGL::sCurOrigin.set(0, 0);
}

void render_disconnected_background()
{
    gUIProgram.bind();

    gGL.color4f(1.f, 1.f, 1.f, 1.f);
    if (!gDisconnectedImagep && gDisconnected)
    {
        LL_INFOS() << "Loading last bitmap..." << LL_ENDL;

        std::string temp_str;
        temp_str = gDirUtilp->getLindenUserDir() + gDirUtilp->getDirDelimiter() + LLStartUp::getScreenLastFilename();

        LLPointer<LLImagePNG> image_png = new LLImagePNG;
        if( !image_png->load(temp_str) )
        {
            //LL_INFOS() << "Bitmap load failed" << LL_ENDL;
            return;
        }

        LLPointer<LLImageRaw> raw = new LLImageRaw;
        if (!image_png->decode(raw, 0.0f))
        {
            LL_INFOS() << "Bitmap decode failed" << LL_ENDL;
            gDisconnectedImagep = NULL;
            return;
        }

        U8 *rawp = raw->getData();
        S32 npixels = (S32)image_png->getWidth()*(S32)image_png->getHeight();
        for (S32 i = 0; i < npixels; i++)
        {
            S32 sum = 0;
            sum = *rawp + *(rawp+1) + *(rawp+2);
            sum /= 3;
            *rawp = ((S32)sum*6 + *rawp)/7;
            rawp++;
            *rawp = ((S32)sum*6 + *rawp)/7;
            rawp++;
            *rawp = ((S32)sum*6 + *rawp)/7;
            rawp++;
        }

        raw->expandToPowerOfTwo();
        gDisconnectedImagep = LLViewerTextureManager::getLocalTexture(raw.get(), false);
        gStartTexture = gDisconnectedImagep;
        gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);
    }

    // Make sure the progress view always fills the entire window.
    S32 width = gViewerWindow->getWindowWidthScaled();
    S32 height = gViewerWindow->getWindowHeightScaled();

    if (gDisconnectedImagep)
    {
        LLGLSUIDefault gls_ui;
        gViewerWindow->setup2DRender();
        gGL.pushMatrix();
        {
            // scale ui to reflect UIScaleFactor
            // this can't be done in setup2DRender because it requires a
            // pushMatrix/popMatrix pair
            const LLVector2& display_scale = gViewerWindow->getDisplayScale();
            gGL.scalef(display_scale.mV[VX], display_scale.mV[VY], 1.f);

            gGL.getTexUnit(0)->bind(gDisconnectedImagep);
            gGL.color4f(1.f, 1.f, 1.f, 1.f);
            gl_rect_2d_simple_tex(width, height);
            gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);
        }
        gGL.popMatrix();
    }
    gGL.flush();

    gUIProgram.unbind();
}

void display_cleanup()
{
    gDisconnectedImagep = nullptr;
}
