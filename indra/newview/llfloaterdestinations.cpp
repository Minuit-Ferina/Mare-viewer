/**
 * @file llfloaterdestinations.h
 * @author Leyla Farazha
 * @brief floater for the destinations guide
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

#include "llfloaterdestinations.h"
#include "llmediactrl.h"
#include "lluictrlfactory.h"
#include "llviewercontrol.h"
#include "llweb.h"

//MK
#include "llagent.h"

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

//mk

LLFloaterDestinations::LLFloaterDestinations(const LLSD& key)
    :   LLFloater(key)
{
}

LLFloaterDestinations::~LLFloaterDestinations()
{
}

bool LLFloaterDestinations::postBuild()
{
    enableResizeCtrls(true, true, false);
    LLMediaCtrl* destinations = get_floater_child<LLMediaCtrl>(this, "destination_guide_contents");
    destinations->setErrorPageURL(gSavedSettings.getString("GenericErrorPageURL"));
    std::string url = gSavedSettings.getString("DestinationGuideURL");
    url = LLWeb::expandURLSubstitutions(url, LLSD());
    destinations->navigateTo(url, HTTP_CONTENT_TEXT_HTML);

    // If cookie is there, will set it now. Otherwise will have to wait for login completion
    // which will also update destinations instance if it already exists.
    LLViewerMedia::getInstance()->getOpenIDCookie(destinations);
    return true;
}

//MK
void LLFloaterDestinations::onOpen(const LLSD& key)
{
    if (gRRenabled && gAgent.mRRInterface.mContainsTp)
    {
        closeFloater();
        return;
    }

    LLFloater::onOpen(key);
}

void LLFloaterDestinations::draw()
{
    if (gRRenabled && gAgent.mRRInterface.mContainsTp)
    {
        closeFloater();
        return;
    }

    LLFloater::draw();
}
//mk

