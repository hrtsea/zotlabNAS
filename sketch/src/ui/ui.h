// SquareLine-style UI main header
#pragma once

#include <lvgl.h>
#include "ui_helpers.h"
#include "ui_events.h"

#ifdef __cplusplus
extern "C" {
#endif

// ══════════════════════════════════════════════════════════
// Image declarations (from SquareLine Studio)
// ══════════════════════════════════════════════════════════
LV_IMG_DECLARE(ui_img_images_globe_png);    // assets/images/globe.png
LV_IMG_DECLARE(ui_img_images_timezone_png); // assets/images/timezone.png
LV_IMG_DECLARE(ui_img_images_button_png);   // assets/images/button.png

// ══════════════════════════════════════════════════════════
// Font declarations (from SquareLine Studio)
// ══════════════════════════════════════════════════════════
LV_FONT_DECLARE(ui_font_NotoSanThai16);

// ═══════════════════════════════════════════════════════════
// Screen objects (defined in ui.c)
// ═══════════════════════════════════════════════════════════
extern lv_obj_t* ui_Screen_Boot;
extern lv_obj_t* ui_Screen_Overview;
extern lv_obj_t* ui_Screen_Settings;

// ═══════════════════════════════════════════════════════════
// Main UI init
// ═══════════════════════════════════════════════════════════
void ui_init(void);

// ═══════════════════════════════════════════════════════════
// Screen init declarations (called by ui_init)
// ═══════════════════════════════════════════════════════════
void ui_Screen_Boot_screen_init(void);
void ui_Screen_Overview_screen_init(void);
void ui_Screen_Settings_screen_init(void);

// ═══════════════════════════════════════════════════════════
// Screen update declarations (called by UI Manager)
// ═══════════════════════════════════════════════════════════
void ui_Screen_Boot_update_progress(uint8_t stage, const char* message);
void ui_Screen_Overview_update(void);

// ═══════════════════════════════════════════════════════════
// Screen cleanup declarations
// ═══════════════════════════════════════════════════════════
void ui_Screen_Boot_screen_cleanup(void);
void ui_Screen_Overview_screen_cleanup(void);

// ═══════════════════════════════════════════════════════════
// Boot screen helpers
// ═══════════════════════════════════════════════════════════
void ui_Screen_Boot_set_complete_callback(void (*cb)(void*), void* user_data);
void ui_Screen_Boot_stop_timeout(void);
bool ui_Screen_Boot_is_active(void);

#ifdef __cplusplus
}
#endif
