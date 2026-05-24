/**
 * @file llfloateraddpaymentmethod.cpp
 * @brief LLFloaterAddPaymentMethod class implementation
 *
 * $LicenseInfo:firstyear=2020&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2020, Linden Research, Inc.
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

#include "llfloateraddpaymentmethod.h"
#include "llnotificationsutil.h"
#include "lluictrlfactory.h"
#include "llweb.h"

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


LLFloaterAddPaymentMethod::LLFloaterAddPaymentMethod(const LLSD& key)
    :   LLFloater(key)
{
}

LLFloaterAddPaymentMethod::~LLFloaterAddPaymentMethod()
{
}

bool LLFloaterAddPaymentMethod::postBuild()
{
    setCanDrag(false);
    setupButtons();
    return true;
}

void LLFloaterAddPaymentMethod::setupButtons()
{
    get_floater_child<LLButton>(this, "continue_btn")->setCommitCallback(boost::bind(&LLFloaterAddPaymentMethod::onContinueBtn, this));
    get_floater_child<LLButton>(this, "close_btn")->setCommitCallback(boost::bind(&LLFloaterAddPaymentMethod::onCloseBtn, this));
}

void LLFloaterAddPaymentMethod::onOpen(const LLSD& key)
{
    centerOnScreen();
}

void LLFloaterAddPaymentMethod::onContinueBtn()
{
    closeFloater();
    showAddPaymentNotification();
}

void LLFloaterAddPaymentMethod::showAddPaymentNotification()
{
    LLNotificationsUtil::add("AddPaymentMethod", LLSD(), LLSD(),
        [this](const LLSD&notif, const LLSD&resp)
    {
        S32 opt = LLNotificationsUtil::getSelectedOption(notif, resp);
        if (opt == 0)
        {
            LLWeb::loadURL(getContinueURL());
        }
    });
}

std::string LLFloaterAddPaymentMethod::getContinueURL()
{
    return getString("continue_url");
}

void LLFloaterAddPaymentMethod::onCloseBtn()
{
    closeFloater();
}

void LLFloaterAddPaymentMethod::centerOnScreen()
{
    LLVector2 window_size = LLUI::getInstance()->getWindowSize();
    centerWithin(LLRect(0, 0, ll_round(window_size.mV[VX]), ll_round(window_size.mV[VY])));
}
