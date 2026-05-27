/**
 * @file llviewerjoint.cpp
 * @brief Implementation of LLViewerJoint class
 *
 * $LicenseInfo:firstyear=2001&license=viewerlgpl$
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

//-----------------------------------------------------------------------------
// Header Files
//-----------------------------------------------------------------------------
#include "llviewerprecompiledheaders.h"

#include "llviewerjoint.h"

#include "llrenderbackend.h"
#include "llrender.h"
#include "llmath.h"

#include "llvoavatar.h"
#include "pipeline.h"
#include "llrenderstate.h"
#include "llrendercontext.h"

static constexpr S32 MIN_PIXEL_AREA_3PASS_HAIR = 64*64;

//-----------------------------------------------------------------------------
// LLViewerJoint()
// Class Constructors
//-----------------------------------------------------------------------------
LLViewerJoint::LLViewerJoint() :
    LLAvatarJoint()
{ }

LLViewerJoint::LLViewerJoint(S32 joint_num) :
    LLAvatarJoint(joint_num)
{ }

LLViewerJoint::LLViewerJoint(const std::string &name, LLJoint *parent) :
    LLAvatarJoint(name, parent)
{ }

//-----------------------------------------------------------------------------
// ~LLViewerJoint()
// Class Destructor
//-----------------------------------------------------------------------------
LLViewerJoint::~LLViewerJoint()
{
}

//--------------------------------------------------------------------
// render()
//--------------------------------------------------------------------
U32 LLViewerJoint::render( F32 pixelArea, bool first_pass, bool is_dummy )
{
    stop_glerror();

    U32 triangle_count = 0;

    //----------------------------------------------------------------
    // ignore invisible objects
    //----------------------------------------------------------------
    if ( mValid )
    {

        //----------------------------------------------------------------
        // if object is transparent, defer it, otherwise
        // give the joint subclass a chance to draw itself
        //----------------------------------------------------------------
        if ( is_dummy )
        {
            triangle_count += drawShape( pixelArea, first_pass, is_dummy );
        }
        else if (LLPipeline::sShadowRender)
        {
            triangle_count += drawShape(pixelArea, first_pass, is_dummy );
        }
        else if ( isTransparent() && !LLPipeline::sReflectionRender)
        {
            // Hair and Skirt
            if ((pixelArea > MIN_PIXEL_AREA_3PASS_HAIR))
            {
                // render all three passes
                LLGLDisable cull(LLRenderCapability::CullFace);
                // first pass renders without writing to the z buffer
                {
                    LLGLDepthTest gls_depth(true, false);
                    triangle_count += drawShape( pixelArea, first_pass, is_dummy );
                }
                // second pass writes to z buffer only
                gGL.setColorMask(false, false);
                {
                    triangle_count += drawShape( pixelArea, false, is_dummy  );
                }
                // third past respects z buffer and writes color
                gGL.setColorMask(true, false);
                {
                    LLGLDepthTest gls_depth(true, false);
                    triangle_count += drawShape( pixelArea, false, is_dummy  );
                }
            }
            else
            {
                // Render Inside (no Z buffer write)
                getRenderBackend().setCullFace(LLRenderCullFace::Front);
                {
                    LLGLDepthTest gls_depth(true, false);
                    triangle_count += drawShape( pixelArea, first_pass, is_dummy  );
                }
                // Render Outside (write to the Z buffer)
                getRenderBackend().setCullFace(LLRenderCullFace::Back);
                {
                    triangle_count += drawShape( pixelArea, false, is_dummy  );
                }
            }
        }
        else
        {
            // set up render state
            triangle_count += drawShape( pixelArea, first_pass );
        }
    }

    //----------------------------------------------------------------
    // render children
    //----------------------------------------------------------------
    for (LLJoint* j : mChildren)
    {
        // LLViewerJoint is derived from LLAvatarJoint,
        // all children of LLAvatarJoint are assumed to be LLAvatarJoint
        LLAvatarJoint* joint = static_cast<LLAvatarJoint*>(j);
        F32 jointLOD = joint->getLOD();
        if (pixelArea >= jointLOD || sDisableLOD)
        {
            triangle_count += joint->render( pixelArea, true, is_dummy );

            if (jointLOD != DEFAULT_AVATAR_JOINT_LOD)
            {
                break;
            }
        }
    }

    return triangle_count;
}

U32 LLViewerJoint::emitWorldCommands(
    LLWorldRenderCommandBuffer& commands,
    F32 pixelArea,
    bool first_pass,
    bool is_dummy)
{
    U32 triangle_count = 0;

    if (mValid)
    {
        if (is_dummy || LLPipeline::sShadowRender)
        {
            triangle_count += appendWorldCommand(commands, pixelArea, first_pass, is_dummy);
        }
        else if (!isTransparent() || LLPipeline::sReflectionRender)
        {
            triangle_count += appendWorldCommand(commands, pixelArea, first_pass);
        }
    }

    for (LLJoint* j : mChildren)
    {
        LLAvatarJoint* joint = static_cast<LLAvatarJoint*>(j);
        F32 jointLOD = joint->getLOD();
        if (pixelArea >= jointLOD || sDisableLOD)
        {
            LLViewerJoint* viewer_joint = dynamic_cast<LLViewerJoint*>(joint);
            if (viewer_joint)
            {
                triangle_count += viewer_joint->emitWorldCommands(commands, pixelArea, true, is_dummy);
            }

            if (jointLOD != DEFAULT_AVATAR_JOINT_LOD)
            {
                break;
            }
        }
    }

    return triangle_count;
}

//--------------------------------------------------------------------
// drawShape()
//--------------------------------------------------------------------
U32 LLViewerJoint::drawShape( F32 pixelArea, bool first_pass, bool is_dummy )
{
    return 0;
}

U32 LLViewerJoint::appendWorldCommand(LLWorldRenderCommandBuffer& commands, F32 pixelArea, bool first_pass, bool is_dummy)
{
    return 0;
}

// End
