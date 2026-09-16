/*!
 * @file        Board.c
 *
 * @brief       This file provides firmware functions to manage Leds and push-buttons
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

/* Includes */
#include "Board.h"

#if defined (APM32F103_MINI)
#include "Board_APM32F103_MINI.h"

#elif defined (APM32F107_MINI)
#include "board_apm32f107_mini.h"

#else
#error "Please select first the APM32 board to be used (in board.c)"
#endif

#define USART1_DR_ADDRESS        (USART1_BASE+0x04)

uint8_t DMA_USART1_RxBuf[0xFF];

void GPIO_Init(void)
{
    BOARD_GPIOOff(SPI1_CS);
    BOARD_GPIOOff(RS485_DE);

    BOARD_LEDInit(SPI1_CS);
    BOARD_LEDInit(RS485_DE);

    BOARD_GPIOOn(SPI1_CS);
}

void USART1_Init(void)
{
    USART_Config_T USART_ConfigStruct;

    USART_ConfigStruct.baudRate = 115200;
    USART_ConfigStruct.hardwareFlow = USART_HARDWARE_FLOW_NONE;
    USART_ConfigStruct.mode = USART_MODE_TX_RX;
    USART_ConfigStruct.parity = USART_PARITY_NONE;
    USART_ConfigStruct.stopBits = USART_STOP_BIT_1;
    USART_ConfigStruct.wordLength = USART_WORD_LEN_8B;
    BOARD_COMInit(COM1, &USART_ConfigStruct);

    /* RXBNE: byte-by-byte fallback when DMA is stopped/full.
     * IDLE:  main path, triggers when the bus goes idle. */
    USART_EnableInterrupt(USART1, USART_INT_RXBNE);
    USART_EnableInterrupt(USART1, USART_INT_IDLE);

    NVIC_EnableIRQRequest(USART1_IRQn, 0, 0);
}

void AFIO_USART1_Init(void)
{
    GPIO_Config_T GPIO_ConfigStruct;
    USART_Config_T USART_ConfigStruct;

    RCM_EnableAPB2PeriphClock(RCM_APB2_PERIPH_USART1);
    RCM_EnableAPB2PeriphClock(RCM_APB2_PERIPH_GPIOB | RCM_APB2_PERIPH_AFIO);
    GPIO_ConfigPinRemap(GPIO_REMAP_USART1);

    GPIO_ConfigStruct.mode = GPIO_MODE_AF_PP;
    GPIO_ConfigStruct.speed = GPIO_SPEED_50MHz;
    GPIO_ConfigStruct.pin = GPIO_PIN_6;
    GPIO_Config(GPIOB, &GPIO_ConfigStruct);

    GPIO_ConfigStruct.mode = GPIO_MODE_IN_FLOATING;
    GPIO_ConfigStruct.speed = GPIO_SPEED_50MHz;
    GPIO_ConfigStruct.pin = GPIO_PIN_7;
    GPIO_Config(GPIOB, &GPIO_ConfigStruct);

    USART_ConfigStruct.baudRate = 115200;
    USART_ConfigStruct.hardwareFlow = USART_HARDWARE_FLOW_NONE;
    USART_ConfigStruct.mode = USART_MODE_TX_RX;
    USART_ConfigStruct.parity = USART_PARITY_NONE;
    USART_ConfigStruct.stopBits = USART_STOP_BIT_1;
    USART_ConfigStruct.wordLength = USART_WORD_LEN_8B;
    BOARD_COMInit(COM1, &USART_ConfigStruct);

    USART_EnableInterrupt(USART1, USART_INT_RXBNE);
    
    NVIC_EnableIRQRequest(USART1_IRQn, 0, 0);
}

void SPI1_Init(void)
{
    GPIO_Config_T GPIO_ConfigStruct;
    SPI_Config_T SPI_ConfigStruct;

    /* Enable related Clock */
    RCM_EnableAPB2PeriphClock(RCM_APB2_PERIPH_SPI1);
    RCM_EnableAPB2PeriphClock(RCM_APB2_PERIPH_GPIOA);

    /* config PIN_6  MISO */
    GPIO_ConfigStruct.pin =  GPIO_PIN_6;
    GPIO_ConfigStruct.mode = GPIO_MODE_IN_FLOATING;
    GPIO_ConfigStruct.speed = GPIO_SPEED_50MHz;
    GPIO_Config(GPIOA, &GPIO_ConfigStruct);

    /* config PIN_5->SCK , PIN_7->MOSI */
    GPIO_ConfigStruct.pin = GPIO_PIN_5 | GPIO_PIN_7;
    GPIO_ConfigStruct.mode = GPIO_MODE_AF_PP;
    GPIO_ConfigStruct.speed = GPIO_SPEED_50MHz;
    GPIO_Config(GPIOA, &GPIO_ConfigStruct);

    SPI_ConfigStructInit(&SPI_ConfigStruct);

    SPI_ConfigStruct.length = SPI_DATA_LENGTH_8B;
    SPI_ConfigStruct.baudrateDiv = SPI_BAUDRATE_DIV_32;
    SPI_ConfigStruct.direction = SPI_DIRECTION_2LINES_FULLDUPLEX;
    SPI_ConfigStruct.firstBit = SPI_FIRSTBIT_MSB;
    SPI_ConfigStruct.mode = SPI_MODE_MASTER;
    SPI_ConfigStruct.polarity = SPI_CLKPOL_LOW;
    SPI_ConfigStruct.nss = SPI_NSS_SOFT;
    SPI_ConfigStruct.phase = SPI_CLKPHA_1EDGE;
    /* SPI config */
    SPI_Config(SPI1, &SPI_ConfigStruct);

    SPI_ConfigDataSize(SPI1, SPI_DATA_LENGTH_8B);

    SPI_Enable(SPI1);
}

void TIM2_Init(void)
{
    TMR_BaseConfig_T TIM2_ConfigStruct;

    RCM_EnableAPB1PeriphClock(RCM_APB1_PERIPH_TMR2);

    TIM2_ConfigStruct.clockDivision = TMR_CLOCK_DIV_1;
    TIM2_ConfigStruct.countMode = TMR_COUNTER_MODE_UP;
    TIM2_ConfigStruct.division = 72 -1;
    TIM2_ConfigStruct.period = 1000 - 1;
    TIM2_ConfigStruct.repetitionCounter = 0;
    TMR_ConfigTimeBase(TMR2, &TIM2_ConfigStruct);

    TMR_ClearStatusFlag(TMR2,TMR_FLAG_UPDATE);
    TMR_EnableInterrupt(TMR2, TMR_INT_UPDATE);
    NVIC_EnableIRQRequest(TMR2_IRQn, 1, 0);
//    TMR_Enable(TMR2);
}

void I2C_Init(void)
{
    GPIO_Config_T GPIO_ConfigStruct;
    I2C_Config_T I2C_ConfigStruct;

//I2C1
    /* Enable I2C related Clock */
   RCM_EnableAPB2PeriphClock(RCM_APB2_PERIPH_GPIOB | RCM_APB2_PERIPH_AFIO);
   RCM_EnableAPB1PeriphClock(RCM_APB1_PERIPH_I2C1);
//    GPIO_ConfigPinRemap(GPIO_REMAP_I2C1);

   /* Free I2C_SCL and I2C_SDA */
   GPIO_ConfigStruct.mode = GPIO_MODE_AF_OD;
   GPIO_ConfigStruct.speed = GPIO_SPEED_50MHz;
   GPIO_ConfigStruct.pin = GPIO_PIN_6;
   GPIO_Config(GPIOB, &GPIO_ConfigStruct);

   GPIO_ConfigStruct.mode = GPIO_MODE_AF_OD;
   GPIO_ConfigStruct.speed = GPIO_SPEED_50MHz;
   GPIO_ConfigStruct.pin = GPIO_PIN_7;
   GPIO_Config(GPIOB, &GPIO_ConfigStruct);

   /* Config I2C1 */
   I2C_Reset(I2C1);
   I2C_ConfigStruct.mode = I2C_MODE_I2C;
   I2C_ConfigStruct.dutyCycle = I2C_DUTYCYCLE_2;
   I2C_ConfigStruct.ackAddress = I2C_ACK_ADDRESS_7BIT;
   I2C_ConfigStruct.ownAddress1 = 0x0;
   I2C_ConfigStruct.ack = I2C_ACK_ENABLE;
   I2C_ConfigStruct.clockSpeed = 400000;
   I2C_Config(I2C1,&I2C_ConfigStruct);

   /* Enable I2Cx */
   I2C_Enable(I2C1);
    
//I2C2
    // /* Enable I2C related Clock */
    // RCM_EnableAPB2PeriphClock(RCM_APB2_PERIPH_GPIOB | RCM_APB2_PERIPH_AFIO);
    // RCM_EnableAPB1PeriphClock(RCM_APB1_PERIPH_I2C2);

    // /* Free I2C_SCL and I2C_SDA */
    // GPIO_ConfigStruct.mode = GPIO_MODE_AF_OD;
    // GPIO_ConfigStruct.speed = GPIO_SPEED_50MHz;
    // GPIO_ConfigStruct.pin = GPIO_PIN_10;
    // GPIO_Config(GPIOB, &GPIO_ConfigStruct);

    // GPIO_ConfigStruct.mode = GPIO_MODE_AF_OD;
    // GPIO_ConfigStruct.speed = GPIO_SPEED_50MHz;
    // GPIO_ConfigStruct.pin = GPIO_PIN_11;
    // GPIO_Config(GPIOB, &GPIO_ConfigStruct);

    // /* Config I2C1 */
    // I2C_Reset(I2C2);
    // I2C_ConfigStruct.mode = I2C_MODE_I2C;
    // I2C_ConfigStruct.dutyCycle = I2C_DUTYCYCLE_2;
    // I2C_ConfigStruct.ackAddress = I2C_ACK_ADDRESS_7BIT;
    // I2C_ConfigStruct.ownAddress1 = 0xA0;
    // I2C_ConfigStruct.ack = I2C_ACK_ENABLE;
    // I2C_ConfigStruct.clockSpeed = 400000;
    // I2C_Config(I2C2,&I2C_ConfigStruct);

    // /* Enable I2Cx */
    // I2C_Enable(I2C2);
}

void USART1_RX_DMA_Init(void)
{
    DMA_Config_T dmaConfig;

    /* Enable DMA Clock */
    RCM_EnableAHBPeriphClock(RCM_AHB_PERIPH_DMA1);

    /* DMA config */
    dmaConfig.peripheralBaseAddr = USART1_DR_ADDRESS;
    dmaConfig.memoryBaseAddr = (uint32_t)DMA_USART1_RxBuf;
    dmaConfig.dir = DMA_DIR_PERIPHERAL_SRC;
    dmaConfig.bufferSize = 0xFF;
    dmaConfig.peripheralInc = DMA_PERIPHERAL_INC_DISABLE;
    dmaConfig.memoryInc = DMA_MEMORY_INC_ENABLE;
    dmaConfig.peripheralDataSize = DMA_PERIPHERAL_DATA_SIZE_BYTE;
    dmaConfig.memoryDataSize = DMA_MEMORY_DATA_SIZE_BYTE;
    dmaConfig.loopMode = DMA_MODE_NORMAL;
    dmaConfig.priority = DMA_PRIORITY_MEDIUM;
    dmaConfig.M2M = DMA_M2MEN_DISABLE;
    
    /* Enable DMA channel */
    DMA_Config(DMA1_Channel5, &dmaConfig);
    
    /* Enable DMA */
    DMA_Enable(DMA1_Channel5);
}


static void CAN_GPIO_Init(void)
{
    GPIO_Config_T configStruct;

    RCM_EnableAPB2PeriphClock(RCM_APB2_PERIPH_AFIO);
    RCM_EnableAPB2PeriphClock(RCM_APB2_PERIPH_GPIOA);

    configStruct.pin   = GPIO_PIN_12;    // CAN1 Tx
    configStruct.mode  = GPIO_MODE_AF_PP;
    configStruct.speed = GPIO_SPEED_50MHz;
    GPIO_Config(GPIOA, &configStruct);

    configStruct.pin   = GPIO_PIN_11;     // CAN1 Rx
    configStruct.mode  = GPIO_MODE_IN_PU;
    GPIO_Config(GPIOA, &configStruct);
}

static void CAN_Config_Init(void)
{
    CAN_Config_T       CAN_ConfigStruct;
    CAN_FilterConfig_T FilterStruct;

    CAN_Reset(CAN1);

    CAN_ConfigStructInit(&CAN_ConfigStruct);
    
    /* CAN1 init */
    CAN_ConfigStruct.autoBusOffManage = DISABLE;
    CAN_ConfigStruct.autoWakeUpMode   = DISABLE;
    CAN_ConfigStruct.nonAutoRetran    = DISABLE;
    CAN_ConfigStruct.rxFIFOLockMode   = DISABLE;
    CAN_ConfigStruct.txFIFOPriority   = ENABLE;
    CAN_ConfigStruct.mode             = CAN_MODE_NORMAL;
    CAN_ConfigStruct.syncJumpWidth    = CAN_SJW_1;
    CAN_ConfigStruct.timeSegment1     = CAN_TIME_SEGMENT1_9;
    CAN_ConfigStruct.timeSegment2     = CAN_TIME_SEGMENT2_2;
    CAN_ConfigStruct.prescaler = 6;

    CAN_Config(CAN1, &CAN_ConfigStruct);

    FilterStruct.filterNumber = 1;
    FilterStruct.filterMode = CAN_FILTER_MODE_IDMASK;
    FilterStruct.filterScale = CAN_FILTER_SCALE_32BIT;
    FilterStruct.filterIdHigh = 0x0000;
    FilterStruct.filterIdLow  = 0x0000;
    FilterStruct.filterMaskIdHigh = 0x0000;
    FilterStruct.filterMaskIdLow  = 0x0000;
    FilterStruct.filterFIFO = CAN_FILTER_FIFO_0;
    FilterStruct.filterActivation =  ENABLE;

    CAN_ConfigFilter(CAN1, &FilterStruct);
}

/* CAN transmit frames and receive frame (declared extern in Board.h) */
CAN_TxMessage_T txMsg[CAN_TX_NUM];
CAN_RxMessage_T rxMsg;

/* CAN transmit frame ID table, index matches CAN_TxIndex_T */
static const uint32_t canTxId[CAN_TX_NUM] = {
    CAN_ID_TX_FORCE,
    CAN_ID_TX_MOMENT,
    CAN_ID_TX_FYFZ,
    CAN_ID_TX_CMD,
};

static void CAN_Transmit_Init(void)
{
    uint8_t i;

    for (i = 0; i < CAN_TX_NUM; i++) {
        txMsg[i].stdID           = canTxId[i];
        txMsg[i].remoteTxReq     = CAN_RTXR_DATA;
        txMsg[i].typeID          = CAN_TYPEID_STD;
        txMsg[i].dataLengthCode  = CAN_DATA_LENGTH;
    }
}

void CAN_Init(void)
{
    RCM_EnableAPB1PeriphClock(RCM_APB1_PERIPH_CAN1);
    
    CAN_GPIO_Init();
    CAN_Config_Init();
    CAN_Transmit_Init();
    CAN_EnableInterrupt(CAN1, CAN_INT_F0MP);
    NVIC_EnableIRQRequest(USBD1_LP_CAN1_RX0_IRQn, 0, 0);
}

/** @addtogroup Board
  @{

*/

/**@} end of group Board */
