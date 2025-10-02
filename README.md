# Soundoff

Soundoff is a simple project that uses an STM32F042 chip to toggle power to a USB port. When the host goes to sleep, power to the port is shut down.

The original goal of this project was to turn off power to Bluetooth speakers that normally stay on even when the host is asleep.

This repository is heavily based on the [dap42 project](https://github.com/devanlai/dap42), which is a full CMSIS-DAP debug firmware for STM32F042 and STM32F103 parts. This project removes support for all other targets and only enumerates as a USB control device with DFU support.

## Flash instructions
The default method to upload new firmware is via [dfu-util](http://dfu-util.sourceforge.net/). The Makefile includes the `dfuse-flash` target to invoke dfu-util. dfu-util automatically detaches the dap42 firmware and uploads the firmware through the on-chip bootloader.

To flash via another debugger, use `make flash`.

For detailed flashing instructions, see [FLASHING.md](./FLASHING.md)

## Acknowledgements
The dap42 USB VID/PID pair is [1209/DA42](http://pid.codes/1209/DA42/), allocated through the [pid.codes](http://pid.codes/) open-source USB PID program.

## Licensing
All contents of the dap42 project are licensed under terms that are compatible with the terms of the GNU Lesser General Public License version 3.

Non-libopencm3 related portions of the dap42 project are licensed under the less restrictive ISC license, except where otherwise specified in the headers of specific files.

See the LICENSE file for full details.
