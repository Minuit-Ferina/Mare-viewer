/**
 * @file llfloaterpreviewtrash.cpp
 * @author AndreyK Productengine
 * @brief LLFloaterPreviewTrash class implementation
 *
 * $LicenseInfo:firstyear=2004&license=viewerlgpl$
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

#include "llfloaterpreviewtrash.h"

#include "llinventoryfunctions.h"
#include "llfloaterreg.h"

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

LLFloaterPreviewTrash::LLFloaterPreviewTrash(const LLSD& key)
:   LLFloater(key)
{
}

bool LLFloaterPreviewTrash::postBuild()
{
    setupButtons();
    // Always center the dialog.  User can change the size,
    // but purchases are important and should be center screen.
    // This also avoids problems where the user resizes the application window
    // mid-session and the saved rect is off-center.
    center();

    return true;
}

void LLFloaterPreviewTrash::setupButtons()
{
    get_floater_child<LLUICtrl>(this, "empty_btn")->setCommitCallback(
        boost::bind(&LLFloaterPreviewTrash::onClickEmpty, this));
    get_floater_child<LLUICtrl>(this, "cancel_btn")->setCommitCallback(
        boost::bind(&LLFloaterPreviewTrash::onClickCancel, this));
}

LLFloaterPreviewTrash::~LLFloaterPreviewTrash()
{
}


// static
void LLFloaterPreviewTrash::show()
{
    LLFloaterReg::showTypedInstance<LLFloaterPreviewTrash>("preview_trash", LLSD(), true);
}

// static
bool LLFloaterPreviewTrash::isVisible()
{
    return LLFloaterReg::instanceVisible("preview_trash");
}


void LLFloaterPreviewTrash::onClickEmpty()
{
    gInventory.emptyFolderType("PurgeSelectedItems", LLFolderType::FT_TRASH);
    closeFloater();
}

void LLFloaterPreviewTrash::onClickCancel()
{
    closeFloater();
}
