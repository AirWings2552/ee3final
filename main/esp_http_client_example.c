#include <string.h>
#include <sys/param.h>
#include <stdlib.h>
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "protocol_examples_common.h"
#include "esp_tls.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_http_client.h"
#include <cJSON.h>
#include <stdio.h>

#if CONFIG_MBEDTLS_CERTIFICATE_BUNDLE
#include "esp_crt_bundle.h"
#endif

#define MAX_HTTP_RECV_BUFFER 512
#define MAX_HTTP_OUTPUT_BUFFER 4096

static const char *TAG = "HTTPS_TEST";

// Global buffer for GET request
static char *output_buffer = NULL;
static int output_len = 0;

// Global buffer for POST request
static char *post_output_buffer = NULL;
static int post_output_len = 0;


extern int is_post_request_sent;
extern char global_sensor_data[32];
extern int data_changed;

/* HTTP event handler (GET request) */
esp_err_t _http_event_handler(esp_http_client_event_t *evt)
{
    switch(evt->event_id) {
        case HTTP_EVENT_ERROR:
            ESP_LOGD(TAG, "HTTP_EVENT_ERROR");
            break;
        case HTTP_EVENT_ON_CONNECTED:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_CONNECTED");
            break;
        case HTTP_EVENT_HEADER_SENT:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_HEADER_SENT");
            break;
        case HTTP_EVENT_ON_HEADER:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_HEADER, key=%s, value=%s", evt->header_key, evt->header_value);
            break;
        case HTTP_EVENT_ON_DATA:
            ESP_LOGI(TAG, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
            if (output_len == 0 && output_buffer) {
                memset(output_buffer, 0, MAX_HTTP_OUTPUT_BUFFER);
            }
            if (output_len + evt->data_len < MAX_HTTP_OUTPUT_BUFFER) {
                memcpy(output_buffer + output_len, evt->data, evt->data_len);
                output_len += evt->data_len;
            } else {
                ESP_LOGW(TAG, "Output buffer overflow! Data may be truncated.");
            }
            break;
        case HTTP_EVENT_ON_FINISH:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_FINISH");
            if (output_len > 0) {
                output_buffer[output_len] = '\0';
                ESP_LOGI(TAG, "GET Final Response: %s", output_buffer);
            } else {
                ESP_LOGE(TAG, "No data received from server (GET)!");
            }
            output_len = 0;
            break;
        case HTTP_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "HTTP_EVENT_DISCONNECTED");
            break;
        default:
            ESP_LOGW(TAG, "Unhandled event: %d", evt->event_id);
            break;
    }
    return ESP_OK;
}

/* HTTP event handler (POST request) */
esp_err_t _http_post_event_handler(esp_http_client_event_t *evt)
{
    switch(evt->event_id) {
        case HTTP_EVENT_ERROR:
            ESP_LOGD(TAG, "POST HTTP_EVENT_ERROR");
            break;
        case HTTP_EVENT_ON_CONNECTED:
            ESP_LOGD(TAG, "POST HTTP_EVENT_ON_CONNECTED");
            break;
        case HTTP_EVENT_HEADER_SENT:
            ESP_LOGD(TAG, "POST HTTP_EVENT_HEADER_SENT");
            break;
        case HTTP_EVENT_ON_HEADER:
            ESP_LOGD(TAG, "POST HTTP_EVENT_ON_HEADER, key=%s, value=%s", evt->header_key, evt->header_value);
            break;
        case HTTP_EVENT_ON_DATA:
            ESP_LOGI(TAG, "POST HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
            if (post_output_len == 0 && post_output_buffer) {
                memset(post_output_buffer, 0, MAX_HTTP_OUTPUT_BUFFER);
            }
            if (post_output_len + evt->data_len < MAX_HTTP_OUTPUT_BUFFER) {
                memcpy(post_output_buffer + post_output_len, evt->data, evt->data_len);
                post_output_len += evt->data_len;
            } else {
                ESP_LOGW(TAG, "POST output buffer overflow! Data may be truncated.");
            }
            break;
        case HTTP_EVENT_ON_FINISH:
            ESP_LOGD(TAG, "POST HTTP_EVENT_ON_FINISH");
            if (post_output_len > 0) {
                post_output_buffer[post_output_len] = '\0';
                ESP_LOGI(TAG, "POST Final Response: %s", post_output_buffer);
            } else {
                ESP_LOGE(TAG, "No data received from server (POST)!");
            }
            post_output_len = 0;
            break;
        case HTTP_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "POST HTTP_EVENT_DISCONNECTED");
            break;
        default:
            ESP_LOGW(TAG, "POST Unhandled event: %d", evt->event_id);
            break;
    }
    return ESP_OK;
}

/* Initialize network */
esp_err_t init_network(void)
{
    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // Initialize network
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    ESP_ERROR_CHECK(example_connect());

    ESP_LOGI(TAG, "Network initialized successfully");
    return ESP_OK;
}

/* Send HTTPS GET request */
esp_err_t https_get_request(void)
{
    if (!output_buffer) {
        output_buffer = (char *) malloc(MAX_HTTP_OUTPUT_BUFFER);
        if (!output_buffer) {
            ESP_LOGE(TAG, "Failed to allocate memory for output buffer");
            return ESP_ERR_NO_MEM;
        }
        memset(output_buffer, 0, MAX_HTTP_OUTPUT_BUFFER);
    }

    esp_http_client_config_t config = {
        .url = "https://ujoniobvifxkbhhfopwu.supabase.co/rest/v1/room?apikey=eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiJzdXBhYmFzZSIsInJlZiI6InVqb25pb2J2aWZ4a2JoaGZvcHd1Iiwicm9sZSI6ImFub24iLCJpYXQiOjE3Mzk4ODU2NDEsImV4cCI6MjA1NTQ2MTY0MX0.aWD395KJsOXsEZKoV-xWx512ypVkSMkMPhFVOa7IYhc",
        .event_handler = _http_event_handler,
        .crt_bundle_attach = esp_crt_bundle_attach,
        .buffer_size = MAX_HTTP_RECV_BUFFER,
        .buffer_size_tx = 1024,
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);
    esp_http_client_set_header(client, "Accept", "application/json");

    esp_err_t err = esp_http_client_perform(client);
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "HTTP GET Status = %d, content_length = %" PRId64,
                 esp_http_client_get_status_code(client),
                 esp_http_client_get_content_length(client));
    } else {
        ESP_LOGE(TAG, "HTTP GET request failed: %s", esp_err_to_name(err));
    }
    esp_http_client_cleanup(client);
    return err;
}

/* Send HTTPS POST request */
esp_err_t https_post_request()
{
    if (!post_output_buffer) {
        post_output_buffer = (char *) malloc(MAX_HTTP_OUTPUT_BUFFER);
        if (!post_output_buffer) {
            ESP_LOGE(TAG, "Failed to allocate memory for POST output buffer");
            return ESP_ERR_NO_MEM;
        }
        memset(post_output_buffer, 0, MAX_HTTP_OUTPUT_BUFFER);
    }

    esp_http_client_config_t config = {
        .url = "https://ujoniobvifxkbhhfopwu.supabase.co/rest/v1/room",
        .event_handler = _http_post_event_handler,
        .crt_bundle_attach = esp_crt_bundle_attach,
        .buffer_size = MAX_HTTP_RECV_BUFFER,
        .buffer_size_tx = 1024,
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);

    // Set POST request headers and data
    esp_http_client_set_header(client, "Content-Type", "application/json");
    esp_http_client_set_header(client, "apikey", "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiJzdXBhYmFzZSIsInJlZiI6InVqb25pb2J2aWZ4a2JoaGZvcHd1Iiwicm9sZSI6ImFub24iLCJpYXQiOjE3Mzk4ODU2NDEsImV4cCI6MjA1NTQ2MTY0MX0.aWD395KJsOXsEZKoV-xWx512ypVkSMkMPhFVOa7IYhc");
    esp_http_client_set_header(client, "Authorization", "");
    esp_http_client_set_header(client, "Prefer", "resolution=merge-duplicates");
    
    // const char *post_data = "{\"id\":3}";
    // esp_http_client_set_method(client, HTTP_METHOD_POST);
    // esp_http_client_set_post_field(client, post_data, strlen(post_data));

    char post_data[128];
    float temp = global_sensor_data[2] + global_sensor_data[3] / 10.0f;
    float humi = global_sensor_data[0] + global_sensor_data[1] / 10.0f;
    snprintf(post_data, sizeof(post_data),"{\"room_id\":1,\"current_temp\":%.2f,\"current_humi\":%.2f}", temp,humi);
    printf("Data sent: %s\n",post_data);
    esp_http_client_set_method(client, HTTP_METHOD_POST);
    esp_http_client_set_post_field(client, post_data, strlen(post_data));

    esp_err_t err = esp_http_client_perform(client);
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "HTTP POST Status = %d, content_length = %" PRId64,
                 esp_http_client_get_status_code(client),
                 esp_http_client_get_content_length(client));
    } else {
        ESP_LOGE(TAG, "HTTP POST request failed: %s", esp_err_to_name(err));
    }
    esp_http_client_cleanup(client);
    return err;
}

/* GET request task */
void get_request_task(void *pvParameters)
{
    while (1) {
        ESP_LOGI(TAG, "Starting HTTPS GET request...");
        esp_err_t err = https_get_request();
        if (err == ESP_OK) {
            if (output_buffer != NULL && strlen(output_buffer) > 0) 
            {
                ESP_LOGI(TAG, "Parsing GET response: %s", output_buffer);
                cJSON *root = cJSON_Parse(output_buffer);
                if (root == NULL) {
                    ESP_LOGE(TAG, "Failed to parse JSON response: %s", cJSON_GetErrorPtr());
                } else {
                    if (cJSON_IsArray(root)) 
                    {
                        // 获取数组中第一个元素
                        cJSON *first_elem = cJSON_GetArrayItem(root, 0);
                        if (first_elem) {
                            cJSON *updated_item = cJSON_GetObjectItem(first_elem, "updated");
                            if (updated_item && cJSON_IsNumber(updated_item)) {
                                int updated_value = updated_item->valueint;
                                if (updated_value == 1) {
                                    ESP_LOGI(TAG, "Database updated flag is 1");
                                    data_changed = 1;  //
                                } else {
                                    ESP_LOGI(TAG, "Database updated flag is 0");
                                    data_changed = 0;
                                }
                            } else {
                                ESP_LOGW(TAG, "No valid 'updated' field found in the first element");
                            }
                        } else {
                            ESP_LOGW(TAG, "JSON array is empty");
                        }
                        cJSON_Delete(root);
                    }
                } 
            }
        }else {
            ESP_LOGE(TAG, "HTTP GET request failed: %s", esp_err_to_name(err));
        }
        vTaskDelay(pdMS_TO_TICKS(10000));  // Wait for 10 seconds
    }
}

/* POST request task */
void post_request_task(void *pvParameters)
{
    while (1) {
        if(!is_post_request_sent){
            ESP_LOGI(TAG, "Starting HTTPS POST request...");
            https_post_request();
            is_post_request_sent = 1;
            // Wait for 10 seconds
        }
        vTaskDelay(pdMS_TO_TICKS(10000));
    }
}

