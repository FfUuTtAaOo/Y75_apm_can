/*!
 * @file        main.h
 *
 * @brief       Main program body
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
#ifndef __MAIN_H
#define __MAIN_H

/* Includes */
#include <stdio.h>
#include <assert.h>
#include "Board.h"
#include "apm32f10x.h"
#include "filter.h"

/** @addtogroup MACRO DEFINITION
  @{
*/
#define ADS131M08_GAIN 128


/* Communication instructions */
#define CONTINUE_DATA_SEND  0x02
#define ZERO_CLEARING       0x03

#define ADC_CHANNEL_NUM         6


/** @addtogroup STRUCTURAL
  @{
*/
typedef union {
    float f32;
    uint32_t u32;
} floatuint_t;

#define U32_TO_U8(u32, u8, offset)  (u8) = (((u32) >> (offset)) & 0xFF)
#define UINT8_EQUAL(a, b) ((a) == (b))
#define U8_TO_U32_SAFE(u8_val, u32_val, index) \
  ({ \
    assert((index) < 4); \
    ((u32_val & ~(0xFFUL << ((index) * 8))) | (((uint32_t)(u8_val)) << ((index) * 8))); \
  })

extern volatile uint32_t led_flicker;

void Delay(uint32_t count);
void delay_ms(uint32_t ms);

#endif /* __MAIN_H */

/**@} end of group Template */
/**@} end of group Examples */
