#pragma once

#include "lvgl.h"
#include <stdint.h>
#include <stdbool.h>

/* ── Screen dimensions (CO5300 390×450) ── */
#define CM_SCR_W  390
#define CM_SCR_H  450

/* ── Colour palette ── */
#define CM_BG       lv_color_hex(0x000000)
#define CM_PANEL    lv_color_hex(0x1F1F1E)
#define CM_TEXT     lv_color_hex(0xFAF9F5)
#define CM_DIM      lv_color_hex(0xB0AEA5)
#define CM_ACCENT   lv_color_hex(0xD97757)
#define CM_GREEN    lv_color_hex(0x788C5D)
#define CM_AMBER    lv_color_hex(0xD97757)
#define CM_RED      lv_color_hex(0xC0392B)
#define CM_BAR_BG   lv_color_hex(0x2A2A28)

/* ── Animation timing ── */
#define CM_ANIM_ROTATE_MS   20000U
#define CM_GRID             20

/* ── Page IDs ── */
#define CM_PAGE_COUNT  3

/* ── Page callbacks ── */
typedef struct {
    const char *name;
    void (*create)(lv_obj_t *parent);   /* build widgets under parent */
    void (*tick)(uint32_t elapsed_ms);  /* periodic update */
} cm_page_ops_t;

/* ── API ── */

/**
 * Initialise the Clawdmeter UI on the given screen object.
 * Creates all 3 pages side-by-side with LVGL scroll snap.
 */
void clawdmeter_ui_init(lv_obj_t *scr);

/* ── Shared helpers (defined in clawdmeter_ui.c) ── */

lv_obj_t *cm_make_panel(lv_obj_t *parent, int16_t x, int16_t y, int16_t w, int16_t h);
lv_obj_t *cm_make_bar(lv_obj_t *parent, int16_t x, int16_t y, int16_t w, int16_t h, lv_color_t color);
