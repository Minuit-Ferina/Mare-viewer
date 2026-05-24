/**
 * @file llpanellandmedia.cpp
 * @brief Allows configuration of "media" for a land parcel,
 *   for example movies, web pages, and audio.
 *
 * $LicenseInfo:firstyear=2007&license=viewerlgpl$
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

#include "llpanellandmedia.h"

// viewer includes
#include "llmimetypes.h"
#include "llviewerparcelmgr.h"
#include "llviewerregion.h"
#include "llviewermedia.h"
#include "llviewerparcelmedia.h"
#include "lluictrlfactory.h"

// library includes
#include "llcheckboxctrl.h"
#include "llcombobox.h"
#include "llfloaterurlentry.h"
#include "llfocusmgr.h"
#include "lllineeditor.h"
#include "llparcel.h"
#include "lltextbox.h"
#include "llradiogroup.h"
#include "llspinctrl.h"
#include "llsdutil.h"
#include "lltexturectrl.h"
#include "roles_constants.h"
#include "llscrolllistctrl.h"

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

//---------------------------------------------------------------------------
// LLPanelLandMedia
//---------------------------------------------------------------------------

LLPanelLandMedia::LLPanelLandMedia(LLParcelSelectionHandle& parcel)
:   LLPanel(),
    mParcel(parcel),
    mMediaURLEdit(NULL),
    mMediaDescEdit(NULL),
    mMediaTypeCombo(NULL),
    mSetURLButton(NULL),
    mMediaHeightCtrl(NULL),
    mMediaWidthCtrl(NULL),
    mMediaSizeCtrlLabel(NULL),
    mMediaTextureCtrl(NULL),
    mMediaAutoScaleCheck(NULL),
    mMediaLoopCheck(NULL)
{
}


// virtual
LLPanelLandMedia::~LLPanelLandMedia()
{
}

bool LLPanelLandMedia::postBuild()
{

    mMediaTextureCtrl = get_owner_child<LLTextureCtrl>(this, "media texture");
    mMediaTextureCtrl->setCommitCallback( onCommitAny, this );
    mMediaTextureCtrl->setAllowNoTexture ( true );
    mMediaTextureCtrl->setImmediateFilterPermMask(PERM_COPY | PERM_TRANSFER);
    mMediaTextureCtrl->setDnDFilterPermMask(PERM_COPY | PERM_TRANSFER);

    mMediaAutoScaleCheck = get_owner_child<LLCheckBoxCtrl>(this, "media_auto_scale");
    childSetCommitCallback("media_auto_scale", onCommitAny, this);

    mMediaLoopCheck = get_owner_child<LLCheckBoxCtrl>(this, "media_loop");
    childSetCommitCallback("media_loop", onCommitAny, this );

    mMediaURLEdit = get_owner_child<LLLineEditor>(this, "media_url");
    childSetCommitCallback("media_url", onCommitAny, this );

    mMediaDescEdit = get_owner_child<LLLineEditor>(this, "url_description");
    childSetCommitCallback("url_description", onCommitAny, this);

    mMediaTypeCombo = get_owner_child<LLComboBox>(this, "media type");
    childSetCommitCallback("media type", onCommitType, this);
    populateMIMECombo();

    mMediaWidthCtrl = get_owner_child<LLSpinCtrl>(this, "media_size_width");
    childSetCommitCallback("media_size_width", onCommitAny, this);
    mMediaHeightCtrl = get_owner_child<LLSpinCtrl>(this, "media_size_height");
    childSetCommitCallback("media_size_height", onCommitAny, this);
    mMediaSizeCtrlLabel = get_owner_child<LLTextBox>(this, "media_size");

    mSetURLButton = get_owner_child<LLButton>(this, "set_media_url");
    childSetAction("set_media_url", onSetBtn, this);

    return true;
}


// public
void LLPanelLandMedia::refresh()
{
    LLParcel *parcel = mParcel->getParcel();

    if (!parcel)
    {
        clearCtrls();
    }
    else
    {
        // something selected, hooray!

        // Display options
        bool can_change_media = LLViewerParcelMgr::isParcelModifiableByAgent(parcel, GP_LAND_CHANGE_MEDIA);

        mMediaURLEdit->setText(parcel->getMediaURL());
        mMediaURLEdit->setEnabled( false );

        get_owner_child<LLUICtrl>(this, "current_url")->setValue(parcel->getMediaCurrentURL());

        mMediaDescEdit->setText(parcel->getMediaDesc());
        mMediaDescEdit->setEnabled( can_change_media );

        std::string mime_type = parcel->getMediaType();
        if (mime_type.empty() || mime_type == LLMIMETypes::getDefaultMimeType())
        {
            mime_type = LLMIMETypes::getDefaultMimeTypeTranslation();
        }
        setMediaType(mime_type);
        mMediaTypeCombo->setEnabled( can_change_media );
        get_owner_child<LLUICtrl>(this, "mime_type")->setValue(mime_type);

        mMediaAutoScaleCheck->set( static_cast<bool>(parcel->getMediaAutoScale()) );
        mMediaAutoScaleCheck->setEnabled ( can_change_media );

        // Special code to disable looping checkbox for HTML MIME type
        // (DEV-10042 -- Parcel Media: "Loop Media" should be disabled for static media types)
        bool allow_looping = LLMIMETypes::findAllowLooping( mime_type );
        if ( allow_looping )
            mMediaLoopCheck->set( static_cast<bool>(parcel->getMediaLoop()) );
        else
            mMediaLoopCheck->set( false );
        mMediaLoopCheck->setEnabled ( can_change_media && allow_looping );

        // disallow media size change for mime types that don't allow it
        bool allow_resize = LLMIMETypes::findAllowResize( mime_type );
        if ( allow_resize )
            mMediaWidthCtrl->setValue( parcel->getMediaWidth() );
        else
            mMediaWidthCtrl->setValue( 0 );
        mMediaWidthCtrl->setEnabled ( can_change_media && allow_resize );

        if ( allow_resize )
            mMediaHeightCtrl->setValue( parcel->getMediaHeight() );
        else
            mMediaHeightCtrl->setValue( 0 );
        mMediaHeightCtrl->setEnabled ( can_change_media && allow_resize );

        // enable/disable for text label for completeness
        mMediaSizeCtrlLabel->setEnabled( can_change_media && allow_resize );

        mMediaTextureCtrl->setImageAssetID ( parcel->getMediaID() );
        mMediaTextureCtrl->setEnabled( can_change_media );

        mSetURLButton->setEnabled( can_change_media );

    }
}

void LLPanelLandMedia::populateMIMECombo()
{
    std::string default_mime_type = LLMIMETypes::getDefaultMimeType();
    std::string default_label;
    LLMIMETypes::mime_widget_set_map_t::const_iterator it;
    for (it = LLMIMETypes::sWidgetMap.begin(); it != LLMIMETypes::sWidgetMap.end(); ++it)
    {
        const std::string& mime_type = it->first;
        const LLMIMETypes::LLMIMEWidgetSet& info = it->second;
        if (info.mDefaultMimeType == default_mime_type)
        {
            // Add this label at the end to make UI look cleaner
            default_label = info.mLabel;
        }
        else
        {
            mMediaTypeCombo->add(info.mLabel, mime_type);
        }
    }

    mMediaTypeCombo->add( default_label, default_mime_type, ADD_BOTTOM );
}

void LLPanelLandMedia::setMediaType(const std::string& mime_type)
{
    LLParcel *parcel = mParcel->getParcel();
    if(parcel)
        parcel->setMediaType(mime_type);

    std::string media_key = LLMIMETypes::widgetType(mime_type);
    mMediaTypeCombo->setValue(media_key);

    std::string mime_str = mime_type;
    if(LLMIMETypes::getDefaultMimeType() == mime_type)
    {
        // Instead of showing predefined "none/none" we are going to show something
        // localizable - "none" for example (see EXT-6542)
        mime_str = LLMIMETypes::getDefaultMimeTypeTranslation();
    }
    get_owner_child<LLUICtrl>(this, "mime_type")->setValue(mime_str);
}

void LLPanelLandMedia::setMediaURL(const std::string& media_url)
{
    mMediaURLEdit->setText(media_url);
    LLParcel *parcel = mParcel->getParcel();
    if(parcel)
        parcel->setMediaCurrentURL(media_url);
    // LLViewerMedia::navigateHome();


    mMediaURLEdit->onCommit();
    // LLViewerParcelMedia::sendMediaNavigateMessage(media_url);
    get_owner_child<LLUICtrl>(this, "current_url")->setValue(media_url);
}
std::string LLPanelLandMedia::getMediaURL()
{
    return mMediaURLEdit->getText();
}

// static
void LLPanelLandMedia::onCommitType(LLUICtrl *ctrl, void *userdata)
{
    LLPanelLandMedia *self = (LLPanelLandMedia *)userdata;
    std::string current_type = LLMIMETypes::widgetType(get_owner_child<LLUICtrl>(self, "mime_type")->getValue().asString());
    std::string new_type = self->mMediaTypeCombo->getValue();
    if(current_type != new_type)
    {
        get_owner_child<LLUICtrl>(self, "mime_type")->setValue(LLMIMETypes::findDefaultMimeType(new_type));
    }
    onCommitAny(ctrl, userdata);

}

// static
void LLPanelLandMedia::onCommitAny(LLUICtrl*, void *userdata)
{
    LLPanelLandMedia *self = (LLPanelLandMedia *)userdata;

    LLParcel* parcel = self->mParcel->getParcel();
    if (!parcel)
    {
        return;
    }

    // Extract data from UI
    std::string media_url   = self->mMediaURLEdit->getText();
    std::string media_desc  = self->mMediaDescEdit->getText();
    std::string mime_type   = get_owner_child<LLUICtrl>(self, "mime_type")->getValue().asString();
    U8 media_auto_scale     = static_cast<U8>(self->mMediaAutoScaleCheck->get());
    U8 media_loop           = static_cast<U8>(self->mMediaLoopCheck->get());
    S32 media_width         = (S32)self->mMediaWidthCtrl->get();
    S32 media_height        = (S32)self->mMediaHeightCtrl->get();
    LLUUID media_id         = self->mMediaTextureCtrl->getImageAssetID();


    get_owner_child<LLUICtrl>(self, "mime_type")->setValue(mime_type);

    // Remove leading/trailing whitespace (common when copying/pasting)
    LLStringUtil::trim(media_url);

    // Push data into current parcel
    parcel->setMediaURL(media_url);
    parcel->setMediaType(mime_type);
    parcel->setMediaDesc(media_desc);
    parcel->setMediaWidth(media_width);
    parcel->setMediaHeight(media_height);
    parcel->setMediaID(media_id);
    parcel->setMediaAutoScale ( media_auto_scale );
    parcel->setMediaLoop ( media_loop );

    // Send current parcel data upstream to server
    LLViewerParcelMgr::getInstance()->sendParcelPropertiesUpdate( parcel );

    // Might have changed properties, so let's redraw!
    self->refresh();
}
// static
void LLPanelLandMedia::onSetBtn(void *userdata)
{
    LLPanelLandMedia *self = (LLPanelLandMedia *)userdata;
    self->mURLEntryFloater = LLFloaterURLEntry::show( self->getHandle(), self->getMediaURL() );
    LLFloater* parent_floater = gFloaterView->getParentFloater(self);
    if (parent_floater)
    {
        parent_floater->addDependentFloater(self->mURLEntryFloater.get());
    }
}

// static
void LLPanelLandMedia::onResetBtn(void *userdata)
{
    LLPanelLandMedia *self = (LLPanelLandMedia *)userdata;
    LLParcel* parcel = self->mParcel->getParcel();
    // LLViewerMedia::navigateHome();
    self->refresh();
    get_owner_child<LLUICtrl>(self, "current_url")->setValue(parcel->getMediaURL());
    // LLViewerParcelMedia::sendMediaNavigateMessage(parcel->getMediaURL());

}

