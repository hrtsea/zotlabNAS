// SquareLine-style UI events implementation
#include "ui_events.h"
#include "ui.h"
#include "screens/ui_Screen_Boot.h"
#include "screens/ui_Screen_Overview.h"
#include "screens/ui_Screen_Settings.h"
#include "lcd_bl_bsp/lcd_bl_pwm_bsp.h"
#include <Arduino.h>

// ═════════════════════════════════════════════════════════
// Static callback pointers
// ═════════════════════════════════════════════════════════
static void (*g_boot_complete_cb)(void*) = NULL;
static void* g_boot_complete_user_data = NULL;

// ═════════════════════════════════════════════════════════
// Global variables
// ═════════════════════════════════════════════════════════
#include <Preferences.h>
Preferences pref;
uint8_t timeout_index = 0;  // 屏幕关闭延迟索引

// 屏幕关闭定时器回调
void screen_off_timer_cb(lv_timer_t* timer) {
    (void)timer;  // 未使用的参数
    
    // 如果 SCREEN_OFF_DELAY = 0，表示永不休眠
    if (SCREEN_OFF_DELAY == 0) return;
    
    // 检查是否超时
    if ((millis() - SCREEN_OFF_TIMER) > SCREEN_OFF_DELAY) {
        // 超时，关闭屏幕（设置 PWM 占空比为 255，即最低亮度）
        setUpduty(LCD_PWM_MODE_255);
        log_i("Screen off timer triggered, screen turned off");
    }
}

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
void resetScreenOffTimer(lv_event_t* e) {
    (void)e;  // 未使用的参数
    SCREEN_OFF_TIMER = millis();  // 重置定时器，防止屏幕关闭
}
void saveWiFiCredential(lv_event_t* e) { (void)e; }
void scanNetwork(lv_event_t* e) { (void)e; }
void toggleWiFi(lv_event_t* e) { (void)e; }
void setBrightness(lv_event_t* e) {
    lv_obj_t *widget = NULL;

    // 动态获取 dropdown 对象（支持从 Settings 或 MainMenu 调用）
    if (e != NULL) {
        widget = lv_event_get_target(e);  // 从事件获取触发对象
    }

    if (widget == NULL) {
        log_e("Brightness dropdown not found");
        return;
    }

    backlight_state = lv_dropdown_get_selected(widget);

    switch (backlight_state) {
        case 0: setUpduty(LCD_PWM_MODE_100); break;  // LOW
        case 1: setUpduty(LCD_PWM_MODE_150); break;  // MEDIUM
        case 2: setUpduty(LCD_PWM_MODE_255); break;  // HIGH
    }
    SCREEN_OFF_TIMER = millis();  // 重置背光定时器
}
void setTimer(lv_event_t* e) {
    lv_obj_t *widget = NULL;

    // 动态获取 dropdown 对象（支持从 Settings 或 MainMenu 调用）
    if (e != NULL) {
        widget = lv_event_get_target(e);  // 从事件获取触发对象
    }

    if (widget == NULL) {
        log_e("Sleep timer dropdown not found");
        return;
    }

    timeout_index = lv_dropdown_get_selected(widget);  // 更新全局变量

    // Dropdown 选项: "0\n15\n30\n60" (秒)
    // 标签显示: "Screen off timer (sec)"
    switch (timeout_index) {
        case 0: SCREEN_OFF_DELAY = 0; break;           // 永不休眠
        case 1: SCREEN_OFF_DELAY = 15 * 1000; break;   // 15秒
        case 2: SCREEN_OFF_DELAY = 30 * 1000; break;   // 30秒
        case 3: SCREEN_OFF_DELAY = 60 * 1000; break;   // 60秒
    }
    SCREEN_OFF_TIMER = millis();  // 重置定时器
}
void setWallpaper(lv_event_t* e) { (void)e; }
void loadStationFromSDCARD(lv_event_t* e) { (void)e; }
void loadMusicFromSDCARD(lv_event_t* e) { (void)e; }
void set_query_para_autoip(lv_event_t* e) { (void)e; }
void setOffsetHour(lv_event_t* e) { (void)e; }
void setOffsetMinute(lv_event_t* e) { (void)e; }
void setTempUnit(lv_event_t* e) { (void)e; }
void saveConfig(lv_event_t* e) {
    (void)e;  // 未使用的参数
    
    pref.begin("config", false); // 读写模式
    pref.putUChar("bl_state", backlight_state);
    pref.putUChar("scroffdelay", timeout_index);
    pref.end();
    
    log_i("Config saved: bl_state=%d, scroffdelay=%d", backlight_state, timeout_index);
}
void readKeyboard(lv_event_t* e) { (void)e; }
void readNumpad(lv_event_t* e) { (void)e; }
void turnonScreen(lv_event_t* e) {
    (void)e;  // 未使用的参数
    
    // 重新打开屏幕（恢复亮度）
    setBrightness(NULL);
    SCREEN_OFF_TIMER = millis();  // 重置定时器
    log_i("Screen turned on by user touch");
}
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
void appStart(lv_event_t* e) {
    (void)e;  // 未使用的参数
    
    // 加载配置
    pref.begin("config", true);  // 只读模式
    backlight_state = pref.getUChar("bl_state", 2);  // 默认亮度状态 2
    timeout_index = pref.getUChar("scroffdelay", 0);  // 默认屏幕关闭延迟索引 0
    pref.end();
    
    // 应用亮度设置
    setBrightness(NULL);
    
    // 应用屏幕关闭延迟设置
    switch (timeout_index) {
        case 0: SCREEN_OFF_DELAY = 0; break;           // 永不休眠
        case 1: SCREEN_OFF_DELAY = 15 * 1000; break;   // 15秒
        case 2: SCREEN_OFF_DELAY = 30 * 1000; break;   // 30秒
        case 3: SCREEN_OFF_DELAY = 60 * 1000; break;   // 60秒
    }
    SCREEN_OFF_TIMER = millis();  // 重置定时器
    
    // 创建屏幕关闭定时器（每秒检查一次）
    lv_timer_create(screen_off_timer_cb, 1000, NULL);
    
    log_i("Config loaded: bl_state=%d, scroffdelay=%d", backlight_state, timeout_index);
}
