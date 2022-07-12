#include "lv_drivers.h"

static constexpr size_t displayBufferSize = 480 * 40;

static lv_disp_drv_t lv_display_device;
/*A static or global variable to store the buffers*/
static lv_disp_draw_buf_t lv_display_buffer;

/*Static or global buffer(s). The second buffer is optional*/
static lv_color_t lv_color_buffer[displayBufferSize]; //
// static lv_color_t buf2[displayBufferSize];

/*Initialize the (dummy) input device driver*/
static lv_indev_drv_t lv_input_device;
static repeating_timer lv_tick_timer;

uint8_t tft_dataPins[8] = {TFT_D0, TFT_D1, TFT_D2, TFT_D3, TFT_D4, TFT_D5, TFT_D6, TFT_D7};
ili9486_drivers *tft;

void init_display()
{
    tft = new ili9486_drivers(tft_dataPins, TFT_RST, TFT_CS, TFT_RS, TFT_WR, TFT_RD, TOUCH_XP, TOUCH_XM, TOUCH_YP,
                              TOUCH_YM, TOUCH_XP_ADC_CHANNEL, TOUCH_YM_ADC_CHANNEL);
    tft->init();
    tft->setRotation(INVERTED_LANDSCAPE);
    tft->dmaInit([]() {
        lv_disp_flush_ready(&lv_display_device);
        tft->dmaClearIRQ();
    });

    lv_init();

    /*Initialize `lv_display_buffer` with the buffer(s). With only one buffer use NULL instead buf_2 */
    lv_disp_draw_buf_init(&lv_display_buffer, lv_color_buffer, NULL, displayBufferSize);

    /*Initialize the display*/
    lv_disp_drv_init(&lv_display_device);
    /*Change the following line to your display resolution*/
    lv_display_device.hor_res = tft->width();
    lv_display_device.ver_res = tft->height();
    lv_display_device.flush_cb = lv_display_flush_cb;
    lv_display_device.draw_buf = &lv_display_buffer;
    lv_disp_drv_register(&lv_display_device);

    lv_indev_drv_init(&lv_input_device);
    lv_input_device.type = LV_INDEV_TYPE_POINTER;
    lv_input_device.read_cb = lv_input_touch_cb;
    lv_indev_drv_register(&lv_input_device);

    add_repeating_timer_ms(
        5,
        [](struct repeating_timer *t) -> bool {
            lv_tick_inc(5);
            return true;
        },
        NULL, &lv_tick_timer);
}

void lv_display_flush_cb(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p)
{
    if (!tft->dmaBusy())
    {
        uint32_t w = (area->x2 - area->x1 + 1);
        uint32_t h = (area->y2 - area->y1 + 1);
        tft->selectTFT();
        tft->setWindow(area->x1, area->y1, area->x2, area->y2);
        tft->pushColorsDMA((uint16_t *)&color_p->full, w * h);
    }
}

void lv_input_touch_cb(lv_indev_drv_t *indev_driver, lv_indev_data_t *data)
{
    TouchCoordinate tc;
    tft->sampleTouch(tc);
    data->state = tc.touched ? LV_INDEV_STATE_PRESSED : LV_INDEV_STATE_RELEASED;
    data->point.x = tc.x;
    data->point.y = tc.y;
}