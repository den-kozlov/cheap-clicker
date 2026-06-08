# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## What This Is

**Cheap Clicker** is a Flipper Zero external app (FAP) that controlls Cheapshot game and intended to automate some actions. It connects to a host (phone, tablet) via Bluetooth and replays stored screen-tap sequences (called "scripts") by moving the virtual cursor and tapping at calibrated coordinates.

## Build Commands

Requires [ufbt](https://github.com/flipperdevices/flipperzero-ufbt) (micro Flipper Build Tool).

```sh
ufbt              # build the .fap
ufbt launch       # build and deploy to connected Flipper via USB
ufbt -c           # clean
ufbt update       # update the Flipper SDK
ufbt vscode_dist  # regenerate .vscode/ config
ufbt cli          # open serial CLI session to the Flipper
```

There is no test suite.

## Code Architecture

### Entry Point

`cheap_clicker.c` — allocates the `CheapClickerApp` struct, loads profiles from SD, starts BLE if a profile exists, runs the scene manager loop, then cleans up on exit.

### App State (`cheap_clicker_i.h`)

`CheapClickerApp` is the single root struct passed everywhere. Key fields:
- `profiles[CC_MAX_PROFILES]` / `profile_count` / `active_profile_idx` — up to 5 `CcProfile` entries; `active_profile_idx == CC_PROFILE_IDX_NONE (0xFF)` means none selected.
- `ble_hid_profile` — non-null when BLE is running.
- `script` — the always-allocated `CcScript` worker.
- `new_profile_pending` — true while a just-created profile has not yet completed BLE pairing; if the user exits before pairing, the profile is discarded.

### Scenes (`scenes/`)

Registered via the `ADD_SCENE` macro in `cc_scene_config.h`. Scene list:

| Scene | Purpose |
|---|---|
| Start | Main screen; shows Devices (profiles) or prompts to add one |
| Profiles | List/add/delete profiles |
| ProfileEdit | Rename profile or BLE device name |
| Pairing | Wait for BLE connection to complete new profile |
| Calibrate | Move the virtual cursor to calibrate trigger and button positions |
| Buttons | List/add/delete buttons for a profile |
| ButtonEdit | Rename a button |
| Scripts | Pick a script file from SD |
| Run | Execute a script; shows live status (line, command, pause/stop) |

### Views (`views/`)

Custom views beyond the SDK stock modules:
- `CcStartView` — main screen, shows profile list or "No devices" prompt.
- `CcCalibrateView` — D-pad controlled cursor movement with OK to confirm position.
- `CcRunView` — live script execution display.

### Helpers (`helpers/`)

**`cc_ble.c`** — wraps `bt_profile_start` / BLE HID mouse. Maintains module-level `s_cur_x/s_cur_y` position state. `cc_ble_press_button` resets cursor to origin, moves to trigger, left-clicks, waits, moves to button, releases. `mac_xor = active_profile_idx` gives each profile a unique BLE MAC address.

**`cc_profile.c`** — load/save profiles to SD. Storage layout:
```
APP_DATA_PATH/
  config.fds           ← active profile index
  profiles/
    p{idx}/
      profile.fds      ← name, ble_name, trigger coords, button count
      buttons.fds      ← per-button name + x/y coords
      bt.keys          ← BLE pairing keys (absence = profile not paired, auto-removed)
  scripts/             ← user .txt script files
```
Profiles are slot-indexed 0–4. On `cc_profile_delete`, remaining profiles are compacted and re-saved at new slot indices with updated `keys_path`.

**`cc_script.c`** — runs a script file in a dedicated FuriThread. Controlled via thread flags (`CC_EVT_RUN/STOP/PAUSE/RESUME`). Status is mutex-protected and read from the UI thread. Script syntax:
```
PRESS <button_name>   # click a named button in the active profile
DELAY <ms>            # wait N milliseconds
LOOP [n]              # repeat n times (0 or omitted = infinite); max nesting depth 4
END                   # end of LOOP body
# comment            # ignored
```

### Event Flow

BLE status changes (`cc_ble_status_cb`) and script state changes send `CheapClickerCustomEvent` values through `view_dispatcher_send_custom_event`, which the scene manager routes to the active scene's `custom_event` handler.

## Coding Conventions

- 4-space indent, 99 column limit, enforced by `.clang-format` (Flipper SDK style).
- All functions are prefixed: `cc_ble_`, `cc_profile_`, `cc_script_`, `cc_*_view_`, `cc_scene_*`.
- `CC_PROFILE_IDX_NONE` and `CC_BUTTON_IDX_NONE` are `0xFF` — use these constants, not raw values.
- `furi_assert` for internal invariants; `furi_check` for allocations that must succeed.
