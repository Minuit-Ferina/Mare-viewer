#ifndef LL_LLVULKANCOMPOSITEPARAMS_H
#define LL_LLVULKANCOMPOSITEPARAMS_H

#include "linden_common.h"
#include "llrenderbackendtypes.h"

struct LLVulkanFinalCompositeSettings
{
    bool mNoPost = true;
    F32 mExposure = 1.f;
    F32 mGamma = 2.2f;
    bool mLegacyGamma = false;
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

struct LLVulkanDeferredCompositeSettings
{
    F32 mAmbientRed = 0.28f;
    F32 mAmbientGreen = 0.28f;
    F32 mAmbientBlue = 0.28f;
    F32 mDirectLightRed = 0.85f;
    F32 mDirectLightGreen = 0.85f;
    F32 mDirectLightBlue = 0.85f;
    F32 mLightDirectionX = 0.f;
    F32 mLightDirectionY = 0.f;
    F32 mLightDirectionZ = 1.f;
    F32 mDirectLightScale = 1.f;
    U32 mDeferredAttachmentCount = 0;
    bool mSSAOEnabled = false;
    F32 mSSAOScale = 0.f;
    F32 mSSAOMaxScale = 0.f;
    F32 mSSAOFactor = 0.f;
    F32 mSSAOEffect = 0.f;
    F32 mDominantLightScreenX = -1.f;
    F32 mDominantLightScreenY = -1.f;
    F32 mDominantLightRadius = 0.f;
    F32 mLocalLightRed = 0.f;
    F32 mLocalLightGreen = 0.f;
    F32 mLocalLightBlue = 0.f;
    F32 mLocalLightStrength = 0.f;
    F32 mReflectionProbeAmbiance = 0.f;
    F32 mTonemapMix = 0.f;
    F32 mSkyLightingValid = 0.f;
    F32 mInverseProjection[16] =
    {
        1.f, 0.f, 0.f, 0.f,
        0.f, 1.f, 0.f, 0.f,
        0.f, 0.f, 1.f, 0.f,
        0.f, 0.f, 0.f, 1.f,
    };
};

inline LLRenderWorldMaterialParameters make_vulkan_deferred_composite_material_parameters(
    const LLVulkanDeferredCompositeSettings& settings)
{
    LLRenderWorldMaterialParameters parameters;

    // Keep these assignments aligned with LLVulkanWorldPushConstants and
    // vulkan/final/class3/deferred/deferred_composite_runtime.frag.
    parameters.mBaseColorRed = llclamp(settings.mAmbientRed, 0.f, 2.f);
    parameters.mBaseColorGreen = llclamp(settings.mAmbientGreen, 0.f, 2.f);
    parameters.mBaseColorBlue = llclamp(settings.mAmbientBlue, 0.f, 2.f);
    parameters.mBaseColorAlpha = settings.mDominantLightScreenY;
    parameters.mEmissiveColorRed = llclamp(settings.mDirectLightRed, 0.f, 2.f);
    parameters.mEmissiveColorGreen = llclamp(settings.mDirectLightGreen, 0.f, 2.f);
    parameters.mEmissiveColorBlue = llclamp(settings.mDirectLightBlue, 0.f, 2.f);
    parameters.mHasEmissiveMap = settings.mDominantLightRadius;
    parameters.mSpecularColorRed = settings.mLightDirectionX;
    parameters.mSpecularColorGreen = settings.mLightDirectionY;
    parameters.mSpecularColorBlue = settings.mLightDirectionZ;
    parameters.mEnvIntensity = settings.mDirectLightScale;
    parameters.mRoughnessFactor =
        static_cast<F32>(settings.mDeferredAttachmentCount);
    parameters.mMetallicFactor = settings.mSSAOEnabled ? 1.f : 0.f;
    parameters.mNormalTextureOffsetS =
        settings.mSSAOEnabled ? llclamp(settings.mSSAOScale, 0.f, 32.f) : 0.f;
    parameters.mNormalTextureOffsetT =
        settings.mSSAOEnabled ? llclamp(settings.mSSAOMaxScale, 0.f, 32.f) : 0.f;
    parameters.mORMTextureScaleS =
        settings.mSSAOEnabled ? llclamp(settings.mSSAOFactor, 0.1f, 8.f) : 0.f;
    parameters.mORMTextureScaleT =
        settings.mSSAOEnabled ? llclamp(settings.mSSAOEffect, 0.f, 2.f) : 0.f;
    parameters.mMaterialFlags = settings.mDominantLightScreenX;
    parameters.mDiffuseAlphaMode = settings.mLocalLightRed;
    parameters.mGLTFAlphaMode = settings.mLocalLightGreen;
    parameters.mBump = settings.mLocalLightBlue;
    parameters.mShiny = settings.mLocalLightStrength;
    parameters.mSceneAmbientRed = settings.mReflectionProbeAmbiance;
    parameters.mSceneAmbientGreen = settings.mTonemapMix;
    parameters.mSceneAmbientBlue = settings.mDirectLightScale;
    parameters.mSceneDirectScale = settings.mSkyLightingValid;
    for (U32 i = 0; i < 16; ++i)
    {
        parameters.mCompositeInverseProjection[i] =
            settings.mInverseProjection[i];
    }

    return parameters;
}

inline LLRenderWorldMaterialParameters make_vulkan_final_composite_material_parameters(
    const LLVulkanFinalCompositeSettings& settings)
{
    LLRenderWorldMaterialParameters parameters;

    const F32 gamma = llclamp(settings.mGamma, 0.1f, 8.f);
    parameters.mBaseColorRed =
        settings.mNoPost ? 1.f : llclamp(settings.mExposure, 0.5f, 4.f);
    parameters.mBaseColorGreen = gamma;
    parameters.mBaseColorBlue = settings.mLegacyGamma ? 2.f : 1.f;
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
