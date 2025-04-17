/*
 * SPDX-FileCopyrightText: 2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

// This demo UI is adapted from LVGL official example: https://docs.lvgl.io/master/examples.html#scatter-chart

#include "lvgl.h"
#include "Untitled2.c"
#include "kul.c"
#include "room1_token.c"
#include "rgb_lcd_example_main.h"
//#include "example1.c"

// Declare the images
LV_IMG_DECLARE(room1_token);
LV_IMG_DECLARE(kul);

static void draw_event_cb(lv_event_t *e)
{
    lv_obj_draw_part_dsc_t *dsc = lv_event_get_draw_part_dsc(e);
    if (dsc->part == LV_PART_ITEMS) {
        lv_obj_t *obj = lv_event_get_target(e);
        lv_chart_series_t *ser = lv_chart_get_series_next(obj, NULL);
        uint32_t cnt = lv_chart_get_point_count(obj);
        /*Make older value more transparent*/
        dsc->rect_dsc->bg_opa = (LV_OPA_COVER *  dsc->id) / (cnt - 1);

        /*Make smaller values blue, higher values red*/
        lv_coord_t *x_array = lv_chart_get_x_array(obj, ser);
        lv_coord_t *y_array = lv_chart_get_y_array(obj, ser);
        /*dsc->id is the tells drawing order, but we need the ID of the point being drawn.*/
        uint32_t start_point = lv_chart_get_x_start_point(obj, ser);
        uint32_t p_act = (start_point + dsc->id) % cnt; /*Consider start point to get the index of the array*/
        lv_opa_t x_opa = (x_array[p_act] * LV_OPA_50) / 200;
        lv_opa_t y_opa = (y_array[p_act] * LV_OPA_50) / 1000;

        dsc->rect_dsc->bg_color = lv_color_mix(lv_palette_main(LV_PALETTE_RED),
                                               lv_palette_main(LV_PALETTE_BLUE),
                                               x_opa + y_opa);
    }
}

static void add_data(lv_timer_t *timer)
{
    lv_obj_t *chart = timer->user_data;
    lv_chart_set_next_value2(chart, lv_chart_get_series_next(chart, NULL), lv_rand(0, 200), lv_rand(0, 1000));
}

void example_start_ui(lv_disp_t *disp){
    lv_obj_t *scr = lv_disp_get_scr_act(disp);
    lv_obj_t *label = lv_label_create(scr);
    lv_label_set_text(label, "Hello"); 
    lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);
}

// Timer callback function
static void image_update_timer_cb(lv_timer_t *timer)
{
    lv_obj_t *img = (lv_obj_t *)timer->user_data;
    if (get_image_toggle_state()) {
        lv_img_set_src(img, &kul);
    } else {
        lv_img_set_src(img, &room1_token);
    }
}

void example_lvgl_demo_ui(lv_disp_t *disp)
{
    // Create a screen
    lv_obj_t *scr = lv_scr_act();
    
    // Create an image object
    lv_obj_t *img = lv_img_create(scr);
    
    // Set initial image based on toggle state
    if (get_image_toggle_state()) {
        lv_img_set_src(img, &kul);
    } else {
        lv_img_set_src(img, &room1_token);
    }
    
    // Center the image
    lv_obj_align(img, LV_ALIGN_CENTER, 0, 0);
    
    // Create a timer to check toggle state and update image
    lv_timer_t *timer = lv_timer_create(image_update_timer_cb, 1000, img);  // Check every 1000ms
}
