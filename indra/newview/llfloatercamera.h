/**
 * @file llfloatercamera.h
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

#ifndef LLFLOATERCAMERA_H
#define LLFLOATERCAMERA_H

#include "llfloater.h"
#include "lliconctrl.h"
#include "lltextbox.h"
#include "llflatlistview.h"

#define FLOATERCAMERA_MIN_WIDTH 230
#define FLOATERCAMERA_MAX_WIDTH 400

class LLJoystickCameraRotate;
class LLJoystickCameraTrack;
class LLFloaterReg;
class LLPanelCameraZoom;
class LLComboBox;

enum ECameraControlMode
{
    CAMERA_CTRL_MODE_PAN,
    CAMERA_CTRL_MODE_FREE_CAMERA,
    CAMERA_CTRL_MODE_PRESETS
};

class LLFloaterCamera
    :   public LLFloater
{
    friend class LLFloaterReg;

public:
    /* return instance if it exists - created by LLFloaterReg */
    static LLFloaterCamera* findInstance();

    /* update camera modes items selection and camera preset items selection according to the currently selected preset */
    void updateItemsSelection();
    /* whether in free camera mode */
    static bool inFreeCameraMode();
    /* callback for camera items selection changing */
    static void onClickCameraItem(const LLSD& param);
    /* Kokua addition for expand/contract floater */
    static void onClickResize(const LLSD& param);
    /* Kokua addition to persist the expand/contract setting */
    static void doResize(bool reduced);
    static bool handleKokuaCameraPresetsHidden(const LLSD& newvalue);

    static void onLeavingMouseLook();

    /** resets current camera mode to orbit mode */
    static void resetCameraMode();

    /** Called when Avatar is entered/exited editing appearance mode */
    static void onAvatarEditingAppearance(bool editing);

    /** Called when opening and when "Advanced | Debug Camera" menu item is toggled */
    static void onDebugCameraToggled();

    /* determines actual mode and updates ui */
    void update();

    /*switch to one of the camera presets (front, rear, side)*/
    static void switchToPreset(const std::string& name);

    virtual void onOpen(const LLSD& key);
    virtual void onClose(bool app_quitting);
    virtual void reshape(S32 width, S32 height, bool called_from_parent = true);

    // MARE: idle callback polls mouse position to fade non-joystick panels.
    // Does NOT touch draw() or handleHover() so joystick input is untouched.
    static void idleCB(void* user_data);

    void onSavePreset();
    void onCustomPresetSelected();

    void populatePresetCombo();

    LLJoystickCameraRotate* mRotate { nullptr };
    LLPanelCameraZoom* mZoom { nullptr };
    LLJoystickCameraTrack* mTrack { nullptr };

private:

    LLFloaterCamera(const LLSD& val);
    ~LLFloaterCamera() {};

    /*virtual*/ bool postBuild();

    F32 getCurrentTransparency();

    void setupChildControls();
    void setupAdvancedControls();
    void syncSavePresetVisibility(S32 height);
    bool isMouseOverFloater();
    void syncFadeState(bool faded);
    void setCameraItemSelected(const std::string& item_name, bool selected);

    void onViewButtonClick(const LLSD& user_data);

    ECameraControlMode determineMode();

    /* resets to the previous mode */
    void toPrevMode();

    /* sets a new mode and performs related actions */
    void switchMode(ECameraControlMode mode);

    /* sets a new mode preserving previous one and updates ui*/
    void setMode(ECameraControlMode mode);

    /* updates the state (UI) according to the current mode */
    void updateState();

// public
//  /* update camera modes items selection and camera preset items selection according to the currently selected preset */
//  void updateItemsSelection();

    // fills flatlist with items from given panel
    void fillFlatlistFromPanel (LLFlatListView* list, LLPanel* panel);

    void handleAvatarEditingAppearance(bool editing);

    void showDebugInfo(bool show);

    // set to true when free camera mode is selected in modes list
    // remains true until preset camera mode is chosen, or pan button is clicked, or escape pressed
    static bool sFreeCamera;
    static bool sAppearanceEditing;
    bool mClosed;

    bool mUseFlatUI;
    ECameraControlMode mPrevMode;
    ECameraControlMode mCurrMode;
    std::map<ECameraControlMode, LLButton*> mMode2Button;

    LLPanel* mViewerCameraInfo { nullptr };
    LLPanel* mAgentCameraInfo { nullptr };
    LLComboBox* mPresetCombo { nullptr };
    LLTextBox* mPreciseCtrls { nullptr };

    // MARE: idle-callback fade. Mouse position polled each frame outside draw()
    // so joystick input is never touched.
    static const F32 COLLAPSE_DELAY;   // seconds after unhover before fading out
    static const F32 FADE_SPEED;       // alpha units per second for the transition

    F32          mFadeAlpha  { 0.f   }; // 0=faded, 1=full
    LLFrameTimer mUnhoverTimer;         // time since mouse left the floater
};

/**
 * Class used to represent widgets from panel_camera_item.xml-
 * panels that contain pictures and text. Pictures are different
 * for selected and unselected state (this state is nor stored- icons
 * are changed in setValue()). This class doesn't implement selection logic-
 * it's items are used inside of flatlist.
 */
class LLPanelCameraItem
    : public LLPanel
{
public:
    struct Params : public LLInitParam::Block<Params, LLPanel::Params>
    {
        Optional<LLIconCtrl::Params> icon_over;
        Optional<LLIconCtrl::Params> icon_selected;
        Optional<LLIconCtrl::Params> picture;
        Optional<LLIconCtrl::Params> selected_picture;

        Optional<LLTextBox::Params> text;
        Optional<CommitCallbackParam> mousedown_callback;
        Params();
    };
    /*virtual*/ bool postBuild();
    /** setting on/off background icon to indicate selected state */
    /*virtual*/ void setValue(const LLSD& value);
    // sends commit signal
    void onAnyMouseClick();
protected:
    friend class LLUICtrlFactory;
    LLPanelCameraItem(const Params&);
    LLIconCtrl* mIconOver;
    LLIconCtrl* mIconSelected;
    LLIconCtrl* mPicture;
    LLIconCtrl* mPictureSelected;
    LLTextBox* mText;
};

#endif
