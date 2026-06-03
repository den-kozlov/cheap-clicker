#include "../cheap_clicker_i.h"
#include "../helpers/cc_profile.h"
#include "../helpers/cc_ble.h"

// Available step sizes and delays
static const uint8_t STEP_VALUES[]  = {5, 10, 15, 20, 30, 40};
static const uint8_t DELAY_VALUES[] = {1, 2, 3, 5, 8, 10, 15, 20};
#define STEP_COUNT  (sizeof(STEP_VALUES)  / sizeof(STEP_VALUES[0]))
#define DELAY_COUNT (sizeof(DELAY_VALUES) / sizeof(DELAY_VALUES[0]))

static const uint16_t SYNC_VALUES[] = {0, 1000, 2000, 3000, 4000, 5000, 6000, 7000, 8000, 9000, 10000};
#define SYNC_COUNT (sizeof(SYNC_VALUES) / sizeof(SYNC_VALUES[0]))

static uint8_t find_sync_idx(uint16_t val) {
    for(uint8_t i = 0; i < SYNC_COUNT; i++)
        if(SYNC_VALUES[i] == val) return i;
    return 0;
}

static void sync_change_cb(VariableItem* item) {
    CheapClickerApp* app = variable_item_get_context(item);
    uint8_t idx = variable_item_get_current_value_index(item);
    app->sync_dist = SYNC_VALUES[idx];

    static char buf[10];
    if(app->sync_dist == 0) {
        snprintf(buf, sizeof(buf), "Off");
    } else {
        snprintf(buf, sizeof(buf), "%upx", (unsigned)app->sync_dist);
    }
    variable_item_set_current_value_text(item, buf);
}

static uint8_t find_step_idx(uint8_t val) {
    for(uint8_t i = 0; i < STEP_COUNT; i++)
        if(STEP_VALUES[i] == val) return i;
    return 1; // default: 10
}

static uint8_t find_delay_idx(uint8_t val) {
    for(uint8_t i = 0; i < DELAY_COUNT; i++)
        if(DELAY_VALUES[i] == val) return i;
    return 0; // default: 1ms
}

static void step_change_cb(VariableItem* item) {
    CheapClickerApp* app = variable_item_get_context(item);
    uint8_t idx = variable_item_get_current_value_index(item);
    app->move_step = STEP_VALUES[idx];

    static char buf[8];
    snprintf(buf, sizeof(buf), "%upx", (unsigned)app->move_step);
    variable_item_set_current_value_text(item, buf);
}

static void delay_change_cb(VariableItem* item) {
    CheapClickerApp* app = variable_item_get_context(item);
    uint8_t idx = variable_item_get_current_value_index(item);
    app->move_delay_ms = DELAY_VALUES[idx];

    static char buf[8];
    snprintf(buf, sizeof(buf), "%ums", (unsigned)app->move_delay_ms);
    variable_item_set_current_value_text(item, buf);
}

void cc_scene_move_speed_on_enter(void* context) {
    CheapClickerApp* app = context;
    VariableItemList* vil = app->var_item_list;
    variable_item_list_reset(vil);

    static char step_buf[8], delay_buf[8];

    VariableItem* item;

    item = variable_item_list_add(vil, "Move Step", STEP_COUNT, step_change_cb, app);
    uint8_t si = find_step_idx(app->move_step);
    variable_item_set_current_value_index(item, si);
    snprintf(step_buf, sizeof(step_buf), "%upx", (unsigned)STEP_VALUES[si]);
    variable_item_set_current_value_text(item, step_buf);

    item = variable_item_list_add(vil, "Move Delay", DELAY_COUNT, delay_change_cb, app);
    uint8_t di = find_delay_idx(app->move_delay_ms);
    variable_item_set_current_value_index(item, di);
    snprintf(delay_buf, sizeof(delay_buf), "%ums", (unsigned)DELAY_VALUES[di]);
    variable_item_set_current_value_text(item, delay_buf);

    static char sync_buf[10];

    item = variable_item_list_add(vil, "Cursor Sync", SYNC_COUNT, sync_change_cb, app);
    uint8_t sci = find_sync_idx(app->sync_dist);
    variable_item_set_current_value_index(item, sci);
    if(SYNC_VALUES[sci] == 0) {
        snprintf(sync_buf, sizeof(sync_buf), "Off");
    } else {
        snprintf(sync_buf, sizeof(sync_buf), "%upx", (unsigned)SYNC_VALUES[sci]);
    }
    variable_item_set_current_value_text(item, sync_buf);

    view_dispatcher_switch_to_view(app->view_dispatcher, CheapClickerViewVariableList);
}

bool cc_scene_move_speed_on_event(void* context, SceneManagerEvent event) {
    UNUSED(context);
    UNUSED(event);
    return false;
}

void cc_scene_move_speed_on_exit(void* context) {
    CheapClickerApp* app = context;
    variable_item_list_reset(app->var_item_list);
    cc_profile_save_active(app);
}
