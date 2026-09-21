#ifndef __LHA7668_8_H_
#define __LHA7668_8_H_

#include "lha7668.h"
#include "main.h"
#include "rs485.h"

#ifdef __cplusplus
extern "C" {
#endif

#define ADC_SAMPLING_RATE       3

/* 单颗 LHA7668 使能的通道数，必须等于 main.h 里的 ADC_CHANNEL_NUM(6)。
   通道与六维力的对应关系（见 lha7668_init）：
       CH0 = AIN0-AIN1   -> raw[0] FX
       CH1 = AIN2-AIN3   -> raw[1] FY
       CH2 = AIN4-AIN5   -> raw[2] FZ
       CH3 = AIN6-AIN7   -> raw[3] TX
       CH4 = AIN8-AIN9   -> raw[4] TY
       CH5 = AIN10-AIN11 -> raw[5] TZ
   改这个宏，读取函数会自动跟随。 */
#define LHA7668_ADC_CH_NUM      ADC_CHANNEL_NUM

void LHA7668_Platform_Init(void *handle);
void LHA7668_Platform_Set(const void *port, const uint32_t pin);
void LHA7668_Platform_Reset(const void *port, const uint32_t pin);
int32_t LHA7668_Platform_ReadWrite(uint8_t *txdata, uint8_t *rxdata, const uint16_t size);
float adcData(void);
void lha7668_init(uint8_t rate);
uint32_t int_adcData(void);


#ifdef __cplusplus
}
#endif
#endif

/*********************************End of File**********************************/
