/*
 * Copyright (c) 2015, Devan Lai
 *
 * Permission to use, copy, modify, and/or distribute this software
 * for any purpose with or without fee is hereby granted, provided
 * that the above copyright notice and this permission notice
 * appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
 * WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE
 * AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT,
 * NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <stdio.h>
#include <string.h>

#include <libopencm3/cm3/nvic.h>
#include <libopencm3/stm32/desig.h>
#include <libopencm3/stm32/iwdg.h>

#include "config.h"
#include "target.h"

#include "USB/composite_usb_conf.h"
#include "USB/dfu.h"
#include "USB/winusb.h"

#include "DAP/app.h"
#include "DAP/CMSIS_DAP_hal.h"
#include "DFU/DFU.h"

#include "tick.h"
#include "retarget.h"

extern void initialise_monitor_handles(void);

static inline uint32_t millis(void)
{
    return get_ticks();
}

static inline void wait_ms(uint32_t duration_ms)
{
    uint32_t now = millis();
    uint32_t end = now + duration_ms;
    if (end < now)
    {
        end = 0xFFFFFFFFU - end;
        while (millis() >= now)
        {
            __asm__("NOP");
        }
    }

    while (millis() < end)
    {
        __asm__("NOP");
    }
}

static bool do_reset_to_dfu = false;
static void on_dfu_request(void)
{
    do_reset_to_dfu = true;
}

usbd_device *usbd_dev = NULL;
void USB_IRQ_NAME(void)
{
    usbd_poll(usbd_dev);
}

int main(void)
{
    if (DFU_AVAILABLE)
    {
        DFU_maybe_jump_to_bootloader();
    }

    cpu_setup();
    clock_setup();
    tick_setup(1000);
    gpio_setup();
    led_num(0);

    if (SEMIHOSTING)
    {
        initialise_monitor_handles();
    }

    led_num(1);

    {
        char serial[USB_SERIAL_NUM_LENGTH + 1];
        desig_get_unique_id_as_string(serial, USB_SERIAL_NUM_LENGTH + 1);
        cmp_set_usb_serial_number(serial);
        DAP_app_set_serial_number(serial);
    }

    usbd_dev = cmp_usb_setup();
    DAP_app_setup(usbd_dev, &on_dfu_request);

    if (DFU_AVAILABLE)
    {
        dfu_setup(usbd_dev, on_dfu_request);
    }

    if (WINUSB_AVAILABLE && (DFU_AVAILABLE || BULK_AVAILABLE))
    {
        winusb_setup(usbd_dev);
    }

    tick_start();

    // /* Enable the watchdog to enable DFU recovery from bad firmware images */
    // iwdg_set_period_ms(1000);
    // iwdg_start();

    /* Enable USB */
    nvic_enable_irq(USB_NVIC_LINE);

    while (1)
    {
        iwdg_reset();

        // Handle DAP
        DAP_app_update();
        if (do_reset_to_dfu && DFU_AVAILABLE)
        {
            /* Blink 3 times to indicate reset */
            int x;
            for (x = 0; x < 3; x++)
            {
                iwdg_reset();
                led_num(7);
                wait_ms(150);
                led_num(0);
                wait_ms(150);
                iwdg_reset();
            }

            DFU_reset_and_jump_to_bootloader();
        }
    }

    return 0;
}
