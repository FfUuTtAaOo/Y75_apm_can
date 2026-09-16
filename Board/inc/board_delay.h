/*!
 * @file        board_delay.h
 *
 * @brief       Header for board_delay.c module
 *
 * @version     V1.0.0
 *
 * @date        2022-05-25
 *
 * @attention
 *
 *  Copyright (C) 2021-2022 Geehy Semiconductor
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
#ifndef _BOARD_DELAY_H
#define _BOARD_DELAY_H

/* Includes ***************************************************************/
#include "main.h"

/* Exported macro *********************************************************/

/* Exported typedef *******************************************************/

/* Exported variables *****************************************************/
extern volatile uint32_t delayCount;

/* Exported function prototypes *******************************************/
void BOARD_DelayConfig(void);
uint32_t BOARD_ReadTick(void);
void BOARD_DelayMs(uint32_t count);
void BOARD_DelayUs(uint32_t count);
void BOARD_DelayIRQHandler(void);

#endif
