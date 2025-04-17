#ifndef ESP_HTTP_CLIENT_EXAMPLE_H
#define ESP_HTTP_CLIENT_EXAMPLE_H

#include "esp_err.h"

// Network initialization function
esp_err_t init_network(void);

// HTTP request functions
esp_err_t https_get_request(void);
esp_err_t https_post_request(void);

// Task functions
void get_request_task(void *pvParameters);
void post_request_task(void *pvParameters);

#endif // ESP_HTTP_CLIENT_EXAMPLE_H 