/**
 * @file llfloaternamedesc.cpp
 * @brief LLFloaterNameDesc class implementation
 *
 * $LicenseInfo:firstyear=2002&license=viewerlgpl$
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

#include "llfloaternamedesc.h"

// project includes
#include "lllineeditor.h"
#include "llresmgr.h"
#include "lltextbox.h"
#include "llbutton.h"
#include "llviewerwindow.h"
#include "llfocusmgr.h"
#include "llrootview.h"
#include "llradiogroup.h"
#include "lldbstrings.h"
#include "lldir.h"
#include "llfloaterperms.h"
#include "llviewercontrol.h"
#include "llviewermenufile.h"   // upload_new_resource()
#include "llstatusbar.h"    // can_afford_transaction()
#include "llnotificationsutil.h"
#include "lluictrlfactory.h"
#include "llstring.h"
#include "llpermissions.h"
#include "lltrans.h"

// linden includes
#include "llassetstorage.h"
#include "llinventorytype.h"
#include "llagentbenefits.h"

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

const S32 PREVIEW_LINE_HEIGHT = 19;
const S32 PREVIEW_BORDER_WIDTH = 2;
const S32 PREVIEW_RESIZE_HANDLE_SIZE = S32(RESIZE_HANDLE_WIDTH * OO_SQRT2) + PREVIEW_BORDER_WIDTH;
const S32 PREVIEW_HPAD = PREVIEW_RESIZE_HANDLE_SIZE;

//-----------------------------------------------------------------------------
// LLFloaterNameDesc()
//-----------------------------------------------------------------------------
LLFloaterNameDesc::LLFloaterNameDesc(const LLSD& args)
    : LLFloater(args)
    , mIsAudio(false)
    , mIsText(false)
{
    if (args.isString())
    {
        mFilenameAndPath = args.asString();
    }
    else
    {
        mFilenameAndPath = args["filename"].asString();
        mDestinationFolderId = args["dest"].asUUID();
    }
    mFilename = gDirUtilp->getBaseFileName(mFilenameAndPath, false);
}

//-----------------------------------------------------------------------------
// postBuild()
//-----------------------------------------------------------------------------
bool LLFloaterNameDesc::postBuild()
{
    LLRect r;

    std::string asset_name = mFilename;
    LLStringUtil::replaceNonstandardASCII( asset_name, '?' );
    LLStringUtil::replaceChar(asset_name, '|', '?');
    LLStringUtil::stripNonprintable(asset_name);
    LLStringUtil::trim(asset_name);

    asset_name = gDirUtilp->getBaseFileName(asset_name, true); // no extsntion

    setTitle(mFilename);

    centerWithin(gViewerWindow->getRootView()->getRect());

    S32 line_width = getRect().getWidth() - 2 * PREVIEW_HPAD;
    S32 y = getRect().getHeight() - PREVIEW_LINE_HEIGHT;

    r.setLeftTopAndSize( PREVIEW_HPAD, y, line_width, PREVIEW_LINE_HEIGHT );
    y -= PREVIEW_LINE_HEIGHT;

    r.setLeftTopAndSize( PREVIEW_HPAD, y, line_width, PREVIEW_LINE_HEIGHT );

    setupNameField(asset_name);

    y -= llfloor(PREVIEW_LINE_HEIGHT * 1.2f);
    y -= PREVIEW_LINE_HEIGHT;

    r.setLeftTopAndSize( PREVIEW_HPAD, y, line_width, PREVIEW_LINE_HEIGHT );
    setupDescriptionField();

    y -= llfloor(PREVIEW_LINE_HEIGHT * 1.2f);

    // Cancel button
    get_floater_child<LLUICtrl>(this, "cancel_btn")->setCommitCallback(boost::bind(&LLFloaterNameDesc::onBtnCancel, this));

    setupUploadCostControls(getExpectedUploadCost());

    setDefaultBtn("ok_btn");

    return true;
}

S32 LLFloaterNameDesc::getExpectedUploadCost() const
{
    std::string exten = gDirUtilp->getExtension(mFilename);
    LLAssetType::EType asset_type;
    S32 upload_cost = -1;
    if (LLResourceUploadInfo::findAssetTypeOfExtension(exten, asset_type))
    {
        if (!LLAgentBenefitsMgr::current().findUploadCost(asset_type, upload_cost))
        {
            LL_WARNS() << "Unable to find upload cost for asset type " << asset_type << LL_ENDL;
        }
    }
    else
    {
        LL_WARNS() << "Unable to find upload cost for " << mFilename << LL_ENDL;
    }
    return upload_cost;
}

void LLFloaterNameDesc::setupNameField(const std::string& asset_name)
{
    get_floater_child<LLUICtrl>(this, "name_form")->setCommitCallback(boost::bind(&LLFloaterNameDesc::doCommit, this));
    get_floater_child<LLUICtrl>(this, "name_form")->setValue(LLSD(asset_name));

    LLLineEditor* name_editor = get_floater_child<LLLineEditor>(this, "name_form");
    if (name_editor)
    {
        name_editor->setMaxTextLength(DB_INV_ITEM_NAME_STR_LEN);
        name_editor->setPrevalidate(&LLTextValidate::validateASCIIPrintableNoPipe);
    }
}

void LLFloaterNameDesc::setupDescriptionField()
{
    get_floater_child<LLUICtrl>(this, "description_form")->setCommitCallback(boost::bind(&LLFloaterNameDesc::doCommit, this));
    LLLineEditor* desc_editor = get_floater_child<LLLineEditor>(this, "description_form");
    if (desc_editor)
    {
        desc_editor->setMaxTextLength(DB_INV_ITEM_DESC_STR_LEN);
        desc_editor->setPrevalidate(&LLTextValidate::validateASCIIPrintableNoPipe);
    }
}

void LLFloaterNameDesc::setupUploadCostControls(S32 expected_upload_cost)
{
    get_floater_child<LLUICtrl>(this, "ok_btn")->setLabelArg("[AMOUNT]", llformat("%d", expected_upload_cost));

    LLTextBox* info_text = get_floater_child<LLTextBox>(this, "info_text");
    if (info_text)
    {
        info_text->setValue(LLTrans::getString("UploadFeeInfo"));
    }
}

void LLFloaterNameDesc::setupUploadCommitAction()
{
    get_floater_child<LLUICtrl>(this, "ok_btn")->setCommitCallback(boost::bind(&LLFloaterNameDesc::onBtnOK, this));
}

void LLFloaterNameDesc::setUploadButtonEnabled(bool enabled)
{
    get_floater_view(this, "ok_btn")->setEnabled(enabled);
}

std::string LLFloaterNameDesc::getUploadName()
{
    return get_floater_child<LLUICtrl>(this, "name_form")->getValue().asString();
}

std::string LLFloaterNameDesc::getUploadDescription()
{
    return get_floater_child<LLUICtrl>(this, "description_form")->getValue().asString();
}

//-----------------------------------------------------------------------------
// LLFloaterNameDesc()
//-----------------------------------------------------------------------------
LLFloaterNameDesc::~LLFloaterNameDesc()
{
    gFocusMgr.releaseFocusIfNeeded( this ); // calls onCommit()
}

// Sub-classes should override this function if they allow editing
//-----------------------------------------------------------------------------
// onCommit()
//-----------------------------------------------------------------------------
void LLFloaterNameDesc::onCommit()
{
}

//-----------------------------------------------------------------------------
// onCommit()
//-----------------------------------------------------------------------------
void LLFloaterNameDesc::doCommit()
{
    onCommit();
}

//-----------------------------------------------------------------------------
// onBtnOK()
//-----------------------------------------------------------------------------
void LLFloaterNameDesc::onBtnOK( )
{
    setUploadButtonEnabled(false); // don't allow inadvertent extra uploads

    LLAssetStorage::LLStoreAssetCallback callback;
    S32 expected_upload_cost = getExpectedUploadCost();
    if (can_afford_transaction(expected_upload_cost))
    {
        void *nruserdata = NULL;
        std::string display_name = LLStringUtil::null;

        LLResourceUploadInfo::ptr_t uploadInfo(std::make_shared<LLNewFileResourceUploadInfo>(
            mFilenameAndPath,
            getUploadName(),
            getUploadDescription(), 0,
            LLFolderType::FT_NONE, LLInventoryType::IT_NONE,
            LLFloaterPerms::getNextOwnerPerms("Uploads"),
            LLFloaterPerms::getGroupPerms("Uploads"),
            LLFloaterPerms::getEveryonePerms("Uploads"),
            expected_upload_cost,
            mDestinationFolderId));

        upload_new_resource(uploadInfo, callback, nruserdata);
    }
    else
    {
        LLSD args;
        args["COST"] = llformat("%d", expected_upload_cost);
        LLNotificationsUtil::add("ErrorCannotAffordUpload", args);
    }

    closeFloater(false);
}

//-----------------------------------------------------------------------------
// onBtnCancel()
//-----------------------------------------------------------------------------
void LLFloaterNameDesc::onBtnCancel()
{
    closeFloater(false);
}


//-----------------------------------------------------------------------------
// LLFloaterSoundPreview()
//-----------------------------------------------------------------------------

LLFloaterSoundPreview::LLFloaterSoundPreview(const LLSD& args )
    : LLFloaterNameDesc(args)
{
    mIsAudio = true;
}

bool LLFloaterSoundPreview::postBuild()
{
    if (!LLFloaterNameDesc::postBuild())
    {
        return false;
    }
    setupUploadCommitAction();
    return true;
}


//-----------------------------------------------------------------------------
// LLFloaterAnimPreview()
//-----------------------------------------------------------------------------

LLFloaterAnimPreview::LLFloaterAnimPreview(const LLSD& args )
    : LLFloaterNameDesc(args)
{
}

bool LLFloaterAnimPreview::postBuild()
{
    if (!LLFloaterNameDesc::postBuild())
    {
        return false;
    }
    setupUploadCommitAction();
    return true;
}

//-----------------------------------------------------------------------------
// LLFloaterScriptPreview()
//-----------------------------------------------------------------------------

LLFloaterScriptPreview::LLFloaterScriptPreview(const LLSD& args )
    : LLFloaterNameDesc(args)
{
    mIsText = true;
}

bool LLFloaterScriptPreview::postBuild()
{
    if (!LLFloaterNameDesc::postBuild())
    {
        return false;
    }
    setupUploadCommitAction();
    return true;
}
