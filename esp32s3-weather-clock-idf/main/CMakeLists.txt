idf_component_register(SRCS "main.c" 
                                   "led.c"
                                   "wifi_manager.c"
                                   "weather_client.c"
                                   "ntp_client.c"
                                   "gzip_decompressor.c"
                                   "display_manager.c"
                                   "system_init.c"
                                   "task_manager.c"
                                   "error_handler.c"
                            INCLUDE_DIRS "."
                            REQUIRES driver 
                                      nvs_flash
                                      BSP
                                      esp_wifi
                                      esp_event
                                      esp_http_client
                                      json
                                      mbedtls
                                      zlib)
