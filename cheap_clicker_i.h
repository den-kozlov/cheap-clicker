#pragma once

#include <furi.h>
#include <furi_hal_bt.h>
#include <furi_hal_usb_hid.h>  // HID_MOUSE_BTN_LEFT/RIGHT constants shared with BLE HID
#include <bt/bt_service/bt.h>  // Bt record, bt_disconnect, bt_keys_storage_set_storage_path
#include <gui/gui.h>
#include <gui/view_dispatcher.h>
#include <gui/scene_manager.h>
#include <gui/modules/submenu.h>
#include <gui/modules/text_input.h>
#include <gui/modules/variable_item_list.h>
#include <gui/modules/popup.h>
#include <notification/notification.h>
#include <storage/storage.h>
#include <ble_profile/extra_profiles/hid_profile.h>

#include "scenes/cc_scene.h"
#include "views/cc_start_view.h"

#define CC_MAX_BUTTONS  17
#define CC_MAX_PROFILES  5
#define CC_MAX_NAME_LEN 32
#define CC_MAX_BLE_NAME_LEN 29  // BT advertising PDU limit (29 bytes usable)
#define CC_BUTTON_IDX_NONE 0xFF
#define CC_PROFILE_IDX_NONE 0xFF

typedef enum {
    CcButtonTypePress = 0,
    CcButtonTypeDragAndRelease = 1,
} CcButtonType;

typedef struct {
    char name[CC_MAX_NAME_LEN];
    CcButtonType type;
} CcButtonDef;

typedef struct {
    int16_t x;
    int16_t y;
} CcButtonCalib;

typedef struct {
    char name[CC_MAX_NAME_LEN];
    char ble_name[CC_MAX_BLE_NAME_LEN];
    char keys_path[128];  // storage path to per-profile bt.keys file; 128 bytes covers longest SD path
    int16_t trigger_x;
    int16_t trigger_y;
    CcButtonCalib calib[CC_MAX_BUTTONS];  // indexed by global button index
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
    CheapClickerCustomEventPopupDismissed,
} CheapClickerCustomEvent;

typedef enum {
    CheapClickerViewSubmenu,
    CheapClickerViewTextInput,
    CheapClickerViewVariableList,
    CheapClickerViewPopup,
    CheapClickerViewCalibrate,
    CheapClickerViewRun,
    CheapClickerViewStart,
    CheapClickerViewManual,
    CheapClickerViewCount,
} CheapClickerView;

// Forward declarations
typedef struct CcScript CcScript;
typedef struct CcCalibrateView CcCalibrateView;
typedef struct CcRunView CcRunView;
typedef struct CcManualView CcManualView;

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
    Popup* popup;
    CcCalibrateView* calibrate_view;
    CcRunView* run_view;
    CcStartView* start_view;
    CcManualView* manual_view;

    CcButtonDef buttons[CC_MAX_BUTTONS];  // global button definitions
    uint8_t button_count;

    CcProfile profiles[CC_MAX_PROFILES];
    uint8_t profile_count;
    uint8_t active_profile_idx;

    char text_input_buf[CC_MAX_NAME_LEN];
    uint8_t edit_button_idx;
    uint8_t button_edit_mode; // 0=profile name, 1=ble name, 0xFF=button name
    bool new_profile_pending; // true while a newly added profile has not yet been paired
    bool is_new_button;       // true when creating a button not yet committed to global list
    CcButtonDef edit_button_staging; // working copy while editing; committed on Save

    uint8_t manual_layout[5];    // InputKey 0-4 → button index, CC_BUTTON_IDX_NONE = unset
    uint8_t manual_pending_key;  // key being reassigned, CC_BUTTON_IDX_NONE when idle

    FuriString* script_path;

    CcScript* script;
} CheapClickerApp;
