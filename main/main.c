#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "lcd_display_task.h"
#include "esp_http_client_example.h"
#include "receive.h"
#include "send.h"

#define SENDER 0
#define RECEIVER 1

volatile uint8_t nrf_mode = RECEIVER;
void nrf24ol_RX_init();
void nrf24ol_TX_init();

volatile int is_post_request_sent = 1;
volatile uint8_t global_sensor_data[4];
volatile int data_changed = 0;

void send_request_to_pic18(){
    // nrf_write_register(NRF_CMD_FLUSH_TX,0xFF);
    nrf_send_payload((uint8_t *)"Request for data",32);
    gpio_set_level(NRF_CE_PIN, 1);
    esp_rom_delay_us(15);
    gpio_set_level(NRF_CE_PIN, 0);
    // uint8_t status = nrf_read_register(NRF_REG_STATUS);
    // printf("STATUS: 0x%02X\n", status);
}

void send_command(){
    // nrf_write_register(NRF_CMD_FLUSH_TX,0xFF);
    nrf_send_payload((uint8_t *)"Command from 32",32);
    gpio_set_level(NRF_CE_PIN, 1);
    esp_rom_delay_us(15);
    gpio_set_level(NRF_CE_PIN, 0);
    printf("Sent command:\n");
    // nrf_clear_interrupts();
    uint8_t status = nrf_read_register(NRF_REG_STATUS);
    printf("STATUS: 0x%02X\n", status);
}

void nrf24ol_RX_init(){
    printf("Initializing nRF24L01 as receiver...\n");
    nrf_init();

    nrf_clear_interrupts();
    nrf_check_configuration();

    gpio_set_level(NRF_CE_PIN, 1); // 拉高 CE 开始接收
}

void nrf24ol_TX_init(){
    printf("Initializing nRF24L01 as sender...\n");
    nrf_init_sender();

    nrf_clear_interrupts();
    nrf_check_configuration();

    gpio_set_level(NRF_CE_PIN, 0);
    esp_rom_delay_us(100);
}


static const char *TAG = "main";

void app_main(void)
{
    ESP_LOGI(TAG, "Starting application...");
    
    // Initialize and start LCD display task
    // lcd_display_init();
    // lcd_display_start();
    
    // Initialize network
    // esp_err_t ret = init_network();
    // if (ret != ESP_OK) {
    //     ESP_LOGE(TAG, "Failed to initialize network");
    //     return;
    // }

    // Create GET request task
    // xTaskCreate(&get_request_task, "get_request_task", 8192, NULL, 5, NULL);
    // Create POST request task
    // xTaskCreate(&post_request_task, "post_request_task", 8192, NULL, 5, NULL);
    
    
    nrf24ol_RX_init(); // Initialize the nrf24 to TX mode to send a request to pic18 asking for data
    

    // Main loop - can be used for other tasks in the future
    while (1) {
        // Code for Update the status from the database
        // if there is no commands need to send to pic18, esp32 remains in RX mode
        // else if there is a command, initialize the nrf24ol to TX mode, send the command, then return to the RX mode

        // above function will be done in the request task;
        if(!data_changed){
            // send_request_to_pic18();
            
            // vTaskDelay(pdMS_TO_TICKS(100));

            uint8_t status = nrf_read_register(NRF_REG_STATUS);
            
            if (status & (1 << 6)) {
                // uint8_t len = nrf_read_ackPayload_len();
                uint8_t ackPayload[4] ={0};
                // ackPayload[10] = '\0';
                nrf_read_payload(ackPayload, 4);
                printf("Received ackPayload: %d %d %d %d\n",ackPayload[0],ackPayload[1],ackPayload[2],ackPayload[3]);//H,R,T
                // printf("Received ackPayload length: %d\n", len);
                nrf_write_register(NRF_CMD_FLUSH_RX,0xFF);
                nrf_write_register(NRF_REG_STATUS,0x40); // 清除中断标志
                for(int i=0;i<4;i++){
                    global_sensor_data[i] = ackPayload[i];
                }
                is_post_request_sent = 0;
                vTaskDelay(pdMS_TO_TICKS(3000)); // 1 second is for testing
            }
            status = nrf_read_register(NRF_REG_STATUS);
            if(status & (1 << 5)){
                nrf_write_register(NRF_REG_STATUS,0x20);
                printf("Sent request and received ack:\n");
            }
            status = nrf_read_register(NRF_REG_STATUS);
            if(status & (1 << 4)){
                nrf_write_register(NRF_REG_STATUS,0x10);
                nrf_write_register(NRF_CMD_FLUSH_TX,0xFF);
                printf("MAX RT:\n");
            }
        }else{
            send_command();
            data_changed = 0;
            // code for write the data changed flag back to database

        }
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
} 