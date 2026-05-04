// SquareLine-style UI main implementation
#include "ui.h"
#include "screens/ui_Screen_Boot.h"
#include "screens/ui_Screen_Overview.h"
#include "screens/ui_Screen_Storage.h"
#include "screens/ui_Screen_Settings.h"

// ═══════════════════════════════════════════════════════════
// Global screen objects (SquareLine style)
// ═══════════════════════════════════════════════════════════
lv_obj_t* ui_Screen_Boot        = NULL;
lv_obj_t* ui_Screen_Overview    = NULL;
// ui_Screen_Storage defined in ui_Screen_Storage.c (extern in ui.h)
// ui_Screen_Settings defined in ui_Screen_Settings.c (extern in ui.h)

void ui_init(void)
{
    // Initialize all screens
    ui_Screen_Boot_screen_init();
    ui_Screen_Overview_screen_init();
    ui_Screen_Storage_screen_init();
    ui_Screen_Settings_screen_init();

    // Load boot screen by default
    lv_scr_load(ui_Screen_Boot);
}
