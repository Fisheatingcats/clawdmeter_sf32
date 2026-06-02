#include "rtthread.h"
#include "bf0_hal.h"
#include "drv_io.h"
#include "stdio.h"
#include "string.h"
#include "board.h"

#define DBG_TAG "main"
#define DBG_LVL DBG_LOG
#include <rtdbg.h>

#include "clawdmeter_ble.h"

#ifdef PKG_USING_LITTLEVGL2RTT
#include "littlevgl2rtt.h"
#include "lvgl.h"
#include "lv_ex_data.h"
#include "clawdmeter_ui.h"
#endif /* PKG_USING_LITTLEVGL2RTT */

/* ════════════════════════════════════════════════════════════════════
 *  LCPU Bluetooth configuration (required for BLE on SF32LB52X)
 * ════════════════════════════════════════════════════════════════════ */
#ifdef SF32LB52X_58
static uint16_t g_em_offset[HAL_LCPU_CONFIG_EM_BUF_MAX_NUM] =
{
    0x178, 0x178, 0x740, 0x7A0, 0x810, 0x880, 0xA00, 0xBB0, 0xD48,
    0x133C, 0x13A4, 0x19BC, 0x21BC, 0x21BC, 0x21BC, 0x21BC, 0x21BC, 0x21BC,
    0x21BC, 0x21BC, 0x263C, 0x265C, 0x2734, 0x2784, 0x28D4, 0x28E8, 0x28FC,
    0x29EC, 0x29FC, 0x2BBC, 0x2BD8, 0x3BE8, 0x5804, 0x5804, 0x5804
};

void lcpu_rom_config(void)
{
    hal_lcpu_bluetooth_em_config_t em_offset;
    memcpy((void *)em_offset.em_buf, (void *)g_em_offset,
           HAL_LCPU_CONFIG_EM_BUF_MAX_NUM * 2);
    em_offset.is_valid = 1;
    HAL_LCPU_CONFIG_set(HAL_LCPU_CONFIG_BT_EM_BUF, &em_offset,
                        sizeof(hal_lcpu_bluetooth_em_config_t));

    hal_lcpu_bluetooth_act_configt_t act_cfg;
    act_cfg.ble_max_act = 6;
    act_cfg.ble_max_iso = 0;
    act_cfg.ble_max_ral = 3;
    act_cfg.bt_max_acl = 7;
    act_cfg.bt_max_sco = 0;
    act_cfg.bit_valid = CO_BIT(0) | CO_BIT(1) | CO_BIT(2) | CO_BIT(3) | CO_BIT(4);
    HAL_LCPU_CONFIG_set(HAL_LCPU_CONFIG_BT_ACT_CFG, &act_cfg,
                        sizeof(hal_lcpu_bluetooth_act_configt_t));
}
#endif /* SF32LB52X_58 */

/**
 * @brief  Main program
 */
int main(void)
{
    rt_kprintf("ClawdMeter started!\n");

    /* Initialise BLE GATT service (registers after BLE power-on) */
    cm_ble_init();
    rt_kprintf("ClawdMeter BLE init\n");

#ifdef PKG_USING_LITTLEVGL2RTT
    rt_err_t ret;
    rt_uint32_t ms;

    ret = littlevgl2rtt_init("lcd");
    if (ret != RT_EOK)
    {
        rt_kprintf("LVGL init failed: %d\n", ret);
        return ret;
    }

    lv_ex_data_pool_init();

    /* Initialise Clawdmeter UI on the active screen */
    clawdmeter_ui_init(lv_scr_act());
    rt_kprintf("ClawdMeter UI initialised\n");

    while (1)
    {
        /* Process BLE events */
        cm_ble_poll();

        ms = lv_task_handler();
        rt_thread_mdelay(ms > 0 ? ms : 1);
    }
#else
    while (1)
    {
        cm_ble_poll();
        rt_thread_mdelay(1000);
        rt_kprintf("ClawdMeter running...\n");
    }
#endif

    return 0;
}
