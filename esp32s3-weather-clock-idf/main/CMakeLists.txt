idf_component_register(SRCS "main.c" 
                           "led.c"
                           "wifi_manager.c"
                           "ntp_client.c"
                           "weather_client.c"
                           "gzip_decompressor.c"
                           "lcd_display.c"
                    INCLUDE_DIRS "."
                    REQUIRES esp_wifi 
                              esp_netif 
                              esp_timer 
                              driver 
                              nvs_flash
                              json
                              esp_http_client
                              mbedtls
                              zlib)
