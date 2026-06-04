#include "ui_Screen_Overview.h"
#include "../ui.h"
#include "../ui_events.h"
#include <Arduino.h>
#include "../../data/config.h"  // 包含配置管理
#include "../../data/nas_data.h"  // 包含数据模型
#include "../../net/data_source.h"  // 包含数据源包装函数

// 自定义颜色定义（和原图保持一致）
#define COLOR_BG        lv_color_hex(0x000000)      // 背景黑色
#define COLOR_PRIMARY   lv_color_hex(0x40E0D0)      // 主色：青绿色
#define COLOR_TEXT      lv_color_hex(0xFFFFFF)      // 文本白色
#define COLOR_INACTIVE  lv_color_hex(0x333333)      // 未激活/背景条灰色
#define COLOR_LED_GREEN lv_color_hex(0x00FF00)      // 绿色指示灯
#define COLOR_LED_GRAY  lv_color_hex(0x666666)      // 灰色指示灯

// 全局指针，用于定时器回调访问时间标签和网络速度标签
static lv_obj_t *s_label_time = NULL;
static lv_obj_t *s_label_up = NULL;
static lv_obj_t *s_label_down = NULL;

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
    // 使用配置中存储的 NAS 用户名，若为空则使用 NAS 类型或默认值
    static char title_str[32];
    if (strlen(g_config.nas_user) > 0) {
        snprintf(title_str, sizeof(title_str), "%s", g_config.nas_user);
    } else if (strlen(g_config.nas_type) > 0 && strcmp(g_config.nas_type, "mock") != 0) {
        snprintf(title_str, sizeof(title_str), "%s", g_config.nas_type);
    } else {
        snprintf(title_str, sizeof(title_str), "NAS Monitor");
    }
    lv_label_set_text(label_title, title_str);
    lv_obj_set_style_text_color(label_title, COLOR_TEXT, 0);
    lv_obj_set_style_text_font(label_title, &lv_font_montserrat_14, 0);
    lv_obj_align(label_title, LV_ALIGN_LEFT_MID, 5, 0);

    lv_obj_t *label_time = lv_label_create(status_bar);
    s_label_time = label_time;  // 保存到全局指针
    static char time_str[9];  // HH:MM:SS + null terminator
    snprintf(time_str, sizeof(time_str), "%02d:%02d:%02d", now.hour, now.minute, now.second);
    lv_label_set_text(label_time, time_str);
    lv_obj_set_style_text_color(label_time, COLOR_TEXT, 0);
    lv_obj_set_style_text_font(label_time, &lv_font_montserrat_14, 0);
    lv_obj_align(label_time, LV_ALIGN_LEFT_MID, 110, 0);

    // 中间上下行速度
    lv_obj_t *label_up = lv_label_create(status_bar);
    static char up_str[16];
    // 初始化显示默认值
    snprintf(up_str, sizeof(up_str), "▲ 0.00KB/s");
    lv_label_set_text(label_up, up_str);
    lv_obj_set_style_text_color(label_up, COLOR_TEXT, 0);
    lv_obj_set_style_text_font(label_up, &lv_font_montserrat_14, 0);
    lv_obj_align(label_up, LV_ALIGN_CENTER, -70, 0);
    s_label_up = label_up;  // 保存到全局指针

    lv_obj_t *label_down = lv_label_create(status_bar);
    static char down_str[16];
    snprintf(down_str, sizeof(down_str), "▼ 0.00KB/s");
    lv_label_set_text(label_down, down_str);
    lv_obj_set_style_text_color(label_down, COLOR_TEXT, 0);
    lv_obj_set_style_text_font(label_down, &lv_font_montserrat_14, 0);
    lv_obj_align(label_down, LV_ALIGN_CENTER, 70, 0);
    s_label_down = label_down;  // 保存到全局指针

    // 右侧IP
    lv_obj_t *label_ip = lv_label_create(status_bar);
    lv_label_set_text(label_ip, "IP: 192.168.1.888");
    lv_obj_set_style_text_color(label_ip, COLOR_TEXT, 0);
    lv_obj_set_style_text_font(label_ip, &lv_font_montserrat_14, 0);
    lv_obj_align(label_ip, LV_ALIGN_RIGHT_MID, -15, 0);

    // 分割线（状态栏下方）
    lv_obj_t *divider = lv_obj_create(parent);
    lv_obj_set_size(divider, 640, 2);
    lv_obj_set_style_bg_color(divider, COLOR_INACTIVE, 0);
    lv_obj_set_style_border_width(divider, 0, 0);
    lv_obj_set_style_radius(divider, 0, 0);
    lv_obj_clear_flag(divider, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_align(divider, LV_ALIGN_TOP_MID, 0, 35);
    
    // 创建网络速度更新定时器（与 Mock 数据更新频率同步）
    lv_timer_create([](lv_timer_t *timer) {
        (void)timer;  // 未使用的参数
        
        // 更新上传速度（TX）
        if (s_label_up != NULL) {
            float tx_speed = data_source_get_tx_speed_mbps();
            static char tx_str[16];
            
            if (tx_speed < 0.01f) {
                snprintf(tx_str, sizeof(tx_str), "▲ 0.00KB/s");
            } else if (tx_speed < 1.0f) {
                // 小于 1 MB/s，显示为 KB/s
                snprintf(tx_str, sizeof(tx_str), "▲ %.2fKB/s", tx_speed * 1024.0f);
            } else {
                // 大于等于 1 MB/s，显示为 MB/s
                snprintf(tx_str, sizeof(tx_str), "▲ %.2fMB/s", tx_speed);
            }
            lv_label_set_text(s_label_up, tx_str);
        }
        
        // 更新下载速度（RX）
        if (s_label_down != NULL) {
            float rx_speed = data_source_get_rx_speed_mbps();
            static char rx_str[16];
            
            if (rx_speed < 0.01f) {
                snprintf(rx_str, sizeof(rx_str), "▼ 0.00KB/s");
            } else if (rx_speed < 1.0f) {
                // 小于 1 MB/s，显示为 KB/s
                snprintf(rx_str, sizeof(rx_str), "▼ %.2fKB/s", rx_speed * 1024.0f);
            } else {
                // 大于等于 1 MB/s，显示为 MB/s
                snprintf(rx_str, sizeof(rx_str), "▼ %.2fMB/s", rx_speed);
            }
            lv_label_set_text(s_label_down, rx_str);
        }
    }, 5000, NULL);  // 与 Mock 数据更新频率同步（5秒）以降低CPU负载
}

/* -------------------------- 左侧CPU模块 -------------------------- */
static void create_cpu_module(lv_obj_t *parent)
{
    lv_obj_t *cpu_container = lv_obj_create(parent);
    lv_obj_set_size(cpu_container, 240, 100);
    lv_obj_set_style_bg_color(cpu_container, COLOR_BG, 0);
    lv_obj_set_style_border_width(cpu_container, 0, 0);
    lv_obj_set_style_radius(cpu_container, 0, 0);
    lv_obj_clear_flag(cpu_container, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_align(cpu_container, LV_ALIGN_LEFT_MID, 0, 8);
    lv_obj_set_flex_flow(cpu_container, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(cpu_container, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    // ------ 仪表盘（CPU使用率）------
    lv_obj_t *meter_cpu = lv_meter_create(cpu_container);
    lv_obj_set_size(meter_cpu, 90, 90);

    lv_meter_scale_t *scale_cpu = lv_meter_add_scale(meter_cpu);
    lv_meter_set_scale_range(meter_cpu, scale_cpu, 0, 100, 240, 150);

    lv_meter_indicator_t *cpu_arc_bg = lv_meter_add_arc(meter_cpu, scale_cpu, 6, COLOR_INACTIVE, 0);
    lv_meter_set_indicator_start_value(meter_cpu, cpu_arc_bg, 0);
    lv_meter_set_indicator_end_value(meter_cpu, cpu_arc_bg, 100);

    lv_meter_indicator_t *cpu_arc_val = lv_meter_add_arc(meter_cpu, scale_cpu, 6, COLOR_PRIMARY, 0);
    lv_meter_set_indicator_start_value(meter_cpu, cpu_arc_val, 0);
    lv_meter_set_indicator_end_value(meter_cpu, cpu_arc_val, 70);

    lv_meter_indicator_t *cpu_needle = lv_meter_add_needle_line(meter_cpu, scale_cpu, 2, COLOR_PRIMARY, 0);
    lv_meter_set_indicator_value(meter_cpu, cpu_needle, 70);

    lv_obj_set_style_bg_color(meter_cpu, COLOR_BG, 0);
    lv_obj_set_style_border_width(meter_cpu, 0, 0);
    lv_obj_set_style_pad_all(meter_cpu, 0, 0);

    lv_obj_t *label_cpu_percent = lv_label_create(meter_cpu);
    lv_label_set_text(label_cpu_percent, "70%");
    lv_obj_set_style_text_color(label_cpu_percent, COLOR_TEXT, 0);
    lv_obj_set_style_text_font(label_cpu_percent, &lv_font_montserrat_14, 0);
    lv_obj_align(label_cpu_percent, LV_ALIGN_CENTER, 0, 20);

    // ------ 仪表盘（CPU温度）------
    lv_obj_t *meter_temp = lv_meter_create(cpu_container);
    lv_obj_set_size(meter_temp, 90, 90);

    lv_meter_scale_t *scale_temp = lv_meter_add_scale(meter_temp);
    lv_meter_set_scale_range(meter_temp, scale_temp, 0, 100, 240, 150);

    lv_meter_indicator_t *temp_arc_bg = lv_meter_add_arc(meter_temp, scale_temp, 6, COLOR_INACTIVE, 0);
    lv_meter_set_indicator_start_value(meter_temp, temp_arc_bg, 0);
    lv_meter_set_indicator_end_value(meter_temp, temp_arc_bg, 100);

    lv_color_t COLOR_TEMP = lv_color_hex(0xFF8C00); // 橙色表示温度
    lv_meter_indicator_t *temp_arc_val = lv_meter_add_arc(meter_temp, scale_temp, 6, COLOR_TEMP, 0);
    lv_meter_set_indicator_start_value(meter_temp, temp_arc_val, 0);
    lv_meter_set_indicator_end_value(meter_temp, temp_arc_val, 58);

    lv_meter_indicator_t *temp_needle = lv_meter_add_needle_line(meter_temp, scale_temp, 2, COLOR_TEMP, 0);
    lv_meter_set_indicator_value(meter_temp, temp_needle, 58);

    lv_obj_set_style_bg_color(meter_temp, COLOR_BG, 0);
    lv_obj_set_style_border_width(meter_temp, 0, 0);
    lv_obj_set_style_pad_all(meter_temp, 0, 0);

    lv_obj_t *label_temp_val = lv_label_create(meter_temp);
    lv_label_set_text(label_temp_val, "58°C");
    lv_obj_set_style_text_color(label_temp_val, COLOR_TEXT, 0);
    lv_obj_set_style_text_font(label_temp_val, &lv_font_montserrat_14, 0);
    lv_obj_align(label_temp_val, LV_ALIGN_CENTER, 0, 20);

    
}

/* -------------------------- 右侧内存/磁盘模块 -------------------------- */
static void create_mem_disk_module(lv_obj_t *parent)
{
    lv_obj_t *md_container = lv_obj_create(parent);
    lv_obj_set_size(md_container, 380, 100);
    lv_obj_set_style_bg_color(md_container, COLOR_BG, 0);
    lv_obj_set_style_border_width(md_container, 0, 0);
    lv_obj_set_style_radius(md_container, 0, 0);
    lv_obj_set_style_pad_all(md_container, 0, 0);
    lv_obj_clear_flag(md_container, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_align(md_container, LV_ALIGN_TOP_RIGHT, 0, 37);

    // 父容器启用垂直 flex 自动布局
    lv_obj_set_flex_flow(md_container, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(md_container, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    // 内存模块容器（上半部分）
    lv_obj_t *mem_container = lv_obj_create(md_container);
    lv_obj_set_size(mem_container, LV_PCT(100), LV_PCT(30));
    lv_obj_set_style_bg_color(mem_container, COLOR_BG, 0);
    lv_obj_set_style_border_width(mem_container, 0, 0);
    lv_obj_set_style_radius(mem_container, 0, 0);
    lv_obj_clear_flag(mem_container, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_flex_flow(mem_container, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(mem_container, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_t *label_mem_title = lv_label_create(mem_container);
    lv_label_set_text(label_mem_title, "MEMORY\n8GB");
    lv_obj_set_style_text_color(label_mem_title, COLOR_TEXT, 0);
    lv_obj_set_style_text_font(label_mem_title, &lv_font_montserrat_14, 0);
    lv_obj_set_width(label_mem_title, LV_PCT(20));

    lv_obj_t *bar_mem = lv_bar_create(mem_container);
    lv_obj_set_size(bar_mem, LV_PCT(60), 18);
    lv_bar_set_value(bar_mem, 72, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(bar_mem, COLOR_INACTIVE, LV_PART_MAIN);
    lv_obj_set_style_bg_color(bar_mem, COLOR_PRIMARY, LV_PART_INDICATOR);
    lv_obj_set_style_radius(bar_mem, 5, LV_PART_MAIN);
    lv_obj_set_style_radius(bar_mem, 5, LV_PART_INDICATOR);

    lv_obj_t *label_mem_percent = lv_label_create(mem_container);
    lv_label_set_text(label_mem_percent, "72%");
    lv_obj_set_style_text_color(label_mem_percent, COLOR_TEXT, 0);
    lv_obj_set_style_text_font(label_mem_percent, &lv_font_montserrat_14, 0);
    lv_obj_set_width(label_mem_percent, LV_PCT(10));
    lv_obj_set_style_text_align(label_mem_percent, LV_TEXT_ALIGN_RIGHT, 0);

    // 磁盘模块容器（下半部分）
    lv_obj_t *disk_container = lv_obj_create(md_container);
    lv_obj_set_size(disk_container, LV_PCT(100), LV_PCT(30));
    lv_obj_set_style_bg_color(disk_container, COLOR_BG, 0);
    lv_obj_set_style_border_width(disk_container, 0, 0);
    lv_obj_set_style_radius(disk_container, 0, 0);
    lv_obj_clear_flag(disk_container, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_flex_flow(disk_container, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(disk_container, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_t *label_disk_title = lv_label_create(disk_container);
    lv_label_set_text(label_disk_title, "DISK\n2048GB");
    lv_obj_set_style_text_color(label_disk_title, COLOR_TEXT, 0);
    lv_obj_set_style_text_font(label_disk_title, &lv_font_montserrat_14, 0);
    lv_obj_set_width(label_disk_title, LV_PCT(20));

    lv_obj_t *bar_disk = lv_bar_create(disk_container);
    lv_obj_set_size(bar_disk, LV_PCT(60), 18);
    lv_bar_set_value(bar_disk, 50, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(bar_disk, COLOR_INACTIVE, LV_PART_MAIN);
    lv_obj_set_style_bg_color(bar_disk, COLOR_PRIMARY, LV_PART_INDICATOR);
    lv_obj_set_style_radius(bar_disk, 5, LV_PART_MAIN);
    lv_obj_set_style_radius(bar_disk, 5, LV_PART_INDICATOR);

    lv_obj_t *label_disk_percent = lv_label_create(disk_container);
    lv_label_set_text(label_disk_percent, "50%");
    lv_obj_set_style_text_color(label_disk_percent, COLOR_TEXT, 0);
    lv_obj_set_style_text_font(label_disk_percent, &lv_font_montserrat_14, 0);
    lv_obj_set_width(label_disk_percent, LV_PCT(10));
    lv_obj_set_style_text_align(label_disk_percent, LV_TEXT_ALIGN_RIGHT, 0);
    lv_obj_align(label_disk_percent, LV_ALIGN_RIGHT_MID, 0, 0);
}

/* -------------------------- 底部硬盘指示灯 -------------------------- */
static void create_hdd_indicators(lv_obj_t *parent)
{
    lv_obj_t *hdd_container = lv_obj_create(parent);
    lv_obj_set_size(hdd_container, 640, 35);
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
        lv_obj_set_style_pad_all(btn_hdd, 0, 0);
        lv_obj_align(btn_hdd, LV_ALIGN_LEFT_MID, 8 + i * 103, 0);
        
        // 添加点击事件：进入 Storage 页面
        lv_obj_add_event_cb(btn_hdd, ui_event_Screen_Overview_hdd_clicked, LV_EVENT_ALL, NULL);

        lv_obj_t *label_hdd = lv_label_create(btn_hdd);
        lv_label_set_text(label_hdd, hdd_labels[i]);
        lv_obj_set_style_text_color(label_hdd, COLOR_TEXT, 0);
        lv_obj_set_style_text_font(label_hdd, &lv_font_montserrat_14, 0);
        lv_obj_center(label_hdd);

        lv_obj_t *led = lv_obj_create(btn_hdd);
        lv_obj_set_size(led, 14, 14);
        lv_obj_set_style_bg_color(led, hdd_active[i] ? COLOR_LED_GREEN : COLOR_LED_GRAY, 0);
        lv_obj_set_style_radius(led, LV_RADIUS_CIRCLE, 0);
        lv_obj_align(led, LV_ALIGN_RIGHT_MID, -5, 0);
    }
}

/* -------------------------- 主界面初始化 -------------------------- */
void ui_Screen_Overview_screen_init(void)
{
    // 防止重复初始化
    if (ui_Screen_Overview != NULL) {
        log_w("[Overview] Screen already initialized, destroying first");
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
    
    // 添加时间更新定时器（5秒更新一次）
    lv_timer_create([](lv_timer_t *timer) {
        static char time_str[9];
        snprintf(time_str, sizeof(time_str), "%02d:%02d:%02d", now.hour, now.minute, now.second);
        lv_label_set_text((lv_obj_t *)timer->user_data, time_str);
    }, 5000, s_label_time);  // 降低更新频率以优化CPU负载
}

void ui_Screen_Overview_screen_destroy(void)
{
    if(ui_Screen_Overview) lv_obj_del(ui_Screen_Overview);

    // NULL screen variables
    ui_Screen_Overview = NULL;
}
