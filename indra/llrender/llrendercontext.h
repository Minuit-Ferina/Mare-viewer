/**
 * @file llrendercontext.h
 * @brief Public render-context and GL capability declarations.
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

#ifndef LL_LLRENDERCONTEXT_H
#define LL_LLRENDERCONTEXT_H

#include <string>

#include "llerror.h"
#include "llfile.h"
#include "stdtypes.h"

class LLSD;

extern bool gDebugGL;
extern bool gDebugSession;
extern bool gDebugGLSession;
extern llofstream gFailLog;

#define LL_GL_ERRS LL_ERRS("RenderState")

void ll_init_fail_log(std::string filename);
void ll_fail(std::string msg);
void ll_close_fail_log();

class LLGLManager
{
public:
    LLGLManager();

    bool initGL();
    void shutdownGL();

    void initWGL();

    std::string getRawGLString();

    bool mInited;
    bool mIsDisabled;

    S32 mMaxSamples;
    S32 mNumTextureImageUnits;
    S32 mMaxSampleMaskWords;
    S32 mMaxColorTextureSamples;
    S32 mMaxDepthTextureSamples;
    S32 mMaxIntegerSamples;
    S32 mGLMaxVertexRange;
    S32 mGLMaxIndexRange;
    S32 mGLMaxTextureSize;
    F32 mMaxAnisotropy = 0.f;
    S32 mMaxUniformBlockSize = 0;
    S32 mMaxVaryingVectors = 0;

    bool mHasCubeMapArray = false;
    bool mHasDebugOutput = false;
    bool mHasTransformFeedback = false;
    bool mHasAnisotropic = false;

    bool mHasAMDAssociations = false;
    bool mHasNVXGpuMemoryInfo = false;

    bool mIsAMD;
    bool mIsNVIDIA;
    bool mIsIntel;
    bool mIsApple = false;

    U32 mDownScaleMethod = 0;

#if LL_DARWIN
    bool mIsMobileGF;
#endif

    bool mHasRequirements;

    S32 mDriverVersionMajor;
    S32 mDriverVersionMinor;
    S32 mDriverVersionRelease;
    F32 mGLVersion;
    S32 mGLSLVersionMajor;
    S32 mGLSLVersionMinor;
    std::string mDriverVersionVendorString;
    std::string mGLVersionString;

    U32 mVRAM;

    std::string getGLInfoString();
    void printGLInfoString();
    void getGLInfo(LLSD& info);

    void asLLSD(LLSD& info);

    std::string mGLVendor;
    std::string mGLVendorShort;
    std::string mGLRenderer;

private:
    void initExtensions();
    void initGLStates();
};

extern LLGLManager gGLManager;

class LLQuaternion;
class LLMatrix4;

void rotate_quat(LLQuaternion& rotation);

void flush_glerror();
void log_glerror();
void assert_glerror();
void clear_glerror();

#define stop_glerror() assert_glerror()
#define llglassertok() assert_glerror()

#if LL_DARWIN && !LL_RELEASE_FOR_DOWNLOAD
#define STOP_GLERROR stop_glerror()
#else
#define STOP_GLERROR
#endif

#define llglassertok_always() assert_glerror()

void init_glstates();
void parse_gl_version(S32* major, S32* minor, S32* release, std::string* vendor_specific, std::string* version_string);

extern bool gClothRipple;
extern bool gHeadlessClient;
extern bool gNonInteractive;
extern bool gGLActive;

#endif // LL_LLRENDERCONTEXT_H
