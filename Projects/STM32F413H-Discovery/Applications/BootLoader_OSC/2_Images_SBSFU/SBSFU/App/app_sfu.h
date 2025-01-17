/**
  ******************************************************************************
  * @file    app_sfu.h
  * @author  MCD Application Team
  * @brief   This file contains the configration of SBSFU application.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2017 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under Ultimate Liberty license
  * SLA0044, the "License"; You may not use this file except in compliance with
  * the License. You may obtain a copy of the License at:
  *                             www.st.com/SLA0044
  *
  ******************************************************************************
  */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef APP_SFU_H
#define APP_SFU_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/

/** @addtogroup SFU Secure Boot / Secure Firmware Update
  * @{
  */

/** @addtogroup SFU_CORE SBSFU Application
  * @{
  */

/** @addtogroup SFU_APP SFU Application Configuration
  * @{
  */

/** @defgroup SFU_CONFIG_IMGPROG Firmware Image Programming Configuration
  * @{
  */

/**
 * Use this define to choose the type of Firmware Image Programming you want to use.
 * This version supports only 2 modes:
 *
 * SFU_ENCRYPTED_IMAGE: Encrypted Firmware Image
 *       The image is received in encrypted format.
 *       The image must be decrypted to be installed:
 *         this is done according to the selected crypto scheme,
 *         see @ref SECBOOT_CRYPTO_SCHEME in the SE_CoreBin part.
 *
 * SFU_CLEAR_IMAGE: Clear Firmware Image
 *       The image is received in clear format.
 *       No decrypt operation is needed before installing the image:
 *         the selected crypto scheme must be compatible with this choice,
 *         see @ref SECBOOT_CRYPTO_SCHEME in the SE_CoreBin part.
 *
 *
 */
#define SFU_IMAGE_PROGRAMMING_TYPE SFU_ENCRYPTED_IMAGE

#define SFU_ENCRYPTED_IMAGE (0U) /*!< The Firmware Image to be installed is downloaded in ENCRYPTED format */
#define SFU_CLEAR_IMAGE     (1U) /*!< The Firmware Image to be installed is downloaded in CLEAR format */

/**
  * @}
  */

/** @defgroup SFU_CONFIG_SWDBG Software Debug Configuration
  * @{
  */


#define SFU_DEBUG_MODE               /*!< Comment this define to optimize memory footprint (debug mode removed)
                                          No more print on terminal during SBSFU execution */

/*#define SFU_VERBOSE_DEBUG_MODE*/   /*!< Uncomment this define when in verbose Debug mode.
                                        this switch activates more debug prints in the console (FSM state info...) */


/*#define SFU_FWIMG_BLOCK_ON_ABNORMAL_ERRORS_MODE*/  /*!< You may uncomment this define when running development tests.
                                                         When this switch is activated, the FWIMG part of SB_SFU will block when an abnormal error is encountered */


#if defined(SFU_VERBOSE_DEBUG_MODE) && !defined(SFU_DEBUG_MODE)
#error You cannot activate SFU_VERBOSE_DEBUG_MODE without activating SFU_DEBUG_MODE too.
#endif /* SFU_VERBOSE_DEBUG_MODE && !SFU_DEBUG_MODE */

#if defined(SFU_FWIMG_BLOCK_ON_ABNORMAL_ERRORS_MODE) && !defined(SFU_DEBUG_MODE)
#error SFU_FWIMG_BLOCK_ON_ABNORMAL_ERRORS_MODE is meant to be used in DEBUG mode
#endif /* SFU_FWIMG_BLOCK_ON_ABNORMAL_ERRORS_MODE && !SFU_DEBUG_MODE */

//#define SFU_TEST_PROTECTION       /*!< Auto-test of protections : WRP, PCROP, MPU, FWALL. Automatically executed @statup */

/**
 * SB_SFU status LED.
 * The constants below define the LED to be used and the LED blinking frequency to identify some situations.
 * This is useful when no log is enabled.
 *
 * \li The LED blinks every see @ref SFU_COM_YMODEM_DOWNLOAD_TIMEOUT seconds when a local download is waited.
 * \li For the other situations, please check the other defines below.
 */
#define SFU_STATUS_LED (LED_GREEN)            /*!< LED to be used to provide the SB_SFU status to the end-user */
#define SFU_STOP_NO_FW_BLINK_DELAY     (100U) /*!< Blinks every 100ms when no valid firmware is available and the local loader feature is disabled - see @ref SECBOOT_USE_LOCAL_LOADER */
#define SFU_INCORRECT_OB_BLINK_DELAY   (250U) /*!< Blinks every 250ms when an Option Bytes issue is detected - see @ref SECBOOT_OB_DEV_MODE */


/**
  * @}
  */

/** @defgroup SFU_CONFIG_OPT_FEATURES Optional Features Software Configuration
  * @{
  */
#define SECBOOT_USE_LOCAL_LOADER /*!< Set this define to enable the local loader feature ( YMODEM over UART) */
/**
  * @}
  */

/** @defgroup SFU_CONFIG_Protection Security Protection Configuration
  * @{
  */

/* The define below allows disabling all security IPs at once.
 *
 * Enabled: all security IPs (WRP, watchdog…) are disabled.
 * Disabled: the security IPs can be used (if their specific compiler switches are enabled too).
 *
 */
#define SECBOOT_DISABLE_SECURITY_IPS /*!< Disable all security IPs at once when activated */
#if !defined(SECBOOT_DISABLE_SECURITY_IPS)
/* Uncomment the following defines when in Release mode.
   In debug mode it can be better to disable some of the following protection
   for a better Debug experience (WRP, RDP, IWDG, DAP, etc.) */

#define SFU_WRP_PROTECT_ENABLE
#define SFU_RDP_PROTECT_ENABLE
#define SFU_DAP_PROTECT_ENABLE      /*!< WARNING: Be Careful if enabling this protection. Debugger will be disconnected. It might be difficult to reconnect the Debugger.*/
#define SFU_DMA_PROTECT_ENABLE
/*#define SFU_IWDG_PROTECT_ENABLE   */ /*!< WARNING:
                                        1. Be Careful if enabling this protection. IWDG will be active also after switching to UserApp: a refresh is needed.
                                        2. The IWDG reload in the SB_SFU code will have to be tuned depending on your platform (flash size...)*/
/* #define SFU_MPU_PROTECT_ENABLE */   /*!< MPU protection:
                                        Enables/Disables the MPU protection.
                                        If Secure Engine isolation is ensured by MPU (see SFU_ISOLATE_SE_WITH_MPU in SE_CoreBin\Inc\se_low_level.h),
                                        then this switch also enables/disables it, in addition to the overall MPU protection. */

/*#define SFU_FINAL_SECURE_LOCK_ENABLE */   /*!< WARNING: Should be enabled at the end of product development and test steps./
                                                 When enabling this lock, Static protections cannot be modified any more
                                                 and Debug is finally disabled. */

#if defined(SFU_FINAL_SECURE_LOCK_ENABLE)
#define SFU_PROTECT_RDP_LEVEL  (OB_RDP_LEVEL_2)  /*!< RDP level2 for product on the field. Final OB lock, Debug completely disabled, OB update no more possible  */
#else
#define SFU_PROTECT_RDP_LEVEL  (OB_RDP_LEVEL_1)  /*!< RDP level is set as 1 for debugging purposes. A product on the field should set it as Level2 */
#endif /* SFU_FINAL_SECURE_LOCK_ENABLE */

#endif /* !SECBOOT_DISABLE_SECURITY_IPS */

/**
 * The define below (SECBOOT_OB_DEV_MODE) determines if the OPTION BYTES should be handled in Development mode or not.
 * This define is taken into account only if RDP level 2 is not set.
 * If RDP level 2 is set no modification can be done anyway.
 *
 * Enabled: Option Bytes Development Mode enabled.
 *          SB_SFU uses a "check and apply" strategy when checking the Option Bytes configuration.
 *          If an OB is not set though it should be then this setting is automatically corrected.
 *          This applies only as long as RDP level 2 is not set.
 *
 * Disabled: Option Bytes Development Mode disabled.
 *           In this mode the Option Bytes are supposed to be already configured properly when the software starts for the first time.
 *           SB_SFU checks the Option Bytes configuration but does not correct it.
 *           If a problem is detected an error message is reported and the execution stops.
 */
#define SECBOOT_OB_DEV_MODE

/**
  * @}
  */

/** @defgroup SFU_CONFIG_IWDG IWDG Configuration
  * @{
  */
#define SFU_IWDG_TIMEOUT  ((uint32_t) 6) /*!<  IWDG timeout in seconds (the max. value that can be set here depends on the prescaler settings: IWDG_PRESCALER_XXX. ) */

/**
  * @}
  */

/** @defgroup SFU_CONFIG_Appli Application Configuration
  * @{
  */
#define SFU_FW_VERSION_START_NUM (1U)    /*!< brief SFU The very first version number a Firmware can have
                                              You can also define an upper bound here if you plan to use it */
/**
  * @}
  */

/** @defgroup SFU_CONFIG_Error Error Managment Configuration
  * @{
  */

/**
  * @brief Error Management defines
  */

#ifdef SFU_TEST_PROTECTION
#define SFU_MAX_CONSEC_COUNTER   256U  /*!< Max number of consecutive errors allowed for the same Fw Image, before invalidate it */
#else
#define SFU_MAX_CONSEC_COUNTER     3U  /*!< Max number of consecutive errors allowed for the same Fw Image, before invalidate it */
#endif /*SFU_TEST_PROTECTION */

#define SFU_MAX_TOTAL_COUNTER     10U  /*!< Max number of total errors allowed for the same Fw Image, before invalidate it */

/**
  * @}
  */

/**
  * @}
  */

/**
  * @}
  */

/**
  * @}
  */

#ifdef __cplusplus
}
#endif

#endif /* APP_SFU_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

