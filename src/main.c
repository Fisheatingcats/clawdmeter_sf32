#include "rtthread.h"
#include "bf0_hal.h"
#include "drv_io.h"
#include "stdio.h"
#include "string.h"
#include "board.h"

#define DBG_TAG "main"
#define DBG_LVL DBG_LOG
#include <rtdbg.h>

#ifdef PKG_USING_LITTLEVGL2RTT
#include "littlevgl2rtt.h"
#include "lvgl.h"
#include "lv_ex_data.h"
#endif

#include "clawdmeter_ui.h"
#include "clawdmeter_ble.h"

int main(void)
{
    rt_kprintf("=== Clawdmeter ===\n");

    /* Start BLE early so the SDK mbox thread is allocated before UI setup. */
    if (cm_ble_init() == 0) {
        rt_kprintf("BLE init OK\n");
    } else {
        rt_kprintf("BLE init failed\n");
    }

#ifdef PKG_USING_LITTLEVGL2RTT
    /* Initialise LVGL display + input via SiFli SDK wrapper */
    littlevgl2rtt_init("lcd");
    lv_ex_data_pool_init();

    /* Build the 3-page Clawdmeter UI on the active screen */
    clawdmeter_ui_init(lv_scr_act());
    rt_kprintf("Clawdmeter UI ready\n");
#endif

    /* Main loop: drive BLE and LVGL handlers. */
    while (1) {
        cm_ble_poll();
#ifdef PKG_USING_LITTLEVGL2RTT
        lv_task_handler();
#endif
        rt_thread_mdelay(5);
    }
    return 0;
}
