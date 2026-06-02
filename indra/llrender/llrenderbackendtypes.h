/**
 * @file llrenderbackendtypes.h
 * @brief Backend-neutral render type declarations.
 *
 * $LicenseInfo:firstyear=2026&license=viewerlgpl$
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
 * $/LicenseInfo$
 */

#ifndef LL_LLRENDERBACKENDTYPES_H
#define LL_LLRENDERBACKENDTYPES_H

#include "stdtypes.h"

using LLRenderDebugMessageCallback = void (*)();

enum class LLRenderBackendType : U8
{
    Unknown,
    OpenGL,
    Vulkan,
    Metal,
    Null,
};

enum class LLRenderLoadAction : U8
{
    Load,
    Clear,
    DontCare,
};

enum class LLRenderStoreAction : U8
{
    Store,
    DontCare,
};

enum class LLRenderBlendFactor : U8
{
    One,
    Zero,
    DestinationColor,
    SourceColor,
    OneMinusDestinationColor,
    OneMinusSourceColor,
    DestinationAlpha,
    SourceAlpha,
    OneMinusDestinationAlpha,
    OneMinusSourceAlpha,
};

enum class LLRenderCapability : U8
{
    AlphaTest,
    Blend,
    ClipPlane0,
    CullFace,
    DebugOutputSynchronous,
    DepthTest,
    DepthClamp,
    LineSmooth,
    Multisample,
    PolygonOffsetFill,
    PolygonOffsetLine,
    ScissorTest,
    StencilTest,
    TextureGenS,
    TextureGenT,
    TextureCubeMapSeamless,
};

enum class LLRenderCullFace : U8
{
    Front,
    Back,
    FrontAndBack,
};

enum class LLRenderDepthFunction : U8
{
    Always,
    Less,
    LessEqual,
    Equal,
    NotEqual,
    GreaterEqual,
    Greater,
};

enum class LLRenderTextureTarget : U8
{
    Texture2D,
    TextureRectangle,
    TextureCubeMap,
    TextureCubeMapPositiveX,
    TextureCubeMapNegativeX,
    TextureCubeMapPositiveY,
    TextureCubeMapNegativeY,
    TextureCubeMapPositiveZ,
    TextureCubeMapNegativeZ,
    TextureCubeMapArray,
    Texture2DMultisample,
    Texture3D,
};

enum class LLRenderTextureAddressMode : U8
{
    Repeat,
    MirroredRepeat,
    ClampToEdge,
};

enum class LLRenderTextureFilter : U8
{
    Nearest,
    Linear,
    LinearMipmapLinear,
    LinearMipmapNearest,
    NearestMipmapNearest,
};

enum class LLRenderPixelStoreParameter : U8
{
    PackAlignment,
    UnpackAlignment,
    UnpackSwapBytes,
    UnpackRowLength,
};

enum class LLRenderTextureLevelParameter : U8
{
    Width,
    Height,
    Compressed,
    CompressedImageSize,
};

enum class LLRenderTextureParameter : U8
{
    GenerateMipmap,
    BaseLevel,
    MaxLevel,
    SwizzleRGBA,
};

enum class LLRenderBufferTarget : U8
{
    Vertex,
    Index,
    PixelPack,
    PixelUnpack,
    Uniform,
};

enum class LLRenderBufferUsage : U8
{
    StaticDraw,
    DynamicDraw,
    StreamDraw,
    StreamCopy,
};

enum class LLRenderPrimitiveType : U8
{
    Triangles,
    TriangleStrip,
    TriangleFan,
    Points,
    Lines,
    LineStrip,
    LineLoop,
};

enum class LLRenderIndexType : U8
{
    UnsignedShort,
    UnsignedInt,
};

enum class LLRenderVertexAttributeType : U8
{
    Float32,
    UnsignedByte,
    UnsignedShort,
    UnsignedInt,
};

enum class LLRenderFramebufferAttachment : U8
{
    Color0,
    Color1,
    Color2,
    Color3,
    Depth,
};

enum class LLRenderFramebufferBindPoint : U8
{
    Read,
    Draw,
    ReadWrite,
};

enum class LLRenderFramebufferStatus : U8
{
    Complete,
    IncompleteMissingAttachment,
    IncompleteAttachment,
    Unsupported,
    Unknown,
};

enum class LLRenderQueryTarget : U8
{
    AnySamplesPassed,
    TimeElapsed,
    SamplesPassed,
    PrimitivesGenerated,
    TransformFeedbackPrimitivesWritten,
};

enum class LLRenderQueryParameter : U8
{
    ResultAvailable,
    Result,
};

enum class LLRenderShaderStage : U8
{
    Vertex,
    Fragment,
    Geometry,
    Compute,
};

enum class LLRenderShaderParameter : U8
{
    InfoLogLength,
    CompileStatus,
};

enum class LLRenderProgramParameter : U8
{
    InfoLogLength,
    LinkStatus,
    ActiveUniforms,
    BinaryLength,
};

enum class LLRenderProgramSetting : U8
{
    BinaryRetrievableHint,
};

enum class LLRenderTextureFormat : U8
{
    None,
    Alpha,
    Alpha8,
    R8,
    R16F,
    R32F,
    RG8,
    RG16F,
    RG32F,
    RGB,
    RGB8,
    RGB16F,
    RGB10A2,
    R11G11B10F,
    RGBA,
    RGBA8,
    RGBA16,
    RGBA16F,
    DepthComponent,
    DepthComponent24,
    Luminance,
};

enum class LLRenderPixelFormat : U8
{
    Alpha,
    DepthComponent,
    Luminance,
    Red,
    RG,
    RGB,
    RGBA,
};

enum class LLRenderPixelType : U8
{
    UnsignedByte,
    UnsignedShort,
    UnsignedInt,
    Float32,
};

enum class LLRenderImageAccess : U8
{
    WriteOnly,
};

enum class LLRenderPolygonFace : U8
{
    FrontAndBack,
};

enum class LLRenderPolygonMode : U8
{
    Fill,
    Line,
};

enum class LLRenderStencilFunction : U8
{
    Always,
    Equal,
};

enum class LLRenderStencilOperation : U8
{
    Keep,
    Replace,
};

enum class LLRenderWorldShaderClass : U8
{
    Textured,
    Sky,
    Water,
    Haze,
    Alpha,
    Glow,
    AlphaMask,
    Fullbright,
    Material,
    PBR,
    Avatar,
    Terrain,
    Shadow,
    ShadowAlphaMask,
    AvatarShadow,
    AvatarAlphaShadow,
    AvatarAlphaMaskShadow,
    TreeShadow,
    PBRAlphaMaskShadow,
    PBRAlphaBlendShadow,
    PointLight,
    MultiPointLight,
    SpotLight,
    MultiSpotLight,
    Copy,
    DeferredLightMap,
    DeferredSoften,
    DeferredComposite,
    FinalComposite,
};

enum class LLRenderMatrixMode : U8
{
    ModelView,
};

enum class LLRenderTextureCoordinate : U8
{
    S,
    T,
};

enum class LLRenderInfoString : U8
{
    Vendor,
    Renderer,
    Version,
};

enum class LLRenderIntegerParameter : U8
{
    DedicatedVideoMemoryKB,
    FreeVideoMemoryKB,
    RedBits,
    GreenBits,
    BlueBits,
    AlphaBits,
    DepthBits,
    StencilBits,
};

enum LLRenderMemoryBarrierMask : U32
{
    LL_RENDER_MEMORY_BARRIER_NONE = 0,
    LL_RENDER_MEMORY_BARRIER_SHADER_IMAGE_ACCESS = 1 << 0,
    LL_RENDER_MEMORY_BARRIER_TEXTURE_FETCH = 1 << 1,
};

inline LLRenderMemoryBarrierMask operator|(LLRenderMemoryBarrierMask lhs, LLRenderMemoryBarrierMask rhs)
{
    return static_cast<LLRenderMemoryBarrierMask>(static_cast<U32>(lhs) | static_cast<U32>(rhs));
}

inline LLRenderMemoryBarrierMask operator&(LLRenderMemoryBarrierMask lhs, LLRenderMemoryBarrierMask rhs)
{
    return static_cast<LLRenderMemoryBarrierMask>(static_cast<U32>(lhs) & static_cast<U32>(rhs));
}

inline LLRenderMemoryBarrierMask& operator|=(LLRenderMemoryBarrierMask& lhs, LLRenderMemoryBarrierMask rhs)
{
    lhs = lhs | rhs;
    return lhs;
}

enum LLRenderClearMask : U32
{
    LL_RENDER_CLEAR_NONE = 0,
    LL_RENDER_CLEAR_COLOR = 1 << 0,
    LL_RENDER_CLEAR_DEPTH = 1 << 1,
    LL_RENDER_CLEAR_STENCIL = 1 << 2,
    LL_RENDER_CLEAR_ALL = LL_RENDER_CLEAR_COLOR | LL_RENDER_CLEAR_DEPTH | LL_RENDER_CLEAR_STENCIL,
};

inline LLRenderClearMask operator|(LLRenderClearMask lhs, LLRenderClearMask rhs)
{
    return static_cast<LLRenderClearMask>(static_cast<U32>(lhs) | static_cast<U32>(rhs));
}

inline LLRenderClearMask operator&(LLRenderClearMask lhs, LLRenderClearMask rhs)
{
    return static_cast<LLRenderClearMask>(static_cast<U32>(lhs) & static_cast<U32>(rhs));
}

inline LLRenderClearMask& operator|=(LLRenderClearMask& lhs, LLRenderClearMask rhs)
{
    lhs = lhs | rhs;
    return lhs;
}

struct LLRenderExtent2D
{
    S32 mWidth = 0;
    S32 mHeight = 0;
};

struct LLRenderFloatRange
{
    F32 mMinimum = 0.f;
    F32 mMaximum = 0.f;
};

struct LLRenderViewport
{
    F32 mX = 0.f;
    F32 mY = 0.f;
    F32 mWidth = 0.f;
    F32 mHeight = 0.f;
    F32 mMinDepth = 0.f;
    F32 mMaxDepth = 1.f;
};

struct LLRenderScissor
{
    S32 mX = 0;
    S32 mY = 0;
    S32 mWidth = 0;
    S32 mHeight = 0;
    bool mEnabled = false;
};

struct LLRenderClearColor
{
    F32 mRed = 0.f;
    F32 mGreen = 0.f;
    F32 mBlue = 0.f;
    F32 mAlpha = 0.f;
};

struct LLRenderColorMask
{
    bool mRed = true;
    bool mGreen = true;
    bool mBlue = true;
    bool mAlpha = true;
};

struct LLRenderBlendState
{
    LLRenderBlendFactor mColorSource = LLRenderBlendFactor::One;
    LLRenderBlendFactor mColorDestination = LLRenderBlendFactor::Zero;
    LLRenderBlendFactor mAlphaSource = LLRenderBlendFactor::One;
    LLRenderBlendFactor mAlphaDestination = LLRenderBlendFactor::Zero;
};

struct LLRenderWorldTerrainParameters
{
    F32 mDetailScale = 1.f;
    F32 mOffsetX = 0.f;
    F32 mOffsetY = 0.f;
    F32 mRegionScale = 256.f;
    F32 mPaintType = 0.f;
    F32 mPlanarSampleCount = 1.f;
    F32 mTriplanarBlendFactor = 8.f;
    F32 mUsesPBRMaterials = 0.f;
    F32 mBaseColorFactors[16] =
    {
        1.f, 1.f, 1.f, 1.f,
        1.f, 1.f, 1.f, 1.f,
        1.f, 1.f, 1.f, 1.f,
        1.f, 1.f, 1.f, 1.f,
    };
    F32 mMetallicFactors[4] =
    {
        0.f, 0.f, 0.f, 0.f,
    };
    F32 mRoughnessFactors[4] =
    {
        1.f, 1.f, 1.f, 1.f,
    };
    F32 mEmissiveMinimumAlpha[16] =
    {
        0.f, 0.f, 0.f, 0.f,
        0.f, 0.f, 0.f, 0.f,
        0.f, 0.f, 0.f, 0.f,
        0.f, 0.f, 0.f, 0.f,
    };
    F32 mTextureTransforms[20] =
    {
        1.f, 1.f, 0.f, 0.f, 0.f,
        1.f, 1.f, 0.f, 0.f, 0.f,
        1.f, 1.f, 0.f, 0.f, 0.f,
        1.f, 1.f, 0.f, 0.f, 0.f,
    };
};

struct LLRenderWorldMaterialParameters
{
    enum : U32
    {
        MaxDeferredMultiLightCount = 8,
    };

    enum MaterialFlag : U32
    {
        HasNormalMap = 1u << 0,
        HasORMMap = 1u << 1,
        Fullbright = 1u << 2,
        Glow = 1u << 3,
        Water = 1u << 4,
        HasSpecularMap = 1u << 5,
        AlphaBlend = 1u << 6,
        AlphaMask = 1u << 7,
        DoubleSided = 1u << 8,
        LegacyBump = 1u << 9,
        LegacyShiny = 1u << 10,
        GLTFPBR = 1u << 11,
        PostDeferred = 1u << 12,
        AtmosphericHaze = 1u << 13,
        WaterHaze = 1u << 14,
        WaterExclusionMask = 1u << 15,
        AvatarImpostor = 1u << 16,
        SceneDepth = 1u << 17,
        SceneColor = 1u << 18,
        SceneDepthFlipY = 1u << 19,
        SceneDepthReversed = 1u << 20,
    };

    F32 mBaseColorRed = 1.f;
    F32 mBaseColorGreen = 1.f;
    F32 mBaseColorBlue = 1.f;
    F32 mBaseColorAlpha = 1.f;
    F32 mEmissiveColorRed = 0.f;
    F32 mEmissiveColorGreen = 0.f;
    F32 mEmissiveColorBlue = 0.f;
    F32 mHasEmissiveMap = 0.f;
    F32 mBaseTextureScaleS = 1.f;
    F32 mBaseTextureScaleT = 1.f;
    F32 mBaseTextureRotation = 0.f;
    F32 mBaseTextureOffsetS = 0.f;
    F32 mBaseTextureOffsetT = 0.f;
    F32 mRoughnessFactor = 1.f;
    F32 mMetallicFactor = 1.f;
    F32 mHasORMMap = 0.f;
    F32 mMaterialFlags = 0.f;
    F32 mSpecularColorRed = 1.f;
    F32 mSpecularColorGreen = 1.f;
    F32 mSpecularColorBlue = 1.f;
    F32 mEnvIntensity = 0.f;
    F32 mDiffuseAlphaMode = 0.f;
    F32 mGLTFAlphaMode = 0.f;
    F32 mBump = 0.f;
    F32 mShiny = 0.f;
    F32 mNormalTextureScaleS = 1.f;
    F32 mNormalTextureScaleT = 1.f;
    F32 mNormalTextureRotation = 0.f;
    F32 mNormalTextureOffsetS = 0.f;
    F32 mNormalTextureOffsetT = 0.f;
    F32 mORMTextureScaleS = 1.f;
    F32 mORMTextureScaleT = 1.f;
    F32 mORMTextureRotation = 0.f;
    F32 mORMTextureOffsetS = 0.f;
    F32 mORMTextureOffsetT = 0.f;
    F32 mEmissiveTextureScaleS = 1.f;
    F32 mEmissiveTextureScaleT = 1.f;
    F32 mEmissiveTextureRotation = 0.f;
    F32 mEmissiveTextureOffsetS = 0.f;
    F32 mEmissiveTextureOffsetT = 0.f;
    F32 mSceneAmbientRed = 0.36f;
    F32 mSceneAmbientGreen = 0.36f;
    F32 mSceneAmbientBlue = 0.36f;
    F32 mSceneDirectScale = 1.f;
    F32 mSceneDirectRed = 1.f;
    F32 mSceneDirectGreen = 1.f;
    F32 mSceneDirectBlue = 1.f;
    F32 mSceneLightingValid = 0.f;
    F32 mSceneLightDirectionX = 0.35f;
    F32 mSceneLightDirectionY = 0.45f;
    F32 mSceneLightDirectionZ = 0.82f;
    F32 mSceneLightDirectionValid = 0.f;
    F32 mCompositeClipPlane[4] =
    {
        0.f, 0.f, 0.f, 0.f,
    };
    F32 mCompositeSunDirection[4] =
    {
        0.35f, 0.45f, 0.82f, 1.f,
    };
    F32 mCompositeMoonDirection[4] =
    {
        -0.25f, -0.15f, 0.95f, 0.f,
    };
    F32 mCompositeSkySettings[4] =
    {
        0.f, 1.f, 1.4f, 4.f,
    };
    F32 mCompositeEnvironmentMatrix[9] =
    {
        1.f, 0.f, 0.f,
        0.f, 1.f, 0.f,
        0.f, 0.f, 1.f,
    };
    F32 mCompositeSSAOEffectMatrix[9] =
    {
        1.f, 0.f, 0.f,
        0.f, 1.f, 0.f,
        0.f, 0.f, 1.f,
    };
    F32 mCompositeSSR0[4] =
    {
        0.f, 16.f, 0.1f, 0.2f,
    };
    F32 mCompositeSSR1[4] =
    {
        0.2f, 1.f, 1.25f, 0.f,
    };
    F32 mCompositeInverseProjection[16] =
    {
        1.f, 0.f, 0.f, 0.f,
        0.f, 1.f, 0.f, 0.f,
        0.f, 0.f, 1.f, 0.f,
        0.f, 0.f, 0.f, 1.f,
    };
    F32 mCompositeInverseModelviewDelta[16] =
    {
        1.f, 0.f, 0.f, 0.f,
        0.f, 1.f, 0.f, 0.f,
        0.f, 0.f, 1.f, 0.f,
        0.f, 0.f, 0.f, 1.f,
    };
    F32 mCompositeShadowMatrix[16 * 6] =
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
    F32 mCompositeShadowClip[4] =
    {
        1.f, 64.f, 128.f, 256.f,
    };
    F32 mCompositeShadowSettings[4] =
    {
        0.f, 0.f, 0.f, 0.f,
    };
    F32 mCompositeShadowResolution[4] =
    {
        1.f, 1.f, 1.f, 1.f,
    };
    F32 mCompositeShadowRuntime[4] =
    {
        0.f, 0.f, 0.f, 0.f,
    };
    F32 mCompositeAtmosBlueHorizonHaze[4] =
    {
        0.4954f, 0.4954f, 0.6399f, 0.19f,
    };
    F32 mCompositeAtmosBlueDensityHaze[4] =
    {
        0.2447f, 0.4487f, 0.7599f, 0.7f,
    };
    F32 mCompositeAtmosDensity[4] =
    {
        0.f, 0.0001f, 0.8f, 1605.f,
    };
    F32 mCompositeAtmosGlow[4] =
    {
        18.f, 0.f, -0.01f, 1.f,
    };
    F32 mCompositeAtmosSunlight[4] =
    {
        1.f, 1.f, 1.f, 1.5f,
    };
    F32 mCompositeAtmosMoonlight[4] =
    {
        1.f, 1.f, 1.f, 1.5f,
    };
    F32 mCompositeAtmosAmbient[4] =
    {
        0.25f, 0.25f, 0.25f, 1.f,
    };
    F32 mCompositeAtmosLightNorm[4] =
    {
        0.f, 1.f, 0.f, 1.f,
    };
    F32 mLocalLight[MaxDeferredMultiLightCount * 4] = {};
    F32 mLocalLightColor[MaxDeferredMultiLightCount * 4] = {};
    F32 mLocalLightScreenSettings[4] =
    {
        1.f, 1.f, 0.f, 0.f,
    };
    F32 mLocalLightSunWashAndCount[4] =
    {
        0.f, 0.f, 0.f, 0.f,
    };
    F32 mLocalLightModelviewProjection[16] =
    {
        1.f, 0.f, 0.f, 0.f,
        0.f, 1.f, 0.f, 0.f,
        0.f, 0.f, 1.f, 0.f,
        0.f, 0.f, 0.f, 1.f,
    };
    F32 mLocalLightModelview[16] =
    {
        1.f, 0.f, 0.f, 0.f,
        0.f, 1.f, 0.f, 0.f,
        0.f, 0.f, 1.f, 0.f,
        0.f, 0.f, 0.f, 1.f,
    };
    F32 mLocalLightCenterSize[4] =
    {
        0.f, 0.f, 0.f, 1.f,
    };
    F32 mLocalLightProjectionMatrix[16] =
    {
        1.f, 0.f, 0.f, 0.f,
        0.f, 1.f, 0.f, 0.f,
        0.f, 0.f, 1.f, 0.f,
        0.f, 0.f, 0.f, 1.f,
    };
    F32 mLocalLightProjectionPAndNear[4] =
    {
        0.f, 0.f, 0.f, 0.f,
    };
    F32 mLocalLightProjectionNAndFocus[4] =
    {
        0.f, 0.f, -1.f, 0.f,
    };
    F32 mLocalLightProjectionLodRangeAmbiance[4] =
    {
        0.f, 1.f, 0.f, 0.f,
    };
    F32 mLocalLightNearFarSunShadow[4] =
    {
        0.f, 1.f, 0.f, 1.f,
    };
    F32 mLocalLightShadowIndices[4] =
    {
        -1.f, -1.f, 0.f, 0.f,
    };
    F32 mLocalLightProjectionOriginSize[4] =
    {
        0.f, 0.f, 0.f, 1.f,
    };
    F32 mLocalLightViewport[4] =
    {
        0.f, 0.f, 1.f, 1.f,
    };
};

struct LLRenderWorldTextureTransform
{
    F32 mS[4] = { 1.f, 0.f, 0.f, 0.f };
    F32 mT[4] = { 0.f, 1.f, 0.f, 0.f };
};

struct LLRenderTargetDesc
{
    const char* mDebugName = nullptr;
    LLRenderExtent2D mExtent;
    U32 mSampleCount = 1;
};

struct LLRenderTextureHandle
{
    U32 mValue = 0;

    LLRenderTextureHandle() = default;
    explicit LLRenderTextureHandle(U32 value) : mValue(value) {}

    U32 asLegacyName() const { return mValue; }
    bool isValid() const { return mValue != 0; }
    explicit operator bool() const { return isValid(); }

    friend bool operator==(LLRenderTextureHandle lhs, LLRenderTextureHandle rhs)
    {
        return lhs.mValue == rhs.mValue;
    }

    friend bool operator!=(LLRenderTextureHandle lhs, LLRenderTextureHandle rhs)
    {
        return !(lhs == rhs);
    }
};

struct LLRenderFramebufferHandle
{
    U32 mValue = 0;

    LLRenderFramebufferHandle() = default;
    explicit LLRenderFramebufferHandle(U32 value) : mValue(value) {}

    U32 asLegacyName() const { return mValue; }
    bool isValid() const { return mValue != 0; }
    explicit operator bool() const { return isValid(); }

    friend bool operator==(LLRenderFramebufferHandle lhs, LLRenderFramebufferHandle rhs)
    {
        return lhs.mValue == rhs.mValue;
    }

    friend bool operator!=(LLRenderFramebufferHandle lhs, LLRenderFramebufferHandle rhs)
    {
        return !(lhs == rhs);
    }
};

struct LLRenderBufferHandle
{
    U32 mValue = 0;

    LLRenderBufferHandle() = default;
    explicit LLRenderBufferHandle(U32 value) : mValue(value) {}

    U32 asLegacyName() const { return mValue; }
    bool isValid() const { return mValue != 0; }
    explicit operator bool() const { return isValid(); }

    friend bool operator==(LLRenderBufferHandle lhs, LLRenderBufferHandle rhs)
    {
        return lhs.mValue == rhs.mValue;
    }

    friend bool operator!=(LLRenderBufferHandle lhs, LLRenderBufferHandle rhs)
    {
        return !(lhs == rhs);
    }
};

struct LLRenderProgramHandle
{
    U32 mValue = 0;

    LLRenderProgramHandle() = default;
    explicit LLRenderProgramHandle(U32 value) : mValue(value) {}

    U32 asLegacyName() const { return mValue; }
    bool isValid() const { return mValue != 0; }
    explicit operator bool() const { return isValid(); }

    friend bool operator==(LLRenderProgramHandle lhs, LLRenderProgramHandle rhs)
    {
        return lhs.mValue == rhs.mValue;
    }

    friend bool operator!=(LLRenderProgramHandle lhs, LLRenderProgramHandle rhs)
    {
        return !(lhs == rhs);
    }
};

struct LLRenderShaderHandle
{
    U32 mValue = 0;

    LLRenderShaderHandle() = default;
    explicit LLRenderShaderHandle(U32 value) : mValue(value) {}

    U32 asLegacyName() const { return mValue; }
    bool isValid() const { return mValue != 0; }
    explicit operator bool() const { return isValid(); }

    friend bool operator==(LLRenderShaderHandle lhs, LLRenderShaderHandle rhs)
    {
        return lhs.mValue == rhs.mValue;
    }

    friend bool operator!=(LLRenderShaderHandle lhs, LLRenderShaderHandle rhs)
    {
        return !(lhs == rhs);
    }
};

struct LLRenderQueryHandle
{
    U32 mValue = 0;

    LLRenderQueryHandle() = default;
    explicit LLRenderQueryHandle(U32 value) : mValue(value) {}

    U32 asLegacyName() const { return mValue; }
    bool isValid() const { return mValue != 0; }
    explicit operator bool() const { return isValid(); }

    friend bool operator==(LLRenderQueryHandle lhs, LLRenderQueryHandle rhs)
    {
        return lhs.mValue == rhs.mValue;
    }

    friend bool operator!=(LLRenderQueryHandle lhs, LLRenderQueryHandle rhs)
    {
        return !(lhs == rhs);
    }

    friend bool operator<(LLRenderQueryHandle lhs, LLRenderQueryHandle rhs)
    {
        return lhs.mValue < rhs.mValue;
    }
};

struct LLRenderVertexArrayHandle
{
    U32 mValue = 0;

    LLRenderVertexArrayHandle() = default;
    explicit LLRenderVertexArrayHandle(U32 value) : mValue(value) {}

    U32 asLegacyName() const { return mValue; }
    bool isValid() const { return mValue != 0; }
    explicit operator bool() const { return isValid(); }

    friend bool operator==(LLRenderVertexArrayHandle lhs, LLRenderVertexArrayHandle rhs)
    {
        return lhs.mValue == rhs.mValue;
    }

    friend bool operator!=(LLRenderVertexArrayHandle lhs, LLRenderVertexArrayHandle rhs)
    {
        return !(lhs == rhs);
    }
};

struct LLRenderPassDesc
{
    const char* mDebugName = nullptr;
    LLRenderTargetDesc mTarget;
    LLRenderViewport mViewport;
    LLRenderScissor mScissor;
    LLRenderClearMask mClearMask = LL_RENDER_CLEAR_NONE;
    LLRenderClearColor mClearColor;
    F32 mClearDepth = 1.f;
    S32 mClearStencil = 0;
    LLRenderLoadAction mColorLoadAction = LLRenderLoadAction::Load;
    LLRenderStoreAction mColorStoreAction = LLRenderStoreAction::Store;
    LLRenderLoadAction mDepthLoadAction = LLRenderLoadAction::Load;
    LLRenderStoreAction mDepthStoreAction = LLRenderStoreAction::Store;
};

struct LLRenderFrameDesc
{
    const char* mDebugName = nullptr;
    LLRenderExtent2D mDrawableExtent;
};

struct LLRenderNativeContextDesc
{
    void* mWindow = nullptr;
    U32 mSamples = 0;
    bool mEnableVSync = true;
};

struct LLRenderNativeContext
{
    void* mView = nullptr;
    void* mContext = nullptr;
    void* mPixelFormat = nullptr;
    U32 mVRAM = 0;
};

#endif
