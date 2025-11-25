/**
 * @file tca8418_driver.cpp
 * @brief TCA8418 I2C Keyboard Controller Driver for ESP-IDF
 * @version 0.2
 * @date 2025-11-09
 *
 * @copyright Copyright (c) 2025
 */
#include "tca8418_driver.h"
#include "esp_log.h"
#include <string.h>

#define TAG "TCA8418"
#define TCA8418_TIMEOUT_MS 1000
#define TCA8418_I2C_FREQ_HZ 400000

namespace KEYBOARD
{
    tca8418_driver::tca8418_driver(i2c_master_bus_handle_t bus_handle, uint8_t addr)
        : _bus_handle(bus_handle), _dev_handle(nullptr), _i2c_addr(addr), _initialized(false)
    {
        // Create device configuration
        i2c_device_config_t dev_cfg = {
            .dev_addr_length = I2C_ADDR_BIT_LEN_7,
            .device_address = addr,
            .scl_speed_hz = TCA8418_I2C_FREQ_HZ, // 400kHz
        };

        // Add device to bus
        esp_err_t ret = i2c_master_bus_add_device(_bus_handle, &dev_cfg, &_dev_handle);
        if (ret != ESP_OK)
        {
            ESP_LOGE(TAG, "Failed to add device to I2C bus: %s", esp_err_to_name(ret));
            _dev_handle = nullptr;
        }
    }

    tca8418_driver::~tca8418_driver()
    {
        if (_dev_handle != nullptr)
        {
            esp_err_t ret = i2c_master_bus_rm_device(_dev_handle);
            if (ret != ESP_OK)
            {
                ESP_LOGE(TAG, "Failed to remove TCA8418 device from I2C bus: %s", esp_err_to_name(ret));
            }
            _dev_handle = nullptr;
        }
    }

    bool tca8418_driver::write_register(uint8_t reg, uint8_t value)
    {
        if (_dev_handle == nullptr)
        {
            ESP_LOGE(TAG, "Device handle is null");
            return false;
        }

        uint8_t write_buf[2] = {reg, value};

        esp_err_t ret = i2c_master_transmit(_dev_handle, write_buf, 2, TCA8418_TIMEOUT_MS);

        if (ret != ESP_OK)
        {
            ESP_LOGE(TAG, "Failed to write register 0x%02X: %s", reg, esp_err_to_name(ret));
            return false;
        }
        return true;
    }

    bool tca8418_driver::read_register(uint8_t reg, uint8_t* value)
    {
        if (_dev_handle == nullptr)
        {
            ESP_LOGE(TAG, "Device handle is null");
            return false;
        }

        if (value == nullptr)
        {
            ESP_LOGE(TAG, "Value pointer is null");
            return false;
        }

        esp_err_t ret = i2c_master_transmit_receive(_dev_handle, &reg, 1, value, 1, TCA8418_TIMEOUT_MS);

        if (ret != ESP_OK)
        {
            ESP_LOGE(TAG, "Failed to read register 0x%02X: %s", reg, esp_err_to_name(ret));
            return false;
        }
        return true;
    }

    bool tca8418_driver::begin()
    {
        // Register initialization table: {register, value}
        struct RegValue
        {
            uint8_t reg;
            uint8_t value;
        };

        const RegValue init_table[] = {
            // GPIO - set default all GPIO pins to INPUT
            {TCA8418_REG_GPIO_DIR_1, 0x00},
            {TCA8418_REG_GPIO_DIR_2, 0x00},
            {TCA8418_REG_GPIO_DIR_3, 0x00},
            // Add all pins to key events
            {TCA8418_REG_GPI_EM_1, 0xFF},
            {TCA8418_REG_GPI_EM_2, 0xFF},
            {TCA8418_REG_GPI_EM_3, 0xFF},
            // Set all pins to FALLING interrupts
            {TCA8418_REG_GPIO_INT_LVL_1, 0x00},
            {TCA8418_REG_GPIO_INT_LVL_2, 0x00},
            {TCA8418_REG_GPIO_INT_LVL_3, 0x00},
            // Add all pins to interrupts
            {TCA8418_REG_GPIO_INT_EN_1, 0xFF},
            {TCA8418_REG_GPIO_INT_EN_2, 0xFF},
            {TCA8418_REG_GPIO_INT_EN_3, 0xFF},
        };

        bool ret = true;
        for (const auto& entry : init_table)
        {
            if (!write_register(entry.reg, entry.value))
            {
                ret = false;
                break;
            }
        }

        if (ret)
        {
            _initialized = true;
            ESP_LOGI(TAG, "Initialized successfully at address 0x%02X", _i2c_addr);
        }
        else
        {
            ESP_LOGE(TAG, "Initialization failed at address 0x%02X", _i2c_addr);
        }

        return ret;
    }

    void tca8418_driver::set_matrix(uint8_t rows, uint8_t cols)
    {
        if (!_initialized)
            return;
        if (rows > 8 || cols > 10)
            return;

        // Configure KP_GPIO registers to enable matrix pins
        // Setup rows (R0-R7) - build mask from LSB
        uint8_t mask = 0x00;
        for (int r = 0; r < rows; r++)
        {
            mask = (mask << 1) | 1;
        }
        write_register(TCA8418_REG_KP_GPIO_1, mask);

        // Setup columns (C0-C7) - build mask from LSB
        mask = 0x00;
        for (int c = 0; c < cols && c < 8; c++)
        {
            mask = (mask << 1) | 1;
        }
        write_register(TCA8418_REG_KP_GPIO_2, mask);

        // Setup columns C8-C9 if needed
        if (cols > 8)
        {
            mask = (cols == 9) ? 0x01 : 0x03;
            write_register(TCA8418_REG_KP_GPIO_3, mask);
        }
        else
        {
            write_register(TCA8418_REG_KP_GPIO_3, 0x00);
        }
    }

    void tca8418_driver::enable_interrupts()
    {
        if (!_initialized)
            return;

        // Enable key event + GPIO interrupts
        uint8_t value;
        read_register(TCA8418_REG_CFG, &value);
        value |= (TCA8418_REG_CFG_GPI_IEN | TCA8418_REG_CFG_KE_IEN);
        write_register(TCA8418_REG_CFG, value);
    }

    void tca8418_driver::disable_interrupts()
    {
        if (!_initialized)
            return;

        // Disable key events + GPIO interrupts
        uint8_t value;
        read_register(TCA8418_REG_CFG, &value);
        value &= ~(TCA8418_REG_CFG_GPI_IEN | TCA8418_REG_CFG_KE_IEN);
        write_register(TCA8418_REG_CFG, value);
    }

    void tca8418_driver::enable_matrix_overflow()
    {
        if (!_initialized)
            return;

        uint8_t value;
        read_register(TCA8418_REG_CFG, &value);
        value |= TCA8418_REG_CFG_OVR_FLOW_M;
        write_register(TCA8418_REG_CFG, value);
    }

    void tca8418_driver::disable_matrix_overflow()
    {
        if (!_initialized)
            return;

        uint8_t value;
        read_register(TCA8418_REG_CFG, &value);
        value &= ~TCA8418_REG_CFG_OVR_FLOW_M;
        write_register(TCA8418_REG_CFG, value);
    }

    void tca8418_driver::enable_debounce()
    {
        if (!_initialized)
            return;

        write_register(TCA8418_REG_DEBOUNCE_DIS_1, 0x00);
        write_register(TCA8418_REG_DEBOUNCE_DIS_2, 0x00);
        write_register(TCA8418_REG_DEBOUNCE_DIS_3, 0x00);
    }

    void tca8418_driver::disable_debounce()
    {
        if (!_initialized)
            return;

        write_register(TCA8418_REG_DEBOUNCE_DIS_1, 0xFF);
        write_register(TCA8418_REG_DEBOUNCE_DIS_2, 0xFF);
        write_register(TCA8418_REG_DEBOUNCE_DIS_3, 0xFF);
    }

    void tca8418_driver::flush()
    {
        if (!_initialized)
            return;

        // Flush key events - read until we get 0
        uint8_t count = 0;
        while (get_event() != 0)
            count++;

        // Flush GPIO events
        uint8_t dummy;
        read_register(TCA8418_REG_GPIO_INT_STAT_1, &dummy);
        read_register(TCA8418_REG_GPIO_INT_STAT_2, &dummy);
        read_register(TCA8418_REG_GPIO_INT_STAT_3, &dummy);

        // Clear INT_STAT register
        write_register(TCA8418_REG_INT_STAT, 3);
    }

    uint8_t tca8418_driver::available()
    {
        if (!_initialized)
            return 0;

        uint8_t available = 0;

        return read_register(TCA8418_REG_KEY_LCK_EC, &available) ? available & 0x0F : 0;
    }

    uint8_t tca8418_driver::get_event()
    {
        if (!_initialized)
            return 0;

        uint8_t event = 0;
        read_register(TCA8418_REG_KEY_EVENT_A, &event);
        return event;
    }

} // namespace KEYBOARD
