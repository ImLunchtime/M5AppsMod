idf_component_register(SRCS "flood.c"
                    PRIV_REQUIRES esp_wifi esp-now nvs_flash
                    INCLUDE_DIRS "include")
