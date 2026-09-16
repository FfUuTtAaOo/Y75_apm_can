/*!
 * @file        board_delay.c
 *
 * @brief       Delay board support package body
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

/* Includes ***************************************************************/
#include "board_delay.h"

/* Private includes *******************************************************/

/* Private macro **********************************************************/

/* Private typedef ********************************************************/

/* Private variables ******************************************************/

/* Private function prototypes ********************************************/

/* External variables *****************************************************/
volatile uint32_t delayCount;

/* External functions *****************************************************/

/*!
 * @brief   Delay configuration
 *
 * @param   None
 *
 * @retval  None
 */
void BOARD_DelayConfig(void)
{
    /* Update SystemCoreClock */
    SystemCoreClockUpdate();
    /* Update SystemCoreClock */
    if(SysTick_Config(SystemCoreClock/1000))
    {
        while (1);
    }
}

/*!
 * @brief   Get tick
 *
 * @param   count: Specifies delay time
 *
 * @retval  None
 */
uint32_t BOARD_ReadTick(void)
{
    return delayCount;
}

/*!
 * @brief   Millisecond delay
 *
 * @param   count: Specifies delay time
 *
 * @retval  None
 */
void BOARD_DelayMs(uint32_t count)
{
    /* Update SystemCoreClock */
    if(SysTick_Config(SystemCoreClock/1000))
    {
        while (1);
    }

    uint32_t tickStart = BOARD_ReadTick();

    while ((BOARD_ReadTick() - tickStart) < count);
}


/*!
 * @brief   Microsecond delay
 *
 * @param   count: Specifies delay time
 *
 * @retval  None
 */
void BOARD_DelayUs(uint32_t count)
{
    /* Update SystemCoreClock */
    if(SysTick_Config(SystemCoreClock/1000000))
    {
        while(1);
    }

    uint32_t tickStart = BOARD_ReadTick();

    while ((BOARD_ReadTick() - tickStart) < count);
}

/*!
 * @brief   Delay interrupt service handling
 *
 * @param   None
 *
 * @retval  None
 */
void BOARD_DelayIRQHandler(void)
{
    delayCount++;
}
