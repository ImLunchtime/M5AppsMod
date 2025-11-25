#pragma once

#include <mooncake.h>
#include "hal.h"
#include "apps/utils/flash/ptable_tools.h"
#include "apps/utils/flash/flash_tools.h"
#include "apps/utils/theme/theme_define.h"
#include "apps/utils/anim/anim_define.h"
#include "apps/utils/icon/icon_define.h"
#include "apps/utils/anim/hl_text.h"
#include <vector>

#include "assets/fdisk_big.h"
#include "assets/fdisk_small.h"
#include "assets/app.h"
#include "assets/app_sel.h"
#include "assets/data.h"
#include "assets/data_sel.h"

namespace MOONCAKE
{
    namespace APPS
    {
        class AppFdisk : public APP_BASE
        {
        private:
            // App states
            enum AppState_t
            {
                state_browsing,      // Viewing partition list
                state_add_partition, // Adding new partition
                state_info,          // Info dialog
                state_erasing,       // Erasing partition
                state_error,         // Error state
                state_hex_view       // Hex/ASCII view of partition data
            };

            // Data structure for partition list item
            struct PartitionItem_t
            {
                std::string name;
                uint8_t type;
                uint8_t subtype;
                std::string subtype_str;
                uint32_t offset;
                uint32_t size;
                uint32_t flags;
                bool is_bootable;
            };

            // App data structure
            struct
            {
                HAL::Hal* hal = nullptr;
                AppState_t state = state_browsing;
                UTILS::FLASH_TOOLS::PartitionTable ptable;
                std::vector<PartitionItem_t> partition_list;
                uint32_t free_space;
                int selected_index = 0;
                int scroll_offset = 0;
                bool update_list = true;
                bool needs_reflash = false;
                std::string error_message;
                std::string confirm_message;
                UTILS::HL_TEXT::HLTextContext_t hint_hl_ctx;

                // Hex viewer state
                bool hex_view_ascii = false;
                uint32_t hex_view_offset = 0;
                uint32_t hex_view_size = 0;
                uint32_t hex_view_cursor = 0;
                uint32_t hex_view_lines_per_page = 0;
                bool hex_view_needs_update = true;
                // buffer for 16 lines of 16 bytes
                uint8_t hex_view_buffer[16 * 16];
            } _data;

            // UI methods
            bool _render_partition_list();
            void _render_scrollbar();
            void _handle_list_navigation();
            bool _show_confirmation_dialog(const std::string& message);
            void _show_error_dialog(const std::string& message);
            void _show_add_partition_dialog();
            void _show_info_dialog();
            void _show_erase_progress(int progress);
            static void _delete_progress_callback(int progress, const char* message, void* arg_cb);

            // Partition operations
            void _update_partition_list();
            void _add_partition();
            void _add_data_partition();
            void _delete_partition();
#if 0
            void _erase_partition();
#endif
            void _rename_partition();
            bool _is_system_partition(const PartitionItem_t& item);
            bool _changes_require_reflash();

            // Utility methods
            std::string _format_size(uint32_t size);
            std::string _format_offset(uint32_t offset);
            void _clear_screen();

            // Hex viewer methods
            void _init_hex_view();
            void _update_hex_view();
            void _handle_hex_view_navigation();
            bool _render_hex_view();
            bool _render_control_hint(const char* hint);

        public:
            // Override base class methods
            void onCreate() override;
            void onResume() override;
            void onRunning() override;
            void onDestroy() override;
        };
        class AppFdisk_Packer : public APP_PACKER_BASE
        {
            std::string getAppName() override { return "FDISK"; }
            std::string getAppDesc() override { return "Manage flash partitions: list, add, delete, rename"; }
            void* getAppIcon() override { return (void*)(new AppIcon_t(image_data_fdisk_big, nullptr)); }
            void* newApp() override { return new AppFdisk; }
            void deleteApp(void* app) override { delete (AppFdisk*)app; }
        };
    } // namespace APPS
} // namespace MOONCAKE