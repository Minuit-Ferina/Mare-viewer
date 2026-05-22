/**
 * marefsr2upscaler.cpp  –  MARE Phase 3 Step 3: FSR 2 OpenGL compute backend.
 *
 * See marefsr2upscaler.h for architecture notes.
 */

#include "llviewerprecompiledheaders.h"
#include "marefsr2upscaler.h"

#include "llrendertarget.h"
#include "llgl.h"
#include "llglcontainment.h"
#include "llglheaders.h"
#include "llglslshader.h"       // LLGLSLShader
#include "llviewershadermgr.h"  // gDeferredTAACopyProgram (copy-back helper)
#include "llrender.h"       // gGL
#include "llviewercontrol.h"
#include "pipeline.h"       // gPipeline.mScreenTriangleVB
#include "lldir.h"          // gDirUtilp->getAppRODataDir()

#include <fstream>
#include <sstream>

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

static void setProgramUniformInteger(GLuint program, const char* name, GLint value)
{
    LLGLContainment::setUniformInteger(LLGLContainment::getUniformLocation(program, name), value);
}

static void setProgramUniformInteger2(GLuint program, const char* name, GLint first, GLint second)
{
    LLGLContainment::setUniformInteger2(LLGLContainment::getUniformLocation(program, name), first, second);
}

static void setProgramUniformFloat(GLuint program, const char* name, GLfloat value)
{
    LLGLContainment::setUniformFloat(LLGLContainment::getUniformLocation(program, name), value);
}

static void setProgramUniformFloat2(GLuint program, const char* name, GLfloat first, GLfloat second)
{
    LLGLContainment::setUniformFloat2(LLGLContainment::getUniformLocation(program, name), first, second);
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

LLGLuint MAREFSR2Upscaler::compileComputeProgram(const std::string& relPath)
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
        return 0;
    }
    std::ostringstream ss;
    ss << f.rdbuf();
    std::string src = ss.str();

    const char* csrc = src.c_str();
    GLuint shader = LLGLContainment::createShader(GL_COMPUTE_SHADER);
    LLGLContainment::setShaderSource(shader, 1, &csrc);
    LLGLContainment::compileShader(shader);

    GLint ok = 0;
    LLGLContainment::getShaderInteger(shader, GL_COMPILE_STATUS, &ok);
    if (!ok)
    {
        GLchar log[2048];
        LLGLContainment::getShaderInfoLog(shader, sizeof(log), nullptr, log);
        LL_WARNS() << "MAREFSR2: compute shader compile error (" << relPath << "):\n" << log << LL_ENDL;
        LLGLContainment::deleteShader(shader);
        return 0;
    }

    GLuint prog = LLGLContainment::createProgram();
    LLGLContainment::attachShader(prog, shader);
    LLGLContainment::linkProgram(prog);
    LLGLContainment::deleteShader(shader);  // shader is now owned by the program

    GLint linked = 0;
    LLGLContainment::getProgramInteger(prog, GL_LINK_STATUS, &linked);
    if (!linked)
    {
        GLchar log[2048];
        LLGLContainment::getProgramInfoLog(prog, sizeof(log), nullptr, log);
        LL_WARNS() << "MAREFSR2: compute program link error (" << relPath << "):\n" << log << LL_ENDL;
        LLGLContainment::deleteProgram(prog);
        return 0;
    }

    return prog;
}

// ─────────────────────────────────────────────────────────────────────────────
// Internal texture helpers
// ─────────────────────────────────────────────────────────────────────────────

LLGLuint MAREFSR2Upscaler::createTexture2D(U32 w, U32 h, LLGLenum internalFmt)
{
    GLuint tex;
    LLGLContainment::createTextures(GL_TEXTURE_2D, 1, &tex);
    LLGLContainment::setTextureStorage2D(tex, 1, internalFmt, w, h);
    LLGLContainment::setNamedTextureParameterInteger(tex, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    LLGLContainment::setNamedTextureParameterInteger(tex, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    LLGLContainment::setNamedTextureParameterInteger(tex, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    LLGLContainment::setNamedTextureParameterInteger(tex, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    return tex;
}

void MAREFSR2Upscaler::deleteTexture(LLGLuint& tex)
{
    if (tex) { LLGLContainment::deleteTextures(1, &tex); tex = 0; }
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
    mDilatedDepth   = createTexture2D(renderW, renderH, GL_R32F);
    mDilatedMV      = createTexture2D(renderW, renderH, GL_RG32F);
    mPrevDepth      = createTexture2D(renderW, renderH, GL_R32F);
    mReconPrevDepth = createTexture2D(renderW, renderH, GL_R32F);
    mLockStatus     = createTexture2D(renderW, renderH, GL_R8);

    // Display-res buffers will be allocated in apply() on first call once we
    // know outputDst dimensions.  Mark them zero for now.
    mAccumBuffer[0] = mAccumBuffer[1] = mRCASBuffer = 0;
    mDisplayW = mDisplayH = 0;
    mAccumIdx = 0;

    mReady = true;
    return true;
}

void MAREFSR2Upscaler::destroy()
{
    if (mProgDepthClip)      { LLGLContainment::deleteProgram(mProgDepthClip);      mProgDepthClip      = 0; }
    if (mProgReconPrevDepth) { LLGLContainment::deleteProgram(mProgReconPrevDepth); mProgReconPrevDepth = 0; }
    if (mProgLock)           { LLGLContainment::deleteProgram(mProgLock);           mProgLock           = 0; }
    if (mProgAccumulate)     { LLGLContainment::deleteProgram(mProgAccumulate);     mProgAccumulate     = 0; }
    if (mProgRCAS)           { LLGLContainment::deleteProgram(mProgRCAS);           mProgRCAS           = 0; }

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

        mAccumBuffer[0] = createTexture2D(dW, dH, GL_RGBA16F);
        mAccumBuffer[1] = createTexture2D(dW, dH, GL_RGBA16F);
        mRCASBuffer     = createTexture2D(dW, dH, GL_RGBA16F);

        mDisplayW = dW;
        mDisplayH = dH;
        mAccumIdx = 0;
    }

    // Dispatch group size: 8×8 threads per group.
    auto groups = [](U32 n) -> GLuint { return (n + 7) / 8; };

    GLuint histIdx = mAccumIdx;
    GLuint outIdx  = 1u - mAccumIdx;

    // Retrieve GL texture handles.
    GLuint colorTex    = colorSrc->getTexture();
    GLuint depthTex    = depthSrc ? depthSrc->getTexture() : 0;
    GLuint velocityTex = velocitySrc->getTexture();

    // ── Pass 1: Depth Clip / Dilate ───────────────────────────────────────────
    // Inputs:  u_depth (tex 0), u_motionVec (tex 1)
    // Outputs: u_dilatedDepth (image 2), u_dilatedMV (image 3)
    {
        LLGLContainment::useProgram(mProgDepthClip);
        setProgramUniformInteger2(mProgDepthClip, "u_renderSize", (GLint)rW, (GLint)rH);

        LLGLContainment::bindTextureUnit(0, depthTex ? depthTex : 0);
        LLGLContainment::bindTextureUnit(1, velocityTex);
        LLGLContainment::bindImageTexture(2, mDilatedDepth, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_R32F);
        LLGLContainment::bindImageTexture(3, mDilatedMV,    0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RG32F);

        LLGLContainment::dispatchCompute(groups(rW), groups(rH), 1);
        LLGLContainment::setMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT);
    }

    // ── Pass 2: Reconstruct Previous Depth ────────────────────────────────────
    // Inputs:  u_dilatedMV (tex 0), u_prevDepth (tex 1)
    // Output:  u_reconPrevDepth (image 2)
    {
        LLGLContainment::useProgram(mProgReconPrevDepth);
        setProgramUniformInteger2(mProgReconPrevDepth, "u_renderSize", (GLint)rW, (GLint)rH);

        LLGLContainment::bindTextureUnit(0, mDilatedMV);
        LLGLContainment::bindTextureUnit(1, mPrevDepth);
        LLGLContainment::bindImageTexture(2, mReconPrevDepth, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_R32F);

        LLGLContainment::dispatchCompute(groups(rW), groups(rH), 1);
        LLGLContainment::setMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT);
    }

    // ── Pass 3: Lock ──────────────────────────────────────────────────────────
    // Inputs:  u_color (tex 0), u_prevColor/history (tex 1),
    //          u_dilatedDepth (tex 2), u_reconPrevDepth (tex 3)
    // Output:  u_lockStatus (image 4)
    {
        LLGLContainment::useProgram(mProgLock);
        setProgramUniformInteger2(mProgLock, "u_renderSize", (GLint)rW, (GLint)rH);

        LLGLContainment::bindTextureUnit(0, colorTex);
        LLGLContainment::bindTextureUnit(1, mAccumBuffer[histIdx]);
        LLGLContainment::bindTextureUnit(2, mDilatedDepth);
        LLGLContainment::bindTextureUnit(3, mReconPrevDepth);
        LLGLContainment::bindImageTexture(4, mLockStatus, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_R8);

        LLGLContainment::dispatchCompute(groups(rW), groups(rH), 1);
        LLGLContainment::setMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT);
    }

    // ── Pass 4: Temporal Accumulation ─────────────────────────────────────────
    // Inputs:  u_color (tex 0), u_prevAccum (tex 1),
    //          u_dilatedMV (tex 2), u_lockStatus (tex 3)
    // Output:  u_output mAccumBuffer[outIdx] (image 4, display res)
    {
        static U32 sFrameIndex = 0;
        ++sFrameIndex;

        LLGLContainment::useProgram(mProgAccumulate);
        setProgramUniformInteger2(mProgAccumulate, "u_renderSize",  (GLint)rW, (GLint)rH);
        setProgramUniformInteger2(mProgAccumulate, "u_displaySize", (GLint)dW, (GLint)dH);
        setProgramUniformFloat2(mProgAccumulate, "u_jitter", jitterX, jitterY);
        setProgramUniformInteger(mProgAccumulate, "u_cameraCut", cameraCut ? 1 : 0);
        setProgramUniformInteger(mProgAccumulate, "u_frameIndex", (GLint)sFrameIndex);

        LLGLContainment::bindTextureUnit(0, colorTex);
        LLGLContainment::bindTextureUnit(1, mAccumBuffer[histIdx]);
        LLGLContainment::bindTextureUnit(2, mDilatedMV);
        LLGLContainment::bindTextureUnit(3, mLockStatus);
        LLGLContainment::bindImageTexture(4, mAccumBuffer[outIdx], 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA16F);

        LLGLContainment::dispatchCompute(groups(dW), groups(dH), 1);
        LLGLContainment::setMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT);

        mAccumIdx = outIdx;  // swap ping-pong for next frame
    }

    // ── Pass 5: RCAS Sharpening ───────────────────────────────────────────────
    // Input:  u_accum (tex 0) = mAccumBuffer[outIdx]
    // Output: u_output = mRCASBuffer (image 1, display res)
    {
        static LLCachedControl<F32> sharpness(gSavedSettings, "RenderNISSharpenStrength", 0.5f);

        LLGLContainment::useProgram(mProgRCAS);
        setProgramUniformInteger2(mProgRCAS, "u_displaySize", (GLint)dW, (GLint)dH);
        setProgramUniformFloat(mProgRCAS, "u_sharpness", (F32)sharpness);

        LLGLContainment::bindTextureUnit(0, mAccumBuffer[outIdx]);
        LLGLContainment::bindImageTexture(1, mRCASBuffer, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA16F);

        LLGLContainment::dispatchCompute(groups(dW), groups(dH), 1);
        LLGLContainment::setMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT);
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
            LLGLDisable   blend(GL_BLEND);
            LLGLDepthTest depth(GL_FALSE);
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
        LLGLContainment::copyImageSubData(
            depthTex,    GL_TEXTURE_2D, 0, 0, 0, 0,
            mPrevDepth,  GL_TEXTURE_2D, 0, 0, 0, 0,
            rW, rH, 1);
    }

    // Unbind compute program.
    LLGLContainment::useProgram(0);
}
