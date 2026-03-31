#include "e1002_display.h"

#include <lvgl.h>
#include <TFT_eSPI.h>

#define TFT_HOR_RES 800
#define TFT_VER_RES 480

/*LVGL draw into this buffer, 1/10 screen size usually works well. The size is in bytes*/
#define DRAW_BUF_SIZE (TFT_HOR_RES * TFT_VER_RES / 10 * (LV_COLOR_DEPTH / 8))
static uint8_t draw_buf[DRAW_BUF_SIZE];

EPaper epaper;

#if LV_USE_LOG != 0
void arduino_print(lv_log_level_t level, const char *buf)
{
    LV_UNUSED(level);
    Serial.println(buf);
    Serial.flush();
}
#endif

static uint8_t rgb888_to_epaper_4gray(uint8_t r, uint8_t g, uint8_t b)
{
    uint8_t gray = (r * 0.299 + g * 0.587 + b * 0.114);
    
    if (gray < 64) return 0x0;      // Black
    else if (gray < 128) return 0x5; // Dark gray
    else if (gray < 192) return 0xA; // Light gray
    else return 0xF;                 // White
}

/* LVGL calls it when a rendered image needs to copied to the display*/
static void e1002_disp_flush(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map)
{
    e1002_driver_t *drv = (e1002_driver_t *)lv_display_get_driver_data(disp);
    EPaper *epd = drv->epd;
    int32_t w = lv_area_get_width(area);
    int32_t h = lv_area_get_height(area);

    Serial.print("Flushing area: ");
    Serial.print(area->x1);
    Serial.print(",");
    Serial.print(area->y1);
    Serial.print(" - ");
    Serial.print(w);
    Serial.print("x");
    Serial.println(h);

    for (int y = 0; y < h; y++)
    {
        for (int x = 0; x < w; x++)
        {
            lv_color_t lv_color = ((lv_color_t *)px_map)[y * w + x];
            
            lv_color32_t c32 = lv_color_to_32(lv_color, 0xFF);
            
            uint8_t r = c32.red;
            uint8_t g = c32.green;
            uint8_t b = c32.blue;
            
            uint8_t epaper_clr_index = rgb888_to_epaper_4gray(r, g, b);
            
            epd->drawPixel(area->x1 + x, area->y1 + y, epaper_clr_index);
        }
    }

    if (lv_display_flush_is_last(disp))
    {
        drv->flush_scheduled = true;
        drv->flush_scheduled_time = millis();
        Serial.println("Last flush - scheduled refresh");
    }

    /*Call it to tell LVGL you are ready*/
    lv_display_flush_ready(disp);
}

/*Read the touchpad*/
static void device_touchpad_read(lv_indev_t *indev, lv_indev_data_t *data)
{
}

/*use Arduinos millis() as tick source*/
static uint32_t get_arduino_tick(void)
{
    return millis();
}

static void resolution_changed_event_cb(lv_event_t *e)
{
    lv_display_t *disp = (lv_display_t *)lv_event_get_target(e);
    e1002_driver_t *drv = (e1002_driver_t *)lv_display_get_driver_data(disp);
    EPaper *epd = drv->epd;
    int32_t hor_res = lv_display_get_horizontal_resolution(disp);
    int32_t ver_res = lv_display_get_vertical_resolution(disp);
    lv_display_rotation_t rot = lv_display_get_rotation(disp);

    /* handle rotation */
    switch (rot)
    {
    case LV_DISPLAY_ROTATION_0:
        epd->setRotation(0); /* Portrait orientation */
        break;
    case LV_DISPLAY_ROTATION_90:
        epd->setRotation(1); /* Landscape orientation */
        break;
    case LV_DISPLAY_ROTATION_180:
        epd->setRotation(2); /* Portrait orientation, flipped */
        break;
    case LV_DISPLAY_ROTATION_270:
        epd->setRotation(3); /* Landscape orientation, flipped */
        break;
    }
}

void e1002_display_init(e1002_driver_t *drv)
{
    Serial.println("  e1002_display_init called");
    
    drv->epd = &epaper;
    drv->flush_scheduled = false;
    drv->flush_scheduled_time = 0;

    Serial.println("  Calling epaper.begin()...");
    drv->epd->begin();
    Serial.println("  epaper.begin() complete");

    Serial.println("  Calling lv_init()...");
    lv_init();
    Serial.println("  lv_init() complete");

    /*Set a tick source so that LVGL will know how much time elapsed. */
    lv_tick_set_cb(get_arduino_tick);

    /* register print function for debugging */
#if LV_USE_LOG != 0
    lv_log_register_print_cb(arduino_print);
#endif

    lv_display_t *disp;

    Serial.println("  Creating LVGL display...");
    disp = lv_display_create(TFT_HOR_RES, TFT_VER_RES);
    lv_display_set_driver_data(disp, (void *)drv);
    lv_display_set_flush_cb(disp, e1002_disp_flush);
    lv_display_set_buffers(disp, draw_buf, NULL, sizeof(draw_buf), LV_DISPLAY_RENDER_MODE_PARTIAL);
    
    Serial.print("  Display buffer size: ");
    Serial.print(sizeof(draw_buf));
    Serial.println(" bytes");
    Serial.print("  LV_COLOR_DEPTH: ");
    Serial.println(LV_COLOR_DEPTH);
    Serial.println("  LVGL display created");
}

void e1002_display_refresh(e1002_driver_t *drv)
{
    Serial.println("  Calling EPaper update()...");
    drv->epd->update();
    drv->flush_scheduled = false;
    Serial.println("  EPaper update() complete");
}

void e1002_display_schedule_refresh(e1002_driver_t *drv)
{
    drv->flush_scheduled = true;
    drv->flush_scheduled_time = millis();
}

bool e1002_display_should_refresh(e1002_driver_t *drv)
{
    return drv->flush_scheduled && (millis() - drv->flush_scheduled_time > 100);
}
