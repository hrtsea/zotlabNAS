#include "../ui.h"
#include "../ui_events.h"  // for ui_event_Screen_Storage_gesture
#include <Arduino.h>

lv_obj_t * ui_Screen_Storage = NULL;

// 颜色定义
// 注意：COLOR_BG 和 COLOR_TEXT 在 nas_config.h 中已有定义（整数值）
// 此处重新定义为 LVGL 颜色类型，用于 UI 渲染
#ifdef COLOR_BG
#undef COLOR_BG
#endif
#define COLOR_BG        lv_color_hex(0x000000)      // 背景黑色

#ifdef COLOR_TEXT
#undef COLOR_TEXT
#endif
#define COLOR_TEXT      lv_color_hex(0xFFFFFF)      // 文本白色

#define COLOR_PRIMARY   lv_color_hex(0x40E0D0)      // 主色：青绿色
#define COLOR_INACTIVE  lv_color_hex(0x333333)      // 未激活/背景条灰色

// 硬盘数量常量
#define HDD_COUNT 8

// LVGL对象创建检查宏
#define LV_OBJ_CHECK(obj, name) \
    if (!(obj)) { \
        log_e("[Storage] Failed to create %s - out of memory", name); \
        return; \
    }

/* -------------------------- 顶部状态栏 -------------------------- */
static void create_storage_status_bar(lv_obj_t *parent)
{
    lv_obj_t *status_bar = lv_obj_create(parent);
    LV_OBJ_CHECK(status_bar, "status_bar");
    
    lv_obj_set_size(status_bar, 640, 35);
    lv_obj_set_style_bg_color(status_bar, COLOR_BG, 0);
    lv_obj_set_style_border_width(status_bar, 0, 0);
    lv_obj_set_style_radius(status_bar, 0, 0);
    lv_obj_clear_flag(status_bar, LV_OBJ_FLAG_SCROLLABLE);

    // 左侧标题+时间
    lv_obj_t *label_title = lv_label_create(status_bar);
    LV_OBJ_CHECK(label_title, "label_title");
    lv_label_set_text(label_title, "BOOL NAS");
    lv_obj_set_style_text_color(label_title, COLOR_TEXT, 0);
    lv_obj_set_style_text_font(label_title, &lv_font_montserrat_14, 0);
    lv_obj_align(label_title, LV_ALIGN_LEFT_MID, 5, 0);

    lv_obj_t *label_time = lv_label_create(status_bar);
    LV_OBJ_CHECK(label_time, "label_time");
    lv_label_set_text(label_time, "22:10");
    lv_obj_set_style_text_color(label_time, COLOR_TEXT, 0);
    lv_obj_set_style_text_font(label_time, &lv_font_montserrat_14, 0);
    lv_obj_align(label_time, LV_ALIGN_LEFT_MID, 110, 0);

    // 中间上下行速度
    lv_obj_t *label_up = lv_label_create(status_bar);
    LV_OBJ_CHECK(label_up, "label_up");
    lv_label_set_text(label_up, "▲ 0.91MB/s");
    lv_obj_set_style_text_color(label_up, COLOR_TEXT, 0);
    lv_obj_set_style_text_font(label_up, &lv_font_montserrat_14, 0);
    lv_obj_align(label_up, LV_ALIGN_CENTER, -70, 0);

    lv_obj_t *label_down = lv_label_create(status_bar);
    LV_OBJ_CHECK(label_down, "label_down");
    lv_label_set_text(label_down, "▼ 5.20MB/s");
    lv_obj_set_style_text_color(label_down, COLOR_TEXT, 0);
    lv_obj_set_style_text_font(label_down, &lv_font_montserrat_14, 0);
    lv_obj_align(label_down, LV_ALIGN_CENTER, 70, 0);

    // 右侧IP
    lv_obj_t *label_ip = lv_label_create(status_bar);
    LV_OBJ_CHECK(label_ip, "label_ip");
    lv_label_set_text(label_ip, "IP: 192.168.1.888");
    lv_obj_set_style_text_color(label_ip, COLOR_TEXT, 0);
    lv_obj_set_style_text_font(label_ip, &lv_font_montserrat_14, 0);
    lv_obj_align(label_ip, LV_ALIGN_RIGHT_MID, -15, 0);

    // 分割线（状态栏下方）
    lv_obj_t *divider = lv_obj_create(parent);
    LV_OBJ_CHECK(divider, "divider");
    lv_obj_set_size(divider, 640, 2);
    lv_obj_set_style_bg_color(divider, COLOR_INACTIVE, 0);
    lv_obj_set_style_border_width(divider, 0, 0);
    lv_obj_set_style_radius(divider, 0, 0);
    lv_obj_clear_flag(divider, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_align(divider, LV_ALIGN_TOP_MID, 0, 35);
}

/* -------------------------- 硬盘存储条区域 -------------------------- */
static void create_hdd_storage_bars(lv_obj_t *parent)
{
    log_i("[Storage] Creating storage container...");
    lv_obj_t *storage_container = lv_obj_create(parent);
    LV_OBJ_CHECK(storage_container, "storage_container");
    
    log_i("[Storage] storage_container = %p", storage_container);
    lv_obj_set_size(storage_container, 640, 130);
    lv_obj_set_style_bg_color(storage_container, COLOR_BG, 0);
    lv_obj_set_style_border_width(storage_container, 0, 0);
    lv_obj_set_style_radius(storage_container, 0, 0);
    lv_obj_clear_flag(storage_container, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_align(storage_container, LV_ALIGN_TOP_MID, 0, 40);

    // 硬盘数据：名称、使用百分比、激活状态
    const char *hdd_names[HDD_COUNT] = {"HDD1", "HDD2", "HDD3", "HDD4", "HDD5", "HDD6", "M.2-1", "M.2-2"};
    const int hdd_usage[HDD_COUNT] = {35, 68, 82, 10, 23, 56, 71, 69};
    const bool hdd_active[HDD_COUNT] = {true, true, true, true, true, true, true, true};

    // 创建两列布局
    for (int col = 0; col < 2; col++) {
        for (int row = 0; row < 4; row++) {
            int index = col * 4 + row;
            if (index >= HDD_COUNT) break;

            log_i("[Storage] Creating HDD %d (col=%d, row=%d)", index, col, row);
            
            int x_offset = col * 310 + 10;
            int y_offset = row * 28 + 5;

            // 硬盘名称标签
            lv_obj_t *label_name = lv_label_create(storage_container);
            LV_OBJ_CHECK(label_name, "label_name");
            lv_label_set_text(label_name, hdd_names[index]);
            lv_obj_set_style_text_color(label_name, COLOR_TEXT, 0);
            lv_obj_set_style_text_font(label_name, &lv_font_montserrat_14, 0);
            lv_obj_align(label_name, LV_ALIGN_TOP_LEFT, x_offset, y_offset);

            // 进度条
            lv_obj_t *bar_hdd = lv_bar_create(storage_container);
            LV_OBJ_CHECK(bar_hdd, "bar_hdd");
            lv_obj_set_size(bar_hdd, 170, 16);
            lv_bar_set_value(bar_hdd, hdd_usage[index], LV_ANIM_OFF);
            lv_obj_set_style_bg_color(bar_hdd, COLOR_INACTIVE, LV_PART_MAIN);
            lv_obj_set_style_bg_color(bar_hdd, COLOR_PRIMARY, LV_PART_INDICATOR);
            lv_obj_set_style_radius(bar_hdd, 4, LV_PART_MAIN);
            lv_obj_set_style_radius(bar_hdd, 4, LV_PART_INDICATOR);
            lv_obj_align(bar_hdd, LV_ALIGN_TOP_LEFT, x_offset + 50, y_offset);

            // 百分比文本
            char percent_text[8];
            snprintf(percent_text, sizeof(percent_text), "%d%%", hdd_usage[index]);
            lv_obj_t *label_percent = lv_label_create(storage_container);
            LV_OBJ_CHECK(label_percent, "label_percent");
            lv_label_set_text(label_percent, percent_text);
            lv_obj_set_style_text_color(label_percent, COLOR_TEXT, 0);
            lv_obj_set_style_text_font(label_percent, &lv_font_montserrat_14, 0);
            lv_obj_align(label_percent, LV_ALIGN_TOP_LEFT, x_offset + 230, y_offset);
        }
    }
}

/* -------------------------- 主界面初始化 -------------------------- */
// 手势事件回调在 ui_events.cpp 中定义

/* 前向声明 */
static void ui_Screen_Storage_screen_destroy(void);

void ui_Screen_Storage_screen_init(void)
{
    // 防止重复初始化
    if (ui_Screen_Storage != NULL) {
        log_w("[Storage] Screen already initialized, destroying first");
        ui_Screen_Storage_screen_destroy();
    }
    
    ui_Screen_Storage = lv_obj_create(NULL);
    LV_OBJ_CHECK(ui_Screen_Storage, "ui_Screen_Storage screen");
    
    lv_obj_clear_flag(ui_Screen_Storage, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(ui_Screen_Storage, COLOR_BG, 0);

    create_storage_status_bar(ui_Screen_Storage);
    create_hdd_storage_bars(ui_Screen_Storage);
    
    // 添加滑动手势事件
    lv_obj_add_event_cb(ui_Screen_Storage, ui_event_Screen_Storage_gesture, LV_EVENT_GESTURE, NULL);
    
    log_i("[Storage] Screen initialized successfully");
}

void ui_Screen_Storage_screen_destroy(void)
{
    if(ui_Screen_Storage) {
        lv_obj_del(ui_Screen_Storage);
        ui_Screen_Storage = NULL;
        log_i("[Storage] Screen destroyed successfully");
    }
}
