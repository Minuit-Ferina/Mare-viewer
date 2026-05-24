/**
 * @file llfloaterconversationlog.cpp
 * @brief Functionality of the "conversation log" floater
 *
 * $LicenseInfo:firstyear=2012&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2012, Linden Research, Inc.
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

#include "llconversationloglist.h"
#include "llfiltereditor.h"
#include "llfloaterconversationlog.h"
#include "llfloaterreg.h"
#include "llmenubutton.h"


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

LLFloaterConversationLog::LLFloaterConversationLog(const LLSD& key)
:   LLFloater(key),
    mConversationLogList(NULL)
{
    mCommitCallbackRegistrar.add("CallLog.Action",  boost::bind(&LLFloaterConversationLog::onCustomAction,  this, _2));
    mEnableCallbackRegistrar.add("CallLog.Check",   boost::bind(&LLFloaterConversationLog::isActionChecked, this, _2));
}

bool LLFloaterConversationLog::postBuild()
{
    mConversationLogList = get_floater_child<LLConversationLogList>(this, "conversation_log_list");

    switch (gSavedSettings.getU32("CallLogSortOrder"))
    {
    case LLConversationLogList::E_SORT_BY_NAME:
        mConversationLogList->sortByName();
        break;

    case LLConversationLogList::E_SORT_BY_DATE:
        mConversationLogList->sortByDate();
        break;
    }

    // Use the context menu of the Conversation list for the Conversation tab gear menu.
    mConversationsGearBtn = get_floater_child<LLMenuButton>(this, "conversations_gear_btn");
    LLToggleableMenu* conversations_gear_menu = mConversationLogList->getContextMenu();
    if (conversations_gear_menu)
    {
        mConversationsGearBtn->setMenu(conversations_gear_menu, LLMenuButton::MP_BOTTOM_LEFT);
    }

    get_floater_child<LLFilterEditor>(this, "people_filter_input")->setCommitCallback(boost::bind(&LLFloaterConversationLog::onFilterEdit, this, _2));

    return LLFloater::postBuild();
}

void LLFloaterConversationLog::draw()
{
    mConversationsGearBtn->setEnabled(mConversationLogList->getSelectedItem() != NULL);
    LLFloater::draw();
}

void LLFloaterConversationLog::onFilterEdit(const std::string& search_string)
{
    std::string filter = search_string;
    LLStringUtil::trimHead(filter);

    mConversationLogList->setNameFilter(filter);
}


void LLFloaterConversationLog::onCustomAction (const LLSD& userdata)
{
    const std::string command_name = userdata.asString();

    if ("sort_by_name" == command_name)
    {
        mConversationLogList->sortByName();
        gSavedSettings.setU32("CallLogSortOrder", LLConversationLogList::E_SORT_BY_NAME);
    }
    else if ("sort_by_date" == command_name)
    {
        mConversationLogList->sortByDate();
        gSavedSettings.setU32("CallLogSortOrder", LLConversationLogList::E_SORT_BY_DATE);
    }
    else if ("sort_friends_on_top" == command_name)
    {
        mConversationLogList->toggleSortFriendsOnTop();
    }
    else if ("view_nearby_chat_history" == command_name)
    {
        LLFloaterReg::showInstance("preview_conversation", LLSD(LLUUID::null), true);
    }
}

bool LLFloaterConversationLog::isActionEnabled(const LLSD& userdata)
{
    return true;
}

bool LLFloaterConversationLog::isActionChecked(const LLSD& userdata)
{
    const std::string command_name = userdata.asString();

    U32 sort_order = gSavedSettings.getU32("CallLogSortOrder");

    if ("sort_by_name" == command_name)
    {
        return sort_order == LLConversationLogList::E_SORT_BY_NAME;
    }
    else if ("sort_by_date" == command_name)
    {
        return sort_order == LLConversationLogList::E_SORT_BY_DATE;
    }
    else if ("sort_friends_on_top" == command_name)
    {
        return gSavedSettings.getBOOL("SortFriendsFirst");
    }

    return false;
}

