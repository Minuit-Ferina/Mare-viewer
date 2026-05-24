/**
 * @file llsaveoutfitcombobtn.cpp
 * @brief Represents outfit save/save as combo button.
 *
 * $LicenseInfo:firstyear=2010&license=viewerlgpl$
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

#include "llappearancemgr.h"
#include "llpaneloutfitsinventory.h"
#include "llsidepanelappearance.h"
#include "llsaveoutfitcombobtn.h"
#include "llviewermenu.h"

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

static const std::string SAVE_BTN("save_btn");
static const std::string SAVE_FLYOUT_BTN("save_flyout_btn");

LLSaveOutfitComboBtn::LLSaveOutfitComboBtn(LLPanel* parent, bool saveAsDefaultAction):
    mParent(parent), mSaveAsDefaultAction(saveAsDefaultAction)
{
    // register action mapping before creating menu
    LLUICtrl::CommitCallbackRegistry::ScopedRegistrar save_registar;
    save_registar.add("Outfit.Save.Action", boost::bind(
            &LLSaveOutfitComboBtn::saveOutfit, this, false));
    save_registar.add("Outfit.SaveAs.Action", boost::bind(
            &LLSaveOutfitComboBtn::saveOutfit, this, true));

    mParent->childSetAction(SAVE_BTN, boost::bind(&LLSaveOutfitComboBtn::saveOutfit, this, mSaveAsDefaultAction));
    mParent->childSetAction(SAVE_FLYOUT_BTN, boost::bind(&LLSaveOutfitComboBtn::showSaveMenu, this));

    mSaveMenu = LLUICtrlFactory::getInstance()->createFromFile<
            LLToggleableMenu> ("menu_save_outfit.xml", gMenuHolder,
            LLViewerMenuHolderGL::child_registry_t::instance());
}

void LLSaveOutfitComboBtn::showSaveMenu()
{
    S32 x, y;
    LLUI::getInstance()->getMousePositionLocal(mParent, &x, &y);

    mSaveMenu->updateParent(LLMenuGL::sMenuContainer);
    LLMenuGL::showPopup(mParent, mSaveMenu, x, y);
}

void LLSaveOutfitComboBtn::saveOutfit(bool as_new)
{
    if (!as_new && LLAppearanceMgr::getInstance()->updateBaseOutfit())
    {
        // we don't need to ask for an outfit name, and updateBaseOutfit() successfully saved.
        // If updateBaseOutfit fails, ask for an outfit name anyways
        return;
    }

    LLPanelOutfitsInventory* panel_outfits_inventory =
            LLPanelOutfitsInventory::findInstance();
    if (panel_outfits_inventory)
    {
        panel_outfits_inventory->onSave();
    }

    //*TODO how to get to know when base outfit is updated or new outfit is created?
}

void LLSaveOutfitComboBtn::setMenuItemEnabled(const std::string& item, bool enabled)
{
    mSaveMenu->setItemEnabled("save_outfit", enabled);
}

void LLSaveOutfitComboBtn::setSaveBtnEnabled(bool enabled)
{
    get_owner_view(mParent, SAVE_BTN)->setEnabled(enabled);
}
