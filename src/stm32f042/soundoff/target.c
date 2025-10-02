/*
 * Copyright (c) 2016, Devan Lai
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

#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/crs.h>
#include <libopencm3/stm32/flash.h>
#include <libopencm3/stm32/gpio.h>

#include "target.h"
#include "config.h"

static void writel(uint32_t address, uint32_t value)
{
    *(volatile uint32_t *)address = value;
}

static void writeh(uint32_t address, uint16_t value)
{
    *(volatile uint16_t *)address = value;
}

/* Reconfigure processor settings */
void cpu_setup(void)
{
    /* Ensure FLASH_OBR_BOOT_SEL is clear, because BOOT0 is
     * shorted to GND on this board.
     */
    if (FLASH_OBR & FLASH_OBR_BOOT_SEL)
    {
        /* Sequence adapted from:
         * https://iwasz.pl/electronics/2020-07-29-stm32f042-option-bytes-linux/
         */

        /* Unlock flash bytes */
        writel(0x40022004, 0x45670123);
        writel(0x40022004, 0xCDEF89AB);

        /* Unlock flash option bytes */
        writel(0x40022008, 0x45670123);
        writel(0x40022008, 0xCDEF89AB);

        /* Set the option to "erase" */
        writel(0x40022010, 0x00000220);
        /* Set the "start" bit */
        writel(0x40022010, 0x00000260);
        flash_wait_for_last_operation();
        writel(0x40022010, 0x00000210);

        /* Write the actual values */
        writeh(0x1ffff800, 0x55AA);
        writeh(0x1ffff802, 0x807f);

        /* The new values will take effect after the next power cycle */
    }
}

/* Set STM32 to 48 MHz. */
void clock_setup(void)
{
    rcc_clock_setup_in_hsi48_out_48mhz();

    // Trim from USB sync frame
    crs_autotrim_usb_enable();
    rcc_set_usbclk_source(RCC_HSI48);
}

static void relay_power(bool on) {
    if (on) {
        gpio_mode_setup(GPIOA, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO1);
        // gpio_set(GPIOA, GPIO1);
    } else {
        gpio_mode_setup(GPIOA, GPIO_MODE_INPUT, GPIO_PUPD_NONE, GPIO1);
        // gpio_clear(GPIOA, GPIO1);
    }
}

static void led_power(bool on) {
    if (on) {
        gpio_clear(GPIOB, GPIO1);
    } else {
        gpio_set(GPIOB, GPIO1);
    }
}

void gpio_setup(void)
{

    /* Enable GPIOA and GPIOB clocks. */
    rcc_periph_clock_enable(RCC_GPIOA);
    rcc_periph_clock_enable(RCC_GPIOB);

    /* Setup LED as an open-drain output */
    gpio_set_output_options(GPIOB, GPIO_OTYPE_OD, GPIO_OSPEED_LOW, GPIO1);
    gpio_mode_setup(GPIOB, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO1);
    led_power(false);

    /* Setup optical relay as open-source output */
    gpio_set_output_options(GPIOA, GPIO_OTYPE_PP, GPIO_OSPEED_LOW, GPIO1);
    gpio_mode_setup(GPIOA, GPIO_MODE_INPUT, GPIO_PUPD_NONE, GPIO1);
    gpio_set(GPIOA, GPIO1);
    relay_power(false);
}

void controlled_power_on(void)
{
    led_power(true);
    relay_power(true);
}

void controlled_power_off(void)
{
    led_power(false);
    relay_power(false);
}
