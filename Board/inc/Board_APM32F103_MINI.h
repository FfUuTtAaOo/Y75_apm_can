/*!
 * @file        board_apm32f103_mini.h
 *
 * @brief       This file contains definitions for APM32F103_MINI's Leds, push-buttons hardware resources
 *
 * @version     V1.0.3
 *
 * @date        2022-12-01
 *
 * @attention
 *
 *  Copyright (C) 2020-2022 Geehy Semiconductor
 *
 *  You may not use this file except in compliance with the
 *  GEEHY COPYRIGHT NOTICE (GEEHY SOFTWARE PACKAGE LICENSE).
 *
 *  The program is only for reference, which is distributed in the hope
 *  that it will be useful and instructional for customers to develop
 *  their software. Unless required by applicable law or agreed to in
 *  writing, the program is distributed on an "AS IS" BASIS, WITHOUT
 *  ANY WARRANTY OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the GEEHY SOFTWARE PACKAGE LICENSE for the governing permissions
 *  and limitations under the License.
 */

/* Define to prevent recursive inclusion */
#ifndef __BOARD_APM32F103_MINI_H
#define __BOARD_APM32F103_MINI_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes */
#include <string.h>
#include "apm32f10x.h"
#include "apm32f10x_gpio.h"
#include "apm32f10x_rcm.h"
#include "apm32f10x_eint.h"
#include "apm32f10x_usart.h"
#include "apm32f10x_misc.h"
#include "apm32f10x_spi.h"
#include "apm32f10x_tmr.h"
#include "apm32f10x_int.h"
#include "apm32f10x_i2c.h"
#include "apm32f10x_dma.h"
#include "apm32f10x_can.h"

/** @addtogroup Board
  @{
*/

/** @addtogroup Board_APM32F103_MINI
  @{
*/

/** @defgroup APM32F103_MINI_Enumerations
  * @{
  */

typedef enum
{
    TAG_LED = 0,
    SPI1_CS = 1,
    SPI1_CS2 = 2,
    ADC_SYNC = 3,
    ADC_SYNC2 = 4,
    RS485_DE = 5
} Led_TypeDef;

typedef enum
{
    ECAT_IRQ = 0,
    ECAT_SYNC0 = 1,
    ECAT_SYNC1 = 2
} Button_TypeDef;

typedef enum
{
    BUTTON_MODE_GPIO = 0,
    BUTTON_MODE_EINT = 1
} ButtonMode_TypeDef;

typedef enum
{
    COM1 = 0,
    COM2 = 1
} COM_TypeDef;

/**@} end of group APM32F103_MINI_Enumerations */

/**
  * @}
  */

/** @addtogroup APM32F103_MINI_LED
  * @{
  */
#define LEDn                             6

#define TAG_LED_Pin                     GPIO_PIN_15
#define TAG_LED_GPIO_Port               GPIOB
#define TAG_LED_GPIO_CLK                RCM_APB2_PERIPH_GPIOB

#define SPI1_CS_Pin                     GPIO_PIN_4
#define SPI1_CS_GPIO_Port               GPIOA
#define SPI1_CS_GPIO_CLK                RCM_APB2_PERIPH_GPIOA

#define SPI1_CS2_Pin                    GPIO_PIN_0
#define SPI1_CS2_GPIO_Port              GPIOB
#define SPI1_CS2_GPIO_CLK               RCM_APB2_PERIPH_GPIOB

#define ADC_SYNC_Pin                    GPIO_PIN_3
#define ADC_SYNC_GPIO_Port              GPIOA
#define ADC_SYNC_GPIO_CLK               RCM_APB2_PERIPH_GPIOA

#define ADC_SYNC2_Pin                   GPIO_PIN_1
#define ADC_SYNC2_GPIO_Port             GPIOB
#define ADC_SYNC2_GPIO_CLK              RCM_APB2_PERIPH_GPIOB

#define RS485_DE_Pin                    GPIO_PIN_5
#define RS485_DE_GPIO_Port              GPIOB
#define RS485_DE_GPIO_CLK               RCM_APB2_PERIPH_GPIOB

/**@} end of group APM32F103_MINI_LED */

/** @addtogroup APM32F103_MINI_BUTTON
  * @{
  */
#define BUTTONn                        3

/**
 * @brief IRQ
 */
#define ECAT_IRQ_PIN                   GPIO_PIN_0
#define ECAT_IRQ_GPIO_PORT             GPIOA
#define ECAT_IRQ_GPIO_CLK              RCM_APB2_PERIPH_GPIOA
#define ECAT_IRQ_EINT_LINE             EINT_LINE_0
#define ECAT_IRQ_EINT_PORT_SOURCE      GPIO_PORT_SOURCE_A
#define ECAT_IRQ_EINT_PIN_SOURCE       GPIO_PIN_SOURCE_0
#define ECAT_IRQ_EINT_IRQn             EINT0_IRQn
/**
 * @brief SYNC0
 */
#define ECAT_SYNC0_PIN                   GPIO_PIN_1
#define ECAT_SYNC0_GPIO_PORT             GPIOA
#define ECAT_SYNC0_GPIO_CLK              RCM_APB2_PERIPH_GPIOA
#define ECAT_SYNC0_EINT_LINE             EINT_LINE_1
#define ECAT_SYNC0_EINT_PORT_SOURCE      GPIO_PORT_SOURCE_A
#define ECAT_SYNC0_EINT_PIN_SOURCE       GPIO_PIN_SOURCE_1
#define ECAT_SYNC0_EINT_IRQn             EINT1_IRQn
/**
 * @brief SYNC1
 */
#define ECAT_SYNC1_PIN                   GPIO_PIN_2
#define ECAT_SYNC1_GPIO_PORT             GPIOA
#define ECAT_SYNC1_GPIO_CLK              RCM_APB2_PERIPH_GPIOA
#define ECAT_SYNC1_EINT_LINE             EINT_LINE_2
#define ECAT_SYNC1_EINT_PORT_SOURCE      GPIO_PORT_SOURCE_A
#define ECAT_SYNC1_EINT_PIN_SOURCE       GPIO_PIN_SOURCE_2
#define ECAT_SYNC1_EINT_IRQn             EINT2_IRQn

/**@} end of group APM32F103_MINI_BUTTON */

/** @addtogroup APM32F103_MINI_COM
  * @{
  */

#define COMn                            2

/**
 * @brief Definition for COM port1, connected to USART1
 */
#define COM1_PORT                       USART1
#define COM1_CLK                        RCM_APB2_PERIPH_USART1
#define COM1_TX_PIN                     GPIO_PIN_9
#define COM1_TX_GPIO_PORT               GPIOA
#define COM1_TX_GPIO_CLK                RCM_APB2_PERIPH_GPIOA
#define COM1_RX_PIN                     GPIO_PIN_10
#define COM1_RX_GPIO_PORT               GPIOA
#define COM1_RX_GPIO_CLK                RCM_APB2_PERIPH_GPIOA
#define COM1_IRQn                       USART1_IRQn

/**
 * @brief Definition for COM port2, connected to USART2
 */
#define COM2_PORT                       USART2
#define COM2_CLK                        RCM_APB1_PERIPH_USART2
#define COM2_TX_PIN                     GPIO_PIN_2
#define COM2_TX_GPIO_PORT               GPIOA
#define COM2_TX_GPIO_CLK                RCM_APB2_PERIPH_GPIOA
#define COM2_RX_PIN                     GPIO_PIN_3
#define COM2_RX_GPIO_PORT               GPIOA
#define COM2_RX_GPIO_CLK                RCM_APB2_PERIPH_GPIOA
#define COM2_IRQn                       USART2_IRQn

/**@} end of group APM32F103_MINI_COM */

/** @defgroup APM32F103_MINI_Functions
  * @{
  */

void BOARD_LEDInit(Led_TypeDef Led);
void BOARD_GPIOOn(Led_TypeDef Led);
void BOARD_GPIOOff(Led_TypeDef Led);
void BOARD_LEDToggle(Led_TypeDef Led);

void BOARD_EINTInit(Button_TypeDef Button, ButtonMode_TypeDef Button_Mode);
uint32_t BOARD_PBGetState(Button_TypeDef Button);

void BOARD_COMInit(COM_TypeDef COM, USART_Config_T* configStruct);

/**@} end of group APM32F103_MINI_Functions */
/**@} end of group Board_APM32F103_MINI */
/**@} end of group Board */

#ifdef __cplusplus
d:\ST\workspace\vscode\huaweike}
#endif

#endif /* __BOARD_APM32F103_MINI_H */
