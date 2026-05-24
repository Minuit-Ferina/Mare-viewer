/**
 * @file llrenderbackend.cpp
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

#include "linden_common.h"

#include "llrenderbackend.h"

#include "llglcontainment.h"
#include "llglheaders.h"

namespace
{
LLGLenum to_opengl_blend_factor(LLRenderBlendFactor factor)
{
    switch (factor)
    {
    case LLRenderBlendFactor::One:
        return GL_ONE;
    case LLRenderBlendFactor::Zero:
        return GL_ZERO;
    case LLRenderBlendFactor::DestinationColor:
        return GL_DST_COLOR;
    case LLRenderBlendFactor::SourceColor:
        return GL_SRC_COLOR;
    case LLRenderBlendFactor::OneMinusDestinationColor:
        return GL_ONE_MINUS_DST_COLOR;
    case LLRenderBlendFactor::OneMinusSourceColor:
        return GL_ONE_MINUS_SRC_COLOR;
    case LLRenderBlendFactor::DestinationAlpha:
        return GL_DST_ALPHA;
    case LLRenderBlendFactor::SourceAlpha:
        return GL_SRC_ALPHA;
    case LLRenderBlendFactor::OneMinusDestinationAlpha:
        return GL_ONE_MINUS_DST_ALPHA;
    case LLRenderBlendFactor::OneMinusSourceAlpha:
        return GL_ONE_MINUS_SRC_ALPHA;
    default:
        return GL_ZERO;
    }
}

class LLNullRenderBackend final : public LLRenderBackend
{
public:
    LLRenderBackendType getType() const override { return LLRenderBackendType::Null; }
    const char* getName() const override { return "Null"; }
    bool isReady() const override { return true; }

    void beginFrame(const LLRenderFrameDesc&) override {}
    void endFrame() override {}

    void beginRenderPass(const LLRenderPassDesc&) override {}
    void endRenderPass() override {}

    void setViewport(const LLRenderViewport&) override {}
    void setScissor(const LLRenderScissor&) override {}
    void clear(const LLRenderPassDesc&) override {}
    void setClearColor(const LLRenderClearColor&) override {}
    void setColorMask(const LLRenderColorMask&) override {}
    void setBlendState(const LLRenderBlendState&) override {}
    void setLineWidth(F32) override {}
};

class LLOpenGLRenderBackend final : public LLRenderBackend
{
public:
    LLRenderBackendType getType() const override { return LLRenderBackendType::OpenGL; }
    const char* getName() const override { return "OpenGL"; }
    bool isReady() const override { return true; }

    void beginFrame(const LLRenderFrameDesc&) override {}
    void endFrame() override {}

    void beginRenderPass(const LLRenderPassDesc&) override {}
    void endRenderPass() override {}

    void setViewport(const LLRenderViewport& viewport) override
    {
        LLGLContainment::setViewport(
            static_cast<LLGLint>(viewport.mX),
            static_cast<LLGLint>(viewport.mY),
            static_cast<LLGLint>(viewport.mWidth),
            static_cast<LLGLint>(viewport.mHeight));
    }

    void setScissor(const LLRenderScissor& scissor) override
    {
        if (!scissor.mEnabled)
        {
            return;
        }

        LLGLContainment::setScissorBox(
            scissor.mX,
            scissor.mY,
            static_cast<U32>(scissor.mWidth),
            static_cast<U32>(scissor.mHeight));
    }

    void clear(const LLRenderPassDesc& desc) override
    {
        LLGLContainment::clearBuffersByIntent(
            (desc.mClearMask & LL_RENDER_CLEAR_COLOR) != 0,
            (desc.mClearMask & LL_RENDER_CLEAR_DEPTH) != 0,
            (desc.mClearMask & LL_RENDER_CLEAR_STENCIL) != 0);
    }

    void setClearColor(const LLRenderClearColor& color) override
    {
        LLGLContainment::setClearColor(color.mRed, color.mGreen, color.mBlue, color.mAlpha);
    }

    void setColorMask(const LLRenderColorMask& mask) override
    {
        LLGLContainment::setColorMask(
            static_cast<LLGLboolean>(mask.mRed),
            static_cast<LLGLboolean>(mask.mGreen),
            static_cast<LLGLboolean>(mask.mBlue),
            static_cast<LLGLboolean>(mask.mAlpha));
    }

    void setBlendState(const LLRenderBlendState& blend) override
    {
        LLGLContainment::setSeparateBlendFunction(
            to_opengl_blend_factor(blend.mColorSource),
            to_opengl_blend_factor(blend.mColorDestination),
            to_opengl_blend_factor(blend.mAlphaSource),
            to_opengl_blend_factor(blend.mAlphaDestination));
    }

    void setLineWidth(F32 width) override
    {
        LLGLContainment::setLineWidth(width);
    }
};
}

LLRenderBackend::~LLRenderBackend() = default;

const char* getRenderBackendTypeName(LLRenderBackendType type)
{
    switch (type)
    {
    case LLRenderBackendType::OpenGL:
        return "OpenGL";
    case LLRenderBackendType::Vulkan:
        return "Vulkan";
    case LLRenderBackendType::Metal:
        return "Metal";
    case LLRenderBackendType::Null:
        return "Null";
    case LLRenderBackendType::Unknown:
    default:
        return "Unknown";
    }
}

LLRenderBackend& getNullRenderBackend()
{
    static LLNullRenderBackend backend;
    return backend;
}

LLRenderBackend& getOpenGLRenderBackend()
{
    static LLOpenGLRenderBackend backend;
    return backend;
}
