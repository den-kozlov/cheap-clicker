#include "cc_manual.h"
#include <flipper_format/flipper_format.h>
#include <storage/storage.h>
#include <furi.h>
#include <string.h>

#define CC_MANUAL_LAYOUT_PATH APP_DATA_PATH("manual_layout.fds")
#define CC_MANUAL_FILE_TYPE   "CheapClicker Manual"
#define CC_MANUAL_VERSION     1

void cc_manual_layout_load(CheapClickerApp* app) {
    furi_assert(app);
    memset(app->manual_layout, CC_BUTTON_IDX_NONE, sizeof(app->manual_layout));

    Storage* storage = furi_record_open(RECORD_STORAGE);
    FlipperFormat* fff = flipper_format_file_alloc(storage);
    FuriString* temp = furi_string_alloc();

    do {
        if(!flipper_format_file_open_existing(fff, CC_MANUAL_LAYOUT_PATH)) break;
        uint32_t version = 0;
        if(!flipper_format_read_header(fff, temp, &version)) break;
        for(uint8_t i = 0; i < 5; i++) {
            char key[16];
            snprintf(key, sizeof(key), "Key%u", (unsigned)i);
            uint32_t val = CC_BUTTON_IDX_NONE;
            flipper_format_read_uint32(fff, key, &val, 1);
            app->manual_layout[i] = (uint8_t)val;
        }
    } while(0);

    flipper_format_file_close(fff);
    flipper_format_free(fff);
    furi_string_free(temp);
    furi_record_close(RECORD_STORAGE);
}

void cc_manual_layout_save(CheapClickerApp* app) {
    furi_assert(app);

    Storage* storage = furi_record_open(RECORD_STORAGE);
    FlipperFormat* fff = flipper_format_file_alloc(storage);

    if(flipper_format_file_open_always(fff, CC_MANUAL_LAYOUT_PATH)) {
        do {
            if(!flipper_format_write_header_cstr(fff, CC_MANUAL_FILE_TYPE, CC_MANUAL_VERSION))
                break;
            for(uint8_t i = 0; i < 5; i++) {
                char key[16];
                snprintf(key, sizeof(key), "Key%u", (unsigned)i);
                uint32_t val = (uint32_t)app->manual_layout[i];
                if(!flipper_format_write_uint32(fff, key, &val, 1)) break;
            }
        } while(0);
        flipper_format_file_close(fff);
    }

    flipper_format_free(fff);
    furi_record_close(RECORD_STORAGE);
}
