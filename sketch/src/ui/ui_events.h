// SquareLine-style UI events header
#pragma once

#include <lvgl.h>

#ifdef __cplusplus
extern "C" {
#endif

// ═══════════════════════════════════════════════════════════
// Event handler declarations
// ═══════════════════════════════════════════════════════════
void ui_Screen_Boot_event_handler(lv_event_t* e);
void ui_Screen_Overview_event_handler(lv_event_t* e);
void ui_event_Screen_Overview_gesture(lv_event_t * e);

// ═══════════════════════════════════════════════════════════
// Callback prototypes
// ═══════════════════════════════════════════════════════════
typedef void (*ui_screen_complete_cb)(void* user_data);

#ifdef __cplusplus
}
#endif
