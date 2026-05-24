/**
 * @file kokuafloaterstreaminfo.cpp
 * @author Chorazin Allen
 * @brief A floater to show music stream info
 *
 * $LicenseInfo:firstyear=2008&license=viewerlgpl$
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
#include "kokuafloaterstreaminfo.h"

#include "llfloaterreg.h"
#include "lltextbox.h"
#include "streamtitledisplay.h"

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

static std::string last_artist_title;

KokuaFloaterStreamInfo::KokuaFloaterStreamInfo(const LLSD& seed)
:   LLFloater(seed)
{
}

// static
void KokuaFloaterStreamInfo::UpdateStreamInfo(const std::string artist_title)
{
    last_artist_title = artist_title;
    KokuaFloaterStreamInfo* stream_floater = LLFloaterReg::findTypedInstance<KokuaFloaterStreamInfo>("stream_info");
    if (stream_floater)
    {
        if (last_artist_title.empty())
        {
            last_artist_title = stream_floater->getString("KFSI_NoStream");
        }
        LLTextBox* stream_status = get_owner_child<LLTextBox>(stream_floater, "stream_status");
        if (stream_status)
        {
            stream_status->setText(last_artist_title);
        }
    }
}

bool KokuaFloaterStreamInfo::postBuild()
{
    LLTextBox* stream_status = get_owner_child<LLTextBox>(this, "stream_status");
    if (stream_status)
    {
        if (last_artist_title.empty())
        {
            last_artist_title = getString("KFSI_NoStream");
        }
        stream_status->setText(last_artist_title);
    }

    return true;
}
