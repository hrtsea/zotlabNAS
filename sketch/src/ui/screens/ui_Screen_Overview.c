#include <Arduino.h>
#include "lvgl.h"

// Forward declarations
static void ui_event_Screen_Overview_gesture(lv_event_t * e);
static void create_status_bar(lv_obj_t *parent);
static void create_cpu_module(lv_obj_t *parent);
static void create_mem_disk_module(lv_obj_t *parent);
static void create_hdd_indicators(lv_obj_t *parent);
void ui_Screen_Overview_screen_destroy(void);

lv_obj_t * ui_Screen_Overview = NULL;

// 自定义颜色定义（和原图保持一致）
#define COLOR_BG        lv_color_hex(0x000000)      // 背景黑色
#define COLOR_PRIMARY   lv_color_hex(0x40E0D0)      // 主色：青绿色
#define COLOR_TEXT      lv_color_hex(0xFFFFFF)      // 文本白色
#define COLOR_INACTIVE  lv_color_hex(0x333333)      // 未激活/背景条灰色
#define COLOR_LED_GREEN lv_color_hex(0x00FF00)      // 绿色指示灯
#define COLOR_LED_GRAY  lv_color_hex(0x666666)      // 灰色指示灯

/* -------------------------- 顶部状态栏 -------------------------- */
static void create_status_bar(lv_obj_t *parent)
{
    lv_obj_t *status_bar = lv_obj_create(parent);
    lv_obj_set_size(status_bar, 640, 35);
    lv_obj_set_style_bg_color(status_bar, COLOR_BG, 0);
    lv_obj_set_style_border_width(status_bar, 0, 0);
    lv_obj_set_style_radius(status_bar, 0, 0);
    lv_obj_clear_flag(status_bar, LV_OBJ_FLAG_SCROLLABLE);

    // 左侧标题+时间
    lv_obj_t *label_title = lv_label_create(status_bar);
    lv_label_set_text(label_title, "BOOL NAS");
    lv_obj_set_style_text_color(label_title, COLOR_TEXT, 0);
    lv_obj_set_style_text_font(label_title, &lv_font_montserrat_18, 0);
    lv_obj_align(label_title, LV_ALIGN_LEFT_MID, 15, 0);

    lv_obj_t *label_time = lv_label_create(status_bar);
    lv_label_set_text(label_time, "22:10");
    lv_obj_set_style_text_color(label_time, COLOR_TEXT, 0);
    lv_obj_set_style_text_font(label_time, &lv_font_montserrat_18, 0);
    lv_obj_align(label_time, LV_ALIGN_LEFT_MID, 110, 0);

    // 中间上下行速度
    lv_obj_t *label_up = lv_label_create(status_bar);
    lv_label_set_text(label_up, "▲ 0.91MB/s");
    lv_obj_set_style_text_color(label_up, COLOR_TEXT, 0);
    lv_obj_set_style_text_font(label_up, &lv_font_montserrat_16, 0);
    lv_obj_align(label_up, LV_ALIGN_CENTER, -70, 0);

    lv_obj_t *label_down = lv_label_create(status_bar);
    lv_label_set_text(label_down, "▼ 5.20MB/s");
    lv_obj_set_style_text_color(label_down, COLOR_TEXT, 0);
    lv_obj_set_style_text_font(label_down, &lv_font_montserrat_16, 0);
    lv_obj_align(label_down, LV_ALIGN_CENTER, 70, 0);

    // 右侧IP
    lv_obj_t *label_ip = lv_label_create(status_bar);
    lv_label_set_text(label_ip, "IP: 192.168.1.888");
    lv_obj_set_style_text_color(label_ip, COLOR_TEXT, 0);
    lv_obj_set_style_text_font(label_ip, &lv_font_montserrat_16, 0);
    lv_obj_align(label_ip, LV_ALIGN_RIGHT_MID, -15, 0);
}

/* -------------------------- 左侧CPU模块 -------------------------- */
static void create_cpu_module(lv_obj_t *parent)
{
    lv_obj_t *cpu_container = lv_obj_create(parent);
    lv_obj_set_size(cpu_container, 220, 85);
    lv_obj_set_style_bg_color(cpu_container, COLOR_BG, 0);
    lv_obj_set_style_border_width(cpu_container, 0, 0);
    lv_obj_set_style_radius(cpu_container, 0, 0);
    lv_obj_clear_flag(cpu_container, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_align(cpu_container, LV_ALIGN_TOP_LEFT, 10, 40);

    // 环形进度条（CPU使用率）
    lv_obj_t *arc_cpu = lv_arc_create(cpu_container);
    lv_obj_set_size(arc_cpu, 70, 70);
    lv_arc_set_rotation(arc_cpu, 180);
    lv_arc_set_bg_angles(arc_cpu, 0, 360);
    lv_arc_set_value(arc_cpu, 70);
    lv_obj_set_style_arc_color(arc_cpu, COLOR_INACTIVE, LV_PART_MAIN);
    lv_obj_set_style_arc_color(arc_cpu, COLOR_PRIMARY, LV_PART_INDICATOR);
    lv_obj_set_style_arc_width(arc_cpu, 8, LV_PART_MAIN);
    lv_obj_set_style_arc_width(arc_cpu, 8, LV_PART_INDICATOR);
    lv_obj_align(arc_cpu, LV_ALIGN_LEFT_MID, 0, 0);

    // 环形中间文本
    lv_obj_t *label_cpu_percent = lv_label_create(arc_cpu);
    lv_label_set_text(label_cpu_percent, "70%");
    lv_obj_set_style_text_color(label_cpu_percent, COLOR_TEXT, 0);
    lv_obj_set_style_text_font(label_cpu_percent, &lv_font_montserrat_16, 0);
    lv_obj_center(label_cpu_percent);

    // CPU 信息文本
    lv_obj_t *label_stale = lv_label_create(cpu_container);
    lv_label_set_text(label_stale, "CPU STALE");
    lv_obj_set_style_text_color(label_stale, COLOR_TEXT, 0);
    lv_obj_set_style_text_font(label_stale, &lv_font_montserrat_14, 0);
    lv_obj_align(label_stale, LV_ALIGN_TOP_RIGHT, 0, 5);

    lv_obj_t *label_temp = lv_label_create(cpu_container);
    lv_label_set_text(label_temp, "58°C");
    lv_obj_set_style_text_color(label_temp, COLOR_TEXT, 0);
    lv_obj_set_style_text_font(label_temp, &lv_font_montserrat_18, 0);
    lv_obj_align(label_temp, LV_ALIGN_TOP_RIGHT, 0, 30);

    lv_obj_t *label_temp_desc = lv_label_create(cpu_container);
    lv_label_set_text(label_temp_desc, "处理器温度");
    lv_obj_set_style_text_color(label_temp_desc, COLOR_TEXT, 0);
    lv_obj_set_style_text_font(label_temp_desc, &lv_font_montserrat_12, 0);
    lv_obj_align(label_temp_desc, LV_ALIGN_TOP_RIGHT, 0, 55);
}

/* -------------------------- 右侧内存/磁盘模块 -------------------------- */
static void create_mem_disk_module(lv_obj_t *parent)
{
    lv_obj_t *md_container = lv_obj_create(parent);
    lv_obj_set_size(md_container, 390, 85);
    lv_obj_set_style_bg_color(md_container, COLOR_BG, 0);
    lv_obj_set_style_border_width(md_container, 0, 0);
    lv_obj_set_style_radius(md_container, 0, 0);
    lv_obj_clear_flag(md_container, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_align(md_container, LV_ALIGN_TOP_RIGHT, -10, 40);

    // 内存模块
    lv_obj_t *label_mem_title = lv_label_create(md_container);
    lv_label_set_text(label_mem_title, "MEMORY\n5.2 / 8GB");
    lv_obj_set_style_text_color(label_mem_title, COLOR_TEXT, 0);
    lv_obj_set_style_text_font(label_mem_title, &lv_font_montserrat_14, 0);
    lv_obj_align(label_mem_title, LV_ALIGN_TOP_LEFT, 0, 5);

    lv_obj_t *bar_mem = lv_bar_create(md_container);
    lv_obj_set_size(bar_mem, 240, 18);
    lv_bar_set_value(bar_mem, 72, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(bar_mem, COLOR_INACTIVE, LV_PART_MAIN);
    lv_obj_set_style_bg_color(bar_mem, COLOR_PRIMARY, LV_PART_INDICATOR);
    lv_obj_set_style_radius(bar_mem, 5, LV_PART_MAIN);
    lv_obj_set_style_radius(bar_mem, 5, LV_PART_INDICATOR);
    lv_obj_align(bar_mem, LV_ALIGN_TOP_RIGHT, 0, 10);

    lv_obj_t *label_mem_percent = lv_label_create(md_container);
    lv_label_set_text(label_mem_percent, "72%");
    lv_obj_set_style_text_color(label_mem_percent, COLOR_TEXT, 0);
    lv_obj_set_style_text_font(label_mem_percent, &lv_font_montserrat_16, 0);
    lv_obj_align(label_mem_percent, LV_ALIGN_TOP_RIGHT, -5, 8);

    // 磁盘模块
    lv_obj_t *label_disk_title = lv_label_create(md_container);
    lv_label_set_text(label_disk_title, "DISK\n1024M / 2048GB");
    lv_obj_set_style_text_color(label_disk_title, COLOR_TEXT, 0);
    lv_obj_set_style_text_font(label_disk_title, &lv_font_montserrat_14, 0);
    lv_obj_align(label_disk_title, LV_ALIGN_TOP_LEFT, 0, 45);

    lv_obj_t *bar_disk = lv_bar_create(md_container);
    lv_obj_set_size(bar_disk, 240, 18);
    lv_bar_set_value(bar_disk, 50, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(bar_disk, COLOR_INACTIVE, LV_PART_MAIN);
    lv_obj_set_style_bg_color(bar_disk, COLOR_PRIMARY, LV_PART_INDICATOR);
    lv_obj_set_style_radius(bar_disk, 5, LV_PART_MAIN);
    lv_obj_set_style_radius(bar_disk, 5, LV_PART_INDICATOR);
    lv_obj_align(bar_disk, LV_ALIGN_TOP_RIGHT, 0, 50);

    lv_obj_t *label_disk_percent = lv_label_create(md_container);
    lv_label_set_text(label_disk_percent, "50%");
    lv_obj_set_style_text_color(label_disk_percent, COLOR_TEXT, 0);
    lv_obj_set_style_text_font(label_disk_percent, &lv_font_montserrat_16, 0);
    lv_obj_align(label_disk_percent, LV_ALIGN_TOP_RIGHT, -5, 48);
}

/* -------------------------- 底部硬盘指示灯 -------------------------- */
static void create_hdd_indicators(lv_obj_t *parent)
{
    lv_obj_t *hdd_container = lv_obj_create(parent);
    lv_obj_set_size(hdd_container, 640, 40);
    lv_obj_set_style_bg_color(hdd_container, COLOR_BG, 0);
    lv_obj_set_style_border_width(hdd_container, 0, 0);
    lv_obj_set_style_radius(hdd_container, 0, 0);
    lv_obj_clear_flag(hdd_container, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_align(hdd_container, LV_ALIGN_BOTTOM_MID, 0, -5);

    // 硬盘状态数组
    const char *hdd_labels[] = {"HDD1", "HDD2", "HDD3", "HDD4", "HDD5", "HDD6"};
    const bool hdd_active[] = {false, true, true, true, true, true};

    for (int i = 0; i < 6; i++) {
        lv_obj_t *btn_hdd = lv_btn_create(hdd_container);
        lv_obj_set_size(btn_hdd, 95, 35);
        lv_obj_set_style_bg_color(btn_hdd, COLOR_INACTIVE, 0);
        lv_obj_set_style_border_width(btn_hdd, 0, 0);
        lv_obj_set_style_radius(btn_hdd, 3, 0);
        lv_obj_align(btn_hdd, LV_ALIGN_LEFT_MID, 8 + i * 103, 0);

        lv_obj_t *label_hdd = lv_label_create(btn_hdd);
        lv_label_set_text(label_hdd, hdd_labels[i]);
        lv_obj_set_style_text_color(label_hdd, COLOR_TEXT, 0);
        lv_obj_set_style_text_font(label_hdd, &lv_font_montserrat_16, 0);
        lv_obj_align(label_hdd, LV_ALIGN_LEFT_MID, 10, 0);

        lv_obj_t *led = lv_obj_create(btn_hdd);
        lv_obj_set_size(led, 14, 14);
        lv_obj_set_style_bg_color(led, hdd_active[i] ? COLOR_LED_GREEN : COLOR_LED_GRAY, 0);
        lv_obj_set_style_radius(led, LV_RADIUS_CIRCLE, 0);
        lv_obj_align(led, LV_ALIGN_RIGHT_MID, -10, 0);
    }
}

/* -------------------------- 手势事件回调 -------------------------- */
static void ui_event_Screen_Overview_gesture(lv_event_t * e)
{
    // 这里可以添加手势处理逻辑
    printf("[Overview] Gesture event received\n");
}

/* -------------------------- 主界面初始化 -------------------------- */
void ui_Screen_Overview_screen_init(void)
{
    // 防止重复初始化
    if (ui_Screen_Overview != NULL) {
        printf("[Overview] Screen already initialized, destroying first\n");
        ui_Screen_Overview_screen_destroy();
    }
    
    ui_Screen_Overview = lv_obj_create(NULL);
    lv_obj_clear_flag(ui_Screen_Overview, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(ui_Screen_Overview, COLOR_BG, 0);

    create_status_bar(ui_Screen_Overview);
    create_cpu_module(ui_Screen_Overview);
    create_mem_disk_module(ui_Screen_Overview);
    create_hdd_indicators(ui_Screen_Overview);
    
    // 添加滑动手势事件
    lv_obj_add_event_cb(ui_Screen_Overview, ui_event_Screen_Overview_gesture, LV_EVENT_GESTURE, NULL);
}

void ui_Screen_Overview_screen_destroy(void)
{
    if(ui_Screen_Overview) lv_obj_del(ui_Screen_Overview);

    // NULL screen variables
    ui_Screen_Overview = NULL;
}
