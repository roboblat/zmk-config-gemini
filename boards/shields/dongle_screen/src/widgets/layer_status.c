/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 *
 * Layer name with a real black outline: 8 black label copies offset 1px around
 * a white center label. LVGL has no outline-font, so this multi-draw is the
 * reliable way to keep white text legible over the bongo cat / purple bg.
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#include <zmk/display.h>
#include "layer_status.h"
#include <zmk/events/layer_state_changed.h>
#include <zmk/event_manager.h>
#include <zmk/endpoints.h>
#include <zmk/keymap.h>

static sys_slist_t widgets = SYS_SLIST_STATIC_INIT(&widgets);

/* 8 offsets around the center for the outline copies */
static const lv_coord_t OFF_X[8] = {-1, 0, 1, -1, 1, -1, 0, 1};
static const lv_coord_t OFF_Y[8] = {-1, -1, -1, 0, 0, 1, 1, 1};

struct layer_status_state
{
    uint8_t index;
    const char *label;
};

static void set_text_all(struct zmk_widget_layer_status *widget, const char *text)
{
    for (int i = 0; i < 8; i++)
    {
        lv_label_set_text(widget->outline[i], text);
    }
    lv_label_set_text(widget->center, text);
}

static void set_layer_symbol(struct zmk_widget_layer_status *widget, struct layer_status_state state)
{
    char text[13] = {};
    if (state.label == NULL)
    {
        snprintf(text, sizeof(text), "%i", state.index);
    }
    else
    {
        snprintf(text, sizeof(text), "%s", state.label);
    }
    set_text_all(widget, text);
}

static void layer_status_update_cb(struct layer_status_state state)
{
    struct zmk_widget_layer_status *widget;
    SYS_SLIST_FOR_EACH_CONTAINER(&widgets, widget, node) { set_layer_symbol(widget, state); }
}

static struct layer_status_state layer_status_get_state(const zmk_event_t *eh)
{
    uint8_t index = zmk_keymap_highest_layer_active();
    return (struct layer_status_state){
        .index = index,
        .label = zmk_keymap_layer_name(index)};
}

ZMK_DISPLAY_WIDGET_LISTENER(widget_layer_status, struct layer_status_state, layer_status_update_cb,
                            layer_status_get_state)

ZMK_SUBSCRIPTION(widget_layer_status, zmk_layer_state_changed);

int zmk_widget_layer_status_init(struct zmk_widget_layer_status *widget, lv_obj_t *parent)
{
    /* Transparent container holds all 9 labels, centered on each other. */
    widget->obj = lv_obj_create(parent);
    lv_obj_set_size(widget->obj, 240, 48);
    lv_obj_set_style_bg_opa(widget->obj, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(widget->obj, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(widget->obj, 0, LV_PART_MAIN);

    /* 8 black outline copies */
    for (int i = 0; i < 8; i++)
    {
        widget->outline[i] = lv_label_create(widget->obj);
        lv_obj_set_style_text_font(widget->outline[i], &lv_font_montserrat_40, 0);
        lv_obj_set_style_text_color(widget->outline[i], lv_color_black(), 0);
        lv_label_set_text(widget->outline[i], "");
        lv_obj_align(widget->outline[i], LV_ALIGN_CENTER, OFF_X[i], OFF_Y[i]);
    }

    /* white center on top */
    widget->center = lv_label_create(widget->obj);
    lv_obj_set_style_text_font(widget->center, &lv_font_montserrat_40, 0);
    lv_obj_set_style_text_color(widget->center, lv_color_white(), 0);
    lv_label_set_text(widget->center, "");
    lv_obj_align(widget->center, LV_ALIGN_CENTER, 0, 0);

    sys_slist_append(&widgets, &widget->node);

    widget_layer_status_init();
    return 0;
}

lv_obj_t *zmk_widget_layer_status_obj(struct zmk_widget_layer_status *widget)
{
    return widget->obj;
}
