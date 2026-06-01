#ifndef LL_LLVULKANCOMPOSITEPARAMS_H
#define LL_LLVULKANCOMPOSITEPARAMS_H

#include "linden_common.h"
#include "llrenderbackendtypes.h"

struct LLVulkanFinalCompositeSettings
{
    bool mNoPost = true;
    F32 mExposure = 1.f;
    F32 mGamma = 2.2f;
    F32 mTonemapMix = 0.f;
    U32 mTonemapType = 0;
    F32 mCASSharpness = 0.4f;
    bool mRenderGlow = false;
    F32 mGlowWarmthAmount = 0.f;
    F32 mFSAAType = 0.f;
    F32 mRenderBufferVisualization = -1.f;
    U32 mDeferredAttachmentCount = 0;
    bool mDepthOfFieldEnabled = false;
    F32 mCameraMaxCoF = 0.f;
    F32 mGlowStrength = 0.f;
    F32 mGlowMaxExtractAlpha = 0.f;
    F32 mGlowWidth = 0.f;
    F32 mGlowIterations = 0.f;
};

inline LLRenderWorldMaterialParameters make_vulkan_final_composite_material_parameters(
    const LLVulkanFinalCompositeSettings& settings)
{
    LLRenderWorldMaterialParameters parameters;

    const F32 gamma = llclamp(settings.mGamma, 0.1f, 8.f);
    parameters.mBaseColorRed =
        settings.mNoPost ? 1.f : llclamp(settings.mExposure, 0.5f, 4.f);
    parameters.mBaseColorGreen = settings.mNoPost ? 1.f : 1.f / gamma;
    parameters.mBaseColorBlue = settings.mNoPost ? 0.f : 1.f;
    parameters.mBaseColorAlpha =
        settings.mNoPost ? 0.f : llclamp(settings.mCASSharpness, 0.f, 1.f);
    parameters.mRoughnessFactor = llclamp(settings.mTonemapMix, 0.f, 1.f);
    parameters.mMetallicFactor =
        settings.mNoPost ? 0.f : static_cast<F32>(settings.mTonemapType);
    parameters.mMaterialFlags =
        settings.mNoPost || !settings.mRenderGlow ?
            0.f :
            llclamp(settings.mGlowWarmthAmount, 0.f, 1.f);
    parameters.mSpecularColorRed = settings.mNoPost ? 0.f : settings.mFSAAType;
    parameters.mSpecularColorGreen =
        settings.mRenderBufferVisualization >= 0.f &&
            settings.mRenderBufferVisualization <= 6.f ?
                settings.mRenderBufferVisualization :
                -1.f;
    parameters.mSpecularColorBlue =
        static_cast<F32>(settings.mDeferredAttachmentCount);
    parameters.mEnvIntensity =
        settings.mDepthOfFieldEnabled ?
            llclamp(settings.mCameraMaxCoF, 0.f, 10.f) :
            0.f;
    parameters.mDiffuseAlphaMode =
        settings.mNoPost || !settings.mRenderGlow ?
            0.f :
            llclamp(settings.mGlowStrength, 0.f, 2.f);
    parameters.mGLTFAlphaMode =
        settings.mNoPost || !settings.mRenderGlow ?
            0.f :
            llclamp(settings.mGlowMaxExtractAlpha, 0.f, 1.f);
    parameters.mBump =
        settings.mNoPost || !settings.mRenderGlow ?
            0.f :
            llclamp(settings.mGlowWidth, 0.f, 8.f);
    parameters.mShiny =
        settings.mNoPost || !settings.mRenderGlow ?
            0.f :
            llclamp(settings.mGlowIterations, 0.f, 8.f);

    return parameters;
}

#endif
