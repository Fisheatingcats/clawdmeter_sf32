#include "clawdmeter_data.h"

#include <stdio.h>
#include <string.h>
#include "rtthread.h"
#include "cJSON.h"

/* ── Rate detection (ring buffer over 4-minute window) ── */

#define RATE_THRESH_NORMAL  0.10f
#define RATE_THRESH_ACTIVE  0.20f
#define RATE_THRESH_HEAVY   0.33f
#define MIN_WINDOW_MS       240000ULL
#define RING_SIZE           6U

typedef struct {
    uint32_t ms;
    float    pct;
} rate_sample_t;

static struct rt_mutex      s_lock;
static bool                 s_lock_inited;
static cm_usage_data_t      s_usage;
static rate_sample_t        s_ring[RING_SIZE];
static uint8_t              s_ring_count;
static uint8_t              s_ring_head;

static void ensure_lock(void)
{
    if (!s_lock_inited) {
        rt_mutex_init(&s_lock, "cm_data", RT_IPC_FLAG_FIFO);
        s_lock_inited = true;
    }
}

static uint32_t now_ms(void)
{
    return (uint32_t)(rt_tick_get_millisecond());
}

static void rate_reset(void)
{
    s_ring_count = 0;
    s_ring_head  = 0;
}

static void rate_sample(float session_pct, uint32_t ts)
{
    if (s_ring_count > 0) {
        uint8_t latest = (uint8_t)((s_ring_head + RING_SIZE - 1U) % RING_SIZE);
        if (session_pct + 5.0f < s_ring[latest].pct) {
            rate_reset();
        }
    }
    s_ring[s_ring_head].ms  = ts;
    s_ring[s_ring_head].pct = session_pct;
    s_ring_head = (uint8_t)((s_ring_head + 1U) % RING_SIZE);
    if (s_ring_count < RING_SIZE) s_ring_count++;
}

static int rate_group(void)
{
    if (s_ring_count < 2) return 0;
    uint8_t oldest = (uint8_t)((s_ring_head + RING_SIZE - s_ring_count) % RING_SIZE);
    uint8_t latest = (uint8_t)((s_ring_head + RING_SIZE - 1U) % RING_SIZE);
    uint32_t dt = s_ring[latest].ms - s_ring[oldest].ms;
    if (dt < MIN_WINDOW_MS) return 0;
    float dp = s_ring[latest].pct - s_ring[oldest].pct;
    if (dp < 0.0f) dp = 0.0f;
    float rate = dp * 60000.0f / (float)dt;
    if (rate < RATE_THRESH_NORMAL) return 0;
    if (rate < RATE_THRESH_ACTIVE) return 1;
    if (rate < RATE_THRESH_HEAVY)  return 2;
    return 3;
}

/* ── JSON helpers ── */

static float jf(const cJSON *r, const char *k, float fb)
{
    const cJSON *i = cJSON_GetObjectItemCaseSensitive(r, k);
    return cJSON_IsNumber(i) ? (float)i->valuedouble : fb;
}

static int ji(const cJSON *r, const char *k, int fb)
{
    const cJSON *i = cJSON_GetObjectItemCaseSensitive(r, k);
    return cJSON_IsNumber(i) ? i->valueint : fb;
}

static bool jhas(const cJSON *r, const char *k)
{
    return cJSON_GetObjectItemCaseSensitive(r, k) != NULL;
}

static void jstr(const cJSON *r, const char *k, char *out, size_t len)
{
    const cJSON *i = cJSON_GetObjectItemCaseSensitive(r, k);
    if (cJSON_IsString(i) && i->valuestring)
        snprintf(out, len, "%s", i->valuestring);
}

/* ── Public API ── */

int cm_data_update_json(const char *json, size_t len)
{
    if (!json || len == 0 || len > CM_JSON_MAX_LEN) return -1;

    ensure_lock();

    cJSON *doc = cJSON_ParseWithLength(json, len);
    if (!doc || !cJSON_IsObject(doc)) {
        cJSON_Delete(doc);
        return -1;
    }

    rt_mutex_take(&s_lock, RT_WAITING_FOREVER);
    cm_usage_data_t p = s_usage;
    rt_mutex_release(&s_lock);

    bool has_session = jhas(doc, "s");
    p.session_pct       = jf(doc, "s",  p.session_pct);
    p.session_reset_mins = ji(doc, "sr", p.session_reset_mins);
    p.weekly_pct        = jf(doc, "w",  p.weekly_pct);
    p.weekly_reset_mins = ji(doc, "wr", p.weekly_reset_mins);

    if (jhas(doc, "ok"))
        p.ok = cJSON_IsTrue(cJSON_GetObjectItemCaseSensitive(doc, "ok"));

    if (!p.valid) {
        p.ok = jhas(doc, "ok") ? p.ok : true;
        snprintf(p.status, sizeof(p.status), "unknown");
        p.session_reset_mins = jhas(doc, "sr") ? p.session_reset_mins : -1;
        p.weekly_reset_mins  = jhas(doc, "wr") ? p.weekly_reset_mins : -1;
        p.override_anim_group = -1;
    }
    p.valid = true;

    jstr(doc, "st",     p.status,  sizeof(p.status));
    jstr(doc, "agent",  p.agent,   sizeof(p.agent));
    jstr(doc, "msg",    p.message, sizeof(p.message));
    jstr(doc, "level",  p.level,   sizeof(p.level));

    if (jhas(doc, "anim")) {
        const cJSON *ai = cJSON_GetObjectItemCaseSensitive(doc, "anim");
        if (cJSON_IsNumber(ai)) {
            int v = ai->valueint;
            p.override_anim_group = (int8_t)((v >= 0 && v <= 3) ? v : -1);
            p.anim_name[0] = '\0';
        } else if (cJSON_IsNull(ai) ||
                   (cJSON_IsString(ai) && strcmp(ai->valuestring, "auto") == 0)) {
            p.override_anim_group = -1;
            p.anim_name[0] = '\0';
        }
    }
    if (jhas(doc, "anim_name")) {
        const cJSON *ni = cJSON_GetObjectItemCaseSensitive(doc, "anim_name");
        if (cJSON_IsString(ni) && ni->valuestring) {
            snprintf(p.anim_name, sizeof(p.anim_name), "%s", ni->valuestring);
            p.override_anim_group = -1;
        } else if (cJSON_IsNull(ni)) {
            p.anim_name[0] = '\0';
        }
    }

    cJSON_Delete(doc);

    uint32_t ts = now_ms();
    rt_mutex_take(&s_lock, RT_WAITING_FOREVER);
    p.generation = s_usage.generation + 1U;
    s_usage = p;
    if (has_session) rate_sample(p.session_pct, ts);
    rt_mutex_release(&s_lock);
    return 0;
}

bool cm_data_get(cm_usage_data_t *out)
{
    if (!out) return false;
    ensure_lock();
    rt_mutex_take(&s_lock, RT_WAITING_FOREVER);
    *out = s_usage;
    bool v = s_usage.valid;
    rt_mutex_release(&s_lock);
    return v;
}

uint32_t cm_data_generation(void)
{
    ensure_lock();
    rt_mutex_take(&s_lock, RT_WAITING_FOREVER);
    uint32_t g = s_usage.generation;
    rt_mutex_release(&s_lock);
    return g;
}

bool cm_data_get_anim_name(char *out, size_t out_len)
{
    if (!out || out_len == 0) return false;
    ensure_lock();
    rt_mutex_take(&s_lock, RT_WAITING_FOREVER);
    bool has = s_usage.anim_name[0] != '\0';
    if (has) snprintf(out, out_len, "%s", s_usage.anim_name);
    rt_mutex_release(&s_lock);
    if (!has) out[0] = '\0';
    return has;
}
