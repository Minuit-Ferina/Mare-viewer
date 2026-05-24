/**
 * @file llpanelvolumepulldown.cpp
 * @author Tofu Linden
 * @brief A floater showing the master volume pull-down
 *
 * $LicenseInfo:firstyear=2008&license=viewerlgpl$
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

#include "llpanelvolumepulldown.h"

// Viewer libs
#include "llviewercontrol.h"
#include "llstatusbar.h"

// Linden libs
#include "llbutton.h"
#include "llcheckboxctrl.h"
#include "lltabcontainer.h"
#include "llfloaterreg.h"
#include "llfloaterpreference.h"
#include "llsliderctrl.h"

namespace
{
template <typename T>
[[maybe_unused]] T* get_owner_child(LLView* owner, const std::string& name, bool recurse = true)
{
    return owner->getChild<T>(name, recurse);
}

template <typename T>
[[maybe_unused]] T* get_owner_child(const LLView* owner, const std::string& name, bool recurse = true)
{
    return const_cast<LLView*>(owner)->getChild<T>(name, recurse);
}

[[maybe_unused]] LLView* get_owner_view(LLView* owner, const std::string& name, bool recurse = true)
{
    return owner->getChildView(name, recurse);
}

[[maybe_unused]] LLView* get_owner_view(const LLView* owner, const std::string& name, bool recurse = true)
{
    return const_cast<LLView*>(owner)->getChildView(name, recurse);
}
}

///----------------------------------------------------------------------------
/// Class LLPanelVolumePulldown
///----------------------------------------------------------------------------

// Default constructor
LLPanelVolumePulldown::LLPanelVolumePulldown()
{
    mCommitCallbackRegistrar.add("Vol.setControlFalse", boost::bind(&LLPanelVolumePulldown::setControlFalse, this, _2));
    mCommitCallbackRegistrar.add("Vol.SetSounds", boost::bind(&LLPanelVolumePulldown::onClickSetSounds, this));
    mCommitCallbackRegistrar.add("Vol.updateCheckbox",  boost::bind(&LLPanelVolumePulldown::updateCheckbox, this, _1, _2));
    mCommitCallbackRegistrar.add("Vol.GoAudioPrefs", boost::bind(&LLPanelVolumePulldown::onAdvancedButtonClick, this, _2));
    buildFromFile( "panel_volume_pulldown.xml");
}

bool LLPanelVolumePulldown::postBuild()
{
    return LLPanelPulldown::postBuild();
}

void LLPanelVolumePulldown::onAdvancedButtonClick(const LLSD& user_data)
{
    // close the global volume minicontrol, we're bringing up the big one
    setVisible(false);

    // bring up the prefs floater
    LLFloaterPreference* prefsfloater = dynamic_cast<LLFloaterPreference*>
        (LLFloaterReg::showInstance("preferences"));
    if (prefsfloater)
    {
        // grab the 'audio' panel from the preferences floater and
        // bring it the front!
        LLTabContainer* tabcontainer = get_owner_child<LLTabContainer>(prefsfloater, "pref core");
        LLPanel* audiopanel = get_owner_child<LLPanel>(prefsfloater, "audio");
        if (tabcontainer && audiopanel)
        {
            tabcontainer->selectTabPanel(audiopanel);
        }
    }
}

void LLPanelVolumePulldown::setControlFalse(const LLSD& user_data)
{
    std::string control_name = user_data.asString();
    LLControlVariable* control = findControl(control_name);

    if (control)
        control->set(LLSD(false));
}

void LLPanelVolumePulldown::updateCheckbox(LLUICtrl* ctrl, const LLSD& user_data)
{
    std::string control_name = user_data.asString();
    if (control_name == "MediaAutoPlay")
    {
        std::string name = ctrl->getName();

        // Disable "Allow Media to auto play" only when both
        // "Streaming Music" and "Media" are unchecked. STORM-513.
        if ((name == "enable_music") || (name == "enable_media"))
        {
            bool music_enabled = get_owner_child<LLCheckBoxCtrl>(this, "enable_music")->get();
            bool media_enabled = get_owner_child<LLCheckBoxCtrl>(this, "enable_media")->get();

            get_owner_child<LLCheckBoxCtrl>(this, "media_auto_play_combo")->setEnabled(music_enabled || media_enabled);
        }
    }
}

void LLPanelVolumePulldown::onClickSetSounds()
{
    // Disable Enable gesture sounds checkbox if the master sound is disabled
    // or if sound effects are disabled.
    get_owner_child<LLCheckBoxCtrl>(this, "gesture_audio_play_btn")->setEnabled(!gSavedSettings.getBOOL("MuteSounds"));
}
