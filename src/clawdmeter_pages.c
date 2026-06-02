#include "clawdmeter_ui.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "rtthread.h"
#include "lvgl.h"

/* ── Asset includes ── */
#include "clawdmeter_assets/icons.h"
#include "clawdmeter_assets/logo_tca.h"
#include "clawdmeter_assets/splash_animations.h"

/* ── Custom font declarations ── */
LV_FONT_DECLARE(font_tiempos_56);
LV_FONT_DECLARE(font_tiempos_34);
LV_FONT_DECLARE(font_styrene_48);
LV_FONT_DECLARE(font_styrene_28);
LV_FONT_DECLARE(font_styrene_24);
LV_FONT_DECLARE(font_styrene_20);
LV_FONT_DECLARE(font_styrene_16);
LV_FONT_DECLARE(font_styrene_14);
LV_FONT_DECLARE(font_mono_32);

/* ════════════════════════════════════════════════════════════════════
 *  Shared image descriptors (initialised once)
 * ════════════════════════════════════════════════════════════════════ */

static lv_img_dsc_t s_icon_bluetooth_dsc;
static lv_img_dsc_t s_logo_dsc;
static bool         s_img_dsc_ready;

static void ensure_img_dsc_ready(void)
{
    if (s_img_dsc_ready) return;

    /* Bluetooth icon — TRUE_COLOR (RGB565) */
    memset(&s_icon_bluetooth_dsc, 0, sizeof(s_icon_bluetooth_dsc));
    s_icon_bluetooth_dsc.header.cf = LV_IMG_CF_TRUE_COLOR;
    s_icon_bluetooth_dsc.header.w  = ICON_BLUETOOTH_W;
    s_icon_bluetooth_dsc.header.h  = ICON_BLUETOOTH_H;
    s_icon_bluetooth_dsc.data_size = (uint32_t)ICON_BLUETOOTH_W * (uint32_t)ICON_BLUETOOTH_H * sizeof(lv_color_t);
    s_icon_bluetooth_dsc.data      = (const uint8_t *)icon_bluetooth_data;

    /* Logo — TRUE_COLOR_ALPHA (converted from RGB565A8) */
    memset(&s_logo_dsc, 0, sizeof(s_logo_dsc));
    s_logo_dsc.header.cf = LV_IMG_CF_TRUE_COLOR_ALPHA;
    s_logo_dsc.header.w  = LOGO_TCA_WIDTH;
    s_logo_dsc.header.h  = LOGO_TCA_HEIGHT;
    s_logo_dsc.data_size = (uint32_t)LOGO_TCA_WIDTH * (uint32_t)LOGO_TCA_HEIGHT * 3U;
    s_logo_dsc.data      = logo_tca_data;

    s_img_dsc_ready = true;
}

/* ════════════════════════════════════════════════════════════════════
 *  SPLASH PAGE — Pixel-art animation
 * ════════════════════════════════════════════════════════════════════ */

typedef struct {
    lv_obj_t  *canvas;
    lv_color_t *canvas_buf;
    lv_color_t *row_buf;
    int16_t     canvas_w;
    int16_t     canvas_h;
    int16_t     cell;
    uint16_t    anim_idx;
    uint16_t    frame_idx;
    uint32_t    frame_elapsed_ms;
    uint32_t    rotate_elapsed_ms;
} splash_data_t;

static splash_data_t s_splash;

static lv_color_t cm_rgb565_to_color(uint16_t c)
{
    uint8_t r = (uint8_t)(((c >> 11) & 0x1F) * 255U / 31U);
    uint8_t g = (uint8_t)(((c >> 5)  & 0x3F) * 255U / 63U);
    uint8_t b = (uint8_t)((c & 0x1F) * 255U / 31U);
    return lv_color_make(r, g, b);
}

static void splash_render_frame(void)
{
    splash_data_t *d = &s_splash;
    if (!d->canvas_buf || !d->row_buf || SPLASH_ANIM_COUNT == 0) return;

    const splash_anim_def_t *anim = &splash_anims[d->anim_idx % SPLASH_ANIM_COUNT];
    if (anim->frame_count == 0) return;

    const uint8_t *cells = anim->frames[d->frame_idx % anim->frame_count];
    for (int gy = 0; gy < CM_GRID; ++gy) {
        for (int gx = 0; gx < CM_GRID; ++gx) {
            uint8_t code = cells[gy * CM_GRID + gx];
            lv_color_t color = lv_color_black();
            if (anim->palette && code < SPLASH_PALETTE_SIZE) {
                color = cm_rgb565_to_color(anim->palette[code]);
            }
            lv_color_t *out = &d->row_buf[gx * d->cell];
            for (int x = 0; x < d->cell; ++x) {
                out[x] = color;
            }
        }
        for (int dy = 0; dy < d->cell; ++dy) {
            memcpy(&d->canvas_buf[(gy * d->cell + dy) * d->canvas_w],
                   d->row_buf, (size_t)d->canvas_w * sizeof(lv_color_t));
        }
    }
    if (d->canvas) lv_obj_invalidate(d->canvas);
}

static void splash_create(lv_obj_t *parent)
{
    splash_data_t *d = &s_splash;
    memset(d, 0, sizeof(*d));

    int16_t min_dim = CM_SCR_W < CM_SCR_H ? CM_SCR_W : CM_SCR_H;
    d->cell = (int16_t)(min_dim / CM_GRID);
    if (d->cell < 4) d->cell = 4;
    d->canvas_w = (int16_t)(d->cell * CM_GRID);
    d->canvas_h = d->canvas_w;

    size_t canvas_bytes = (size_t)d->canvas_w * (size_t)d->canvas_h * sizeof(lv_color_t);
    size_t row_bytes    = (size_t)d->canvas_w * sizeof(lv_color_t);

    d->canvas_buf = rt_malloc(canvas_bytes);
    d->row_buf    = rt_malloc(row_bytes);

    if (d->canvas_buf && d->row_buf) {
        d->canvas = lv_canvas_create(parent);
        lv_canvas_set_buffer(d->canvas, d->canvas_buf,
                             d->canvas_w, d->canvas_h, LV_IMG_CF_TRUE_COLOR);
        lv_obj_center(d->canvas);
        splash_render_frame();
    } else {
        lv_obj_t *lbl = lv_label_create(parent);
        lv_obj_set_style_text_color(lbl, CM_DIM, 0);
        lv_obj_set_style_text_font(lbl, &font_styrene_28, 0);
        lv_obj_set_width(lbl, CM_SCR_W - 40);
        lv_obj_set_style_text_align(lbl, LV_TEXT_ALIGN_CENTER, 0);
        lv_label_set_text(lbl, "Clawdmeter\ncanvas alloc failed");
        lv_obj_center(lbl);
    }
}

static void splash_tick(uint32_t elapsed_ms)
{
    splash_data_t *d = &s_splash;
    if (!d->canvas_buf || SPLASH_ANIM_COUNT == 0) return;

    d->frame_elapsed_ms  += elapsed_ms;
    d->rotate_elapsed_ms += elapsed_ms;

    const splash_anim_def_t *anim = &splash_anims[d->anim_idx % SPLASH_ANIM_COUNT];
    uint16_t hold_ms = anim->holds[d->frame_idx % anim->frame_count];

    if (d->frame_elapsed_ms >= hold_ms) {
        d->frame_elapsed_ms = 0;
        d->frame_idx = (uint16_t)((d->frame_idx + 1U) % anim->frame_count);
        splash_render_frame();
    }

    if (d->rotate_elapsed_ms >= CM_ANIM_ROTATE_MS) {
        d->rotate_elapsed_ms = 0;
        d->anim_idx = (uint16_t)((d->anim_idx + 1U) % SPLASH_ANIM_COUNT);
        d->frame_idx = 0;
        d->frame_elapsed_ms = 0;
        splash_render_frame();
    }
}

const cm_page_ops_t cm_page_splash = {
    .name   = "Splash",
    .create = splash_create,
    .tick   = splash_tick,
};

/* ════════════════════════════════════════════════════════════════════
 *  USAGE PAGE — Usage dashboard (demo mode, no BLE data)
 * ════════════════════════════════════════════════════════════════════ */

static const char *const s_spinner_frames[] = { ".", "o", "O", "o" };
static const char *const s_status_messages[] = {
    "Accomplishing", "Elucidating",  "Computing",
    "Thinking",      "Synthesizing", "Processing",
};

typedef struct {
    lv_obj_t *session_pct;
    lv_obj_t *session_bar;
    lv_obj_t *session_reset;
    lv_obj_t *weekly_pct;
    lv_obj_t *weekly_bar;
    lv_obj_t *weekly_reset;
    lv_obj_t *anim_label;
    uint32_t  spinner_elapsed_ms;
    uint32_t  message_elapsed_ms;
    uint8_t   spinner_idx;
    uint8_t   message_idx;
} usage_data_t;

static usage_data_t s_usage;

static void usage_make_panel(lv_obj_t *parent, int16_t y, const char *pill_text,
                             lv_obj_t **out_pct, lv_obj_t **out_bar, lv_obj_t **out_reset)
{
    lv_obj_t *panel = cm_make_panel(parent, 20, y, 350, 130);

    lv_obj_t *pct = lv_label_create(panel);
    lv_obj_set_style_text_color(pct, CM_TEXT, 0);
    lv_obj_set_style_text_font(pct, &font_styrene_48, 0);
    lv_label_set_text(pct, "---%");
    lv_obj_set_pos(pct, 0, 0);
    *out_pct = pct;

    lv_obj_t *pill = lv_label_create(panel);
    lv_obj_set_style_text_color(pill, CM_TEXT, 0);
    lv_obj_set_style_text_font(pill, &font_styrene_28, 0);
    lv_obj_set_style_bg_color(pill, CM_BAR_BG, 0);
    lv_obj_set_style_bg_opa(pill, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(pill, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_pad_left(pill, 14, 0);
    lv_obj_set_style_pad_right(pill, 14, 0);
    lv_obj_set_style_pad_top(pill, 5, 0);
    lv_obj_set_style_pad_bottom(pill, 5, 0);
    lv_label_set_text(pill, pill_text);
    lv_obj_align(pill, LV_ALIGN_TOP_RIGHT, 0, 1);

    *out_bar = cm_make_bar(panel, 0, 48, 326, 24, CM_GREEN);

    lv_obj_t *reset = lv_label_create(panel);
    lv_obj_set_style_text_color(reset, CM_DIM, 0);
    lv_obj_set_style_text_font(reset, &font_styrene_28, 0);
    lv_label_set_text(reset, "Waiting for data");
    lv_obj_set_pos(reset, 0, 78);
    *out_reset = reset;
}

static void usage_create(lv_obj_t *parent)
{
    usage_data_t *d = &s_usage;
    memset(d, 0, sizeof(*d));

    ensure_img_dsc_ready();

    /* Logo */
    lv_obj_t *logo = lv_img_create(parent);
    lv_img_set_src(logo, &s_logo_dsc);
    lv_obj_set_pos(logo, 20, 20);

    lv_obj_t *title = lv_label_create(parent);
    lv_obj_set_style_text_color(title, CM_TEXT, 0);
    lv_obj_set_style_text_font(title, &font_tiempos_56, 0);
    lv_label_set_text(title, "Usage");
    lv_obj_align(title, LV_ALIGN_TOP_MID, 16, 26);

    usage_make_panel(parent, 100, "Current", &d->session_pct, &d->session_bar, &d->session_reset);
    usage_make_panel(parent, 250, "Weekly",  &d->weekly_pct,  &d->weekly_bar,  &d->weekly_reset);

    d->anim_label = lv_label_create(parent);
    lv_obj_set_style_text_color(d->anim_label, CM_ACCENT, 0);
    lv_obj_set_style_text_font(d->anim_label, &font_mono_32, 0);
    lv_obj_set_width(d->anim_label, CM_SCR_W - 40);
    lv_label_set_long_mode(d->anim_label, LV_LABEL_LONG_DOT);
    lv_obj_set_style_text_align(d->anim_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_label_set_text(d->anim_label, ". Accomplishing...");
    lv_obj_align(d->anim_label, LV_ALIGN_BOTTOM_MID, 0, -22);
}

static void usage_tick(uint32_t elapsed_ms)
{
    usage_data_t *d = &s_usage;
    if (!d->anim_label) return;

    d->spinner_elapsed_ms += elapsed_ms;
    d->message_elapsed_ms += elapsed_ms;

    if (d->message_elapsed_ms >= 4000U) {
        d->message_elapsed_ms = 0;
        d->message_idx = (uint8_t)((d->message_idx + 1U) %
                                    (sizeof(s_status_messages) / sizeof(s_status_messages[0])));
    }
    if (d->spinner_elapsed_ms >= 180U) {
        d->spinner_elapsed_ms = 0;
        d->spinner_idx = (uint8_t)((d->spinner_idx + 1U) %
                                    (sizeof(s_spinner_frames) / sizeof(s_spinner_frames[0])));
        lv_label_set_text_fmt(d->anim_label, "%s %s...",
                              s_spinner_frames[d->spinner_idx],
                              s_status_messages[d->message_idx]);
    }
}

const cm_page_ops_t cm_page_usage = {
    .name   = "Usage",
    .create = usage_create,
    .tick   = usage_tick,
};

/* ════════════════════════════════════════════════════════════════════
 *  BLUETOOTH PAGE — Connection status (stub, no BLE yet)
 * ════════════════════════════════════════════════════════════════════ */

typedef struct {
    lv_obj_t *status;
    lv_obj_t *enabled;
    lv_obj_t *device;
} ble_data_t;

static ble_data_t s_ble;

static void bluetooth_create(lv_obj_t *parent)
{
    ble_data_t *d = &s_ble;
    memset(d, 0, sizeof(*d));

    ensure_img_dsc_ready();

    lv_obj_t *title = lv_label_create(parent);
    lv_obj_set_style_text_color(title, CM_TEXT, 0);
    lv_obj_set_style_text_font(title, &font_tiempos_56, 0);
    lv_label_set_text(title, "Bluetooth");
    lv_obj_align(title, LV_ALIGN_TOP_MID, 16, 28);

    lv_obj_t *panel = cm_make_panel(parent, 20, 106, 350, 160);

    lv_obj_t *icon = lv_img_create(panel);
    lv_img_set_src(icon, &s_icon_bluetooth_dsc);
    lv_obj_set_pos(icon, 0, 0);

    d->status = lv_label_create(panel);
    lv_obj_set_style_text_font(d->status, &font_styrene_28, 0);
    lv_obj_set_style_text_color(d->status, CM_RED, 0);
    lv_label_set_text(d->status, "Disconnected");
    lv_obj_set_pos(d->status, 62, 8);

    d->enabled = lv_label_create(panel);
    lv_obj_set_style_text_color(d->enabled, CM_DIM, 0);
    lv_obj_set_style_text_font(d->enabled, &font_styrene_20, 0);
    lv_label_set_text(d->enabled, "Enabled: no");
    lv_obj_set_pos(d->enabled, 0, 64);

    d->device = lv_label_create(panel);
    lv_obj_set_style_text_color(d->device, CM_DIM, 0);
    lv_obj_set_style_text_font(d->device, &font_styrene_20, 0);
    lv_label_set_text(d->device, "Device: N/A");
    lv_obj_set_pos(d->device, 0, 100);

    lv_obj_t *credit = lv_label_create(parent);
    lv_obj_set_style_text_color(credit, CM_DIM, 0);
    lv_obj_set_style_text_font(credit, &font_styrene_14, 0);
    lv_label_set_text(credit, "Clawdmeter port scaffold");
    lv_obj_align(credit, LV_ALIGN_BOTTOM_MID, 0, -24);
}

static void bluetooth_tick(uint32_t elapsed_ms)
{
    (void)elapsed_ms;
}

const cm_page_ops_t cm_page_bluetooth = {
    .name   = "Bluetooth",
    .create = bluetooth_create,
    .tick   = bluetooth_tick,
};
