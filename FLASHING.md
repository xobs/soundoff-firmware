# Flashing instructions
## Overview
Depending on the board, there are a few different options for flashing firmware.

### SWD
If you already have a working debugger, you can always use that debugger to connect to the SWD pins on your board and reflash it. You may need an adapter PCB or jumpers to connect to the SWD pins.

### On-chip USB bootloader
The STM32F042 series has an on-chip USB [DFuSe](http://dfu-util.sourceforge.net/dfuse.html) bootloader. As it's part of the chip's ROM, it can't be overwritten, so it's almost always accessible. It will automatically run if the chip is blank or if the `BOOT0` pin is pulled high.
Once the main firmware has been flashed, it can retrigger the on-chip bootloader from USB to facilitate firmware updates without needing to touch the `BOOT0` pin.

### dfu-util
[dfu-util](http://dfu-util.sourceforge.net/) is a cross-platform open-source command-line utility that understands the USB DFU protocol.
It can handle discovering USB DFU devices, starting the bootloader from USB, and reading/writing firmware.

On Windows, if you plan to use `dfu-util`, you will need to use [Zadig](http://zadig.akeo.ie/) to install the WinUSB or libusb driver for the bootloader.

Let's start by verifying that `dfu-util` can detect the board:

    user@host:~/dap42/src$ dfu-util --list
    dfu-util 0.8
    
    Copyright 2005-2009 Weston Schmidt, Harald Welte and OpenMoko Inc.
    Copyright 2010-2014 Tormod Volden and Stefan Schmidt
    This program is Free Software and has ABSOLUTELY NO WARRANTY
    Please report bugs to http://sourceforge.net/p/dfu-util/tickets/
    
    Found DFU: [0483:df11] ver=2200, devnum=100, cfg=1, intf=0, alt=1, name="@Option Bytes  /0x1FFFF800/01*016 e", serial="FFFFFFFEFFFF"
    Found DFU: [0483:df11] ver=2200, devnum=100, cfg=1, intf=0, alt=0, name="@Internal Flash  /0x08000000/032*0001Kg", serial="FFFFFFFEFFFF"

If the chip is blank or you otherwise forced it into the bootloader, there should be two DFU interfaces - the one that we want is "Internal Flash".
If you're upgrading from a previous version of the debugger firmware, you might see one interface instead:

    Found Runtime: [1209:da42] ver=0120, devnum=3, cfg=1, intf=3, alt=0, name="DAP42 DFU", serial="383037201943425635001600"

If you're on Linux and you get something like `Cannot open DFU device 0483:df11` you may need to fix up the device permissions or use `sudo`.

You can use the `make dfuse-flash` target to flash the firmware via the bootloader:  

    user@host:~/dap42/src$ make dfuse-flash
    dfu-util -d 1209:da42,0483:df11  -a 0 -s 0x08000000:leave -D DAP42.bin
    dfu-util 0.8
    dfu-util: Invalid DFU suffix signature
    dfu-util: A valid DFU suffix will be required in a future dfu-util release!!!
    Opening DFU capable USB device...
    ID 0483:df11
    Run-time device DFU version 011a
    Claiming USB DFU Interface...
    Setting Alternate Setting #0 ...
    Determining device status: state = dfuERROR, status = 10
    dfuERROR, clearing status
    Determining device status: state = dfuIDLE, status = 0
    dfuIDLE, continuing
    DFU mode device DFU version 011a
    Device returned transfer size 2048
    DfuSe interface name: "Internal Flash  "
    Downloading to address = 0x08000000, size = 15764
    Download	[=========================] 100%        15764 bytes
    Download done.
    File downloaded successfully
    Transitioning to dfuMANIFEST state

At this point, you should have a functional debugger. On Windows, you may need to install drivers for the USB-serial interface and the USB DFU interface (again), but the CMSIS-DAP interface should work.

### STMicro's DfuSe tools
If you want to use the official STMicro Windows driver for the bootloader, you must use STMicro's official tools:

http://www.st.com/en/development-tools/stsw-stm32080.html
