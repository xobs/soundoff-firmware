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

#include "DFU/DFU.h"

#include "tick.h"

// Shut down power if we don't receive a SOF packet
// within this time period.
#define SHUTDOWN_TIME_MS 1000

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

static volatile bool do_reset_to_dfu = false;
static void on_dfu_request(void)
{
    do_reset_to_dfu = true;
}

usbd_device *usbd_dev = NULL;
void USB_IRQ_NAME(void)
{
    usbd_poll(usbd_dev);
}

// The number of millis that we last saw a "SOF" packet.
static volatile uint32_t last_sof_millis = 0;

// Turn the power switch on. Additionally, reset any timer
// that is running that's trying to turn the switch off.
static void saw_sof(void)
{
    last_sof_millis = millis();
    controlled_power_on();
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
    

    {
        char serial[USB_SERIAL_NUM_LENGTH + 1];
        desig_get_unique_id_as_string(serial, USB_SERIAL_NUM_LENGTH + 1);
        cmp_set_usb_serial_number(serial);
    }

    usbd_dev = cmp_usb_setup();

    if (DFU_AVAILABLE)
    {
        dfu_setup(usbd_dev, on_dfu_request);
    }

    if (WINUSB_AVAILABLE && DFU_AVAILABLE)
    {
        winusb_setup(usbd_dev);
    }

    tick_start();
    usbd_register_sof_callback(usbd_dev, saw_sof);

    /* Enable the watchdog to enable DFU recovery from bad firmware images */
    iwdg_set_period_ms(1000);
    iwdg_start();

    /* Enable USB */
    nvic_enable_irq(USB_NVIC_LINE);

    while (1)
    {
        iwdg_reset();

        // Handle resetting to the bootloader
        if (do_reset_to_dfu && DFU_AVAILABLE)
        {
            DFU_reset_and_jump_to_bootloader();
        }

        // Turn off the power if it's exceeded the
        // shutdown timer.
        // Note that if this wraps, that will be fine since
        // we only turn ON the power when a SOF packet is
        // received.
        if ((millis() - last_sof_millis) > SHUTDOWN_TIME_MS)
        {
            controlled_power_off();
        }
    }

    return 0;
}
