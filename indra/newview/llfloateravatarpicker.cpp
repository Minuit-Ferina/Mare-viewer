/**
 * @file llfloateravatarpicker.cpp
 *
 * $LicenseInfo:firstyear=2003&license=viewerlgpl$
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

#include "llfloateravatarpicker.h"

// Viewer includes
#include "llagent.h"
#include "llcallingcard.h"
#include "llfocusmgr.h"
#include "llfloaterreg.h"
#include "llimview.h"           // for gIMMgr
#include "lltooldraganddrop.h"  // for LLToolDragAndDrop
#include "lltrans.h"
#include "llviewercontrol.h"
#include "llviewerregion.h"     // getCapability()
#include "llworld.h"

// Linden libraries
#include "llavatarnamecache.h"  // IDEVO
#include "llbutton.h"
#include "llcachename.h"
#include "lllineeditor.h"
#include "llscrolllistctrl.h"
#include "llscrolllistitem.h"
#include "llscrolllistcell.h"
#include "lltabcontainer.h"
#include "lluictrlfactory.h"
#include "llfocusmgr.h"
#include "lldraghandle.h"
#include "message.h"
#include "llcorehttputil.h"

//#include "llsdserialize.h"

#include "fsavatarsearchmenu.h"
#include "fsscrolllistctrl.h"
#include "lltransientfloatermgr.h"


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

static const U32 AVATAR_PICKER_SEARCH_TIMEOUT = 180U;

//put it back as a member once the legacy path is out?
static std::map<LLUUID, LLAvatarName> sAvatarNameMap;

LLFloaterAvatarPicker* LLFloaterAvatarPicker::show(select_callback_t callback,
                                                   bool allow_multiple,
                                                   bool closeOnSelect,
                                                   bool skip_agent,
                                                   const std::string& name,
                                                   LLView * frustumOrigin)
{
    // *TODO: Use a key to allow this not to be an effective singleton
    LLFloaterAvatarPicker* floater =
        LLFloaterReg::showTypedInstance<LLFloaterAvatarPicker>("avatar_picker", LLSD(name));
    if (!floater)
    {
        LL_WARNS() << "Cannot instantiate avatar picker" << LL_ENDL;
        return NULL;
    }

    floater->mSelectionCallback = callback;
    floater->setAllowMultiple(allow_multiple);
    floater->mNearMeListComplete = false;
    floater->mCloseOnSelect = closeOnSelect;
    floater->mExcludeAgentFromSearchResults = skip_agent;

    if (!closeOnSelect)
    {
        // Use Select/Close
        std::string select_string = floater->getString("Select");
        std::string close_string = floater->getString("Close");
        get_floater_child<LLButton>(floater, "ok_btn")->setLabel(select_string);
        get_floater_child<LLButton>(floater, "cancel_btn")->setLabel(close_string);
    }

    if(frustumOrigin)
    {
        floater->mFrustumOrigin = frustumOrigin->getHandle();
    }

    return floater;
}

// Default constructor
LLFloaterAvatarPicker::LLFloaterAvatarPicker(const LLSD& key)
  : LLFloater(key),
    mNumResultsReturned(0),
    mNearMeListComplete(false),
    mCloseOnSelect(false),
    mExcludeAgentFromSearchResults(false),
    mContextConeOpacity (0.f),
    mContextConeInAlpha(CONTEXT_CONE_IN_ALPHA),
    mContextConeOutAlpha(CONTEXT_CONE_OUT_ALPHA),
    mContextConeFadeTime(CONTEXT_CONE_FADE_TIME),
    mFindUUIDAvatarNameCacheConnection() // <FS:Ansariel> Search by UUID
{
    mCommitCallbackRegistrar.add("Refresh.FriendList", boost::bind(&LLFloaterAvatarPicker::populateFriend, this));
}

bool LLFloaterAvatarPicker::postBuild()
{
    get_floater_child<LLLineEditor>(this, "Edit")->setKeystrokeCallback( boost::bind(&LLFloaterAvatarPicker::editKeystroke, this, _1, _2),NULL);

    childSetAction("Find", boost::bind(&LLFloaterAvatarPicker::onBtnFind, this));
    get_floater_view(this, "Find")->setEnabled(false);
    childSetAction("Refresh", boost::bind(&LLFloaterAvatarPicker::onBtnRefresh, this));
    get_floater_child<LLUICtrl>(this, "near_me_range")->setCommitCallback(boost::bind(&LLFloaterAvatarPicker::onRangeAdjust, this));

    // <FS:Ansariel> FIRE-5096: Add context menu for result lists
    //LLScrollListCtrl* searchresults = getChild<LLScrollListCtrl>("SearchResults");
    FSScrollListCtrl* searchresults = get_floater_child<FSScrollListCtrl>(this, "SearchResults");
    searchresults->setContextMenu(&gFSAvatarSearchMenu);
    // </FS:Ansariel>
    searchresults->setDoubleClickCallback( boost::bind(&LLFloaterAvatarPicker::onBtnSelect, this));
    searchresults->setCommitCallback(boost::bind(&LLFloaterAvatarPicker::onList, this));
    get_floater_view(this, "SearchResults")->setEnabled(false);

    // <FS:Ansariel> FIRE-5096: Add context menu for result lists
    //LLScrollListCtrl* nearme = getChild<LLScrollListCtrl>("NearMe");
    FSScrollListCtrl* nearme = get_floater_child<FSScrollListCtrl>(this, "NearMe");
    nearme->setContextMenu(&gFSAvatarSearchMenu);
    // </FS:Ansariel>
    nearme->setDoubleClickCallback(boost::bind(&LLFloaterAvatarPicker::onBtnSelect, this));
    nearme->setCommitCallback(boost::bind(&LLFloaterAvatarPicker::onList, this));

    // <FS:Ansariel> FIRE-5096: Add context menu for result lists
    //LLScrollListCtrl* friends = getChild<LLScrollListCtrl>("Friends");
    FSScrollListCtrl* friends = get_floater_child<FSScrollListCtrl>(this, "Friends");
    friends->setContextMenu(&gFSAvatarSearchMenu);
    // </FS:Ansariel>
    friends->setDoubleClickCallback(boost::bind(&LLFloaterAvatarPicker::onBtnSelect, this));
    get_floater_child<LLUICtrl>(this, "Friends")->setCommitCallback(boost::bind(&LLFloaterAvatarPicker::onList, this));

    childSetAction("ok_btn", boost::bind(&LLFloaterAvatarPicker::onBtnSelect, this));
    get_floater_view(this, "ok_btn")->setEnabled(false);
    childSetAction("cancel_btn", boost::bind(&LLFloaterAvatarPicker::onBtnClose, this));

    get_floater_child<LLUICtrl>(this, "Edit")->setFocus(true);

    LLPanel* search_panel = get_floater_child<LLPanel>(this, "SearchPanel");
    if (search_panel)
    {
        // Start searching when Return is pressed in the line editor.
        search_panel->setDefaultBtn("Find");
    }

    get_floater_child<LLScrollListCtrl>(this, "SearchResults")->setCommentText(getString("no_results"));

    get_floater_child<LLTabContainer>(this, "ResidentChooserTabs")->setCommitCallback(
        boost::bind(&LLFloaterAvatarPicker::onTabChanged, this));

    // <FS:Ansariel> Search by UUID
    get_floater_child<LLLineEditor>(this, "EditUUID")->setKeystrokeCallback(boost::bind(&LLFloaterAvatarPicker::editKeystrokeUUID, this, _1, _2), NULL);
    childSetAction("FindUUID", boost::bind(&LLFloaterAvatarPicker::onBtnFindUUID, this));
    get_floater_view(this, "FindUUID")->setEnabled(FALSE);

    FSScrollListCtrl* searchresultsuuid = get_floater_child<FSScrollListCtrl>(this, "SearchResultsUUID");
    searchresultsuuid->setContextMenu(&gFSAvatarSearchMenu);
    searchresultsuuid->setDoubleClickCallback( boost::bind(&LLFloaterAvatarPicker::onBtnSelect, this));
    searchresultsuuid->setCommitCallback(boost::bind(&LLFloaterAvatarPicker::onList, this));
    searchresultsuuid->setEnabled(FALSE);
    searchresultsuuid->setCommentText(getString("no_results"));

    get_floater_child<LLPanel>(this, "SearchPanelUUID")->setDefaultBtn("FindUUID");
    // </FS:Ansariel>

    setAllowMultiple(false);

    center();

    populateFriend();

    return true;
}

void LLFloaterAvatarPicker::setOkBtnEnableCb(validate_callback_t cb)
{
    mOkButtonValidateSignal.connect(cb);
}

void LLFloaterAvatarPicker::onTabChanged()
{
    get_floater_view(this, "ok_btn")->setEnabled(isSelectBtnEnabled());
}

// Destroys the object
LLFloaterAvatarPicker::~LLFloaterAvatarPicker()
{
    // <FS:Ansariel> Search by UUID
    if (mFindUUIDAvatarNameCacheConnection.connected())
    {
        mFindUUIDAvatarNameCacheConnection.disconnect();
    }
    // </FS:Ansariel>

    gFocusMgr.releaseFocusIfNeeded( this );

    LLTransientFloaterMgr::getInstance()->removeControlView(LLTransientFloaterMgr::IM, this);
}

// <FS:Ansariel> Search by UUID
void LLFloaterAvatarPicker::onBtnFindUUID()
{
    LLScrollListCtrl* search_results = get_floater_child<LLScrollListCtrl>(this, "SearchResultsUUID");
    search_results->deleteAllItems();
    search_results->setCommentText(getString("searching"));

    if (mFindUUIDAvatarNameCacheConnection.connected())
    {
        mFindUUIDAvatarNameCacheConnection.disconnect();
    }
    LLAvatarNameCache::get(LLUUID(get_floater_child<LLLineEditor>(this, "EditUUID")->getText()), boost::bind(&LLFloaterAvatarPicker::onFindUUIDAvatarNameCache, this, _1, _2));
}

void LLFloaterAvatarPicker::onFindUUIDAvatarNameCache(const LLUUID& av_id, const LLAvatarName& av_name)
{
    mFindUUIDAvatarNameCacheConnection.disconnect();
    LLScrollListCtrl* search_results = get_floater_child<LLScrollListCtrl>(this, "SearchResultsUUID");
    search_results->deleteAllItems();

    if (av_name.getAccountName() != "(?\?\?).(?\?\?)")
    {
        LLSD data;
        data["id"] = av_id;

        data["columns"][0]["name"] = "nameUUID";
        data["columns"][0]["value"] = av_name.getDisplayName();

        data["columns"][1]["name"] = "usernameUUID";
        data["columns"][1]["value"] = av_name.getUserName();

        search_results->addElement(data);
        search_results->setEnabled(TRUE);
        search_results->sortByColumnIndex(1, TRUE);
        search_results->selectFirstItem();
        onList();
        search_results->setFocus(TRUE);

        get_floater_view(this, "ok_btn")->setEnabled(TRUE);
    }
    else
    {
        LLStringUtil::format_map_t map;
        map["[TEXT]"] = get_floater_child<LLUICtrl>(this, "EditUUID")->getValue().asString();
        LLSD data;
        data["id"] = LLUUID::null;
        data["columns"][0]["column"] = "nameUUID";
        data["columns"][0]["value"] = getString("not_found", map);
        search_results->addElement(data);
        search_results->setEnabled(FALSE);
        get_floater_view(this, "ok_btn")->setEnabled(FALSE);
    }
}
// </FS:Ansariel>

void LLFloaterAvatarPicker::onBtnFind()
{
    find();
}

static void getSelectedAvatarData(const LLScrollListCtrl* from, uuid_vec_t& avatar_ids, std::vector<LLAvatarName>& avatar_names)
{
    std::vector<LLScrollListItem*> items = from->getAllSelected();
    for (std::vector<LLScrollListItem*>::iterator iter = items.begin(); iter != items.end(); ++iter)
    {
        LLScrollListItem* item = *iter;
        if (item->getUUID().notNull())
        {
            avatar_ids.push_back(item->getUUID());

            std::map<LLUUID, LLAvatarName>::iterator iter = sAvatarNameMap.find(item->getUUID());
            if (iter != sAvatarNameMap.end())
            {
                avatar_names.push_back(iter->second);
            }
            else
            {
                // the only case where it isn't in the name map is friends
                // but it should be in the name cache
                LLAvatarName av_name;
                LLAvatarNameCache::get(item->getUUID(), &av_name);
                avatar_names.push_back(av_name);
            }
        }
    }
}

void LLFloaterAvatarPicker::onBtnSelect()
{

    // If select btn not enabled then do not callback
    if (!isSelectBtnEnabled())
        return;

    if(mSelectionCallback)
    {
        std::string acvtive_panel_name;
        LLScrollListCtrl* list =  NULL;
        LLPanel* active_panel = get_floater_child<LLTabContainer>(this, "ResidentChooserTabs")->getCurrentPanel();
        if(active_panel)
        {
            acvtive_panel_name = active_panel->getName();
        }
        if(acvtive_panel_name == "SearchPanel")
        {
            list = get_floater_child<LLScrollListCtrl>(this, "SearchResults");
        }
        else if(acvtive_panel_name == "NearMePanel")
        {
            list = get_floater_child<LLScrollListCtrl>(this, "NearMe");
        }
        else if (acvtive_panel_name == "FriendsPanel")
        {
            list = get_floater_child<LLScrollListCtrl>(this, "Friends");
        }
        // <FS:Ansariel> Search by UUID
        else if (acvtive_panel_name == "SearchPanelUUID")
        {
            list = get_floater_child<LLScrollListCtrl>(this, "SearchResultsUUID");
        }
        // </FS:Ansariel>

        if(list)
        {
            uuid_vec_t          avatar_ids;
            std::vector<LLAvatarName>   avatar_names;
            getSelectedAvatarData(list, avatar_ids, avatar_names);
            mSelectionCallback(avatar_ids, avatar_names);
        }
    }
    get_floater_child<LLScrollListCtrl>(this, "SearchResults")->deselectAllItems(true);
    get_floater_child<LLScrollListCtrl>(this, "NearMe")->deselectAllItems(true);
    get_floater_child<LLScrollListCtrl>(this, "Friends")->deselectAllItems(true);
    // <FS:Ansariel> Search by UUID
    get_floater_child<LLScrollListCtrl>(this, "SearchResultsUUID")->deselectAllItems(TRUE);
    // </FS:Ansariel>
    if(mCloseOnSelect)
    {
        mCloseOnSelect = false;
        closeFloater();
    }
}

void LLFloaterAvatarPicker::onBtnRefresh()
{
    get_floater_child<LLScrollListCtrl>(this, "NearMe")->deleteAllItems();
    get_floater_child<LLScrollListCtrl>(this, "NearMe")->setCommentText(getString("searching"));
    mNearMeListComplete = false;
}

void LLFloaterAvatarPicker::onBtnClose()
{
    closeFloater();
}

void LLFloaterAvatarPicker::onRangeAdjust()
{
    onBtnRefresh();
}

void LLFloaterAvatarPicker::onList()
{
    get_floater_view(this, "ok_btn")->setEnabled(isSelectBtnEnabled());
}

void LLFloaterAvatarPicker::populateNearMe()
{
    bool all_loaded = true;
    bool empty = true;
    LLScrollListCtrl* near_me_scroller = get_floater_child<LLScrollListCtrl>(this, "NearMe");
    near_me_scroller->deleteAllItems();

//MK
    if (gRRenabled && (gAgent.mRRInterface.mContainsShownames || gAgent.mRRInterface.mContainsShownametags || gAgent.mRRInterface.mContainsShowNearby))
    {
        return;
    }
//mk

    uuid_vec_t avatar_ids;
    LLWorld::getInstance()->getAvatars(&avatar_ids, NULL, gAgent.getPositionGlobal(), gSavedSettings.getF32("NearMeRange"));
    for(U32 i=0; i<avatar_ids.size(); i++)
    {
        LLUUID& av = avatar_ids[i];
        if(mExcludeAgentFromSearchResults && (av == gAgent.getID())) continue;
        LLSD element;
        element["id"] = av; // value
        LLAvatarName av_name;

        if (!LLAvatarNameCache::get(av, &av_name))
        {
            element["columns"][0]["column"] = "name";
            element["columns"][0]["value"] = gCacheName->getDefaultName();
            all_loaded = false;
        }
        else
        {
            element["columns"][0]["column"] = "name";
            element["columns"][0]["value"] = av_name.getDisplayName();
            element["columns"][1]["column"] = "username";
            element["columns"][1]["value"] = av_name.getUserName();

            sAvatarNameMap[av] = av_name;
        }
        near_me_scroller->addElement(element);
        empty = false;
    }

    if (empty)
    {
        get_floater_view(this, "NearMe")->setEnabled(false);
        get_floater_view(this, "ok_btn")->setEnabled(false);
        near_me_scroller->setCommentText(getString("no_one_near"));
    }
    else
    {
        get_floater_view(this, "NearMe")->setEnabled(true);
        get_floater_view(this, "ok_btn")->setEnabled(true);
        near_me_scroller->selectFirstItem();
        onList();
        near_me_scroller->setFocus(true);
    }

    if (all_loaded)
    {
        mNearMeListComplete = true;
    }
}

void LLFloaterAvatarPicker::populateFriend()
{
    LLScrollListCtrl* friends_scroller = get_floater_child<LLScrollListCtrl>(this, "Friends");
    friends_scroller->deleteAllItems();
    // <FS:Ansariel> FIRE-16846: Make friend list sortable
    //LLCollectAllBuddies collector;
    //LLAvatarTracker::instance().applyFunctor(collector);
    //LLCollectAllBuddies::buddy_map_t::iterator it;
    //
    //for(it = collector.mOnline.begin(); it!=collector.mOnline.end(); it++)
    //{
    //  friends_scroller->addStringUUIDItem(it->second, it->first);
    //}
    //for(it = collector.mOffline.begin(); it!=collector.mOffline.end(); it++)
    //{
    //  friends_scroller->addStringUUIDItem(it->second, it->first);
    //}
    //friends_scroller->sortByColumnIndex(0, TRUE);

    LLAvatarTracker::buddy_map_t friend_list;
    LLAvatarTracker::instance().copyBuddyList(friend_list);
    for (LLAvatarTracker::buddy_map_t::iterator it = friend_list.begin(); it != friend_list.end(); ++it)
    {
        const LLUUID& av_id = it->first;

        LLSD element;
        element["id"] = av_id;

        LLAvatarName av_name;
        LLAvatarNameCache::get(av_id, &av_name); // Should have the name in the cache already
        element["columns"][0]["column"] = "name";
        element["columns"][0]["value"] = av_name.getDisplayName();
        element["columns"][1]["column"] = "username";
        element["columns"][1]["value"] = av_name.getUserName();

        friends_scroller->addElement(element);
    }
    friends_scroller->sortByColumnIndex(0, true);
    // </FS:Ansariel>
}

void LLFloaterAvatarPicker::drawFrustum()
{
    static LLCachedControl<F32> max_opacity(gSavedSettings, "PickerContextOpacity", 0.4f);
    drawConeToOwner(mContextConeOpacity, max_opacity, mFrustumOrigin.get(), mContextConeFadeTime, mContextConeInAlpha, mContextConeOutAlpha);
}

void LLFloaterAvatarPicker::draw()
{
    drawFrustum();

    // sometimes it is hard to determine when Select/Ok button should be disabled (see LLAvatarActions::shareWithAvatars).
    // lets check this via mOkButtonValidateSignal callback periodically.
    static LLFrameTimer timer;
    if (timer.hasExpired())
    {
        timer.setTimerExpirySec(0.33f); // three times per second should be enough.

        // simulate list changes.
        onList();
        timer.start();
    }

    LLFloater::draw();
    if (!mNearMeListComplete && get_floater_child<LLTabContainer>(this, "ResidentChooserTabs")->getCurrentPanel() == get_floater_child<LLPanel>(this, "NearMePanel"))
    {
        populateNearMe();
    }
}

bool LLFloaterAvatarPicker::visibleItemsSelected() const
{
    LLPanel* active_panel = get_floater_child<LLTabContainer>(this, "ResidentChooserTabs")->getCurrentPanel();

    if(active_panel == get_floater_child<LLPanel>(this, "SearchPanel"))
    {
        return get_floater_child<LLScrollListCtrl>(this, "SearchResults")->getFirstSelectedIndex() >= 0;
    }
    else if(active_panel == get_floater_child<LLPanel>(this, "NearMePanel"))
    {
        return get_floater_child<LLScrollListCtrl>(this, "NearMe")->getFirstSelectedIndex() >= 0;
    }
    else if(active_panel == get_floater_child<LLPanel>(this, "FriendsPanel"))
    {
        return get_floater_child<LLScrollListCtrl>(this, "Friends")->getFirstSelectedIndex() >= 0;
    }
    // <FS:Ansariel> Search by UUID
    else if (active_panel == get_floater_child<LLPanel>(this, "SearchPanelUUID"))
    {
        return get_floater_child<LLScrollListCtrl>(this, "SearchResultsUUID")->getFirstSelectedIndex() >= 0;
    }
    // </FS:Ansariel>
    return false;
}

/*static*/
void LLFloaterAvatarPicker::findByIdCoro(std::string url, LLUUID query_id, LLUUID agent_id, std::string floater_key)
{
    LLCore::HttpRequest::policy_t httpPolicy(LLCore::HttpRequest::DEFAULT_POLICY_ID);
    LLCoreHttpUtil::HttpCoroutineAdapter::ptr_t
        httpAdapter = std::make_shared<LLCoreHttpUtil::HttpCoroutineAdapter>("findByIdCoro", httpPolicy);
    LLCore::HttpRequest::ptr_t httpRequest = std::make_shared<LLCore::HttpRequest>();
    LLCore::HttpOptions::ptr_t httpOpts = std::make_shared<LLCore::HttpOptions>();

    httpOpts->setTimeout(AVATAR_PICKER_SEARCH_TIMEOUT);

    LLSD result = httpAdapter->getAndSuspend(httpRequest, url, httpOpts);

    LL_DEBUGS("Agent") << result << LL_ENDL;

    LLSD httpResults = result[LLCoreHttpUtil::HttpCoroutineAdapter::HTTP_RESULTS];
    LLCore::HttpStatus status = LLCoreHttpUtil::HttpCoroutineAdapter::getStatusFromLLSD(httpResults);

    if (status || (status == LLCore::HttpStatus(HTTP_BAD_REQUEST)))
    {
        result.erase(LLCoreHttpUtil::HttpCoroutineAdapter::HTTP_RESULTS);
    }
    else
    {
        result["failure_reason"] = status.toString();
    }

    LLFloaterAvatarPicker* floater =
        LLFloaterReg::findTypedInstance<LLFloaterAvatarPicker>("avatar_picker", floater_key);
    if (floater)
    {
        floater->processResponse(query_id, result);
    }
}

/*static*/
void LLFloaterAvatarPicker::findByNameCoro(std::string url, LLUUID queryID, std::string name)
{
    LLCore::HttpRequest::policy_t httpPolicy(LLCore::HttpRequest::DEFAULT_POLICY_ID);
    LLCoreHttpUtil::HttpCoroutineAdapter::ptr_t
        httpAdapter = std::make_shared<LLCoreHttpUtil::HttpCoroutineAdapter>("findByNameCoro", httpPolicy);
    LLCore::HttpRequest::ptr_t httpRequest = std::make_shared<LLCore::HttpRequest>();
    LLCore::HttpOptions::ptr_t httpOpts = std::make_shared<LLCore::HttpOptions>();

    LL_INFOS("HttpCoroutineAdapter", "genericPostCoro", "Agent") << "Generic POST for " << url << LL_ENDL;

    httpOpts->setTimeout(AVATAR_PICKER_SEARCH_TIMEOUT);

    LLSD result = httpAdapter->getAndSuspend(httpRequest, url, httpOpts);

    LLSD httpResults = result[LLCoreHttpUtil::HttpCoroutineAdapter::HTTP_RESULTS];
    LLCore::HttpStatus status = LLCoreHttpUtil::HttpCoroutineAdapter::getStatusFromLLSD(httpResults);

    if (status || (status == LLCore::HttpStatus(HTTP_BAD_REQUEST)))
    {
        result.erase(LLCoreHttpUtil::HttpCoroutineAdapter::HTTP_RESULTS);
    }
    else
    {
        result["failure_reason"] = status.toString();
    }

    LLFloaterAvatarPicker* floater =
        LLFloaterReg::findTypedInstance<LLFloaterAvatarPicker>("avatar_picker", name);
    if (floater)
    {
        floater->processResponse(queryID, result);
    }
}


void LLFloaterAvatarPicker::find()
{
    //clear our stored LLAvatarNames
    sAvatarNameMap.clear();

    std::string text = get_floater_child<LLUICtrl>(this, "Edit")->getValue().asString();

    LLUUID agent_id;
    size_t separator_index = text.find_first_of(" ._");
    if (separator_index != text.npos)
    {
        std::string first = text.substr(0, separator_index);
        std::string last = text.substr(separator_index+1, text.npos);
        LLStringUtil::trim(last);
        if("Resident" == last)
        {
            text = first;
        }
    }
    else if (!text.empty())
    {
        agent_id.set(text);
    }

    mQueryID.generate();
    mNumResultsReturned = 0;

    get_floater_child<LLScrollListCtrl>(this, "SearchResults")->deleteAllItems();
    get_floater_child<LLScrollListCtrl>(this, "SearchResults")->setCommentText(getString("searching"));
    get_floater_view(this, "ok_btn")->setEnabled(false);

    if (agent_id.notNull())
    {
        // Search by uuid
        // While cache could have been nicer, it neither has a failure callback, nor
        // can cleanup in case of an invalid uuid. So we go directly to the capability.
        LLViewerRegion* region = gAgent.getRegion();
        if (region)
        {
            std::string url;
            url.reserve(128);
            url = region->getCapability("GetDisplayNames");
            if (!url.empty())
            {
                // capability urls don't end in '/', but we need one to parse
                // query parameters correctly
                if (url[url.size() - 1] != '/')
                {
                    url += "/";
                }
                url += "?ids=";
                url += agent_id.asString();
                LL_DEBUGS("Agent") << "avatar picker " << url << LL_ENDL;

                LLCoros::instance().launch("LLFloaterAvatarPicker::findCoro",
                    boost::bind(&LLFloaterAvatarPicker::findByIdCoro, url, mQueryID, agent_id, getKey().asString()));
            }
            else
            {
                LLSD content;
                content["failure_reason"] = LLTrans::getString("ServerUnavailable");
                processResponse(mQueryID, content);
            }
        }
    }
    else
    {
        std::string url;
        url.reserve(128); // avoid a memory allocation or two

        LLViewerRegion* region = gAgent.getRegion();
        if (region)
        {
            url = region->getCapability("AvatarPickerSearch");
            // Prefer use of capabilities to search on both SLID and display name
            if (!url.empty())
            {
                // capability urls don't end in '/', but we need one to parse
                // query parameters correctly
                if (url.size() > 0 && url[url.size() - 1] != '/')
                {
                    url += "/";
                }
                url += "?page_size=100&names=";
                std::replace(text.begin(), text.end(), '.', ' ');
                url += LLURI::escape(text);
                LL_DEBUGS("Agent") << "avatar picker " << url << LL_ENDL;

                LLCoros::instance().launch("LLFloaterAvatarPicker::findCoro",
                    boost::bind(&LLFloaterAvatarPicker::findByNameCoro, url, mQueryID, getKey().asString()));
            }
            else
            {
                LLMessageSystem* msg = gMessageSystem;
                msg->newMessage("AvatarPickerRequest");
                msg->nextBlock("AgentData");
                msg->addUUID("AgentID", gAgent.getID());
                msg->addUUID("SessionID", gAgent.getSessionID());
                msg->addUUID("QueryID", mQueryID);  // not used right now
                msg->nextBlock("Data");
                msg->addString("Name", text);
                gAgent.sendReliableMessage();
            }
        }
    }
}

void LLFloaterAvatarPicker::setAllowMultiple(bool allow_multiple)
{
    get_floater_child<LLScrollListCtrl>(this, "SearchResults")->setAllowMultipleSelection(allow_multiple);
    get_floater_child<LLScrollListCtrl>(this, "NearMe")->setAllowMultipleSelection(allow_multiple);
    get_floater_child<LLScrollListCtrl>(this, "Friends")->setAllowMultipleSelection(allow_multiple);
    // <FS:Ansariel> Search by UUID
    get_floater_child<LLScrollListCtrl>(this, "SearchResultsUUID")->setAllowMultipleSelection(allow_multiple);
}

LLScrollListCtrl* LLFloaterAvatarPicker::getActiveList()
{
    std::string acvtive_panel_name;
    LLScrollListCtrl* list = NULL;
    LLPanel* active_panel = get_floater_child<LLTabContainer>(this, "ResidentChooserTabs")->getCurrentPanel();
    if(active_panel)
    {
        acvtive_panel_name = active_panel->getName();
    }
    if(acvtive_panel_name == "SearchPanel")
    {
        list = get_floater_child<LLScrollListCtrl>(this, "SearchResults");
    }
    else if(acvtive_panel_name == "NearMePanel")
    {
        list = get_floater_child<LLScrollListCtrl>(this, "NearMe");
    }
    else if (acvtive_panel_name == "FriendsPanel")
    {
        list = get_floater_child<LLScrollListCtrl>(this, "Friends");
    }
    // <FS:Ansariel> Search by UUID
    else if (acvtive_panel_name == "SearchPanelUUID")
    {
        list = get_floater_child<LLScrollListCtrl>(this, "SearchResultsUUID");
    }
    // </FS:Ansariel>
    return list;
}

bool LLFloaterAvatarPicker::handleDragAndDrop(S32 x, S32 y, MASK mask,
                                              bool drop, EDragAndDropType cargo_type,
                                              void *cargo_data, EAcceptance *accept,
                                              std::string& tooltip_msg)
{
    LLScrollListCtrl* list = getActiveList();
    if(list)
    {
        LLRect rc_list;
        LLRect rc_point(x,y,x,y);
        if (localRectToOtherView(rc_point, &rc_list, list))
        {
            // Keep selected only one item
            list->deselectAllItems(true);
            list->selectItemAt(rc_list.mLeft, rc_list.mBottom, mask);
            LLScrollListItem* selection = list->getFirstSelected();
            if (selection)
            {
                LLUUID session_id = LLUUID::null;
                LLUUID dest_agent_id = selection->getUUID();
                std::string avatar_name = selection->getColumn(0)->getValue().asString();
                if (dest_agent_id.notNull() && dest_agent_id != gAgentID)
                {
                    if (drop)
                    {
                        // Start up IM before give the item
                        session_id = gIMMgr->addSession(avatar_name, IM_NOTHING_SPECIAL, dest_agent_id);
                    }
                    return LLToolDragAndDrop::handleGiveDragAndDrop(dest_agent_id, session_id, drop,
                                                                    cargo_type, cargo_data, accept, getName());
                }
            }
        }
    }
    *accept = ACCEPT_NO;
    return true;
}


void LLFloaterAvatarPicker::openFriendsTab()
{
    LLTabContainer* tab_container = get_floater_child<LLTabContainer>(this, "ResidentChooserTabs");
    if (tab_container == NULL)
    {
        llassert(tab_container != NULL);
        return;
    }

    tab_container->selectTabByName("FriendsPanel");
}

// static
void LLFloaterAvatarPicker::processAvatarPickerReply(LLMessageSystem* msg, void**)
{
    LLUUID  agent_id;
    LLUUID  query_id;
    LLUUID  avatar_id;
    std::string first_name;
    std::string last_name;

    msg->getUUID("AgentData", "AgentID", agent_id);
    msg->getUUID("AgentData", "QueryID", query_id);

    // Not for us
    if (agent_id != gAgent.getID()) return;

    LLFloaterAvatarPicker* floater = LLFloaterReg::findTypedInstance<LLFloaterAvatarPicker>("avatar_picker");

    // floater is closed or these are not results from our last request
    if (NULL == floater || query_id != floater->mQueryID)
    {
        return;
    }

    LLScrollListCtrl* search_results = get_floater_child<LLScrollListCtrl>(floater, "SearchResults");

    // clear "Searching" label on first results
    if (floater->mNumResultsReturned++ == 0)
    {
        search_results->deleteAllItems();
    }

    bool found_one = false;
    S32 num_new_rows = msg->getNumberOfBlocks("Data");
    for (S32 i = 0; i < num_new_rows; i++)
    {
        msg->getUUIDFast(  _PREHASH_Data,_PREHASH_AvatarID, avatar_id, i);
        msg->getStringFast(_PREHASH_Data,_PREHASH_FirstName, first_name, i);
        msg->getStringFast(_PREHASH_Data,_PREHASH_LastName, last_name, i);

        if (avatar_id != agent_id || !floater->isExcludeAgentFromSearchResults()) // exclude agent from search results?
        {
            std::string avatar_name;
            if (avatar_id.isNull())
            {
                LLStringUtil::format_map_t map;
                map["[TEXT]"] = get_floater_child<LLUICtrl>(floater, "Edit")->getValue().asString();
                avatar_name = floater->getString("not_found", map);
                search_results->setEnabled(false);
                get_floater_view(floater, "ok_btn")->setEnabled(false);
            }
            else
            {
                avatar_name = LLCacheName::buildFullName(first_name, last_name);
                search_results->setEnabled(true);
                found_one = true;

                LLAvatarName av_name;
                av_name.fromString(avatar_name);
                const LLUUID& agent_id = avatar_id;
                sAvatarNameMap[agent_id] = av_name;

            }
            LLSD element;
            element["id"] = avatar_id; // value
            element["columns"][0]["column"] = "name";
            element["columns"][0]["value"] = avatar_name;
            search_results->addElement(element);
        }
    }

    if (found_one)
    {
        get_floater_view(floater, "ok_btn")->setEnabled(true);
        search_results->selectFirstItem();
        floater->onList();
        search_results->setFocus(true);
    }
}

void LLFloaterAvatarPicker::processResponse(const LLUUID& query_id, const LLSD& content)
{
    // Check for out-of-date query
    if (query_id == mQueryID)
    {
        LLScrollListCtrl* search_results = get_floater_child<LLScrollListCtrl>(this, "SearchResults");

        // clear "Searching" label on first results
        search_results->deleteAllItems();

        if (content.has("failure_reason"))
        {
            get_floater_child<LLScrollListCtrl>(this, "SearchResults")->setCommentText(content["failure_reason"].asString());
            get_floater_view(this, "ok_btn")->setEnabled(false);
        }
        else
        {
            LLSD agents = content["agents"];

            LLSD item;
            LLSD::array_const_iterator it = agents.beginArray();
            for (; it != agents.endArray(); ++it)
            {
                const LLSD& row = *it;
                if (row["id"].asUUID() != gAgent.getID() || !mExcludeAgentFromSearchResults)
                {
                    item["id"] = row["id"];
                    LLSD& columns = item["columns"];
                    columns[0]["column"] = "name";
                    columns[0]["value"] = row["display_name"];
                    columns[1]["column"] = "username";
                    columns[1]["value"] = row["username"];
                    search_results->addElement(item);

                    // add the avatar name to our list
                    LLAvatarName avatar_name;
                    avatar_name.fromLLSD(row);
                    sAvatarNameMap[row["id"].asUUID()] = avatar_name;
                }
            }

            if (search_results->isEmpty())
            {
                std::string name = "'" + get_floater_child<LLUICtrl>(this, "Edit")->getValue().asString() + "'";
                LLSD item;
                item["id"] = LLUUID::null;
                item["columns"][0]["column"] = "name";
                item["columns"][0]["value"] = name;
                item["columns"][1]["column"] = "username";
                item["columns"][1]["value"] = getString("not_found_text");
                search_results->addElement(item);
                search_results->setEnabled(false);
                get_floater_view(this, "ok_btn")->setEnabled(false);
            }
            else
            {
                get_floater_view(this, "ok_btn")->setEnabled(true);
                search_results->setEnabled(true);
                search_results->sortByColumnIndex(1, true);
                std::string text = get_floater_child<LLUICtrl>(this, "Edit")->getValue().asString();
                if (!search_results->selectItemByLabel(text, true, 1))
                {
                    search_results->selectFirstItem();
                }
                onList();
                search_results->setFocus(true);
            }
        }
    }
}

void LLFloaterAvatarPicker::editKeystroke(LLLineEditor* caller, void* user_data)
{
    get_floater_view(this, "Find")->setEnabled(caller->getText().size() > 0);
}

// <FS:Ansariel> Search by UUID
void LLFloaterAvatarPicker::editKeystrokeUUID(LLLineEditor* caller, void* user_data)
{
    if (caller)
    {
        LLUUID id(caller->getText());
        get_floater_view(this, "FindUUID")->setEnabled(!id.isNull());
    }
}
// </FS:Ansariel>

// virtual
bool LLFloaterAvatarPicker::handleKeyHere(KEY key, MASK mask)
{
    if (key == KEY_RETURN && mask == MASK_NONE)
    {
        if (get_floater_child<LLUICtrl>(this, "Edit")->hasFocus())
        {
            onBtnFind();
        }
        // <FS:Ansariel> Search by UUID
        else if (get_floater_child<LLUICtrl>(this, "EditUUID")->hasFocus())
        {
            onBtnFindUUID();
        }
        // </FS:Ansariel>
        else
        {
            onBtnSelect();
        }
        return true;
    }
    else if (key == KEY_ESCAPE && mask == MASK_NONE)
    {
        closeFloater();
        return true;
    }

    return LLFloater::handleKeyHere(key, mask);
}

bool LLFloaterAvatarPicker::isSelectBtnEnabled()
{
    bool ret_val = visibleItemsSelected();

    if ( ret_val && !isMinimized())
    {
        std::string acvtive_panel_name;
        LLScrollListCtrl* list =  NULL;
        LLPanel* active_panel = get_floater_child<LLTabContainer>(this, "ResidentChooserTabs")->getCurrentPanel();

        if(active_panel)
        {
            acvtive_panel_name = active_panel->getName();
        }

        if(acvtive_panel_name == "SearchPanel")
        {
            list = get_floater_child<LLScrollListCtrl>(this, "SearchResults");
        }
        else if(acvtive_panel_name == "NearMePanel")
        {
            list = get_floater_child<LLScrollListCtrl>(this, "NearMe");
        }
        else if (acvtive_panel_name == "FriendsPanel")
        {
            list = get_floater_child<LLScrollListCtrl>(this, "Friends");
        }
        // <FS:Ansariel> Search by UUID
        else if (acvtive_panel_name == "SearchPanelUUID")
        {
            list = get_floater_child<LLScrollListCtrl>(this, "SearchResultsUUID");
        }
        // </FS:Ansariel>

        if(list)
        {
            uuid_vec_t avatar_ids;
            std::vector<LLAvatarName> avatar_names;
            getSelectedAvatarData(list, avatar_ids, avatar_names);
            if (avatar_ids.size() >= 1)
            {
                ret_val = mOkButtonValidateSignal.num_slots()?mOkButtonValidateSignal(avatar_ids):true;
            }
            else
            {
                ret_val = false;
            }
        }
    }

    return ret_val;
}
