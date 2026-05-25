/**
 * marefsr2upscaler.cpp  –  MARE Phase 3 Step 3: FSR 2 OpenGL compute backend.
 *
 * See marefsr2upscaler.h for architecture notes.
 */

#include "llviewerprecompiledheaders.h"
#include "marefsr2upscaler.h"

#include "llrendertarget.h"

#include "llglslshader.h"       // LLGLSLShader
#include "llviewershadermgr.h"  // gDeferredTAACopyProgram (copy-back helper)
#include "llrender.h"       // gGL
#include "llviewercontrol.h"
#include "pipeline.h"       // gPipeline.mScreenTriangleVB
#include "lldir.h"          // gDirUtilp->getAppRODataDir()

#include <fstream>
#include <sstream>
#include "llrenderstate.h"

// ─────────────────────────────────────────────────────────────────────────────
// Halton low-discrepancy sequence (same as Phase 1 Halton but with FSR 2
// scaling: phase count = ceil(8 * renderW / displayW)).
// ─────────────────────────────────────────────────────────────────────────────

static F32 halton(U32 index, U32 base)
{
    F32 f = 1.f, r = 0.f;
    U32 i = index;
    while (i > 0)
    {
        f /= (F32)base;
        r += f * (F32)(i % base);
        i /= base;
    }
    return r;
}

static void setProgramUniformInteger(LLRenderProgramHandle program, const char* name, S32 value)
{
    getRenderBackend().setUniformInteger(
        getRenderBackend().getUniformLocation(program, name),
        value);
}

static void setProgramUniformInteger2(LLRenderProgramHandle program, const char* name, S32 first, S32 second)
{
    getRenderBackend().setUniformInteger2(
        getRenderBackend().getUniformLocation(program, name),
        first,
        second);
}

static void setProgramUniformFloat(LLRenderProgramHandle program, const char* name, F32 value)
{
    getRenderBackend().setUniformFloat(
        getRenderBackend().getUniformLocation(program, name),
        value);
}

static void setProgramUniformFloat2(LLRenderProgramHandle program, const char* name, F32 first, F32 second)
{
    getRenderBackend().setUniformFloat2(
        getRenderBackend().getUniformLocation(program, name),
        first,
        second);
}

/*static*/
void MAREFSR2Upscaler::computeJitter(
    U32 frameIndex,
    U32 renderW, U32 displayW,
    U32 renderH, U32 displayH,
    F32& outX, F32& outY)
{
    // FSR 2 uses a phase count of ceil(8 * renderW / displayW).
    U32 phaseCount = (U32)std::ceil(8.0 * (double)renderW / (double)displayW);
    U32 phase      = frameIndex % phaseCount;

    // Halton(2,3) in pixel space [−0.5, 0.5]
    F32 pxX = halton(phase + 1, 2) - 0.5f;
    F32 pxY = halton(phase + 1, 3) - 0.5f;

    // Scale to UV space (NDC-style: 1 pixel = 2/renderW in NDC, 1/renderW in UV)
    outX = pxX / (F32)renderW;
    outY = pxY / (F32)renderH;
}

// ─────────────────────────────────────────────────────────────────────────────
// Shader loading
// ─────────────────────────────────────────────────────────────────────────────

LLRenderProgramHandle MAREFSR2Upscaler::compileComputeProgram(const std::string& relPath)
{
    // Build absolute path via LL_PATH_APP_SETTINGS so the path resolves to
    // Release/app_settings/shaders/class1/deferred/<relPath> in dev builds.
    // (getAppRODataDir() alone omits the "app_settings/" component.)
    std::string absPath = gDirUtilp->getExpandedFilename(LL_PATH_APP_SETTINGS,
                            "shaders/class1/deferred/" + relPath);

    std::ifstream f(absPath);
    if (!f.is_open())
    {
        LL_WARNS() << "MAREFSR2: cannot open shader: " << absPath << LL_ENDL;
        return LLRenderProgramHandle();
    }
    std::ostringstream ss;
    ss << f.rdbuf();
    std::string src = ss.str();

    const char* csrc = src.c_str();
    LLRenderShaderHandle shader = getRenderBackend().createShaderHandle(LLRenderShaderStage::Compute);
    getRenderBackend().setShaderSource(shader, 1, &csrc);
    getRenderBackend().compileShader(shader);

    S32 ok = 0;
    getRenderBackend().getShaderInteger(
        shader,
        LLRenderShaderParameter::CompileStatus,
        &ok);
    if (!ok)
    {
        char log[2048];
        getRenderBackend().getShaderInfoLog(shader, sizeof(log), nullptr, log);
        LL_WARNS() << "MAREFSR2: compute shader compile error (" << relPath << "):\n" << log << LL_ENDL;
        getRenderBackend().deleteShader(shader);
        return LLRenderProgramHandle();
    }

    LLRenderProgramHandle prog = getRenderBackend().createProgramHandle();
    getRenderBackend().attachShader(prog, shader);
    getRenderBackend().linkProgram(prog);
    getRenderBackend().deleteShader(shader);  // shader is now owned by the program

    S32 linked = 0;
    getRenderBackend().getProgramInteger(
        prog,
        LLRenderProgramParameter::LinkStatus,
        &linked);
    if (!linked)
    {
        char log[2048];
        getRenderBackend().getProgramInfoLog(prog, sizeof(log), nullptr, log);
        LL_WARNS() << "MAREFSR2: compute program link error (" << relPath << "):\n" << log << LL_ENDL;
        getRenderBackend().deleteProgram(prog);
        return LLRenderProgramHandle();
    }

    return prog;
}

// ─────────────────────────────────────────────────────────────────────────────
// Internal texture helpers
// ─────────────────────────────────────────────────────────────────────────────

LLRenderTextureHandle MAREFSR2Upscaler::createTexture2D(U32 w, U32 h, LLRenderTextureFormat internalFmt)
{
    return getRenderBackend().createTexture2D(internalFmt, w, h);
}

void MAREFSR2Upscaler::deleteTexture(LLRenderTextureHandle& tex)
{
    if (tex)
    {
        getRenderBackend().deleteTextureHandle(tex);
        tex = LLRenderTextureHandle();
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Lifecycle
// ─────────────────────────────────────────────────────────────────────────────

bool MAREFSR2Upscaler::initialize(U32 renderW, U32 renderH)
{
    mRenderW  = renderW;
    mRenderH  = renderH;
    mReady    = false;

    // Compile / link all 5 compute passes.
    mProgDepthClip      = compileComputeProgram("fsr2/fsr2_depth_clip.comp.glsl");
    mProgReconPrevDepth = compileComputeProgram("fsr2/fsr2_reconstruct_prev_depth.comp.glsl");
    mProgLock           = compileComputeProgram("fsr2/fsr2_lock.comp.glsl");
    mProgAccumulate     = compileComputeProgram("fsr2/fsr2_accumulate.comp.glsl");
    mProgRCAS           = compileComputeProgram("fsr2/fsr2_rcas.comp.glsl");

    if (!mProgDepthClip || !mProgReconPrevDepth || !mProgLock
        || !mProgAccumulate || !mProgRCAS)
    {
        LL_WARNS() << "MAREFSR2: one or more compute shaders failed to compile." << LL_ENDL;
        destroy();
        return false;
    }

    // Allocate render-res internal textures.
    mDilatedDepth   = createTexture2D(renderW, renderH, LLRenderTextureFormat::R32F);
    mDilatedMV      = createTexture2D(renderW, renderH, LLRenderTextureFormat::RG32F);
    mPrevDepth      = createTexture2D(renderW, renderH, LLRenderTextureFormat::R32F);
    mReconPrevDepth = createTexture2D(renderW, renderH, LLRenderTextureFormat::R32F);
    mLockStatus     = createTexture2D(renderW, renderH, LLRenderTextureFormat::R8);

    // Display-res buffers will be allocated in apply() on first call once we
    // know outputDst dimensions.  Mark them zero for now.
    mAccumBuffer[0] = LLRenderTextureHandle();
    mAccumBuffer[1] = LLRenderTextureHandle();
    mRCASBuffer = LLRenderTextureHandle();
    mDisplayW = mDisplayH = 0;
    mAccumIdx = 0;

    mReady = true;
    return true;
}

void MAREFSR2Upscaler::destroy()
{
    if (mProgDepthClip)      { getRenderBackend().deleteProgram(mProgDepthClip);      mProgDepthClip      = LLRenderProgramHandle(); }
    if (mProgReconPrevDepth) { getRenderBackend().deleteProgram(mProgReconPrevDepth); mProgReconPrevDepth = LLRenderProgramHandle(); }
    if (mProgLock)           { getRenderBackend().deleteProgram(mProgLock);           mProgLock           = LLRenderProgramHandle(); }
    if (mProgAccumulate)     { getRenderBackend().deleteProgram(mProgAccumulate);     mProgAccumulate     = LLRenderProgramHandle(); }
    if (mProgRCAS)           { getRenderBackend().deleteProgram(mProgRCAS);           mProgRCAS           = LLRenderProgramHandle(); }

    deleteTexture(mDilatedDepth);
    deleteTexture(mDilatedMV);
    deleteTexture(mPrevDepth);
    deleteTexture(mReconPrevDepth);
    deleteTexture(mLockStatus);
    deleteTexture(mAccumBuffer[0]);
    deleteTexture(mAccumBuffer[1]);
    deleteTexture(mRCASBuffer);

    mRenderW = mRenderH = mDisplayW = mDisplayH = 0;
    mAccumIdx = 0;
    mReady = false;
}

// ─────────────────────────────────────────────────────────────────────────────
// Per-frame: apply()
// ─────────────────────────────────────────────────────────────────────────────

void MAREFSR2Upscaler::apply(
    LLRenderTarget* colorSrc,
    LLRenderTarget* depthSrc,
    LLRenderTarget* velocitySrc,
    LLRenderTarget* outputDst,
    F32  jitterX,
    F32  jitterY,
    bool cameraCut)
{
    if (!mReady) return;
    if (!colorSrc || !velocitySrc || !outputDst) return;

    const U32 rW = mRenderW;
    const U32 rH = mRenderH;
    const U32 dW = (U32)outputDst->getWidth();
    const U32 dH = (U32)outputDst->getHeight();

    // Lazy-allocate display-res buffers on first call or after resize.
    if (dW != mDisplayW || dH != mDisplayH)
    {
        deleteTexture(mAccumBuffer[0]);
        deleteTexture(mAccumBuffer[1]);
        deleteTexture(mRCASBuffer);

        mAccumBuffer[0] = createTexture2D(dW, dH, LLRenderTextureFormat::RGBA16F);
        mAccumBuffer[1] = createTexture2D(dW, dH, LLRenderTextureFormat::RGBA16F);
        mRCASBuffer     = createTexture2D(dW, dH, LLRenderTextureFormat::RGBA16F);

        mDisplayW = dW;
        mDisplayH = dH;
        mAccumIdx = 0;
    }

    // Dispatch group size: 8×8 threads per group.
    auto groups = [](U32 n) -> U32 { return (n + 7) / 8; };

    U32 histIdx = mAccumIdx;
    U32 outIdx  = 1u - mAccumIdx;

    // Retrieve backend texture handles.
    LLRenderTextureHandle colorTex = colorSrc->getTextureHandle();
    LLRenderTextureHandle depthTex = depthSrc ? depthSrc->getTextureHandle() : LLRenderTextureHandle();
    LLRenderTextureHandle velocityTex = velocitySrc->getTextureHandle();

    // ── Pass 1: Depth Clip / Dilate ───────────────────────────────────────────
    // Inputs:  u_depth (tex 0), u_motionVec (tex 1)
    // Outputs: u_dilatedDepth (image 2), u_dilatedMV (image 3)
    {
        getRenderBackend().useProgram(mProgDepthClip);
        setProgramUniformInteger2(mProgDepthClip, "u_renderSize", (S32)rW, (S32)rH);

        getRenderBackend().bindTextureUnit(0, depthTex);
        getRenderBackend().bindTextureUnit(1, velocityTex);
        getRenderBackend().bindImageTexture(
            2, mDilatedDepth, 0, false, 0, LLRenderImageAccess::WriteOnly, LLRenderTextureFormat::R32F);
        getRenderBackend().bindImageTexture(
            3, mDilatedMV, 0, false, 0, LLRenderImageAccess::WriteOnly, LLRenderTextureFormat::RG32F);

        getRenderBackend().dispatchCompute(groups(rW), groups(rH), 1);
        getRenderBackend().setMemoryBarrier(
            LL_RENDER_MEMORY_BARRIER_SHADER_IMAGE_ACCESS |
            LL_RENDER_MEMORY_BARRIER_TEXTURE_FETCH);
    }

    // ── Pass 2: Reconstruct Previous Depth ────────────────────────────────────
    // Inputs:  u_dilatedMV (tex 0), u_prevDepth (tex 1)
    // Output:  u_reconPrevDepth (image 2)
    {
        getRenderBackend().useProgram(mProgReconPrevDepth);
        setProgramUniformInteger2(mProgReconPrevDepth, "u_renderSize", (S32)rW, (S32)rH);

        getRenderBackend().bindTextureUnit(0, mDilatedMV);
        getRenderBackend().bindTextureUnit(1, mPrevDepth);
        getRenderBackend().bindImageTexture(
            2, mReconPrevDepth, 0, false, 0, LLRenderImageAccess::WriteOnly, LLRenderTextureFormat::R32F);

        getRenderBackend().dispatchCompute(groups(rW), groups(rH), 1);
        getRenderBackend().setMemoryBarrier(
            LL_RENDER_MEMORY_BARRIER_SHADER_IMAGE_ACCESS |
            LL_RENDER_MEMORY_BARRIER_TEXTURE_FETCH);
    }

    // ── Pass 3: Lock ──────────────────────────────────────────────────────────
    // Inputs:  u_color (tex 0), u_prevColor/history (tex 1),
    //          u_dilatedDepth (tex 2), u_reconPrevDepth (tex 3)
    // Output:  u_lockStatus (image 4)
    {
        getRenderBackend().useProgram(mProgLock);
        setProgramUniformInteger2(mProgLock, "u_renderSize", (S32)rW, (S32)rH);

        getRenderBackend().bindTextureUnit(0, colorTex);
        getRenderBackend().bindTextureUnit(1, mAccumBuffer[histIdx]);
        getRenderBackend().bindTextureUnit(2, mDilatedDepth);
        getRenderBackend().bindTextureUnit(3, mReconPrevDepth);
        getRenderBackend().bindImageTexture(
            4, mLockStatus, 0, false, 0, LLRenderImageAccess::WriteOnly, LLRenderTextureFormat::R8);

        getRenderBackend().dispatchCompute(groups(rW), groups(rH), 1);
        getRenderBackend().setMemoryBarrier(
            LL_RENDER_MEMORY_BARRIER_SHADER_IMAGE_ACCESS |
            LL_RENDER_MEMORY_BARRIER_TEXTURE_FETCH);
    }

    // ── Pass 4: Temporal Accumulation ─────────────────────────────────────────
    // Inputs:  u_color (tex 0), u_prevAccum (tex 1),
    //          u_dilatedMV (tex 2), u_lockStatus (tex 3)
    // Output:  u_output mAccumBuffer[outIdx] (image 4, display res)
    {
        static U32 sFrameIndex = 0;
        ++sFrameIndex;

        getRenderBackend().useProgram(mProgAccumulate);
        setProgramUniformInteger2(mProgAccumulate, "u_renderSize",  (S32)rW, (S32)rH);
        setProgramUniformInteger2(mProgAccumulate, "u_displaySize", (S32)dW, (S32)dH);
        setProgramUniformFloat2(mProgAccumulate, "u_jitter", jitterX, jitterY);
        setProgramUniformInteger(mProgAccumulate, "u_cameraCut", cameraCut ? 1 : 0);
        setProgramUniformInteger(mProgAccumulate, "u_frameIndex", (S32)sFrameIndex);

        getRenderBackend().bindTextureUnit(0, colorTex);
        getRenderBackend().bindTextureUnit(1, mAccumBuffer[histIdx]);
        getRenderBackend().bindTextureUnit(2, mDilatedMV);
        getRenderBackend().bindTextureUnit(3, mLockStatus);
        getRenderBackend().bindImageTexture(
            4, mAccumBuffer[outIdx], 0, false, 0, LLRenderImageAccess::WriteOnly, LLRenderTextureFormat::RGBA16F);

        getRenderBackend().dispatchCompute(groups(dW), groups(dH), 1);
        getRenderBackend().setMemoryBarrier(
            LL_RENDER_MEMORY_BARRIER_SHADER_IMAGE_ACCESS |
            LL_RENDER_MEMORY_BARRIER_TEXTURE_FETCH);

        mAccumIdx = outIdx;  // swap ping-pong for next frame
    }

    // ── Pass 5: RCAS Sharpening ───────────────────────────────────────────────
    // Input:  u_accum (tex 0) = mAccumBuffer[outIdx]
    // Output: u_output = mRCASBuffer (image 1, display res)
    {
        static LLCachedControl<F32> sharpness(gSavedSettings, "RenderNISSharpenStrength", 0.5f);

        getRenderBackend().useProgram(mProgRCAS);
        setProgramUniformInteger2(mProgRCAS, "u_displaySize", (S32)dW, (S32)dH);
        setProgramUniformFloat(mProgRCAS, "u_sharpness", (F32)sharpness);

        getRenderBackend().bindTextureUnit(0, mAccumBuffer[outIdx]);
        getRenderBackend().bindImageTexture(
            1, mRCASBuffer, 0, false, 0, LLRenderImageAccess::WriteOnly, LLRenderTextureFormat::RGBA16F);

        getRenderBackend().dispatchCompute(groups(dW), groups(dH), 1);
        getRenderBackend().setMemoryBarrier(
            LL_RENDER_MEMORY_BARRIER_SHADER_IMAGE_ACCESS |
            LL_RENDER_MEMORY_BARRIER_TEXTURE_FETCH);
    }

    // ── Copy mRCASBuffer → outputDst (mDisplayScreen) ─────────────────────────
    // Reuse gDeferredTAACopyProgram (mareUpscaleV.glsl + mareCopyF.glsl).
    if (gDeferredTAACopyProgram.mProgramObject && gPipeline.mScreenTriangleVB)
    {
        outputDst->bindTarget();
        gDeferredTAACopyProgram.bind();

        static LLStaticHashedString sCopyMap("colorMap");
        gDeferredTAACopyProgram.uniform1i(sCopyMap, 0);
        gGL.getTexUnit(0)->bindManual(LLTexUnit::TT_TEXTURE, mRCASBuffer);

        {
            LLGLDisable   blend(LLRenderCapability::Blend);
            LLGLDepthTest depth(false);
            gPipeline.mScreenTriangleVB->setBuffer();
            gPipeline.mScreenTriangleVB->drawArrays(LLRender::TRIANGLES, 0, 3);
        }

        gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);
        gDeferredTAACopyProgram.unbind();
        outputDst->flush();
    }

    // ── Save current depth for next frame's reconstruct pass ──────────────────
    if (depthTex)
    {
        // Blit current depth into mPrevDepth via a simple copy image call.
        getRenderBackend().copyImageSubData(
            depthTex,    LLRenderTextureTarget::Texture2D, 0, 0, 0, 0,
            mPrevDepth,  LLRenderTextureTarget::Texture2D, 0, 0, 0, 0,
            rW, rH, 1);
    }

    // Unbind compute program.
    getRenderBackend().useProgram(LLRenderProgramHandle());
}
