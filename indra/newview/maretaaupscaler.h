#pragma once

// ─────────────────────────────────────────────────────────────────────────────
// MARETAAUpscaler  –  MARE Phase 3 Step 1 internal OpenGL TAA backend.
//
// Algorithm
//   Per frame the shader:
//     1. Samples the current jittered scene colour (colorMap).
//     2. Builds a 3×3 neighbourhood AABB in the current frame.
//     3. Reprojects the previous accumulated frame via the NDC motion vector
//        stored in velocityMap (prevUV = uv − velocity × 0.5).
//     4. Clamps the reprojected history sample to the neighbourhood AABB to
//        suppress ghosting near disocclusion edges.
//     5. Blends current and clamped history: more history for static pixels
//        (low velocity, good anti-aliasing), less for fast-moving ones.
//
// GPU resources owned by this class
//   mAccumBuffer[2]  – ping-pong RGBA16F accumulation buffers (render res)
//   mAccumIndex      – which buffer is the CURRENT history (0 or 1)
// ─────────────────────────────────────────────────────────────────────────────

#include "mareupscaler.h"
#include "llrendertarget.h"

class MARETAAUpscaler : public IUpscaler
{
public:
    MARETAAUpscaler()  = default;
    ~MARETAAUpscaler() override { destroy(); }

    bool initialize(U32 renderW, U32 renderH) override;
    void destroy() override;

    void apply(
        LLRenderTarget* colorSrc,
        LLRenderTarget* depthSrc,       // ignored by TAA
        LLRenderTarget* velocitySrc,
        LLRenderTarget* outputDst,
        F32             jitterX,
        F32             jitterY,
        bool            cameraCut) override;

    bool isReady() const override
    {
        return mAccumBuffer[0].isComplete() && mAccumBuffer[1].isComplete();
    }

private:
    LLRenderTarget mAccumBuffer[2]; // ping-pong HDR accumulation buffers
    U32            mAccumIndex  = 0;    // index of the CURRENT history buffer
    U32            mWidth       = 0;
    U32            mHeight      = 0;
    bool           mFirstFrame  = true; // MARE: true until the first apply(); bypasses stale/uninitialised history
};
