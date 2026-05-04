// SquareLine-style UI events implementation
#include "ui_events.h"
#include "ui.h"
#include "screens/ui_Screen_Boot.h"
#include "screens/ui_Screen_Overview.h"
#include "screens/ui_Screen_Settings.h"
#include "lcd_bl_bsp/lcd_bl_pwm_bsp.h"
#include "data/config.h"
#include "net/wifi_manager.h"
#include "nas_config.h"
#include <Arduino.h>
#include <WiFi.h>

// 外部变量声明
extern WiFiManager g_wifi;
extern AppConfig g_config;

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

// 时区配置
const int8_t TIMEZONE_HOUR_OFFSETS[] = {14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0, -1, -2, -3, -4, -5, -6, -7, -8, -9, -10, -11, -12};
const int8_t TIMEZONE_MINUTE_OFFSETS[] = {0, 30, 45};
uint8_t timezone_hour_index = 6;   // 默认 UTC+8 (index 6)
uint8_t timezone_minute_index = 0; // 默认 0 分钟

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
// Overview screen HDD button click event handler
// ═════════════════════════════════════════════════════════
void ui_event_Screen_Overview_hdd_clicked(lv_event_t *e) {
    if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
        if (ui_Screen_Storage != NULL) {
            lv_scr_load_anim(ui_Screen_Storage, LV_SCR_LOAD_ANIM_MOVE_LEFT, 300, 0, false);
            log_i("[Overview] HDD button clicked, switching to Storage");
        }
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
// Storage screen gesture event handler
// ═════════════════════════════════════════════════════════
void ui_event_Screen_Storage_gesture(lv_event_t *e) {
    lv_event_code_t event_code = lv_event_get_code(e);

    // 上滑（LV_DIR_TOP）- 返回 Overview 页面
    if(event_code == LV_EVENT_GESTURE && lv_indev_get_gesture_dir(lv_indev_get_act()) == LV_DIR_TOP) {
        lv_indev_wait_release(lv_indev_get_act());
        log_i("Storage: swipe up, switching back to Overview");
        if (ui_Screen_Overview != NULL) {
            lv_scr_load_anim(ui_Screen_Overview, LV_SCR_LOAD_ANIM_MOVE_BOTTOM, 300, 0, false);
        }
    }
    // 左滑 - 进入 Settings 页面（如果需要）
    else if(event_code == LV_EVENT_GESTURE && lv_indev_get_gesture_dir(lv_indev_get_act()) == LV_DIR_LEFT) {
        lv_indev_wait_release(lv_indev_get_act());
        log_i("Storage: swipe left, switching to Settings");
        if (ui_Screen_Settings != NULL) {
            lv_scr_load_anim(ui_Screen_Settings, LV_SCR_LOAD_ANIM_MOVE_LEFT, 300, 0, false);
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
void saveWiFiCredential(lv_event_t* e) {
    (void)e;  // 未使用参数
    
    // 直接从全局变量获取 UI 对象（不依赖事件目标）
    if (ui_MainMenu_Dropdown_NetworkList == NULL) {
        log_e("Network dropdown not initialized");
        return;
    }
    
    if (ui_MainMenu_Textarea_Password == NULL) {
        log_e("Password textarea not initialized");
        return;
    }
    
    // 获取密码
    const char *password = lv_textarea_get_text(ui_MainMenu_Textarea_Password);
    
    // 获取选中的 SSID
    char ssid[64];
    lv_dropdown_get_selected_str(ui_MainMenu_Dropdown_NetworkList, ssid, sizeof(ssid));
    
    // 去除 SSID 中的 RSSI 后缀（如 "dd-wrt (-31 dBm)" → "dd-wrt"）
    char clean_ssid[64];
    char* rssi_pos = strstr(ssid, " (");
    if (rssi_pos != NULL) {
        int len = rssi_pos - ssid;
        strncpy(clean_ssid, ssid, len);
        clean_ssid[len] = '\0';
    } else {
        strncpy(clean_ssid, ssid, sizeof(clean_ssid));
        clean_ssid[sizeof(clean_ssid)-1] = '\0';
    }
    
    if (strlen(clean_ssid) == 0 || strlen(password) == 0) {
        log_e("WiFi credential empty: SSID='%s', password_len=%d", clean_ssid, strlen(password));
        return;
    }
    
    // 保存到配置
    config_save_wifi(clean_ssid, password);
    
    // 检查当前状态，避免重复连接
    if (g_wifi.getState() == WIFI_CONNECTING) {
        log_w("WiFi is already connecting, skipping duplicate connect");
        SCREEN_OFF_TIMER = millis();
        return;
    }
    
    // 如果已连接，先断开
    if (g_wifi.getState() == WIFI_CONNECTED) {
        log_i("Disconnecting from current WiFi before connecting to new one");
        WiFi.disconnect();
        delay(100);  // 等待断开完成
    }
    
    // 触发 WiFi 连接
    g_wifi.connectNonBlocking(clean_ssid, password);
    
    log_i("WiFi credential saved: SSID=%s", clean_ssid);
    SCREEN_OFF_TIMER = millis();
}
void scanNetwork(lv_event_t* e) {
    if (e == NULL) return;
    
    log_i("Scanning WiFi networks...");
    
    // 扫描网络
    int count = g_wifi.scanNetworks();
    
    if (count <= 0) {
        log_w("No WiFi networks found");
        lv_dropdown_clear_options(ui_MainMenu_Dropdown_NetworkList);
        lv_dropdown_add_option(ui_MainMenu_Dropdown_NetworkList, "No networks found", 0);
        return;
    }
    
    // 更新 dropdown 选项
    lv_dropdown_clear_options(ui_MainMenu_Dropdown_NetworkList);
    for (int i = 0; i < count; i++) {
        String ssid = g_wifi.getSSID(i);
        int32_t rssi = g_wifi.getNetworkRSSI(i);
        
        // 格式化显示：SSID (RSSI)
        char option[128];
        snprintf(option, sizeof(option), "%s (%d dBm)", ssid.c_str(), rssi);
        lv_dropdown_add_option(ui_MainMenu_Dropdown_NetworkList, option, i);
    }
    
    log_i("Found %d networks", count);
    SCREEN_OFF_TIMER = millis();
}
void toggleWiFi(lv_event_t* e) {
    if (e == NULL) return;
    
    lv_obj_t *sw = lv_event_get_target(e);
    bool checked = lv_obj_has_state(sw, LV_STATE_CHECKED);
    
    if (checked) {
        // 开启 WiFi
        log_i("WiFi enabled");
        
        // 检查当前状态，避免重复连接
        if (g_wifi.getState() == WIFI_CONNECTING) {
            log_w("WiFi is already connecting, skipping duplicate connect");
            SCREEN_OFF_TIMER = millis();
            return;
        }
        
        if (strlen(g_config.ssid) > 0) {
            g_wifi.connectNonBlocking(g_config.ssid, g_config.wifipass);
        } else {
            log_w("No WiFi credential saved");
        }
    } else {
        // 关闭 WiFi
        log_i("WiFi disabled");
        WiFi.disconnect(true);  // 断开连接并关闭 WiFi
    }
    
    SCREEN_OFF_TIMER = millis();
}
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
void setOffsetHour(lv_event_t* e) {
    if (e == NULL) return;
    
    lv_obj_t *widget = lv_event_get_target(e);
    timezone_hour_index = lv_roller_get_selected(widget);
    
    // 更新 g_config.timezone（仅小时部分）
    g_config.timezone = TIMEZONE_HOUR_OFFSETS[timezone_hour_index];
    
    SCREEN_OFF_TIMER = millis();
    log_i("Timezone hour index: %d, offset: %d", timezone_hour_index, g_config.timezone);
}

void setOffsetMinute(lv_event_t* e) {
    if (e == NULL) return;
    
    lv_obj_t *widget = lv_event_get_target(e);
    timezone_minute_index = lv_roller_get_selected(widget);
    
    // 分钟偏移量可能影响实际时区（但 g_config.timezone 是 int8_t，只支持整小时）
    // 这里保存索引，实际使用时可以组合 hour + minute/60.0
    
    SCREEN_OFF_TIMER = millis();
    log_i("Timezone minute index: %d, offset: %d", timezone_minute_index, TIMEZONE_MINUTE_OFFSETS[timezone_minute_index]);
}
void setTempUnit(lv_event_t* e) { (void)e; }
void saveConfig(lv_event_t* e) {
    if (e == NULL) return;
    
    pref.begin("config", false); // 读写模式
    pref.putUChar("bl_state", backlight_state);
    pref.putUChar("scroffdelay", timeout_index);
    
    // 保存时区设置
    pref.putUChar("tz_hour_idx", timezone_hour_index);
    pref.putUChar("tz_min_idx", timezone_minute_index);
    
    // 更新 g_config.timezone
    g_config.timezone = TIMEZONE_HOUR_OFFSETS[timezone_hour_index];
    // 如果有分钟偏移，可以保存到单独字段或使用浮点数
    
    pref.end();
    
    // 如果 WiFi 已连接，同步时间
    if (g_wifi.isConnected()) {
        g_wifi.syncTime(g_config.timezone);
        log_i("Time synced with timezone: UTC+%d", g_config.timezone);
    }
    
    log_i("Config saved: bl_state=%d, scroffdelay=%d, timezone=%d", 
           backlight_state, timeout_index, g_config.timezone);
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
void showSystemInfo(lv_event_t* e) {
    if (e == NULL) return;
    
    SCREEN_OFF_TIMER = millis();
    
    // 获取系统信息
    static char infoText[256];
    snprintf(infoText, sizeof(infoText), 
        "ZotLab NAS Monitor\n"
        "Version: %s\n"
        "Chip: %s\n"
        "CPU Freq: %d MHz\n"
        "Flash: %d MB\n"
        "Free Heap: %d KB\n"
        "WiFi: %s\n"
        "IP: %s",
        APP_VERSION,
        ESP.getChipModel(),
        ESP.getCpuFreqMHz(),
        ESP.getFlashChipSize() / 1024 / 1024,
        ESP.getFreeHeap() / 1024,
        g_wifi.isConnected() ? "Connected" : "Disconnected",
        g_wifi.isConnected() ? g_wifi.getIP() : "N/A"
    );
    
    // 假设 UI 有一个显示系统信息的 label
    // lv_label_set_text(ui_Label_SystemInfo, infoText);
    
    log_i("%s", infoText);
}
void torch_OFF(lv_event_t* e) { (void)e; }
void ota_update(lv_event_t* e) {
    if (e == NULL) return;
    
    SCREEN_OFF_TIMER = millis();
    
    log_i("Starting OTA update...");
    
    // 简单的 OTA 更新提示（实际实现需要包含 ArduinoOTA 或 Web updater）
    // 这里只是示例，实际需要：
    // 1. 包含 <ArduinoOTA.h>
    // 2. 在 setup() 中调用 ArduinoOTA.begin()
    // 3. 在 loop() 中调用 ArduinoOTA.handle()
    
    lv_obj_t *label = lv_label_create(NULL);
    lv_label_set_text(label, "OTA Update\nNot implemented yet\nUse Arduino IDE or Web Updater");
    lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);
    
    log_w("OTA update not fully implemented");
}
void appStart(lv_event_t* e) {
    (void)e;  // 未使用的参数
    
    // 加载配置
    pref.begin("config", true);  // 只读模式
    backlight_state = pref.getUChar("bl_state", 2);  // 默认亮度状态 2
    timeout_index = pref.getUChar("scroffdelay", 0);  // 默认屏幕关闭延迟索引 0
    
    // 加载时区设置
    timezone_hour_index = pref.getUChar("tz_hour_idx", 6);   // 默认 UTC+8 (index 6)
    timezone_minute_index = pref.getUChar("tz_min_idx", 0);  // 默认 0 分钟
    
    // 设置滚轮选中状态
    if (ui_MainMenu_Roller_Hour != NULL) {
        lv_roller_set_selected(ui_MainMenu_Roller_Hour, timezone_hour_index, LV_ANIM_OFF);
    }
    if (ui_MainMenu_Roller_Minute != NULL) {
        lv_roller_set_selected(ui_MainMenu_Roller_Minute, timezone_minute_index, LV_ANIM_OFF);
    }
    
    pref.end();
    
    // 更新 g_config.timezone
    g_config.timezone = TIMEZONE_HOUR_OFFSETS[timezone_hour_index];
    
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
    lv_timer_create(screen_off_timer_cb, 2000, NULL);
    
    // 如果有保存的 WiFi 配置，自动连接
    if (strlen(g_config.ssid) > 0) {
        log_i("WiFi auto-connecting to %s", g_config.ssid);
        g_wifi.connectNonBlocking(g_config.ssid, g_config.wifipass);
    }
    
    // 如果 WiFi 已连接，同步时间
    if (g_wifi.isConnected()) {
        g_wifi.syncTime(g_config.timezone);
        log_i("Time synced with timezone: UTC+%d", g_config.timezone);
    }
    
    log_i("Config loaded: bl_state=%d, scroffdelay=%d, timezone=%d", 
           backlight_state, timeout_index, g_config.timezone);
}
