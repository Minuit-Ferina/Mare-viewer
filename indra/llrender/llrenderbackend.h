/**
 * @file llrenderbackend.h
 * @brief Backend-neutral rendering intent interface.
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

#ifndef LL_LLRENDERBACKEND_H
#define LL_LLRENDERBACKEND_H

#include "stdtypes.h"

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
    DebugOutputSynchronous,
    DepthTest,
    Multisample,
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

enum LLRenderClearMask : U32
{
    LL_RENDER_CLEAR_NONE = 0,
    LL_RENDER_CLEAR_COLOR = 1 << 0,
    LL_RENDER_CLEAR_DEPTH = 1 << 1,
    LL_RENDER_CLEAR_STENCIL = 1 << 2,
};

struct LLRenderExtent2D
{
    S32 mWidth = 0;
    S32 mHeight = 0;
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

struct LLRenderTargetDesc
{
    const char* mDebugName = nullptr;
    LLRenderExtent2D mExtent;
    U32 mSampleCount = 1;
};

struct LLRenderPassDesc
{
    const char* mDebugName = nullptr;
    LLRenderTargetDesc mTarget;
    LLRenderViewport mViewport;
    LLRenderScissor mScissor;
    U32 mClearMask = LL_RENDER_CLEAR_NONE;
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

class LLRenderBackend
{
public:
    virtual ~LLRenderBackend();

    virtual LLRenderBackendType getType() const = 0;
    virtual const char* getName() const = 0;
    virtual bool isReady() const = 0;

    virtual void beginFrame(const LLRenderFrameDesc& desc) = 0;
    virtual void endFrame() = 0;

    virtual void beginRenderPass(const LLRenderPassDesc& desc) = 0;
    virtual void endRenderPass() = 0;

    virtual void setViewport(const LLRenderViewport& viewport) = 0;
    virtual void setScissor(const LLRenderScissor& scissor) = 0;
    virtual void clear(const LLRenderPassDesc& desc) = 0;
    virtual void setClearColor(const LLRenderClearColor& color) = 0;
    virtual void setColorMask(const LLRenderColorMask& mask) = 0;
    virtual void setBlendState(const LLRenderBlendState& blend) = 0;
    virtual void setLineWidth(F32 width) = 0;
    virtual void setCapability(LLRenderCapability capability, bool enabled) = 0;
    virtual void setCullFace(LLRenderCullFace face) = 0;
    virtual void setDepthFunction(LLRenderDepthFunction function) = 0;
    virtual void setDepthWriteEnabled(bool enabled) = 0;
};

const char* getRenderBackendTypeName(LLRenderBackendType type);
LLRenderBackend& getNullRenderBackend();
LLRenderBackend& getOpenGLRenderBackend();

#endif
