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
#include "clawdmeter_ui.h"
#endif /* PKG_USING_LITTLEVGL2RTT */

/**
 * @brief  Main program
 */
int main(void)
{
    rt_kprintf("ClawdMeter started!\n");

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
        ms = lv_task_handler();
        rt_thread_mdelay(ms > 0 ? ms : 1);
    }
#else
    while (1)
    {
        rt_thread_mdelay(1000);
        rt_kprintf("ClawdMeter running...\n");
    }
#endif

    return 0;
}
