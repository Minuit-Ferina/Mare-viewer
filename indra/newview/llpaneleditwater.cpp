/**
* @file llpaneleditwater.cpp
* @brief Floaters to create and edit fixed settings for sky and water.
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

#include "llpaneleditwater.h"

#include "llslider.h"
#include "lltexturectrl.h"
#include "llcolorswatch.h"
#include "llxyvector.h"
#include "llviewercontrol.h"

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

namespace
{
    const std::string   FIELD_WATER_FOG_COLOR("water_fog_color");
    const std::string   FIELD_WATER_FOG_DENSITY("water_fog_density");
    const std::string   FIELD_WATER_UNDERWATER_MOD("water_underwater_mod");
    const std::string   FIELD_WATER_NORMAL_MAP("water_normal_map");

    const std::string   FIELD_WATER_WAVE1_XY("water_wave1_xy");
    const std::string   FIELD_WATER_WAVE2_XY("water_wave2_xy");

    const std::string   FIELD_WATER_NORMAL_SCALE_X("water_normal_scale_x");
    const std::string   FIELD_WATER_NORMAL_SCALE_Y("water_normal_scale_y");
    const std::string   FIELD_WATER_NORMAL_SCALE_Z("water_normal_scale_z");

    const std::string   FIELD_WATER_FRESNEL_SCALE("water_fresnel_scale");
    const std::string   FIELD_WATER_FRESNEL_OFFSET("water_fresnel_offset");

    const std::string   FIELD_WATER_SCALE_ABOVE("water_scale_above");
    const std::string   FIELD_WATER_SCALE_BELOW("water_scale_below");
    const std::string   FIELD_WATER_BLUR_MULTIP("water_blur_multip");
}

static LLPanelInjector<LLPanelSettingsWaterMainTab> t_settings_water("panel_settings_water");

//==========================================================================
LLPanelSettingsWater::LLPanelSettingsWater() :
    LLSettingsEditPanel(),
    mWaterSettings()
{

}


//==========================================================================
LLPanelSettingsWaterMainTab::LLPanelSettingsWaterMainTab():
    LLPanelSettingsWater(),
    mClrFogColor(nullptr),
    mTxtNormalMap(nullptr)
{
}


bool LLPanelSettingsWaterMainTab::postBuild()
{
    mClrFogColor = get_owner_child<LLColorSwatchCtrl>(this, FIELD_WATER_FOG_COLOR);
    mTxtNormalMap = get_owner_child<LLTextureCtrl>(this, FIELD_WATER_NORMAL_MAP);

    get_owner_child<LLXYVector>(this, FIELD_WATER_WAVE1_XY)->setCommitCallback([this](LLUICtrl *, const LLSD &) { onLargeWaveChanged(); });

    mClrFogColor->setCommitCallback([this](LLUICtrl *, const LLSD &) { onFogColorChanged(); });
    get_owner_child<LLUICtrl>(this, FIELD_WATER_FOG_DENSITY)->setCommitCallback([this](LLUICtrl *, const LLSD &) { onFogDensityChanged(); });
//    getChild<LLUICtrl>(FIELD_WATER_FOG_DENSITY)->setCommitCallback([this](LLUICtrl *, const LLSD &) { onFogDensityChanged(getChild<LLUICtrl>(FIELD_WATER_FOG_DENSITY)->getValue().asReal()); });
    get_owner_child<LLUICtrl>(this, FIELD_WATER_UNDERWATER_MOD)->setCommitCallback([this](LLUICtrl *, const LLSD &) { onFogUnderWaterChanged(); });

    mTxtNormalMap->setDefaultImageAssetID(LLSettingsWater::GetDefaultWaterNormalAssetId());
    mTxtNormalMap->setBlankImageAssetID(BLANK_OBJECT_NORMAL);
    mTxtNormalMap->setCommitCallback([this](LLUICtrl *, const LLSD &) { onNormalMapChanged(); });

    get_owner_child<LLUICtrl>(this, FIELD_WATER_WAVE2_XY)->setCommitCallback([this](LLUICtrl *, const LLSD &) { onSmallWaveChanged(); });

    get_owner_child<LLUICtrl>(this, FIELD_WATER_NORMAL_SCALE_X)->setCommitCallback([this](LLUICtrl *, const LLSD &) { onNormalScaleChanged(); });
    get_owner_child<LLUICtrl>(this, FIELD_WATER_NORMAL_SCALE_Y)->setCommitCallback([this](LLUICtrl *, const LLSD &) { onNormalScaleChanged(); });
    get_owner_child<LLUICtrl>(this, FIELD_WATER_NORMAL_SCALE_Z)->setCommitCallback([this](LLUICtrl *, const LLSD &) { onNormalScaleChanged(); });

    get_owner_child<LLUICtrl>(this, FIELD_WATER_FRESNEL_SCALE)->setCommitCallback([this](LLUICtrl *, const LLSD &) { onFresnelScaleChanged(); });
    get_owner_child<LLUICtrl>(this, FIELD_WATER_FRESNEL_OFFSET)->setCommitCallback([this](LLUICtrl *, const LLSD &) { onFresnelOffsetChanged(); });
    get_owner_child<LLUICtrl>(this, FIELD_WATER_SCALE_ABOVE)->setCommitCallback([this](LLUICtrl *, const LLSD &) { onScaleAboveChanged(); });
    get_owner_child<LLUICtrl>(this, FIELD_WATER_SCALE_BELOW)->setCommitCallback([this](LLUICtrl *, const LLSD &) { onScaleBelowChanged(); });
    get_owner_child<LLUICtrl>(this, FIELD_WATER_BLUR_MULTIP)->setCommitCallback([this](LLUICtrl *, const LLSD &) { onBlurMultipChanged(); });

    refresh();

    return true;
}

//virtual
void LLPanelSettingsWaterMainTab::setEnabled(bool enabled)
{
    LLPanelSettingsWater::setEnabled(enabled);
    get_owner_child<LLUICtrl>(this, FIELD_WATER_FOG_DENSITY)->setEnabled(enabled);
    get_owner_child<LLUICtrl>(this, FIELD_WATER_UNDERWATER_MOD)->setEnabled(enabled);
    get_owner_child<LLUICtrl>(this, FIELD_WATER_FRESNEL_SCALE)->setEnabled(enabled);
    get_owner_child<LLUICtrl>(this, FIELD_WATER_FRESNEL_OFFSET)->setEnabled(enabled);

    get_owner_child<LLUICtrl>(this, FIELD_WATER_NORMAL_SCALE_X)->setEnabled(enabled);
    get_owner_child<LLUICtrl>(this, FIELD_WATER_NORMAL_SCALE_Y)->setEnabled(enabled);
    get_owner_child<LLUICtrl>(this, FIELD_WATER_NORMAL_SCALE_Z)->setEnabled(enabled);

    get_owner_child<LLUICtrl>(this, FIELD_WATER_SCALE_ABOVE)->setEnabled(enabled);
    get_owner_child<LLUICtrl>(this, FIELD_WATER_SCALE_BELOW)->setEnabled(enabled);
    get_owner_child<LLUICtrl>(this, FIELD_WATER_BLUR_MULTIP)->setEnabled(enabled);
}

//==========================================================================
void LLPanelSettingsWaterMainTab::refresh()
{
    if (!mWaterSettings)
    {
        setAllChildrenEnabled(false);
        setEnabled(false);
        return;
    }

    setEnabled(getCanChangeSettings());
    setAllChildrenEnabled(getCanChangeSettings());
    mClrFogColor->set(mWaterSettings->getWaterFogColor());
    get_owner_child<LLUICtrl>(this, FIELD_WATER_FOG_DENSITY)->setValue(mWaterSettings->getWaterFogDensity());
    get_owner_child<LLUICtrl>(this, FIELD_WATER_UNDERWATER_MOD)->setValue(mWaterSettings->getFogMod());
    mTxtNormalMap->setValue(mWaterSettings->getNormalMapID());
    LLVector2 vect2 = mWaterSettings->getWave1Dir() * -1.0; // Flip so that north and east are +
    get_owner_child<LLUICtrl>(this, FIELD_WATER_WAVE1_XY)->setValue(vect2.getValue());
    vect2 = mWaterSettings->getWave2Dir() * -1.0; // Flip so that north and east are +
    get_owner_child<LLUICtrl>(this, FIELD_WATER_WAVE2_XY)->setValue(vect2.getValue());
    LLVector3 vect3 = mWaterSettings->getNormalScale();
    get_owner_child<LLUICtrl>(this, FIELD_WATER_NORMAL_SCALE_X)->setValue(vect3[0]);
    get_owner_child<LLUICtrl>(this, FIELD_WATER_NORMAL_SCALE_Y)->setValue(vect3[1]);
    get_owner_child<LLUICtrl>(this, FIELD_WATER_NORMAL_SCALE_Z)->setValue(vect3[2]);
    get_owner_child<LLUICtrl>(this, FIELD_WATER_FRESNEL_SCALE)->setValue(mWaterSettings->getFresnelScale());
    get_owner_child<LLUICtrl>(this, FIELD_WATER_FRESNEL_OFFSET)->setValue(mWaterSettings->getFresnelOffset());
    get_owner_child<LLUICtrl>(this, FIELD_WATER_SCALE_ABOVE)->setValue(mWaterSettings->getScaleAbove());
    get_owner_child<LLUICtrl>(this, FIELD_WATER_SCALE_BELOW)->setValue(mWaterSettings->getScaleBelow());
    get_owner_child<LLUICtrl>(this, FIELD_WATER_BLUR_MULTIP)->setValue(mWaterSettings->getBlurMultiplier());
}

//==========================================================================

void LLPanelSettingsWaterMainTab::onFogColorChanged()
{
    if (!mWaterSettings) return;
    mWaterSettings->setWaterFogColor(LLColor3(mClrFogColor->get()));
    setIsDirty();
}

void LLPanelSettingsWaterMainTab::onFogDensityChanged()
{
    if (!mWaterSettings) return;
    mWaterSettings->setWaterFogDensity((F32)get_owner_child<LLUICtrl>(this, FIELD_WATER_FOG_DENSITY)->getValue().asReal());
    setIsDirty();
}

void LLPanelSettingsWaterMainTab::onFogUnderWaterChanged()
{
    if (!mWaterSettings) return;
    mWaterSettings->setFogMod((F32)get_owner_child<LLUICtrl>(this, FIELD_WATER_UNDERWATER_MOD)->getValue().asReal());
    setIsDirty();
}

void LLPanelSettingsWaterMainTab::onNormalMapChanged()
{
    if (!mWaterSettings) return;
    mWaterSettings->setNormalMapID(mTxtNormalMap->getImageAssetID());
    setIsDirty();
}

void LLPanelSettingsWaterMainTab::onLargeWaveChanged()
{
    if (!mWaterSettings) return;
    LLVector2 vect(get_owner_child<LLUICtrl>(this, FIELD_WATER_WAVE1_XY)->getValue());
    vect *= -1.0; // Flip so that north and east are -
    mWaterSettings->setWave1Dir(vect);
    setIsDirty();
}

void LLPanelSettingsWaterMainTab::onSmallWaveChanged()
{
    if (!mWaterSettings) return;
    LLVector2 vect(get_owner_child<LLUICtrl>(this, FIELD_WATER_WAVE2_XY)->getValue());
    vect *= -1.0; // Flip so that north and east are -
    mWaterSettings->setWave2Dir(vect);
    setIsDirty();
}


void LLPanelSettingsWaterMainTab::onNormalScaleChanged()
{
    if (!mWaterSettings) return;
    LLVector3 vect((F32)get_owner_child<LLUICtrl>(this, FIELD_WATER_NORMAL_SCALE_X)->getValue().asReal(), (F32)get_owner_child<LLUICtrl>(this, FIELD_WATER_NORMAL_SCALE_Y)->getValue().asReal(), (F32)get_owner_child<LLUICtrl>(this, FIELD_WATER_NORMAL_SCALE_Z)->getValue().asReal());
    mWaterSettings->setNormalScale(vect);
    setIsDirty();
}

void LLPanelSettingsWaterMainTab::onFresnelScaleChanged()
{
    if (!mWaterSettings) return;
    mWaterSettings->setFresnelScale((F32)get_owner_child<LLUICtrl>(this, FIELD_WATER_FRESNEL_SCALE)->getValue().asReal());
    setIsDirty();
}

void LLPanelSettingsWaterMainTab::onFresnelOffsetChanged()
{
    if (!mWaterSettings) return;
    mWaterSettings->setFresnelOffset((F32)get_owner_child<LLUICtrl>(this, FIELD_WATER_FRESNEL_OFFSET)->getValue().asReal());
    setIsDirty();
}

void LLPanelSettingsWaterMainTab::onScaleAboveChanged()
{
    if (!mWaterSettings) return;
    mWaterSettings->setScaleAbove((F32)get_owner_child<LLUICtrl>(this, FIELD_WATER_SCALE_ABOVE)->getValue().asReal());
    setIsDirty();
}

void LLPanelSettingsWaterMainTab::onScaleBelowChanged()
{
    if (!mWaterSettings) return;
    mWaterSettings->setScaleBelow((F32)get_owner_child<LLUICtrl>(this, FIELD_WATER_SCALE_BELOW)->getValue().asReal());
    setIsDirty();
}

void LLPanelSettingsWaterMainTab::onBlurMultipChanged()
{
    if (!mWaterSettings) return;
    mWaterSettings->setBlurMultiplier((F32)get_owner_child<LLUICtrl>(this, FIELD_WATER_BLUR_MULTIP)->getValue().asReal());
    setIsDirty();
}
