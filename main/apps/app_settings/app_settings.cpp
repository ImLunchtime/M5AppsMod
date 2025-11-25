#include "app_settings.h"
#include "apps/utils/ui/settings_screen.h"
#include "esp_log.h"

static const char* TAG = "APP_SETTINGS";
static const std::string SETTINGS_FILE_NAME = "/sdcard/settings.txt";

// scroll constants
#define DESC_SCROLL_PAUSE 1000
#define DESC_SCROLL_SPEED 20

using namespace MOONCAKE::APPS;

void AppSettings::onCreate()
{
    _data.hal = mcAppGetDatabase()->Get("HAL")->value<HAL::Hal*>();

    scroll_text_init(&_data.desc_scroll_ctx,
                     _data.hal->canvas(),
                     _data.hal->canvas()->width(),
                     16,
                     DESC_SCROLL_SPEED,
                     DESC_SCROLL_PAUSE);
    hl_text_init(&_data.hint_hl_ctx, _data.hal->canvas(), 20, 1500);
    // Get metadata from settings
    _data.groups = _data.hal->settings()->getMetadata();
}

void AppSettings::onResume()
{
    ANIM_APP_OPEN();
    _data.hal->canvas()->fillScreen(THEME_COLOR_BG);
    _data.hal->canvas()->setFont(FONT_16);
    _data.hal->canvas()->setTextColor(TFT_ORANGE, THEME_COLOR_BG);
    _data.hal->canvas()->setTextSize(1);
    _data.hal->canvas_update();

    _data.selected_group = 0;
    _data.selected_item = 0;
    _data.scroll_offset = 0;
    _data.update_list = true;
    _data.in_group = false;
}

void AppSettings::onRunning()
{
    // Update the settings screen
    bool need_update = UTILS::UI::SETTINGS_SCREEN::update(
        _data.hal,
        _data.groups,
        &_data.hint_hl_ctx,
        &_data.desc_scroll_ctx,
        [this](int group_index)
        {
            // ESP_LOGI(TAG, "handleSettingsMenu on_enter() group_index=%d", group_index);
            switch (group_index)
            {
            case -1:
                // destroy app
                destroyApp();
                break;
            case SETTINGS_GROUP_EXPORT:
                // export settings
                _data.hal->sdcard()->mount(false);
                if (_data.hal->sdcard()->is_mounted())
                {
                    _data.hal->settings()->exportToFile(SETTINGS_FILE_NAME);
                    _data.hal->sdcard()->eject();
                    UTILS::UI::show_message_dialog(_data.hal, "Success", "Settings saved to: " + SETTINGS_FILE_NAME, 0);
                }
                else
                {
                    UTILS::UI::show_message_dialog(_data.hal, "Error", "Failed to mount SD card", 0);
                }
                break;
            case SETTINGS_GROUP_IMPORT:
                // import settings
                _data.hal->sdcard()->mount(false);
                if (_data.hal->sdcard()->is_mounted())
                {
                    if (_data.hal->settings()->importFromFile(SETTINGS_FILE_NAME))
                    {
                        _data.hal->sdcard()->eject();
                        // Stop WiFi
                        UTILS::UI::show_progress(_data.hal, "WiFi", -1, "Stopping...");
                        // Stop LED
                        if (!_data.hal->settings()->getBool("system", "use_led"))
                        {
                            _data.hal->led()->off();
                        }
                        delay(500);
                        _data.hal->wifi()->init();
                        // Connect to WiFi if enabled
                        if (_data.hal->settings()->getBool("wifi", "enabled"))
                        {
                            _data.hal->wifi()->update_status();
                            UTILS::UI::show_progress(_data.hal, "WiFi", -1, "Starting...");
                            delay(500);
                            _data.hal->wifi()->connect();
                        }
                        UTILS::UI::show_message_dialog(_data.hal, "Success", "Loaded from: " + SETTINGS_FILE_NAME, 0);
                    }
                    else
                    {
                        UTILS::UI::show_error_dialog(_data.hal,
                                                     "Error",
                                                     "Failed to import settings from: " + SETTINGS_FILE_NAME);
                    }
                }
                else
                {
                    UTILS::UI::show_error_dialog(_data.hal, "Error", "Failed to mount SD card", "OK");
                }
                break;
            }
        });

    // Update the display if needed
    if (need_update)
    {
        _data.hal->canvas_update();
    }
}

void AppSettings::onDestroy()
{
    // Free scroll context
    scroll_text_free(&_data.desc_scroll_ctx);
    // Free hint text context
    hl_text_free(&_data.hint_hl_ctx);
}