// SquareLine-style UI events implementation
#include "ui_events.h"
#include "ui.h"
#include "screens/ui_Screen_Boot.h"
#include "screens/ui_Screen_Overview.h"
#include "screens/ui_Screen_Settings.h"
#include <Arduino.h>

// ═════════════════════════════════════════════════════════
// Static callback pointers
// ═════════════════════════════════════════════════════════
static void (*g_boot_complete_cb)(void*) = NULL;
static void* g_boot_complete_user_data = NULL;

// ═════════════════════════════════════════════════════════
// Boot screen event handler
// ═════════════════════════════════════════════════════════
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

// ═════════════════════════════════════════════════════════
// Overview screen event handler
// ═════════════════════════════════════════════════════════
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

// ═════════════════════════════════════════════════════════
// Overview screen gesture event handler
// ═════════════════════════════════════════════════════════
void ui_event_Screen_Overview_gesture(lv_event_t *e) {
    lv_event_code_t event_code = lv_event_get_code(e);

    // 左滑 - 进入 Storage 页面（待实现）
    if(event_code == LV_EVENT_GESTURE && lv_indev_get_gesture_dir(lv_indev_get_act()) == LV_DIR_LEFT) {
        lv_indev_wait_release(lv_indev_get_act());
        log_i("Overview: swipe left detected (Storage page not implemented yet)");
    }
    // 上边缘下滑（LV_DIR_BOTTOM）- 进入 Settings 页面
    if(event_code == LV_EVENT_GESTURE && lv_indev_get_gesture_dir(lv_indev_get_act()) == LV_DIR_BOTTOM) {
        lv_indev_wait_release(lv_indev_get_act());
        log_i("Overview: swipe down from top, switching to Settings");
        if (ui_Screen_Settings != NULL) {
            lv_scr_load_anim(ui_Screen_Settings, LV_SCR_LOAD_ANIM_MOVE_BOTTOM, 300, 0, false);
        }
    }
}

// ═════════════════════════════════════════════════════════
// Settings screen gesture event handler
// ═════════════════════════════════════════════════════════
void ui_event_Screen_Settings_gesture(lv_event_t *e) {
    lv_event_code_t event_code = lv_event_get_code(e);

    // 下边缘上滑（LV_DIR_TOP）- 返回 Overview 页面
    if(event_code == LV_EVENT_GESTURE && lv_indev_get_gesture_dir(lv_indev_get_act()) == LV_DIR_TOP) {
        lv_indev_wait_release(lv_indev_get_act());
        log_i("Settings: swipe up from bottom, switching back to Overview");
        if (ui_Screen_Overview != NULL) {
            lv_scr_load_anim(ui_Screen_Overview, LV_SCR_LOAD_ANIM_MOVE_BOTTOM, 300, 0, false);
        }
    }
}

// ═════════════════════════════════════════════════════════
// Callback setter
// ═════════════════════════════════════════════════════════
void ui_Screen_Boot_set_complete_callback(void (*cb)(void*), void* user_data)
{
    g_boot_complete_cb = cb;
    g_boot_complete_user_data = user_data;
}

// ═════════════════════════════════════════════════════════
// Callback invoker (internal use)
// ═════════════════════════════════════════════════════════
void ui_Screen_Boot_invoke_complete_callback(void)
{
    if (g_boot_complete_cb) {
        g_boot_complete_cb(g_boot_complete_user_data);
    }
}

// ═════════════════════════════════════════════════════════
// Stub functions for unimplemented features (called by Settings screen)
// TODO: Implement these functions later
// ═════════════════════════════════════════════════════════
void resetScreenOffTimer(lv_event_t* e) { (void)e; }
void saveWiFiCredential(lv_event_t* e) { (void)e; }
void scanNetwork(lv_event_t* e) { (void)e; }
void toggleWiFi(lv_event_t* e) { (void)e; }
void setBrightness(lv_event_t* e) { (void)e; }
void setTimer(lv_event_t* e) { (void)e; }
void setWallpaper(lv_event_t* e) { (void)e; }
void loadStationFromSDCARD(lv_event_t* e) { (void)e; }
void loadMusicFromSDCARD(lv_event_t* e) { (void)e; }
void set_query_para_autoip(lv_event_t* e) { (void)e; }
void setOffsetHour(lv_event_t* e) { (void)e; }
void setOffsetMinute(lv_event_t* e) { (void)e; }
void setTempUnit(lv_event_t* e) { (void)e; }
void saveConfig(lv_event_t* e) { (void)e; }
void readKeyboard(lv_event_t* e) { (void)e; }
void readNumpad(lv_event_t* e) { (void)e; }
void turnonScreen(lv_event_t* e) { (void)e; }
void stopWeatherAnimation(lv_event_t* e) { (void)e; }
void informationMode(lv_event_t* e) { (void)e; }
void utilityMode(lv_event_t* e) { (void)e; }
void changeInfoPanelNext(lv_event_t* e) { (void)e; }
void changeInfoPanelPrev(lv_event_t* e) { (void)e; }
void showArrow(lv_event_t* e) { (void)e; }
void updateAQIwidget(lv_event_t* e) { (void)e; }
void torch_ON(lv_event_t* e) { (void)e; }
void showSystemInfo(lv_event_t* e) { (void)e; }
void torch_OFF(lv_event_t* e) { (void)e; }
void ota_update(lv_event_t* e) { (void)e; }
void appStart(lv_event_t* e) { (void)e; }
