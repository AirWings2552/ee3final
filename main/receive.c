#include "receive.h"

// SPI 设备句柄
spi_device_handle_t spi;

// 初始化 CE 和 CSN 引脚
void nrf_gpio_init() {
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << NRF_CE_PIN) | (1ULL << NRF_CSN_PIN),
        .mode = GPIO_MODE_OUTPUT,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&io_conf);

    gpio_set_level(NRF_CE_PIN, 0);   // 默认拉低 CE
    gpio_set_level(NRF_CSN_PIN, 1); // 默认拉高 CSN
}

// SPI 写寄存器
void nrf_write_register(uint8_t reg, uint8_t value) {
    uint8_t tx_data[2] = { NRF_CMD_W_REGISTER | reg, value };
    spi_transaction_t t = {
        .length = 16,
        .tx_buffer = tx_data,
    };
    gpio_set_level(NRF_CSN_PIN, 0); // 拉低 CSN 开始通信
    spi_device_transmit(spi, &t);
    gpio_set_level(NRF_CSN_PIN, 1); // 拉高 CSN 结束通信
    esp_rom_delay_us(15);
}

// SPI 写多字节数据（如地址）
void nrf_write_register_multi(uint8_t reg, uint8_t *data, size_t length) {
    uint8_t tx_data[length + 1];
    tx_data[0] = NRF_CMD_W_REGISTER | reg;
    memcpy(&tx_data[1], data, length);
    spi_transaction_t t = {
        .length = (length + 1) * 8,
        .tx_buffer = tx_data,
    };
    gpio_set_level(NRF_CSN_PIN, 0); // 拉低 CSN 开始通信
    spi_device_transmit(spi, &t);
    gpio_set_level(NRF_CSN_PIN, 1); // 拉高 CSN 结束通信
    esp_rom_delay_us(15);
}

// SPI 读寄存器
uint8_t nrf_read_register(uint8_t reg) {
    uint8_t tx_data[2] = { NRF_CMD_R_REGISTER | reg, 0xFF };
    uint8_t rx_data[2] = {0};
    spi_transaction_t t = {
        .length = 16,
        .tx_buffer = tx_data,
        .rx_buffer = rx_data,
    };
    gpio_set_level(NRF_CSN_PIN, 0); // 拉低 CSN 开始通信
    spi_device_transmit(spi, &t);
    gpio_set_level(NRF_CSN_PIN, 1); // 拉高 CSN 结束通信
    esp_rom_delay_us(15);
    return rx_data[1];
}

// SPI 读取 RX PAYLOAD（接收数据）
void nrf_read_payload(uint8_t *data, size_t length) {
    uint8_t cmd = NRF_CMD_R_RX_PAYLOAD;
    spi_transaction_t t = {
        .length = 8,
        .tx_buffer = &cmd,
    };
    gpio_set_level(NRF_CSN_PIN, 0); // 拉低 CSN 开始通信
    spi_device_transmit(spi, &t);  // 发送读 PAYLOAD 指令
    t.length = length * 8;
    t.tx_buffer = NULL;
    t.rx_buffer = data;
    spi_device_transmit(spi, &t);  // 接收数据
    gpio_set_level(NRF_CSN_PIN, 1); // 拉高 CSN 结束通信
    esp_rom_delay_us(15);
}

// 初始化 SPI
void spi_init() {
    spi_bus_config_t buscfg = {
        .miso_io_num = GPIO_NUM_41, //miso gpio41
        .mosi_io_num = GPIO_NUM_39, //miso gpio39
        .sclk_io_num = GPIO_NUM_1, //sclk gpio1
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 32,
    };
    spi_device_interface_config_t devcfg = {
        .clock_speed_hz = SPI_CLOCK_SPEED,
        .mode = 0, // SPI 模式 0
        .spics_io_num = -1, // 手动控制 CSN
        .queue_size = 7,
    };

    // 初始化 SPI 总线
    spi_bus_initialize(SPI_HOST, &buscfg, SPI_DMA_CH_AUTO);
    spi_bus_add_device(SPI_HOST, &devcfg, &spi);
}

// 初始化 nRF24L01 为接收模式
void nrf_init() {
    nrf_gpio_init(); // 初始化 GPIO
    spi_init();      // 初始化 SPI

    // 配置 nRF24L01 的寄存器
    nrf_write_register(NRF_REG_CONFIG, 0x0F);   // 上电，接收模式，启用 CRC
    nrf_write_register(NRF_REG_RF_SETUP, 0x06); // 配置 RF 输出功率和数据速率
    nrf_write_register(NRF_REG_RF_CH, 0x6A);    // 设置 RF 频道
    uint8_t rx_address[5] = { 0x00, 0x00, 0x00, 0x00, 0x01 }; // RX 地址E7 C9 3F 03 00
    nrf_write_register_multi(NRF_REG_RX_ADDR_P0, rx_address, 5);
    
    nrf_write_register(NRF_REG_RX_PW_P0, 0x20); // 设置接收数据宽度为 32 字节
    nrf_write_register(NRF_REG_EN_AA, 0x00);    // 启用自动应答
}

// 检查是否有数据可读
int nrf_data_ready() {
    uint8_t status = nrf_read_register(NRF_REG_STATUS);
    return status & (1 << 6); // RX_DR 位
}

// 清除中断标志
void nrf_clear_interrupts() {
    nrf_write_register(NRF_REG_STATUS, 0x70); // 清除 RX_DR, TX_DS, MAX_RT 标志
}

void nrf_read_register_multi(uint8_t reg, uint8_t *data, size_t length) {
    uint8_t tx_data = NRF_CMD_R_REGISTER | reg; // 读寄存器命令和寄存器地址
    spi_transaction_t t = {
        .length = 8,           // 首先发送 1 字节的读寄存器命令
        .tx_buffer = &tx_data, // 发送的命令
        .rx_buffer = NULL
    };

    gpio_set_level(NRF_CSN_PIN, 0); // 拉低 CSN 开始通信
    spi_device_transmit(spi, &t);  // 发送读寄存器命令

    t.length = length * 8;  // 准备接收 length 字节的数据
    t.tx_buffer = NULL;     // 无需发送数据
    t.rx_buffer = data;     // 接收缓冲区
    spi_device_transmit(spi, &t);  // 接收数据
    gpio_set_level(NRF_CSN_PIN, 1); // 拉高 CSN 结束通信
}

void nrf_check_configuration(void) {
    printf("=== Checking nRF24L01 Configuration ===\n");

    // 读取 CONFIG 寄存器
    uint8_t config = nrf_read_register(NRF_REG_CONFIG);
    printf("CONFIG: 0x%02X\n", config);
    if ((config & 0x0F) != 0x0E && (config & 0x0F) != 0x0F) {
        printf("ERROR: CONFIG register is not properly configured (0x%02X)\n", config);
    } else {
        printf("CONFIG register is properly configured.\n");
    }

    // 读取 EN_AA 寄存器
    uint8_t en_aa = nrf_read_register(NRF_REG_EN_AA);
    printf("EN_AA: 0x%02X (Auto Acknowledgment)\n", en_aa);

    // 读取 RF_SETUP 寄存器
    uint8_t rf_setup = nrf_read_register(NRF_REG_RF_SETUP);
    printf("RF_SETUP: 0x%02X (RF settings)\n", rf_setup);

    // 读取 RF_CH 寄存器
    uint8_t rf_ch = nrf_read_register(NRF_REG_RF_CH);
    printf("RF_CH: 0x%02X (RF channel)\n", rf_ch);

    // 读取 STATUS 寄存器
    uint8_t status = nrf_read_register(NRF_REG_STATUS);
    printf("STATUS: 0x%02X\n", status);

    // 读取 RX_ADDR_P0 (接收地址)
    uint8_t rx_address[5];
    nrf_read_register_multi(NRF_REG_RX_ADDR_P0, rx_address, 5);
    printf("RX_ADDR_P0: ");
    for (int i = 0; i < 5; i++) {
        printf("%02X ", rx_address[i]);
    }
    printf("\n");

    // 读取 TX_ADDR (发送地址)
    uint8_t tx_address[5];
    nrf_read_register_multi(NRF_REG_TX_ADDR, tx_address, 5);
    printf("TX_ADDR: ");
    for (int i = 0; i < 5; i++) {
        printf("%02X ", tx_address[i]);
    }
    printf("\n");

    printf("=== Configuration Check Complete ===\n");
}

// 主任务
