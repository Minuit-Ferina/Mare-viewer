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
#include "llimagegl.h"
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
LLRenderTextureTarget to_render_texture_target(LLTexUnit::eTextureType type)
{
    switch (type)
    {
    case LLTexUnit::TT_TEXTURE:
        return LLRenderTextureTarget::Texture2D;
    case LLTexUnit::TT_RECT_TEXTURE:
        return LLRenderTextureTarget::TextureRectangle;
    case LLTexUnit::TT_CUBE_MAP:
        return LLRenderTextureTarget::TextureCubeMap;
    case LLTexUnit::TT_CUBE_MAP_ARRAY:
        return LLRenderTextureTarget::TextureCubeMapArray;
    case LLTexUnit::TT_MULTISAMPLE_TEXTURE:
        return LLRenderTextureTarget::Texture2DMultisample;
    case LLTexUnit::TT_TEXTURE_3D:
        return LLRenderTextureTarget::Texture3D;
    case LLTexUnit::TT_NONE:
    default:
        llassert(false);
        return LLRenderTextureTarget::Texture2D;
    }
}

LLRenderFramebufferAttachment to_render_color_attachment(U32 index)
{
    switch (index)
    {
    case 0:
        return LLRenderFramebufferAttachment::Color0;
    case 1:
        return LLRenderFramebufferAttachment::Color1;
    case 2:
        return LLRenderFramebufferAttachment::Color2;
    case 3:
        return LLRenderFramebufferAttachment::Color3;
    default:
        llassert(false);
        return LLRenderFramebufferAttachment::Color0;
    }
}

LLRenderTextureFormat infer_render_target_color_format(const LLImageGL& image)
{
    switch (image.getComponents())
    {
    case 1:
        return LLRenderTextureFormat::R8;
    case 2:
        return LLRenderTextureFormat::RG8;
    case 3:
        return LLRenderTextureFormat::RGB8;
    case 4:
    default:
        return LLRenderTextureFormat::RGBA;
    }
}

void check_current_draw_framebuffer_status()
{
    if (gDebugGL)
    {
        if (!getRenderBackend().isDrawFramebufferComplete())
        {
            LL_WARNS() << "check_framebuffer_status failed" << LL_ENDL;
            ll_fail("check_framebuffer_status failed");
        }
    }
}

void set_render_target_viewport(U32 width, U32 height)
{
    LLRenderViewport viewport;
    viewport.mWidth = static_cast<F32>(width);
    viewport.mHeight = static_cast<F32>(height);
    getRenderBackend().setViewport(viewport);
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
    getRenderBackend().setViewport(viewport);
    LLRenderTarget::sCurResX = gGLViewport[2];
    LLRenderTarget::sCurResY = gGLViewport[3];
}

void bind_render_target_fbo(LLRenderFramebufferHandle fbo)
{
    getRenderBackend().bindReadWriteFramebuffer(fbo);
    LLRenderTarget::sCurFBO = fbo.asLegacyName();
}

void bind_attachment_fbo(LLRenderFramebufferHandle fbo)
{
    getRenderBackend().bindReadWriteFramebuffer(fbo);
}

LLRenderFramebufferHandle generate_framebuffer_handle()
{
    return getRenderBackend().createFramebufferHandle();
}

void delete_framebuffer_handle(LLRenderFramebufferHandle& fbo)
{
    getRenderBackend().deleteFramebufferHandle(fbo);
    fbo = LLRenderFramebufferHandle();
}

void restore_tracked_fbo_binding()
{
    getRenderBackend().bindReadWriteFramebuffer(LLRenderTarget::sCurFBO);
}

void set_framebuffer_texture_attachment(
    LLRenderFramebufferAttachment attachment,
    LLTexUnit::eTextureType usage,
    LLRenderTextureHandle texture)
{
    getRenderBackend().attachFramebufferTexture2D(
        attachment,
        to_render_texture_target(usage),
        texture,
        0);
}

void set_framebuffer_texture_attachment(
    LLRenderFramebufferAttachment attachment,
    LLTexUnit::eTextureType usage,
    LLRenderTextureHandle texture,
    LLRenderTextureFormat format,
    U32 width,
    U32 height)
{
    getRenderBackend().noteTextureAllocation(
        texture,
        format,
        width,
        height);
    set_framebuffer_texture_attachment(attachment, usage, texture);
}

void clear_framebuffer_texture_attachment(
    LLRenderFramebufferAttachment attachment,
    LLTexUnit::eTextureType usage)
{
    set_framebuffer_texture_attachment(attachment, usage, LLRenderTextureHandle());
}

void bind_default_framebuffer_for_flush()
{
    getRenderBackend().bindReadWriteFramebuffer(0);
    LLRenderTarget::sCurFBO = 0;
}

void forget_current_fbo_and_bind_default()
{
    LLRenderTarget::sCurFBO = 0;
    getRenderBackend().bindReadWriteFramebuffer(0);
}

void set_render_target_buffer_routing(U32 color_attachment_count)
{
    getRenderBackend().setFramebufferBufferRouting(color_attachment_count);
}

void restore_default_framebuffer_buffer_routing()
{
    getRenderBackend().restoreDefaultFramebufferBufferRouting();
}

void generate_bound_render_target_mipmaps()
{
    getRenderBackend().generateMipmaps(LLRenderTextureTarget::Texture2D);
}

void clear_render_target_buffers(LLRenderClearMask mask)
{
    getRenderBackend().clear(mask);
}

void set_render_target_scissor(U32 width, U32 height)
{
    LLRenderScissor scissor;
    scissor.mWidth = width;
    scissor.mHeight = height;
    scissor.mEnabled = true;
    getRenderBackend().setScissor(scissor);
}

bool render_target_texture_allocation_failed()
{
    return getRenderBackend().hasError();
}

U32 get_render_target_max_texture_size()
{
    S32 max_texture_size = gGLManager.mGLMaxTextureSize;
    if (max_texture_size <= 0)
    {
        getRenderBackend().getLegacyInteger(GL_MAX_TEXTURE_SIZE, &max_texture_size);
        if (max_texture_size > 0)
        {
            gGLManager.mGLMaxTextureSize = max_texture_size;
            LL_INFOS_ONCE("RenderBackend")
                << "Render target max texture size initialized from render backend: "
                << max_texture_size
                << "."
                << LL_ENDL;
        }
    }

    if (max_texture_size <= 0)
    {
        LL_WARNS_ONCE("RenderBackend")
            << "Render target max texture size is unavailable; refusing zero-sized render target allocation."
            << LL_ENDL;
        return 0;
    }

    return static_cast<U32>(max_texture_size);
}
}

LLRenderTarget::LLRenderTarget() :
    mResX(0),
    mResY(0),
    mUseDepth(false),
    mUsage(LLTexUnit::TT_TEXTURE)
{
}

LLRenderTarget::~LLRenderTarget()
{
    // Render targets are explicitly released by their owners while the render
    // context and LLImageGL deletion queues are still alive.  During process
    // shutdown, global/static LLRenderTarget destruction can run after those
    // systems have already been torn down, so doing backend work here is not
    // safe.
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
        getRenderBackend().noteTextureAllocation(
            mTex[i],
            mInternalFormat[i],
            mResX,
            mResY);
        LLImageGL::setManualImage(
            to_render_texture_target(mUsage),
            0,
            mInternalFormat[i],
            mResX,
            mResY,
            LLRenderPixelFormat::RGBA,
            LLRenderPixelType::UnsignedByte,
            NULL,
            false);
        sBytesAllocated += pix_diff*4;
    }

    if (mDepth)
    {
        gGL.getTexUnit(0)->bindManual(mUsage, mDepth);
        getRenderBackend().noteTextureAllocation(
            mDepth,
            LLRenderTextureFormat::DepthComponent24,
            mResX,
            mResY);
        LLImageGL::setManualImage(
            to_render_texture_target(mUsage),
            0,
            LLRenderTextureFormat::DepthComponent24,
            mResX,
            mResY,
            LLRenderPixelFormat::DepthComponent,
            LLRenderPixelType::UnsignedInt,
            NULL,
            false);

        sBytesAllocated += pix_diff*4;
    }
}


bool LLRenderTarget::allocate(
    U32 resx,
    U32 resy,
    LLRenderTextureFormat color_fmt,
    bool depth,
    LLTexUnit::eTextureType usage,
    LLTexUnit::eTextureMipGeneration generateMipMaps)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_DISPLAY;
    llassert(usage == LLTexUnit::TT_TEXTURE);
    llassert(!isBoundInStack());

    const U32 max_texture_size = get_render_target_max_texture_size();
    if (!max_texture_size)
    {
        return false;
    }

    resx = llmin(resx, max_texture_size);
    resy = llmin(resy, max_texture_size);

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

    mFBO = generate_framebuffer_handle();

    if (mDepth)
    {
        bind_attachment_fbo(mFBO);

        set_framebuffer_texture_attachment(
            LLRenderFramebufferAttachment::Depth,
            mUsage,
            mDepth,
            LLRenderTextureFormat::DepthComponent24,
            mResX,
            mResY);

        restore_tracked_fbo_binding();
    }

    return addColorAttachment(color_fmt);
}

void LLRenderTarget::setColorAttachment(LLImageGL* img, U32 use_name)
{
    LL_PROFILE_ZONE_SCOPED;
    llassert(img != nullptr); // img must not be null
    llassert(sUseFBO); // FBO support must be enabled
    llassert(!mDepth); // depth buffers not supported with this mode
    llassert(mTex.empty()); // mTex must be empty with this mode (binding target should be done via LLImageGL)
    llassert(!isBoundInStack());

    if (!mFBO)
    {
        mFBO = generate_framebuffer_handle();
    }

    mResX = img->getWidth();
    mResY = img->getHeight();
    mUsage = img->getTarget();

    if (use_name == 0)
    {
        use_name = img->getTexName();
    }

    const LLRenderTextureFormat color_format = infer_render_target_color_format(*img);
    getRenderBackend().noteTextureAllocation(
        LLRenderTextureHandle(use_name),
        color_format,
        mResX,
        mResY);

    mTex.push_back(LLRenderTextureHandle(use_name));

    bind_attachment_fbo(mFBO);
        set_framebuffer_texture_attachment(
            LLRenderFramebufferAttachment::Color0,
            mUsage,
            LLRenderTextureHandle(use_name),
            color_format,
            mResX,
            mResY);
    stop_glerror();

    check_current_draw_framebuffer_status();

    restore_tracked_fbo_binding();
}

void LLRenderTarget::releaseColorAttachment()
{
    LL_PROFILE_ZONE_SCOPED;
    llassert(!isBoundInStack());
    llassert(mTex.size() == 1); //cannot use releaseColorAttachment with LLRenderTarget managed color targets
    llassert(mFBO);  // mFBO must be valid

    bind_attachment_fbo(mFBO);
    clear_framebuffer_texture_attachment(LLRenderFramebufferAttachment::Color0, mUsage);
    restore_tracked_fbo_binding();

    mTex.clear();
}

bool LLRenderTarget::addColorAttachment(LLRenderTextureFormat color_fmt)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_DISPLAY;
    llassert(!isBoundInStack());

    if (color_fmt == LLRenderTextureFormat::None)
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
    if( offset > 0 && !mFBO )
    {
        llassert(mFBO);
        return false;
    }

    U32 texture_name = 0;
    LLImageGL::generateTextures(1, &texture_name);
    LLRenderTextureHandle texture(texture_name);
    gGL.getTexUnit(0)->bindManual(mUsage, texture);
    getRenderBackend().noteTextureAllocation(
        texture,
        color_fmt,
        mResX,
        mResY);

    stop_glerror();


    {
        clear_glerror();
        LLImageGL::setManualImage(
            to_render_texture_target(mUsage),
            0,
            color_fmt,
            mResX,
            mResY,
            LLRenderPixelFormat::RGBA,
            LLRenderPixelType::UnsignedByte,
            NULL,
            false);
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
        set_framebuffer_texture_attachment(
            to_render_color_attachment(offset),
            mUsage,
            texture,
            color_fmt,
            mResX,
            mResY);

        check_current_draw_framebuffer_status();

        restore_tracked_fbo_binding();
    }

    mTex.push_back(texture);
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
    U32 depth_name = 0;
    LLImageGL::generateTextures(1, &depth_name);
    mDepth = LLRenderTextureHandle(depth_name);
    gGL.getTexUnit(0)->bindManual(mUsage, mDepth);
    getRenderBackend().noteTextureAllocation(
        mDepth,
        LLRenderTextureFormat::DepthComponent24,
        mResX,
        mResY);

    stop_glerror();
    clear_glerror();
    LLImageGL::setManualImage(
        to_render_texture_target(mUsage),
        0,
        LLRenderTextureFormat::DepthComponent24,
        mResX,
        mResY,
        LLRenderPixelFormat::DepthComponent,
        LLRenderPixelType::UnsignedInt,
        NULL,
        false);
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

        set_framebuffer_texture_attachment(
            LLRenderFramebufferAttachment::Depth,
            mUsage,
            mDepth,
            LLRenderTextureFormat::DepthComponent24,
            mResX,
            mResY);

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
        U32 depth_name = mDepth.asLegacyName();
        LLImageGL::deleteTextures(1, &depth_name);

        mDepth = LLRenderTextureHandle();

        sBytesAllocated -= mResX*mResY*4;
    }
    else if (mFBO)
    {
        bind_attachment_fbo(mFBO);

        if (mUseDepth)
        { //detach shared depth buffer
            clear_framebuffer_texture_attachment(LLRenderFramebufferAttachment::Depth, mUsage);
            mUseDepth = false;
        }

        restore_tracked_fbo_binding();
    }

    // Detach any extra color buffers (e.g. SRGB spec buffers)
    //
    if (mFBO && (mTex.size() > 1))
    {
        bind_attachment_fbo(mFBO);
        for (size_t z = mTex.size(); z-- > 1;)
        {
            sBytesAllocated -= mResX*mResY*4;
            clear_framebuffer_texture_attachment(to_render_color_attachment(static_cast<U32>(z)), mUsage);
            U32 texture_name = mTex[z].asLegacyName();
            LLImageGL::deleteTextures(1, &texture_name);
        }
        restore_tracked_fbo_binding();
    }

    if (mFBO)
    {
        if (mFBO.asLegacyName() == sCurFBO)
        {
            forget_current_fbo_and_bind_default();
        }

        delete_framebuffer_handle(mFBO);
    }

    if (mTex.size() > 0)
    {
        sBytesAllocated -= mResX*mResY*4;
        U32 texture_name = mTex[0].asLegacyName();
        LLImageGL::deleteTextures(1, &texture_name);
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

void LLRenderTarget::clear(LLRenderClearMask requested_mask)
{
    LL_PROFILE_GPU_ZONE("clear");
    llassert(mFBO);
    LLRenderClearMask mask = LL_RENDER_CLEAR_COLOR;
    if (mUseDepth)
    {
        mask |= LL_RENDER_CLEAR_DEPTH;

    }
    if (mFBO)
    {
        check_current_draw_framebuffer_status();
        stop_glerror();
        clear_render_target_buffers(mask & requested_mask);
        stop_glerror();
    }
    else
    {
        LLGLEnable scissor(LLRenderCapability::ScissorTest);
        set_render_target_scissor(mResX, mResY);
        stop_glerror();
        clear_render_target_buffers(mask & requested_mask);
    }
}

U32 LLRenderTarget::getTexture(U32 attachment) const
{
    return getTextureHandle(attachment).asLegacyName();
}

LLRenderTextureHandle LLRenderTarget::getTextureHandle(U32 attachment) const
{
    if (attachment >= mTex.size())
    {
        LL_WARNS() << "Invalid attachment index " << attachment << " for size " << mTex.size() << LL_ENDL;
        llassert(false);
        return LLRenderTextureHandle();
    }
    return mTex[attachment];
}

U32 LLRenderTarget::getNumTextures() const
{
    return static_cast<U32>(mTex.size());
}

void LLRenderTarget::bindTexture(U32 index, S32 channel, LLTexUnit::eTextureFilterOptions filter_options)
{
    gGL.getTexUnit(channel)->bindManual(
        mUsage,
        getTextureHandle(index),
        filter_options == LLTexUnit::TFO_TRILINEAR || filter_options == LLTexUnit::TFO_ANISOTROPIC,
        true);
    gGL.getTexUnit(channel)->setTextureFilteringOption(filter_options);
}

void LLRenderTarget::flush()
{
    LL_PROFILE_GPU_ZONE("rt flush");
    gGL.flush();
    llassert(mFBO);
    llassert(sCurFBO == mFBO.asLegacyName());
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
    llassert(sCurFBO != mFBO.asLegacyName());
    llassert(sCurFBO != other.mFBO.asLegacyName());
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
