#include "usbh_core.h"
#include "usbh_hid.h"
#include "usbh_standard_hid.h"

USB_NOCACHE_RAM_SECTION USB_MEM_ALIGNX uint8_t hid_buffer[128];

extern rt_mailbox_t key_mb;
static uint8_t key_buffer[6] = {0};

static void keyboard_scan(uint8_t *buf, int nbytes) {
    if (key_mb == RT_NULL) return;

    int max_len = nbytes - 2;
    if (max_len > sizeof(key_buffer)) {
        max_len = sizeof(key_buffer);
    }

    for (int i = 0; i < max_len; i++) {
        uint32_t key_data = 0;

        if (buf[i + 2] != key_buffer[i]) {
            if (buf[i + 2] != 0) {
                key_data = buf[i + 2];
                key_data |= 1 << 8;
            } else {
                key_data = key_buffer[i];
            }

            rt_mb_send(key_mb, key_data);

            key_buffer[i] = buf[i + 2];
        }
    }
}

void usbh_hid_callback(void *arg, int nbytes)
{
    struct usbh_hid *hid_class = (struct usbh_hid *)arg;

    if (nbytes > 0) {
        // for (int i = 0; i < nbytes; i++) {
        //     USB_LOG_RAW("0x%02x ", hid_buffer[i]);
        // }
        // USB_LOG_RAW("nbytes:%d\r\n", nbytes);
        if (hid_class->protocol == HID_PROTOCOL_MOUSE) {
            if (nbytes == 8 || nbytes == 13)
                usbh_hid_mouse_decode(hid_buffer);
        } else if (hid_class->protocol == HID_PROTOCOL_KEYBOARD) {
            keyboard_scan(hid_buffer, nbytes);
        }

        usbh_int_urb_fill(&hid_class->intin_urb, hid_class->hport, hid_class->intin, hid_buffer, hid_class->intin->wMaxPacketSize, 0, usbh_hid_callback, hid_class);
        usbh_submit_urb(&hid_class->intin_urb);
    } else if (nbytes == -USB_ERR_NAK) { /* only dwc2 should do this */
        usbh_int_urb_fill(&hid_class->intin_urb, hid_class->hport, hid_class->intin, hid_buffer, hid_class->intin->wMaxPacketSize, 0, usbh_hid_callback, hid_class);
        usbh_submit_urb(&hid_class->intin_urb);
    } else {
    }
}

static void usbh_hid_thread(CONFIG_USB_OSAL_THREAD_SET_ARGV)
{
    int ret;
    struct usbh_hid *hid_class = (struct usbh_hid *)CONFIG_USB_OSAL_THREAD_GET_ARGV;
    ;

    usbh_hid_mouse_init();
    usbh_hid_keybrd_init();
    /* test with only one buffer, if you have more hid class, modify by yourself */

    /* Suggest you to use timer for int transfer and use ep interval */
    usbh_int_urb_fill(&hid_class->intin_urb, hid_class->hport, hid_class->intin, hid_buffer, hid_class->intin->wMaxPacketSize, 0, usbh_hid_callback, hid_class);
    ret = usbh_submit_urb(&hid_class->intin_urb);
    if (ret < 0) {
        goto delete;
    }
    // clang-format off
delete:
    usb_osal_thread_delete(NULL);
    // clang-format on
}

void usbh_hid_run(struct usbh_hid *hid_class)
{
    usb_osal_thread_create("usbh_hid", 2048, CONFIG_USBHOST_PSC_PRIO + 1, usbh_hid_thread, hid_class);
}

void usbh_hid_stop(struct usbh_hid *hid_class)
{
}
