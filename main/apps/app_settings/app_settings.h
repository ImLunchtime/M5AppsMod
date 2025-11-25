#pragma once

#include "../apps.h"
#include "nvs_flash.h"
#include <vector>
#include <string>

#include "../utils/theme/theme_define.h"
#include "../utils/anim/anim_define.h"
#include "../utils/icon/icon_define.h"
#include "../utils/anim/scroll_text.h"
#include "../utils/ui/dialog.h"
#include "settings/settings.h"

#include "assets/setting_big.h"
#include "assets/setting_small.h"

using namespace SETTINGS;
namespace MOONCAKE::APPS
{

    class AppSettings : public APP_BASE
    {
    private:
        struct
        {
            HAL::Hal* hal;
            std::vector<SettingGroup_t> groups;
            int selected_group;
            int selected_item;
            int scroll_offset;
            bool update_list;
            bool in_group;
            UTILS::SCROLL_TEXT::ScrollTextContext_t desc_scroll_ctx;
            UTILS::HL_TEXT::HLTextContext_t hint_hl_ctx;
        } _data;

        void _init_nvs();
        void _deinit_nvs();
        void _init_groups();
        void _load_settings();
        void _save_setting(const SettingGroup_t& group, const SettingItem_t& item);

        bool _render_groups();
        bool _render_items();
        bool _handle_group_selection();
        bool _handle_item_selection();
        bool _render_scrolling_desc();
        bool _render_control_hint(const char* hint);

        bool _edit_bool_setting(SettingItem_t& item);
        bool _edit_number_setting(SettingItem_t& item);
        bool _edit_string_setting(SettingItem_t& item);
        bool _edit_wifi_network_setting(SettingItem_t& item);

        void _show_edit_dialog(const std::string& title, SettingItem_t& item);

    public:
        void onCreate() override;
        void onResume() override;
        void onRunning() override;
        void onDestroy() override;
    };

    class AppSettings_Packer : public APP_PACKER_BASE
    {
        std::string getAppName() override { return "SETTINGS"; }
        std::string getAppDesc() override { return "Configure system settings"; }
        void* getAppIcon() override { return (void*)(new AppIcon_t(image_data_setting_big, nullptr)); }
        void* newApp() override { return new AppSettings; }
        void deleteApp(void* app) override { delete (AppSettings*)app; }
    };

} // namespace MOONCAKE::APPS