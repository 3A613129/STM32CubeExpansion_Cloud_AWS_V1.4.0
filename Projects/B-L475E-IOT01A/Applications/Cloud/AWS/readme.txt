/**
  @page AWS AWS Cloud application

  @verbatim
  ******************************************************************************
  * @file    readme.txt
  * @author  MCD Application Team
  * @brief   Description of AWS Cloud application.
  ******************************************************************************
  *
  * Copyright (c) 2017 STMicroelectronics International N.V. All rights reserved.
  *
  * This software component is licensed by ST under Ultimate Liberty license
  * SLA0044, the "License"; You may not use this file except in compliance with
  * the License. You may obtain a copy of the License at:
  *                             www.st.com/SLA0044
  *
  ******************************************************************************
  @endverbatim

@par Application Description


This application implements an Amazon AWS Cloud IoT client for the B-L475E-IOT01 board
using the on-board Inventek ISM43362 Wifi module connectivity,
and mbedTLS for secure network communication.

The application connects to Amazon AWS IoT Cloud with the credentials provided
by the user. When the User button is pushed, it sends a LED toggle command to the IoT Cloud
endpoint, which returns back the message to the board and trigs the LED toggle.

The board sensor data are also collected and published to the cloud about every 10 seconds.

A Firmware Other The Air (FOTA) update via HTTP(S) can be trigged by a specific AWS IoT job.

Upon a double-push of the User button, the application exits.

FOTA update notes:
- The application is launched by a BootLoader which also implements on-the-field firmware upgrade.
  For flashing the board locally in a single step, the IDE builds a complete image composed of
  the BootLoader executable and of the user application.
  This executable can be flashed and used for debugging the application (debug information are not present
  for the BootLoader part).
  The binary merge of the bootloader with the user application is done by post-build actions, after link stage.

  The BootLoader itself is generated from the X-CUBE-SBSFU STM32Cube expansion package,
  and provided as a pre-build executable.
  In order to enable application debug, the BootLoader is configured not to enforce the security features.
  Some specific directories of the X-CUBE-SBSFU package are included in the present package
  so that the BootLoader may be rebuilt with a different configuration;
  please refer to Projects/<board>/Applications/Cloud/AWS/bootloader_readme.txt.

- SW4STM32 tools:
  When debugging with SW4STM32, the debugger starts the execution from the application main(), and
  not from the BootLoader main().
  A workaround is to perform a hard-reset on the board to force it to reboot correctly.


@par Directory contents

---> in .
Binary
  <boardname>_AWS.sfb                Pre-built secure image of the sample application. To be used in FOTA case.
  SBSFU_<boardname>_AWS.bin          Pre-built bootloader + sample application image.

Inc
  es_wifi_conf.h                     es-WiFi configuration.
  flash.h                            Management of the internal flash memory.
  main.h                             Configuration parameters for the application and globals.
  mbedtls_config.h                   Static configuration of the mbedTLS library.
  mbedtls_entropy.h                  mbedTLS RNG abstraction interface.
  net_conf.h                         Configuration parameters for the STM32_Connect_Library.
  sensor_data.h                      Board-wide sensor initialization interface.
  stm32l475e_iot01_conf.h            BSP sensor configuration.
  stm32l4xx_hal_conf.h               HAL configuration file.
  stm32l4xx_it.h                     STM32 interrupt handlers header file.
  vl53l0x_platform.h                 vl53l0x proximity sensor platform include file.
  vl53l0x_proximity.h                vl53l0x proximity sensor include file.

Src
  flash_l4.c                         Flash programming interface.
  main.c                             Main application file.
  mbedtls_entropy.c                  mbedTLS RNG abstraction interface.
  net_conf.c                         Bus mapping of the STM32_Connect_Library to the eS_WiFi module.
  set_credentials.c                  Interactive provisionning of the network connection settings.
  stm32l4xx_hal_msp.c                Specific initializations.
  stm32l4xx_it.c                     STM32 interrupt handlers.
  system_stm32l4xx.c                 System initialization.
  vl53l0x_platform.c                 vl53l0x proximity sensor platform file.
  vl53l0x_proximity.c                vl53l0x proximity sensor.

---> in Projects/Misc_Utils
Inc
  cloud.h
  iot_flash_config.h
  msg.h                              Log interface
  rfu.h

Src
  cloud.c                            Cloud application init and deinit functions
  iot_flash_config.c                 Dialog and storage management utils for the user-configured settings.
  rfu.c                              Firmware update helpers.

---> in Middlewares/Third_Party/AWS
certs
  Amazon1_Comodo_Baltimore.crt       List of root CA certificates to be pasted on the board console at first launch.
                                       * AWS IoT device created _SINCE_ September 2018 use Amazon1;
                                       * the RTC init over HTTPS uses Comodo;
                                       * the remote firmware update file may be downloaded from Amazon S3,
                                         and then use Digicert Baltimore.

  Verisign_Comodo_Baltimore.crt      List of root CA certificates to be pasted on the board console at first launch.
                                       * AWS IoT device created _BEFORE_ September 2018 use Verisign;
                                       * the RTC init over HTTPS uses Comodo;
                                       * the remote firmware update file may be downloaded from Amazon S3,
                                         and then use Digicert Baltimore.

platform/STM32Cube
  aws_network_st_wrapper.c           AWS SDK network porting layer.
  aws_timer.c                        AWS SDK time porting layer.
  network_platform.h
  timer_platform.h

samples/STM32Cube
  aws_subscribe_publish_sensor_values.c  Sample application.
  aws_iot_config.h                   AWS IoT C SDK configuration.
  aws_version.h                      X-CUBE-AWS package version information.

---> in Utilities
CLD_utils/http_lib
  http_lib.c                         Light HTTP client
  http_lib.h

Time
  STM32CubeRTCInterface.c            Libc time porting to the RTC.
  timedate.c                         Initialization of the RTC from the network.
  timingSystem.c                     Libc time porting to the RTC.
  timedate.h
  timingSystem.h

---> in ../../BootLoader_OSC
2_Images_SBSFU/*                      Bootloader binary, and security settings (in SBSFU/App/app_sfu.h)
2_Images_SECoreBin/*                  Post-build scripts for the user application.
Linker_Common/*                       Image memory mapping definitions for the user application.


@par Hardware and Software environment
  - B-L475E-IOT01 board.
    MCU board: B-L475E-IOT01 (MB1297 rev D), with FW "Inventek eS-WiFi ISM43362-M3G-L44-SPI C3.5.2.3.BETA9"
    Note: the FW version is displayed on the board console at boot time.

  - WiFi access point.
      * With a transparent Internet connectivity: No proxy, no firewall blocking the outgoing traffic.
      * Running a DHCP server delivering the IP and DNS configuration to the board.

  - Amazon AWS account. Security keys and endpoint for AWS IoT service must be created.
    See http://docs.aws.amazon.com/iot/latest/developerguide/iot-gs.html

@par How to use it ?

For the application to work, you must do the following:

 - WARNING: Before opening the project with any toolchain be sure your folder
   installation path is not too in-depth since the toolchain may report errors
   after building.

 - Open the IAR IDE and compile the project (see the release note for detailed
   information about the version). Alternatively you can use the Keil uVision
   toolchain (see the release note for detailed information about the version).
   Alternatively you can use the System Workbench for STM32 (see the release note
   for detailed information about the version).

 - Program the firmware on the STM32 board: If you generated a raw binary file,
   you can copy (or drag and drop) it from
   Projects\B-L475E-IOT01\Applications\Cloud\AWS\<toolchain>\PostBuild\SBSFU_<boardname>_AWS.bin
   to the USB mass storage location created when you plug the STM32 board to your PC.
   If the host is a Linux PC, the STM32 device can be found in the /media folder
   with the name e.g. "DIS_L4IOT". For example, if the created mass
   storage location is "/media/DIS_L4IOT", then the command to program the board
   with a binary file named "my_firmware.bin" is simply: cp my_firmware.bin
   /media/DIS_L4IOT.

   Alternatively, you can program the STM32 board directly through one of the
   supported development toolchains, the STM32 ST-Link Utility, or STM32CubeProgrammer.

   Note: If the board was programmed with an earlier version of the application,
   the Flash memory must be fully erased before re-programming, bBecause of possible change
   of the format of the user data stored in Flash.

 - Configure the required settings (to be done only once):
   - When the board is connected to a PC with USB (ST-LINK USB port),
     open a serial terminal emulator, find the board's COM port and configure it with:
      - 8N1, 115200 bauds, no HW flow control
      - set the line endings to LF or CR-LF (Transmit) and LF (receive).

   - At first boot, enter the requested parameters:

     - Wifi network configuration (SSID, security mode, password).

     - Enter the TLS network security keys used by the AWS IoT connection: root CA certificate(s),
       device certificate, and private key.

       In order to use HTTPS for initializing the RTC, the root CA certificate of the target
       server must be provided together (in the same copy-paste) with the Amazon root CA certificate.
       You can use the provided file: Amazon1_Comodo_Baltimore.crt.

       If you use an old AWS IoT device created before September 2018, the root CA may be
       Symantec/Verisign instead of Amazon1. If so, Verisign_Comodo_Baltimore.crt must be used instead.

       HTTPS remote firmware update from AWS S3 is enabled by the third certificate contained in *_Comodo_Baltimore.crt.
       If you use your own HTTPS server instead of AWS S3, that third certificate must be replaced
       by the root CA of your web server.
       Note: File download over clear HTTP is also supported. Then no third certificate is needed.

     - Enter the Amazon AWS IoT end-point (Amazon server address) and device ID.

   - After the parameters are configured, it is possible to change them by restarting the board
     and pushing User button (blue button) when prompted during the initialization.

@par Connectivity test
  - Makes an HTTPS request to retrieve the current time and date, and configures the RTC.

    Note: HTTPS has the advantage over NTP that the server can be authenticated by the board, preventing
          a possible man-in-the-middle attack.
          However, the first time the board is switched on (and each time it is powered down and up, if the RTC
          is not backed up), the verification of the server certificate will fail as its validity period will
          not match the RTC value.
          The following log will be printed to the console: "x509_verify_cert() returned -9984 (-0x2700)"

          If the CLOUD_TIMEDATE_TLS_VERIFICATION_IGNORE switch is defined in cloud.c (it is the case by default),
          this error is ignored and the RTC is updated from the "Date:" field of the HTTP response header.
          Else, a few more error log lines are printed, and the application retries to connect without verifying
          the server certificate.
          If/once ok, the RTC is updated.

  - By default the AWS sample application connects to Amazon AWS IoT cloud, using the parameters as described above.

  - Push the User button (blue button) to toggle and publish the LED desired value.
    A message is sent back by the AWS IoT cloud, which lets the LED toggle.

  - The physical user interaction can be simulated by creating a custom AWS IoT job:
    Post your job file to an AWS S3 bucket, click on "Create" / "Create custom job" on the Manage/Jobs
     section of the AWS IoT console, and fill-in the forms.

    The contents of the job file can be either:
      { "myOperation": "simulate_single_push" }
    or:
      { "myOperation": "simulate_multiple_push" }

    A possible usage is to enroll several devices in a device group, and to deploy a single_push job
    to the whole group. It will toggle the LED of the devices almost synchronously.

  - The sensor data are published about every 10 seconds.

@par Firmware upgrade

  The BootLoader allows to update the STM32 microcontroller built-in program with new firmware versions,
  adding new features and correcting potential issues. The update process can be performed in a secure way
  to prevent unauthorized updates and access to confidential on-device data. In addition, Secure Boot
  (Root of Trust services) checks and activates STM32 security mechanisms, and checks the authenticity and
  integrity of user application code before every execution to ensure that invalid or malicious code
  cannot be run. The Secure Firmware Update application receives the encrypted firmware image, checks its
  authenticity, decrypts it, and checks the integrity of the code before installing it.

  When building the AWS application a file containing the encrypted firmware image is generated:
  Projects/<board>/Applications/Cloud/AWS/<toolchain>/PostBuild/<boardname>_AWS.sfb

  To test the firmware upgrade feature, place this file on an HTTP(S) server. Enter the URL of this file when
  prompted by the AWS application running on STM32 device (or use an AWS IoT job as described above).
  The encrypted firmware image is downloaded to device, the device reboots and checks
  its integrity before running the new firmware.

  It is possible to do a firmware update with an AWS IoT job. The syntax for the AWS IoT job document is:
    {
      "firmwareUpdate" : "http[s]://path.to/Firmware.sfb"
    }

  Here is an example using AWS S3 service for firmware storage:

  - you must have a device (board) flashed with an AWS firmware compiled with SBSFU build configuration
    and a .sfb SBSFU firmware file which version differs from the one of the device.

  - store the .sfb firmware file on AWS S3 service (in a S3 bucket)
    example: Project.sfb in bucket "bucket"
    make sure the file is readable by everyone (file properties / permissions) and is made public

  - create a text file "firmwareupdate.txt" with the following content and store it on S3:

    {
      "firmwareUpdate" : "https://s3.eu-central-1.amazonaws.com/bucket/Project.sfb"
    }

  - On AWS web portal, select the IoT Core service, go to Manage / Jobs
    * Click the "Create" button, then "create custom job"
    * Enter a job ID (number)
    * Select the device you wish to update
    * Select the job file you just uplodaded to S3: firmwareupdate.txt
    * Complete the job creation: Next / Create

  - Once the job is created and the device is connected to AWS IoT through MQTT, the device receives the job,
    the application exits the MQTT loop and starts downloading the new firmware file over HTTP(S).
    If successful, it reboots to let SBSFU update the firmware, and launch the new application.
    Once the new application runs and is connected to AWS IoT again, it verifies that its version
    has changed since the update job was received, and notifies AWS IoT of the job status: Completed, or Failed.

@par AWS-based web dashboard
A companion AWS-based web dashboard is available to quickly evaluate the firmware package functions for an easy sensor
data visualization and device control.

On the web application side:
  - Open it at: http://st-dashboard-iot-v2.s3-website-us-east-1.amazonaws.com
  - Sign-up the first time only
  - Sign-in
  - Create a device and save the credentials file. This information will be needed for your device to connect to the web dashboard.

On the device side:
 - Open your IDE preprocessor defined symbols tab and add AWS_IOT_DASHBOARD.
 - Build
 - Start the board as indicated in the previous sections
 - With the board console, enter the AWS credentials as follows:
   * For the server address, enter a1dyrwwtjpwo0n-ats.iot.us-east-1.amazonaws.com
   * Enter the device name with the MAC address that you have entered when you created the device with the web application.
   * Enter the root CA. See above section "Directory contents"
   * For the device certificate, enter the 1st part of the credentials file received when you created the device with the web application.
   * For the device key, enter the 2nd part of the credentials file received when you created the device with the web application.

 - If everything went well this message will be displayed to the board console: "Connected to a1dyrwwtjpwo0n-ats.iot.us-east-1.amazonaws.com:8883"

You can now use the dashboard.

More information can be found on line on the web application side.


 * <h3><center>&copy; COPYRIGHT STMicroelectronics</center></h3>
 */
