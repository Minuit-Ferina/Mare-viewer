/**
 * @file llrenderstate.h
 * @brief Public render state helpers without platform OpenGL headers.
 *
 * $LicenseInfo:firstyear=2026&license=viewerlgpl$
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
 * $/LicenseInfo$
 */

#ifndef LL_LLRENDERSTATE_H
#define LL_LLRENDERSTATE_H

#include <algorithm>
#include <functional>
#include <list>

#include <boost/unordered_map.hpp>

#include "glm/mat4x4.hpp"
#include "lldefs.h"
#include "llgltypes.h"
#include "llrenderbackendtypes.h"
#include "stdtypes.h"
#include "v4color.h"

class LLMatrix4;
class LLPlane;

class LLGLState
{
public:
    static void initClass();
    static void restoreGL();

    static void resetTextureStates();
    static void dumpStates();
    static void checkStates(LLGLboolean writeAlpha = true);

protected:
    static boost::unordered_map<LLGLenum, LLGLboolean> sStateMap;

public:
    enum { CURRENT_STATE = -2, DISABLED_STATE = 0, ENABLED_STATE = 1 };
    LLGLState(LLGLenum state, S32 enabled = CURRENT_STATE);
    LLGLState(LLRenderCapability capability, S32 enabled = CURRENT_STATE);
    ~LLGLState();
    void setEnabled(S32 enabled);
    void enable() { setEnabled(ENABLED_STATE); }
    void disable() { setEnabled(DISABLED_STATE); }

protected:
    LLGLenum mState;
    bool mWasEnabled;
    bool mIsEnabled;
};

class LLGLEnableBlending : public LLGLState
{
public:
    LLGLEnableBlending(bool enable);
};

class LLGLEnableAlphaReject : public LLGLState
{
public:
    LLGLEnableAlphaReject(bool enable);
};

class LLGLEnableFunc : LLGLState
{
public:
    LLGLEnableFunc(LLGLenum state, bool enable, std::function<void()> func)
        : LLGLState(state, enable)
    {
        if (enable)
        {
            func();
        }
    }
};

class LLGLEnable : public LLGLState
{
public:
    LLGLEnable(LLGLenum state) : LLGLState(state, ENABLED_STATE) {}
    LLGLEnable(LLRenderCapability capability) : LLGLState(capability, ENABLED_STATE) {}
};

class LLGLDisable : public LLGLState
{
public:
    LLGLDisable(LLGLenum state) : LLGLState(state, DISABLED_STATE) {}
    LLGLDisable(LLRenderCapability capability) : LLGLState(capability, DISABLED_STATE) {}
};

class LLGLUserClipPlane
{
public:
    LLGLUserClipPlane(const LLPlane& plane, const glm::mat4& modelview, const glm::mat4& projection, bool apply = true);
    ~LLGLUserClipPlane();

    void setPlane(F32 a, F32 b, F32 c, F32 d);
    void disable();

private:
    bool mApply;

    glm::mat4 mProjection;
    glm::mat4 mModelview;
};

class LLGLSquashToFarClip
{
public:
    LLGLSquashToFarClip();
    LLGLSquashToFarClip(const glm::mat4& projection, U32 layer = 0);

    void setProjectionMatrix(glm::mat4 projection, U32 layer);

    ~LLGLSquashToFarClip();
};

class LLGLUpdate
{
public:
    static std::list<LLGLUpdate*> sGLQ;

    bool mInQ;
    LLGLUpdate()
        : mInQ(false)
    {
    }
    virtual ~LLGLUpdate()
    {
        if (mInQ)
        {
            std::list<LLGLUpdate*>::iterator iter = std::find(sGLQ.begin(), sGLQ.end(), this);
            if (iter != sGLQ.end())
            {
                sGLQ.erase(iter);
            }
        }
    }
    virtual void updateGL() = 0;
};

const U32 FENCE_WAIT_TIME_NANOSECONDS = 1000;

class LLGLFence
{
public:
    virtual ~LLGLFence()
    {
    }

    virtual void placeFence() = 0;
    virtual bool isCompleted() = 0;
    virtual void wait() = 0;
};

class LLGLSyncFence : public LLGLFence
{
public:
    LLGLsync mSync;

    LLGLSyncFence();
    virtual ~LLGLSyncFence();

    void placeFence();
    bool isCompleted();
    void wait();
};

extern LLMatrix4 gGLObliqueProjectionInverse;

class LLGLDepthTest
{
public:
    LLGLDepthTest(LLGLboolean depth_enabled, LLGLboolean write_enabled, LLGLenum depth_func);
    LLGLDepthTest(bool depth_enabled, bool write_enabled = true, LLRenderDepthFunction depth_func = LLRenderDepthFunction::LessEqual);

    ~LLGLDepthTest();

    void checkState();

    LLGLboolean mPrevDepthEnabled;
    LLGLenum mPrevDepthFunc;
    LLGLboolean mPrevWriteEnabled;

private:
    static LLGLboolean sDepthEnabled;
    static LLGLenum sDepthFunc;
    static LLGLboolean sWriteEnabled;
};

class LLGLSDefault
{
protected:
    LLGLDisable mBlend, mCullFace;

public:
    LLGLSDefault()
        : mBlend(LLRenderCapability::Blend),
          mCullFace(LLRenderCapability::CullFace)
    { }
};

class LLGLSObjectSelect
{
protected:
    LLGLDisable mBlend;
    LLGLEnable mCullFace;

public:
    LLGLSObjectSelect()
        : mBlend(LLRenderCapability::Blend),
          mCullFace(LLRenderCapability::CullFace)
    { }
};

class LLGLSUIDefault
{
protected:
    LLGLEnable mBlend;
    LLGLDisable mCullFace;
    LLGLDepthTest mDepthTest;

public:
    LLGLSUIDefault()
        : mBlend(LLRenderCapability::Blend),
          mCullFace(LLRenderCapability::CullFace),
          mDepthTest(false, true, LLRenderDepthFunction::LessEqual)
    {}
};

class LLGLSPipeline
{
protected:
    LLGLEnable mCullFace;
    LLGLDepthTest mDepthTest;

public:
    LLGLSPipeline()
        : mCullFace(LLRenderCapability::CullFace),
          mDepthTest(true, true, LLRenderDepthFunction::LessEqual)
    { }
};

class LLGLSPipelineAlpha
{
protected:
    LLGLEnable mBlend;

public:
    LLGLSPipelineAlpha()
        : mBlend(LLRenderCapability::Blend)
    { }
};

class LLGLSPipelineSelection
{
protected:
    LLGLDisable mCullFace;

public:
    LLGLSPipelineSelection()
        : mCullFace(LLRenderCapability::CullFace)
    {}
};

class LLGLSPipelineSkyBox
{
protected:
    LLGLDisable mCullFace;
    LLGLSquashToFarClip mSquashClip;

public:
    LLGLSPipelineSkyBox();
   ~LLGLSPipelineSkyBox();
};

class LLGLSPipelineDepthTestSkyBox : public LLGLSPipelineSkyBox
{
public:
    LLGLSPipelineDepthTestSkyBox(bool depth_test, bool depth_write);

    LLGLDepthTest mDepth;
};

class LLGLSPipelineBlendSkyBox : public LLGLSPipelineDepthTestSkyBox
{
public:
    LLGLSPipelineBlendSkyBox(bool depth_test, bool depth_write);
    LLGLEnable mBlend;
};

class LLGLSTracker
{
protected:
    LLGLEnable mCullFace, mBlend;

public:
    LLGLSTracker()
        : mCullFace(LLRenderCapability::CullFace),
          mBlend(LLRenderCapability::Blend)
    { }
};

class LLGLSSpecular
{
public:
    F32 mShininess;
    LLGLSSpecular(const LLColor4& color, F32 shininess);
    ~LLGLSSpecular();
};

#endif // LL_LLRENDERSTATE_H
