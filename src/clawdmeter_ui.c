#include "clawdmeter_ui.h"

#include <stdlib.h>
#include <string.h>
#include "rtthread.h"

/* ── External page ops (defined in clawdmeter_pages.c) ── */
extern const cm_page_ops_t cm_page_splash;
extern const cm_page_ops_t cm_page_usage;
extern const cm_page_ops_t cm_page_bluetooth;

static const cm_page_ops_t *s_pages[CM_PAGE_COUNT] = {
    &cm_page_splash,
    &cm_page_usage,
    &cm_page_bluetooth,
};

/* ── State ── */
static lv_obj_t  *s_scr;
static uint32_t   s_last_tick_ms;

/* ── Helpers ── */

lv_obj_t *cm_make_panel(lv_obj_t *parent, int16_t x, int16_t y, int16_t w, int16_t h)
{
    lv_obj_t *panel = lv_obj_create(parent);
    lv_obj_set_pos(panel, x, y);
    lv_obj_set_size(panel, w, h);
    lv_obj_set_style_bg_color(panel, CM_PANEL, 0);
    lv_obj_set_style_bg_opa(panel, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(panel, 0, 0);
    lv_obj_set_style_radius(panel, 8, 0);
    lv_obj_set_style_pad_all(panel, 12, 0);
    lv_obj_clear_flag(panel, LV_OBJ_FLAG_SCROLLABLE);
    return panel;
}

lv_obj_t *cm_make_bar(lv_obj_t *parent, int16_t x, int16_t y, int16_t w, int16_t h, lv_color_t color)
{
    lv_obj_t *bar = lv_bar_create(parent);
    lv_obj_set_pos(bar, x, y);
    lv_obj_set_size(bar, w, h);
    lv_bar_set_range(bar, 0, 100);
    lv_bar_set_value(bar, 0, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(bar, CM_BAR_BG, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(bar, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_radius(bar, 6, LV_PART_MAIN);
    lv_obj_set_style_bg_color(bar, color, LV_PART_INDICATOR);
    lv_obj_set_style_bg_opa(bar, LV_OPA_COVER, LV_PART_INDICATOR);
    lv_obj_set_style_radius(bar, 6, LV_PART_INDICATOR);
    return bar;
}

/* ── Tick timer: updates all pages every ~50ms ── */

static void tick_timer_cb(lv_timer_t *timer)
{
    (void)timer;
    uint32_t now = lv_tick_get();
    uint32_t elapsed = now - s_last_tick_ms;
    s_last_tick_ms = now;

    for (int i = 0; i < CM_PAGE_COUNT; i++) {
        if (s_pages[i] && s_pages[i]->tick) {
            s_pages[i]->tick(elapsed);
        }
    }
}

/* ── Init ── */

void clawdmeter_ui_init(lv_obj_t *scr)
{
    s_scr = scr;

    /* Configure screen as a horizontally scrollable container */
    lv_obj_set_style_bg_color(scr, CM_BG, 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);
    lv_obj_set_scroll_dir(scr, LV_DIR_HOR);
    lv_obj_set_scroll_snap_x(scr, LV_SCROLL_SNAP_CENTER);
    lv_obj_set_scrollbar_mode(scr, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_pad_all(scr, 0, 0);
    lv_obj_set_style_pad_column(scr, 0, 0);

    /* Create all 3 pages side by side */
    for (int i = 0; i < CM_PAGE_COUNT; i++) {
        lv_obj_t *page = lv_obj_create(scr);
        lv_obj_set_size(page, CM_SCR_W, CM_SCR_H);
        lv_obj_set_pos(page, CM_SCR_W * i, 0);
        lv_obj_set_scroll_dir(page, LV_DIR_NONE);
        lv_obj_set_style_bg_color(page, CM_BG, 0);
        lv_obj_set_style_bg_opa(page, LV_OPA_COVER, 0);
        lv_obj_set_style_border_width(page, 0, 0);
        lv_obj_set_style_radius(page, 0, 0);
        lv_obj_set_style_pad_all(page, 0, 0);

        if (s_pages[i] && s_pages[i]->create) {
            s_pages[i]->create(page);
        }
    }

    /* Snap to the first page */
    lv_obj_scroll_to_view(lv_obj_get_child(scr, 0), LV_ANIM_OFF);

    /* Start tick timer */
    s_last_tick_ms = lv_tick_get();
    lv_timer_create(tick_timer_cb, 50, NULL);
}
