/**
 * @file llfloatercamera.cpp
 * @brief Container for camera control buttons (zoom, pan, orbit)
 *
 * $LicenseInfo:firstyear=2001&license=viewerlgpl$
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

#include "llfloatercamera.h"

// Library includes
#include "llfloaterreg.h"

// Viewer includes
#include "llagent.h"
#include "llagentcamera.h"
#include "llpresetsmanager.h"
#include "lljoystickbutton.h"
#include "llviewercontrol.h"
#include "llviewercamera.h"
#include "lltoolmgr.h"
#include "lltoolfocus.h"
#include "llslider.h"
#include "llfirstuse.h"
#include "llhints.h"
#include "lltabcontainer.h"
#include "llviewercamera.h"
#include "llvoavatarself.h"
#include "llcallbacklist.h"  // gIdleCallbacks

static LLDefaultChildRegistry::Register<LLPanelCameraItem> r("panel_camera_item");

const F32 NUDGE_TIME = 0.25f;       // in seconds
const F32 ORBIT_NUDGE_RATE = 0.05f; // fraction of normal speed

// constants
#define ORBIT "cam_rotate_stick"
#define PAN "cam_track_stick"
#define ZOOM "zoom"
#define CONTROLS "controls"

bool LLFloaterCamera::sFreeCamera = false;
bool LLFloaterCamera::sAppearanceEditing = false;

// Zoom the camera in and out
class LLPanelCameraZoom
:   public LLPanel
{
    LOG_CLASS(LLPanelCameraZoom);

public:
    struct Params : public LLInitParam::Block<Params, LLPanel::Params> {};

    LLPanelCameraZoom() { onCreate(); }

    /* virtual */ bool  postBuild();
    /* virtual */ void  draw();
    /* virtual */ void  reshape(S32 width, S32 height, bool called_from_parent = true);

protected:
    LLPanelCameraZoom(const Params& p) { onCreate(); }

    void    onCreate();
    void    onZoomPlusHeldDown();
    void    onZoomMinusHeldDown();
    void    onSliderValueChanged();
    void    onCameraTrack();
    void    onCameraRotate();
    F32     getOrbitRate(F32 time);
    void    setupControls();
    void    syncSliderFromCamera();
    void    syncJoystickShapes();
    void    resizeJoystickToPanel(const char* panel_name, const char* stick_name);

private:
    LLButton*   mPlusBtn { nullptr };
    LLButton*   mMinusBtn{ nullptr };
    LLSlider*   mSlider{ nullptr };

    friend class LLUICtrlFactory;
};

LLPanelCameraItem::Params::Params()
:   icon_over("icon_over"),
    icon_selected("icon_selected"),
    picture("picture"),
    text("text"),
    selected_picture("selected_picture"),
    mousedown_callback("mousedown_callback")
{
}

LLPanelCameraItem::LLPanelCameraItem(const LLPanelCameraItem::Params& p)
:   LLPanel(p)
{
    LLIconCtrl::Params icon_params = p.picture;
    mPicture = LLUICtrlFactory::create<LLIconCtrl>(icon_params);
    addChild(mPicture);

    icon_params = p.icon_over;
    mIconOver = LLUICtrlFactory::create<LLIconCtrl>(icon_params);
    addChild(mIconOver);

    icon_params = p.icon_selected;
    mIconSelected = LLUICtrlFactory::create<LLIconCtrl>(icon_params);
    addChild(mIconSelected);

    icon_params = p.selected_picture;
    mPictureSelected = LLUICtrlFactory::create<LLIconCtrl>(icon_params);
    addChild(mPictureSelected);

    LLTextBox::Params text_params = p.text;
    mText = LLUICtrlFactory::create<LLTextBox>(text_params);
    addChild(mText);

    if (p.mousedown_callback.isProvided())
    {
        setCommitCallback(initCommitCallback(p.mousedown_callback));
    }
}

void set_view_visible(LLView* parent, const std::string& name, bool visible)
{
    parent->getChildView(name)->setVisible(visible);
}

void LLPanelCameraItem::setValue(const LLSD& value)
{
    if (!value.isMap()) return;;
    if (!value.has("selected")) return;
    const bool selected = value["selected"].asBoolean();
    getChildView("selected_icon")->setVisible(selected);
    getChildView("picture")->setVisible(!selected);
    getChildView("selected_picture")->setVisible(selected);
}

bool LLPanelCameraItem::postBuild()
{
    setMouseEnterCallback(boost::bind(set_view_visible, this, "hovered_icon", true));
    setMouseLeaveCallback(boost::bind(set_view_visible, this, "hovered_icon", false));
    setMouseDownCallback(boost::bind(&LLPanelCameraItem::onAnyMouseClick, this));
    setRightMouseDownCallback(boost::bind(&LLPanelCameraItem::onAnyMouseClick, this));
    return true;
}

void LLPanelCameraItem::onAnyMouseClick()
{
    if (mCommitSignal) (*mCommitSignal)(this, LLSD());
}


static LLPanelInjector<LLPanelCameraZoom> t_camera_zoom_panel("camera_zoom_panel");

//-------------------------------------------------------------------------------
// LLPanelCameraZoom
//-------------------------------------------------------------------------------

void LLPanelCameraZoom::onCreate()
{
    mCommitCallbackRegistrar.add("Zoom.minus", boost::bind(&LLPanelCameraZoom::onZoomMinusHeldDown, this));
    mCommitCallbackRegistrar.add("Zoom.plus", boost::bind(&LLPanelCameraZoom::onZoomPlusHeldDown, this));
    mCommitCallbackRegistrar.add("Slider.value_changed", boost::bind(&LLPanelCameraZoom::onSliderValueChanged, this));
    mCommitCallbackRegistrar.add("Camera.track", boost::bind(&LLPanelCameraZoom::onCameraTrack, this));
    mCommitCallbackRegistrar.add("Camera.rotate", boost::bind(&LLPanelCameraZoom::onCameraRotate, this));
}

bool LLPanelCameraZoom::postBuild()
{
    setupControls();
    return LLPanel::postBuild();
}

void LLPanelCameraZoom::setupControls()
{
    mPlusBtn  = getChild<LLButton>("zoom_plus_btn");
    mMinusBtn = getChild<LLButton>("zoom_minus_btn");
    mSlider   = getChild<LLSlider>("zoom_slider");
}

void LLPanelCameraZoom::draw()
{
    // Size joystick widgets each frame ONLY when not dragging.
    // LLLayoutStack::updateLayout() runs during the parent draw pass and
    // overwrites layout_panel rects after reshape(), so we must do this here.
    //
    // Keep joystick widgets square and as large as possible within their panels.
    // MUST be square: LLJoystick::pointInCenterDot() returns true for any
    // non-square widget, treating every click as a center-dot reset so the
    // camera never orbits.  We use min(pw,ph) so the widget scales with the
    // floater.  Only resize when no drag is active — changing the rect
    // mid-drag shifts the coordinate origin and breaks input.
    syncJoystickShapes();
    syncSliderFromCamera();
    LLPanel::draw();
}

void LLPanelCameraZoom::reshape(S32 width, S32 height, bool called_from_parent)
{
    LLPanel::reshape(width, height, called_from_parent);
}

void LLPanelCameraZoom::onZoomPlusHeldDown()
{
    F32 val = mSlider->getValueF32();
    F32 inc = mSlider->getIncrement();
    mSlider->setValue(val - inc);
    F32 time = mPlusBtn->getHeldDownTime();
    gAgentCamera.unlockView();
    gAgentCamera.setOrbitInKey(getOrbitRate(time));
}

void LLPanelCameraZoom::onZoomMinusHeldDown()
{
    F32 val = mSlider->getValueF32();
    F32 inc = mSlider->getIncrement();
    mSlider->setValue(val + inc);
    F32 time = mMinusBtn->getHeldDownTime();
    gAgentCamera.unlockView();
    gAgentCamera.setOrbitOutKey(getOrbitRate(time));
}

void LLPanelCameraZoom::onCameraTrack()
{
    // EXP-202 when camera panning activated, remove the hint
    LLFirstUse::viewPopup( false );
}

void LLPanelCameraZoom::onCameraRotate()
{
    // EXP-202 when camera rotation activated, remove the hint
    LLFirstUse::viewPopup( false );
}

F32 LLPanelCameraZoom::getOrbitRate(F32 time)
{
    if( time < NUDGE_TIME )
    {
        F32 rate = ORBIT_NUDGE_RATE + time * (1 - ORBIT_NUDGE_RATE)/ NUDGE_TIME;
        return rate;
    }
    else
    {
        return 1;
    }
}

void  LLPanelCameraZoom::onSliderValueChanged()
{
    F32 zoom_level = mSlider->getValueF32();
    gAgentCamera.setCameraZoomFraction(zoom_level);
}

void LLPanelCameraZoom::syncSliderFromCamera()
{
    mSlider->setValue(gAgentCamera.getCameraZoomFraction());
}

void LLPanelCameraZoom::syncJoystickShapes()
{
    if (gFocusMgr.getMouseCapture())
    {
        return;
    }

    resizeJoystickToPanel("rotate_panel", "cam_rotate_stick");
    resizeJoystickToPanel("track_panel",  "cam_track_stick");
}

void LLPanelCameraZoom::resizeJoystickToPanel(const char* panel_name, const char* stick_name)
{
    LLView* panel = findChildView(panel_name, true);
    LLView* stick = findChildView(stick_name, true);
    if (!panel || !stick)
    {
        return;
    }

    S32 pw = panel->getRect().getWidth();
    S32 ph = panel->getRect().getHeight();
    S32 sz = llmin(pw, ph);
    S32 ox = (pw - sz) / 2;
    S32 oy = (ph - sz) / 2;
    stick->setShape(LLRect(ox, ph - oy, ox + sz, ph - oy - sz));
}

void activate_camera_tool()
{
    LLToolMgr::getInstance()->setTransientTool(LLToolCamera::getInstance());
};

class LLCameraInfoPanel : public LLPanel
{
public:
    typedef std::function<LLVector3()> get_vector_t;

    LLCameraInfoPanel(
        const LLView* parent,
        const char* title,
        const LLCoordFrame& camera,
        const get_vector_t get_focus
    )
    : LLPanel([&]() -> LLPanel::Params
        {
            LLPanel::Params params;
            params.rect = LLRect(parent->getLocalRect());
            return params;
        }())
    , mTitle(title)
    , mCamera(camera)
    , mGetFocus(get_focus)
    , mFont(LLFontGL::getFontSansSerifBig())
    {
    }

    virtual void draw() override
    {
        LLPanel::draw();

        static const U32 HPADDING = 10;
        static const U32 VPADDING = 5;
        LLVector3 focus = mGetFocus();
        LLVector3 sight = focus - mCamera.mOrigin;
        std::pair<const char*, const LLVector3&> const data[] =
        {
            { "Origin:", mCamera.mOrigin },
            { "X Axis:", mCamera.mXAxis },
            { "Y Axis:", mCamera.mYAxis },
            { "Z Axis:", mCamera.mZAxis },
            { "Focus:", focus },
            { "Sight:", sight }
        };
        S32 width = getRect().getWidth();
        S32 height = getRect().getHeight();
        S32 row_count = 1 + sizeof(data) / sizeof(*data);
        S32 row_height = (height - VPADDING * 2) / row_count;
        S32 top = height - VPADDING - row_height / 2;
        mFont->renderUTF8(mTitle, 0, HPADDING, top, LLColor4::white, LLFontGL::LEFT, LLFontGL::VCENTER);
        for (const auto& row : data)
        {
            top -= row_height;
            mFont->renderUTF8(row.first, 0, HPADDING, top, LLColor4::white, LLFontGL::LEFT, LLFontGL::VCENTER);
            const LLVector3& vector = row.second;
            for (S32 i = 0; i < 3; ++i)
            {
                std::string text = llformat("%.6f", vector[i]);
                S32 right = width / 4 * (i + 2) - HPADDING;
                mFont->renderUTF8(text, 0, right, top, LLColor4::white, LLFontGL::RIGHT, LLFontGL::VCENTER);
            }
        }
    }

private:
    const char* mTitle;
    const LLCoordFrame& mCamera;
    const get_vector_t mGetFocus;
    const LLFontGL* mFont;
};

//
// Member functions
//

// static
bool LLFloaterCamera::inFreeCameraMode()
{
    LLFloaterCamera* floater_camera = LLFloaterCamera::findInstance();
    if (floater_camera && floater_camera->mCurrMode == CAMERA_CTRL_MODE_FREE_CAMERA && gAgentCamera.getCameraMode() != CAMERA_MODE_MOUSELOOK)
    {
        return true;
    }
    return false;
}

// static
void LLFloaterCamera::resetCameraMode()
{
    LLFloaterCamera* floater_camera = LLFloaterCamera::findInstance();
    if (!floater_camera) return;
    floater_camera->switchMode(CAMERA_CTRL_MODE_PAN);
}

// static
void LLFloaterCamera::onAvatarEditingAppearance(bool editing)
{
    sAppearanceEditing = editing;
    LLFloaterCamera* floater_camera = LLFloaterCamera::findInstance();
    if (!floater_camera) return;
    floater_camera->handleAvatarEditingAppearance(editing);
}

// static
void LLFloaterCamera::onDebugCameraToggled()
{
    if (LLFloaterCamera* instance = LLFloaterCamera::findInstance())
    {
        instance->showDebugInfo(LLView::sDebugCamera);
    }

    if (LLView::sDebugCamera)
    {
        LLFloaterReg::showInstanceOrBringToFront("camera");
    }
}

void LLFloaterCamera::showDebugInfo(bool show)
{
    // Initially LLPanel contains 1 child "view_border"
    if (show && mViewerCameraInfo->getChildCount() < 2)
    {
        mViewerCameraInfo->addChild(new LLCameraInfoPanel(mViewerCameraInfo, "Viewer Camera", *LLViewerCamera::getInstance(),
            []() { return LLViewerCamera::getInstance()->getPointOfInterest(); }));
        mAgentCameraInfo->addChild(new LLCameraInfoPanel(mAgentCameraInfo, "Agent Camera", gAgent.getFrameAgent(),
            []() { return gAgent.getPosAgentFromGlobal(gAgentCamera.calcFocusPositionTargetGlobal()); }));
    }

    mAgentCameraInfo->setVisible(show);
    mViewerCameraInfo->setVisible(show);
}

void LLFloaterCamera::handleAvatarEditingAppearance(bool editing)
{
}

void LLFloaterCamera::update()
{
    ECameraControlMode mode = determineMode();
    if (mode != mCurrMode)
    {
        setMode(mode);
    }
}


void LLFloaterCamera::toPrevMode()
{
    switchMode(mPrevMode);
}

// static
void LLFloaterCamera::onLeavingMouseLook()
{
    LLFloaterCamera* floater_camera = LLFloaterCamera::findInstance();
    if (floater_camera)
    {
        floater_camera->updateItemsSelection();
        if(floater_camera->inFreeCameraMode())
        {
            activate_camera_tool();
        }
    }
}

LLFloaterCamera* LLFloaterCamera::findInstance()
{
    return LLFloaterReg::findTypedInstance<LLFloaterCamera>("camera");
}

// MARE v1.1.0: fully proportional reshape so all three rows scale together.
//
// Layout (y increases upward in local floater coords):
//
//   [buttons_panel]        proportional ~17% of content height (28–52 px)
//   [zoom]                 fills the middle, min MIN_ZOOM_H
//   [preset_buttons_panel] scales between PRESET_MIN and PRESET_MAX at bottom
//
// buttons_panel: height scales so the row stays visually balanced at any size.
// At the default floater height (230 px, ch=212), buttons_h = round(212*0.17)
// = 36 px — identical to the old fixed value, so the default view is unchanged.
// The five panel_camera_item children are redistributed evenly across the full
// panel width and their icon layers are resized to fill each item square.
void LLFloaterCamera::reshape(S32 width, S32 height, bool called_from_parent)
{
    // Let the XML follows= system handle all panel sizing/positioning.
    LLFloater::reshape(width, height, called_from_parent);

    syncSavePresetVisibility(height);
}

void LLFloaterCamera::onOpen(const LLSD& key)
{
    LLFirstUse::viewPopup();

    mZoom->onOpen(key);

    // Returns to previous mode, see EXT-2727(View tool should remember state).
    // In case floater was just hidden and it isn't reset the mode
    // just update state to current one. Else go to previous.
    if ( !mClosed )
        updateState();
    else
        toPrevMode();
    mClosed = false;

    populatePresetCombo();
    showDebugInfo(LLView::sDebugCamera);

    // MARE: open faded — controls hidden until user hovers.
    mFadeAlpha = 0.f;
    mUnhoverTimer.reset();
    gIdleCallbacks.addFunction(idleCB, this);
}

void LLFloaterCamera::onClose(bool app_quitting)
{
    gIdleCallbacks.deleteFunction(idleCB, this);

    //We don't care of camera mode if app is quitting
    if (app_quitting)
        return;
    // It is necessary to reset mCurrMode to CAMERA_CTRL_MODE_PAN so
    // to avoid seeing an empty floater when reopening the control.
    if (mCurrMode == CAMERA_CTRL_MODE_FREE_CAMERA)
        mCurrMode = CAMERA_CTRL_MODE_PAN;
    // When mCurrMode is in CAMERA_CTRL_MODE_PAN
    // switchMode won't modify mPrevMode, so force it here.
    // It is needed to correctly return to previous mode on open, see EXT-2727.
    if (mCurrMode == CAMERA_CTRL_MODE_PAN)
        mPrevMode = CAMERA_CTRL_MODE_PAN;

    switchMode(CAMERA_CTRL_MODE_PAN);
    mClosed = true;

    gAgent.setMovementLocked(false);
}

LLFloaterCamera::LLFloaterCamera(const LLSD& val)
:   LLFloater(val),
    mClosed(false),
    mUseFlatUI(false),
    mCurrMode(CAMERA_CTRL_MODE_PAN),
    mPrevMode(CAMERA_CTRL_MODE_PAN)
{
    LLHints::getInstance()->registerHintTarget("view_popup", getHandle());
    mCommitCallbackRegistrar.add("CameraPresets.ChangeView", boost::bind(&LLFloaterCamera::onClickCameraItem, _2));
    // CameraPresets.Resize removed — expand/contract mechanism dropped in MARE v1.1.0 (Firestorm-style layout)
    mCommitCallbackRegistrar.add("CameraPresets.Save", boost::bind(&LLFloaterCamera::onSavePreset, this));
    mCommitCallbackRegistrar.add("CameraPresets.ShowPresetsList", boost::bind(&LLFloaterReg::showInstance, "camera_presets", LLSD(), false));
}

// virtual
bool LLFloaterCamera::postBuild()
{
    updateTransparency(TT_ACTIVE); // force using active floater transparency (STORM-730)

    setupChildControls();
    setupAdvancedControls();

    update();

    // ensure that appearance mode is handled while building. See EXT-7796.
    handleAvatarEditingAppearance(sAppearanceEditing);

    return LLFloater::postBuild();
}

void LLFloaterCamera::setupChildControls()
{
    mAgentCameraInfo = getChild<LLPanel>("agent_camera_info");
    mViewerCameraInfo = getChild<LLPanel>("viewer_camera_info");
    mRotate = getChild<LLJoystickCameraRotate>(ORBIT);
    mZoom = getChild<LLPanelCameraZoom>(ZOOM);
    mTrack = getChild<LLJoystickCameraTrack>(PAN);
    mPresetCombo = getChild<LLComboBox>("preset_combo");
}

void LLFloaterCamera::setupAdvancedControls()
{
    if (hasString("use_flat_ui"))
    {
        mUseFlatUI = true;
        return;
    }

    mPreciseCtrls = getChild<LLTextBox>("precise_ctrs_label");

    mPreciseCtrls->setShowCursorHand(false);
    mPreciseCtrls->setSoundFlags(LLView::MOUSE_UP);
    mPreciseCtrls->setClickedCallback(boost::bind(&LLFloaterReg::showInstance, "prefs_view_advanced", LLSD(), false));


    mPresetCombo->setCommitCallback(boost::bind(&LLFloaterCamera::onCustomPresetSelected, this));
    LLPresetsManager::getInstance()->setPresetListChangeCameraCallback(boost::bind(&LLFloaterCamera::populatePresetCombo, this));
    // KokuaCameraPresetsHidden signal handler removed — expand/contract dropped (MARE v1.1.0)
}

void LLFloaterCamera::syncSavePresetVisibility(S32 height)
{
    // Hide "Save as preset…" when the floater is too short for two rows.
    S32 ch = height - getHeaderHeight();
    LLView* save_btn = findChildView("save_preset_btn", true);
    if (save_btn)
    {
        save_btn->setVisible(ch >= 180);
    }
}

F32 LLFloaterCamera::getCurrentTransparency()
{
    static LLCachedControl<F32> camera_opacity(gSavedSettings, "CameraOpacity");
    static LLCachedControl<F32> active_floater_transparency(gSavedSettings, "ActiveFloaterTransparency");
    return llmin(camera_opacity(), active_floater_transparency());
}

/*static*/ const F32 LLFloaterCamera::COLLAPSE_DELAY = 0.5f;
/*static*/ const F32 LLFloaterCamera::FADE_SPEED     = 4.0f;  // full fade in 0.25s

// ---------------------------------------------------------------------------
// MARE: idle-callback fade.
//
// We poll mouse position each frame in an idle callback (registered on open,
// unregistered on close).  This runs BEFORE draw() and event dispatch, so it
// never touches GL state or interferes with joystick mouse-capture routing.
//
// When faded:  buttons_panel and preset_buttons_panel are hidden (setVisible
// false blocks input).  The floater background is also hidden.  mZoom is
// always visible so joystick clicks and drag always reach the widgets.
//
// When hovered: alpha ramps up → panels shown → floater looks normal.
// When unhovered: after COLLAPSE_DELAY alpha ramps down → panels hidden.
// ---------------------------------------------------------------------------

// static
void LLFloaterCamera::idleCB(void* user_data)
{
    LLFloaterCamera* self = static_cast<LLFloaterCamera*>(user_data);
    if (!self || !self->isInVisibleChain()) return;

    F32 dt = llclamp(LLFrameTimer::getFrameDeltaTimeF32(), 0.f, 0.1f);

    bool hovered = self->isMouseOverFloater();

    if (hovered)
    {
        self->mUnhoverTimer.reset();
        self->mFadeAlpha = llmin(self->mFadeAlpha + FADE_SPEED * dt, 1.f);
    }
    else if (self->mUnhoverTimer.getElapsedTimeF32() >= COLLAPSE_DELAY)
    {
        self->mFadeAlpha = llmax(self->mFadeAlpha - FADE_SPEED * dt, 0.f);
    }

    self->syncFadeState(self->mFadeAlpha < 0.99f);
}

bool LLFloaterCamera::isMouseOverFloater()
{
    S32 mx, my;
    LLUI::getInstance()->getMousePositionLocal(this, &mx, &my);
    return getLocalRect().pointInRect(mx, my);
}

void LLFloaterCamera::syncFadeState(bool faded)
{
    LLView* buttons = findChildView("buttons_panel",        false);
    LLView* preset  = findChildView("preset_buttons_panel", false);

    if (buttons) buttons->setVisible(!faded);
    if (preset)  preset->setVisible(!faded);
    setBackgroundVisible(!faded);  // hide gray frame when faded
    // mZoom always stays visible — joystick input is never interrupted.
}

void LLFloaterCamera::fillFlatlistFromPanel (LLFlatListView* list, LLPanel* panel)
{
    // copying child list and then iterating over a copy, because list itself
    // is changed in process
    const child_list_t child_list = *panel->getChildList();
    child_list_t::const_reverse_iterator iter = child_list.rbegin();
    child_list_t::const_reverse_iterator end = child_list.rend();
    for ( ; iter != end; ++iter)
    {
        LLView* view = *iter;
        LLPanel* item = dynamic_cast<LLPanel*>(view);
        if (panel)
            list->addItem(item);
    }

}

ECameraControlMode LLFloaterCamera::determineMode()
{
    if (sAppearanceEditing)
    {
        // this is the only enabled camera mode while editing agent appearance.
        return CAMERA_CTRL_MODE_PAN;
    }

    LLTool* curr_tool = LLToolMgr::getInstance()->getCurrentTool();
    if (curr_tool == LLToolCamera::getInstance())
    {
        return CAMERA_CTRL_MODE_FREE_CAMERA;
    }

    if (gAgentCamera.getCameraMode() == CAMERA_MODE_MOUSELOOK)
    {
        return CAMERA_CTRL_MODE_PRESETS;
    }

    return CAMERA_CTRL_MODE_PAN;
}


void clear_camera_tool()
{
    LLToolMgr* tool_mgr = LLToolMgr::getInstance();
    if (tool_mgr->usingTransientTool() &&
        tool_mgr->getCurrentTool() == LLToolCamera::getInstance())
    {
        tool_mgr->clearTransientTool();
    }
}


void LLFloaterCamera::setMode(ECameraControlMode mode)
{
    if (mode != mCurrMode)
    {
        mPrevMode = mCurrMode;
        mCurrMode = mode;
    }

    updateState();
}

void LLFloaterCamera::switchMode(ECameraControlMode mode)
{
    switch (mode)
    {
    case CAMERA_CTRL_MODE_PRESETS:
    case CAMERA_CTRL_MODE_PAN:
        sFreeCamera = false;
        setMode(mode); // depends onto sFreeCamera
        clear_camera_tool();
        break;

    case CAMERA_CTRL_MODE_FREE_CAMERA:
        // KKA-748 change so that a second click on Object View deselects the mode without selecting another mode, so leaving the camera where it is
        if (sFreeCamera)
        {
            sFreeCamera = false;
            setMode(mode);
            clear_camera_tool();
        }
        else
        {
            sFreeCamera = true;
            setMode(mode);
            activate_camera_tool();
        }
        break;

    default:
        //normally we won't occur here
        llassert_always(false);
    }
}

void LLFloaterCamera::updateState()
{
    if (mUseFlatUI)
    {
        return;
    }

    updateItemsSelection();

    if (CAMERA_CTRL_MODE_FREE_CAMERA == mCurrMode)
    {
        return;
    }

    //updating buttons
    std::map<ECameraControlMode, LLButton*>::const_iterator iter = mMode2Button.begin();
    for (; iter != mMode2Button.end(); ++iter)
    {
        iter->second->setToggleState(iter->first == mCurrMode);
    }
}

void LLFloaterCamera::updateItemsSelection()
{
    ECameraPreset preset = (ECameraPreset) gSavedSettings.getU32("CameraPresetType");
    setCameraItemSelected("rear_view", (preset == CAMERA_PRESET_REAR_VIEW) && !sFreeCamera);
    setCameraItemSelected("group_view", (preset == CAMERA_PRESET_GROUP_VIEW) && !sFreeCamera);
    setCameraItemSelected("front_view", (preset == CAMERA_PRESET_FRONT_VIEW) && !sFreeCamera);
    setCameraItemSelected("mouselook_view", gAgentCamera.getCameraMode() == CAMERA_MODE_MOUSELOOK);
    //KKA-748 include sFreeCamera in the validation so that the item will deselect as sFreeCamera toggles
    //argument["selected"] = mCurrMode == CAMERA_CTRL_MODE_FREE_CAMERA;
    setCameraItemSelected("object_view", sFreeCamera && mCurrMode == CAMERA_CTRL_MODE_FREE_CAMERA);
}

void LLFloaterCamera::setCameraItemSelected(const std::string& item_name, bool selected)
{
    LLSD argument;
    argument["selected"] = selected;
    getChild<LLPanelCameraItem>(item_name)->setValue(argument);
}

// static
void LLFloaterCamera::onClickCameraItem(const LLSD& param)
{
    std::string name = param.asString();

    LLFloaterCamera* camera_floater = LLFloaterCamera::findInstance();

    if ("reset_view" == name)
    {
        gAgentCamera.switchCameraPreset(CAMERA_PRESET_REAR_VIEW);
        gAgentCamera.changeCameraToDefault();
        if (camera_floater)
            camera_floater->switchMode(CAMERA_CTRL_MODE_PAN);
    }
    else if ("mouselook_view" == name)
    {
        gAgentCamera.changeCameraToMouselook();
    }
    else if ("object_view" == name && camera_floater)
    {
        if (camera_floater->mUseFlatUI)
        {
            camera_floater->mCurrMode == CAMERA_CTRL_MODE_FREE_CAMERA ? camera_floater->switchMode(CAMERA_CTRL_MODE_PAN) : camera_floater->switchMode(CAMERA_CTRL_MODE_FREE_CAMERA);
        }
        else
        {
            LLFloaterCamera* camera_floater = LLFloaterCamera::findInstance();
            if (camera_floater)
            {
                camera_floater->switchMode(CAMERA_CTRL_MODE_FREE_CAMERA);
                camera_floater->updateItemsSelection();
            }
        }
    }
    else
    {
        LLFloaterCamera* camera_floater = LLFloaterCamera::findInstance();
        if (camera_floater)
            camera_floater->switchMode(CAMERA_CTRL_MODE_PAN);
        switchToPreset(name);
    }
}

// MARE v1.1.0: expand/contract mechanism removed. Firestorm-style 3-panel layout
// (buttons_panel fixed top, zoom follows=all, preset_buttons_panel fixed bottom)
// makes these functions unnecessary. Stubs kept so .h declarations compile cleanly.
/*static*/
bool LLFloaterCamera::handleKokuaCameraPresetsHidden(const LLSD& /*newvalue*/)
{
    return true;
}

/*static*/
void LLFloaterCamera::doResize(bool /*reduced*/)
{
    // No-op: Firestorm-style layout self-manages all sizing via follows attributes.
}

/*static*/
void LLFloaterCamera::onClickResize(const LLSD& /*param*/)
{
    // No-op: expand/contract buttons removed from UI (MARE v1.1.0).
}

// static
void LLFloaterCamera::switchToPreset(const std::string& name)
{
//MK
    if (gRRenabled && gAgent.mRRInterface.mContainsLockedCamera)
    {
        return;
    }
//mk
    sFreeCamera = false;
    clear_camera_tool();
    if (PRESETS_REAR_VIEW == name)
    {
        gAgentCamera.switchCameraPreset(CAMERA_PRESET_REAR_VIEW);
    }
    else if (PRESETS_SIDE_VIEW == name)
    {
        gAgentCamera.switchCameraPreset(CAMERA_PRESET_GROUP_VIEW);
    }
    else if (PRESETS_FRONT_VIEW == name)
    {
        gAgentCamera.switchCameraPreset(CAMERA_PRESET_FRONT_VIEW);
    }
    else
    {
        gAgentCamera.switchCameraPreset(CAMERA_PRESET_CUSTOM);
    }

    if (gSavedSettings.getString("PresetCameraActive") != name)
    {
        LLPresetsManager::getInstance()->loadPreset(PRESETS_CAMERA, name);
    }

    if (isAgentAvatarValid() && gAgentAvatarp->getParent())
    {
        LLQuaternion sit_rot(gSavedSettings.getLLSD("AvatarSitRotation"));
        if (sit_rot != LLQuaternion())
        {
            gAgent.rotate(~gAgent.getFrameAgent().getQuaternion());
            gAgent.rotate(sit_rot);
        }
        else
        {
            gAgentCamera.rotateToInitSitRot();
        }
    }
    gAgentCamera.resetCameraZoomFraction();

    LLFloaterCamera* camera_floater = LLFloaterCamera::findInstance();
    if (camera_floater)
    {
        camera_floater->updateItemsSelection();
        camera_floater->switchMode(CAMERA_CTRL_MODE_PRESETS);
    }
}

void LLFloaterCamera::populatePresetCombo()
{
    LLPresetsManager::getInstance()->setPresetNamesInComboBox(PRESETS_CAMERA, mPresetCombo, EDefaultOptions::DEFAULT_HIDE_IF_ICON);
    std::string active_preset_name = gSavedSettings.getString("PresetCameraActive");
    if (active_preset_name.empty())
    {
        gSavedSettings.setU32("CameraPresetType", CAMERA_PRESET_CUSTOM);
        updateItemsSelection();
        mPresetCombo->setLabel(getString("inactive_combo_text"));
    }
    else if ((ECameraPreset)gSavedSettings.getU32("CameraPresetType") == CAMERA_PRESET_CUSTOM)
    {
        mPresetCombo->selectByValue(active_preset_name);
    }
    else
    {
        mPresetCombo->setLabel(getString("inactive_combo_text"));
    }
    updateItemsSelection();
}

void LLFloaterCamera::onSavePreset()
{
    LLFloaterReg::hideInstance("delete_pref_preset", PRESETS_CAMERA);
    LLFloaterReg::hideInstance("load_pref_preset", PRESETS_CAMERA);

    LLFloaterReg::showInstance("save_camera_preset");
}

void LLFloaterCamera::onCustomPresetSelected()
{
    std::string selected_preset = mPresetCombo->getSelectedItemLabel();
    if (getString("inactive_combo_text") != selected_preset)
    {
        switchToPreset(selected_preset);
    }
}
