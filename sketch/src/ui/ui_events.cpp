// SquareLine-style UI events implementation
#include "ui_events.h"
#include "ui.h"
#include "screens/ui_Screen_Boot.h"
#include "screens/ui_Screen_Overview.h"
#include "screens/ui_Screen_Settings.h"
#include "screens/ui_Screen_Storage.h"
#include "lcd_bl_bsp/lcd_bl_pwm_bsp.h"
#include "data/config.h"
#include "net/network.h"
#include "net/data_source.h"
#include "data/nas_config.h"
#include "pcf85063/pcf85063.h"
#include <Arduino.h>
#include <WiFi.h>

// 外部变量声明
extern PCF85063 rtc;
extern AppConfig g_config;

// 数据轮询任务声明
void data_poll_task(void *param);

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

#define WIFI_CHECK_INTERVAL 5000 //wifi connection checking interval 5 second.

//wifi monitor call back from timer
void lvgl_wifi_check_cb(lv_timer_t *t) {
  if (wifiEnable) {//if enable
    if (WiFi.status() != WL_CONNECTED && wifiTaskHandle == NULL) {//wifi disconnected
        wifi_need_connect = true;
        lv_timer_pause(wifi_check_timer);//pause the timer
        wifiConnect();  // create task ONCE
    }
  }  
}

// WiFi status update function
void updateWiFiStatus(const char *status, uint32_t text_color, uint32_t icon_color) {
    if (ui_MainMenu_Label_connectStatus) {
        lv_label_set_text(ui_MainMenu_Label_connectStatus, status);
        lv_obj_set_style_text_color(ui_MainMenu_Label_connectStatus, lv_color_hex(text_color), LV_PART_MAIN);
    }
}

// WiFi dropdown option update function
void updateWiFiOption(const char *ssid_list) {
    if (ui_MainMenu_Dropdown_NetworkList) {
        lv_dropdown_clear_options(ui_MainMenu_Dropdown_NetworkList);
        
        char *temp_list = strdup(ssid_list);
        if (temp_list) {
            char *token = strtok(temp_list, "\n");
            int index = 0;
            while (token != NULL) {
                lv_dropdown_add_option(ui_MainMenu_Dropdown_NetworkList, token, index);
                token = strtok(NULL, "\n");
                index++;
            }
            free(temp_list);
        }
    }
}

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
    // 顶部下滑（LV_DIR_BOTTOM）- 进入 Settings 页面
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

    // 底部上滑（LV_DIR_TOP）- 返回 Overview 页面
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

// Load saved WiFi config from g_config into Settings UI
void loadWiFiConfigToUI(void) {
    if (ui_MainMenu_Dropdown_NetworkList == NULL || ui_MainMenu_Textarea_Password == NULL) {
        log_w("[WiFi] Settings UI not ready, skipping config load");
        return;
    }

    if (strlen(g_config.ssid) > 0) {
        // Set dropdown text to saved SSID (shows even if not in scanned list)
        lv_dropdown_set_text(ui_MainMenu_Dropdown_NetworkList, g_config.ssid);
        lv_textarea_set_text(ui_MainMenu_Textarea_Password, g_config.wifipass);

        // Update status label
        char status[64];
        snprintf(status, sizeof(status), "Saved: %s", g_config.ssid);
        lv_label_set_text(ui_MainMenu_Label_connectStatus, status);
        lv_obj_set_style_text_color(ui_MainMenu_Label_connectStatus, lv_color_hex(0x088CFD), 0);

        // Ensure WiFi switch is ON
        lv_obj_add_state(ui_MainMenu_Switch_Wifi, LV_STATE_CHECKED);

        log_i("[WiFi] Loaded saved config to UI: SSID='%s', Pass length=%d",
              g_config.ssid, strlen(g_config.wifipass));
    } else {
        lv_label_set_text(ui_MainMenu_Label_connectStatus, "Not configured");
        lv_obj_set_style_text_color(ui_MainMenu_Label_connectStatus, lv_color_hex(0xFF0000), 0);
        log_i("[WiFi] No saved config to load");
    }

    // 创建 WiFi 状态监控定时器（每 2 秒检查一次并更新 UI 状态标签）
    static bool wifi_status_timer_created = false;
    if (!wifi_status_timer_created) {
        wifi_status_timer_created = true;
        lv_timer_create([](lv_timer_t *timer) {
            (void)timer;
            if (ui_MainMenu_Label_connectStatus == NULL) return;

            wl_status_t status = WiFi.status();
            if (status == WL_CONNECTED) {
                // 获取当前连接的SSID和IP
                const char* ssid = WiFi.SSID().c_str();
                IPAddress ip = WiFi.localIP();

                // 更新状态标签：显示SSID和IP
                char status_buf[128];
                snprintf(status_buf, sizeof(status_buf), "%s\nIP: %d.%d.%d.%d",
                         ssid, ip[0], ip[1], ip[2], ip[3]);
                lv_label_set_text(ui_MainMenu_Label_connectStatus, status_buf);
                lv_obj_set_style_text_color(ui_MainMenu_Label_connectStatus, lv_color_hex(0x00FF00), 0);

                // 更新下拉框：显示当前连接的SSID
                if (ui_MainMenu_Dropdown_NetworkList != NULL && strlen(ssid) > 0) {
                    lv_dropdown_set_text(ui_MainMenu_Dropdown_NetworkList, ssid);
                }
            } else if (status == WL_CONNECT_FAILED) {
                lv_label_set_text(ui_MainMenu_Label_connectStatus, "Connect failed");
                lv_obj_set_style_text_color(ui_MainMenu_Label_connectStatus, lv_color_hex(0xFF0000), 0);
            } else if (status == WL_DISCONNECTED) {
                const char* cur = lv_label_get_text(ui_MainMenu_Label_connectStatus);
                // 如果是用户手动关闭了 WiFi，保持显示 "Disabled"
                if (wifiEnable == false) {
                    if (strcmp(cur, "Disabled") != 0) {
                        lv_label_set_text(ui_MainMenu_Label_connectStatus, "Disabled");
                        lv_obj_set_style_text_color(ui_MainMenu_Label_connectStatus, lv_color_hex(0xFF0000), 0);
                    }
                }
                // 否则是网络问题断开，显示 "Disconnected"
                else if (cur && (strncmp(cur, "IP:", 3) == 0 || strchr(cur, '\n') != NULL)) {
                    lv_label_set_text(ui_MainMenu_Label_connectStatus, "Disconnected");
                    lv_obj_set_style_text_color(ui_MainMenu_Label_connectStatus, lv_color_hex(0xFF0000), 0);
                }
            }
        }, 2000, NULL);
    }
}

void saveWiFiCredential(lv_event_t *e) {
  char newSSID[64];
  lv_dropdown_get_selected_str(ui_MainMenu_Dropdown_NetworkList, newSSID, sizeof(newSSID));
  // read from your input box
  const char *newPWD = lv_textarea_get_text(ui_MainMenu_Textarea_Password);
  // Load → update → save
  uint8_t wifiCount = loadWifiList(wifiList);
  wifiCount = addOrUpdateWifi(newSSID, newPWD, wifiList, wifiCount);
  saveWifiList(wifiList, wifiCount);
  // Also save to NVS config for auto-connect
  config_save_wifi(newSSID, newPWD);
  SCREEN_OFF_TIMER = millis(); // reset timer
}

// Discovery wifi network into dropdown
void scanNetwork(lv_event_t *e) {
  SCREEN_OFF_TIMER = millis(); // reset timer
  scanWiFi(true); // scan and update wifi list in dropdown
}

//toggle wifi on/off
void toggleWiFi(lv_event_t * e) {
  SCREEN_OFF_TIMER = millis(); // reset timer

  // Get the switch object from event
  lv_obj_t *sw = lv_event_get_target(e);
  if (sw == NULL) {
    log_e("toggleWiFi: switch object is NULL");
    return;
  }

  // Check the NEW state of the switch (what the user just set)
  bool new_state = lv_obj_has_state(sw, LV_STATE_CHECKED);

  if (new_state) {
    // Enable WiFi
    wifiEnable = true;
    // Start wifi check timer
    wifi_check_timer = lv_timer_create(lvgl_wifi_check_cb, WIFI_CHECK_INTERVAL, NULL);
    log_d("WiFi Enabled");
  } else {
    // Disable WiFi
    wifiEnable = false;
    // Delete wifi check timer
    if (wifi_check_timer) {
      lv_timer_del(wifi_check_timer);
      wifi_check_timer = NULL;
    }
    if(wifiTaskHandle != NULL) {
      vTaskDelete(wifiTaskHandle);
      wifiTaskHandle = NULL;
    }
    WiFi.disconnectAsync();
    lv_label_set_text(ui_MainMenu_Label_connectStatus, "Disabled");
    lv_obj_set_style_text_color(ui_MainMenu_Label_connectStatus, lv_color_hex(0xFF0000), LV_PART_MAIN);
    log_d("WiFi Disabled");
  }
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
    
    // 如果 WiFi 已连接，同步时间到外部RTC
    if (WiFi.status() == WL_CONNECTED) {
        int8_t tz_min = (timezone_minute_index < 3) ? TIMEZONE_MINUTE_OFFSETS[timezone_minute_index] : 0;
        rtc.ntp_sync(g_config.timezone, tz_min);
        log_i("Time synced with timezone: UTC+%d:%02d", g_config.timezone, tz_min);
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
        WiFi.isConnected() ? "Connected" : "Disconnected",
        WiFi.isConnected() ? String(WiFi.localIP()).c_str() : "N/A"
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
        WiFi.begin(g_config.ssid, g_config.wifipass);
    }
    
    // 如果 WiFi 已连接，同步时间到外部RTC
    if (WiFi.status() == WL_CONNECTED) {
        int8_t tz_min = (timezone_minute_index < 3) ? TIMEZONE_MINUTE_OFFSETS[timezone_minute_index] : 0;
        rtc.ntp_sync(g_config.timezone, tz_min);
        log_i("Time synced with timezone: UTC+%d:%02d", g_config.timezone, tz_min);
    }
    
    // 初始化数据源（使用 switchDataSource 安全切换）
    if (strlen(g_config.nas_type) > 0) {
        log_i("Initializing data source for type: %s", g_config.nas_type);
        
        if (switchDataSource(g_config.nas_type)) {
            // 创建数据轮询任务（在 Core 1 上运行）
            xTaskCreatePinnedToCore(data_poll_task, "data_poll", 4 * 1024, NULL, 2, NULL, 1);
            log_i("Data source initialized, poll task created");
        } else {
            log_e("Failed to initialize data source for type: %s", g_config.nas_type);
        }
    } else {
        log_w("No NAS type configured, data source not created");
    }
    
    log_i("Config loaded: bl_state=%d, scroffdelay=%d, timezone=%d", 
           backlight_state, timeout_index, g_config.timezone);
}

// ============================================================================
// 数据轮询任务（FreeRTOS Task - Core 1）
// ============================================================================
void data_poll_task(void *param) {
    (void)param;  // 未使用的参数
    
    TickType_t last_wake_time = xTaskGetTickCount();
    TickType_t poll_interval = pdMS_TO_TICKS(g_config.poll_sec * 1000UL);  // 使用配置的轮询间隔
    
    log_i("[DataPoll] Task started, poll interval: %ds", g_config.poll_sec);
    
    for (;;) {
        if (g_data_source != nullptr) {
            // 调用 poll() 获取数据
            bool updated = g_data_source->poll();
            
            if (updated) {
                log_d("[DataPoll] Data updated successfully");
            }
            
            // 检查连接状态
            if (!g_data_source->isConnected()) {
                log_w("[DataPoll] Data source disconnected, will retry in next poll");
            }
        } else {
            log_w("[DataPoll] g_data_source is nullptr, skipping poll");
        }
        
        // 等待下次轮询
        vTaskDelayUntil(&last_wake_time, poll_interval);
    }
}

// ============================================================================
// NAS Tab 事件处理函数
// ============================================================================

// NAS 类型枚举
typedef enum {
    NAS_TYPE_SYNOLOGY = 0,    // Synology DSM
    NAS_TYPE_QNAP = 1,         // QNAP QTS
    NAS_TYPE_TRUENAS = 2,      // TrueNAS
    NAS_TYPE_FNOS = 3,          // FNOS
    NAS_TYPE_UNRAID = 4,       // Unraid
    NAS_TYPE_NETDATA = 5,      // Netdata
    NAS_TYPE_SNMP = 6,         // SNMP
    NAS_TYPE_LINUX_HTTP = 7,   // Linux (HTTP)
    NAS_TYPE_LINUX_SERIAL = 8, // Linux (Serial)
    NAS_TYPE_WINDOWS = 9,      // Windows
    NAS_TYPE_MOCK = 10,        // Mock (Test)
    NAS_TYPE_COUNT = 11         // 类型总数
} NasType_t;

// NAS 类型索引到字符串ID的映射
static const char* NAS_TYPE_IDS[] = {
    "synology",      // 0: Synology DSM
    "qnap",          // 1: QNAP QTS
    "truenas",       // 2: TrueNAS
    "fnos",          // 3: FNOS
    "unraid",        // 4: Unraid
    "netdata",       // 5: Netdata
    "snmp",          // 6: SNMP
    "linux_http",    // 7: Linux (HTTP)
    "linux_serial",  // 8: Linux (Serial)
    "windows",       // 9: Windows
    "mock"           // 10: Mock (Test)
};

// NAS 类型的默认端口
static const uint32_t NAS_DEFAULT_PORTS[] = {
    5000,    // Synology DSM (5000 HTTP, 5001 HTTPS)
    8080,    // QNAP QTS
    80,      // TrueNAS
    80,      // FNOS
    80,      // Unraid
    19999,   // Netdata
    161,     // SNMP (UDP)
    8080,    // Linux HTTP
    115200,  // Linux Serial (Baud)
    80       // Windows
};

// NAS 类型是否需要认证（用户名/密码）
static const bool NAS_NEEDS_AUTH[] = {
    true,   // Synology DSM - 需要认证
    true,   // QNAP QTS - 需要认证
    true,   // TrueNAS - 需要认证
    true,   // FNOS - 需要认证
    true,   // Unraid - 需要认证
    false,  // Netdata - 不需要认证
    false,  // SNMP - 不需要密码（使用 community）
    true,   // Linux HTTP - 需要认证
    false,  // Linux Serial - 不需要认证
    true,   // Windows - 可能需要认证
    false   // Mock - 不需要认证
};

// NAS 类型是否需要 HTTPS
static const bool NAS_SUPPORTS_HTTPS[] = {
    true,   // Synology DSM
    true,   // QNAP QTS
    true,   // TrueNAS
    false,  // FNOS (不确定)
    false,  // Unraid
    false,  // Netdata
    false,  // SNMP
    true,   // Linux HTTP
    false,  // Linux Serial
    true,   // Windows
    false   // Mock
};

// NAS 类型是否需要额外配置面板（SNMP/Serial）
static const bool NAS_NEEDS_EXTRA[] = {
    false,  // Synology DSM
    false,  // QNAP QTS
    false,  // TrueNAS
    false,  // FNOS
    false,  // Unraid
    false,  // Netdata
    true,   // SNMP - 需要 community 和版本
    false,  // Linux HTTP
    true,   // Linux Serial - 需要设备名和波特率
    false,  // Windows
    false   // Mock
};

// Tab page 事件处理 - 重置屏幕关闭定时器
void ui_event_MainMenu_Tabpage_nas(lv_event_t * e) {
    lv_event_code_t event_code = lv_event_get_code(e);
    if (event_code == LV_EVENT_PRESSED) {
        resetScreenOffTimer(e);
        // 加载 NAS 配置到 UI
        loadNasConfigToUI();
    }
}

// NAS 类型下拉选择事件
void ui_event_MainMenu_Dropdown_NasType(lv_event_t * e) {
    lv_event_code_t event_code = lv_event_get_code(e);
    if (event_code == LV_EVENT_VALUE_CHANGED) {
        resetScreenOffTimer(e);
        
        lv_obj_t *dropdown = lv_event_get_target(e);
        int selected_index = lv_dropdown_get_selected(dropdown);
        
        // 更新默认端口
        if (ui_MainMenu_Textarea_NasPort != NULL) {
            char port_str[8];
            snprintf(port_str, sizeof(port_str), "%d", NAS_DEFAULT_PORTS[selected_index]);
            lv_textarea_set_text(ui_MainMenu_Textarea_NasPort, port_str);
        }
        
        // 更新字段可见性
        updateNasFieldsVisibility(selected_index);
        
        log_i("NAS type changed to index %d: %s", selected_index, NAS_TYPE_IDS[selected_index]);
    }
}

// HTTPS 开关事件
void ui_event_MainMenu_Switch_NasHttps(lv_event_t * e) {
    lv_event_code_t event_code = lv_event_get_code(e);
    if (event_code == LV_EVENT_VALUE_CHANGED) {
        resetScreenOffTimer(e);

        lv_obj_t *sw = lv_event_get_target(e);
        bool https_enabled = lv_obj_has_state(sw, LV_STATE_CHECKED);

        // 调整端口（如果是支持 HTTPS 的类型）
        if (ui_MainMenu_Textarea_NasPort != NULL && ui_MainMenu_Dropdown_NasType != NULL) {
            int selected_index = lv_dropdown_get_selected(ui_MainMenu_Dropdown_NasType);
            if (NAS_SUPPORTS_HTTPS[selected_index]) {
                if (https_enabled) {
                    lv_textarea_set_text(ui_MainMenu_Textarea_NasPort, "5001");
                } else {
                    char port_str[8];
                    snprintf(port_str, sizeof(port_str), "%d", NAS_DEFAULT_PORTS[selected_index]);
                    lv_textarea_set_text(ui_MainMenu_Textarea_NasPort, port_str);
                }
            }
        }

        log_i("HTTPS switch: %s", https_enabled ? "ON" : "OFF");
    }
}

// 保存按钮事件
void ui_event_MainMenu_Button_NasSave(lv_event_t * e) {
    lv_event_code_t event_code = lv_event_get_code(e);
    if (event_code == LV_EVENT_CLICKED) {
        saveNasConfig(e);
    }
}

// 更新 NAS 字段可见性
void updateNasFieldsVisibility(int nas_type_index) {
    bool needs_auth = NAS_NEEDS_AUTH[nas_type_index];
    bool supports_https = NAS_SUPPORTS_HTTPS[nas_type_index];
    bool needs_extra = NAS_NEEDS_EXTRA[nas_type_index];
    
    // 更新认证面板可见性
    if (ui_MainMenu_Panel_NasAuth != NULL) {
        if (needs_auth) {
            lv_obj_clear_flag(ui_MainMenu_Panel_NasAuth, LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_add_flag(ui_MainMenu_Panel_NasAuth, LV_OBJ_FLAG_HIDDEN);
        }
    }
    
    // 更新 HTTPS 开关可见性
    if (ui_MainMenu_Switch_NasHttps != NULL) {
        if (supports_https) {
            lv_obj_clear_flag(ui_MainMenu_Switch_NasHttps, LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_add_flag(ui_MainMenu_Switch_NasHttps, LV_OBJ_FLAG_HIDDEN);
        }
    }
    
    // 更新额外配置面板可见性
    if (ui_MainMenu_Panel_NasExtra != NULL) {
        if (needs_extra) {
            lv_obj_clear_flag(ui_MainMenu_Panel_NasExtra, LV_OBJ_FLAG_HIDDEN);
            
            // 根据类型更新额外配置标签
            lv_obj_t *label1 = lv_obj_get_child(ui_MainMenu_Panel_NasExtra, 0);  // 第一个 label
            lv_obj_t *label2 = lv_obj_get_child(ui_MainMenu_Panel_NasExtra, 2); // 第二个 label

            if (nas_type_index == NAS_TYPE_SNMP) {
                lv_label_set_text(label1, "SNMP Community:");
                lv_label_set_text(label2, "SNMP Ver:");
            } else if (nas_type_index == NAS_TYPE_LINUX_SERIAL) {
                lv_label_set_text(label1, "Device:");
                lv_label_set_text(label2, "Baud:");
            }
        } else {
            lv_obj_add_flag(ui_MainMenu_Panel_NasExtra, LV_OBJ_FLAG_HIDDEN);
        }
    }
}

// 加载 NAS 配置到 UI
void loadNasConfigToUI(void) {
    if (ui_MainMenu_Dropdown_NasType == NULL || ui_MainMenu_Textarea_NasIP == NULL) {
        log_w("[NAS] Settings UI not ready, skipping config load");
        return;
    }
    
    // 查找 NAS 类型索引
    int nas_type_index = 0;
    for (int i = 0; i < 11; i++) {
        if (strcmp(g_config.nas_type, NAS_TYPE_IDS[i]) == 0) {
            nas_type_index = i;
            break;
        }
    }
    
    // 设置 NAS 类型下拉
    lv_dropdown_set_selected(ui_MainMenu_Dropdown_NasType, nas_type_index);
    
    // 设置 IP 地址
    if (strlen(g_config.nas_ip) > 0) {
        lv_textarea_set_text(ui_MainMenu_Textarea_NasIP, g_config.nas_ip);
    }
    
    // 设置端口
    if (g_config.nas_port > 0) {
        char port_str[8];
        snprintf(port_str, sizeof(port_str), "%d", g_config.nas_port);
        lv_textarea_set_text(ui_MainMenu_Textarea_NasPort, port_str);
    } else {
        // 使用默认端口
        char port_str[8];
        snprintf(port_str, sizeof(port_str), "%d", NAS_DEFAULT_PORTS[nas_type_index]);
        lv_textarea_set_text(ui_MainMenu_Textarea_NasPort, port_str);
    }
    
    // 设置用户名
    if (strlen(g_config.nas_user) > 0) {
        lv_textarea_set_text(ui_MainMenu_Textarea_NasUser, g_config.nas_user);
    }
    
    // 设置密码
    if (strlen(g_config.nas_pass) > 0) {
        lv_textarea_set_text(ui_MainMenu_Textarea_NasPass, g_config.nas_pass);
    }
    
    // 设置 HTTPS 开关
    if (g_config.nas_https) {
        lv_obj_add_state(ui_MainMenu_Switch_NasHttps, LV_STATE_CHECKED);
    } else {
        lv_obj_clear_state(ui_MainMenu_Switch_NasHttps, LV_STATE_CHECKED);
    }
    
    // 设置 SNMP community
    if (strlen(g_config.snmp_comm) > 0 && ui_MainMenu_Textarea_NasExtra1 != NULL) {
        lv_textarea_set_text(ui_MainMenu_Textarea_NasExtra1, g_config.snmp_comm);
    }

    // 设置 SATA/M.2 数量
    if (ui_MainMenu_Dropdown_SataCount != NULL) {
        lv_dropdown_set_selected(ui_MainMenu_Dropdown_SataCount, g_config.sata_disk_count);
        lv_dropdown_set_selected(ui_MainMenu_Dropdown_M2Count, g_config.m2_disk_count);
    }

    // 设置字段可见性
    updateNasFieldsVisibility(nas_type_index);
    
    // 更新状态标签
    if (strlen(g_config.nas_ip) > 0) {
        lv_label_set_text(ui_MainMenu_Label_NasStatus, "Configured");
        lv_obj_set_style_text_color(ui_MainMenu_Label_NasStatus, lv_color_hex(0x00FF00), 0);
    } else {
        lv_label_set_text(ui_MainMenu_Label_NasStatus, "Not configured");
        lv_obj_set_style_text_color(ui_MainMenu_Label_NasStatus, lv_color_hex(0xFF0000), 0);
    }
    
    log_i("[NAS] Loaded NAS config: type=%s, ip=%s, port=%d",
          g_config.nas_type, g_config.nas_ip, g_config.nas_port);
}

// 保存 NAS 配置
void saveNasConfig(lv_event_t * e) {
    (void)e;
    
    if (ui_MainMenu_Dropdown_NasType == NULL) {
        log_e("[NAS] UI not ready");
        return;
    }
    
    SCREEN_OFF_TIMER = millis();  // 重置定时器
    
    // 获取 NAS 类型
    int nas_type_index = lv_dropdown_get_selected(ui_MainMenu_Dropdown_NasType);
    const char *nas_type = NAS_TYPE_IDS[nas_type_index];
    
    // 获取 IP 地址
    const char *nas_ip = lv_textarea_get_text(ui_MainMenu_Textarea_NasIP);
    if (nas_ip == NULL || strlen(nas_ip) == 0) {
        lv_label_set_text(ui_MainMenu_Label_NasStatus, "IP required!");
        lv_obj_set_style_text_color(ui_MainMenu_Label_NasStatus, lv_color_hex(0xFF0000), 0);
        log_w("[NAS] IP address is required");
        return;
    }
    
    // 获取端口
    const char *port_str = lv_textarea_get_text(ui_MainMenu_Textarea_NasPort);
    uint16_t nas_port = atoi(port_str);
    if (nas_port == 0) {
        nas_port = NAS_DEFAULT_PORTS[nas_type_index];
    }
    
    // 获取用户名和密码
    const char *nas_user = "";
    const char *nas_pass = "";
    if (NAS_NEEDS_AUTH[nas_type_index]) {
        nas_user = lv_textarea_get_text(ui_MainMenu_Textarea_NasUser);
        nas_pass = lv_textarea_get_text(ui_MainMenu_Textarea_NasPass);
    }
    
    // 获取 HTTPS 状态
    bool nas_https = lv_obj_has_state(ui_MainMenu_Switch_NasHttps, LV_STATE_CHECKED);
    
    // 获取额外配置（SNMP/Serial）
    const char *snmp_comm = "";
    uint8_t snmp_ver = 1;
    if (NAS_NEEDS_EXTRA[nas_type_index]) {
        if (ui_MainMenu_Textarea_NasExtra1 != NULL) {
            snmp_comm = lv_textarea_get_text(ui_MainMenu_Textarea_NasExtra1);
        }
        // 获取 SNMP 版本
        if (ui_MainMenu_Textarea_NasExtra2 != NULL) {
            int ver_idx = lv_dropdown_get_selected(ui_MainMenu_Textarea_NasExtra2);
            snmp_ver = ver_idx + 1;  // v1=1, v2c=2, v3=3
        }
    }
    
    // 调用 config_save_nas 保存配置
    config_save_nas(nas_type, nas_ip, nas_port, nas_user, nas_pass, nas_https);

    // 保存 SNMP 配置
    pref.begin(NVS_NAMESPACE, false);
    pref.putString(NVS_SNMP_COMM, snmp_comm);
    pref.putUChar(NVS_SNMP_VER, snmp_ver);
    pref.end();
    
    // 更新全局配置
    g_config.snmp_ver = snmp_ver;
    strncpy(g_config.snmp_comm, snmp_comm, sizeof(g_config.snmp_comm) - 1);
    
    // 更新状态标签
    char status[64];
    snprintf(status, sizeof(status), "Saved: %s@%s:%d", nas_user, nas_ip, nas_port);
    lv_label_set_text(ui_MainMenu_Label_NasStatus, status);
    lv_obj_set_style_text_color(ui_MainMenu_Label_NasStatus, lv_color_hex(0x00FF00), 0);
    
    // 尝试切换数据源
    if (switchDataSource(nas_type)) {
        log_i("[NAS] Data source switched to %s", nas_type);
        lv_label_set_text(ui_MainMenu_Label_NasStatus, status);
    } else {
        log_w("[NAS] Failed to switch data source to %s", nas_type);
    }
    
    log_i("[NAS] Config saved: type=%s, ip=%s, port=%d, https=%d",
          nas_type, nas_ip, nas_port, nas_https);
}

// 保存硬盘数量配置（选择后立即保存）
void saveDiskConfig(lv_event_t * e)
{
    if (ui_MainMenu_Dropdown_SataCount == NULL || ui_MainMenu_Dropdown_M2Count == NULL) {
        return;
    }

    uint8_t sata_count = lv_dropdown_get_selected(ui_MainMenu_Dropdown_SataCount);
    uint8_t m2_count = lv_dropdown_get_selected(ui_MainMenu_Dropdown_M2Count);

    config_save_disk_config(sata_count, m2_count);

    g_config.sata_disk_count = sata_count;
    g_config.m2_disk_count = m2_count;

    log_i("[Disk] Config saved: SATA=%d, M.2=%d", sata_count, m2_count);
}

// SATA 硬盘数量下拉框事件
void ui_event_MainMenu_Dropdown_SataCount(lv_event_t * e) {
    lv_event_code_t event_code = lv_event_get_code(e);
    // LV_EVENT_CLICKED (25) 在下拉框关闭时触发，此时值已改变
    if (event_code == LV_EVENT_CLICKED) {
        resetScreenOffTimer(e);
        saveDiskConfig(e);
    }
}

// M.2 硬盘数量下拉框事件
void ui_event_MainMenu_Dropdown_M2Count(lv_event_t * e) {
    lv_event_code_t event_code = lv_event_get_code(e);
    // LV_EVENT_CLICKED (25) 在下拉框关闭时触发，此时值已改变
    if (event_code == LV_EVENT_CLICKED) {
        resetScreenOffTimer(e);
        saveDiskConfig(e);
    }
}
