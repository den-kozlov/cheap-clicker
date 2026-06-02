#include "cc_profile.h"
#include <flipper_format/flipper_format.h>
#include <storage/storage.h>
#include <furi.h>

#define CC_DATA_DIR        APP_DATA_PATH("")
#define CC_PROFILES_DIR    APP_DATA_PATH("profiles")
#define CC_SCRIPTS_DIR     APP_DATA_PATH("scripts")
#define CC_CONFIG_PATH     APP_DATA_PATH("config.fds")

#define CC_PROFILE_FILE_TYPE    "CheapClicker Profile"
#define CC_PROFILE_VERSION      1

#define CC_BUTTONS_FILE_TYPE    "CheapClicker Buttons"
#define CC_BUTTONS_VERSION      1

#define CC_CONFIG_FILE_TYPE     "CheapClicker Config"
#define CC_CONFIG_VERSION       1

// Build the path to a per-profile subdirectory: profiles/p{idx}
static void cc_profile_slot_dir(char* buf, size_t buf_size, uint8_t idx) {
    snprintf(buf, buf_size, APP_DATA_PATH("profiles/p%u"), (unsigned)idx);
}

// Build the path to profiles/p{idx}/profile.fds
static void cc_profile_fds_path(char* buf, size_t buf_size, uint8_t idx) {
    snprintf(buf, buf_size, APP_DATA_PATH("profiles/p%u/profile.fds"), (unsigned)idx);
}

// Build the path to profiles/p{idx}/buttons.fds
static void cc_buttons_fds_path(char* buf, size_t buf_size, uint8_t idx) {
    snprintf(buf, buf_size, APP_DATA_PATH("profiles/p%u/buttons.fds"), (unsigned)idx);
}

// Build the path to profiles/p{idx}/bt.keys
static void cc_bt_keys_path(char* buf, size_t buf_size, uint8_t idx) {
    snprintf(buf, buf_size, APP_DATA_PATH("profiles/p%u/bt.keys"), (unsigned)idx);
}

static void cc_profile_remove_slot(Storage* storage, uint8_t idx) {
    char slot_dir[128];
    cc_profile_slot_dir(slot_dir, sizeof(slot_dir), idx);
    storage_simply_remove_recursive(storage, slot_dir);
}

void cc_profile_save(CheapClickerApp* app, uint8_t idx) {
    furi_assert(app);
    furi_assert(idx < CC_MAX_PROFILES);

    CcProfile* p = &app->profiles[idx];

    // Set keys_path for this slot
    cc_bt_keys_path(p->keys_path, sizeof(p->keys_path), idx);

    Storage* storage = furi_record_open(RECORD_STORAGE);

    // Ensure slot directory exists
    char slot_dir[128];
    cc_profile_slot_dir(slot_dir, sizeof(slot_dir), idx);
    storage_simply_mkdir(storage, slot_dir);

    FlipperFormat* fff = flipper_format_file_alloc(storage);

    // Write profile.fds
    char profile_path[144];
    cc_profile_fds_path(profile_path, sizeof(profile_path), idx);

    if(flipper_format_file_open_always(fff, profile_path)) {
        do {
            if(!flipper_format_write_header_cstr(fff, CC_PROFILE_FILE_TYPE, CC_PROFILE_VERSION))
                break;
            if(!flipper_format_write_string_cstr(fff, "Name", p->name)) break;
            if(!flipper_format_write_string_cstr(fff, "BleName", p->ble_name)) break;
            if(!flipper_format_write_string_cstr(fff, "KeysPath", p->keys_path)) break;
            uint32_t tx = (uint32_t)p->trigger_x;
            uint32_t ty = (uint32_t)p->trigger_y;
            if(!flipper_format_write_uint32(fff, "TriggerX", &tx, 1)) break;
            if(!flipper_format_write_uint32(fff, "TriggerY", &ty, 1)) break;
            uint32_t btn_count = p->button_count;
            if(!flipper_format_write_uint32(fff, "ButtonCount", &btn_count, 1)) break;
        } while(0);
        flipper_format_file_close(fff);
    }

    // Write buttons.fds
    char buttons_path[144];
    cc_buttons_fds_path(buttons_path, sizeof(buttons_path), idx);

    if(flipper_format_file_open_always(fff, buttons_path)) {
        do {
            if(!flipper_format_write_header_cstr(fff, CC_BUTTONS_FILE_TYPE, CC_BUTTONS_VERSION))
                break;
            for(uint8_t i = 0; i < p->button_count; i++) {
                char key[48];
                snprintf(key, sizeof(key), "Button%uName", (unsigned)i);
                if(!flipper_format_write_string_cstr(fff, key, p->buttons[i].name)) break;
                snprintf(key, sizeof(key), "Button%uX", (unsigned)i);
                uint32_t bx = (uint32_t)p->buttons[i].x;
                if(!flipper_format_write_uint32(fff, key, &bx, 1)) break;
                snprintf(key, sizeof(key), "Button%uY", (unsigned)i);
                uint32_t by = (uint32_t)p->buttons[i].y;
                if(!flipper_format_write_uint32(fff, key, &by, 1)) break;
            }
        } while(0);
        flipper_format_file_close(fff);
    }

    flipper_format_free(fff);
    furi_record_close(RECORD_STORAGE);
}

static bool cc_profile_load_slot(CheapClickerApp* app, uint8_t idx) {
    furi_assert(app);
    furi_assert(idx < CC_MAX_PROFILES);

    char profile_path[144];
    cc_profile_fds_path(profile_path, sizeof(profile_path), idx);

    Storage* storage = furi_record_open(RECORD_STORAGE);

    // Check if profile.fds exists
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

        if(flipper_format_read_string(fff, "Name", temp)) {
            strlcpy(p->name, furi_string_get_cstr(temp), sizeof(p->name));
        }

        if(flipper_format_read_string(fff, "BleName", temp)) {
            strlcpy(p->ble_name, furi_string_get_cstr(temp), sizeof(p->ble_name));
        }

        if(flipper_format_read_string(fff, "KeysPath", temp)) {
            strlcpy(p->keys_path, furi_string_get_cstr(temp), sizeof(p->keys_path));
        } else {
            // Fall back to derived path if not stored
            cc_bt_keys_path(p->keys_path, sizeof(p->keys_path), idx);
        }

        uint32_t tx = 0, ty = 0;
        flipper_format_read_uint32(fff, "TriggerX", &tx, 1);
        flipper_format_read_uint32(fff, "TriggerY", &ty, 1);
        p->trigger_x = (int16_t)tx;
        p->trigger_y = (int16_t)ty;

        uint32_t btn_count = 0;
        flipper_format_read_uint32(fff, "ButtonCount", &btn_count, 1);
        if(btn_count > CC_MAX_BUTTONS) btn_count = CC_MAX_BUTTONS;
        p->button_count = (uint8_t)btn_count;

        ok = true;
    } while(0);

    flipper_format_file_close(fff);

    // Load buttons.fds if profile loaded ok
    if(ok && p->button_count > 0) {
        char buttons_path[144];
        cc_buttons_fds_path(buttons_path, sizeof(buttons_path), idx);

        FlipperFormat* fff_buttons = flipper_format_file_alloc(storage);

        if(flipper_format_file_open_existing(fff_buttons, buttons_path)) {
            uint32_t version = 0;
            flipper_format_read_header(fff_buttons, temp, &version);

            for(uint8_t i = 0; i < p->button_count && i < CC_MAX_BUTTONS; i++) {
                char key[48];
                snprintf(key, sizeof(key), "Button%uName", (unsigned)i);
                if(flipper_format_read_string(fff_buttons, key, temp)) {
                    strlcpy(p->buttons[i].name, furi_string_get_cstr(temp),
                            sizeof(p->buttons[i].name));
                }
                snprintf(key, sizeof(key), "Button%uX", (unsigned)i);
                uint32_t bx = 0;
                flipper_format_read_uint32(fff_buttons, key, &bx, 1);
                p->buttons[i].x = (int16_t)bx;

                snprintf(key, sizeof(key), "Button%uY", (unsigned)i);
                uint32_t by = 0;
                flipper_format_read_uint32(fff_buttons, key, &by, 1);
                p->buttons[i].y = (int16_t)by;
            }
        }
        // Always close before free, even if open failed (SDK no-op on closed handle)
        flipper_format_file_close(fff_buttons);
        flipper_format_free(fff_buttons);
    }

    // Only keep profiles that completed BLE pairing (bt.keys on disk)
    if(ok && !storage_file_exists(storage, p->keys_path)) {
        cc_profile_remove_slot(storage, idx);
        memset(p, 0, sizeof(CcProfile));
        ok = false;
    }

    flipper_format_free(fff);
    furi_string_free(temp);
    furi_record_close(RECORD_STORAGE);

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
            // Compact: move into next contiguous position if needed
            if(count != i) {
                app->profiles[count] = app->profiles[i];
            }
            count++;
        }
    }
    app->profile_count = count;

    // Clamp active index to valid range (handles zero profiles too)
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

    if(app->profile_count >= CC_MAX_PROFILES) {
        return CC_PROFILE_IDX_NONE;
    }

    uint8_t idx = app->profile_count;
    CcProfile* p = &app->profiles[idx];

    memset(p, 0, sizeof(CcProfile));
    strlcpy(p->name, name, sizeof(p->name));
    strlcpy(p->ble_name, ble_name, sizeof(p->ble_name));
    cc_bt_keys_path(p->keys_path, sizeof(p->keys_path), idx);
    p->trigger_x = 0;
    p->trigger_y = 0;
    p->button_count = 0;

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

    // Shift profiles down in memory
    for(uint8_t i = idx; i + 1 < app->profile_count; i++) {
        app->profiles[i] = app->profiles[i + 1];
    }
    memset(&app->profiles[app->profile_count - 1], 0, sizeof(CcProfile));
    app->profile_count--;

    // Adjust active_profile_idx
    if(app->profile_count == 0) {
        app->active_profile_idx = CC_PROFILE_IDX_NONE;
    } else if(app->active_profile_idx >= app->profile_count) {
        app->active_profile_idx = app->profile_count - 1;
    } else if(app->active_profile_idx == idx) {
        // Active profile was deleted; reset to first profile
        app->active_profile_idx = 0;
    } else if(app->active_profile_idx > idx) {
        app->active_profile_idx--;
    }

    // Re-save all remaining profiles at their new indices (slot directories)
    for(uint8_t i = 0; i < app->profile_count; i++) {
        // Update keys_path to reflect new slot index
        cc_bt_keys_path(app->profiles[i].keys_path, sizeof(app->profiles[i].keys_path), i);
        cc_profile_save(app, i);
    }

    cc_profile_save_active(app);
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
    } while(0);

    flipper_format_file_close(fff);
    flipper_format_free(fff);
    furi_string_free(temp);
    furi_record_close(RECORD_STORAGE);
}
