idf_component_register(
    SRCS 
        "main.c"
        # "lcd_display_task.c"
        # "lvgl_demo_ui.c"
        # "room1_token.c"
        # "example1.c"
        "esp_http_client_example.c"
        "receive.c"
        "send.c"
    INCLUDE_DIRS 
        "."
    REQUIRES 
        driver
        esp_lcd
        lvgl
        esp_timer
        esp_http_client
        esp_https_ota
        nvs_flash
        esp_netif
        esp_event
        esp-tls
        protocol_examples_common
        json
)
