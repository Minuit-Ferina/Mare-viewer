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
    // True only when environment/probe descriptors are bound for DeferredSoften.
    bool mReflectionInputsValid = false;
    F32 mMaxProbeLOD = 6.f;
    F32 mTonemapMix = 0.f;
    F32 mSkyLightingValid = 0.f;
    F32 mScreenWidth = 1.f;
    F32 mScreenHeight = 1.f;
    F32 mClipPlaneX = 0.f;
    F32 mClipPlaneY = 0.f;
    F32 mClipPlaneZ = 0.f;
    F32 mClipPlaneW = 0.f;
    F32 mSunDirectionX = 0.35f;
    F32 mSunDirectionY = 0.45f;
    F32 mSunDirectionZ = 0.82f;
    F32 mSunUpFactor = 1.f;
    F32 mMoonDirectionX = -0.25f;
    F32 mMoonDirectionY = -0.15f;
    F32 mMoonDirectionZ = 0.95f;
    F32 mClassicMode = 0.f;
    F32 mCubeSnapshot = 0.f;
    F32 mSkyHDRScale = 1.f;
    F32 mBlurSize = 1.4f;
    F32 mBlurFidelity = 4.f;
    F32 mSSAOIrradianceScale = 0.6f;
    F32 mSSAOIrradianceMax = 0.18f;
    F32 mWaterPlaneX = 0.f;
    F32 mWaterPlaneY = 0.f;
    F32 mWaterPlaneZ = 1.f;
    F32 mWaterPlaneW = 1.f;
    F32 mEnvironmentMatrix[9] =
    {
        1.f, 0.f, 0.f,
        0.f, 1.f, 0.f,
        0.f, 0.f, 1.f,
    };
    F32 mSSAOEffectMatrix[9] =
    {
        1.f, 0.f, 0.f,
        0.f, 1.f, 0.f,
        0.f, 0.f, 1.f,
    };
    F32 mSSRParameters0[4] =
    {
        0.f, 16.f, 0.1f, 0.2f,
    };
    F32 mSSRParameters1[4] =
    {
        0.2f, 1.f, 1.25f, 0.f,
    };
    F32 mInverseProjection[16] =
    {
        1.f, 0.f, 0.f, 0.f,
        0.f, 1.f, 0.f, 0.f,
        0.f, 0.f, 1.f, 0.f,
        0.f, 0.f, 0.f, 1.f,
    };
    F32 mInverseModelviewDelta[16] =
    {
        1.f, 0.f, 0.f, 0.f,
        0.f, 1.f, 0.f, 0.f,
        0.f, 0.f, 1.f, 0.f,
        0.f, 0.f, 0.f, 1.f,
    };
    F32 mShadowMatrix[16 * 6] =
    {
        1.f, 0.f, 0.f, 0.f,
        0.f, 1.f, 0.f, 0.f,
        0.f, 0.f, 1.f, 0.f,
        0.f, 0.f, 0.f, 1.f,
        1.f, 0.f, 0.f, 0.f,
        0.f, 1.f, 0.f, 0.f,
        0.f, 0.f, 1.f, 0.f,
        0.f, 0.f, 0.f, 1.f,
        1.f, 0.f, 0.f, 0.f,
        0.f, 1.f, 0.f, 0.f,
        0.f, 0.f, 1.f, 0.f,
        0.f, 0.f, 0.f, 1.f,
        1.f, 0.f, 0.f, 0.f,
        0.f, 1.f, 0.f, 0.f,
        0.f, 0.f, 1.f, 0.f,
        0.f, 0.f, 0.f, 1.f,
        1.f, 0.f, 0.f, 0.f,
        0.f, 1.f, 0.f, 0.f,
        0.f, 0.f, 1.f, 0.f,
        0.f, 0.f, 0.f, 1.f,
        1.f, 0.f, 0.f, 0.f,
        0.f, 1.f, 0.f, 0.f,
        0.f, 0.f, 1.f, 0.f,
        0.f, 0.f, 0.f, 1.f,
    };
    F32 mShadowClip[4] =
    {
        1.f, 64.f, 128.f, 256.f,
    };
    F32 mShadowSettings[4] =
    {
        0.f, 0.f, 0.f, 0.f,
    };
    F32 mShadowResolution[4] =
    {
        1.f, 1.f, 1.f, 1.f,
    };
    F32 mShadowRuntime[4] =
    {
        0.f, 0.f, 0.f, 0.f,
    };
    F32 mBlurSettings[4] =
    {
        1.f, 0.f, 1.f, 1.5f,
    };
    F32 mBlurScreen[4] =
    {
        1.f, 1.f, 1.4f, 4.f,
    };
    F32 mBlurKernel[4 * 4] =
    {
        1.f, 1.f, 0.f, 0.f,
        0.f, 0.f, 1.f, 0.f,
        0.f, 0.f, 2.f, 0.f,
        0.f, 0.f, 3.f, 0.f,
    };
    F32 mAtmosBlueHorizonHaze[4] =
    {
        0.4954f, 0.4954f, 0.6399f, 0.19f,
    };
    F32 mAtmosBlueDensityHaze[4] =
    {
        0.2447f, 0.4487f, 0.7599f, 0.7f,
    };
    F32 mAtmosDensity[4] =
    {
        0.f, 0.0001f, 0.8f, 1605.f,
    };
    F32 mAtmosGlow[4] =
    {
        18.f, 0.f, -0.01f, 1.f,
    };
    F32 mAtmosSunlight[4] =
    {
        1.f, 1.f, 1.f, 1.5f,
    };
    F32 mAtmosMoonlight[4] =
    {
        1.f, 1.f, 1.f, 1.5f,
    };
    F32 mAtmosAmbient[4] =
    {
        0.25f, 0.25f, 0.25f, 1.f,
    };
    F32 mAtmosLightNorm[4] =
    {
        0.f, 1.f, 0.f, 1.f,
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
    parameters.mSceneAmbientGreen = settings.mReflectionInputsValid ? 1.f : 0.f;
    parameters.mSceneAmbientBlue = llmax(settings.mMaxProbeLOD, 0.f);
    parameters.mSceneDirectScale = settings.mSkyLightingValid;
    parameters.mSceneDirectRed = llmax(settings.mScreenWidth, 1.f);
    parameters.mSceneDirectGreen = llmax(settings.mScreenHeight, 1.f);
    parameters.mSceneDirectBlue = settings.mSSAOIrradianceScale;
    parameters.mSceneLightingValid = settings.mSSAOIrradianceMax;
    parameters.mSceneLightDirectionX = settings.mWaterPlaneX;
    parameters.mSceneLightDirectionY = settings.mWaterPlaneY;
    parameters.mSceneLightDirectionZ = settings.mWaterPlaneZ;
    parameters.mSceneLightDirectionValid = settings.mWaterPlaneW;
    parameters.mCompositeClipPlane[0] = settings.mClipPlaneX;
    parameters.mCompositeClipPlane[1] = settings.mClipPlaneY;
    parameters.mCompositeClipPlane[2] = settings.mClipPlaneZ;
    parameters.mCompositeClipPlane[3] = settings.mClipPlaneW;
    parameters.mCompositeSunDirection[0] = settings.mSunDirectionX;
    parameters.mCompositeSunDirection[1] = settings.mSunDirectionY;
    parameters.mCompositeSunDirection[2] = settings.mSunDirectionZ;
    parameters.mCompositeSunDirection[3] = settings.mSunUpFactor;
    parameters.mCompositeMoonDirection[0] = settings.mMoonDirectionX;
    parameters.mCompositeMoonDirection[1] = settings.mMoonDirectionY;
    parameters.mCompositeMoonDirection[2] = settings.mMoonDirectionZ;
    parameters.mCompositeMoonDirection[3] = settings.mClassicMode;
    parameters.mCompositeSkySettings[0] = settings.mCubeSnapshot;
    parameters.mCompositeSkySettings[1] = settings.mSkyHDRScale;
    parameters.mCompositeSkySettings[2] = settings.mBlurSize;
    parameters.mCompositeSkySettings[3] = settings.mBlurFidelity;
    for (U32 i = 0; i < 9; ++i)
    {
        parameters.mCompositeEnvironmentMatrix[i] =
            settings.mEnvironmentMatrix[i];
        parameters.mCompositeSSAOEffectMatrix[i] =
            settings.mSSAOEffectMatrix[i];
    }
    for (U32 i = 0; i < 4; ++i)
    {
        parameters.mCompositeSSR0[i] =
            settings.mSSRParameters0[i];
        parameters.mCompositeSSR1[i] =
            settings.mSSRParameters1[i];
    }
    for (U32 i = 0; i < 16; ++i)
    {
        parameters.mCompositeInverseProjection[i] =
            settings.mInverseProjection[i];
        parameters.mCompositeInverseModelviewDelta[i] =
            settings.mInverseModelviewDelta[i];
    }
    for (U32 i = 0; i < 16 * 6; ++i)
    {
        parameters.mCompositeShadowMatrix[i] =
            settings.mShadowMatrix[i];
    }
    for (U32 i = 0; i < 4; ++i)
    {
        parameters.mCompositeShadowClip[i] =
            settings.mShadowClip[i];
        parameters.mCompositeShadowSettings[i] =
            settings.mShadowSettings[i];
        parameters.mCompositeShadowResolution[i] =
            settings.mShadowResolution[i];
        parameters.mCompositeShadowRuntime[i] =
            settings.mShadowRuntime[i];
        parameters.mCompositeBlurSettings[i] =
            settings.mBlurSettings[i];
        parameters.mCompositeBlurScreen[i] =
            settings.mBlurScreen[i];
        parameters.mCompositeAtmosBlueHorizonHaze[i] =
            settings.mAtmosBlueHorizonHaze[i];
        parameters.mCompositeAtmosBlueDensityHaze[i] =
            settings.mAtmosBlueDensityHaze[i];
        parameters.mCompositeAtmosDensity[i] =
            settings.mAtmosDensity[i];
        parameters.mCompositeAtmosGlow[i] =
            settings.mAtmosGlow[i];
        parameters.mCompositeAtmosSunlight[i] =
            settings.mAtmosSunlight[i];
        parameters.mCompositeAtmosMoonlight[i] =
            settings.mAtmosMoonlight[i];
        parameters.mCompositeAtmosAmbient[i] =
            settings.mAtmosAmbient[i];
        parameters.mCompositeAtmosLightNorm[i] =
            settings.mAtmosLightNorm[i];
    }
    for (U32 i = 0; i < 4 * 4; ++i)
    {
        parameters.mCompositeBlurKernel[i] =
            settings.mBlurKernel[i];
    }

    return parameters;
}

inline void set_vulkan_deferred_reflection_inputs_valid(
    LLRenderWorldMaterialParameters& parameters,
    bool valid)
{
    parameters.mSceneAmbientGreen = valid ? 1.f : 0.f;
}

inline LLRenderWorldMaterialParameters make_vulkan_final_composite_material_parameters(
    const LLVulkanFinalCompositeSettings& settings)
{
    LLRenderWorldMaterialParameters parameters;

    const F32 gamma = llclamp(settings.mGamma, 0.1f, 8.f);
    parameters.mBaseColorRed = llclamp(settings.mExposure, 0.5f, 4.f);
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
