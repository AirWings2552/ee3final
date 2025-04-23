#include "receive.h"
#include "send.h"

void nrf_activate(void) {
    uint8_t tx_data[2] = { 0x50, 0x73 }; // 0x50: 激活命令，0x73: 激活数据
    spi_transaction_t t = {
        .length = 16,
        .tx_buffer = tx_data,
    };
    gpio_set_level(NRF_CSN_PIN, 0); // 拉低 CSN 开始通信
    spi_device_transmit(spi, &t);
    gpio_set_level(NRF_CSN_PIN, 1); // 拉高 CSN 结束通信
    esp_rom_delay_us(15);
}

uint8_t nrf_read_ackPayload_len(void) {
    uint8_t tx_data[2] = {NRF_CMD_R_RX_PL_WID,0xFF};
    uint8_t rx_data[2] = {0};
    spi_transaction_t t = {
        .length = 16,
        .tx_buffer = tx_data,
        .rx_buffer = rx_data
    };
    gpio_set_level(NRF_CSN_PIN, 0); // 拉低 CSN 开始通信
    spi_device_transmit(spi, &t);
    gpio_set_level(NRF_CSN_PIN, 1); // 拉高 CSN 结束通信
    esp_rom_delay_us(15);
    return rx_data[1];
}


// 初始化 nRF24L01 为接收模式
void nrf_init_sender() {
    nrf_gpio_init(); // 初始化 GPIO
    spi_init();      // 初始化 SPI

    nrf_activate();
    
    nrf_write_register(NRF_REG_FEATURE, 0x06); //ackPayload
    nrf_write_register(NRF_REG_DYNPD, 0x01);
    // 配置 nRF24L01 的寄存器
    nrf_write_register(NRF_REG_CONFIG, 0x0E);   // 上电，接收模式，启用 CRC
    nrf_write_register(NRF_REG_RF_SETUP, 0x06); // 配置 RF 输出功率和数据速率
    nrf_write_register(NRF_REG_RF_CH, 0x6A);    // 设置 RF 频道
    uint8_t rx_address[5] = { 0x00, 0x00, 0x00, 0x00, 0x02 }; // Be the same for auto retransmit
    uint8_t tx_address[5] = { 0x00, 0x00, 0x00, 0x00, 0x02 };// Be the same for auto retransmit
    nrf_write_register(NRF_REG_EN_RXADDR,0x03);
    nrf_write_register(NRF_REG_SETUP_AW,0x03);
    nrf_write_register_multi(NRF_REG_RX_ADDR_P0, rx_address, 5);
    nrf_write_register_multi(NRF_REG_TX_ADDR,tx_address,5);
    // nrf_write_register(NRF_REG_RX_PW_P0, 0x20); // 设置接收数据宽度为 32 字节
    nrf_write_register(NRF_REG_EN_AA, 0x01);    // 启用自动应答
    nrf_write_register(NRF_REG_SETUP_RETR, 0x26);
}

void nrf_send_payload(uint8_t *data, size_t length){
    uint8_t cmd = NRF_CMD_W_TX_PAYLOAD;
    spi_transaction_t t = {
        .length = 8,
        .tx_buffer = &cmd,
    };
    gpio_set_level(NRF_CSN_PIN, 0); // 拉低 CSN 开始通信
    spi_device_transmit(spi, &t);  // 发送读 PAYLOAD 指令
    t.length = length * 8;
    t.tx_buffer = data;
    t.rx_buffer = NULL;
    spi_device_transmit(spi, &t);  // 接收数据
    gpio_set_level(NRF_CSN_PIN, 1); // 拉高 CSN 结束通信
    esp_rom_delay_us(15);
}