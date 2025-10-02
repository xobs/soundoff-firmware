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

void gpio_setup(void)
{

    /* Enable GPIOA and GPIOB clocks. */
    rcc_periph_clock_enable(RCC_GPIOA);
    rcc_periph_clock_enable(RCC_GPIOB);

    /* Setup LEDs as open-drain outputs */
    gpio_set_output_options(GPIOA, GPIO_OTYPE_OD, GPIO_OSPEED_LOW,
                            GPIO0 | GPIO1 | GPIO4);

    gpio_mode_setup(GPIOA, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE,
                    GPIO0 | GPIO1 | GPIO4);
}

void led_bit(uint8_t position, bool state)
{
    uint32_t gpio = 0xFFFFFFFFU;
    if (position == 0)
    {
        gpio = GPIO4;
    }
    else if (position == 1)
    {
        gpio = GPIO1;
    }
    else if (position == 2)
    {
        gpio = GPIO0;
    }

    if (gpio != 0xFFFFFFFFU)
    {
        if (state)
        {
            gpio_clear(GPIOA, gpio);
        }
        else
        {
            gpio_set(GPIOA, gpio);
        }
    }
}

void led_num(uint8_t value)
{
    if (value & 0x4)
    {
        gpio_clear(GPIOA, GPIO0);
    }
    else
    {
        gpio_set(GPIOA, GPIO0);
    }
    if (value & 0x2)
    {
        gpio_clear(GPIOA, GPIO1);
    }
    else
    {
        gpio_set(GPIOA, GPIO1);
    }
    if (value & 0x1)
    {
        gpio_clear(GPIOA, GPIO4);
    }
    else
    {
        gpio_set(GPIOA, GPIO4);
    }
}

void controlled_power_on(void)
{
    gpio_set(GPIOA, GPIO4);
}

void controlled_power_off(void)
{
    gpio_clear(GPIOA, GPIO4);
}
