/*!
 * @file        Board.h
 *
 * @brief       Header file for Board
 *
 * @version     V1.0.4
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
#ifndef BOARD_H
#define BOARD_H



// #define APM32F103_MINI

#if defined (APM32F103_MINI)
#include "board_apm32f103_mini.h"

#elif defined (APM32F107_MINI)
#include "board_apm32f107_mini.h"

#else
#error "Please select first the APM32  board to be used (in board.h)"
#endif

void GPIO_Init(void);
void USART1_Init(void);
void AFIO_USART1_Init(void);
void SPI1_Init(void);
void TIM2_Init(void);
void I2C_Init(void);
void USART1_RX_DMA_Init(void);
void CAN_Init(void);

/** @addtogroup Board
  @{
*/

/** @defgroup CAN_IDS CAN frame IDs
  @{
*/
#define CAN_ID_TX_FORCE   0x101u   /*!< 发送: FX FY */
#define CAN_ID_TX_MOMENT  0x102u   /*!< 发送: FZ TX */
#define CAN_ID_TX_FYFZ    0x103u   /*!< 发送: FY FZ */
#define CAN_ID_TX_CMD     0x201u   /*!< 发送: FY FZ */
#define CAN_ID_RX_CMD     0x201u   /*!< 接收       */
#define CAN_DATA_LENGTH   8
/**@}*/

/** @defgroup CAN_TxIndex CAN transmit frame index
  @{
*/
typedef enum
{
    CAN_TX_FORCE = 0,   /*!< 对应 CAN_ID_TX_FORCE  (0x101) */
    CAN_TX_MOMENT,      /*!< 对应 CAN_ID_TX_MOMENT (0x102) */
    CAN_TX_FYFZ,        /*!< 对应 CAN_ID_TX_FYFZ   (0x103) */
    CAN_TX_CMD,
    CAN_TX_NUM
} CAN_TxIndex_T;
/**@}*/

/* CAN transmit / receive frames (defined in Board.c) */
extern CAN_TxMessage_T txMsg[CAN_TX_NUM];
extern CAN_RxMessage_T rxMsg;

/**@} end of group Board */

#endif
