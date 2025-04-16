#include <rtthread.h>
#include <stdint.h>
#include <board.h>
#include <rtdevice.h>
#include <drv_gpio.h>
#include <usbh_core.h>

//#define DRV_DEBUG
#define LOG_TAG             "usb_demo"
#include <drv_log.h>

extern int cdc_acm_demo(void);
extern int cdc_acm_multi_demo(void);
extern int msc_ram_demo(void);

void usb_demo_init(void)
{
#ifdef BSP_USING_USB_DEVICE
    cdc_acm_demo();
    // cdc_acm_multi_demo();
    // msc_ram_demo();
#else

    usbh_initialize(0, USB_OTG_HS_PERIPH_BASE);
#endif

}
