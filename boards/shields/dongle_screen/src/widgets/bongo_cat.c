/*
 * Bongo Cat widget for YADS - snappy, keypress-driven.
 *
 * Behaviour:
 *   - Every keypress instantly flips the cat to a "paws down" frame, alternating
 *     left/right paw so it looks like real bonking. This is immediate (no WPM
 *     averaging lag).
 *   - ~160ms after the last keypress the cat returns to paws-up (idle).
 *   - A live WPM number is drawn on the widget (replaces the separate WPM
 *     widget), so the cat shows your speed too.
 *
 * Frames: bongo_idle (paws up), bongo_slow (one paw), bongo_fast (both paws).
 * We use slow/fast as the two "bonk" poses, alternating for a typing feel.
 */

#include <zephyr/kernel.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#include <zmk/display.h>
#include <zmk/event_manager.h>
#include <zmk/events/keycode_state_changed.h>
#include <zmk/events/wpm_state_changed.h>

#include "bongo_cat.h"
#include <fonts.h>

/* ms of no typing before the paws lift back to idle */
#define BONGO_IDLE_RETURN_MS 160

extern const lv_image_dsc_t bongo_idle;
extern const lv_image_dsc_t bongo_slow;
extern const lv_image_dsc_t bongo_fast;

static sys_slist_t widgets = SYS_SLIST_STATIC_INIT(&widgets);

/* which paw to show on the next keypress (toggles for an alternating bonk) */
static int bonk_toggle = 0;

static void set_frame(struct zmk_widget_bongo_cat *widget, int frame)
{
    if (frame == widget->last_frame)
    {
        return;
    }
    widget->last_frame = frame;
    const lv_image_dsc_t *src = &bongo_idle;
    if (frame == 1)
        src = &bongo_slow;
    else if (frame == 2)
        src = &bongo_fast;
    lv_image_set_src(widget->img, src);
}

/* --- idle-return timer: lifts paws after typing stops --- */
static void idle_return_work_cb(struct k_work *work)
{
    struct zmk_widget_bongo_cat *widget;
    SYS_SLIST_FOR_EACH_CONTAINER(&widgets, widget, node)
    {
        set_frame(widget, 0); // paws up
    }
}
static K_WORK_DELAYABLE_DEFINE(idle_return_work, idle_return_work_cb);

/* --- WPM label (live number on the cat) --- */
struct bongo_wpm_state
{
    int wpm;
};
static struct bongo_wpm_state wpm_get_state(const zmk_event_t *_eh)
{
    const struct zmk_wpm_state_changed *ev = as_zmk_wpm_state_changed(_eh);
    return (struct bongo_wpm_state){.wpm = ev ? ev->state : 0};
}
static void wpm_update_cb(struct bongo_wpm_state state)
{
    struct zmk_widget_bongo_cat *widget;
    SYS_SLIST_FOR_EACH_CONTAINER(&widgets, widget, node)
    {
        char buf[12];
        snprintf(buf, sizeof(buf), "%d", state.wpm);
        lv_label_set_text(widget->wpm_label, buf);
    }
}
ZMK_DISPLAY_WIDGET_LISTENER(widget_bongo_wpm, struct bongo_wpm_state,
                            wpm_update_cb, wpm_get_state)
ZMK_SUBSCRIPTION(widget_bongo_wpm, zmk_wpm_state_changed);

/* --- keypress -> instant bonk --- */
static int bongo_key_listener(const zmk_event_t *eh)
{
    const struct zmk_keycode_state_changed *ev = as_zmk_keycode_state_changed(eh);
    if (!ev || !ev->state)
    {
        return 0; // only on key-down
    }
    bonk_toggle ^= 1;
    int frame = bonk_toggle ? 1 : 2;
    struct zmk_widget_bongo_cat *widget;
    SYS_SLIST_FOR_EACH_CONTAINER(&widgets, widget, node)
    {
        widget->last_frame = -1;
        set_frame(widget, frame);
    }
    k_work_reschedule(&idle_return_work, K_MSEC(BONGO_IDLE_RETURN_MS));
    return 0;
}
ZMK_LISTENER(bongo_key, bongo_key_listener);
ZMK_SUBSCRIPTION(bongo_key, zmk_keycode_state_changed);

int zmk_widget_bongo_cat_init(struct zmk_widget_bongo_cat *widget, lv_obj_t *parent)
{
    widget->obj = lv_obj_create(parent);
    lv_obj_set_size(widget->obj, 150, 100);
    lv_obj_set_style_bg_opa(widget->obj, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(widget->obj, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(widget->obj, 0, LV_PART_MAIN);

    widget->img = lv_image_create(widget->obj);
    lv_obj_align(widget->img, LV_ALIGN_CENTER, 0, 0);
    lv_image_set_src(widget->img, &bongo_idle);
    widget->last_frame = 0;

    /* WPM number, top-left corner of the widget */
    widget->wpm_label = lv_label_create(widget->obj);
    lv_label_set_text(widget->wpm_label, "0");
    lv_obj_align(widget->wpm_label, LV_ALIGN_TOP_LEFT, 2, 2);

    sys_slist_append(&widgets, &widget->node);
    widget_bongo_wpm_init();
    return 0;
}

lv_obj_t *zmk_widget_bongo_cat_obj(struct zmk_widget_bongo_cat *widget)
{
    return widget->obj;
}
