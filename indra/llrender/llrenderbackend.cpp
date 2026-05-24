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

LLGLenum to_opengl_capability(LLRenderCapability capability)
{
    switch (capability)
    {
    case LLRenderCapability::DebugOutputSynchronous:
        return GL_DEBUG_OUTPUT_SYNCHRONOUS;
    case LLRenderCapability::DepthTest:
        return GL_DEPTH_TEST;
    case LLRenderCapability::Multisample:
        return GL_MULTISAMPLE;
    case LLRenderCapability::TextureCubeMapSeamless:
        return GL_TEXTURE_CUBE_MAP_SEAMLESS;
    default:
        return 0;
    }
}

LLGLenum to_opengl_cull_face(LLRenderCullFace face)
{
    switch (face)
    {
    case LLRenderCullFace::Front:
        return GL_FRONT;
    case LLRenderCullFace::Back:
        return GL_BACK;
    case LLRenderCullFace::FrontAndBack:
        return GL_FRONT_AND_BACK;
    default:
        return GL_BACK;
    }
}

LLGLenum to_opengl_depth_function(LLRenderDepthFunction function)
{
    switch (function)
    {
    case LLRenderDepthFunction::Always:
        return GL_ALWAYS;
    case LLRenderDepthFunction::Less:
        return GL_LESS;
    case LLRenderDepthFunction::LessEqual:
        return GL_LEQUAL;
    case LLRenderDepthFunction::Equal:
        return GL_EQUAL;
    case LLRenderDepthFunction::NotEqual:
        return GL_NOTEQUAL;
    case LLRenderDepthFunction::GreaterEqual:
        return GL_GEQUAL;
    case LLRenderDepthFunction::Greater:
        return GL_GREATER;
    default:
        return GL_LESS;
    }
}

LLGLenum to_opengl_texture_target(LLRenderTextureTarget target)
{
    switch (target)
    {
    case LLRenderTextureTarget::Texture2D:
        return GL_TEXTURE_2D;
    case LLRenderTextureTarget::TextureCubeMap:
        return GL_TEXTURE_CUBE_MAP;
    default:
        return GL_TEXTURE_2D;
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
    void setCapability(LLRenderCapability, bool) override {}
    void setCullFace(LLRenderCullFace) override {}
    void setDepthFunction(LLRenderDepthFunction) override {}
    void setDepthWriteEnabled(bool) override {}
    void generateMipmaps(LLRenderTextureTarget) override {}
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

    void setCapability(LLRenderCapability capability, bool enabled) override
    {
        LLGLenum gl_capability = to_opengl_capability(capability);
        if (!gl_capability)
        {
            return;
        }

        if (enabled)
        {
            LLGLContainment::enableCapability(gl_capability);
        }
        else
        {
            LLGLContainment::disableCapability(gl_capability);
        }
    }

    void setCullFace(LLRenderCullFace face) override
    {
        LLGLContainment::setCullFace(to_opengl_cull_face(face));
    }

    void setDepthFunction(LLRenderDepthFunction function) override
    {
        LLGLContainment::setDepthFunction(to_opengl_depth_function(function));
    }

    void setDepthWriteEnabled(bool enabled) override
    {
        LLGLContainment::setDepthMask(static_cast<LLGLboolean>(enabled));
    }

    void generateMipmaps(LLRenderTextureTarget target) override
    {
        LLGLContainment::generateTextureMipmap(to_opengl_texture_target(target));
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
