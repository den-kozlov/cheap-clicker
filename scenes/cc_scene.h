#pragma once

#include <gui/scene_manager.h>

#define ADD_SCENE(prefix, name, id) CheapClickerScene##id,
typedef enum {
#include "cc_scene_config.h"
    CheapClickerSceneNum,
} CheapClickerScene;
#undef ADD_SCENE

extern const SceneManagerHandlers cc_scene_handlers;

#define ADD_SCENE(prefix, name, id) void prefix##_scene_##name##_on_enter(void*);
#include "cc_scene_config.h"
#undef ADD_SCENE

#define ADD_SCENE(prefix, name, id) \
    bool prefix##_scene_##name##_on_event(void* context, SceneManagerEvent event);
#include "cc_scene_config.h"
#undef ADD_SCENE

#define ADD_SCENE(prefix, name, id) void prefix##_scene_##name##_on_exit(void* context);
#include "cc_scene_config.h"
#undef ADD_SCENE
