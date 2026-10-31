set(srcs "app_main.c"
                    "app_driver.c"
                    "ws2812_driver.c"
                    "ota_local.c")
set(include_dirs "include")
set(DEVELOPMENT_BOARD "board_esp32c3_devkitc.h")

idf_component_register(SRCS "${srcs}"
                    INCLUDE_DIRS "${include_dirs}"
                    REQUIRES esp_rainmaker app_wifi app_storage button
                             esp_driver_rmt esp_driver_gpio esp_https_ota console)

idf_build_set_property(COMPILE_DEFINITIONS "-DDEVELOPMENT_BOARD=\"${DEVELOPMENT_BOARD}\"" APPEND)
