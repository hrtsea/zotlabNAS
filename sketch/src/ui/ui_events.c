// SquareLine-style UI events implementation
#include "ui_events.h"
#include "ui.h"
#include "screens/ui_Screen_Boot.h"
#include "screens/ui_Screen_Overview.h"
#include <Arduino.h>

// ═══════════════════════════════════════════════════════════
// Static callback pointers
// ═══════════════════════════════════════════════════════════
static ui_screen_complete_cb g_boot_complete_cb = NULL;
static void* g_boot_complete_user_data = NULL;

// ═══════════════════════════════════════════════════════════
// Boot screen event handler
// ═══════════════════════════════════════════════════════════
void ui_Screen_Boot_event_handler(lv_event_t* e)
{
    lv_event_code_t event = lv_event_get_code(e);
    lv_obj_t* target = lv_event_get_target(e);
    lv_obj_t* current_target = lv_event_get_current_target(e);

    if (event == LV_EVENT_SCREEN_LOADED) {
        log_i("Boot screen loaded");
    }
    else if (event == LV_EVENT_SCREEN_UNLOADED) {
        log_i("Boot screen unloaded");
    }
    else if (event == LV_EVENT_CLICKED) {
        log_i("Boot screen clicked");
    }
}

// ═══════════════════════════════════════════════════════════
// Overview screen event handler
// ═══════════════════════════════════════════════════════════
void ui_Screen_Overview_event_handler(lv_event_t* e)
{
    lv_event_code_t event = lv_event_get_code(e);
    lv_obj_t* target = lv_event_get_target(e);
    lv_obj_t* current_target = lv_event_get_current_target(e);

    if (event == LV_EVENT_SCREEN_LOADED) {
        log_i("Overview screen loaded");
    }
    else if (event == LV_EVENT_SCREEN_UNLOADED) {
        log_i("Overview screen unloaded");
    }
    else if (event == LV_EVENT_CLICKED) {
        log_i("Overview screen clicked");
    }
}

// ══════════════════════════════════════════════════════════
// Overview screen gesture event handler
// ══════════════════════════════════════════════════════════
void ui_event_Screen_Overview_gesture(lv_event_t *e) {
    lv_event_code_t event_code = lv_event_get_code(e);
    
    // 左滑 - 进入 Storage 页面（待实现）
    if(event_code == LV_EVENT_GESTURE && lv_indev_get_gesture_dir(lv_indev_get_act()) == LV_DIR_LEFT) {
        lv_indev_wait_release(lv_indev_get_act());
        log_i("Overview: swipe left detected (Storage page not implemented yet)");
    }
    // 上边缘下滑 - 进入 Settings 页面（待实现）
    if(event_code == LV_EVENT_GESTURE && lv_indev_get_gesture_dir(lv_indev_get_act()) == LV_DIR_BOTTOM) {
        lv_indev_wait_release(lv_indev_get_act());
        log_i("Overview: swipe up detected (Settings page not implemented yet)");
    }
}

// ═══════════════════════════════════════════════════════════
// Callback setter
// ═══════════════════════════════════════════════════════════
void ui_Screen_Boot_set_complete_callback(void (*cb)(void*), void* user_data)
{
    g_boot_complete_cb = cb;
    g_boot_complete_user_data = user_data;
}

// ═══════════════════════════════════════════════════════════
// Callback invoker (internal use)
// ═══════════════════════════════════════════════════════════
void ui_Screen_Boot_invoke_complete_callback(void)
{
    if (g_boot_complete_cb) {
        g_boot_complete_cb(g_boot_complete_user_data);
    }
}
