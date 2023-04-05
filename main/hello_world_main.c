#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_freertos_hooks.h"
#include "freertos/semphr.h"
#include "esp_system.h"
#include "driver/gpio.h"

/* Littlevgl specific */
#ifdef LV_LVGL_H_INCLUDE_SIMPLE
#include "lvgl.h"
#else
#include "lvgl/lvgl.h"
#endif

#include "lvgl_helpers.h"

/*********************
 *      DEFINES
 *********************/
#define TAG "main"
#define LV_TICK_PERIOD_MS 1

/**********************
 *  STATIC PROTOTYPES
 **********************/
static void lv_tick_task(void *arg);
static void guiTask(void *pvParameter);
static void create_demo_application(void);

/**********************
 *   APPLICATION MAIN
 **********************/
void app_main() {
    vTaskDelay(3000 / portTICK_PERIOD_MS);
    printf("app_main start...\n");
    /* 使用任务创建图形时，必须使用固定任务创建
     * 注意：不使用Wi-Fi或蓝牙时，可以将gui任务固定到核心0 */
    xTaskCreatePinnedToCore(guiTask, "gui", 4096*2, NULL, 0, NULL, 1);
}

/* Creates a semaphore to handle concurrent call to lvgl stuff
 * If you wish to call *any* lvgl function from other threads/tasks
 * you should lock on the very same semaphore! */
SemaphoreHandle_t xGuiSemaphore;

static void guiTask(void *pvParameter) {

    (void) pvParameter;
    xGuiSemaphore = xSemaphoreCreateMutex();

    lv_init();
    printf("%s(),LINE=%d...\n", __FUNCTION__, __LINE__);

    /* 初始化驱动程序使用的SPI或I2C总线 */
    lvgl_driver_init();
    printf("%s(),LINE=%d...\n", __FUNCTION__, __LINE__);

    /* 创建堆内存，用过图形缓冲区buf1 */
    lv_color_t* buf1 = heap_caps_malloc(DISP_BUF_SIZE * sizeof(lv_color_t), MALLOC_CAP_DMA);
    assert(buf1 != NULL);
    printf("%s(),LINE=%d...\n", __FUNCTION__, __LINE__);

    /* 不使用单色显示器时使用双缓冲 */
#ifndef CONFIG_LV_TFT_DISPLAY_MONOCHROME
    lv_color_t* buf2 = heap_caps_malloc(DISP_BUF_SIZE * sizeof(lv_color_t), MALLOC_CAP_DMA);
    assert(buf2 != NULL);
#else
    static lv_color_t *buf2 = NULL;
#endif

    static lv_disp_draw_buf_t disp_buf;

    uint32_t size_in_px = DISP_BUF_SIZE;
    printf("%s(),LINE=%d...\n", __FUNCTION__, __LINE__);

#if defined CONFIG_LV_TFT_DISPLAY_CONTROLLER_IL3820         \
    || defined CONFIG_LV_TFT_DISPLAY_CONTROLLER_JD79653A    \
    || defined CONFIG_LV_TFT_DISPLAY_CONTROLLER_UC8151D     \
    || defined CONFIG_LV_TFT_DISPLAY_CONTROLLER_SSD1306

    /* Actual size in pixels, not bytes. */
    size_in_px *= 8;
#endif

    /* Initialize the working buffer depending on the selected display.
     * NOTE: buf2 == NULL when using monochrome displays. */
    lv_disp_draw_buf_init(&disp_buf, buf1, buf2, size_in_px);
    printf("%s(),LINE=%d...\n", __FUNCTION__, __LINE__);

    lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    disp_drv.flush_cb = disp_driver_flush;
    printf("%s(),LINE=%d...\n", __FUNCTION__, __LINE__);

    /* When using a monochrome display we need to register the callbacks:
     * - rounder_cb
     * - set_px_cb */
#ifdef CONFIG_LV_TFT_DISPLAY_MONOCHROME
    disp_drv.rounder_cb = disp_driver_rounder;
    disp_drv.set_px_cb = disp_driver_set_px;
#endif

    disp_drv.draw_buf = &disp_buf;
    lv_disp_drv_register(&disp_drv);
    printf("%s(),LINE=%d...\n", __FUNCTION__, __LINE__);

    /* Register an input device when enabled on the menuconfig */
#if CONFIG_LV_TOUCH_CONTROLLER != TOUCH_CONTROLLER_NONE
    lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.read_cb = touch_driver_read;
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    lv_indev_drv_register(&indev_drv);
#endif

    /* Create and start a periodic timer interrupt to call lv_tick_inc */
    const esp_timer_create_args_t periodic_timer_args = {
        .callback = &lv_tick_task,
        .name = "periodic_gui"
    };
    esp_timer_handle_t periodic_timer;
    ESP_ERROR_CHECK(esp_timer_create(&periodic_timer_args, &periodic_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(periodic_timer, LV_TICK_PERIOD_MS * 1000));
    printf("%s(),LINE=%d...\n", __FUNCTION__, __LINE__);
    /* Create the demo application */
    create_demo_application();
    printf("%s(),LINE=%d...\n", __FUNCTION__, __LINE__);

    while (1) {
        /* Delay 1 tick (assumes FreeRTOS tick is 10ms */
        vTaskDelay(pdMS_TO_TICKS(10));
        //printf("%s(),LINE=%d...\n", __FUNCTION__, __LINE__);
        /* Try to take the semaphore, call lvgl related function on success */
        if (pdTRUE == xSemaphoreTake(xGuiSemaphore, portMAX_DELAY)) {
            lv_task_handler();
            xSemaphoreGive(xGuiSemaphore);
       }
    }

    /* A task should NEVER return */
    free(buf1);
#ifndef CONFIG_LV_TFT_DISPLAY_MONOCHROME
    free(buf2);
#endif
    vTaskDelete(NULL);
}

/**
 * @brief 创建标签
 */
void lvgl_lable_test(){
    /* 创建一个标签 */
    lv_obj_t* label = lv_label_create(lv_scr_act());
    if (NULL != label)
    {
        // lv_obj_set_x(label, 90);                         // 设置控件的X坐标
        // lv_obj_set_y(label, 100);                        // 设置控件的Y坐标
        // lv_obj_set_size(label, 60, 20);                  // 设置控件大小
        lv_label_set_text(label, "Counter");                // 初始显示 0
        // lv_obj_center(label);                            // 居中显示
        lv_obj_align(label, LV_ALIGN_CENTER, 0, -50);       // 居中显示后，向上偏移50
    }
}

/**
 * @brief 按钮事件回调函数
 */
static void btn_event_callback(lv_event_t* event)
{
    static uint32_t counter = 1;

    lv_obj_t* btn = lv_event_get_target(event);                 //获取事件对象
    if (btn != NULL)
    {
        lv_obj_t* btn_parent =  lv_obj_get_parent(btn);
        lv_obj_t* label = lv_obj_get_child(btn_parent, 0);
        lv_label_set_text_fmt(label, "%d", counter);            //设置显示内容
        lv_obj_align(label, LV_ALIGN_CENTER, 0, -50);           // 居中显示后，向上偏移50
        counter++;
    }
}

/**
 * @brief 创建按钮
 */
void lvgl_button_test(){
    /* 在当前界面中创建一个按钮 */
    lv_obj_t* btn = lv_btn_create(lv_scr_act());                                        // 创建Button对象
    if (btn != NULL)
    {
        lv_obj_set_size(btn, 80, 20);                                                   // 设置对象宽度和高度
        // lv_obj_set_pos(btn, 90, 200);                                                // 设置按钮的X和Y坐标
        lv_obj_add_event_cb(btn, btn_event_callback, LV_EVENT_CLICKED, NULL);           // 给对象添加CLICK事件和事件处理回调函数
        lv_obj_align(btn, LV_ALIGN_CENTER, 0, 50);                                      // 居中显示后，向下偏移50

        lv_obj_t* btn_label = lv_label_create(btn);                                     // 基于Button对象创建Label对象
        if (btn_label != NULL)
        {
            lv_label_set_text(btn_label, "button");                                     // 设置显示内容
            lv_obj_center(btn_label);                                                   // 对象居中显示
        }
    }
}

static void create_demo_application(void)
{
    /* 加载标签 */
    lvgl_lable_test();
    /* 加载按钮 */
    lvgl_button_test();

}

static void lv_tick_task(void *arg) {
    (void) arg;

    lv_tick_inc(LV_TICK_PERIOD_MS);
}
