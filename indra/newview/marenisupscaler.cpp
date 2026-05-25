/**
 * marenisupscaler.cpp  –  MARE Phase 3 Step 2: NIS adaptive sharpening backend.
 *
 * See marenisupscaler.h for algorithm notes.
 */

#include "llviewerprecompiledheaders.h"
#include "marenisupscaler.h"

#include "pipeline.h"           // gPipeline.mScreenTriangleVB
#include "llviewershadermgr.h"  // gDeferredNISProgram, gDeferredTAACopyProgram
#include "llrendertarget.h"
#include "llrender.h"           // gGL
#include "llglslshader.h"
#include "llviewercontrol.h"    // gSavedSettings, LLCachedControl
#include "llrenderstate.h"

// ── MARENISUpscaler ────────────────────────────────────────────────────────────

bool MARENISUpscaler::initialize(U32 renderW, U32 renderH)
{
    mWidth  = renderW;
    mHeight = renderH;
    if (!mNISBuffer.allocate(renderW, renderH, LLRenderTextureFormat::RGBA16F)) return false;
    return true;
}

void MARENISUpscaler::destroy()
{
    mNISBuffer.release();
    mWidth  = 0;
    mHeight = 0;
}

void MARENISUpscaler::apply(
    LLRenderTarget* colorSrc,
    LLRenderTarget* /*depthSrc*/,
    LLRenderTarget* /*velocitySrc*/,
    LLRenderTarget* outputDst,
    F32             /*jitterX*/,
    F32             /*jitterY*/,
    bool            /*cameraCut*/)
{
    if (!isReady())                                  return;
    if (!gDeferredNISProgram.mProgramObject)         return;
    if (!gDeferredTAACopyProgram.mProgramObject)     return;

    LLVertexBuffer* triVB = gPipeline.mScreenTriangleVB;
    if (!triVB) return;

    static LLCachedControl<F32> sharpenStrength(gSavedSettings, "RenderNISSharpenStrength", 0.5f);

    // ── Pass 1: NIS sharpening  colorSrc → mNISBuffer ────────────────────────
    {
        mNISBuffer.bindTarget();

        gDeferredNISProgram.bind();

        static LLStaticHashedString sColorMap        ("colorMap");
        static LLStaticHashedString sSharpenStrength ("sharpenStrength");

        gDeferredNISProgram.uniform1i(sColorMap, 0);
        gDeferredNISProgram.uniform1f(sSharpenStrength, (F32)sharpenStrength);

        gGL.getTexUnit(0)->bindManual(LLTexUnit::TT_TEXTURE, colorSrc->getTextureHandle());

        {
            LLGLDisable   blend(LLRenderCapability::Blend);
            LLGLDepthTest depth(false);
            triVB->setBuffer();
            triVB->drawArrays(LLRender::TRIANGLES, 0, 3);
        }

        gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);
        gGL.flush();  // MARE: drain pending gGL state while shader is still bound
        gDeferredNISProgram.unbind();
        mNISBuffer.flush();
    }

    // ── Pass 2: copy mNISBuffer → outputDst ──────────────────────────────────
    {
        outputDst->bindTarget();

        gDeferredTAACopyProgram.bind();

        static LLStaticHashedString sCopyMap("colorMap");
        gDeferredTAACopyProgram.uniform1i(sCopyMap, 0);

        gGL.getTexUnit(0)->bindManual(LLTexUnit::TT_TEXTURE, mNISBuffer.getTextureHandle());

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
}

// ── MARETAANISUpscaler ────────────────────────────────────────────────────────

bool MARETAANISUpscaler::initialize(U32 renderW, U32 renderH)
{
    if (!mTAA.initialize(renderW, renderH)) return false;
    if (!mNIS.initialize(renderW, renderH)) return false;
    return true;
}

void MARETAANISUpscaler::destroy()
{
    mTAA.destroy();
    mNIS.destroy();
}

void MARETAANISUpscaler::apply(
    LLRenderTarget* colorSrc,
    LLRenderTarget* depthSrc,
    LLRenderTarget* velocitySrc,
    LLRenderTarget* outputDst,
    F32             jitterX,
    F32             jitterY,
    bool            cameraCut)
{
    // Step 1: temporal accumulation into outputDst (via ping-pong internally).
    mTAA.apply(colorSrc, depthSrc, velocitySrc, outputDst, jitterX, jitterY, cameraCut);

    // Step 2: adaptive sharpening on the TAA result.
    mNIS.apply(outputDst, nullptr, nullptr, outputDst, 0.f, 0.f, false);
}
