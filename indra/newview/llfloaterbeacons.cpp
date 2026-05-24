/**
 * @file llfloaterbeacons.cpp
 * @brief Front-end to LLPipeline controls for highlighting various kinds of objects.
 * @author Coco
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

#include "llviewerprecompiledheaders.h"
#include "llfloaterbeacons.h"
#include "llviewercontrol.h"
#include "lluictrlfactory.h"
#include "llcheckboxctrl.h"
#include "pipeline.h"
//MK
#include "llagent.h"

namespace
{
template <typename T>
[[maybe_unused]] T* get_floater_child(LLView* owner, const std::string& name, bool recurse = false)
{
    return owner->getChild<T>(name, recurse);
}

template <typename T>
[[maybe_unused]] T* get_floater_child(const LLView* owner, const std::string& name, bool recurse = false)
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

//mk

LLFloaterBeacons::LLFloaterBeacons(const LLSD& seed)
:   LLFloater(seed)
{
    // Initialize pipeline states from saved settings.
    // OK to do at floater constructor time because beacons do not display unless the floater is open
    // therefore it is OK to not initialize the pipeline state before needed.
    // Note also that we should replace those pipeline statics with direct lookup of the saved settings
    // eliminating the need to keep these states in sync.
    LLPipeline::setRenderScriptedTouchBeacons(gSavedSettings.getBOOL("scripttouchbeacon"));
    LLPipeline::setRenderScriptedBeacons(     gSavedSettings.getBOOL("scriptsbeacon"));
    LLPipeline::setRenderPhysicalBeacons(     gSavedSettings.getBOOL("physicalbeacon"));
    LLPipeline::setRenderSoundBeacons(        gSavedSettings.getBOOL("soundsbeacon"));
    LLPipeline::setRenderParticleBeacons(     gSavedSettings.getBOOL("particlesbeacon"));
    LLPipeline::setRenderHighlights(          gSavedSettings.getBOOL("renderhighlights"));
    LLPipeline::setRenderBeacons(             gSavedSettings.getBOOL("renderbeacons"));
    LLPipeline::setRenderMOAPBeacons(         gSavedSettings.getBOOL("moapbeacon"));
    mCommitCallbackRegistrar.add("Beacons.UICheck", boost::bind(&LLFloaterBeacons::onClickUICheck, this,_1));
}

bool LLFloaterBeacons::postBuild()
{
    return true;
}

// Callback attached to each check box control to both affect their main purpose
// and to implement the couple screwy interdependency rules that some have.

void LLFloaterBeacons::onClickUICheck(LLUICtrl *ctrl)
{
    LLCheckBoxCtrl *check = (LLCheckBoxCtrl *)ctrl;
    std::string name = check->getName();

//MK
    if (gRRenabled && gAgent.mRRInterface.mContainsEdit)
    {
        LLPipeline::setRenderScriptedBeacons(FALSE);
        LLPipeline::setRenderScriptedTouchBeacons(FALSE);
        get_floater_child<LLCheckBoxCtrl>(this, "scripted")->setControlValue(LLSD(FALSE));
        get_floater_child<LLCheckBoxCtrl>(this, "touch_only")->setControlValue(LLSD(FALSE));
        LLPipeline::setRenderPhysicalBeacons(FALSE);
        LLPipeline::setRenderSoundBeacons(FALSE);
        LLPipeline::setRenderParticleBeacons(FALSE);
        LLPipeline::setRenderBeacons(FALSE);
        LLPipeline::setRenderHighlights(FALSE);
        get_floater_child<LLCheckBoxCtrl>(this, "beacons")->setControlValue(LLSD(FALSE));
        get_floater_child<LLCheckBoxCtrl>(this, "highlights")->setControlValue(LLSD(FALSE));
        return;
    }
//mk

    if(name == "touch_only")
    {
        LLPipeline::toggleRenderScriptedTouchBeacons();
        // Don't allow both to be ON at the same time. Toggle the other one off if both now on.
        if (
            LLPipeline::getRenderScriptedTouchBeacons() &&
            LLPipeline::getRenderScriptedBeacons() )
        {
            LLPipeline::setRenderScriptedBeacons(false);
            get_floater_child<LLCheckBoxCtrl>(this, "scripted")->setControlValue(LLSD(false));
            get_floater_child<LLCheckBoxCtrl>(this, "scripted")->setValue(false);
            get_floater_child<LLCheckBoxCtrl>(this, "touch_only")->setControlValue(LLSD(true)); // just to be sure it's in sync with llpipeline
            get_floater_child<LLCheckBoxCtrl>(this, "touch_only")->setValue(true);
        }
    }
    else if(name == "scripted")
    {
        LLPipeline::toggleRenderScriptedBeacons();
        // Don't allow both to be ON at the same time. Toggle the other one off if both now on.
        if (
            LLPipeline::getRenderScriptedTouchBeacons() &&
            LLPipeline::getRenderScriptedBeacons() )
        {
            LLPipeline::setRenderScriptedTouchBeacons(false);
            get_floater_child<LLCheckBoxCtrl>(this, "touch_only")->setControlValue(LLSD(false));
            get_floater_child<LLCheckBoxCtrl>(this, "touch_only")->setValue(false);
            get_floater_child<LLCheckBoxCtrl>(this, "scripted")->setControlValue(LLSD(true)); // just to be sure it's in sync with llpipeline
            get_floater_child<LLCheckBoxCtrl>(this, "scripted")->setValue(true);
        }
    }
    else if(name == "physical")       LLPipeline::setRenderPhysicalBeacons(check->get());
    else if(name == "sounds")         LLPipeline::setRenderSoundBeacons(check->get());
    else if(name == "particles")      LLPipeline::setRenderParticleBeacons(check->get());
    else if(name == "moapbeacon")     LLPipeline::setRenderMOAPBeacons(check->get());
    else if(name == "highlights")
    {
        LLPipeline::toggleRenderHighlights();
        // Don't allow both to be OFF at the same time. Toggle the other one on if both now off.
        if (
            !LLPipeline::getRenderBeacons() &&
            !LLPipeline::getRenderHighlights() )
        {
            LLPipeline::setRenderBeacons(true);
            get_floater_child<LLCheckBoxCtrl>(this, "beacons")->setControlValue(LLSD(true));
            get_floater_child<LLCheckBoxCtrl>(this, "beacons")->setValue(true);
            get_floater_child<LLCheckBoxCtrl>(this, "highlights")->setControlValue(LLSD(false)); // just to be sure it's in sync with llpipeline
            get_floater_child<LLCheckBoxCtrl>(this, "highlights")->setValue(false);
        }
    }
    else if(name == "beacons")
    {
        LLPipeline::toggleRenderBeacons();
        // Don't allow both to be OFF at the same time. Toggle the other one on if both now off.
        if (
            !LLPipeline::getRenderBeacons() &&
            !LLPipeline::getRenderHighlights() )
        {
            LLPipeline::setRenderHighlights(true);
            get_floater_child<LLCheckBoxCtrl>(this, "highlights")->setControlValue(LLSD(true));
            get_floater_child<LLCheckBoxCtrl>(this, "highlights")->setValue(true);
            get_floater_child<LLCheckBoxCtrl>(this, "beacons")->setControlValue(LLSD(false)); // just to be sure it's in sync with llpipeline
            get_floater_child<LLCheckBoxCtrl>(this, "beacons")->setValue(false);
        }
    }
}
