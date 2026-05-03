// SquareLine Studio UI helpers implementation
#include "ui_helpers.h"
#include <stdarg.h>
#include <stdio.h>

// ═══════════════════════════════════════════════════════════
// Helper: Initialize UI helpers
// ═══════════════════════════════════════════════════════════
void ui_helpers_init(void)
{
    // Initialize UI helper system
}

// ═══════════════════════════════════════════════════════════
// Helper: Convert hex color to lv_color_t
// ═══════════════════════════════════════════════════════════
lv_color_t ui_color_hex(uint32_t hex)
{
    return lv_color_hex(hex);
}

// ═══════════════════════════════════════════════════════════
// Helper: Create screen
// ═══════════════════════════════════════════════════════════
lv_obj_t* ui_create_screen(lv_obj_t* parent)
{
    lv_obj_t* screen = lv_obj_create(parent);
    return screen;
}

// ═══════════════════════════════════════════════════════════
// Helper: Set label text with format
// ═══════════════════════════════════════════════════════════
void ui_label_set_text_fmt(lv_obj_t* label, const char* fmt, ...)
{
    char buf[256];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    lv_label_set_text(label, buf);
}

// ═══════════════════════════════════════════════════════════
// Helper: Animate object property
// ═══════════════════════════════════════════════════════════
void ui_animate_obj(lv_obj_t* obj, int32_t start, int32_t end, uint32_t time, lv_anim_exec_xcb_t exec_cb)
{
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, obj);
    lv_anim_set_values(&a, start, end);
    lv_anim_set_time(&a, time);
    lv_anim_set_exec_cb(&a, exec_cb);
    lv_anim_start(&a);
}

// ═══════════════════════════════════════════════════════════
// Helper: Get event code
// ═══════════════════════════════════════════════════════════
lv_event_code_t ui_get_event_code(lv_event_t* e)
{
    return lv_event_get_code(e);
}

// ═══════════════════════════════════════════════════════════
// Helper: Get target object
// ═══════════════════════════════════════════════════════════
lv_obj_t* ui_get_target_obj(lv_event_t* e)
{
    return lv_event_get_target(e);
}

// ═══════════════════════════════════════════════════════════
// Helper: Get current target object
// ═══════════════════════════════════════════════════════════
lv_obj_t* ui_get_current_target_obj(lv_event_t* e)
{
    return lv_event_get_current_target(e);
}

// ═══════════════════════════════════════════════════════════
// Helper: Get event user data
// ═══════════════════════════════════════════════════════════
void* ui_get_event_user_data(lv_event_t* e)
{
    return lv_event_get_user_data(e);
}
