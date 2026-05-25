/**
 * maretaaupscaler.cpp  –  MARE Phase 3 Step 1: internal OpenGL TAA backend.
 *
 * See maretaaupscaler.h for algorithm notes.
 */

#include "llviewerprecompiledheaders.h"
#include "maretaaupscaler.h"
#include "marenisupscaler.h"    // MARENISUpscaler, MARETAANISUpscaler
#if MARE_ENABLE_FSR2
#include "marefsr2upscaler.h"   // MAREFSR2Upscaler — Phase 3 Step 3
#endif

#include "pipeline.h"           // gPipeline.mScreenTriangleVB
#include "llviewershadermgr.h"  // gDeferredTAAProgram, gDeferredTAACopyProgram
#include "llrendertarget.h"
#include "llrender.h"           // gGL
#include "llrenderbackend.h"
#include "llglslshader.h"       // LLGLSLProgram
#include "llviewercontrol.h"    // gSavedSettings, LLCachedControl
#include "llrenderstate.h"

// ── IUpscaler factory ─────────────────────────────────────────────────────────
// Defined here so that only maretaaupscaler.cpp (and its includes) need linking.
// RenderUpscalerMode: 0 = TAA, 1 = NIS, 2 = TAA+NIS, 3 = FSR 2 when enabled.

std::unique_ptr<IUpscaler> IUpscaler::create()
{
    static LLCachedControl<U32> mode(gSavedSettings, "RenderUpscalerMode", 0u);
    switch ((U32)mode)
    {
        case 1:  return std::make_unique<MARENISUpscaler>();
        case 2:  return std::make_unique<MARETAANISUpscaler>();
#if MARE_ENABLE_FSR2
        case 3:  return std::make_unique<MAREFSR2Upscaler>();  // FSR 2 compute pipeline
#endif
        default: return std::make_unique<MARETAAUpscaler>();   // 0 = TAA only
    }
}

// ── lifecycle ─────────────────────────────────────────────────────────────────

bool MARETAAUpscaler::initialize(U32 renderW, U32 renderH)
{
    mWidth  = renderW;
    mHeight = renderH;
    mAccumIndex = 0;

    // Allocate two ping-pong accumulation buffers in HDR float format.
    // No depth attachment required — this is a full-screen 2-D pass.
    if (!mAccumBuffer[0].allocate(renderW, renderH, LLRenderTextureFormat::RGBA16F)) return false;
    if (!mAccumBuffer[1].allocate(renderW, renderH, LLRenderTextureFormat::RGBA16F)) return false;

    // MARE: Clear accumulation buffers to black so the very first apply() blends
    // a known-good (black) history rather than uninitialised GPU memory.
    // gGL.flush() inside target.flush() is safe here because initialise is called
    // before any batched gGL draw commands are issued for the upscaler (mCount == 0).
    for (int i = 0; i < 2; ++i)
    {
        mAccumBuffer[i].bindTarget();
        getRenderBackend().setClearColor(0.f, 0.f, 0.f, 0.f);
        getRenderBackend().clear(LL_RENDER_CLEAR_COLOR);
        mAccumBuffer[i].flush();
    }

    mFirstFrame = true;  // reset so apply() seeds the history from the live frame
    return true;
}

void MARETAAUpscaler::destroy()
{
    mAccumBuffer[0].release();
    mAccumBuffer[1].release();
    mWidth  = 0;
    mHeight = 0;
}

// ── apply (per-frame TAA pass) ────────────────────────────────────────────────

void MARETAAUpscaler::apply(
    LLRenderTarget* colorSrc,
    LLRenderTarget* /*depthSrc*/,   // ignored by TAA
    LLRenderTarget* velocitySrc,
    LLRenderTarget* outputDst,
    F32 jitterX,
    F32 jitterY,
    bool cameraCut)
{
    if (!isReady()) return;
    if (!gDeferredTAAProgram.mProgramObject)     return; // shaders not loaded
    if (!gDeferredTAACopyProgram.mProgramObject) return;

    // MARE: On the very first apply() after (re)initialise, treat as a camera cut
    // so the TAA shader outputs the raw current frame rather than blending in the
    // cleared (black) history.  This gives the accumulation a valid seed immediately.
    bool doCut = mFirstFrame || cameraCut;
    mFirstFrame = false;

    U32 histIdx = mAccumIndex;
    U32 outIdx  = 1u - mAccumIndex;

    LLVertexBuffer* triVB = gPipeline.mScreenTriangleVB;
    if (!triVB) return;

    // ── Pass 1: TAA accumulation ──────────────────────────────────────────────
    // Reads  : colorSrc  (current jittered frame,  tex unit 0)
    //          history   (previous accumulated,     tex unit 1)
    //          velocity  (NDC motion vectors,       tex unit 2)
    // Writes : mAccumBuffer[outIdx]  (new accumulated frame)
    {
        mAccumBuffer[outIdx].bindTarget();

        gDeferredTAAProgram.bind();

        // Sampler uniforms — tell each sampler2D which texture unit to use.
        static LLStaticHashedString sColorMap  ("colorMap");
        static LLStaticHashedString sHistoryMap("historyMap");
        static LLStaticHashedString sVelocityMap("velocityMap");
        static LLStaticHashedString sJitter    ("jitter");
        static LLStaticHashedString sCameraCut ("cameraCut");

        gDeferredTAAProgram.uniform1i(sColorMap,    0);
        gDeferredTAAProgram.uniform1i(sHistoryMap,  1);
        gDeferredTAAProgram.uniform1i(sVelocityMap, 2);
        gDeferredTAAProgram.uniform2f(sJitter, jitterX, jitterY);
        gDeferredTAAProgram.uniform1i(sCameraCut, doCut ? 1 : 0);

        // Bind textures to their respective units.
        gGL.getTexUnit(0)->bindManual(LLTexUnit::TT_TEXTURE, colorSrc->getTextureHandle());
        gGL.getTexUnit(1)->bindManual(LLTexUnit::TT_TEXTURE, mAccumBuffer[histIdx].getTextureHandle());
        gGL.getTexUnit(2)->bindManual(LLTexUnit::TT_TEXTURE, velocitySrc->getTextureHandle());

        {
            LLGLDisable   blend(LLRenderCapability::Blend);
            LLGLDepthTest depth(false);
            triVB->setBuffer();
            triVB->drawArrays(LLRender::TRIANGLES, 0, 3);
        }

        gGL.getTexUnit(2)->unbind(LLTexUnit::TT_TEXTURE);
        gGL.getTexUnit(1)->unbind(LLTexUnit::TT_TEXTURE);
        gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);
        gGL.flush();  // MARE: drain pending gGL state while shader is still bound

        gDeferredTAAProgram.unbind();
        mAccumBuffer[outIdx].flush();
    }

    // ── Pass 2: copy TAA result → outputDst ──────────────────────────────────
    // The downstream post-processing chain reads from the screen render target
    // (outputDst), so we blit the new accumulated frame back into it here.
    // colorSrc == outputDst is explicitly allowed: the TAA pass above already
    // completed and wrote to a DIFFERENT FBO (mAccumBuffer[outIdx]), so
    // overwriting outputDst's FBO now does not conflict.
    {
        outputDst->bindTarget();

        gDeferredTAACopyProgram.bind();

        static LLStaticHashedString sCopyMap("colorMap");
        gDeferredTAACopyProgram.uniform1i(sCopyMap, 0);

        gGL.getTexUnit(0)->bindManual(LLTexUnit::TT_TEXTURE, mAccumBuffer[outIdx].getTextureHandle());

        {
            LLGLDisable   blend(LLRenderCapability::Blend);
            LLGLDepthTest depth(false);
            triVB->setBuffer();
            triVB->drawArrays(LLRender::TRIANGLES, 0, 3);
        }

        gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);
        gGL.flush();  // MARE: drain pending gGL state while shader is still bound

        gDeferredTAACopyProgram.unbind();
        outputDst->flush();
    }

    // Advance ping-pong index so next frame reads what we just wrote.
    mAccumIndex = outIdx;
}
