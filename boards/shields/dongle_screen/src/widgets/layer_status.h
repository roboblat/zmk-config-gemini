/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <lvgl.h>
#include <zephyr/kernel.h>

struct zmk_widget_layer_status {
    sys_snode_t node;
    lv_obj_t *obj;            /* container; aligned by the status screen */
    lv_obj_t *outline[8];     /* black copies, offset around the center */
    lv_obj_t *center;         /* white label on top */
};

int zmk_widget_layer_status_init(struct zmk_widget_layer_status *widget, lv_obj_t *parent);
lv_obj_t *zmk_widget_layer_status_obj(struct zmk_widget_layer_status *widget);
