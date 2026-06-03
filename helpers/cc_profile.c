#include "cc_profile.h"
#include <flipper_format/flipper_format.h>
#include <storage/storage.h>
#include <furi.h>

#define CC_DATA_DIR        APP_DATA_PATH("")
#define CC_PROFILES_DIR    APP_DATA_PATH("profiles")
#define CC_SCRIPTS_DIR     APP_DATA_PATH("scripts")
#define CC_CONFIG_PATH     APP_DATA_PATH("config.fds")
#define CC_BUTTONS_PATH    APP_DATA_PATH("buttons.fds")
#define CC_MONUMENT_PATH   APP_DATA_PATH("monument.fds")

#define CC_MONUMENT_FILE_TYPE "CheapClicker Monument"
#define CC_MONUMENT_VERSION   1

#define CC_PROFILE_FILE_TYPE    "CheapClicker Profile"
#define CC_PROFILE_VERSION      1

#define CC_CALIB_FILE_TYPE      "CheapClicker Calib"
#define CC_CALIB_VERSION        1

#define CC_BUTTONS_FILE_TYPE    "CheapClicker Buttons"
#define CC_BUTTONS_VERSION      1

#define CC_CONFIG_FILE_TYPE     "CheapClicker Config"
#define CC_CONFIG_VERSION       1

static void cc_profile_slot_dir(char* buf, size_t buf_size, uint8_t idx) {
    snprintf(buf, buf_size, APP_DATA_PATH("profiles/p%u"), (unsigned)idx);
}

static void cc_profile_fds_path(char* buf, size_t buf_size, uint8_t idx) {
    snprintf(buf, buf_size, APP_DATA_PATH("profiles/p%u/profile.fds"), (unsigned)idx);
}

static void cc_calib_fds_path(char* buf, size_t buf_size, uint8_t idx) {
    snprintf(buf, buf_size, APP_DATA_PATH("profiles/p%u/calib.fds"), (unsigned)idx);
}

static void cc_bt_keys_path(char* buf, size_t buf_size, uint8_t idx) {
    snprintf(buf, buf_size, APP_DATA_PATH("profiles/p%u/bt.keys"), (unsigned)idx);
}

static void cc_profile_remove_slot(Storage* storage, uint8_t idx) {
    char slot_dir[128];
    cc_profile_slot_dir(slot_dir, sizeof(slot_dir), idx);
    storage_simply_remove_recursive(storage, slot_dir);
}

// ---------------------------------------------------------------------------
// Global buttons
// ---------------------------------------------------------------------------

void cc_buttons_save(CheapClickerApp* app) {
    furi_assert(app);

    Storage* storage = furi_record_open(RECORD_STORAGE);
    FlipperFormat* fff = flipper_format_file_alloc(storage);

    if(flipper_format_file_open_always(fff, CC_BUTTONS_PATH)) {
        do {
            if(!flipper_format_write_header_cstr(fff, CC_BUTTONS_FILE_TYPE, CC_BUTTONS_VERSION))
                break;
            uint32_t count = app->button_count;
            if(!flipper_format_write_uint32(fff, "ButtonCount", &count, 1)) break;
            for(uint8_t i = 0; i < app->button_count; i++) {
                char key[48];
                snprintf(key, sizeof(key), "Button%uName", (unsigned)i);
                if(!flipper_format_write_string_cstr(fff, key, app->buttons[i].name)) break;
                snprintf(key, sizeof(key), "Button%uType", (unsigned)i);
                uint32_t t = (uint32_t)app->buttons[i].type;
                if(!flipper_format_write_uint32(fff, key, &t, 1)) break;
            }
        } while(0);
        flipper_format_file_close(fff);
    }

    flipper_format_free(fff);
    furi_record_close(RECORD_STORAGE);
}

void cc_buttons_load(CheapClickerApp* app) {
    furi_assert(app);

    app->button_count = 0;

    Storage* storage = furi_record_open(RECORD_STORAGE);
    FlipperFormat* fff = flipper_format_file_alloc(storage);
    FuriString* temp = furi_string_alloc();

    do {
        if(!flipper_format_file_open_existing(fff, CC_BUTTONS_PATH)) break;

        uint32_t version = 0;
        if(!flipper_format_read_header(fff, temp, &version)) break;

        uint32_t count = 0;
        if(!flipper_format_read_uint32(fff, "ButtonCount", &count, 1)) break;
        if(count > CC_MAX_BUTTONS) count = CC_MAX_BUTTONS;

        for(uint8_t i = 0; i < (uint8_t)count; i++) {
            char key[48];
            snprintf(key, sizeof(key), "Button%uName", (unsigned)i);
            if(!flipper_format_read_string(fff, key, temp)) break;
            strlcpy(app->buttons[i].name, furi_string_get_cstr(temp), CC_MAX_NAME_LEN);

            snprintf(key, sizeof(key), "Button%uType", (unsigned)i);
            uint32_t t = (uint32_t)CcButtonTypeDragAndRelease;
            flipper_format_read_uint32(fff, key, &t, 1);
            app->buttons[i].type = (CcButtonType)t;

            app->button_count = i + 1;
        }
    } while(0);

    flipper_format_file_close(fff);
    flipper_format_free(fff);
    furi_string_free(temp);
    furi_record_close(RECORD_STORAGE);
}

// ---------------------------------------------------------------------------
// Per-profile calibration
// ---------------------------------------------------------------------------

static void cc_calib_save(CheapClickerApp* app, uint8_t idx) {
    furi_assert(app);
    furi_assert(idx < CC_MAX_PROFILES);

    CcProfile* p = &app->profiles[idx];

    Storage* storage = furi_record_open(RECORD_STORAGE);
    FlipperFormat* fff = flipper_format_file_alloc(storage);

    char calib_path[144];
    cc_calib_fds_path(calib_path, sizeof(calib_path), idx);

    if(flipper_format_file_open_always(fff, calib_path)) {
        do {
            if(!flipper_format_write_header_cstr(fff, CC_CALIB_FILE_TYPE, CC_CALIB_VERSION))
                break;
            uint32_t tx = (uint32_t)p->trigger_x;
            uint32_t ty = (uint32_t)p->trigger_y;
            if(!flipper_format_write_uint32(fff, "TriggerX", &tx, 1)) break;
            if(!flipper_format_write_uint32(fff, "TriggerY", &ty, 1)) break;
            for(uint8_t i = 0; i < app->button_count; i++) {
                char key[48];
                snprintf(key, sizeof(key), "Button%uX", (unsigned)i);
                uint32_t bx = (uint32_t)p->calib[i].x;
                if(!flipper_format_write_uint32(fff, key, &bx, 1)) break;
                snprintf(key, sizeof(key), "Button%uY", (unsigned)i);
                uint32_t by = (uint32_t)p->calib[i].y;
                if(!flipper_format_write_uint32(fff, key, &by, 1)) break;
            }
        } while(0);
        flipper_format_file_close(fff);
    }

    flipper_format_free(fff);
    furi_record_close(RECORD_STORAGE);
}

static void cc_calib_load(CheapClickerApp* app, uint8_t idx) {
    furi_assert(app);
    furi_assert(idx < CC_MAX_PROFILES);

    CcProfile* p = &app->profiles[idx];

    Storage* storage = furi_record_open(RECORD_STORAGE);
    FlipperFormat* fff = flipper_format_file_alloc(storage);
    FuriString* temp = furi_string_alloc();

    char calib_path[144];
    cc_calib_fds_path(calib_path, sizeof(calib_path), idx);

    if(flipper_format_file_open_existing(fff, calib_path)) {
        uint32_t version = 0;
        flipper_format_read_header(fff, temp, &version);

        uint32_t tx = 0, ty = 0;
        flipper_format_read_uint32(fff, "TriggerX", &tx, 1);
        flipper_format_read_uint32(fff, "TriggerY", &ty, 1);
        p->trigger_x = (int16_t)tx;
        p->trigger_y = (int16_t)ty;

        for(uint8_t i = 0; i < app->button_count; i++) {
            char key[48];
            snprintf(key, sizeof(key), "Button%uX", (unsigned)i);
            uint32_t bx = 0;
            flipper_format_read_uint32(fff, key, &bx, 1);
            p->calib[i].x = (int16_t)bx;

            snprintf(key, sizeof(key), "Button%uY", (unsigned)i);
            uint32_t by = 0;
            flipper_format_read_uint32(fff, key, &by, 1);
            p->calib[i].y = (int16_t)by;
        }
    }

    flipper_format_file_close(fff);
    flipper_format_free(fff);
    furi_string_free(temp);
    furi_record_close(RECORD_STORAGE);
}

// ---------------------------------------------------------------------------
// Profile save / load
// ---------------------------------------------------------------------------

void cc_profile_save(CheapClickerApp* app, uint8_t idx) {
    furi_assert(app);
    furi_assert(idx < CC_MAX_PROFILES);

    CcProfile* p = &app->profiles[idx];

    cc_bt_keys_path(p->keys_path, sizeof(p->keys_path), idx);

    Storage* storage = furi_record_open(RECORD_STORAGE);

    char slot_dir[128];
    cc_profile_slot_dir(slot_dir, sizeof(slot_dir), idx);
    storage_simply_mkdir(storage, slot_dir);

    FlipperFormat* fff = flipper_format_file_alloc(storage);

    char profile_path[144];
    cc_profile_fds_path(profile_path, sizeof(profile_path), idx);

    if(flipper_format_file_open_always(fff, profile_path)) {
        do {
            if(!flipper_format_write_header_cstr(fff, CC_PROFILE_FILE_TYPE, CC_PROFILE_VERSION))
                break;
            if(!flipper_format_write_string_cstr(fff, "Name", p->name)) break;
            if(!flipper_format_write_string_cstr(fff, "BleName", p->ble_name)) break;
            if(!flipper_format_write_string_cstr(fff, "KeysPath", p->keys_path)) break;
        } while(0);
        flipper_format_file_close(fff);
    }

    flipper_format_free(fff);
    furi_record_close(RECORD_STORAGE);

    cc_calib_save(app, idx);
}

static bool cc_profile_load_slot(CheapClickerApp* app, uint8_t idx) {
    furi_assert(app);
    furi_assert(idx < CC_MAX_PROFILES);

    char profile_path[144];
    cc_profile_fds_path(profile_path, sizeof(profile_path), idx);

    Storage* storage = furi_record_open(RECORD_STORAGE);

    if(!storage_file_exists(storage, profile_path)) {
        furi_record_close(RECORD_STORAGE);
        return false;
    }

    CcProfile* p = &app->profiles[idx];
    FlipperFormat* fff = flipper_format_file_alloc(storage);
    FuriString* temp = furi_string_alloc();
    bool ok = false;

    do {
        if(!flipper_format_file_open_existing(fff, profile_path)) break;

        uint32_t version = 0;
        if(!flipper_format_read_header(fff, temp, &version)) break;

        if(flipper_format_read_string(fff, "Name", temp))
            strlcpy(p->name, furi_string_get_cstr(temp), sizeof(p->name));

        if(flipper_format_read_string(fff, "BleName", temp))
            strlcpy(p->ble_name, furi_string_get_cstr(temp), sizeof(p->ble_name));

        if(flipper_format_read_string(fff, "KeysPath", temp))
            strlcpy(p->keys_path, furi_string_get_cstr(temp), sizeof(p->keys_path));
        else
            cc_bt_keys_path(p->keys_path, sizeof(p->keys_path), idx);

        ok = true;
    } while(0);

    flipper_format_file_close(fff);

    if(ok && !storage_file_exists(storage, p->keys_path)) {
        cc_profile_remove_slot(storage, idx);
        memset(p, 0, sizeof(CcProfile));
        ok = false;
    }

    flipper_format_free(fff);
    furi_string_free(temp);
    furi_record_close(RECORD_STORAGE);

    if(ok) cc_calib_load(app, idx);

    return ok;
}

void cc_profile_load_all(CheapClickerApp* app) {
    furi_assert(app);

    Storage* storage = furi_record_open(RECORD_STORAGE);
    storage_simply_mkdir(storage, CC_DATA_DIR);
    storage_simply_mkdir(storage, CC_PROFILES_DIR);
    storage_simply_mkdir(storage, CC_SCRIPTS_DIR);
    furi_record_close(RECORD_STORAGE);

    app->profile_count = 0;

    uint8_t count = 0;
    for(uint8_t i = 0; i < CC_MAX_PROFILES; i++) {
        if(cc_profile_load_slot(app, i)) {
            if(count != i) app->profiles[count] = app->profiles[i];
            count++;
        }
    }
    app->profile_count = count;

    if(app->profile_count == 0) {
        app->active_profile_idx = CC_PROFILE_IDX_NONE;
    } else if(app->active_profile_idx >= app->profile_count) {
        app->active_profile_idx = 0;
    }
}

uint8_t cc_profile_add(CheapClickerApp* app, const char* name, const char* ble_name, bool save) {
    furi_assert(app);
    furi_assert(name);
    furi_assert(ble_name);

    if(app->profile_count >= CC_MAX_PROFILES) return CC_PROFILE_IDX_NONE;

    uint8_t idx = app->profile_count;
    CcProfile* p = &app->profiles[idx];

    memset(p, 0, sizeof(CcProfile));
    strlcpy(p->name, name, sizeof(p->name));
    strlcpy(p->ble_name, ble_name, sizeof(p->ble_name));
    cc_bt_keys_path(p->keys_path, sizeof(p->keys_path), idx);

    app->profile_count++;

    if(save) cc_profile_save(app, idx);

    return idx;
}

void cc_profile_delete(CheapClickerApp* app, uint8_t idx) {
    furi_assert(app);

    if(idx >= app->profile_count) return;

    Storage* storage = furi_record_open(RECORD_STORAGE);
    cc_profile_remove_slot(storage, idx);
    furi_record_close(RECORD_STORAGE);

    for(uint8_t i = idx; i + 1 < app->profile_count; i++)
        app->profiles[i] = app->profiles[i + 1];
    memset(&app->profiles[app->profile_count - 1], 0, sizeof(CcProfile));
    app->profile_count--;

    if(app->profile_count == 0) {
        app->active_profile_idx = CC_PROFILE_IDX_NONE;
    } else if(app->active_profile_idx >= app->profile_count) {
        app->active_profile_idx = app->profile_count - 1;
    } else if(app->active_profile_idx == idx) {
        app->active_profile_idx = 0;
    } else if(app->active_profile_idx > idx) {
        app->active_profile_idx--;
    }

    for(uint8_t i = 0; i < app->profile_count; i++) {
        cc_bt_keys_path(app->profiles[i].keys_path, sizeof(app->profiles[i].keys_path), i);
        cc_profile_save(app, i);
    }

    cc_profile_save_active(app);
}

void cc_profile_save_calib(CheapClickerApp* app, uint8_t idx) {
    furi_assert(app);
    furi_assert(idx < CC_MAX_PROFILES);
    cc_calib_save(app, idx);
}

void cc_profile_save_active(CheapClickerApp* app) {
    furi_assert(app);

    Storage* storage = furi_record_open(RECORD_STORAGE);
    FlipperFormat* fff = flipper_format_file_alloc(storage);

    if(flipper_format_file_open_always(fff, CC_CONFIG_PATH)) {
        do {
            if(!flipper_format_write_header_cstr(fff, CC_CONFIG_FILE_TYPE, CC_CONFIG_VERSION))
                break;
            uint32_t active = app->active_profile_idx;
            if(!flipper_format_write_uint32(fff, "ActiveProfile", &active, 1)) break;
            uint32_t ms = app->move_step;
            if(!flipper_format_write_uint32(fff, "MoveStep", &ms, 1)) break;
            uint32_t md = app->move_delay_ms;
            if(!flipper_format_write_uint32(fff, "MoveDelayMs", &md, 1)) break;
            // Quadratic accel coefficients stored as int × 100000
            uint32_t ac0 = (uint32_t)((app->accel_c[0] + 10.0f) * 100000.0f);
            if(!flipper_format_write_uint32(fff, "AccelC0", &ac0, 1)) break;
            uint32_t ac1 = (uint32_t)((app->accel_c[1] + 10.0f) * 100000.0f);
            if(!flipper_format_write_uint32(fff, "AccelC1", &ac1, 1)) break;
            uint32_t ac2 = (uint32_t)((app->accel_c[2] + 10.0f) * 100000.0f);
            if(!flipper_format_write_uint32(fff, "AccelC2", &ac2, 1)) break;
            uint32_t sd = app->sync_dist;
            if(!flipper_format_write_uint32(fff, "SyncDist", &sd, 1)) break;
        } while(0);
        flipper_format_file_close(fff);
    }

    flipper_format_free(fff);
    furi_record_close(RECORD_STORAGE);
}

void cc_profile_load_active(CheapClickerApp* app) {
    furi_assert(app);

    Storage* storage = furi_record_open(RECORD_STORAGE);
    FlipperFormat* fff = flipper_format_file_alloc(storage);
    FuriString* temp = furi_string_alloc();

    do {
        if(!flipper_format_file_open_existing(fff, CC_CONFIG_PATH)) break;

        uint32_t version = 0;
        if(!flipper_format_read_header(fff, temp, &version)) break;

        uint32_t active = 0;
        if(!flipper_format_read_uint32(fff, "ActiveProfile", &active, 1)) break;
        app->active_profile_idx = (uint8_t)active;

        uint32_t ms = 10; // default = CC_ACCEL_CAL_STEP_1
        if(flipper_format_read_uint32(fff, "MoveStep", &ms, 1))
            app->move_step = (uint8_t)ms;

        uint32_t md = 1; // default = CC_ACCEL_CAL_DELAY_1
        if(flipper_format_read_uint32(fff, "MoveDelayMs", &md, 1))
            app->move_delay_ms = (uint8_t)md;

        // Quadratic accel coefficients; default: a(v)=1 (c0=1, c1=c2=0)
        uint32_t ac0 = (uint32_t)((1.0f + 10.0f) * 100000.0f);
        if(flipper_format_read_uint32(fff, "AccelC0", &ac0, 1))
            app->accel_c[0] = (float)ac0 / 100000.0f - 10.0f;

        uint32_t ac1 = (uint32_t)((0.0f + 10.0f) * 100000.0f);
        if(flipper_format_read_uint32(fff, "AccelC1", &ac1, 1))
            app->accel_c[1] = (float)ac1 / 100000.0f - 10.0f;

        uint32_t ac2 = (uint32_t)((0.0f + 10.0f) * 100000.0f);
        if(flipper_format_read_uint32(fff, "AccelC2", &ac2, 1))
            app->accel_c[2] = (float)ac2 / 100000.0f - 10.0f;

        uint32_t sd = 0;
        if(flipper_format_read_uint32(fff, "SyncDist", &sd, 1))
            app->sync_dist = sd;
    } while(0);

    flipper_format_file_close(fff);
    flipper_format_free(fff);
    furi_string_free(temp);
    furi_record_close(RECORD_STORAGE);
}

// ---------------------------------------------------------------------------
// Button delete: remove from global list, scrub calib in all profiles
// ---------------------------------------------------------------------------

void cc_button_delete(CheapClickerApp* app, uint8_t btn_idx) {
    furi_assert(app);
    if(btn_idx >= app->button_count) return;

    for(uint8_t i = btn_idx; i + 1 < app->button_count; i++)
        app->buttons[i] = app->buttons[i + 1];
    memset(&app->buttons[app->button_count - 1], 0, sizeof(CcButtonDef));
    app->button_count--;
    cc_buttons_save(app);

    for(uint8_t p = 0; p < app->profile_count; p++) {
        CcProfile* prof = &app->profiles[p];
        for(uint8_t i = btn_idx; i < app->button_count; i++)
            prof->calib[i] = prof->calib[i + 1];
        memset(&prof->calib[app->button_count], 0, sizeof(CcButtonCalib));
        cc_calib_save(app, p);
    }
}

// ---------------------------------------------------------------------------
// Monument Attack global settings (monument.fds)
// ---------------------------------------------------------------------------

void cc_monument_settings_save(CheapClickerApp* app) {
    furi_assert(app);

    Storage* storage = furi_record_open(RECORD_STORAGE);
    FlipperFormat* fff = flipper_format_file_alloc(storage);

    if(flipper_format_file_open_always(fff, CC_MONUMENT_PATH)) {
        do {
            if(!flipper_format_write_header_cstr(fff, CC_MONUMENT_FILE_TYPE, CC_MONUMENT_VERSION))
                break;
            uint32_t mauto = app->monument_auto_btn;
            if(!flipper_format_write_uint32(fff, "AutoBtn", &mauto, 1)) break;
            uint32_t mheal = app->monument_heal_btn;
            if(!flipper_format_write_uint32(fff, "HealBtn", &mheal, 1)) break;
        } while(0);
        flipper_format_file_close(fff);
    }

    flipper_format_free(fff);
    furi_record_close(RECORD_STORAGE);
}

void cc_monument_settings_load(CheapClickerApp* app) {
    furi_assert(app);

    Storage* storage = furi_record_open(RECORD_STORAGE);
    FlipperFormat* fff = flipper_format_file_alloc(storage);
    FuriString* temp = furi_string_alloc();

    do {
        if(!flipper_format_file_open_existing(fff, CC_MONUMENT_PATH)) break;

        uint32_t version = 0;
        if(!flipper_format_read_header(fff, temp, &version)) break;

        uint32_t mauto = CC_BUTTON_IDX_NONE;
        if(flipper_format_read_uint32(fff, "AutoBtn", &mauto, 1))
            app->monument_auto_btn = (uint8_t)mauto;

        uint32_t mheal = CC_BUTTON_IDX_NONE;
        if(flipper_format_read_uint32(fff, "HealBtn", &mheal, 1))
            app->monument_heal_btn = (uint8_t)mheal;
    } while(0);

    flipper_format_file_close(fff);
    flipper_format_free(fff);
    furi_string_free(temp);
    furi_record_close(RECORD_STORAGE);
}
