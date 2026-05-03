// SquareLine Studio UI helpers
#pragma once

#include <lvgl.h>

#ifdef __cplusplus
extern "C" {
#endif

// ═══════════════════════════════════════════════════════════
// UI helper functions
// ═══════════════════════════════════════════════════════════
void ui_helpers_init(void);

// Color helpers
lv_color_t ui_color_hex(uint32_t hex);

// Object creation helpers
lv_obj_t* ui_create_screen(lv_obj_t* parent);

// Text helpers
void ui_label_set_text_fmt(lv_obj_t* label, const char* fmt, ...);

// Animation helpers
void ui_animate_obj(lv_obj_t* obj, int32_t start, int32_t end, uint32_t time, lv_anim_exec_xcb_t exec_cb);

// Event helpers
lv_event_code_t ui_get_event_code(lv_event_t* e);
lv_obj_t* ui_get_target_obj(lv_event_t* e);
lv_obj_t* ui_get_current_target_obj(lv_event_t* e);
void* ui_get_event_user_data(lv_event_t* e);

#ifdef __cplusplus
}
#endif
