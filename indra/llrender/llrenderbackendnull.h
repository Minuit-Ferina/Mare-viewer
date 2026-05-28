/**
 * @file llrenderbackendnull.h
 * @brief Null render backend implementation helper.
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

#ifndef LL_LLRENDERBACKENDNULL_H
#define LL_LLRENDERBACKENDNULL_H

#include "llrenderbackend.h"

class LLNullRenderBackend : public LLRenderBackend
{
public:
    LLRenderBackendType getType() const override { return LLRenderBackendType::Null; }
    const char* getName() const override { return "Null"; }
    bool isReady() const override { return true; }
    void initPlatformContextExtensions() override {}
    bool initContextCapabilities() override { return true; }
    void shutdownContextCapabilities() override {}
    bool createNativeContext(const LLRenderNativeContextDesc&, LLRenderNativeContext&) override { return false; }
    void destroyNativeContext(LLRenderNativeContext& context) override { context = {}; }
    bool makeNativeContextCurrent(void*) override { return false; }
    void clearCurrentNativeContext() override {}
    void swapNativeBuffers(void*) override {}
    void setNativeVSync(void*, bool) override {}
    bool setNativeContextThreadedOptimization(bool) override { return true; }
    void* createSharedNativeContext(void*, void*, bool) override { return nullptr; }
    void destroySharedNativeContext(void*) override {}

    void beginFrame(const LLRenderFrameDesc&) override {}
    void endFrame() override {}

    void beginRenderPass(const LLRenderPassDesc&) override {}
    void endRenderPass() override {}

    void setViewport(const LLRenderViewport&) override {}
    void setViewport(S32, S32, S32, S32) override {}
    void setScissor(const LLRenderScissor&) override {}
    void setScissor(S32, S32, S32, S32) override {}
    void clear(const LLRenderPassDesc&) override {}
    void clear(LLRenderClearMask) override {}
    void setClearColor(const LLRenderClearColor&) override {}
    void setClearColor(F32, F32, F32, F32) override {}
    void setColorMask(const LLRenderColorMask&) override {}
    void setBlendState(const LLRenderBlendState&) override {}
    void setLineWidth(F32) override {}
    void setPointSize(F32) override {}
    F32 getLineWidth() override { return 1.f; }
    void setCapability(LLRenderCapability, bool) override {}
    bool isCapabilityEnabled(LLRenderCapability) const override { return false; }
    void setCullFace(LLRenderCullFace) override {}
    void setDepthFunction(LLRenderDepthFunction) override {}
    void setDepthWriteEnabled(bool) override {}
    void setAlphaMaskCutoff(F32) override {}
    void setWorldDrawEnabled(bool) override {}
    void setWorldShaderClass(LLRenderWorldShaderClass) override {}
    void setWorldTerrainParameters(const LLRenderWorldTerrainParameters&) override {}
    void setWorldMaterialParameters(const LLRenderWorldMaterialParameters&) override {}
    void setWorldTextureTransform(const LLRenderWorldTextureTransform&) override {}
    void setWorldSkinningMatrixPalette(U32, const F32*) override {}
    LLRenderFloatRange getLineWidthRange(bool) const override { return {1.f, 1.f}; }
    void setPixelStoreInteger(LLRenderPixelStoreParameter, S32) override {}
    S32 getActiveTextureUnit() const override { return 0; }
    void setActiveTextureUnit(S32) override {}
    void bindTexture(LLRenderTextureTarget, U32) override {}
    void setTextureAddressMode(LLRenderTextureTarget, LLRenderTextureAddressMode) override {}
    void setTextureAddressMode(LLRenderTextureTarget, LLRenderTextureCoordinate, LLRenderTextureAddressMode) override {}
    void setTextureFilter(LLRenderTextureTarget, LLRenderTextureFilter, LLRenderTextureFilter) override {}
    void setTextureMagFilter(LLRenderTextureTarget, LLRenderTextureFilter) override {}
    void setTextureCompareMode(LLRenderTextureTarget, bool) override {}
    void setTextureMaxAnisotropy(LLRenderTextureTarget, F32) override {}
    void generateMipmaps(LLRenderTextureTarget) override {}
    void generateTextures(S32, U32*) override {}
    void deleteTextures(S32, const U32*) override {}
    void generateBuffers(S32, U32*) override {}
    void deleteBuffers(S32, const U32*) override {}
    void bindBuffer(LLRenderBufferTarget, U32) override {}
    void bindBufferBase(LLRenderBufferTarget, U32, U32) override {}
    void allocateBufferStorage(LLRenderBufferTarget, U64, const void*, LLRenderBufferUsage) override {}
    void updateBufferSubData(LLRenderBufferTarget, U32, U32, const void*) override {}
    void enableVertexAttributeArray(U32) override {}
    void disableVertexAttributeArray(U32) override {}
    void setVertexAttributePointer(
        U32,
        S32,
        LLRenderVertexAttributeType,
        bool,
        S32,
        const void*) override {}
    void setIntegerVertexAttributePointer(
        U32,
        S32,
        LLRenderVertexAttributeType,
        S32,
        const void*) override {}
    void drawIndexedRange(
        LLRenderPrimitiveType,
        U32,
        U32,
        S32,
        LLRenderIndexType,
        const void*) override {}
    void drawArrays(LLRenderPrimitiveType, S32, S32) override {}
    void drawElements(LLRenderPrimitiveType, S32, LLRenderIndexType, const void*) override {}
    void setLegacyVertexPointer(S32, LLRenderVertexAttributeType, S32, const void*) override {}
    void setLegacyTextureCoordinatePointer(S32, LLRenderVertexAttributeType, S32, const void*) override {}
    void setLegacyTextureCoordinateArray(bool) override {}
    void generateFramebuffers(S32, U32*) override {}
    void deleteFramebuffers(S32, const U32*) override {}
    void bindFramebuffer(LLRenderFramebufferBindPoint, U32) override {}
    void bindReadWriteFramebuffer(U32) override {}
    LLRenderFramebufferStatus getReadWriteFramebufferStatus() const override
    {
        return LLRenderFramebufferStatus::Complete;
    }
    bool isDrawFramebufferComplete() const override { return true; }
    void attachFramebufferTexture2D(
        LLRenderFramebufferAttachment,
        LLRenderTextureTarget,
        U32,
        S32) override {}
    void setFramebufferBufferRouting(U32) override {}
    void restoreDefaultFramebufferBufferRouting() override {}
    bool hasError() override { return false; }
    U32 getErrorCode() override { return 0; }
    void setDebugMessageCallback(LLRenderDebugMessageCallback, void*) override {}
    bool hasVertexArraySupport() const override { return true; }
    void generateVertexArrays(S32, U32*) override {}
    void bindVertexArray(U32) override {}
    void generateQueries(S32, U32*) override {}
    void deleteQueries(S32, const U32*) override {}
    void beginQuery(LLRenderQueryTarget, U32) override {}
    void endQuery(LLRenderQueryTarget) override {}
    void getQueryObjectUnsignedInteger64(U32, LLRenderQueryParameter, U64* value) override { *value = 0; }
    void getQueryObjectUnsignedInteger(U32, LLRenderQueryParameter, U32* value) override { *value = 0; }
    U32 createProgram() override { return 0; }
    void deleteProgram(U32) override {}
    U32 createShader(LLRenderShaderStage) override { return 0; }
    void deleteShader(U32) override {}
    bool isShader(U32) const override { return false; }
    bool isProgram(U32) const override { return false; }
    void attachShader(U32, U32) override {}
    void detachShader(U32, U32) override {}
    void getAttachedShaders(U32, S32, S32* count, U32*) override { *count = 0; }
    void setShaderSource(U32, S32, const char* const*) override {}
    void compileShader(U32) override {}
    void linkProgram(U32) override {}
    void validateProgram(U32) override {}
    void useProgram(U32) override {}
    void getShaderInteger(U32, LLRenderShaderParameter, S32* value) override { *value = 0; }
    void getProgramInteger(U32, LLRenderProgramParameter, S32* value) override { *value = 0; }
    void getShaderInfoLog(U32, S32, S32* length, char*) override { *length = 0; }
    void getProgramInfoLog(U32, S32, S32* length, char*) override { *length = 0; }
    void setProgramParameterInteger(U32, LLRenderProgramSetting, S32) override {}
    void setProgramBinary(U32, U32, const void*, S32) override {}
    void getProgramBinary(U32, S32, S32* length, U32* binary_format, void*) override
    {
        *length = 0;
        *binary_format = 0;
    }
    S32 getUniformLocation(U32, const char*) override { return -1; }
    S32 getAttributeLocation(U32, const char*) override { return -1; }
    void bindAttributeLocation(U32, U32, const char*) override {}
    void getActiveUniform(U32, U32, S32, S32* length, S32* size, U32* type, char* name) override
    {
        *length = 0;
        *size = 0;
        *type = 0;
        if (name)
        {
            name[0] = '\0';
        }
    }
    U32 getUniformBlockIndex(U32, const char*) override { return 0; }
    void bindUniformBlock(U32, U32, U32) override {}
    void setUniformInteger(S32, S32) override {}
    void setUniformInteger2(S32, S32, S32) override {}
    void setUniformIntegerVector(S32, S32, const S32*) override {}
    void setUniformIntegerVector4(S32, S32, const S32*) override {}
    void setUniformUnsignedIntegerVector4(S32, S32, const U32*) override {}
    void setUniformFloat(S32, F32) override {}
    void setUniformFloat2(S32, F32, F32) override {}
    void setUniformFloat3(S32, F32, F32, F32) override {}
    void setUniformFloat4(S32, F32, F32, F32, F32) override {}
    void setUniformFloatVector(S32, S32, const F32*) override {}
    void setUniformFloatVector2(S32, S32, const F32*) override {}
    void setUniformFloatVector3(S32, S32, const F32*) override {}
    void setUniformFloatVector4(S32, S32, const F32*) override {}
    void setUniformMatrix2(S32, S32, bool, const F32*) override {}
    void setUniformMatrix3(S32, S32, bool, const F32*) override {}
    void setUniformMatrix3x4(S32, S32, bool, const F32*) override {}
    void setUniformMatrix4(S32, S32, bool, const F32*) override {}
    void setVertexAttribute4(U32, F32, F32, F32, F32) override {}
    void setVertexAttributeVector4(U32, const F32*) override {}
    void bindTextureUnit(U32, LLRenderTextureHandle) override {}
    void bindImageTexture(U32, LLRenderTextureHandle, S32, bool, S32, LLRenderImageAccess, LLRenderTextureFormat) override {}
    void dispatchCompute(U32, U32, U32) override {}
    void setMemoryBarrier(LLRenderMemoryBarrierMask) override {}
    void pushLegacyAllAttributes() override {}
    void pushLegacyAllClientAttributes() override {}
    void popLegacyClientAttributes() override {}
    void popLegacyAttributes() override {}
    void copyTextureImage2D(LLRenderTextureTarget, S32, U32, S32, S32, S32, S32, S32) override {}
    void setTextureImage2D(
        LLRenderTextureTarget,
        S32,
        S32,
        S32,
        S32,
        S32,
        U32,
        U32,
        const void*) override {}
    void setTextureImage2D(
        LLRenderTextureTarget,
        S32,
        LLRenderTextureFormat,
        S32,
        S32,
        S32,
        LLRenderPixelFormat,
        LLRenderPixelType,
        const void*) override {}
    LLRenderTextureHandle createTexture2D(LLRenderTextureFormat, S32, S32) override { return LLRenderTextureHandle(); }
    void readPixels(S32, S32, S32, S32, LLRenderPixelFormat, LLRenderPixelType, void*) override {}
    void readTextureImage(LLRenderTextureTarget, S32, U32, U32, void*) override {}
    void readTextureImage(LLRenderTextureTarget, S32, LLRenderPixelFormat, LLRenderPixelType, void*) override {}
    void readCompressedTextureImage(LLRenderTextureTarget, S32, void*) override {}
    void copyTextureSubImage2D(LLRenderTextureTarget, S32, S32, S32, S32, S32, S32, S32) override {}
    void copyTextureSubImage3D(LLRenderTextureTarget, S32, S32, S32, S32, S32, S32, S32, S32) override {}
    void copyImageSubData(
        LLRenderTextureHandle,
        LLRenderTextureTarget,
        S32,
        S32,
        S32,
        S32,
        LLRenderTextureHandle,
        LLRenderTextureTarget,
        S32,
        S32,
        S32,
        S32,
        S32,
        S32,
        S32) override {}
    void setCompressedTextureImage2D(LLRenderTextureTarget, S32, S32, S32, S32, S32, S32, const void*) override {}
    void setTextureSubImage2D(LLRenderTextureTarget, S32, S32, S32, S32, S32, U32, U32, const void*) override {}
    void setTextureSubImage3D(
        LLRenderTextureTarget,
        S32,
        S32,
        S32,
        S32,
        S32,
        S32,
        S32,
        U32,
        U32,
        const void*) override {}
    void setTextureImage3D(
        LLRenderTextureTarget,
        S32,
        S32,
        S32,
        S32,
        S32,
        S32,
        U32,
        U32,
        const void*) override {}
    void getTextureLevelParameterInteger(LLRenderTextureTarget, S32, LLRenderTextureLevelParameter, S32* value) override
    {
        *value = 0;
    }
    void setTextureParameterInteger(LLRenderTextureTarget, LLRenderTextureParameter, S32) override {}
    void setTextureParameterIntegerVector(LLRenderTextureTarget, LLRenderTextureParameter, const S32*) override {}
    void setTextureGenerationMode(LLRenderTextureCoordinate, bool) override {}
    void setTextureGenerationObjectPlane(LLRenderTextureCoordinate, const F32*) override {}
    void areTexturesResident(S32 count, const U32*, bool* residences) override
    {
        for (S32 i = 0; i < count; ++i)
        {
            residences[i] = false;
        }
    }
    void getViewport(S32* viewport) override
    {
        viewport[0] = 0;
        viewport[1] = 0;
        viewport[2] = 0;
        viewport[3] = 0;
    }
    U32 getBoundTexture2D() override { return 0; }
    void* createSyncObject() override { return nullptr; }
    void flushCommands() override {}
    void finishCommands() override {}
    void clientWaitSyncObject(void*) override {}
    U32 clientWaitSyncObjectStatus(void*, U64) override { return 0; }
    void waitSyncObject(void*) override {}
    void deleteSyncObject(void*) override {}
    void setLegacyMaterialSpecular(const F32*, S32) override {}
    void getInteger(LLRenderIntegerParameter, S32* value) override { *value = 0; }
    void getLegacyInteger(U32, S32* value) override { *value = 0; }
    void getLegacyBoolean(U32, U8* value) override { *value = 0; }
    void getLegacyFloat(U32, F32* value) override { *value = 0.f; }
    void getLegacyBufferObjectParameterInteger(U32, U32, S32* value) override { *value = 0; }
    const char* getLegacyString(U32) override { return ""; }
    const char* getLegacyStringIndexed(U32, U32) override { return ""; }
    void setLegacyHint(U32, U32) override {}
    void setClientActiveTextureUnit(S32) override {}
    void setLegacyCapability(U32, bool) override {}
    bool isLegacyCapabilityEnabled(U32) override { return false; }
    void setPolygonOffset(F32, F32) override {}
    void setPolygonMode(LLRenderPolygonFace, LLRenderPolygonMode) override {}
    void setStencilFunction(LLRenderStencilFunction, S32, U32) override {}
    void setStencilMask(U32) override {}
    void setStencilOperation(
        LLRenderStencilOperation,
        LLRenderStencilOperation,
        LLRenderStencilOperation) override {}
    void setMatrixMode(LLRenderMatrixMode) override {}
    void pushMatrix() override {}
    void popMatrix() override {}
    const char* getInfoString(LLRenderInfoString) override { return ""; }
};

LLRenderBackend& getNullRenderBackend();

#endif
