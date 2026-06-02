#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define CM_JSON_MAX_LEN       512U
#define CM_STATUS_MAX_LEN     16U
#define CM_AGENT_MAX_LEN      16U
#define CM_MESSAGE_MAX_LEN    96U
#define CM_LEVEL_MAX_LEN      12U
#define CM_ANIM_NAME_MAX_LEN  24U

typedef struct {
    float    session_pct;
    int      session_reset_mins;
    float    weekly_pct;
    int      weekly_reset_mins;
    char     status[CM_STATUS_MAX_LEN];
    char     agent[CM_AGENT_MAX_LEN];
    char     message[CM_MESSAGE_MAX_LEN];
    char     level[CM_LEVEL_MAX_LEN];
    bool     ok;
    bool     valid;
    int8_t   override_anim_group;   /* -1 = auto, 0..3 = forced */
    char     anim_name[CM_ANIM_NAME_MAX_LEN];
    uint32_t generation;
} cm_usage_data_t;

/**
 * Parse JSON from BLE write and update the shared data.
 * Returns 0 on success, -1 on error.
 */
int cm_data_update_json(const char *json, size_t len);

/**
 * Copy current usage data to out_data.
 * Returns true if data is valid.
 */
bool cm_data_get(cm_usage_data_t *out_data);

/**
 * Monotonic generation counter (increments on each update).
 */
uint32_t cm_data_generation(void);

/**
 * Get the current animation name (if set).
 * Returns true if a named animation is active.
 */
bool cm_data_get_anim_name(char *out, size_t out_len);
