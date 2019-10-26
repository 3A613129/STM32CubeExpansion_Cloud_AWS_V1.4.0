@verbatim
******************************************************************************
*
* Portions Copyright (C) 2018 STMicroelectronics. All rights reserved.
* Portions Copyright (C) 2010-2015 Amazon.com, Inc. or its affiliates. All Rights Reserved.
*
* @file st_readme.txt
* @author MCD Application Team
* @brief This file lists the main modification done by STMicroelectronics on
*        "AWS IoT device SDK for embedded C" for integration with STM32Cube solution.
* For usage more details, please refer to UM2178 "AWS IoT software expansion
* for STM32Cube"
******************************************************************************
*
* Amazon.com license: Apache 2.0
* STMicroelectronics licence: ST license SLA0044
*
******************************************************************************
@endverbatim

### 22-May-2019 ###
=======================
   + Enable the platform name reporting to AWS.

### 11-April-2019 ###
========================
   + Remove redundant 'breaks' in aws_iot_jobs_topics.c to avoid compilation warnings.
   + Add the STM32Cube sample application in samples/, and the STM32Cube platform porting files in platform/.

### 25-July-2018 ###
========================
   + Fix format strings to avoid SW4STM32 compilation warnings

### 23-May-2018 ###
========================
   + Merge "AWS IoT device SDK for embedded C" Version [3.0.1], (May 11, 2018)

### 01-February-2018 ###
========================
   + Alignment with "AWS IoT device SDK for embedded C" Version [2.2.1], (Dec 26, 2017)
   + "SUCCESS" return code redefined to AWS_SUCCESS to avoid conflict with the HAL
   + Function prototype additions
   + st_readme.txt file addition

