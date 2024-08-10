# Microchip ATSAME51 BLE Peripheral Example : CAN Bus Sniffer

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

## Hardware Requirements

* Base Adapter Board: ["Curiosity Nano Base for Click Boards"](https://www.microchip.com/en-us/development-tool/ac164162)

    <img src=".//media/curiosity-nano-adapter.jpg" width=375/>

* Host MCU Add-On Board: ["SAM E51 Curiosity Nano"](https://www.microchip.com/en-us/development-tool/ev76s68a)

    <img src=".//media/sam-e51-curiosity-nano.jpg" width=375/>

* BLE Module: ["RNBD451 Add-On Board"](https://www.microchip.com/en-us/development-tool/ev25f14a) board featuring Microchip's [RNBD451PE](https://www.microchip.com/en-us/product/RNBD451PE) certified BLE module

    <img src=".//media/rnbd451-add-on-board.jpg" width=375/>

* High Speed CAN FD Transceiver: ["ATA6563 Click board"](https://www.mikroe.com/ata6563-click)

    <img src=".//media/ata6563-click.jpg" width=375/>

* USB-to-CAN Adapter for Host PC: ["PCAN-USB FD Adapter"](https://phytools.com/collections/peak-system-technik/products/pcan-usb-fd-adapter)

    <img src=".//media/PCAN-USB-FD_Adapter.jpg" width=375/>

## Demo Platform Assembly

1. Set the jumper on the RNBD451 Add-On Board so that the two pins furthest away from the module are shorted.

    <img src=".//media/HW_Setup_01.png" width=400/>

2. Install the RNBD451 Add-On Board onto mikroBUS socket #2 of the Curiosity Nano Base.

    <img src=".//media/HW_Setup_02.png" width=400/>

3. Connect the supplied 28-pin headers underneath the SAM E51 Curiosity Nano (soldering is optional) and then install the SAM E51 Curiosity Nano onto the 56-pin socket of the Curiosity Nano Base.

    <img src=".//media/HW_Setup_03.png" width=400/>

4. Connect the PC to the SAM E51 Curiosity Nano using a micro-USB cable and note the new Virtual COM port number that is associated with your connection (e.g. use the Windows Device Manager).

    <img src=".//media/HW_Setup_04.png" width=500/>

5. Install the ATA6563 Click board onto mikroBUS socket #1 of the Curiosity Nano Base.

    <img src=".//media/HW_Setup_05.png" width=500/>

6. Confirm that all boards and connections have been made for the complete hardware platform.

    <img src=".//media/HW_Setup_06.png" width=600/>

## Software Requirements

Embedded software development tools need to be installed in order to properly program the WBZ451 Curiosity Development Board and then provision/test it for use with Microsoft Azure IoT services.

1. Microchip `MPLAB X` tool chain for embedded code development on 32-bit architecture MCU/MPU platforms (made up of 3 major components)

    - [MPLAB X IDE](https://www.microchip.com/mplab/mplab-x-ide) (when prompted, enable the installation of the [MPLAB IPE](https://www.microchip.com/en-us/tools-resources/production/mplab-integrated-programming-environment) too)
        
        NOTE: This demonstration project was last tested successfully with MPLAB X **v6.05**. If a previous version of the IDE is needed, download it from the [MPLAB Development Ecosystem Downloads Archive](https://www.microchip.com/en-us/tools-resources/archives/mplab-ecosystem) (to fall back to the version Microchip successfully tested prior to release). 

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

1. Clone/download the MPLAB X demo project by issuing the following command line in a `Command Prompt` or `PowerShell` window

    ```bash
    git clone https://github.com/MicrochipTech/AzureDemo_WBZ451
    ```

2. Connect the board to the PC using a micro USB cable. If using a board that has not been programmed previously, a device named `CURIOSITY` may show up as a Mass Storage Device (MSD) on the `Desktop` or in a `File Explorer` window. If this is the case, "drag and drop" (i.e. copy) the pre-built `*.hex` file (e.g. "WBZ451_Trust_WiFi7.X.production.hex") directly to the `CURIOSITY` drive. The hex file should be located in the folder at `AzureDemo_WBX451` > `WBZ451_Curiosity` >`firmware` > `WBZ451_Trust_WiFi7.X` > `dist` > `default` > `production`

    NOTE: If this file copy operation fails for any reason or is not possible, [Make and Program Device](./AzurePnP_MPLABX.md) by building the original MPLAB X source code project that was used to generate the `*.hex` file.

3. Set up a Command Line Interface (CLI) to the board

    - Open a serial terminal (e.g. PuTTY, TeraTerm, etc.) and connect to the COM port corresponding to your board at `115200 baud` (e.g. open PuTTY Configuration window &gt; choose `session` &gt; choose `Serial`&gt; Enter the right COMx port). You can find the COM info by opening your PC’s `Device Manager` &gt; expand `Ports(COM & LPT)` &gt; take note of `Curiosity Virtual COM Port` 

        <img src=".//media/image27.png" width=400/>

4. Before typing anything in the terminal emulator window, **disable** the local echo feature in the terminal settings for best results.  In the terminal window, hit `[RETURN]` to bring up the Command Line Interface prompt (which is simply the `>` character). Type `help` and then hit `[RETURN]` to get the list of available commands for the CLI implemented in the board's firmware.  The Command Line Interface allows you to send simple ASCII-string commands to set or get the user-configurable operating parameters of the application while it is running.

    <img src=".//media/image44.png" width=450/>

5. Press the `RESET` button on the WBZ451 Curiosity board and then immediately turn your attention to the terminal window.

    <img src=".//media/WBZ451_RESET.png" width=375/>

6. Look for the signer and device certificate information which is output in the terminal emulator window. This information is always the first set of messages output to the terminal window whenever the WBZ451 board is reset. Pause the terminal window's serial output so that you can copy the signer certificate information from the window and then paste the contents into a new file called "signer.pem". This file contains the information of the certificate for the signer of the client certificate that were both pre-provisioned into the ATECC608B Trust&GO secure element on the ATECC608 TRUST accessory board. This certificate will need to be uploaded into your IoT Central application in a future step when creating an enrollment group. The signer *.pem file needs to begin with the ASCII text "-----BEGIN CERTIFICATE-----" and end with "-----END CERTIFICATE-----".

    <img src=".//media/signer_cert_output.png" width=600/>

7. In the terminal emulator window, set the debug messaging level to 0 to temporarily disable the output messages. The debug level can be set anywhere from 0 to 4.  Use the `debug <level>` command by manually typing it into the CLI.  The complete command must be followed by hitting `[RETURN]`
    ```bash
    >debug 0
    ```

8. Perform a Wi-Fi scan to see the list of Access Points that are currently in range and being detected by the WiFi7 click board's Wi-Fi network controller.  Use  the `wifi` command's `scan` option by manually typing it into the CLI.  The complete command must be followed by hitting `[RETURN]`
    ```bash
    >wifi -scan
    ```

9. Configure the board's internal Wi-Fi settings with your wireless router’s SSID and password using the `wifi` command's `set` option by manually typing it into the CLI.  The complete command must be followed by hitting `[RETURN]` (there cannot be any spaces used in the SSID or password)
    ```bash
    >wifi -set <NETWORK_SSID>,<PASSWORD>,<SECURITY_OPTION[1=Open|2=WPA|3=WEP]>
    ```
    For example, if the SSID of the router is "MyWirelessRouter" and the WPA/WPA2 key is "MyRoutersPassword", the exact command to type into the CLI (followed by `[RETURN]`) would be
    ```bash
    >wifi -set MyWirelessRouter,MyRoutersPassword,2
    ```

10. At the CLI prompt, type in the command `reset` and hit `[RETURN]` to restart the host application.  The Blue LED should eventually stay solidly ON to signify that the board has successfully connected to the wireless router.
    ```bash
    >reset
    ```
11. In the terminal emulator window, set the debug messaging level to 0 to temporarily disable the output messages. The debug level can be set anywhere from 0 to 4.  Use the `debug <level>` command by manually typing it into the CLI.  The complete command must be followed by hitting `[RETURN]`
    ```bash
    >debug 0
    ```

12. At this point, the board is connected to Wi-Fi, but has not yet established a connection with the cloud. The `cloud` command can be used at any time to confirm the cloud connection status (which as of right now should be "false").  The complete command must be followed by hitting `[RETURN]`
    ```bash
    >cloud -status
    ```

## Testing Procedure

1. Launch a terminal emulator window and connect to the COM port corresponding to the WBZ451 Curiosity board at `115200` baud (**disable** local echo for the terminal settings for best results).  If there are continuous non-stop messages being displayed on the terminal, disable them by typing `debug 0` followed by `[RETURN]`. Hit `[RETURN]` a couple of times to bring up the Command Line Interface prompt (which is simply the `>` character). Type `help` and then hit `[RETURN]` to get the list of available commands for the CLI.  The Command Line Interface allows you to send simple ASCII-string commands to set or get the user-configurable operating parameters of the application while it is running.

    <img src=".//media/image44.png" width=450/>

2.	Look up the `ID Scope` for your IoT Central application (navigate to your application's web page and using the left-hand navigation pane, select `Permissions` > `Device connection groups`).  The `ID Scope` will be programmed/saved into the [ATECC608B](https://www.microchip.com/wwwproducts/en/atecc608b) secure element on the ATECC608B TRUST board in the next step

    <img src=".//media/image84a.png" width=450/>

3. In the terminal emulator window, hit `[RETURN]` to bring up the Command Line Interface prompt (which is simply the `>` character>). At the CLI prompt, type in the `idscope <your_ID_scope>` command to set it (which gets saved in the [ATECC608B](https://www.microchip.com/wwwproducts/en/atecc608b) secure element on the board) and then hit `[RETURN]`.  The ID Scope can be read out from the board by issuing the `idscope` command without specifying any parameter on the command line - confirm that the ID Scope has been read back correctly before proceeding to the next step

    <img src=".//media/image85.png" width=450/>

    NOTE: Make sure the ID scope reads back correctly. If not, keep repeating the write/read sequence until the correct ID scope has been read back from the board

4. In the terminal emulator window, hit `[RETURN]` to bring up the CLI prompt. Type in the command `reset` and hit `[RETURN]`

5. The board should go through its connection process and may take a minute or two to successfully connect to the cloud. The `cloud` command can be used at any time to confirm the cloud connection status using the CLI.  The complete command must be followed by hitting `[RETURN]`
    ```bash
    >cloud -status
    ```

6. Periodically execute the `cloud` command until the status returned is displayed/returned as "true".

7. Go back to your web browser to access the Azure IoT Central application.  Use the left-hand side pane and select `Devices` > `All Devices`.  Confirm that your device is listed – that the "Device name" & "Device ID" shown is the Common Name in the client certificate (which should be `sn + {17-digit device ID}`).  Click on the device name to see the additional details available for viewing

    <img src=".//media/image86.png" width=600/>

8. [OPTIONAL] If desired, change the Device name by clicking on `Manage device` > `Rename`

    <img src=".//media/image87.png" width=350/>

9. Click on the `Commands` tab; type `PT5S` in the `Delay before reboot operation` field and then click on `Run` to send the command to the device to reboot in 5 seconds

    <img src=".//media/image88.png" width=400/>

10. Within 5 seconds of sending the Reboot command, the development board should reset itself. Wait a minute or two and periodically issue the `cloud` command until the status is displayed/returned as "true". Locate the button labeled `USR-BTN` on the board and press it a few times.

    <img src=".//media/WBZ451_USR-BTN.png" width=375/>

11. Click on the `Raw data` tab and confirm that the button press telemetry messages were received (scroll the page to the right to view the `Button Press` column)

    <img src=".//media/image90.png" width=300/>

12. Click on the `Refresh` icon to display all messages received since the previous page refresh operation.  Confirm that periodic telemetry messages are being continuously received every few seconds.

    <img src=".//media/image91.png" width=75/>

    <img src=".//media/image92.png" width=400/>

13. Click [here](./DeviceTemplate_CreatingViews.md) to create an additional "Properties" view that allows you to change any of the Cloud-writable properties. Once this new view has been added to the device template, click on the Properties view and type in a new value for the Telemetry Interval. Click on the **Save** icon to send the property update request to the physical device. You should see the status of the property listed as "Pending" until a confirmation has been received from the physical device that the property was successfully updated. At this point in time, the status of the property should revert back to the "Accepted" state.

    <img src=".//media/Properties_tab.png" width=400/>

14. Click on the `Properties` tab and try changing the 3 PWM duty cycle properties of the RGB LED to 100% (remember to hit the `Save` icon for each property write operation to take effect). Confirm that the RGB LED lights up based on different combinations of 0% and/or 100% duty cycle values. In addition, try changing the state of the User LED to On, Off, and Blinking.

    NOTE: Depending how quickly the write property response is received, it is possible that IoT Central will show the value as "Pending". If the device is offline or doesn't respond to a writable property request, the value can display as "Pending" indefinitely in IoT Central until a valid property update acknowledge has been received from the device.

15. Click on the `About` tab to conveniently view all of the device's property settings/states on a single page.
