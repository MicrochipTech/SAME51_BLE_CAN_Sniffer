# ATSAME51 BLE Peripheral Example : CAN Bus Sniffer

## Introduction

This document describes how to program a Microchip 32-bit Cortex-M4F MCU to connect to a Smartphone via BLE while monitoring messages received over its connected CAN bus. A sample smartphone app is provided to communicate with the MCU via BLE and display the CAN FD messages received by the MCU. A Host PC running a terminal emulator serves as a debug window to send/receive diagnostic messages to/from the Host MCU.

<img src=".//media/SAME51_BLE_CAN_BlockDiagram.png" width=700/>

## Table of Contents

- [Introduction](#introduction)
- [Hardware Requirements](#hardware-requirements)
- [Demo Platform Assembly](#demo-platform-assembly)
- [Software Requirements](#software-requirements)
- [Program Demo Firmware](#program-demo-firmware)
- [Testing Procedure](#testing-procedure)
- [Custom GATT Services](#custom-gatt-services)

## Hardware Requirements

* Base Adapter Board: ["Curiosity Nano Base for Click Boards"](https://www.microchip.com/en-us/development-tool/ac164162)

    <img src=".//media/curiosity-nano-adapter.jpg" width=375/>

* Host MCU Add-On Board: ["SAM E51 Curiosity Nano Evaluation Kit"](https://www.microchip.com/en-us/development-tool/ev76s68a)

    <img src=".//media/sam-e51-curiosity-nano.jpg" width=375/>

* BLE Module: ["RNBD451 Add-on Board"](https://www.microchip.com/en-us/development-tool/ev25f14a) board featuring Microchip's [RNBD451PE](https://www.microchip.com/en-us/product/RNBD451PE) certified BLE module

    <img src=".//media/rnbd451-add-on-board.jpg" width=250/>

* High Speed CAN FD Transceiver: ["ATA6563 Click"](https://www.mikroe.com/ata6563-click) featuring Microchip's [ATA6563](https://www.microchip.com/en-us/product/ata6563)

    <img src=".//media/ata6563-click.jpg" width=250/>

* USB-to-CAN Adapter for Host PC: ["PCAN-USB FD Adapter"](https://phytools.com/collections/peak-system-technik/products/pcan-usb-fd-adapter) manufactured by [phytools](https://phytools.com)

    <img src=".//media/PCAN-USB-FD_Adapter.png" width=300/>

    <img src=".//media/phytools_logo.png" width=300/>

## Demo Platform Assembly

1. Set the jumper on the RNBD451 Add-On Board so that the two pins furthest away from the module are shorted.

    <img src=".//media/HW_Setup_01.png" width=500/>

2. Install the RNBD451 Add-On Board onto mikroBUS socket #2 of the Curiosity Nano Base.

    <img src=".//media/HW_Setup_02.png" width=500/>

3. Connect the supplied 28-pin headers underneath the SAM E51 Curiosity Nano (soldering is optional) and then install the SAM E51 Curiosity Nano onto the 56-pin socket of the Curiosity Nano Base.

    <img src=".//media/HW_Setup_03.png" width=500/>

4. Connect the PC to the SAM E51 Curiosity Nano using a micro-USB cable and note the new Virtual COM port number that is associated with your connection (e.g. use the Windows Device Manager).

    <img src=".//media/HW_Setup_04.png" width=500/>

5. Install the ATA6563 Click board onto mikroBUS socket #1 of the Curiosity Nano Base.

    <img src=".//media/HW_Setup_05.png" width=500/>

6. Using a DB-9 female-to-female serial cable, connect the PCAN-USB FD Adapter to the ATA6563 Click board, then connect the adapter's USB cable to an available USB port of the Host PC.

    <img src=".//media/HW_Setup_06.png" width=500/>

7. Confirm that all boards and connections have been made for the complete hardware platform.

    <img src=".//media/HW_Setup_07.png" width=600/>

## Software Requirements

Embedded software development tools need to be installed in order to properly program the WBZ451 Curiosity Development Board and then provision/test it for use with Microsoft Azure IoT services.

1. Microchip `MPLAB X` tool chain for embedded code development on 32-bit architecture MCU/MPU platforms (made up of 3 major components)

    - [MPLAB X IDE](https://www.microchip.com/mplab/mplab-x-ide) (when prompted, enable the installation of the [MPLAB IPE](https://www.microchip.com/en-us/tools-resources/production/mplab-integrated-programming-environment) too)
        
        NOTE: This demonstration project was last tested successfully with MPLAB X **v6.20**. If a previous version of the IDE is needed, download it from the [MPLAB Development Ecosystem Downloads Archive](https://www.microchip.com/en-us/tools-resources/archives/mplab-ecosystem) (to fall back to the version Microchip successfully tested prior to release). 

    - [MPLAB XC32 Compiler](https://www.microchip.com/en-us/development-tools-tools-and-software/mplab-xc-compilers#tabs)

        NOTE: This demonstration project was tested successfully with XC32 **v4.45**, and in general should work with later versions of the compiler as they become available. If you encounter issues building the project with a newer version of the compiler, it is recommended to download the compiler version that was last tested successfully from the [MPLAB Development Ecosystem Downloads Archive](https://www.microchip.com/en-us/tools-resources/archives/mplab-ecosystem) (to fall back to the version Microchip successfully tested prior to release). 

     - [MPLAB Harmony Software Framework](https://microchipdeveloper.com/harmony3:mhc-overview)

2. [Git](https://git-scm.com) (a free and open source distributed version control system)

    - Download/install the latest version of [Git for Windows, macOS, or Linux](https://git-scm.com/downloads)

    - Verify working operation of the `Git Bash` prompt (e.g. for Windows: click `Start` > type `Git Bash`)

3. Any [Terminal Emulator](https://en.wikipedia.org/wiki/List_of_terminal_emulators) program of your choice

4. [Microchip Bluetooth Data](https://play.google.com/store/apps/details?id=com.microchip.bluetooth.data&hl=en_US&pli=1) smartphone app

5. PCAN-USB FD Adapter [Device Driver for Windows](https://www.peak-system.com/quick/DrvSetup) 

## Program Demo Firmware

1. Clone/download the MPLAB X demo project by issuing the following command line in a `Command Prompt` or `PowerShell` window.

    ```bash
    git clone https://github.com/MicrochipTech/SAME51_BLE_CAN_Sniffer.git
    ```

2. Launch the MPLAB X IDE.

3. Open the demo project:

    - From the MPLAB X mail toolbar, select `File > Open Project`
    - Navigate to the `SAME51_BLE_CAN_Sniffer\firmware` folder
    - Select (click on) the `sam_e51_cnano_rnbd451.X` project folder
    - Click on the `Open Project` button

4. Clean and build the project:

    - Click on the `Clean and Build Main Project` icon in the MPLAB X main toolbar

        <img src=".//media/mplab_clean_build.png" width=450/>

5. Program the ATSAME51 Host MCU:

    - Click on the Make and Program Device Main Project icon in the MPLAB X main toolbar 
        <img src=".//media/mplab_make_program.png" width=375/>

    - Verify that the programming phase was successful
        <img src=".//media/mplab_prog_success.png" width=600/>

6. Connect a smartphone to the RNBD451 BLE module using the Microchip Bluetooth Data (MBD) app:

    - Follow the procedure outlined in [Section 7.2.2.1](https://onlinedocs.microchip.com/oxy/GUID-26457D23-798C-47B0-9F40-C5DA6E995C6F-en-US-2/GUID-4D5EDD33-45BA-469D-BB9F-DE40465F8E82.html) ("Text Mode") of the [RNBD451 Bluetooth Low Energy User's Guide](https://onlinedocs.microchip.com/oxy/GUID-26457D23-798C-47B0-9F40-C5DA6E995C6F-en-US-2/index.html)

## Testing Procedure

1. Open a serial terminal window (e.g. TeraTerm) and connect to the COM port corresponding to your board at `115200 baud` (e.g. choose `File` &gt; choose `New connection`&gt; choose `Serial`&gt; select `Curiosity Virtual COM Port`). It is recommended to enable the "Local Echo" feature in the terminal settings.

    <img src=".//media/TeraTerm_settings.png" width=400/>

2. Change the toggle rate of `LED0` on the SAM E51 Curiosity Nano board:
    - Press the **SW0** button on the SAM E51 Curiosity Nano and note the LED toggle rate change on every press
    - View the toggle rate confirmation messages in the Microchip Bluetooth Data (MBD) smartphone app

3. Send text messages from the Host PC to the smartphone:

    - Type characters in the terminal window and confirm reception/display in the MBD app
        <img src=".//media/terminal_to_mbd.png" width=600/>
    - Type characters in the MBD app and confirm reception/display in the terminal window
        <img src=".//media/mbd_to_terminal.png" width=600/>

4. Launch the PCAN-View PC application.

5. In the `CAN Setup` tab, select `SAE J2284-4 (500k/2M)` for the Bit Rate Preset:
    <img src=".//media/PCAN-View_CAN_Setup.png" width=400/>

6. In the serial terminal window, type `M` or `m` to bring up the list of available CAN operations. Enter the keys `1`, `2`, `3`, `4`, & `5` to send out sample standard and extended CAN messages.
    <img src=".//media/CAN_demo_menu.png" width=600/>

7. In the PCAN-View GUI, confirm that 5 different CAN messages show up in the `Receive` window.
    <img src=".//media/PCAN-View_Receive.png" width=600/>

8. Create/transmit a new **standard** CAN message by selecting `File > New Message` in the PCAN-View's main toolbar. In the **New Transmit Message** pop-up window:

    - Enter an ID that's less than 0x7FF
    - Select the number of data bytes in the message and set the value of each byte as desired 
    - Under Message Type, ensure that only the boxes checked are for `CAN FD` and `Bit Rate Switch`
    - Enter a cycle time to send out the same message periodically (in milliseconds)
    - Click on the OK button to start the transmission of repeating messages

        <img src=".//media/PCAN-View_StandardMesg.png" width=400/>

9. Create/transmit a new **extended** CAN message by selecting `File > New Message` in the PCAN-View's main toolbar. In the **New Transmit Message** pop-up window:

    - Enter an ID that's greater than or equal to 0x10000
    - Select the number of data bytes in the message and set the value of each byte as desired
    - Under Message Type, ensure that the 3 boxes checked are `Extended Frame`, `CAN FD`, and `Bit Rate Switch`
    - Enter a cycle time to send out the same message periodically (in milliseconds)
    - Click on the OK button to start the transmission of repeating messages

        <img src=".//media/PCAN-View_ExtendedMesg.png" width=400/>

10. In the `Transmit` window of the PCAN-View GUI, confirm that both messages are being received periodically based on the settings of each message's cycle time.

    <img src=".//media/PCAN-View_Transmit.png" width=700/>

11. Confirm that the same periodic CAN message transmissions are displayed in the Microchip Bluetooth Data (MBD) smartphone app.

## Custom GATT Services

The [RNBD451](https://www.microchip.com/en-us/product/rnbd451pe) BLE module allows the user to create Bluetooth SIG-defined public GATT services as well as customer private services through simple UART commands. The specifications published by the Bluetooth SIG defines the public GATT services while the user defines their own private GATT services.

For an example of creating/programming a custom GATT service into the [RNBD451PE](https://www.microchip.com/en-us/product/rnbd451pe) BLE module, follow the procedure outlined in [Section 7.3](https://onlinedocs.microchip.com/oxy/GUID-26457D23-798C-47B0-9F40-C5DA6E995C6F-en-US-2/GUID-983D3FDA-5470-49F5-B8AC-E142DB5C1461.html
) (“Creating and Accessing GATT Services using UART Commands”) of the [RNBD451 Bluetooth Low Energy User's Guide](https://onlinedocs.microchip.com/oxy/GUID-26457D23-798C-47B0-9F40-C5DA6E995C6F-en-US-2/index.html)

