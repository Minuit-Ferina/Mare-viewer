/**
 * @file llfloaterfixedenvironment.cpp
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

#include "llfloaterenvironmentadjust.h"

#include "llnotificationsutil.h"
#include "llslider.h"
#include "llsliderctrl.h"
#include "llcolorswatch.h"
#include "lltexturectrl.h"
#include "llvirtualtrackball.h"
#include "llenvironment.h"
#include "llviewercontrol.h"
#include "pipeline.h"
#include "llagent.h" // for RLV

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

//=========================================================================
namespace
{
    const std::string FIELD_SKY_AMBIENT_LIGHT("ambient_light");
    const std::string FIELD_SKY_BLUE_HORIZON("blue_horizon");
    const std::string FIELD_SKY_BLUE_DENSITY("blue_density");
    const std::string FIELD_SKY_SUN_COLOR("sun_color");
    const std::string FIELD_SKY_CLOUD_COLOR("cloud_color");
    const std::string FIELD_SKY_HAZE_HORIZON("haze_horizon");
    const std::string FIELD_SKY_HAZE_DENSITY("haze_density");
    const std::string FIELD_SKY_CLOUD_COVERAGE("cloud_coverage");
    const std::string FIELD_SKY_CLOUD_MAP("cloud_map");
    const std::string FIELD_WATER_NORMAL_MAP("water_normal_map");
    const std::string FIELD_SKY_CLOUD_SCALE("cloud_scale");
    const std::string FIELD_SKY_SCENE_GAMMA("scene_gamma");
    const std::string FIELD_SKY_SUN_ROTATION("sun_rotation");
    const std::string FIELD_SKY_SUN_AZIMUTH("sun_azimuth");
    const std::string FIELD_SKY_SUN_ELEVATION("sun_elevation");
    const std::string FIELD_SKY_SUN_SCALE("sun_scale");
    const std::string FIELD_SKY_GLOW_FOCUS("glow_focus");
    const std::string FIELD_SKY_GLOW_SIZE("glow_size");
    const std::string FIELD_SKY_STAR_BRIGHTNESS("star_brightness");
    const std::string FIELD_SKY_MOON_ROTATION("moon_rotation");
    const std::string FIELD_SKY_MOON_AZIMUTH("moon_azimuth");
    const std::string FIELD_SKY_MOON_ELEVATION("moon_elevation");
    const std::string FIELD_REFLECTION_PROBE_AMBIANCE("probe_ambiance");
    const std::string BTN_RESET("btn_reset");

    const F32 SLIDER_SCALE_SUN_AMBIENT(3.0f);
    const F32 SLIDER_SCALE_BLUE_HORIZON_DENSITY(2.0f);
    const F32 SLIDER_SCALE_GLOW_R(20.0f);
    const F32 SLIDER_SCALE_GLOW_B(-5.0f);
    //const F32 SLIDER_SCALE_DENSITY_MULTIPLIER(0.001f);

    const S32 FLOATER_ENVIRONMENT_UPDATE(-2);
}

//=========================================================================
LLFloaterEnvironmentAdjust::LLFloaterEnvironmentAdjust(const LLSD &key):
    LLFloater(key)
{}

LLFloaterEnvironmentAdjust::~LLFloaterEnvironmentAdjust()
{}

//-------------------------------------------------------------------------
bool LLFloaterEnvironmentAdjust::postBuild()
{
    setupControlCallbacks();
    setupTextureControls();

    refresh();
    return true;
}

void LLFloaterEnvironmentAdjust::setupControlCallbacks()
{
    get_floater_child<LLUICtrl>(this, FIELD_SKY_AMBIENT_LIGHT)->setCommitCallback([this](LLUICtrl *, const LLSD &) { onAmbientLightChanged(); });
    get_floater_child<LLUICtrl>(this, FIELD_SKY_BLUE_HORIZON)->setCommitCallback([this](LLUICtrl *, const LLSD &) { onBlueHorizonChanged(); });
    get_floater_child<LLUICtrl>(this, FIELD_SKY_BLUE_DENSITY)->setCommitCallback([this](LLUICtrl *, const LLSD &) { onBlueDensityChanged(); });
    get_floater_child<LLUICtrl>(this, FIELD_SKY_HAZE_HORIZON)->setCommitCallback([this](LLUICtrl *, const LLSD &) { onHazeHorizonChanged(); });
    get_floater_child<LLUICtrl>(this, FIELD_SKY_HAZE_DENSITY)->setCommitCallback([this](LLUICtrl *, const LLSD &) { onHazeDensityChanged(); });
    get_floater_child<LLUICtrl>(this, FIELD_SKY_SCENE_GAMMA)->setCommitCallback([this](LLUICtrl *, const LLSD &) { onSceneGammaChanged(); });

    get_floater_child<LLUICtrl>(this, FIELD_SKY_CLOUD_COLOR)->setCommitCallback([this](LLUICtrl *, const LLSD &) { onCloudColorChanged(); });
    get_floater_child<LLUICtrl>(this, FIELD_SKY_CLOUD_COVERAGE)->setCommitCallback([this](LLUICtrl *, const LLSD &) { onCloudCoverageChanged(); });
    get_floater_child<LLUICtrl>(this, FIELD_SKY_CLOUD_SCALE)->setCommitCallback([this](LLUICtrl *, const LLSD &) { onCloudScaleChanged(); });
    get_floater_child<LLUICtrl>(this, FIELD_SKY_SUN_COLOR)->setCommitCallback([this](LLUICtrl *, const LLSD &) { onSunColorChanged(); });

    get_floater_child<LLUICtrl>(this, FIELD_SKY_GLOW_FOCUS)->setCommitCallback([this](LLUICtrl *, const LLSD &) { onGlowChanged(); });
    get_floater_child<LLUICtrl>(this, FIELD_SKY_GLOW_SIZE)->setCommitCallback([this](LLUICtrl *, const LLSD &) { onGlowChanged(); });
    get_floater_child<LLUICtrl>(this, FIELD_SKY_STAR_BRIGHTNESS)->setCommitCallback([this](LLUICtrl *, const LLSD &) { onStarBrightnessChanged(); });
    get_floater_child<LLUICtrl>(this, FIELD_SKY_SUN_ROTATION)->setCommitCallback([this](LLUICtrl *, const LLSD &) { onSunRotationChanged(); });
    get_floater_child<LLUICtrl>(this, FIELD_SKY_SUN_AZIMUTH)->setCommitCallback([this](LLUICtrl *, const LLSD &) { onSunAzimElevChanged(); });
    get_floater_child<LLUICtrl>(this, FIELD_SKY_SUN_ELEVATION)->setCommitCallback([this](LLUICtrl *, const LLSD &) { onSunAzimElevChanged(); });
    get_floater_child<LLUICtrl>(this, FIELD_SKY_SUN_SCALE)->setCommitCallback([this](LLUICtrl *, const LLSD &) { onSunScaleChanged(); });

    get_floater_child<LLUICtrl>(this, FIELD_SKY_MOON_ROTATION)->setCommitCallback([this](LLUICtrl *, const LLSD &) { onMoonRotationChanged(); });
    get_floater_child<LLUICtrl>(this, FIELD_SKY_MOON_AZIMUTH)->setCommitCallback([this](LLUICtrl *, const LLSD &) { onMoonAzimElevChanged(); });
    get_floater_child<LLUICtrl>(this, FIELD_SKY_MOON_ELEVATION)->setCommitCallback([this](LLUICtrl *, const LLSD &) { onMoonAzimElevChanged(); });
    get_floater_child<LLUICtrl>(this, BTN_RESET)->setCommitCallback([this](LLUICtrl *, const LLSD &) { onButtonReset(); });

    get_floater_child<LLTextureCtrl>(this, FIELD_SKY_CLOUD_MAP)->setCommitCallback([this](LLUICtrl *, const LLSD &) { onCloudMapChanged(); });
    get_floater_child<LLTextureCtrl>(this, FIELD_WATER_NORMAL_MAP)->setCommitCallback([this](LLUICtrl *, const LLSD &) { onWaterMapChanged(); });
    get_floater_child<LLUICtrl>(this, FIELD_REFLECTION_PROBE_AMBIANCE)->setCommitCallback([this](LLUICtrl*, const LLSD&) { onReflectionProbeAmbianceChanged(); });
}

void LLFloaterEnvironmentAdjust::setupTextureControls()
{
    get_floater_child<LLTextureCtrl>(this, FIELD_SKY_CLOUD_MAP)->setDefaultImageAssetID(LLSettingsSky::GetDefaultCloudNoiseTextureId());
    get_floater_child<LLTextureCtrl>(this, FIELD_SKY_CLOUD_MAP)->setAllowNoTexture(true);

    get_floater_child<LLTextureCtrl>(this, FIELD_WATER_NORMAL_MAP)->setDefaultImageAssetID(LLSettingsWater::GetDefaultWaterNormalAssetId());
    get_floater_child<LLTextureCtrl>(this, FIELD_WATER_NORMAL_MAP)->setBlankImageAssetID(BLANK_OBJECT_NORMAL);
}

void LLFloaterEnvironmentAdjust::onOpen(const LLSD& key)
{
//MK by CA
    if (gRRenabled && gAgent.mRRInterface.mContainsSetenv)
    {
        closeFloater();
        return;
    }
//mk by CA
    if (!mLiveSky)
    {
        LLEnvironment::instance().saveBeaconsState();
    }
    captureCurrentEnvironment();

    mEventConnection = LLEnvironment::instance().setEnvironmentChanged([this](LLEnvironment::EnvSelection_t env, S32 version){ onEnvironmentUpdated(env, version); });

    // HACK -- resume reflection map manager because "setEnvironmentChanged" may pause it (SL-20456)
    gPipeline.mReflectionMapManager.resume();

    LLFloater::onOpen(key);
    refresh();
}

void LLFloaterEnvironmentAdjust::onClose(bool app_quitting)
{
    LLEnvironment::instance().revertBeaconsState();
    mEventConnection.disconnect();
    mLiveSky.reset();
    mLiveWater.reset();
    LLFloater::onClose(app_quitting);
}


//-------------------------------------------------------------------------
void LLFloaterEnvironmentAdjust::refresh()
{
    if (!mLiveSky || !mLiveWater)
    {
        setAllChildrenEnabled(false);
        return;
    }

    setEnabled(true);
    setAllChildrenEnabled(true);
    syncEnvironmentControls();
}

void LLFloaterEnvironmentAdjust::syncEnvironmentControls()
{
    setColorValue(FIELD_SKY_AMBIENT_LIGHT, mLiveSky->getAmbientColor() / SLIDER_SCALE_SUN_AMBIENT);
    setColorValue(FIELD_SKY_BLUE_HORIZON, mLiveSky->getBlueHorizon() / SLIDER_SCALE_BLUE_HORIZON_DENSITY);
    setColorValue(FIELD_SKY_BLUE_DENSITY, mLiveSky->getBlueDensity() / SLIDER_SCALE_BLUE_HORIZON_DENSITY);
    setControlValue(FIELD_SKY_HAZE_HORIZON, mLiveSky->getHazeHorizon());
    setControlValue(FIELD_SKY_HAZE_DENSITY, mLiveSky->getHazeDensity());
    setControlValue(FIELD_SKY_SCENE_GAMMA, mLiveSky->getGamma());
    setColorValue(FIELD_SKY_CLOUD_COLOR, mLiveSky->getCloudColor());
    setControlValue(FIELD_SKY_CLOUD_COVERAGE, mLiveSky->getCloudShadow());
    setControlValue(FIELD_SKY_CLOUD_SCALE, mLiveSky->getCloudScale());
    setColorValue(FIELD_SKY_SUN_COLOR, mLiveSky->getSunlightColor() / SLIDER_SCALE_SUN_AMBIENT);

    setTextureValue(FIELD_SKY_CLOUD_MAP, mLiveSky->getCloudNoiseTextureId());
    setTextureValue(FIELD_WATER_NORMAL_MAP, mLiveWater->getNormalMapID());

    static LLCachedControl<bool> should_auto_adjust(gSavedSettings, "RenderSkyAutoAdjustLegacy", false);
    setControlValue(FIELD_REFLECTION_PROBE_AMBIANCE, mLiveSky->getReflectionProbeAmbiance(should_auto_adjust));

    LLColor3 glow(mLiveSky->getGlow());

    // takes 40 - 0.2 range -> 0 - 1.99 UI range
    setControlValue(FIELD_SKY_GLOW_SIZE, 2.0 - (glow.mV[0] / SLIDER_SCALE_GLOW_R));
    setControlValue(FIELD_SKY_GLOW_FOCUS, glow.mV[2] / SLIDER_SCALE_GLOW_B);
    setControlValue(FIELD_SKY_STAR_BRIGHTNESS, mLiveSky->getStarBrightness());
    setControlValue(FIELD_SKY_SUN_SCALE, mLiveSky->getSunScale());

    syncSunRotationControls(mLiveSky->getSunRotation());
    syncMoonRotationControls(mLiveSky->getMoonRotation());
    updateGammaLabel();
}

void LLFloaterEnvironmentAdjust::syncSunRotationControls(const LLQuaternion& rotation)
{
    F32 azimuth;
    F32 elevation;
    LLVirtualTrackball::getAzimuthAndElevationDeg(rotation, azimuth, elevation);

    setControlValue(FIELD_SKY_SUN_AZIMUTH, azimuth);
    setControlValue(FIELD_SKY_SUN_ELEVATION, elevation);
    setTrackballRotation(FIELD_SKY_SUN_ROTATION, rotation);
}

void LLFloaterEnvironmentAdjust::syncMoonRotationControls(const LLQuaternion& rotation)
{
    F32 azimuth;
    F32 elevation;
    LLVirtualTrackball::getAzimuthAndElevationDeg(rotation, azimuth, elevation);

    setControlValue(FIELD_SKY_MOON_AZIMUTH, azimuth);
    setControlValue(FIELD_SKY_MOON_ELEVATION, elevation);
    setTrackballRotation(FIELD_SKY_MOON_ROTATION, rotation);
}

F32 LLFloaterEnvironmentAdjust::getControlF32(const std::string& name)
{
    return (F32)get_floater_child<LLUICtrl>(this, name)->getValue().asReal();
}

void LLFloaterEnvironmentAdjust::setControlValue(const std::string& name, const LLSD& value)
{
    get_floater_child<LLUICtrl>(this, name)->setValue(value);
}

const LLColor4& LLFloaterEnvironmentAdjust::getColorValue(const std::string& name)
{
    return get_floater_child<LLColorSwatchCtrl>(this, name)->get();
}

void LLFloaterEnvironmentAdjust::setColorValue(const std::string& name, const LLColor4& value)
{
    get_floater_child<LLColorSwatchCtrl>(this, name)->set(value);
}

LLUUID LLFloaterEnvironmentAdjust::getTextureValue(const std::string& name)
{
    return get_floater_child<LLTextureCtrl>(this, name)->getValue().asUUID();
}

void LLFloaterEnvironmentAdjust::setTextureValue(const std::string& name, const LLUUID& value)
{
    get_floater_child<LLTextureCtrl>(this, name)->setValue(value);
}

LLQuaternion LLFloaterEnvironmentAdjust::getTrackballRotation(const std::string& name)
{
    return get_floater_child<LLVirtualTrackball>(this, name)->getRotation();
}

void LLFloaterEnvironmentAdjust::setTrackballRotation(const std::string& name, const LLQuaternion& rotation)
{
    get_floater_child<LLVirtualTrackball>(this, name)->setRotation(rotation);
}

void LLFloaterEnvironmentAdjust::setControlTooltip(const std::string& name, const std::string& tooltip)
{
    get_floater_child<LLUICtrl>(this, name)->setToolTip(tooltip);
}

void LLFloaterEnvironmentAdjust::markLocalPreset()
{
//MK
    // Clear the name of the preset
    gAgent.mRRInterface.setLastLoadedPreset("Local");
//mk
}


void LLFloaterEnvironmentAdjust::captureCurrentEnvironment()
{
    LLEnvironment &environment(LLEnvironment::instance());
    bool updatelocal(false);

    if (environment.hasEnvironment(LLEnvironment::ENV_LOCAL))
    {
        if (environment.getEnvironmentDay(LLEnvironment::ENV_LOCAL))
        {   // We have a full day cycle in the local environment.  Freeze the sky
            mLiveSky = environment.getEnvironmentFixedSky(LLEnvironment::ENV_LOCAL)->buildClone();
            mLiveWater = environment.getEnvironmentFixedWater(LLEnvironment::ENV_LOCAL)->buildClone();
            updatelocal = true;
        }
        else
        {   // otherwise we can just use the sky.
            mLiveSky = environment.getEnvironmentFixedSky(LLEnvironment::ENV_LOCAL);
            mLiveWater = environment.getEnvironmentFixedWater(LLEnvironment::ENV_LOCAL);
        }
    }
    else
    {
        mLiveSky = environment.getEnvironmentFixedSky(LLEnvironment::ENV_PARCEL, true)->buildClone();
        mLiveWater = environment.getEnvironmentFixedWater(LLEnvironment::ENV_PARCEL, true)->buildClone();
        updatelocal = true;
    }

    if (updatelocal)
    {
        environment.setEnvironment(LLEnvironment::ENV_LOCAL, mLiveSky, FLOATER_ENVIRONMENT_UPDATE);
        environment.setEnvironment(LLEnvironment::ENV_LOCAL, mLiveWater, FLOATER_ENVIRONMENT_UPDATE);
    }
    environment.setSelectedEnvironment(LLEnvironment::ENV_LOCAL, LLEnvironment::TRANSITION_INSTANT);
}

void LLFloaterEnvironmentAdjust::onButtonReset()
{
    LLNotificationsUtil::add("PersonalSettingsConfirmReset", LLSD(), LLSD(),
        [this](const LLSD&notif, const LLSD&resp)
    {
        S32 opt = LLNotificationsUtil::getSelectedOption(notif, resp);
        if (opt == 0)
        {
            this->closeFloater();
            LLEnvironment::instance().clearEnvironment(LLEnvironment::ENV_LOCAL);
            LLEnvironment::instance().setSelectedEnvironment(LLEnvironment::ENV_LOCAL);
        }
    });

}
//-------------------------------------------------------------------------
void LLFloaterEnvironmentAdjust::onAmbientLightChanged()
{
    if (!mLiveSky)
        return;
    mLiveSky->setAmbientColor(LLColor3(getColorValue(FIELD_SKY_AMBIENT_LIGHT) * SLIDER_SCALE_SUN_AMBIENT));
    mLiveSky->update();
    markLocalPreset();
}

void LLFloaterEnvironmentAdjust::onBlueHorizonChanged()
{
    if (!mLiveSky)
        return;
    mLiveSky->setBlueHorizon(LLColor3(getColorValue(FIELD_SKY_BLUE_HORIZON) * SLIDER_SCALE_BLUE_HORIZON_DENSITY));
    mLiveSky->update();
    markLocalPreset();
}

void LLFloaterEnvironmentAdjust::onBlueDensityChanged()
{
    if (!mLiveSky)
        return;
    mLiveSky->setBlueDensity(LLColor3(getColorValue(FIELD_SKY_BLUE_DENSITY) * SLIDER_SCALE_BLUE_HORIZON_DENSITY));
    mLiveSky->update();
    markLocalPreset();
}

void LLFloaterEnvironmentAdjust::onHazeHorizonChanged()
{
    if (!mLiveSky)
        return;
    mLiveSky->setHazeHorizon(getControlF32(FIELD_SKY_HAZE_HORIZON));
    mLiveSky->update();
    markLocalPreset();
}

void LLFloaterEnvironmentAdjust::onHazeDensityChanged()
{
    if (!mLiveSky)
        return;
    mLiveSky->setHazeDensity(getControlF32(FIELD_SKY_HAZE_DENSITY));
    mLiveSky->update();
    markLocalPreset();
}

void LLFloaterEnvironmentAdjust::onSceneGammaChanged()
{
    if (!mLiveSky)
        return;
    mLiveSky->setGamma(getControlF32(FIELD_SKY_SCENE_GAMMA));
    mLiveSky->update();
    markLocalPreset();
}

void LLFloaterEnvironmentAdjust::onCloudColorChanged()
{
    if (!mLiveSky)
        return;
    mLiveSky->setCloudColor(LLColor3(getColorValue(FIELD_SKY_CLOUD_COLOR)));
    mLiveSky->update();
    markLocalPreset();
}

void LLFloaterEnvironmentAdjust::onCloudCoverageChanged()
{
    if (!mLiveSky)
        return;
    mLiveSky->setCloudShadow(getControlF32(FIELD_SKY_CLOUD_COVERAGE));
    mLiveSky->update();
    markLocalPreset();
}

void LLFloaterEnvironmentAdjust::onCloudScaleChanged()
{
    if (!mLiveSky)
        return;
    mLiveSky->setCloudScale(getControlF32(FIELD_SKY_CLOUD_SCALE));
    mLiveSky->update();
    markLocalPreset();
}

void LLFloaterEnvironmentAdjust::onGlowChanged()
{
    if (!mLiveSky)
        return;
    LLColor3 glow(getControlF32(FIELD_SKY_GLOW_SIZE), 0.0f, getControlF32(FIELD_SKY_GLOW_FOCUS));

    // takes 0 - 1.99 UI range -> 40 -> 0.2 range
    glow.mV[0] = (2.0f - glow.mV[0]) * SLIDER_SCALE_GLOW_R;
    glow.mV[2] *= SLIDER_SCALE_GLOW_B;

    mLiveSky->setGlow(glow);
    mLiveSky->update();
    markLocalPreset();
}

void LLFloaterEnvironmentAdjust::onStarBrightnessChanged()
{
    if (!mLiveSky)
        return;
    mLiveSky->setStarBrightness(getControlF32(FIELD_SKY_STAR_BRIGHTNESS));
    mLiveSky->update();
    markLocalPreset();
}

void LLFloaterEnvironmentAdjust::onSunRotationChanged()
{
    LLQuaternion quat = getTrackballRotation(FIELD_SKY_SUN_ROTATION);
    F32 azimuth;
    F32 elevation;
    LLVirtualTrackball::getAzimuthAndElevationDeg(quat, azimuth, elevation);
    setControlValue(FIELD_SKY_SUN_AZIMUTH, azimuth);
    setControlValue(FIELD_SKY_SUN_ELEVATION, elevation);
    if (mLiveSky)
    {
        mLiveSky->setSunRotation(quat);
        mLiveSky->update();
    }
}

void LLFloaterEnvironmentAdjust::onSunAzimElevChanged()
{
    F32 azimuth = getControlF32(FIELD_SKY_SUN_AZIMUTH);
    F32 elevation = getControlF32(FIELD_SKY_SUN_ELEVATION);
    LLQuaternion quat;

    azimuth *= DEG_TO_RAD;
    elevation *= DEG_TO_RAD;

    if (is_approx_zero(elevation))
    {
        elevation = F_APPROXIMATELY_ZERO;
    }

    quat.setAngleAxis(-elevation, 0, 1, 0);
    LLQuaternion az_quat;
    az_quat.setAngleAxis(F_TWO_PI - azimuth, 0, 0, 1);
    quat *= az_quat;

    setTrackballRotation(FIELD_SKY_SUN_ROTATION, quat);

    if (mLiveSky)
    {
        mLiveSky->setSunRotation(quat);
        mLiveSky->update();
    }
    markLocalPreset();
}

void LLFloaterEnvironmentAdjust::onSunScaleChanged()
{
    if (!mLiveSky)
        return;
    mLiveSky->setSunScale(getControlF32(FIELD_SKY_SUN_SCALE));
    mLiveSky->update();
    markLocalPreset();
}

void LLFloaterEnvironmentAdjust::onMoonRotationChanged()
{
    LLQuaternion quat = getTrackballRotation(FIELD_SKY_MOON_ROTATION);
    F32 azimuth;
    F32 elevation;
    LLVirtualTrackball::getAzimuthAndElevationDeg(quat, azimuth, elevation);
    setControlValue(FIELD_SKY_MOON_AZIMUTH, azimuth);
    setControlValue(FIELD_SKY_MOON_ELEVATION, elevation);
    if (mLiveSky)
    {
        mLiveSky->setMoonRotation(quat);
        mLiveSky->update();
    }
}

void LLFloaterEnvironmentAdjust::onMoonAzimElevChanged()
{
    F32 azimuth = getControlF32(FIELD_SKY_MOON_AZIMUTH);
    F32 elevation = getControlF32(FIELD_SKY_MOON_ELEVATION);
    LLQuaternion quat;

    azimuth *= DEG_TO_RAD;
    elevation *= DEG_TO_RAD;

    if (is_approx_zero(elevation))
    {
        elevation = F_APPROXIMATELY_ZERO;
    }

    quat.setAngleAxis(-elevation, 0, 1, 0);
    LLQuaternion az_quat;
    az_quat.setAngleAxis(F_TWO_PI - azimuth, 0, 0, 1);
    quat *= az_quat;

    setTrackballRotation(FIELD_SKY_MOON_ROTATION, quat);

    if (mLiveSky)
    {
        mLiveSky->setMoonRotation(quat);
        mLiveSky->update();
    }
    markLocalPreset();
}

void LLFloaterEnvironmentAdjust::onCloudMapChanged()
{
    if (!mLiveSky)
    {
        return;
    }

    LLUUID new_texture_id = getTextureValue(FIELD_SKY_CLOUD_MAP);

    LLEnvironment::instance().setSelectedEnvironment(LLEnvironment::ENV_LOCAL);

    LLSettingsSky::ptr_t sky_to_set = mLiveSky->buildClone();
    if (!sky_to_set)
    {
        return;
    }

    sky_to_set->setCloudNoiseTextureId(new_texture_id);

    LLEnvironment::instance().setEnvironment(LLEnvironment::ENV_LOCAL, sky_to_set);

    LLEnvironment::instance().updateEnvironment(LLEnvironment::TRANSITION_INSTANT, true);

    setTextureValue(FIELD_SKY_CLOUD_MAP, new_texture_id);
    markLocalPreset();
}

void LLFloaterEnvironmentAdjust::onWaterMapChanged()
{
    if (!mLiveWater)
        return;
    mLiveWater->setNormalMapID(getTextureValue(FIELD_WATER_NORMAL_MAP));
    mLiveWater->update();
    markLocalPreset();
}

void LLFloaterEnvironmentAdjust::onSunColorChanged()
{
    if (!mLiveSky)
        return;
    LLColor3 color(getColorValue(FIELD_SKY_SUN_COLOR));

    color *= SLIDER_SCALE_SUN_AMBIENT;

    mLiveSky->setSunlightColor(color);
    mLiveSky->update();
    markLocalPreset();
}

void LLFloaterEnvironmentAdjust::onReflectionProbeAmbianceChanged()
{
    if (!mLiveSky) return;
    F32 ambiance = getControlF32(FIELD_REFLECTION_PROBE_AMBIANCE);
    mLiveSky->setReflectionProbeAmbiance(ambiance);

    updateGammaLabel();
    mLiveSky->update();
}

void LLFloaterEnvironmentAdjust::updateGammaLabel()
{
    if (!mLiveSky) return;

    static LLCachedControl<bool> should_auto_adjust(gSavedSettings, "RenderSkyAutoAdjustLegacy", false);
    F32 ambiance = mLiveSky->getReflectionProbeAmbiance(should_auto_adjust);
    if (ambiance != 0.f)
    {
        childSetValue("scene_gamma_label", getString("hdr_string"));
        setControlTooltip(FIELD_SKY_SCENE_GAMMA, getString("hdr_tooltip"));
    }
    else
    {
        childSetValue("scene_gamma_label", getString("brightness_string"));
        setControlTooltip(FIELD_SKY_SCENE_GAMMA, std::string());
    }
}

void LLFloaterEnvironmentAdjust::onEnvironmentUpdated(LLEnvironment::EnvSelection_t env, S32 version)
{
    if (env == LLEnvironment::ENV_LOCAL)
    {   // a new local environment has been applied
        if (version != FLOATER_ENVIRONMENT_UPDATE)
        {   // not by this floater
            captureCurrentEnvironment();
            refresh();
        }
    }
}
