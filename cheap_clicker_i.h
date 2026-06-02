#pragma once

#include <furi.h>
#include <furi_hal_bt.h>
#include <furi_hal_usb_hid.h>
#include <bt/bt_service/bt.h>
#include <gui/gui.h>
#include <gui/view_dispatcher.h>
#include <gui/scene_manager.h>
#include <gui/modules/submenu.h>
#include <gui/modules/text_input.h>
#include <gui/modules/variable_item_list.h>
#include <notification/notification.h>
#include <storage/storage.h>
#include <ble_profile/extra_profiles/hid_profile.h>

#include "scenes/cc_scene.h"

#define CC_MAX_BUTTONS  17
#define CC_MAX_PROFILES  5
#define CC_MAX_NAME_LEN 32

typedef struct {
    char name[CC_MAX_NAME_LEN];
    int16_t x;
    int16_t y;
} CcButton;

typedef struct {
    char name[CC_MAX_NAME_LEN];
    char ble_name[CC_MAX_NAME_LEN];
    char keys_path[128];
    int16_t trigger_x;
    int16_t trigger_y;
    uint8_t button_count;
    CcButton buttons[CC_MAX_BUTTONS];
} CcProfile;

typedef enum {
    CheapClickerCustomEventBleConnected = 100,
    CheapClickerCustomEventBleDisconnected,
    CheapClickerCustomEventScriptDone,
    CheapClickerCustomEventScriptError,
    CheapClickerCustomEventScriptUpdate,
    CheapClickerCustomEventCalibrateDone,
    CheapClickerCustomEventProfileSelected,
    CheapClickerCustomEventScriptSelected,
} CheapClickerCustomEvent;

typedef enum {
    CheapClickerViewSubmenu,
    CheapClickerViewTextInput,
    CheapClickerViewVariableList,
    CheapClickerViewCalibrate,
    CheapClickerViewRun,
    CheapClickerViewCount,
} CheapClickerView;

// Forward declarations
typedef struct CcScript CcScript;
typedef struct CcCalibrateView CcCalibrateView;
typedef struct CcRunView CcRunView;

typedef struct {
    Gui* gui;
    Bt* bt;
    NotificationApp* notifications;
    FuriHalBleProfileBase* ble_hid_profile;
    ViewDispatcher* view_dispatcher;
    SceneManager* scene_manager;

    Submenu* submenu;
    TextInput* text_input;
    VariableItemList* var_item_list;
    CcCalibrateView* calibrate_view;
    CcRunView* run_view;

    CcProfile profiles[CC_MAX_PROFILES];
    uint8_t profile_count;
    uint8_t active_profile_idx;

    char text_input_buf[CC_MAX_NAME_LEN];
    uint8_t edit_button_idx;

    FuriString* script_path;

    CcScript* script;
} CheapClickerApp;
