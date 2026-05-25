#pragma once

// ─────────────────────────────────────────────────────────────────────────────
// marenisupscaler.h  –  MARE Phase 3 Step 2: NIS adaptive sharpening backends.
//
// MARENISUpscaler
//   NIS-inspired single-pass adaptive sharpening.  Allocates one internal
//   RGBA16F buffer (mNISBuffer) to avoid the read/write feedback loop that
//   would occur when colorSrc == outputDst.
//
//   Pass 1: NIS shader reads colorSrc → writes to mNISBuffer.
//   Pass 2: copy-back blits mNISBuffer → outputDst (reuses gDeferredTAACopyProgram).
//
// MARETAANISUpscaler
//   Composite upscaler: owns a MARETAAUpscaler and a MARENISUpscaler.
//   apply() runs TAA first (into outputDst via ping-pong), then NIS on the result.
// ─────────────────────────────────────────────────────────────────────────────

#include "mareupscaler.h"
#include "maretaaupscaler.h"
#include "llrendertarget.h"

// ── NIS-only backend ──────────────────────────────────────────────────────────

class MARENISUpscaler : public IUpscaler
{
public:
    MARENISUpscaler()  = default;
    ~MARENISUpscaler() override { destroy(); }

    bool initialize(U32 renderW, U32 renderH) override;
    void destroy() override;

    void apply(
        LLRenderTarget* colorSrc,
        LLRenderTarget* depthSrc,       // unused by NIS
        LLRenderTarget* velocitySrc,    // unused by NIS
        LLRenderTarget* outputDst,
        F32             jitterX,        // unused by NIS
        F32             jitterY,        // unused by NIS
        bool            cameraCut) override;  // unused by NIS

    bool isReady() const override { return mNISBuffer.isComplete(); }

private:
    LLRenderTarget mNISBuffer;  // intermediate buffer (avoids read/write feedback loop)
    U32 mWidth  = 0;
    U32 mHeight = 0;
};

// ── TAA + NIS composite backend ───────────────────────────────────────────────

class MARETAANISUpscaler : public IUpscaler
{
public:
    MARETAANISUpscaler()  = default;
    ~MARETAANISUpscaler() override { destroy(); }

    bool initialize(U32 renderW, U32 renderH) override;
    void destroy() override;

    void apply(
        LLRenderTarget* colorSrc,
        LLRenderTarget* depthSrc,       // unused by NIS composite
        LLRenderTarget* velocitySrc,
        LLRenderTarget* outputDst,
        F32             jitterX,
        F32             jitterY,
        bool            cameraCut) override;

    bool isReady() const override { return mTAA.isReady() && mNIS.isReady(); }

private:
    MARETAAUpscaler mTAA;
    MARENISUpscaler mNIS;
};
