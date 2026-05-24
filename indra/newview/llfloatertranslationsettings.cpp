/**
 * @file llfloatertranslationsettings.cpp
 * @brief Machine translation settings for chat
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

#include "llfloatertranslationsettings.h"

// Viewer includes
#include "llfloaterimnearbychat.h"
#include "lltranslate.h"
#include "llviewercontrol.h" // for gSavedSettings

// Linden library includes
#include "llbutton.h"
#include "llcheckboxctrl.h"
#include "llcombobox.h"
#include "llfloaterreg.h"
#include "lllineeditor.h"
#include "llnotificationsutil.h"
#include "llradiogroup.h"

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

LLFloaterTranslationSettings::LLFloaterTranslationSettings(const LLSD& key)
:   LLFloater(key)
,   mMachineTranslationCB(NULL)
,   mAzureKeyVerified(false)
,   mGoogleKeyVerified(false)
,   mDeepLKeyVerified(false)
{
}

// virtual
bool LLFloaterTranslationSettings::postBuild()
{
    setupControls();
    setupCallbacks();

    center();
    return true;
}

void LLFloaterTranslationSettings::setupControls()
{
    mMachineTranslationCB = get_floater_child<LLCheckBoxCtrl>(this, "translate_chat_checkbox");
    mLanguageCombo = get_floater_child<LLComboBox>(this, "translate_language_combo");
    mTranslationServiceRadioGroup = get_floater_child<LLRadioGroup>(this, "translation_service_rg");
    mAzureAPIEndpointEditor = get_floater_child<LLComboBox>(this, "azure_api_endpoint_combo");
    mAzureAPIKeyEditor = get_floater_child<LLLineEditor>(this, "azure_api_key");
    mAzureAPIRegionEditor = get_floater_child<LLLineEditor>(this, "azure_api_region");
    mGoogleAPIKeyEditor = get_floater_child<LLLineEditor>(this, "google_api_key");
    mDeepLAPIDomainCombo = get_floater_child<LLComboBox>(this, "deepl_api_domain_combo");
    mDeepLAPIKeyEditor = get_floater_child<LLLineEditor>(this, "deepl_api_key");
    mAzureVerifyBtn = get_floater_child<LLButton>(this, "verify_azure_api_key_btn");
    mGoogleVerifyBtn = get_floater_child<LLButton>(this, "verify_google_api_key_btn");
    mDeepLVerifyBtn = get_floater_child<LLButton>(this, "verify_deepl_api_key_btn");
    mOKBtn = get_floater_child<LLButton>(this, "ok_btn");
}

void LLFloaterTranslationSettings::setupCallbacks()
{
    mMachineTranslationCB->setCommitCallback(boost::bind(&LLFloaterTranslationSettings::updateControlsEnabledState, this));
    mTranslationServiceRadioGroup->setCommitCallback(boost::bind(&LLFloaterTranslationSettings::updateControlsEnabledState, this));
    mOKBtn->setClickedCallback(boost::bind(&LLFloaterTranslationSettings::onBtnOK, this));
    get_floater_child<LLButton>(this, "cancel_btn")->setClickedCallback(boost::bind(&LLFloater::closeFloater, this, false));
    mAzureVerifyBtn->setClickedCallback(boost::bind(&LLFloaterTranslationSettings::onBtnAzureVerify, this));
    mGoogleVerifyBtn->setClickedCallback(boost::bind(&LLFloaterTranslationSettings::onBtnGoogleVerify, this));
    mDeepLVerifyBtn->setClickedCallback(boost::bind(&LLFloaterTranslationSettings::onBtnDeepLVerify, this));

    mAzureAPIKeyEditor->setFocusReceivedCallback(boost::bind(&LLFloaterTranslationSettings::onEditorFocused, this, _1));
    mAzureAPIKeyEditor->setKeystrokeCallback(boost::bind(&LLFloaterTranslationSettings::onAzureKeyEdited, this), NULL);
    mAzureAPIRegionEditor->setFocusReceivedCallback(boost::bind(&LLFloaterTranslationSettings::onEditorFocused, this, _1));
    mAzureAPIRegionEditor->setKeystrokeCallback(boost::bind(&LLFloaterTranslationSettings::onAzureKeyEdited, this), NULL);

    mAzureAPIEndpointEditor->setFocusLostCallback([this](LLFocusableElement*)
                                                  {
                                                      setAzureVerified(false, false, 0);
                                                  });
    mAzureAPIEndpointEditor->setCommitCallback([this](LLUICtrl* ctrl, const LLSD& param)
                                               {
                                                   setAzureVerified(false, false, 0);
                                               });

    mGoogleAPIKeyEditor->setFocusReceivedCallback(boost::bind(&LLFloaterTranslationSettings::onEditorFocused, this, _1));
    mGoogleAPIKeyEditor->setKeystrokeCallback(boost::bind(&LLFloaterTranslationSettings::onGoogleKeyEdited, this), NULL);

    mDeepLAPIKeyEditor->setFocusReceivedCallback(boost::bind(&LLFloaterTranslationSettings::onEditorFocused, this, _1));
    mDeepLAPIKeyEditor->setKeystrokeCallback(boost::bind(&LLFloaterTranslationSettings::onDeepLKeyEdited, this), NULL);

    mDeepLAPIDomainCombo->setFocusLostCallback([this](LLFocusableElement*)
                                                  {
                                                      setDeepLVerified(false, false, 0);
                                                  });
    mDeepLAPIDomainCombo->setCommitCallback([this](LLUICtrl* ctrl, const LLSD& param)
                                               {
                                                setDeepLVerified(false, false, 0);
                                               });
}

// virtual
void LLFloaterTranslationSettings::onOpen(const LLSD& key)
{
    syncSettingsFromSaved();
    updateControlsEnabledState();
}

void LLFloaterTranslationSettings::syncSettingsFromSaved()
{
    mMachineTranslationCB->setValue(gSavedSettings.getBOOL("TranslateChat"));
    mLanguageCombo->setSelectedByValue(gSavedSettings.getString("TranslateLanguage"), true);
    mTranslationServiceRadioGroup->setSelectedByValue(gSavedSettings.getString("TranslationService"), true);

    syncAzureKeyFromSaved(gSavedSettings.getLLSD("AzureTranslateAPIKey"));
    syncGoogleKeyFromSaved(gSavedSettings.getString("GoogleTranslateAPIKey"));
    syncDeepLKeyFromSaved(gSavedSettings.getLLSD("DeepLTranslateAPIKey"));
}

void LLFloaterTranslationSettings::syncAzureKeyFromSaved(const LLSD& azure_key)
{
    if (azure_key.isMap() && !azure_key["id"].asString().empty())
    {
        mAzureAPIKeyEditor->setText(azure_key["id"].asString());
        mAzureAPIKeyEditor->setTentative(false);
        if (azure_key.has("region") && !azure_key["region"].asString().empty())
        {
            mAzureAPIRegionEditor->setText(azure_key["region"].asString());
            mAzureAPIRegionEditor->setTentative(false);
        }
        else
        {
            mAzureAPIRegionEditor->setTentative(true);
        }
        mAzureAPIEndpointEditor->setValue(azure_key["endpoint"]);
        verifyKey(LLTranslate::SERVICE_AZURE, azure_key, false);
    }
    else
    {
        mAzureAPIKeyEditor->setTentative(true);
        mAzureAPIRegionEditor->setTentative(true);
        mAzureKeyVerified = false;
    }
}

void LLFloaterTranslationSettings::syncGoogleKeyFromSaved(const std::string& google_key)
{
    if (!google_key.empty())
    {
        mGoogleAPIKeyEditor->setText(google_key);
        mGoogleAPIKeyEditor->setTentative(false);
        verifyKey(LLTranslate::SERVICE_GOOGLE, google_key, false);
    }
    else
    {
        mGoogleAPIKeyEditor->setTentative(true);
        mGoogleKeyVerified = false;
    }
}

void LLFloaterTranslationSettings::syncDeepLKeyFromSaved(const LLSD& deepl_key)
{
    if (deepl_key.isMap() && !deepl_key["id"].asString().empty())
    {
        mDeepLAPIKeyEditor->setText(deepl_key["id"].asString());
        mDeepLAPIKeyEditor->setTentative(false);
        mDeepLAPIDomainCombo->setValue(deepl_key["domain"]);
        verifyKey(LLTranslate::SERVICE_DEEPL, deepl_key, false);
    }
    else
    {
        mDeepLAPIKeyEditor->setTentative(true);
        mDeepLKeyVerified = false;
    }
}

void LLFloaterTranslationSettings::setAzureVerified(bool ok, bool alert, S32 status)
{
    if (alert)
    {
        showAlert(ok ? "azure_api_key_verified" : "azure_api_key_not_verified", status);
    }

    mAzureKeyVerified = ok;
    updateControlsEnabledState();
}

void LLFloaterTranslationSettings::setGoogleVerified(bool ok, bool alert, S32 status)
{
    if (alert)
    {
        showAlert(ok ? "google_api_key_verified" : "google_api_key_not_verified", status);
    }

    mGoogleKeyVerified = ok;
    updateControlsEnabledState();
}

void LLFloaterTranslationSettings::setDeepLVerified(bool ok, bool alert, S32 status)
{
    if (alert)
    {
        showAlert(ok ? "deepl_api_key_verified" : "deepl_api_key_not_verified", status);
    }

    mDeepLKeyVerified = ok;
    updateControlsEnabledState();
}

std::string LLFloaterTranslationSettings::getSelectedService() const
{
    return mTranslationServiceRadioGroup->getSelectedValue().asString();
}

LLSD LLFloaterTranslationSettings::getEnteredAzureKey() const
{
    LLSD key;
    if (!mAzureAPIKeyEditor->getTentative())
    {
        key["endpoint"] = mAzureAPIEndpointEditor->getValue();
        key["id"] = mAzureAPIKeyEditor->getText();
        if (!mAzureAPIRegionEditor->getTentative())
        {
            key["region"] = mAzureAPIRegionEditor->getText();
        }
    }
    return key;
}

std::string LLFloaterTranslationSettings::getEnteredGoogleKey() const
{
    return mGoogleAPIKeyEditor->getTentative() ? LLStringUtil::null : mGoogleAPIKeyEditor->getText();
}

LLSD LLFloaterTranslationSettings::getEnteredDeepLKey() const
{
    LLSD key;
    if (!mDeepLAPIKeyEditor->getTentative())
    {
        key["domain"] = mDeepLAPIDomainCombo->getValue();
        key["id"] = mDeepLAPIKeyEditor->getText();
    }
    return key;
}

void LLFloaterTranslationSettings::showAlert(const std::string& msg_name, S32 status) const
{
    LLStringUtil::format_map_t string_args;
    // For now just show an http error code, whole 'reason' string might be added later
    string_args["[STATUS]"] = llformat("%d", status);
    std::string message = getString(msg_name, string_args);

    LLSD args;
    args["MESSAGE"] = message;
    LLNotificationsUtil::add("GenericAlert", args);
}

void LLFloaterTranslationSettings::updateControlsEnabledState()
{
    // Enable/disable controls based on the checkbox value.
    bool on = mMachineTranslationCB->getValue().asBoolean();
    std::string service = getSelectedService();
    bool azure_selected = service == "azure";
    bool google_selected = service == "google";
    bool deepl_selected = service == "deepl";

    mTranslationServiceRadioGroup->setEnabled(on);
    mLanguageCombo->setEnabled(on);

    // MS Azure
    setLabelEnabled("azure_api_endoint_label", on);
    mAzureAPIEndpointEditor->setEnabled(on && azure_selected);
    setLabelEnabled("azure_api_key_label", on);
    mAzureAPIKeyEditor->setEnabled(on && azure_selected);
    setLabelEnabled("azure_api_region_label", on);
    mAzureAPIRegionEditor->setEnabled(on && azure_selected);

    mAzureVerifyBtn->setEnabled(on && azure_selected &&
                                !mAzureKeyVerified && getEnteredAzureKey().isMap());

    // Google
    setLabelEnabled("google_api_key_label", on);
    mGoogleAPIKeyEditor->setEnabled(on && google_selected);

    mGoogleVerifyBtn->setEnabled(on && google_selected &&
        !mGoogleKeyVerified && !getEnteredGoogleKey().empty());

    // DeepL
    setLabelEnabled("deepl_api_domain_label", on);
    mDeepLAPIDomainCombo->setEnabled(on && deepl_selected);
    setLabelEnabled("deepl_api_key_label", on);
    mDeepLAPIKeyEditor->setEnabled(on && deepl_selected);

    mDeepLVerifyBtn->setEnabled(on && deepl_selected &&
                                 !mDeepLKeyVerified && getEnteredDeepLKey().isMap());

    bool service_verified =
        (azure_selected && mAzureKeyVerified)
        || (google_selected && mGoogleKeyVerified)
        || (deepl_selected && mDeepLKeyVerified);
    gSavedPerAccountSettings.setBOOL("TranslatingEnabled", service_verified);

    mOKBtn->setEnabled(!on || service_verified);
}

void LLFloaterTranslationSettings::setLabelEnabled(const std::string& name, bool enabled)
{
    get_floater_child<LLTextBox>(this, name)->setEnabled(enabled);
}

/*static*/
void LLFloaterTranslationSettings::setVerificationStatus(int service, bool ok, bool alert, S32 status)
{
    LLFloaterTranslationSettings* floater =
        LLFloaterReg::getTypedInstance<LLFloaterTranslationSettings>("prefs_translation");

    if (!floater)
    {
        LL_WARNS() << "Cannot find translation settings floater" << LL_ENDL;
        return;
    }

    switch (service)
    {
    case LLTranslate::SERVICE_AZURE:
        floater->setAzureVerified(ok, alert, status);
        break;
    case LLTranslate::SERVICE_GOOGLE:
        floater->setGoogleVerified(ok, alert, status);
        break;
    case LLTranslate::SERVICE_DEEPL:
        floater->setDeepLVerified(ok, alert, status);
        break;
    }
}


void LLFloaterTranslationSettings::verifyKey(int service, const LLSD& key, bool alert)
{
    LLTranslate::verifyKey(static_cast<LLTranslate::EService>(service), key,
        boost::bind(&LLFloaterTranslationSettings::setVerificationStatus, _1, _2, alert, _3));
}

void LLFloaterTranslationSettings::onEditorFocused(LLFocusableElement* control)
{
    LLLineEditor* editor = dynamic_cast<LLLineEditor*>(control);
    if (editor && editor->hasTabStop()) // if enabled. getEnabled() doesn't work
    {
        if (editor->getTentative())
        {
            editor->setText(LLStringUtil::null);
            editor->setTentative(false);
        }
    }
}

void LLFloaterTranslationSettings::onAzureKeyEdited()
{
    if (mAzureAPIKeyEditor->isDirty()
        || mAzureAPIRegionEditor->isDirty())
    {
        // todo: verify mAzureAPIEndpointEditor url
        setAzureVerified(false, false, 0);
    }
}

void LLFloaterTranslationSettings::onGoogleKeyEdited()
{
    if (mGoogleAPIKeyEditor->isDirty())
    {
        setGoogleVerified(false, false, 0);
    }
}

void LLFloaterTranslationSettings::onDeepLKeyEdited()
{
    if (mDeepLAPIKeyEditor->isDirty())
    {
        setDeepLVerified(false, false, 0);
    }
}

void LLFloaterTranslationSettings::onBtnAzureVerify()
{
    LLSD key = getEnteredAzureKey();
    if (key.isMap())
    {
        verifyKey(LLTranslate::SERVICE_AZURE, key);
    }
}

void LLFloaterTranslationSettings::onBtnGoogleVerify()
{
    std::string key = getEnteredGoogleKey();
    if (!key.empty())
    {
        verifyKey(LLTranslate::SERVICE_GOOGLE, LLSD(key));
    }
}

void LLFloaterTranslationSettings::onBtnDeepLVerify()
{
    LLSD key = getEnteredDeepLKey();
    if (key.isMap())
    {
        verifyKey(LLTranslate::SERVICE_DEEPL, key);
    }
}

void LLFloaterTranslationSettings::onClose(bool app_quitting)
{
    std::string service = gSavedSettings.getString("TranslationService");
    bool azure_selected = service == "azure";
    bool google_selected = service == "google";
    bool deepl_selected = service == "deepl";

    bool service_verified =
        (azure_selected && mAzureKeyVerified)
        || (google_selected && mGoogleKeyVerified)
        || (deepl_selected && mDeepLKeyVerified);
    gSavedPerAccountSettings.setBOOL("TranslatingEnabled", service_verified);
}
void LLFloaterTranslationSettings::onBtnOK()
{
    gSavedSettings.setBOOL("TranslateChat", mMachineTranslationCB->getValue().asBoolean());
    gSavedSettings.setString("TranslateLanguage", mLanguageCombo->getSelectedValue().asString());
    gSavedSettings.setString("TranslationService", getSelectedService());
    gSavedSettings.setLLSD("AzureTranslateAPIKey", getEnteredAzureKey());
    gSavedSettings.setString("GoogleTranslateAPIKey", getEnteredGoogleKey());
    gSavedSettings.setLLSD("DeepLTranslateAPIKey", getEnteredDeepLKey());

    closeFloater(false);
}
