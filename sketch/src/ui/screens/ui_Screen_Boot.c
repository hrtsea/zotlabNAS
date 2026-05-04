// SquareLine-style Boot screen implementation
#include "ui_Screen_Boot.h"
#include "../ui_helpers.h"
#include "../ui_events.h"
#include "../ui.h"
#include <stdio.h>
#include <Arduino.h>
#include "../../data/nas_config.h"

// ═══════════════════════════════════════════════════════════
// Local variables
// ═══════════════════════════════════════════════════════════
static lv_obj_t* ui_LogoLabel = NULL;
static lv_obj_t* ui_VersionLabel = NULL;
static lv_obj_t* ui_ProgressBar = NULL;
static lv_obj_t* ui_StatusLabel = NULL;
static lv_timer_t* ui_TimeoutTimer = NULL;
static uint8_t ui_CurrentStage = 0;
static bool ui_TimerActive = false;

// ═══════════════════════════════════════════════════════════
// Local functions
// ═══════════════════════════════════════════════════════════
static void ui_Screen_Boot_timeout_callback(lv_timer_t* timer)
{
    ui_TimerActive = false;
    
    // Load overview screen
    if (ui_Screen_Overview == NULL) {
        ui_Screen_Overview_screen_init();
    }
    lv_scr_load(ui_Screen_Overview);
    
    ui_Screen_Boot_stop_timeout();
}

// ═══════════════════════════════════════════════════════════
// Screen initialization
// ═══════════════════════════════════════════════════════════
void ui_Screen_Boot_screen_init(void)
{
    // Create screen object
    ui_Screen_Boot = lv_obj_create(NULL);
    lv_obj_clear_flag(ui_Screen_Boot, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(ui_Screen_Boot, lv_color_hex(0x000000), 0);
    lv_obj_set_style_border_width(ui_Screen_Boot, 0, 0);

    // Add event handler
    lv_obj_add_event_cb(ui_Screen_Boot, ui_Screen_Boot_event_handler, LV_EVENT_ALL, NULL);

    // Create logo label
    ui_LogoLabel = lv_label_create(ui_Screen_Boot);
    static char logo_str[32];
    snprintf(logo_str, sizeof(logo_str), "%s %s", NAS_LOGO, NAS_TYPE);
    lv_label_set_text(ui_LogoLabel, logo_str);
    lv_obj_set_style_text_font(ui_LogoLabel, &lv_font_montserrat_48, 0);
    lv_obj_set_style_text_color(ui_LogoLabel, lv_color_hex(0x00E676), 0);
    lv_obj_align(ui_LogoLabel, LV_ALIGN_CENTER, 0, -40);

    // Create version label
    ui_VersionLabel = lv_label_create(ui_Screen_Boot);
    static char version_str[16];
    snprintf(version_str, sizeof(version_str), "v%s", APP_VERSION);
    lv_label_set_text(ui_VersionLabel, version_str);
    lv_obj_set_style_text_font(ui_VersionLabel, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(ui_VersionLabel, lv_color_hex(0x888888), 0);
    lv_obj_align(ui_VersionLabel, LV_ALIGN_CENTER, 0, 10);

    // Create progress bar
    ui_ProgressBar = lv_bar_create(ui_Screen_Boot);
    lv_obj_set_size(ui_ProgressBar, 200, 8);
    lv_obj_align(ui_ProgressBar, LV_ALIGN_CENTER, 0, 40);
    lv_obj_set_style_bg_color(ui_ProgressBar, lv_color_hex(0x333333), 0);
    lv_obj_set_style_bg_color(ui_ProgressBar, lv_color_hex(0x00E676), LV_PART_INDICATOR);

    // Create status label
    ui_StatusLabel = lv_label_create(ui_Screen_Boot);
    lv_label_set_text(ui_StatusLabel, "Initializing system...");
    lv_obj_set_style_text_font(ui_StatusLabel, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(ui_StatusLabel, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(ui_StatusLabel, LV_ALIGN_CENTER, 0, 60);

    // Start timeout timer (3 seconds)
    ui_TimeoutTimer = lv_timer_create(ui_Screen_Boot_timeout_callback, 3000, NULL);
    ui_TimerActive = true;

    log_i("Boot screen initialized");
}

// ═══════════════════════════════════════════════════════════
// Screen cleanup
// ═══════════════════════════════════════════════════════════
void ui_Screen_Boot_screen_cleanup(void)
{
    if (ui_Screen_Boot != NULL) {
        lv_obj_del(ui_Screen_Boot);
        ui_Screen_Boot = NULL;
    }
    ui_Screen_Boot_stop_timeout();
    log_i("Boot screen cleaned up");
}

// ═══════════════════════════════════════════════════════════
// Update progress
// ═══════════════════════════════════════════════════════════
void ui_Screen_Boot_update_progress(uint8_t stage, const char* message)
{
    if (ui_Screen_Boot == NULL) return;
    
    ui_CurrentStage = stage;
    
    // Update progress bar
    uint8_t progress = (stage * 25);
    lv_bar_set_value(ui_ProgressBar, progress, LV_ANIM_ON);
    
    // Update status label
    if (message != NULL) {
        lv_label_set_text(ui_StatusLabel, message);
    }
}

// ═══════════════════════════════════════════════════════════
// Stop timeout timer
// ═══════════════════════════════════════════════════════════
void ui_Screen_Boot_stop_timeout(void)
{
    if (ui_TimeoutTimer != NULL) {
        lv_timer_del(ui_TimeoutTimer);
        ui_TimeoutTimer = NULL;
        ui_TimerActive = false;
    }
}

// ═══════════════════════════════════════════════════════════
// Check if active
// ═══════════════════════════════════════════════════════════
bool ui_Screen_Boot_is_active(void)
{
    return (ui_Screen_Boot != NULL);
}
