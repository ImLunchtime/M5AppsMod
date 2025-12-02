/**
 * @file hal_cardputer.cpp
 * @author Forairaaaaa
 * @brief
 * @version 0.1
 * @date 2023-09-22
 *
 * @copyright Copyright (c) 2023
 *
 */
#include "hal_cardputer.h"
#include <mooncake.h>
#include "apps/utils/common_define.h"
#include "bat/adc_read.h"
#include "esp_log.h"

static const char* TAG = "HAL";

using namespace HAL;

void HalCardputer::_init_display()
{
    ESP_LOGI(TAG, "init display");

    // Display
    // _display = new LGFX_Cardputer;
    _display = new M5GFX;
    _display->init();
    // _display->setRotation(1);

    // Canvas
    _canvas = new LGFX_Sprite(_display);
    _canvas->createSprite(_display->width() - 16 - 2, 109);

    _canvas_space_bar = new LGFX_Sprite(_display);
    _canvas_space_bar->createSprite(_display->width() - _canvas->width(), display()->height());

    _canvas_system_bar = new LGFX_Sprite(_display);
    _canvas_system_bar->createSprite(_canvas->width(), _display->height() - _canvas->height());
}

void HalCardputer::_init_keyboard()
{
    ESP_LOGI(TAG, "init keyboard");
    _keyboard = new KEYBOARD::Keyboard;
    _keyboard->init();
    _board_type = _keyboard->boardType();
}

void HalCardputer::_init_speaker()
{
    ESP_LOGI(TAG, "init speaker");

    _speaker = new Speaker();
    _speaker->begin();
}

void HalCardputer::_init_button() { _homeButton = new Button(0); }

void HalCardputer::_init_bat() { adc_read_init(); }

void HalCardputer::_init_sdcard() { _sdcard = new SDCard; }

void HalCardputer::_init_usb() { _usb = new USB(this); }

void HalCardputer::_init_wifi()
{
    _wifi = new WiFi(_settings);
    _wifi->set_status_callback(
        [this](wifi_status_t status)
        {
            // ESP_LOGI(TAG, "WiFi status: %d", status);
            if (!_settings->getBool("system", "use_led"))
            {
                return;
            }
            LED* led = this->led();
            if (led == nullptr)
            {
                return;
            }
            switch (status)
            {
            case WIFI_STATUS_IDLE:
                led->off();
                break;
            case WIFI_STATUS_DISCONNECTED:
                led->blink_periodic({127, 0, 0}, 50, 2000);
                break;
            case WIFI_STATUS_CONNECTING:
                led->blink_periodic({127, 0, 0}, 50, 1000);
                break;
            case WIFI_STATUS_CONNECTED_WEAK:
                led->blink_periodic_double({255 / 2, 106 / 2, 0}, 50, 50, 2000);
                break;
            case WIFI_STATUS_CONNECTED_GOOD:
                led->blink_periodic_double({120 / 2, 255 / 2, 32 / 2}, 50, 50, 2000);
                break;
            case WIFI_STATUS_CONNECTED_STRONG:
                led->blink_periodic_double({0, 38 / 2, 255 / 2}, 50, 50, 2000);
                break;
            };
        });
}

void HalCardputer::_init_led()
{
    _led = new LED(RGB_LED_GPIO);
    _led->init();
}

void HalCardputer::init()
{
    ESP_LOGI(TAG, "HAL init");

    _init_display();
    _init_keyboard();
    _init_speaker();
    _init_button();
    _init_bat();
    _init_sdcard();
    _init_led();
    _init_wifi();
}

uint8_t HalCardputer::getBatLevel(float voltage)
{
    uint8_t result = 0;
    if (voltage >= 4.12)
        result = 100;
    else if (voltage >= 3.88)
        result = 75;
    else if (voltage >= 3.61)
        result = 50;
    else if (voltage >= 3.40)
        result = 25;
    else
        result = 0;
    return result;
}

float HalCardputer::getBatVoltage() { return static_cast<float>(adc_read_get_value()) * 2 / 1000; }
