/*!
 * @file        main.c
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

/* Includes */
#include "main.h"
#include "apm32f10x_fmc.h"
#include "lha7668-8.h"
#include "self_test.h"
#include "rs485_cmd.h"
#include "sensor_types.h"
#include "flash_storage.h"
#include "rs485.h"
#include "calibration.h"
#include "can_cmd.h"
#include "at24c04.h"

#define ADC_DATA_BUFF_LEN       64
#define KILOGRAM_TO_NEWTON      9.8
/*--------variable---------------------------------------*/
volatile uint32_t led_flicker = 0;

volatile sensor_data_t   g_sensor;
volatile system_state_t  g_sys;
config_t                 g_config = {
    .sn         = "HS-01234567",
    .fw_version = 0x00010000,
};
decouple_matrix_t        g_matrix;

uint8_t output_interface = 0;

static uint32_t g_last_rs485_send_ms;
volatile uint32_t g_tick_ms = 0;

static uint8_t asc_buff[128] = { 0 };

uint32_t adc_data_buff[64][6] = { 0 };
uint8_t adc_data_int_count = 0;
uint8_t adc_data_usr_count = 0;

static float adc_data_calculate(uint8_t index)
{
    return (float)((int32_t)adc_data_buff[adc_data_usr_count][index] - 0x800000) * (1.0f / 8388608.0f) * 2500;
}

static void adc_data_proc(float data[6])
{
    int i = 0;
    if (g_sys.data_format == 0) {
        for (i = 0; i < ADC_CHANNEL_NUM; i++) {
            g_sensor.force[i] = data[i];
        }
    } else if (g_sys.data_format == 1) {
        g_sensor.force[0] = g_matrix.m[0][0] * data[0] / KILOGRAM_TO_NEWTON + g_matrix.m[0][1] * data[1] / KILOGRAM_TO_NEWTON +
                    g_matrix.m[0][2] * data[2] / KILOGRAM_TO_NEWTON + g_matrix.m[0][3] * data[3] / KILOGRAM_TO_NEWTON +
                    g_matrix.m[0][4] * data[4] / KILOGRAM_TO_NEWTON + g_matrix.m[0][5] * data[5] / KILOGRAM_TO_NEWTON;

        g_sensor.force[1] = g_matrix.m[1][0] * data[0] / KILOGRAM_TO_NEWTON + g_matrix.m[1][1] * data[1] / KILOGRAM_TO_NEWTON +
                    g_matrix.m[1][2] * data[2] / KILOGRAM_TO_NEWTON + g_matrix.m[1][3] * data[3] / KILOGRAM_TO_NEWTON +
                    g_matrix.m[1][4] * data[4] / KILOGRAM_TO_NEWTON + g_matrix.m[1][5] * data[5] / KILOGRAM_TO_NEWTON;

        g_sensor.force[2] = g_matrix.m[2][0] * data[0] / KILOGRAM_TO_NEWTON + g_matrix.m[2][1] * data[1] / KILOGRAM_TO_NEWTON +
                    g_matrix.m[2][2] * data[2] / KILOGRAM_TO_NEWTON + g_matrix.m[2][3] * data[3] / KILOGRAM_TO_NEWTON +
                    g_matrix.m[2][4] * data[4] / KILOGRAM_TO_NEWTON + g_matrix.m[2][5] * data[5] / KILOGRAM_TO_NEWTON;

        g_sensor.force[3] = g_matrix.m[3][0] * data[0] / KILOGRAM_TO_NEWTON + g_matrix.m[3][1] * data[1] / KILOGRAM_TO_NEWTON +
                    g_matrix.m[3][2] * data[2] / KILOGRAM_TO_NEWTON + g_matrix.m[3][3] * data[3] / KILOGRAM_TO_NEWTON +
                    g_matrix.m[3][4] * data[4] / KILOGRAM_TO_NEWTON + g_matrix.m[3][5] * data[5] / KILOGRAM_TO_NEWTON;

        g_sensor.force[4] = g_matrix.m[4][0] * data[0] / KILOGRAM_TO_NEWTON + g_matrix.m[4][1] * data[1] / KILOGRAM_TO_NEWTON +
                    g_matrix.m[4][2] * data[2] / KILOGRAM_TO_NEWTON + g_matrix.m[4][3] * data[3] / KILOGRAM_TO_NEWTON +
                    g_matrix.m[4][4] * data[4] / KILOGRAM_TO_NEWTON + g_matrix.m[4][5] * data[5] / KILOGRAM_TO_NEWTON;

        g_sensor.force[5] = g_matrix.m[5][0] * data[0] / KILOGRAM_TO_NEWTON + g_matrix.m[5][1] * data[1] / KILOGRAM_TO_NEWTON +
                    g_matrix.m[5][2] * data[2] / KILOGRAM_TO_NEWTON + g_matrix.m[5][3] * data[3] / KILOGRAM_TO_NEWTON +
                    g_matrix.m[5][4] * data[4] / KILOGRAM_TO_NEWTON + g_matrix.m[5][5] * data[5] / KILOGRAM_TO_NEWTON;
    } else if (g_sys.data_format == 2) {
        g_sensor.force[0] = g_matrix.m[0][0] * data[0] + g_matrix.m[0][1] * data[1] +
                    g_matrix.m[0][2] * data[2] + g_matrix.m[0][3] * data[3] +
                    g_matrix.m[0][4] * data[4] + g_matrix.m[0][5] * data[5];

        g_sensor.force[1] = g_matrix.m[1][0] * data[0] + g_matrix.m[1][1] * data[1] +
                    g_matrix.m[1][2] * data[2] + g_matrix.m[1][3] * data[3] +
                    g_matrix.m[1][4] * data[4] + g_matrix.m[1][5] * data[5];

        g_sensor.force[2] = g_matrix.m[2][0] * data[0] + g_matrix.m[2][1] * data[1] +
                    g_matrix.m[2][2] * data[2] + g_matrix.m[2][3] * data[3] +
                    g_matrix.m[2][4] * data[4] + g_matrix.m[2][5] * data[5];

        g_sensor.force[3] = g_matrix.m[3][0] * data[0] + g_matrix.m[3][1] * data[1] +
                    g_matrix.m[3][2] * data[2] + g_matrix.m[3][3] * data[3] +
                    g_matrix.m[3][4] * data[4] + g_matrix.m[3][5] * data[5];

        g_sensor.force[4] = g_matrix.m[4][0] * data[0] + g_matrix.m[4][1] * data[1] +
                    g_matrix.m[4][2] * data[2] + g_matrix.m[4][3] * data[3] +
                    g_matrix.m[4][4] * data[4] + g_matrix.m[4][5] * data[5];

        g_sensor.force[5] = g_matrix.m[5][0] * data[0] + g_matrix.m[5][1] * data[1] +
                    g_matrix.m[5][2] * data[2] + g_matrix.m[5][3] * data[3] +
                    g_matrix.m[5][4] * data[4] + g_matrix.m[5][5] * data[5];
    }

    for (i = 0; i < ADC_CHANNEL_NUM; i++) {
        g_sensor.force[i] = FloatFilter_UpdateChannel(i, g_sensor.force[i]);
    }
}

uint8_t hex_buf_send[256] = { 0 };

#define HEX_BUF_SEND_OFFSET 1

static void rs485_send_continuous(void)
{
    uint32_t interval = (g_sys.freq_mode == 0) ? 2U : 1U;  /* 500Hz or 1kHz */
    if ((g_tick_ms - g_last_rs485_send_ms) >= interval) {
        g_last_rs485_send_ms = g_tick_ms;

        if (g_sys.debug_mode == 0) {
            if (output_interface == 1) {
                /* Build data frame: AA 55 00 [SEQ4B] [6×float32] 0D 0A */
                // uint32_t seq = g_sys.frame_seq++;
                hex_buf_send[0] = RS485_CMD_CONT_DATA;
                // hex_buf_send[1] = (uint8_t)(seq);
                // hex_buf_send[2] = (uint8_t)(seq >> 8);
                // hex_buf_send[3] = (uint8_t)(seq >> 16);
                // hex_buf_send[4] = (uint8_t)(seq >> 24);
                for (int i = 0; i < 6; i++) {
                    float f = g_sensor.force[i];   /* read volatile once */
                    uint32_t u; memcpy(&u, &f, 4);
                    hex_buf_send[HEX_BUF_SEND_OFFSET + i * 4 + 0] = (uint8_t)(u);
                    hex_buf_send[HEX_BUF_SEND_OFFSET + i * 4 + 1] = (uint8_t)(u >> 8);
                    hex_buf_send[HEX_BUF_SEND_OFFSET + i * 4 + 2] = (uint8_t)(u >> 16);
                    hex_buf_send[HEX_BUF_SEND_OFFSET + i * 4 + 3] = (uint8_t)(u >> 24);
                }

                rs485_send_raw(hex_buf_send, 24 + HEX_BUF_SEND_OFFSET);
            }
        } else if (g_sys.debug_mode == 1) {
            for (int i = 0; i < 6; i++) {
                float f = g_sensor.force[i];   /* read volatile once */
                uint32_t u; memcpy(&u, &f, 4);
                hex_buf_send[i * 4 + 0] = (uint8_t)(u);
                hex_buf_send[i * 4 + 1] = (uint8_t)(u >> 8);
                hex_buf_send[i * 4 + 2] = (uint8_t)(u >> 16);
                hex_buf_send[i * 4 + 3] = (uint8_t)(u >> 24);
            }
            hex_buf_send[24] = 0x00;
            hex_buf_send[25] = 0x00;
            hex_buf_send[26] = 0x80;
            hex_buf_send[27] = 0x7F;
            just_float_send_raw(hex_buf_send, 28);
        }
    }
}

int main(void)
{
    /* System clocks configuration */
    SystemClockConfig();

    GPIO_Init();
    USART1_RX_DMA_Init();
    USART1_Init();
    SPI1_Init();
    TIM2_Init();
    I2C_Init();

    /* 从 EEPROM 恢复单位 (0x7C) 与 CAN ID (0x98)，须在 CAN 中断使能前完成 */
    eeprom_config_load();

    CAN_Init();

    USART_EnableDMA(USART1, USART_DMA_RX);

    calib_init();
    FloatFilter_Init();

    BOARD_GPIOOn(SPI1_CS);

    lha7668_init(ADC_SAMPLING_RATE);

    rs485_init(USART1, rs485_cmd_dispatch);

    flash_load_all();

//test
    memcpy(asc_buff, "Init OK", 7);
    asc_send(asc_buff, 7);

    /* 上电完成通知：CAN ID 0x201，数据 00 01 02 03 04 05 06 07 */
    {
        static const uint8_t power_on_frame[CAN_DATA_LENGTH] = {
            0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07
        };
        can_cmd_send_frame(g_can_id_cmd, power_on_frame);
    }

    TMR_Enable(TMR2);

    while(1) {
        {
            float adc_tmp[ADC_CHANNEL_NUM] = { 0 };

            if (adc_data_usr_count != adc_data_int_count) {
                adc_data_usr_count = adc_data_int_count;
                for (uint8_t i = 0; i < ADC_CHANNEL_NUM; i++) {
                    adc_tmp[i] = adc_data_calculate(i);

                    if (g_sys.zero_calib_busy == 0) {
                        adc_tmp[i] -= g_sensor.force_zero[i];
                    }
                }

                if (g_sys.zero_calib_busy == 1) {
                    calib_zero_cale(adc_tmp);
                }

                adc_data_proc(adc_tmp);
            }
        }

        if (calib_zero_is_done()) {
            g_sys.zero_calib_busy = 0;
            calib_zero_over();
            flash_save_zero();
        }

        if (g_sys.send_mode == 1) {
            rs485_send_continuous();
        } else if (g_sys.send_mode == 2) {
            can_cmd_send_continuous();
        } else {
            g_last_rs485_send_ms = g_tick_ms;   /* reset timer on stop */
        }
        
        rs485_process();

        can_cmd_process();
    }
}

int fputc(int ch, FILE *f)
{
    /* send a byte of data to the serial port */
    USART_TxData(USART1, (uint8_t)ch);

    /* wait for the data to be send  */
    while (USART_ReadStatusFlag(USART1, USART_FLAG_TXBE) == RESET);

    return (ch);
}

void Delay(uint32_t count)
{
    volatile uint32_t delay = count * 8000;

    while (delay--);
}

/**
 * @brief  Precise millisecond delay based on the TIM2 1ms tick.
 * @note   Requires TIM2 to be running with TMR2 interrupt enabled
 *         (TIM2_Init() + TMR_Enable(TMR2) done in main).
 *         Uses unsigned wrap-around, so it handles long delays
 *         without overflow issues.
 */
void delay_ms(uint32_t ms)
{
    uint32_t start = g_tick_ms;

    while ((uint32_t)(g_tick_ms - start) < ms) {
    }
}

/**@} end of group Template_Functions */
/**@} end of group Template */
/**@} end of group Examples */
