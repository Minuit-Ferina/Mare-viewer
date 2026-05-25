#pragma once

// ─────────────────────────────────────────────────────────────────────────────
// MAREFSR2Upscaler  –  MARE Phase 3 Step 3: FSR 2 temporal upscaler backend.
//
// Implements the IUpscaler interface using a custom OpenGL compute-shader
// pipeline that follows the FSR 2 algorithm structure:
//
//   Pass 1 – Depth Clip / Dilate         (fsr2_depth_clip.comp.glsl)
//   Pass 2 – Reconstruct Previous Depth  (fsr2_reconstruct_prev_depth.comp.glsl)
//   Pass 3 – Shading Change / Lock       (fsr2_lock.comp.glsl)
//   Pass 4 – Temporal Accumulation       (fsr2_accumulate.comp.glsl)
//   Pass 5 – RCAS Sharpening             (fsr2_rcas.comp.glsl)
//
// Because LLGLSLShader has no compute support, all five passes are managed
// through backend compute program handles.
//
// Internal textures (all owned and freed by this class):
//   mDilatedDepth     – R32F  (render res) — Pass 1 → Passes 2 & 3
//   mDilatedMV        – RG32F (render res) — Pass 1 → Passes 2 & 4
//   mPrevDepth        – R32F  (render res) — previous frame depth (ping-pong)
//   mReconPrevDepth   – R32F  (render res) — Pass 2 → Pass 3
//   mLockStatus       – R8    (render res) — Pass 3 → Pass 4
//   mAccumBuffer[2]   – RGBA16F (display res) — ping-pong accumulation
//   mRCASBuffer       – RGBA16F (display res) — Pass 5 intermediate
//
// Jitter
//   computeJitter() provides the FSR 2 Halton(2,3) sequence scaled to the
//   render/display resolution ratio.  The camera system calls this each frame
//   when mode 3 is active.
// ─────────────────────────────────────────────────────────────────────────────

#include "mareupscaler.h"
#include "llrenderbackend.h"

class LLRenderTarget;

class MAREFSR2Upscaler : public IUpscaler
{
public:
    MAREFSR2Upscaler()  = default;
    ~MAREFSR2Upscaler() override { destroy(); }

    // IUpscaler — initialise at RENDER resolution; display res is inferred
    // from the pipeline (mDisplayScreen dimensions) at apply() time.
    bool initialize(U32 renderW, U32 renderH) override;
    void destroy()  override;

    // apply() — runs all 5 FSR 2 compute passes.
    //   colorSrc    – current jittered scene (render res, RGBA16F)
    //   depthSrc    – scene depth attachment (render res, bound in deferredScreen)
    //   velocitySrc – NDC motion vectors     (render res, RG16F mVelocityBuffer)
    //   outputDst   – FSR 2 output           (display res, mDisplayScreen)
    void apply(
        LLRenderTarget* colorSrc,
        LLRenderTarget* depthSrc,
        LLRenderTarget* velocitySrc,
        LLRenderTarget* outputDst,
        F32             jitterX,
        F32             jitterY,
        bool            cameraCut) override;

    bool isReady() const override { return mReady; }

    // Static helper: compute the FSR 2 Halton(2,3) sub-pixel jitter for the
    // given frame index, scaled from pixel space to UV space.
    // outX / outY are in UV space (divide by renderW/H to get UV).
    static void computeJitter(U32 frameIndex, U32 renderW, U32 displayW,
                               U32 renderH, U32 displayH,
                               F32& outX, F32& outY);

private:
    // ── Compute program management ────────────────────────────────────────────
    LLRenderProgramHandle compileComputeProgram(const std::string& sourcePath);

    // ── Internal texture helpers ──────────────────────────────────────────────
    LLRenderTextureHandle createTexture2D(U32 w, U32 h, LLRenderTextureFormat internalFmt);
    void deleteTexture(LLRenderTextureHandle& tex);

    // ── Compute programs ──────────────────────────────────────────────────────
    LLRenderProgramHandle mProgDepthClip;
    LLRenderProgramHandle mProgReconPrevDepth;
    LLRenderProgramHandle mProgLock;
    LLRenderProgramHandle mProgAccumulate;
    LLRenderProgramHandle mProgRCAS;

    // ── Internal textures ──────────────────────────────────────────────────────
    LLRenderTextureHandle mDilatedDepth;      // R32F,   render res
    LLRenderTextureHandle mDilatedMV;         // RG32F,  render res
    LLRenderTextureHandle mPrevDepth;         // R32F,   render res  (previous frame depth)
    LLRenderTextureHandle mReconPrevDepth;    // R32F,   render res
    LLRenderTextureHandle mLockStatus;        // R8,     render res
    LLRenderTextureHandle mAccumBuffer[2];    // RGBA16F, display res (ping-pong)
    LLRenderTextureHandle mRCASBuffer;        // RGBA16F, display res

    U32  mRenderW   = 0;
    U32  mRenderH   = 0;
    U32  mDisplayW  = 0;  // set each frame from outputDst dimensions
    U32  mDisplayH  = 0;
    U32  mAccumIdx  = 0;  // which mAccumBuffer is the current history
    bool mReady     = false;
};
