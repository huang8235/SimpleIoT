
#include "driver/spi_master.h"
#include "driver/i2c_master.h"
#include "esp_check.h"
#include "esp_log.h"

#include "esp32c3_simpleIoT_board.h"

#define I2C_MASTER_SCL_IO   CONFIG_I2C_MASTER_SCL   /*!< gpio number for I2C master clock */
#define I2C_MASTER_SDA_IO   CONFIG_I2C_MASTER_SDA   /*!< gpio number for I2C master data  */
#define I2C_MASTER_NUM      I2C_NUM_0               /*!< I2C port number for master dev */

#define AHT20_SLAVE_ADDRESS AHT20_ADDRRES_0

#define SPI_BUS_MOSI_IO     CONFIG_SPI_BUS_MOSI
#define SPI_BUS_SCLK_IO     CONFIG_SPI_BUS_SCLK
#define SPI_BUS_CLK_HZ      8000000

#define SSD1306_SPI_CS_IO   CONFIG_SSD1306_SPI_CS
#define SSD1306_DC_IO       CONFIG_SSD1306_DC
#define SSD1306_RESET_IO    CONFIG_SSD1306_RESET
#define SSD1306_OLED_WIDTH  128
#define SSD1306_OLED_HEIGHT 64

static const char *TAG = "esp32c3_simpleIoT_board";

static aht20_dev_handle_t aht20 = NULL;
static ssd1306_handle_t ssd1306 = NULL;


/* ================ I2C bus and device initialization ================ */
static i2c_master_bus_handle_t i2c_bus_handle;

static void i2c_master_init(void)
{
    i2c_master_bus_config_t i2c_mst_config = {
        .i2c_port = I2C_MASTER_NUM,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };

    ESP_ERROR_CHECK(i2c_new_master_bus(&i2c_mst_config, &i2c_bus_handle));
}

static void aht20_init(aht20_dev_handle_t *handle)
{
    ESP_ERROR_CHECK(aht20_new_sensor(i2c_bus_handle, AHT20_SLAVE_ADDRESS, handle));
}

/*  ================ SPI bus and device initialization ================ */
static void spi_bus_init(void)
{
    const spi_bus_config_t buscfg = {
        .mosi_io_num     = SPI_BUS_MOSI_IO,
        .miso_io_num     = -1,
        .sclk_io_num     = SPI_BUS_SCLK_IO,
        .quadwp_io_num   = -1,
        .quadhd_io_num   = -1,
        .max_transfer_sz = 0,
    };

    ESP_ERROR_CHECK(spi_bus_initialize(SPI2_HOST, &buscfg, SPI_DMA_CH_AUTO));
}

static void ssd1306_init(ssd1306_handle_t *handle)
{
    ssd1306_config_t cfg = {
        .bus    = SSD1306_SPI,
        .width  = SSD1306_OLED_WIDTH,
        .height = SSD1306_OLED_HEIGHT,
        .iface.spi =
            {
                .host     = SPI2_HOST,
                .cs_gpio  = SSD1306_SPI_CS_IO,     // Chip-select pin
                .dc_gpio  = SSD1306_DC_IO,         // Data/Command pin
                .rst_gpio = SSD1306_RESET_IO,      // Reset pin (GPIO_NUM_NC if tied high)
                .clk_hz   = SPI_BUS_CLK_HZ,        // 8 MHz
            },
        .fb     = NULL,                            // Let driver allocate framebuffer
        .fb_len = 0,
    };

    ESP_ERROR_CHECK(ssd1306_new_spi(&cfg, handle));
}

/*  ================ Exported functions ================ */
void board_init(void)
{
    i2c_master_init();
    spi_bus_init();

    aht20_init(&aht20);
    ssd1306_init(&ssd1306);

    ESP_LOGI(TAG, "Board initialized");
}

aht20_dev_handle_t board_get_aht20_handle(void)
{
    return aht20;
}

ssd1306_handle_t board_get_ssd1306_handle(void)
{
    return ssd1306;
}
