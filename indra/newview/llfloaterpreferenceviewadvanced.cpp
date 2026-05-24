/**
 * @file llfloaterpreferenceviewadvanced.cpp
 * @brief floater for adjusting camera position
 *
 * $LicenseInfo:firstyear=2018&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2018, Linden Research, Inc.
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

#include "llviewerprecompiledheaders.h"
#include "llagentcamera.h"
#include "llfloaterpreferenceviewadvanced.h"
#include "llfloater.h"
#include "llfloaterreg.h"
#include "lluictrlfactory.h"
#include "llspinctrl.h"
#include "llviewercontrol.h"

namespace
{
template <typename T>
[[maybe_unused]] T* get_floater_child(LLView* owner, const std::string& name, bool recurse = true)
{
    return owner->getChild<T>(name, recurse);
}

template <typename T>
[[maybe_unused]] T* get_floater_child(const LLView* owner, const std::string& name, bool recurse = true)
{
    return const_cast<LLView*>(owner)->getChild<T>(name, recurse);
}

[[maybe_unused]] LLView* get_floater_view(LLView* owner, const std::string& name)
{
    return owner->getChildView(name);
}

[[maybe_unused]] LLView* get_floater_view(const LLView* owner, const std::string& name)
{
    return const_cast<LLView*>(owner)->getChildView(name);
}
}


LLFloaterPreferenceViewAdvanced::LLFloaterPreferenceViewAdvanced(const LLSD& key)
:   LLFloater(key)
{
    mCommitCallbackRegistrar.add("CommitSettings",  boost::bind(&LLFloaterPreferenceViewAdvanced::onCommitSettings, this));
}

LLFloaterPreferenceViewAdvanced::~LLFloaterPreferenceViewAdvanced()
{}

void LLFloaterPreferenceViewAdvanced::updateCameraControl(const LLVector3& vector)
{
    setCameraAxisControls(vector);
}

void LLFloaterPreferenceViewAdvanced::updateFocusControl(const LLVector3d& vector3d)
{
    setFocusAxisControls(vector3d);
}

void LLFloaterPreferenceViewAdvanced::draw()
{
    updateCameraControl(gAgentCamera.getCameraOffsetInitial());
    updateFocusControl(gAgentCamera.getFocusOffsetInitial());

    LLFloater::draw();
}

void LLFloaterPreferenceViewAdvanced::onCommitSettings()
{
    gSavedSettings.setVector3("CameraOffsetRearView", getCameraAxisControls());
    gSavedSettings.setVector3d("FocusOffsetRearView", getFocusAxisControls());
}

void LLFloaterPreferenceViewAdvanced::setCameraAxisControls(const LLVector3& vector)
{
    get_floater_child<LLSpinCtrl>(this, "camera_x")->setValue(vector[VX]);
    get_floater_child<LLSpinCtrl>(this, "camera_y")->setValue(vector[VY]);
    get_floater_child<LLSpinCtrl>(this, "camera_z")->setValue(vector[VZ]);
}

void LLFloaterPreferenceViewAdvanced::setFocusAxisControls(const LLVector3d& vector3d)
{
    get_floater_child<LLSpinCtrl>(this, "focus_x")->setValue(vector3d[VX]);
    get_floater_child<LLSpinCtrl>(this, "focus_y")->setValue(vector3d[VY]);
    get_floater_child<LLSpinCtrl>(this, "focus_z")->setValue(vector3d[VZ]);
}

LLVector3 LLFloaterPreferenceViewAdvanced::getCameraAxisControls()
{
    LLVector3 vector;
    vector.mV[VX] = getControlF32("camera_x");
    vector.mV[VY] = getControlF32("camera_y");
    vector.mV[VZ] = getControlF32("camera_z");
    return vector;
}

LLVector3d LLFloaterPreferenceViewAdvanced::getFocusAxisControls()
{
    LLVector3d vector3d;
    vector3d.mdV[VX] = getControlF32("focus_x");
    vector3d.mdV[VY] = getControlF32("focus_y");
    vector3d.mdV[VZ] = getControlF32("focus_z");
    return vector3d;
}

F32 LLFloaterPreferenceViewAdvanced::getControlF32(const std::string& name)
{
    return (F32)get_floater_child<LLUICtrl>(this, name)->getValue().asReal();
}
