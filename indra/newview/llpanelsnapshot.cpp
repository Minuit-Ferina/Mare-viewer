/**
 * @file llpanelsnapshot.cpp
 * @brief Snapshot panel base class
 *
 * $LicenseInfo:firstyear=2011&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2011, Linden Research, Inc.
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
#include "llpanelsnapshot.h"

// libs
#include "llcombobox.h"
#include "llfloater.h"
#include "llfloatersnapshot.h"
#include "llsliderctrl.h"
#include "llspinctrl.h"
#include "lltrans.h"

// newview
#include "llsidetraypanelcontainer.h"
#include "llsnapshotlivepreview.h"
#include "llviewercontrol.h" // gSavedSettings

#include "llagentbenefits.h"

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

constexpr S32 MAX_TEXTURE_SIZE = 2048 ; //max upload texture size 2048 * 2048

S32 power_of_two(S32 sz, S32 upper)
{
    S32 res = upper;
    while( upper >= sz)
    {
        res = upper;
        upper >>= 1;
    }
    return res;
}

LLPanelSnapshot::LLPanelSnapshot()
    : mSnapshotFloater(NULL)
{}

// virtual
bool LLPanelSnapshot::postBuild()
{
    S32 w = getTypedPreviewWidth();
    S32 h = getTypedPreviewHeight();
    get_owner_child<LLUICtrl>(this, "save_btn")->setLabelArg("[UPLOAD_COST]", std::to_string(LLAgentBenefitsMgr::current().getTextureUploadCost(w, h)));
    get_owner_child<LLUICtrl>(this, getImageSizeComboName())->setCommitCallback(boost::bind(&LLPanelSnapshot::onResolutionComboCommit, this, _1));
    if (!getWidthSpinnerName().empty())
    {
        get_owner_child<LLUICtrl>(this, getWidthSpinnerName())->setCommitCallback(boost::bind(&LLPanelSnapshot::onCustomResolutionCommit, this));
    }
    if (!getHeightSpinnerName().empty())
    {
        get_owner_child<LLUICtrl>(this, getHeightSpinnerName())->setCommitCallback(boost::bind(&LLPanelSnapshot::onCustomResolutionCommit, this));
    }
    if (!getAspectRatioCBName().empty())
    {
        get_owner_child<LLUICtrl>(this, getAspectRatioCBName())->setCommitCallback(boost::bind(&LLPanelSnapshot::onKeepAspectRatioCommit, this, _1));
    }
    updateControls(LLSD());

    mSnapshotFloater = getParentByType<LLFloaterSnapshotBase>();
    return true;
}

// virtual
void LLPanelSnapshot::onOpen(const LLSD& key)
{
    S32 old_format = gSavedSettings.getS32("SnapshotFormat");
    S32 new_format = (S32) getImageFormat();

    gSavedSettings.setS32("SnapshotFormat", new_format);
    setCtrlsEnabled(true);

    // Switching panels will likely change image format.
    // Not updating preview right away may lead to errors,
    // e.g. attempt to send a large BMP image by email.
    if (old_format != new_format)
    {
        getParentByType<LLFloater>()->notify(LLSD().with("image-format-change", true));
    }

    // If resolution is set to "Current Window", force a snapshot update
    // each time a snapshot panel is opened to determine the correct
    // image size (and upload fee) depending on the snapshot type.
    if (mSnapshotFloater && get_owner_child<LLUICtrl>(this, getImageSizeComboName())->getValue().asString() == "[i0,i0]")
    {
        if (LLSnapshotLivePreview* preview = mSnapshotFloater->getPreviewView())
        {
            preview->mForceUpdateSnapshot = true;
        }
    }
}

LLSnapshotModel::ESnapshotFormat LLPanelSnapshot::getImageFormat() const
{
    return LLSnapshotModel::SNAPSHOT_FORMAT_JPEG;
}

void LLPanelSnapshot::enableControls(bool enable)
{
    setCtrlsEnabled(enable);
}

LLSpinCtrl* LLPanelSnapshot::getWidthSpinner()
{
    llassert(!getWidthSpinnerName().empty());
    return get_owner_child<LLSpinCtrl>(this, getWidthSpinnerName());
}

LLSpinCtrl* LLPanelSnapshot::getHeightSpinner()
{
    llassert(!getHeightSpinnerName().empty());
    return get_owner_child<LLSpinCtrl>(this, getHeightSpinnerName());
}

S32 LLPanelSnapshot::getTypedPreviewWidth() const
{
    llassert(!getWidthSpinnerName().empty());
    return get_owner_child<LLUICtrl>(this, getWidthSpinnerName())->getValue().asInteger();
}

S32 LLPanelSnapshot::getTypedPreviewHeight() const
{
    llassert(!getHeightSpinnerName().empty());
    return get_owner_child<LLUICtrl>(this, getHeightSpinnerName())->getValue().asInteger();
}

void LLPanelSnapshot::enableAspectRatioCheckbox(bool enable)
{
    llassert(!getAspectRatioCBName().empty());
    get_owner_child<LLUICtrl>(this, getAspectRatioCBName())->setEnabled(enable);
}

LLSideTrayPanelContainer* LLPanelSnapshot::getParentContainer()
{
    LLSideTrayPanelContainer* parent = dynamic_cast<LLSideTrayPanelContainer*>(getParent());
    if (!parent)
    {
        LL_WARNS() << "Cannot find panel container" << LL_ENDL;
        return NULL;
    }

    return parent;
}

void LLPanelSnapshot::updateImageQualityLevel()
{
    LLSliderCtrl* quality_slider = get_owner_child<LLSliderCtrl>(this, "image_quality_slider");
    S32 quality_val = llfloor((F32) quality_slider->getValue().asReal());

    std::string quality_lvl;

    if (quality_val < 20)
    {
        quality_lvl = LLTrans::getString("snapshot_quality_very_low");
    }
    else if (quality_val < 40)
    {
        quality_lvl = LLTrans::getString("snapshot_quality_low");
    }
    else if (quality_val < 60)
    {
        quality_lvl = LLTrans::getString("snapshot_quality_medium");
    }
    else if (quality_val < 80)
    {
        quality_lvl = LLTrans::getString("snapshot_quality_high");
    }
    else
    {
        quality_lvl = LLTrans::getString("snapshot_quality_very_high");
    }

    get_owner_child<LLTextBox>(this, "image_quality_level")->setTextArg("[QLVL]", quality_lvl);
}

void LLPanelSnapshot::goBack()
{
    LLSideTrayPanelContainer* parent = getParentContainer();
    if (parent)
    {
        parent->openPreviousPanel();
        parent->getCurrentPanel()->onOpen(LLSD());
    }
}

void LLPanelSnapshot::cancel()
{
    goBack();
    getParentByType<LLFloater>()->notify(LLSD().with("set-ready", true));
}

void LLPanelSnapshot::onCustomResolutionCommit()
{
    LLSD info;
    std::string widthSpinnerName = getWidthSpinnerName();
    std::string heightSpinnerName = getHeightSpinnerName();
    llassert(!widthSpinnerName.empty() && !heightSpinnerName.empty());
    LLSpinCtrl *widthSpinner = get_owner_child<LLSpinCtrl>(this, widthSpinnerName);
    LLSpinCtrl *heightSpinner = get_owner_child<LLSpinCtrl>(this, heightSpinnerName);
    if (getName() == "panel_snapshot_inventory")
    {
        S32 width = widthSpinner->getValue().asInteger();
        width = power_of_two(width, MAX_TEXTURE_SIZE);
        info["w"] = width;
        widthSpinner->setIncrement((F32)(width >> 1));
        widthSpinner->forceSetValue(width);
        S32 height =  heightSpinner->getValue().asInteger();
        height = power_of_two(height, MAX_TEXTURE_SIZE);
        heightSpinner->setIncrement((F32)(height >> 1));
        heightSpinner->forceSetValue((F32)height);
        info["h"] = height;
    }
    else
    {
        info["w"] = widthSpinner->getValue().asInteger();
        info["h"] = heightSpinner->getValue().asInteger();
    }
    getParentByType<LLFloater>()->notify(LLSD().with("custom-res-change", info));
}

void LLPanelSnapshot::onResolutionComboCommit(LLUICtrl* ctrl)
{
    LLSD info;
    info["combo-res-change"]["control-name"] = ctrl->getName();
    getParentByType<LLFloater>()->notify(info);
}

void LLPanelSnapshot::onKeepAspectRatioCommit(LLUICtrl* ctrl)
{
    getParentByType<LLFloater>()->notify(LLSD().with("keep-aspect-change", ctrl->getValue().asBoolean()));
}

LLSnapshotModel::ESnapshotType LLPanelSnapshot::getSnapshotType()
{
    return LLSnapshotModel::SNAPSHOT_WEB;
}
