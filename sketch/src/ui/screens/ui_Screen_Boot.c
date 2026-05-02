// Boot Screen Implementation - Modern Splash Screen
// Refactored to remove image background, use text + animations instead

#include <Arduino.h>
#include "lvgl.h"
#include "ui_Screen_Overview.h"

// Static object pointers
lv_obj_t * ui_Screen_Boot = NULL;
static lv_obj_t * logo_box = NULL;         // Brand container
static lv_obj_t * brand_label = NULL;      // Brand name: "TuneBar"
static lv_obj_t * subtitle_label = NULL;   // Subtitle
static lv_obj_t * progress_bar = NULL;     // Progress bar
static lv_obj_t * status_label = NULL;     // Status text

// Animation objects
static lv_anim_t breathe_anim;
static lv_anim_t scale_anim;

// Forward declaration
void ui_Screen_Boot_screen_destroy(void);

// Transition callback
static void boot_transition_cb(lv_timer_t * timer) {
    // 1. 先加载新页面
    ui_Screen_Overview_screen_init();
    lv_scr_load(ui_Screen_Overview);
    
    // 2. 再删除旧页面
    ui_Screen_Boot_screen_destroy();
    
    lv_timer_del(timer);
    printf("[BOOT] Screen transition to Overview\n");
}

// Event functions
void ui_event_Screen_Boot(lv_event_t * e)
{
    lv_event_code_t event_code = lv_event_get_code(e);

    if(event_code == LV_EVENT_SCREEN_LOADED) {
        printf("[BOOT] LV_EVENT_SCREEN_LOADED triggered\n");
    }
}

// Animation callbacks
static void breathe_anim_cb(void* var, int32_t opacity) {
    if (brand_label) lv_obj_set_style_opa(brand_label, (lv_opa_t)opacity, 0);
    if (logo_box) lv_obj_set_style_border_opa(logo_box, (lv_opa_t)opacity, 0);
}

static void scale_anim_cb(void* var, int32_t width) {
    if (logo_box) {
        lv_obj_set_width(logo_box, width);
    }
}

// Build functions
void ui_Screen_Boot_screen_init(void)
{
    printf("[BOOT] ui_Screen_Boot_screen_init() started\n");
    
    // Create screen with black background (640x172)
    ui_Screen_Boot = lv_obj_create(NULL);
    lv_obj_set_size(ui_Screen_Boot, 640, 172);
    lv_obj_set_style_bg_color(ui_Screen_Boot, lv_color_hex(0x000000), 0);
    lv_obj_set_style_border_width(ui_Screen_Boot, 0, 0);
    lv_obj_clear_flag(ui_Screen_Boot, LV_OBJ_FLAG_SCROLLABLE);
    
    // Brand container (modern card design) - centered vertically
    logo_box = lv_obj_create(ui_Screen_Boot);
    lv_obj_set_size(logo_box, 240, 50);
    lv_obj_align(logo_box, LV_ALIGN_CENTER, 0, -35);
    
    // Container styling
    lv_obj_set_style_bg_color(logo_box, lv_color_hex(0x1A1A2E), 0);
    lv_obj_set_style_bg_opa(logo_box, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(logo_box, lv_color_hex(0x333355), 0);
    lv_obj_set_style_border_width(logo_box, 1, 0);
    lv_obj_set_style_border_opa(logo_box, LV_OPA_60, 0);
    lv_obj_set_style_radius(logo_box, 8, 0);
    lv_obj_set_style_pad_all(logo_box, 6, 0);
    
    // Shadow effect
    lv_obj_set_style_shadow_color(logo_box, lv_color_hex(0x000000), 0);
    lv_obj_set_style_shadow_width(logo_box, 6, 0);
    lv_obj_set_style_shadow_ofs_x(logo_box, 0, 0);
    lv_obj_set_style_shadow_ofs_y(logo_box, 2, 0);
    lv_obj_set_style_shadow_opa(logo_box, LV_OPA_40, 0);
    lv_obj_set_style_shadow_spread(logo_box, 1, 0);
    
    lv_obj_clear_flag(logo_box, LV_OBJ_FLAG_SCROLLABLE);
    
    // Brand name label
    brand_label = lv_label_create(logo_box);
    lv_label_set_text(brand_label, "TuneBar");
    lv_obj_set_style_text_color(brand_label, lv_color_hex(0x00E676), 0);
    lv_obj_set_style_text_font(brand_label, &lv_font_montserrat_24, 0);
    lv_obj_center(brand_label);
    
    // Subtitle - below logo box
    subtitle_label = lv_label_create(ui_Screen_Boot);
    lv_label_set_text(subtitle_label, "Smart Audio System");
    lv_obj_align(subtitle_label, LV_ALIGN_CENTER, 0, 10);
    lv_obj_set_style_text_color(subtitle_label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(subtitle_label, &lv_font_montserrat_14, 0);
    
    // Progress bar - below subtitle
    progress_bar = lv_bar_create(ui_Screen_Boot);
    lv_obj_set_size(progress_bar, 220, 10);
    lv_obj_align(progress_bar, LV_ALIGN_CENTER, 0, 35);
    lv_bar_set_range(progress_bar, 0, 100);
    lv_bar_set_value(progress_bar, 0, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(progress_bar, lv_color_hex(0x333333), 0);
    lv_obj_set_style_bg_color(progress_bar, lv_color_hex(0x00E676), LV_PART_INDICATOR);
    
    // Status label - below progress bar
    status_label = lv_label_create(ui_Screen_Boot);
    lv_label_set_text(status_label, "Initializing...");
    lv_obj_align(status_label, LV_ALIGN_CENTER, 0, 55);
    lv_obj_set_style_text_color(status_label, lv_color_hex(0x888888), 0);
    lv_obj_set_style_text_font(status_label, &lv_font_montserrat_12, 0);
    
    // Create breathing animation (opacity: 40% <-> 100%, cycle: 2s)
    lv_anim_init(&breathe_anim);
    lv_anim_set_var(&breathe_anim, logo_box);
    lv_anim_set_values(&breathe_anim, LV_OPA_40, LV_OPA_COVER);
    lv_anim_set_time(&breathe_anim, 1000);
    lv_anim_set_playback_time(&breathe_anim, 1000);
    lv_anim_set_repeat_count(&breathe_anim, LV_ANIM_REPEAT_INFINITE);
    lv_anim_set_exec_cb(&breathe_anim, breathe_anim_cb);
    lv_anim_start(&breathe_anim);
    
    // Create scale animation (width: 230 <-> 250px, cycle: 4s) - adjusted for smaller container
    lv_anim_init(&scale_anim);
    lv_anim_set_var(&scale_anim, logo_box);
    lv_anim_set_values(&scale_anim, 230, 250);
    lv_anim_set_time(&scale_anim, 2000);
    lv_anim_set_playback_time(&scale_anim, 2000);
    lv_anim_set_repeat_count(&scale_anim, LV_ANIM_REPEAT_INFINITE);
    lv_anim_set_exec_cb(&scale_anim, scale_anim_cb);
    lv_anim_start(&scale_anim);
    
    printf("[BOOT] Boot screen created with text + animations\n");
    
    lv_obj_add_event_cb(ui_Screen_Boot, ui_event_Screen_Boot, LV_EVENT_ALL, NULL);
    printf("[BOOT] Event callback registered\n");
    
    // 3秒后自动跳转
    lv_timer_create(boot_transition_cb, 3000, NULL);
}

void ui_Screen_Boot_screen_destroy(void)
{
    // Stop animations
    lv_anim_del(&breathe_anim, NULL);
    lv_anim_del(&scale_anim, NULL);
    
    if(ui_Screen_Boot) lv_obj_del(ui_Screen_Boot);
    
    // NULL screen variables
    ui_Screen_Boot = NULL;
    logo_box = NULL;
    brand_label = NULL;
    subtitle_label = NULL;
    progress_bar = NULL;
    status_label = NULL;
}
