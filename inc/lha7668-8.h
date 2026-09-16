#ifndef __LHA7668_8_H_
#define __LHA7668_8_H_

#include "lha7668.h"
#include "main.h"
#include "rs485.h"

#ifdef __cplusplus
extern "C" {
#endif

#define ADC_SAMPLING_RATE       3

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
