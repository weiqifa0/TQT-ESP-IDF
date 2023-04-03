/**
 * @file ili9341.c
 *
 */

/*********************
 *      INCLUDES
 *********************/
#include "ili9341.h"
#include "disp_spi.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

/*********************
 *      DEFINES
 *********************/
 #define TAG "ILI9341"

/**********************
 *      TYPEDEFS
 **********************/

/*The LCD needs a bunch of command/argument values to be initialized. They are stored in this struct. */
typedef struct {
    uint8_t cmd;
    uint8_t data[16];
    uint8_t databytes; //No of data in data; bit 7 = delay after set; 0xFF = end of cmds.
} lcd_init_cmd_t;

/**********************
 *  STATIC PROTOTYPES
 **********************/
static void ili9341_set_orientation(uint8_t orientation);

static void ili9341_send_cmd(uint8_t cmd);
static void ili9341_send_data(void * data, uint16_t length);
static void ili9341_send_data_8bits(uint8_t data);
static void ili9341_send_color(void * data, uint16_t length);
static void ili9341_send_color_8bits(uint8_t data);
/**********************
 *  STATIC VARIABLES
 **********************/

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

void reg_init(void)
{
  ili9341_send_cmd(0xEF);
  ili9341_send_data_8bits(0x03);
  ili9341_send_data_8bits(0x80);
  ili9341_send_data_8bits(0x02);

  ili9341_send_cmd(0xCF);
  ili9341_send_data_8bits(0x00);
  ili9341_send_data_8bits(0XC1);
  ili9341_send_data_8bits(0X30);

  ili9341_send_cmd(0xED);
  ili9341_send_data_8bits(0x64);
  ili9341_send_data_8bits(0x03);
  ili9341_send_data_8bits(0X12);
  ili9341_send_data_8bits(0X81);

  ili9341_send_cmd(0xE8);
  ili9341_send_data_8bits(0x85);
  ili9341_send_data_8bits(0x00);
  ili9341_send_data_8bits(0x78);

  ili9341_send_cmd(0xCB);
  ili9341_send_data_8bits(0x39);
  ili9341_send_data_8bits(0x2C);
  ili9341_send_data_8bits(0x00);
  ili9341_send_data_8bits(0x34);
  ili9341_send_data_8bits(0x02);

  ili9341_send_cmd(0xF7);
  ili9341_send_data_8bits(0x20);

  ili9341_send_cmd(0xEA);
  ili9341_send_data_8bits(0x00);
  ili9341_send_data_8bits(0x00);

  ili9341_send_cmd(ILI9341_PWCTR1);    //Power control
  ili9341_send_data_8bits(0x23);   //VRH[5:0]

  ili9341_send_cmd(ILI9341_PWCTR2);    //Power control
  ili9341_send_data_8bits(0x10);   //SAP[2:0];BT[3:0]

  ili9341_send_cmd(ILI9341_VMCTR1);    //VCM control
  ili9341_send_data_8bits(0x3e);
  ili9341_send_data_8bits(0x28);

  ili9341_send_cmd(ILI9341_VMCTR2);    //VCM control2
  ili9341_send_data_8bits(0x86);  //--

  ili9341_send_cmd(ILI9341_MADCTL);    // Memory Access Control
#ifdef M5STACK
  ili9341_send_data_8bits(TFT_MAD_MY | TFT_MAD_MV | TFT_MAD_COLOR_ORDER); // Rotation 0 (portrait mode)
#else
  ili9341_send_data_8bits(TFT_MAD_MX | TFT_MAD_COLOR_ORDER); // Rotation 0 (portrait mode)
#endif

  ili9341_send_cmd(ILI9341_PIXFMT);
  ili9341_send_data_8bits(0x55);

  ili9341_send_cmd(ILI9341_FRMCTR1);
  ili9341_send_data_8bits(0x00);
  ili9341_send_data_8bits(0x13); // 0x18 79Hz, 0x1B default 70Hz, 0x13 100Hz

  ili9341_send_cmd(ILI9341_DFUNCTR);    // Display Function Control
  ili9341_send_data_8bits(0x08);
  ili9341_send_data_8bits(0x82);
  ili9341_send_data_8bits(0x27);

  ili9341_send_cmd(0xF2);    // 3Gamma Function Disable
  ili9341_send_data_8bits(0x00);

  ili9341_send_cmd(ILI9341_GAMMASET);    //Gamma curve selected
  ili9341_send_data_8bits(0x01);

  ili9341_send_cmd(ILI9341_GMCTRP1);    //Set Gamma
  ili9341_send_data_8bits(0x0F);
  ili9341_send_data_8bits(0x31);
  ili9341_send_data_8bits(0x2B);
  ili9341_send_data_8bits(0x0C);
  ili9341_send_data_8bits(0x0E);
  ili9341_send_data_8bits(0x08);
  ili9341_send_data_8bits(0x4E);
  ili9341_send_data_8bits(0xF1);
  ili9341_send_data_8bits(0x37);
  ili9341_send_data_8bits(0x07);
  ili9341_send_data_8bits(0x10);
  ili9341_send_data_8bits(0x03);
  ili9341_send_data_8bits(0x0E);
  ili9341_send_data_8bits(0x09);
  ili9341_send_data_8bits(0x00);

  ili9341_send_cmd(ILI9341_GMCTRN1);    //Set Gamma
  ili9341_send_data_8bits(0x00);
  ili9341_send_data_8bits(0x0E);
  ili9341_send_data_8bits(0x14);
  ili9341_send_data_8bits(0x03);
  ili9341_send_data_8bits(0x11);
  ili9341_send_data_8bits(0x07);
  ili9341_send_data_8bits(0x31);
  ili9341_send_data_8bits(0xC1);
  ili9341_send_data_8bits(0x48);
  ili9341_send_data_8bits(0x08);
  ili9341_send_data_8bits(0x0F);
  ili9341_send_data_8bits(0x0C);
  ili9341_send_data_8bits(0x31);
  ili9341_send_data_8bits(0x36);
  ili9341_send_data_8bits(0x0F);

  ili9341_send_cmd(ILI9341_SLPOUT);    //Exit Sleep

  //end_tft_write();
  //delay(120);
  //begin_tft_write();
  vTaskDelay(120 / portTICK_RATE_MS);

  ili9341_send_cmd(ILI9341_DISPON);    //Display on
}


static void lcd_set_windows(uint16_t xStar,uint16_t yStar,uint16_t xWidth,uint16_t yHeight)
{
  int i, j;
  ili9341_send_cmd(ILI9341_CASET);		//发送设置X轴坐标的命令0x2A
  //参数SC[15:0]	->	设置起始列地址，也就是设置X轴起始坐标
  ili9341_send_data_8bits(xStar>>8);				//先写高8位
  ili9341_send_data_8bits(xStar&0x00FF);			//再写低8位
  //参数EC[15:0]	->	设置窗口X轴结束的列地址，因为参数xWidth是窗口长度，所以要转为列地址再发送
  ili9341_send_data_8bits((xStar+xWidth-1)>>8);				//先写高8位
  ili9341_send_data_8bits((xStar+xWidth-1)&0x00FF);			//再写低8位

  ili9341_send_cmd(ILI9341_PASET);		//发送设置Y轴坐标的命令0x2B
  //参数SP[15:0]	->	设置起始行地址，也就是设置Y轴起始坐标
  ili9341_send_data_8bits(yStar>>8);				//先写高8位
  ili9341_send_data_8bits(yStar&0x00FF);			//再写低8位
  //参数EP[15:0]	->	设置窗口Y轴结束的列地址，因为参数yHeight是窗口高度，所以要转为行地址再发送
  ili9341_send_data_8bits((yStar+yHeight-1)>>8);				//先写高8位
  ili9341_send_data_8bits((yStar+yHeight-1)&0x00FF);			//再写低8位

  ili9341_send_cmd(ILI9341_RAMWR);			//开始往GRAM里写数据，写GRAM的指令为0x2C

  for(i = xStar; i < (xStar+xWidth); i++)
  {
    for(j = 0; j < (yStar+yHeight); j++)
    {
      ili9341_send_data_8bits(0xF8);
    }
  }
}

void ili9341_init(void)
{
  //Initialize non-SPI GPIOs
  gpio_pad_select_gpio(ILI9341_DC);
  gpio_set_direction(ILI9341_DC, GPIO_MODE_OUTPUT);

  gpio_pad_select_gpio(CONFIG_LV_DISP_SPI_MOSI);
  gpio_set_direction(CONFIG_LV_DISP_SPI_MOSI, GPIO_MODE_OUTPUT);

  gpio_pad_select_gpio(CONFIG_LV_DISP_SPI_CLK);
  gpio_set_direction(CONFIG_LV_DISP_SPI_CLK, GPIO_MODE_OUTPUT);

  gpio_pad_select_gpio(CONFIG_LV_DISP_SPI_CS);
  gpio_set_direction(CONFIG_LV_DISP_SPI_CS, GPIO_MODE_OUTPUT);

#if ILI9341_USE_RST
  gpio_pad_select_gpio(ILI9341_RST);
  gpio_set_direction(ILI9341_RST, GPIO_MODE_OUTPUT);

  //Reset the display
  gpio_set_level(ILI9341_RST, 0);
  vTaskDelay(100 / portTICK_RATE_MS);
  gpio_set_level(ILI9341_RST, 1);
  vTaskDelay(100 / portTICK_RATE_MS);
#endif

  ESP_LOGI(TAG, "Initialization.");
  reg_init();

  ili9341_set_orientation(CONFIG_LV_DISPLAY_ORIENTATION);

  ili9341_send_cmd(ILI9341_DISPOFF);
  ili9341_send_cmd(ILI9341_DISPON);

  lcd_set_windows(0,0,256,256);
}


void ili9341_flush(lv_disp_drv_t * drv, const lv_area_t * area, lv_color_t * color_map)
{
	uint8_t data[4];

	/*Column addresses*/
	ili9341_send_cmd(0x2A);
	data[0] = (area->x1 >> 8) & 0xFF;
	data[1] = area->x1 & 0xFF;
	data[2] = (area->x2 >> 8) & 0xFF;
	data[3] = area->x2 & 0xFF;
	ili9341_send_data(data, 4);

	/*Page addresses*/
	ili9341_send_cmd(0x2B);
	data[0] = (area->y1 >> 8) & 0xFF;
	data[1] = area->y1 & 0xFF;
	data[2] = (area->y2 >> 8) & 0xFF;
	data[3] = area->y2 & 0xFF;
	ili9341_send_data(data, 4);

	/*Memory write*/
	ili9341_send_cmd(0x2C);
	uint32_t size = lv_area_get_width(area) * lv_area_get_height(area);
	ili9341_send_color((void*)color_map, size * 2);
}

void ili9341_sleep_in()
{
	uint8_t data[] = {0x08};
	ili9341_send_cmd(0x10);
	ili9341_send_data(&data, 1);
}

void ili9341_sleep_out()
{
	uint8_t data[] = {0x08};
	ili9341_send_cmd(0x11);
	ili9341_send_data(&data, 1);
}

/**********************
 *   STATIC FUNCTIONS
 **********************/


static void spi_send_8bits(uint8_t data)
{
    for (int i = 0; i < 8; i++) {
        gpio_set_level(CONFIG_LV_DISP_SPI_CLK, 0);
        gpio_set_level(CONFIG_LV_DISP_SPI_MOSI, data & 0x80);
        data <<= 1;
        gpio_set_level(CONFIG_LV_DISP_SPI_CLK, 1);
    }
}

static void ili9341_send_cmd(uint8_t cmd)
{
    //disp_wait_for_pending_transactions();
    gpio_set_level(ILI9341_DC, 0);	 /*Command mode*/
    //disp_spi_send_data(&cmd, 1);
    spi_send_8bits(cmd);
}

static void ili9341_send_data(void * data, uint16_t length)
{
    disp_wait_for_pending_transactions();
    gpio_set_level(ILI9341_DC, 1);	 /*Data mode*/
    //disp_spi_send_data(data, length);
    spi_send_8bits(data);
}

static void ili9341_send_data_8bits(uint8_t data)
{
    //disp_wait_for_pending_transactions();
    gpio_set_level(ILI9341_DC, 1);	 /*Data mode*/
    //disp_spi_send_data(data, length);
    spi_send_8bits(data);
}

static void ili9341_send_color(void * data, uint16_t length)
{
    disp_wait_for_pending_transactions();
    gpio_set_level(ILI9341_DC, 1);   /*Data mode*/
    //disp_spi_send_colors(data, length);
    spi_send_8bits(data);
}

static void ili9341_send_color_8bits(uint8_t data)
{
    //disp_wait_for_pending_transactions();
    gpio_set_level(ILI9341_DC, 1);   /*Data mode*/
    //disp_spi_send_colors(data, length);
    spi_send_8bits(data);
}

static void ili9341_set_orientation(uint8_t orientation)
{
    // ESP_ASSERT(orientation < 4);

    const char *orientation_str[] = {
        "PORTRAIT", "PORTRAIT_INVERTED", "LANDSCAPE", "LANDSCAPE_INVERTED"
    };

    ESP_LOGI(TAG, "Display orientation: %s", orientation_str[orientation]);

#if defined CONFIG_LV_PREDEFINED_DISPLAY_M5STACK
    uint8_t data[] = {0x68, 0x68, 0x08, 0x08};
#elif defined (CONFIG_LV_PREDEFINED_DISPLAY_M5CORE2)
	uint8_t data[] = {0x08, 0x88, 0x28, 0xE8};
#elif defined (CONFIG_LV_PREDEFINED_DISPLAY_WROVER4)
    uint8_t data[] = {0x6C, 0xEC, 0xCC, 0x4C};
#elif defined (CONFIG_LV_PREDEFINED_DISPLAY_NONE)
    uint8_t data[] = {0x48, 0x88, 0x28, 0xE8};
#endif

    ESP_LOGI(TAG, "0x36 command value: 0x%02X", data[orientation]);

    ili9341_send_cmd(0x36);
    ili9341_send_data((void *) &data[orientation], 1);
}
