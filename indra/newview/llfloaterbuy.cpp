/**
 * @file llfloaterbuy.cpp
 * @author James Cook
 * @brief LLFloaterBuy class implementation
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

/**
 * Floater that appears when buying an object, giving a preview
 * of its contents and their permissions.
 */

#include "llviewerprecompiledheaders.h"

#include "llfloaterbuy.h"

#include "llagent.h"            // for agent id
#include "llinventorymodel.h"   // for gInventory
#include "llfloaterreg.h"
#include "llinventoryicon.h"
#include "llinventorydefines.h"
#include "llinventoryfunctions.h"
#include "llnotificationsutil.h"
#include "llselectmgr.h"
#include "llscrolllistctrl.h"
#include "llviewerobject.h"
#include "lluictrlfactory.h"
#include "llviewerwindow.h"
#include "lltrans.h"

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

LLFloaterBuy::LLFloaterBuy(const LLSD& key)
:   LLFloater(key),
    mSelectionUpdateSlot()
{
}

bool LLFloaterBuy::postBuild()
{
    setupControls();

    // Always center the dialog.  User can change the size,
    // but purchases are important and should be center screen.
    // This also avoids problems where the user resizes the application window
    // mid-session and the saved rect is off-center.
    center();

    return true;
}

void LLFloaterBuy::setupControls()
{
    get_floater_view(this, "object_list")->setEnabled(false);
    get_floater_view(this, "item_list")->setEnabled(false);

    get_floater_child<LLUICtrl>(this, "cancel_btn")->setCommitCallback( boost::bind(&LLFloaterBuy::onClickCancel, this));
    get_floater_child<LLUICtrl>(this, "buy_btn")->setCommitCallback( boost::bind(&LLFloaterBuy::onClickBuy, this));

    setDefaultBtn("cancel_btn"); // to avoid accidental buy (SL-43130)
}

LLFloaterBuy::~LLFloaterBuy()
{
    mObjectSelection = nullptr;
}

void LLFloaterBuy::reset()
{
    resetList("object_list");
    resetList("item_list");
}

void LLFloaterBuy::resetList(const std::string& list_name)
{
    LLScrollListCtrl* list = get_floater_child<LLScrollListCtrl>(this, list_name);
    if (list)
    {
        list->deleteAllItems();
    }
}

void LLFloaterBuy::syncBuyTitle(const LLSaleInfo& sale_info, const std::string& object_name)
{
    LLUIString title;
    switch (sale_info.getSaleType())
    {
      case LLSaleInfo::FS_ORIGINAL:
        title = getString("title_buy_text");
        break;
      case LLSaleInfo::FS_COPY:
      default:
        title = getString("title_buy_copy_text");
        break;
    }
    title.setArg("[NAME]", object_name);
    setTitle(title);
}

void LLFloaterBuy::syncBuyText(S32 price, const std::string& owner_name)
{
    get_floater_child<LLUICtrl>(this, "buy_text")->setTextArg("[AMOUNT]", llformat("%d", price));
    get_floater_child<LLUICtrl>(this, "buy_name_text")->setTextArg("[NAME]", owner_name);
}

// static
void LLFloaterBuy::show(const LLSaleInfo& sale_info)
{
    LLObjectSelectionHandle selection = LLSelectMgr::getInstance()->getSelection();

    if (selection->getRootObjectCount() != 1)
    {
        LLNotificationsUtil::add("BuyOneObjectOnly");
        return;
    }

    LLFloaterBuy* floater = LLFloaterReg::showTypedInstance<LLFloaterBuy>("buy_object");
    if (!floater)
        return;

    // Clean up the lists...
    floater->reset();
    floater->mSaleInfo = sale_info;
    floater->mObjectSelection = LLSelectMgr::getInstance()->getEditSelection();

    LLSelectNode* node = selection->getFirstRootNode();
    if (!node)
        return;

    floater->syncBuyTitle(sale_info, node->mName);

    LLUUID owner_id;
    std::string owner_name;
    bool owners_identical = LLSelectMgr::getInstance()->selectGetOwner(owner_id, owner_name);
    if (!owners_identical)
    {
        LLNotificationsUtil::add("BuyObjectOneOwner");
        return;
    }

    LLCtrlListInterface *object_list = floater->childGetListInterface("object_list");
    if (!object_list)
    {
        return;
    }

    // Update the display
    // Display next owner permissions
    LLSD row;

    // Compute icon for this item
    std::string icon_name = LLInventoryIcon::getIconName(LLAssetType::AT_OBJECT,
                                     LLInventoryType::IT_OBJECT);

    row["columns"][0]["column"] = "icon";
    row["columns"][0]["type"] = "icon";
    row["columns"][0]["value"] = icon_name;

    // Append the permissions that you will acquire (not the current
    // permissions).
    U32 next_owner_mask = node->mPermissions->getMaskNextOwner();
    std::string text = node->mName;
    if (!(next_owner_mask & PERM_COPY))
    {
        text.append(floater->getString("no_copy_text"));
    }
    if (!(next_owner_mask & PERM_MODIFY))
    {
        text.append(floater->getString("no_modify_text"));
    }
    if (!(next_owner_mask & PERM_TRANSFER))
    {
        text.append(floater->getString("no_transfer_text"));
    }

    row["columns"][1]["column"] = "text";
    row["columns"][1]["value"] = text;
    row["columns"][1]["font"] = "SANSSERIF";

    // Add after columns added so appropriate heights are correct.
    object_list->addElement(row);

    floater->syncBuyText(sale_info.getSalePrice(), owner_name);

    floater->showViews(true);

    // Must do this after the floater is created, because
    // sometimes the inventory is already there and
    // the callback is called immediately.
    LLViewerObject* obj = selection->getFirstRootObject();
    floater->registerVOInventoryListener(obj,NULL);
    floater->requestVOInventory();

    if (!floater->mSelectionUpdateSlot.connected())
    {
        floater->mSelectionUpdateSlot = LLSelectMgr::getInstance()->mUpdateSignal.connect(boost::bind(&LLFloaterBuy::onSelectionChanged, floater));
    }
}

void LLFloaterBuy::inventoryChanged(LLViewerObject* obj,
                                 LLInventoryObject::object_list_t* inv,
                                 S32 serial_num,
                                 void* data)
{
    if (!obj)
    {
        LL_WARNS() << "No object in LLFloaterBuy::inventoryChanged" << LL_ENDL;
        return;
    }

    if (!inv)
    {
        LL_WARNS() << "No inventory in LLFloaterBuy::inventoryChanged"
            << LL_ENDL;
        removeVOInventoryListener();
        return;
    }

    LLCtrlListInterface *item_list = childGetListInterface("item_list");
    if (!item_list)
    {
        removeVOInventoryListener();
        return;
    }

    LLInventoryObject::object_list_t::const_iterator it = inv->begin();
    LLInventoryObject::object_list_t::const_iterator end = inv->end();
    for ( ; it != end; ++it )
    {
        LLInventoryObject* obj = (LLInventoryObject*)(*it);

        // Skip folders, so we know we have inventory items only
        if (obj->getType() == LLAssetType::AT_CATEGORY)
            continue;

        // Skip the mysterious blank InventoryObject
        if (obj->getType() == LLAssetType::AT_NONE)
            continue;


        LLInventoryItem* inv_item = (LLInventoryItem*)(obj);

        // Skip items we can't transfer
        if (!inv_item->getPermissions().allowTransferTo(gAgent.getID()))
            continue;

        // Create the line in the list
        LLSD row;

        // Compute icon for this item
        bool item_is_multi = false;
        if (( inv_item->getFlags() & LLInventoryItemFlags::II_FLAGS_LANDMARK_VISITED
            || inv_item->getFlags() & LLInventoryItemFlags::II_FLAGS_OBJECT_HAS_MULTIPLE_ITEMS)
            && !(inv_item->getFlags() & LLInventoryItemFlags::II_FLAGS_SUBTYPE_MASK))
        {
            item_is_multi = true;
        }

        std::string icon_name = LLInventoryIcon::getIconName(inv_item->getType(),
                             inv_item->getInventoryType(),
                             inv_item->getFlags(),
                             item_is_multi);
        row["columns"][0]["column"] = "icon";
        row["columns"][0]["type"] = "icon";
        row["columns"][0]["value"] = icon_name;

        // Append the permissions that you will acquire (not the current
        // permissions).
        U32 next_owner_mask = inv_item->getPermissions().getMaskNextOwner();
        std::string text = obj->getName();
        if (!(next_owner_mask & PERM_COPY))
        {
            text.append(LLTrans::getString("no_copy"));
        }
        if (!(next_owner_mask & PERM_MODIFY))
        {
            text.append(LLTrans::getString("no_modify"));
        }
        if (!(next_owner_mask & PERM_TRANSFER))
        {
            text.append(LLTrans::getString("no_transfer"));
        }

        row["columns"][1]["column"] = "text";
        row["columns"][1]["value"] = text;
        row["columns"][1]["font"] = "SANSSERIF";

        item_list->addElement(row);
    }
    removeVOInventoryListener();
}

void LLFloaterBuy::onSelectionChanged()
{

    if (LLSelectMgr::getInstance()->getEditSelection()->getRootObjectCount() == 0)
    {
        removeVOInventoryListener();
        closeFloater();
    }
    else if (LLSelectMgr::getInstance()->getEditSelection()->getRootObjectCount() > 1)
    {
        removeVOInventoryListener();
        showViews(false);
        reset();
        setTitle(getString("mupliple_selected"));
    }
}

void LLFloaterBuy::showViews(bool show)
{
    get_floater_child<LLUICtrl>(this, "buy_btn")->setEnabled(show);
    get_floater_child<LLUICtrl>(this, "buy_text")->setVisible(show);
    get_floater_child<LLUICtrl>(this, "buy_name_text")->setVisible(show);
}

void LLFloaterBuy::onClickBuy()
{
    // Put the items where we put new folders.
    LLUUID category_id;
    category_id = gInventory.findCategoryUUIDForType(LLFolderType::FT_OBJECT);

    // *NOTE: doesn't work for multiple object buy, which UI does not
    // currently support sale info is used for verification only, if
    // it doesn't match region info then sale is canceled.
    LLSelectMgr::getInstance()->sendBuy(gAgent.getID(), category_id, mSaleInfo );

    closeFloater();
}


void LLFloaterBuy::onClickCancel()
{
    closeFloater();
}

// virtual
void LLFloaterBuy::onClose(bool app_quitting)
{
    if (mSelectionUpdateSlot.connected())
    {
        mSelectionUpdateSlot.disconnect();
    }

    mObjectSelection.clear();
}
