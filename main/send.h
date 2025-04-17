#include <stdio.h>
#include <string.h>
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

void nrf_init_sender();
void nrf_send_payload(uint8_t *data, size_t length);
void nrf_activate(void);
uint8_t nrf_read_ackPayload_len(void);