#ifndef RTCONFIG_PROJECT_H__
#define RTCONFIG_PROJECT_H__

#if defined(_MSC_VER)
    #define RT_HEAP_SIZE   (680000)
    #define NORESOURCE
    #define _CRT_ERRNO_DEFINED
    #define _INC_WTIME_INL
    #define _INC_TIME_INL

    #pragma warning(disable:4273)
    #pragma warning(disable:4312)
    #pragma warning(disable:4311)
    #pragma warning(disable:4996)
    #pragma warning(disable:4267)
    #pragma warning(disable:4244)

#endif /* end of _MSC_VER */

/* BLE host configuration. Keep this HCPU-only; LCPU builds the controller image. */
#if defined(BF0_HCPU)
#ifndef BLUETOOTH
#define BLUETOOTH           1
#endif
#ifndef CFG_BLE
#define CFG_BLE             1
#endif
#ifndef BSP_BLE_SIBLES
#define BSP_BLE_SIBLES      1
#endif
#endif

#endif
