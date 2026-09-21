/*!
 * @file        apm32f10x_int.c
 *
 * @brief      Main Interrupt Service Routines
 *
 * @version     V1.0.2
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

/* Includes */
#include "main.h"
#include "apm32f10x_int.h"
#include "lha7668-8.h"
#include "rs485.h"
#include "can_cmd.h"

extern volatile uint32_t g_tick_ms;
extern uint32_t adc_data_buff[64][6];
extern uint8_t adc_data_int_count;

extern CAN_RxMessage_T rxMsg;

/** @addtogroup Examples
  @{
  */

/** @addtogroup Template
  @{
  */

/** @defgroup Template_INT_Functions INT Functions
  @{
  */

/*!
 * @brief   This function handles NMI exception
 *
 * @param   None
 *
 * @retval  None
 *
 */
void NMI_Handler(void)
{
}

/*!
 * @brief   This function handles Hard Fault exception
 *
 * @param   None
 *
 * @retval  None
 *
 */
void HardFault_Handler(void)
{
    /* Go to infinite loop when Hard Fault exception occurs */
    while (1)
    {
    }
}

/*!
 * @brief   This function handles Memory Manage exception
 *
 * @param   None
 *
 * @retval  None
 *
 */
void MemManage_Handler(void)
{
    /* Go to infinite loop when Memory Manage exception occurs */
    while (1)
    {
    }
}

/*!
 * @brief   This function handles Bus Fault exception
 *
 * @param   None
 *
 * @retval  None
 *
 */
void BusFault_Handler(void)
{
    /* Go to infinite loop when Bus Fault exception occurs */
    while (1)
    {
    }
}

/*!
 * @brief   This function handles Usage Fault exception
 *
 * @param   None
 *
 * @retval  None
 *
 */
void UsageFault_Handler(void)
{
    /* Go to infinite loop when Usage Fault exception occurs */
    while (1)
    {
    }
}

/*!
 * @brief   This function handles SVCall exception
 *
 * @param   None
 *
 * @retval  None
 *
 */
void SVC_Handler(void)
{
}

/*!
 * @brief   This function handles Debug Monitor exception
 *
 * @param   None
 *
 * @retval  None
 *
 */
void DebugMon_Handler(void)
{
}

/*!
 * @brief   This function handles PendSV_Handler exception
 *
 * @param   None
 *
 * @retval  None
 *
 */
void PendSV_Handler(void)
{
}

/*!
 * @brief   This function handles SysTick Handler
 *
 * @param   None
 *
 * @retval  None
 *
 */
void SysTick_Handler(void)
{
}



/*!
 * @brief       USART1_Interrupt
 *
 * @param       None
 *
 * @retval      None
 *
 */
void USART1_Isr(void)
{
    /* Fallback: byte-by-byte reception when DMA is stopped or full.
     * During normal DMA transfers RXNE is cleared by DMA before the
     * interrupt can fire, so this only triggers when DMA is not
     * consuming the data (e.g. 512-byte buffer filled up). */
    if (USART_ReadIntFlag(USART1, USART_INT_RXBNE)) {
        rs485_rx_byte((uint8_t)USART_RxData(USART1));
    }

    /* Main path: DMA reception finished on idle line */
    if (USART_ReadIntFlag(USART1, USART_INT_IDLE)) {
        uint16_t remaining = DMA_ReadDataNumber(DMA1_Channel5);
        /* Standard IDLE clear sequence: read STS, then read DATA.
         * (APM32F103/STM32F103 IDLE bit cannot be cleared by software write.) */
        (void)USART1->STS;
        (void)USART1->DATA;
        rs485_rx_idle_cb(remaining);
    }
}

void USART1_IRQHandler(void)
{
    USART1_Isr();
}


/*!
 * @brief   CAN1 Rx interrupt Handler
 *
 * @param   None
 *
 * @retval  None
 *
 */
void CAN1_RX0_ISR(void)
{
    CAN_RxMessage(CAN1, CAN_RX_FIFO_0, &rxMsg);
    if (rxMsg.stdID == g_can_id_cmd) {
        can_rx_frame_isr(rxMsg.data, (uint8_t)rxMsg.dataLengthCode);
    }
}

void USBD1_LP_CAN1_RX0_IRQHandler(void)
{
    CAN1_RX0_ISR();
}

void SPI_MISO_Init(void)
{
    GPIO_Config_T GPIO_ConfigStruct;
    
    GPIO_ConfigStruct.pin =  GPIO_PIN_6;
    GPIO_ConfigStruct.mode = GPIO_MODE_IN_PU;
    GPIO_ConfigStruct.speed = GPIO_SPEED_50MHz;
    GPIO_Config(GPIOA, &GPIO_ConfigStruct);
}

/*!
 * @brief   This function handles External lines 0 Handler
 *
 * @param   None
 *
 * @retval  None
 *
 */

void EINT0_IRQHandler(void)
{
//    ADC_DRDY_CallBack();
}

void EINT9_5_IRQHandler(void)
{
    // ADC_DRDY_CallBack();
}


/*!
 * @brief   This function handles TMR2 Handler
 *
 * @param   None
 *
 * @retval  None
 *
 */
void TMR2_IRQHandler(void)
{
    if(TMR_ReadIntFlag(TMR2, TMR_INT_UPDATE) == SET)
    {
        g_tick_ms++;

        if (g_tick_ms % 2 == 0) {
            adc_data_buff[adc_data_int_count][0] = int_adcData();
            adc_data_buff[adc_data_int_count][1] = int_adcData();
            adc_data_buff[adc_data_int_count][2] = int_adcData();
            adc_data_buff[adc_data_int_count][3] = int_adcData();
            adc_data_buff[adc_data_int_count][4] = int_adcData();
            adc_data_buff[adc_data_int_count][5] = int_adcData();
            adc_data_int_count++;
            if (adc_data_int_count >= 64) {
                adc_data_int_count = 0;
            }
        }

        TMR_ClearIntFlag(TMR2, TMR_INT_UPDATE);
    }
}

/**@} end of group Template_INT_Functions */
/**@} end of group Template */
/**@} end of group Examples */
