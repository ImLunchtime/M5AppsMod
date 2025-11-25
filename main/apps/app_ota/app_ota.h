#pragma once

#include <mooncake.h>
#include "../../hal/hal.h"
#include "../utils/theme/theme_define.h"
#include "../utils/anim/anim_define.h"
#include "../utils/icon/icon_define.h"
#include "../utils/anim/scroll_text.h"
#include "../utils/flash/flash_tools.h"
#include "../utils/flash/ptable_tools.h"
#include "esp_partition.h"
#include <string>

#include "assets/ota_big.h"
#include "assets/ota_small.h"

namespace MOONCAKE
{
    namespace APPS
    {
        /**
         * @brief App class for OTA partitions
         * Allows booting into different OTA partitions
         */
        class OtaApp : public APP_BASE
        {
        private:
            const esp_partition_t* _partition;

            // App data structure
            struct
            {
                HAL::Hal* hal = nullptr;
            } _data;

        public:
            OtaApp(const esp_partition_t* partition) : _partition(partition) {};
            void onCreate() override;
        };

        /**
         * @brief App packer for OTA apps
         * Creates apps for bootable OTA partitions
         */
        class OtaApp_Packer : public APP_PACKER_BASE
        {
        private:
            const esp_partition_t* _partition;
            std::string _name;

        public:
            OtaApp_Packer(const esp_partition_t* partition) : _partition(partition)
            {
                _name = std::string(((const char*)partition->label));
            };
            std::string getAppName() override { return _name; }
            std::string getAppDesc() override { return "App installed by user. To delete or rename use FDISK app"; }
            void* getAppIcon() override { return (void*)(new AppIcon_t(image_data_ota_big, nullptr)); }
            void* newApp() override { return new OtaApp(_partition); };
            void deleteApp(void* app) override { delete static_cast<OtaApp*>(app); }
        };
    } // namespace APPS
} // namespace MOONCAKE
