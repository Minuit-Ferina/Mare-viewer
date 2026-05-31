/**
 * @file lldrawpoolwlsky.h
 * @brief LLDrawPoolWLSky class definition
 *
 * $LicenseInfo:firstyear=2007&license=viewerlgpl$
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

#ifndef LL_DRAWPOOLWLSKY_H
#define LL_DRAWPOOLWLSKY_H

#include "lldrawpool.h"

class LLGLSLShader;

class LLDrawPoolWLSky : public LLDrawPool {
public:

    static const U32 SKY_VERTEX_DATA_MASK = LLVertexBuffer::MAP_VERTEX |
                            LLVertexBuffer::MAP_TEXCOORD0;
    static const U32 STAR_VERTEX_DATA_MASK =    LLVertexBuffer::MAP_VERTEX |
        LLVertexBuffer::MAP_COLOR | LLVertexBuffer::MAP_TEXCOORD0;
    static const U32 ADV_ATMO_SKY_VERTEX_DATA_MASK = LLVertexBuffer::MAP_VERTEX
                                                   | LLVertexBuffer::MAP_TEXCOORD0;
    LLDrawPoolWLSky(void);
    /*virtual*/ ~LLDrawPoolWLSky();

    bool isDead() override { return false; }

    S32 getNumDeferredPasses() override { return 1; }
    void beginDeferredPass(S32 pass) override;
    void endDeferredPass(S32 pass) override;
    void renderDeferred(S32 pass) override;
    bool emitDeferredCommands(LLWorldRenderCommandBuffer& commands, S32 pass) override;

    LLViewerTexture *getDebugTexture() override;
    U32 getVertexDataMask() override { return SKY_VERTEX_DATA_MASK; }
    bool verify() const override { return true; }        // Verify that all data in the draw pool is correct!
    S32 getShaderLevel() const override { return mShaderLevel; }

    //static LLDrawPool* createPool(const U32 type, LLViewerTexture *tex0 = NULL);

    LLViewerTexture* getTexture() override;
    bool isFacePool() override { return false; }
    void resetDrawOrders() override;

    static void cleanupGL();
    static void restoreGL();
private:
    void renderDome(const LLVector3& camPosLocal, F32 camHeightLocal, LLGLSLShader * shader) const;

    void renderSkyHazeDeferred(const LLVector3& camPosLocal, F32 camHeightLocal) const;
    void renderSkyCloudsDeferred(const LLVector3& camPosLocal, F32 camHeightLocal, LLGLSLShader* cloudshader) const;

    void renderStarsDeferred(const LLVector3& camPosLocal) const;
    void renderHeavenlyBodies();

    void emitHeavenlyBodyCommands(LLWorldRenderCommandBuffer& commands, const LLMatrix4& model_matrix) const;
    void emitStarCommands(LLWorldRenderCommandBuffer& commands, const LLMatrix4& model_matrix) const;
    void emitCloudCommands(LLWorldRenderCommandBuffer& commands, const LLMatrix4& model_matrix) const;
};

#endif // LL_DRAWPOOLWLSKY_H
