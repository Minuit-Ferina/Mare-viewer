/**
 * @file llrendertarget.cpp
 * @brief LLRenderTarget implementation
 *
 * $LicenseInfo:firstyear=2001&license=viewerlgpl$
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

#include "linden_common.h"

#include "llrendertarget.h"
#include "llrender.h"
#include "llgl.h"
#include "llglcontainment.h"
#include "llrenderbackend.h"

LLRenderTarget* LLRenderTarget::sBoundTarget = NULL;
U32 LLRenderTarget::sBytesAllocated = 0;

bool LLRenderTarget::sUseFBO = false;
U32 LLRenderTarget::sCurFBO = 0;


extern S32 gGLViewport[4];

U32 LLRenderTarget::sCurResX = 0;
U32 LLRenderTarget::sCurResY = 0;

namespace
{
void check_current_draw_framebuffer_status()
{
    if (gDebugGL)
    {
        U32 status = LLGLContainment::getDrawFramebufferStatus();
        switch (status)
        {
        case GL_FRAMEBUFFER_COMPLETE:
            break;
        default:
            LL_WARNS() << "check_framebuffer_status failed -- " << std::hex << status << LL_ENDL;
            ll_fail("check_framebuffer_status failed");
            break;
        }
    }
}

void set_render_target_viewport(U32 width, U32 height)
{
    LLRenderViewport viewport;
    viewport.mWidth = static_cast<F32>(width);
    viewport.mHeight = static_cast<F32>(height);
    getOpenGLRenderBackend().setViewport(viewport);
    LLRenderTarget::sCurResX = width;
    LLRenderTarget::sCurResY = height;
}

void restore_default_framebuffer_viewport()
{
    LLRenderViewport viewport;
    viewport.mX = static_cast<F32>(gGLViewport[0]);
    viewport.mY = static_cast<F32>(gGLViewport[1]);
    viewport.mWidth = static_cast<F32>(gGLViewport[2]);
    viewport.mHeight = static_cast<F32>(gGLViewport[3]);
    getOpenGLRenderBackend().setViewport(viewport);
    LLRenderTarget::sCurResX = gGLViewport[2];
    LLRenderTarget::sCurResY = gGLViewport[3];
}

void bind_render_target_fbo(U32 fbo)
{
    LLGLContainment::bindReadWriteFramebuffer(fbo);
    LLRenderTarget::sCurFBO = fbo;
}

void bind_attachment_fbo(U32 fbo)
{
    LLGLContainment::bindReadWriteFramebuffer(fbo);
}

void generate_framebuffer_name(U32* fbo)
{
    LLGLContainment::generateFramebuffers(1, fbo);
}

void delete_framebuffer_name(U32* fbo)
{
    LLGLContainment::deleteFramebuffers(1, fbo);
}

void restore_tracked_fbo_binding()
{
    LLGLContainment::bindReadWriteFramebuffer(LLRenderTarget::sCurFBO);
}

void set_framebuffer_texture_attachment(GLenum attachment, LLTexUnit::eTextureType usage, U32 texture)
{
    LLGLContainment::setReadWriteFramebufferTexture2D(
        static_cast<LLGLenum>(attachment),
        static_cast<LLGLenum>(LLTexUnit::getInternalType(usage)),
        static_cast<LLGLuint>(texture),
        0);
}

void clear_framebuffer_texture_attachment(GLenum attachment, LLTexUnit::eTextureType usage)
{
    set_framebuffer_texture_attachment(attachment, usage, 0);
}

void bind_default_framebuffer_for_flush()
{
    LLGLContainment::bindReadWriteFramebuffer(0);
    LLRenderTarget::sCurFBO = 0;
}

void forget_current_fbo_and_bind_default()
{
    LLRenderTarget::sCurFBO = 0;
    LLGLContainment::bindReadWriteFramebuffer(0);
}

void set_render_target_buffer_routing(U32 color_attachment_count)
{
    LLGLenum drawbuffers[] = {GL_COLOR_ATTACHMENT0,
                              GL_COLOR_ATTACHMENT1,
                              GL_COLOR_ATTACHMENT2,
                              GL_COLOR_ATTACHMENT3};

    if (color_attachment_count == 0)
    {
        LLGLContainment::setDrawBuffer(GL_NONE);
        LLGLContainment::setReadBuffer(GL_NONE);
    }
    else
    {
        LLGLContainment::setDrawBuffers(static_cast<S32>(color_attachment_count), drawbuffers);
        LLGLContainment::setReadBuffer(GL_COLOR_ATTACHMENT0);
    }
}

void restore_default_framebuffer_buffer_routing()
{
    LLGLContainment::setReadBuffer(GL_BACK);
    LLGLContainment::setDrawBuffer(GL_BACK);
}

void generate_bound_render_target_mipmaps()
{
    LLGLContainment::generateTextureMipmap(GL_TEXTURE_2D);
}

void clear_render_target_buffers(U32 mask)
{
    LLGLContainment::clearBuffers(mask);
}

void set_render_target_scissor(U32 width, U32 height)
{
    LLRenderScissor scissor;
    scissor.mWidth = width;
    scissor.mHeight = height;
    scissor.mEnabled = true;
    getOpenGLRenderBackend().setScissor(scissor);
}

bool render_target_texture_allocation_failed()
{
    return LLGLContainment::getError() != GL_NO_ERROR;
}
}

LLRenderTarget::LLRenderTarget() :
    mResX(0),
    mResY(0),
    mFBO(0),
    mDepth(0),
    mUseDepth(false),
    mUsage(LLTexUnit::TT_TEXTURE)
{
}

LLRenderTarget::~LLRenderTarget()
{
    release();
}

void LLRenderTarget::resize(U32 resx, U32 resy)
{
    //for accounting, get the number of pixels added/subtracted
    S32 pix_diff = (resx*resy)-(mResX*mResY);

    mResX = resx;
    mResY = resy;

    llassert(mInternalFormat.size() == mTex.size());

    for (U32 i = 0; i < mTex.size(); ++i)
    { //resize color attachments
        gGL.getTexUnit(0)->bindManual(mUsage, mTex[i]);
        LLImageGL::setManualImage(LLTexUnit::getInternalType(mUsage), 0, mInternalFormat[i], mResX, mResY, GL_RGBA, GL_UNSIGNED_BYTE, NULL, false);
        sBytesAllocated += pix_diff*4;
    }

    if (mDepth)
    {
        gGL.getTexUnit(0)->bindManual(mUsage, mDepth);
        U32 internal_type = LLTexUnit::getInternalType(mUsage);
        LLImageGL::setManualImage(internal_type, 0, GL_DEPTH_COMPONENT24, mResX, mResY, GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, NULL, false);

        sBytesAllocated += pix_diff*4;
    }
}


bool LLRenderTarget::allocate(U32 resx, U32 resy, U32 color_fmt, bool depth, LLTexUnit::eTextureType usage, LLTexUnit::eTextureMipGeneration generateMipMaps)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_DISPLAY;
    llassert(usage == LLTexUnit::TT_TEXTURE);
    llassert(!isBoundInStack());

    resx = llmin(resx, (U32) gGLManager.mGLMaxTextureSize);
    resy = llmin(resy, (U32) gGLManager.mGLMaxTextureSize);

    release();

    mResX = resx;
    mResY = resy;

    mUsage = usage;
    mUseDepth = depth;

    mGenerateMipMaps = generateMipMaps;

    if (mGenerateMipMaps != LLTexUnit::TMG_NONE) {
        // Calculate the number of mip levels based upon resolution that we should have.
        mMipLevels = 1 + (U32)floor(log10((float)llmax(mResX, mResY)) / log10(2.0));
    }

    if (depth)
    {
        if (!allocateDepth())
        {
            LL_WARNS() << "Failed to allocate depth buffer for render target." << LL_ENDL;
            return false;
        }
    }

    generate_framebuffer_name(&mFBO);

    if (mDepth)
    {
        bind_attachment_fbo(mFBO);

        set_framebuffer_texture_attachment(GL_DEPTH_ATTACHMENT, mUsage, mDepth);

        restore_tracked_fbo_binding();
    }

    return addColorAttachment(color_fmt);
}

void LLRenderTarget::setColorAttachment(LLImageGL* img, LLGLuint use_name)
{
    LL_PROFILE_ZONE_SCOPED;
    llassert(img != nullptr); // img must not be null
    llassert(sUseFBO); // FBO support must be enabled
    llassert(mDepth == 0); // depth buffers not supported with this mode
    llassert(mTex.empty()); // mTex must be empty with this mode (binding target should be done via LLImageGL)
    llassert(!isBoundInStack());

    if (mFBO == 0)
    {
        generate_framebuffer_name(&mFBO);
    }

    mResX = img->getWidth();
    mResY = img->getHeight();
    mUsage = img->getTarget();

    if (use_name == 0)
    {
        use_name = img->getTexName();
    }

    mTex.push_back(use_name);

    bind_attachment_fbo(mFBO);
    set_framebuffer_texture_attachment(GL_COLOR_ATTACHMENT0, mUsage, use_name);
    stop_glerror();

    check_current_draw_framebuffer_status();

    restore_tracked_fbo_binding();
}

void LLRenderTarget::releaseColorAttachment()
{
    LL_PROFILE_ZONE_SCOPED;
    llassert(!isBoundInStack());
    llassert(mTex.size() == 1); //cannot use releaseColorAttachment with LLRenderTarget managed color targets
    llassert(mFBO != 0);  // mFBO must be valid

    bind_attachment_fbo(mFBO);
    clear_framebuffer_texture_attachment(GL_COLOR_ATTACHMENT0, mUsage);
    restore_tracked_fbo_binding();

    mTex.clear();
}

bool LLRenderTarget::addColorAttachment(U32 color_fmt)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_DISPLAY;
    llassert(!isBoundInStack());

    if (color_fmt == 0)
    {
        return true;
    }

    U32 offset = static_cast<U32>(mTex.size());

    if( offset >= 4 )
    {
        LL_WARNS() << "Too many color attachments" << LL_ENDL;
        llassert( offset < 4 );
        return false;
    }
    if( offset > 0 && (mFBO == 0) )
    {
        llassert(  mFBO != 0 );
        return false;
    }

    U32 tex;
    LLImageGL::generateTextures(1, &tex);
    gGL.getTexUnit(0)->bindManual(mUsage, tex);

    stop_glerror();


    {
        clear_glerror();
        LLImageGL::setManualImage(LLTexUnit::getInternalType(mUsage), 0, color_fmt, mResX, mResY, GL_RGBA, GL_UNSIGNED_BYTE, NULL, false);
        if (render_target_texture_allocation_failed())
        {
            LL_WARNS() << "Could not allocate color buffer for render target." << LL_ENDL;
            return false;
        }
    }

    sBytesAllocated += mResX*mResY*4;

    stop_glerror();


    if (offset == 0)
    { //use bilinear filtering on single texture render targets that aren't multisampled
        gGL.getTexUnit(0)->setTextureFilteringOption(LLTexUnit::TFO_BILINEAR);
        stop_glerror();
    }
    else
    { //don't filter data attachments
        gGL.getTexUnit(0)->setTextureFilteringOption(LLTexUnit::TFO_POINT);
        stop_glerror();
    }

    if (mUsage != LLTexUnit::TT_RECT_TEXTURE)
    {
        gGL.getTexUnit(0)->setTextureAddressMode(LLTexUnit::TAM_MIRROR);
        stop_glerror();
    }
    else
    {
        // ATI doesn't support mirrored repeat for rectangular textures.
        gGL.getTexUnit(0)->setTextureAddressMode(LLTexUnit::TAM_CLAMP);
        stop_glerror();
    }

    if (mFBO)
    {
        bind_attachment_fbo(mFBO);
        set_framebuffer_texture_attachment(static_cast<GLenum>(GL_COLOR_ATTACHMENT0 + offset), mUsage, tex);

        check_current_draw_framebuffer_status();

        restore_tracked_fbo_binding();
    }

    mTex.push_back(tex);
    mInternalFormat.push_back(color_fmt);

    if (gDebugGL)
    { //bind and unbind to validate target
        bindTarget();
        flush();
    }


    return true;
}

bool LLRenderTarget::allocateDepth()
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_DISPLAY;
    LLImageGL::generateTextures(1, &mDepth);
    gGL.getTexUnit(0)->bindManual(mUsage, mDepth);

    U32 internal_type = LLTexUnit::getInternalType(mUsage);
    stop_glerror();
    clear_glerror();
    LLImageGL::setManualImage(internal_type, 0, GL_DEPTH_COMPONENT24, mResX, mResY, GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, NULL, false);
    gGL.getTexUnit(0)->setTextureFilteringOption(LLTexUnit::TFO_POINT);

    sBytesAllocated += mResX*mResY*4;

    if (render_target_texture_allocation_failed())
    {
        LL_WARNS() << "Unable to allocate depth buffer for render target." << LL_ENDL;
        return false;
    }

    return true;
}

void LLRenderTarget::shareDepthBuffer(LLRenderTarget& target)
{
    llassert(!isBoundInStack());

    if (!mFBO || !target.mFBO)
    {
        LL_ERRS() << "Cannot share depth buffer between non FBO render targets." << LL_ENDL;
    }

    if (target.mDepth)
    {
        LL_ERRS() << "Attempting to override existing depth buffer.  Detach existing buffer first." << LL_ENDL;
    }

    if (target.mUseDepth)
    {
        LL_ERRS() << "Attempting to override existing shared depth buffer. Detach existing buffer first." << LL_ENDL;
    }

    if (mDepth)
    {
        bind_attachment_fbo(target.mFBO);

        set_framebuffer_texture_attachment(GL_DEPTH_ATTACHMENT, mUsage, mDepth);

        check_current_draw_framebuffer_status();

        restore_tracked_fbo_binding();

        target.mUseDepth = true;
    }
}

void LLRenderTarget::release()
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_DISPLAY;
    llassert(!isBoundInStack());

    if (mDepth)
    {
        LLImageGL::deleteTextures(1, &mDepth);

        mDepth = 0;

        sBytesAllocated -= mResX*mResY*4;
    }
    else if (mFBO)
    {
        bind_attachment_fbo(mFBO);

        if (mUseDepth)
        { //detach shared depth buffer
            clear_framebuffer_texture_attachment(GL_DEPTH_ATTACHMENT, mUsage);
            mUseDepth = false;
        }

        restore_tracked_fbo_binding();
    }

    // Detach any extra color buffers (e.g. SRGB spec buffers)
    //
    if (mFBO && (mTex.size() > 1))
    {
        bind_attachment_fbo(mFBO);
        size_t z;
        for (z = mTex.size() - 1; z >= 1; z--)
        {
            sBytesAllocated -= mResX*mResY*4;
            clear_framebuffer_texture_attachment(static_cast<GLenum>(GL_COLOR_ATTACHMENT0 + z), mUsage);
            LLImageGL::deleteTextures(1, &mTex[z]);
        }
        restore_tracked_fbo_binding();
    }

    if (mFBO)
    {
        if (mFBO == sCurFBO)
        {
            forget_current_fbo_and_bind_default();
        }

        delete_framebuffer_name(&mFBO);
        mFBO = 0;
    }

    if (mTex.size() > 0)
    {
        sBytesAllocated -= mResX*mResY*4;
        LLImageGL::deleteTextures(1, &mTex[0]);
    }

    mTex.clear();
    mInternalFormat.clear();

    mResX = mResY = 0;
}

void LLRenderTarget::bindTarget()
{
    LL_PROFILE_GPU_ZONE("bindTarget");
    llassert(mFBO);
    llassert(!isBoundInStack());

    bind_render_target_fbo(mFBO);

    set_render_target_buffer_routing(static_cast<U32>(mTex.size()));
    check_current_draw_framebuffer_status();

    set_render_target_viewport(mResX, mResY);

    mPreviousRT = sBoundTarget;
    sBoundTarget = this;
}

void LLRenderTarget::clear(U32 mask_in)
{
    LL_PROFILE_GPU_ZONE("clear");
    llassert(mFBO);
    U32 mask = GL_COLOR_BUFFER_BIT;
    if (mUseDepth)
    {
        mask |= GL_DEPTH_BUFFER_BIT;

    }
    if (mFBO)
    {
        check_current_draw_framebuffer_status();
        stop_glerror();
        clear_render_target_buffers(mask & mask_in);
        stop_glerror();
    }
    else
    {
        LLGLEnable scissor(GL_SCISSOR_TEST);
        set_render_target_scissor(mResX, mResY);
        stop_glerror();
        clear_render_target_buffers(mask & mask_in);
    }
}

U32 LLRenderTarget::getTexture(U32 attachment) const
{
    if (attachment >= mTex.size())
    {
        LL_WARNS() << "Invalid attachment index " << attachment << " for size " << mTex.size() << LL_ENDL;
        llassert(false);
        return 0;
    }
    return mTex[attachment];
}

U32 LLRenderTarget::getNumTextures() const
{
    return static_cast<U32>(mTex.size());
}

void LLRenderTarget::bindTexture(U32 index, S32 channel, LLTexUnit::eTextureFilterOptions filter_options)
{
    gGL.getTexUnit(channel)->bindManual(mUsage, getTexture(index), filter_options == LLTexUnit::TFO_TRILINEAR || filter_options == LLTexUnit::TFO_ANISOTROPIC);
    gGL.getTexUnit(channel)->setTextureFilteringOption(filter_options);
}

void LLRenderTarget::flush()
{
    LL_PROFILE_GPU_ZONE("rt flush");
    gGL.flush();
    llassert(mFBO);
    llassert(sCurFBO == mFBO);
    llassert(sBoundTarget == this);

    if (mGenerateMipMaps == LLTexUnit::TMG_AUTO)
    {
        LL_PROFILE_GPU_ZONE("rt generate mipmaps");
        bindTexture(0, 0, LLTexUnit::TFO_TRILINEAR);
        generate_bound_render_target_mipmaps();
    }

    if (mPreviousRT)
    {
        // a bit hacky -- pop the RT stack back two frames and push
        // the previous frame back on to play nice with the GL state machine
        sBoundTarget = mPreviousRT->mPreviousRT;
        mPreviousRT->bindTarget();
    }
    else
    {
        sBoundTarget = nullptr;
        bind_default_framebuffer_for_flush();
        restore_default_framebuffer_viewport();
        restore_default_framebuffer_buffer_routing();
    }
}

bool LLRenderTarget::isComplete() const
{
    return !mTex.empty() || mDepth;
}

void LLRenderTarget::getViewport(S32* viewport)
{
    viewport[0] = 0;
    viewport[1] = 0;
    viewport[2] = mResX;
    viewport[3] = mResY;
}

bool LLRenderTarget::isBoundInStack() const
{
    LLRenderTarget* cur = sBoundTarget;
    while (cur && cur != this)
    {
        cur = cur->mPreviousRT;
    }

    return cur == this;
}

void LLRenderTarget::swapFBORefs(LLRenderTarget& other)
{
    // Must be initialized
    llassert(mFBO);
    llassert(other.mFBO);

    // Must be unbound
    // *NOTE: mPreviousRT can be non-null even if this target is unbound - presumably for debugging purposes?
    llassert(sCurFBO != mFBO);
    llassert(sCurFBO != other.mFBO);
    llassert(!isBoundInStack());
    llassert(!other.isBoundInStack());

    // Must be same type
    llassert(sUseFBO == other.sUseFBO);
    llassert(mResX == other.mResX);
    llassert(mResY == other.mResY);
    llassert(mInternalFormat == other.mInternalFormat);
    llassert(mTex.size() == other.mTex.size());
    llassert(mDepth == other.mDepth);
    llassert(mUseDepth == other.mUseDepth);
    llassert(mGenerateMipMaps == other.mGenerateMipMaps);
    llassert(mMipLevels == other.mMipLevels);
    llassert(mUsage == other.mUsage);

    std::swap(mFBO, other.mFBO);
    std::swap(mTex, other.mTex);
}
