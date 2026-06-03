/*
 * Bongo Cat widget for YADS (color ST7789).
 * Bonks on each keypress (snappy), shows live WPM number, and picks a paw
 * frame by typing speed. Designed to sit top-left and replace the separate
 * WPM widget.
 */

#pragma once

#include <lvgl.h>
#include <zephyr/kernel.h>

struct zmk_widget_bongo_cat
{
    lv_obj_t *obj;
    lv_obj_t *img;      // the cat image
    lv_obj_t *wpm_label; // live WPM number overlaid on the widget
    int last_frame;     // 0 up, 1 one-paw, 2 both-paws - avoids redundant redraws
    sys_snode_t node;
};

int zmk_widget_bongo_cat_init(struct zmk_widget_bongo_cat *widget, lv_obj_t *parent);
lv_obj_t *zmk_widget_bongo_cat_obj(struct zmk_widget_bongo_cat *widget);
