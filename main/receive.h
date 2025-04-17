#include <stdio.h>
#include <string.h>
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

extern spi_device_handle_t spi;

// 定义 nRF24L01 的 GPIO 引脚
#define NRF_CE_PIN 37
#define NRF_CSN_PIN 38


#define SPI_HOST SPI2_HOST  // 使用 HSPI_HOST 或 VSPI_HOST
#define SPI_CLOCK_SPEED 1000000  // SPI 时钟速度（1MHz）

// nRF24L01 寄存器地址
#define NRF_REG_CONFIG      0x00
#define NRF_REG_EN_AA       0x01
#define NRF_REG_EN_RXADDR   0x02
#define NRF_REG_SETUP_AW    0x03
#define NRF_REG_SETUP_RETR  0x04
#define NRF_REG_RF_CH       0x05
#define NRF_REG_RF_SETUP    0x06
#define NRF_REG_STATUS      0x07
#define NRF_REG_RX_ADDR_P0  0x0A
#define NRF_REG_TX_ADDR     0x10
#define NRF_REG_RX_PW_P0    0x11
#define NRF_REG_FIFO_STATUS 0x17
#define NRF_REG_FEATURE     0x1D
#define NRF_REG_DYNPD       0x1C



// nRF24L01 指令
#define NRF_CMD_R_REGISTER  0x00
#define NRF_CMD_W_REGISTER  0x20
#define NRF_CMD_R_RX_PAYLOAD 0x61
#define NRF_CMD_W_TX_PAYLOAD  0xA0
#define NRF_CMD_FLUSH_RX    0xE2
#define NRF_CMD_FLUSH_TX  0xE1
#define NRF_CMD_ACTIVATE 0x50
#define NRF_CMD_R_RX_PL_WID 0x60


// 初始化 CE 和 CSN 引脚
void nrf_gpio_init();

// SPI 写寄存器
void nrf_write_register(uint8_t reg, uint8_t value);

// SPI 写多字节数据（如地址）
void nrf_write_register_multi(uint8_t reg, uint8_t *data, size_t length) ;

// SPI 读寄存器
uint8_t nrf_read_register(uint8_t reg);

// SPI 读取 RX PAYLOAD（接收数据）
void nrf_read_payload(uint8_t *data, size_t length);

// 初始化 SPI
void spi_init();

// 初始化 nRF24L01 为接收模式
void nrf_init();

// 检查是否有数据可读
int nrf_data_ready() ;

// 清除中断标志
void nrf_clear_interrupts();

void nrf_read_register_multi(uint8_t reg, uint8_t *data, size_t length);

void nrf_check_configuration(void);

// 主任务
