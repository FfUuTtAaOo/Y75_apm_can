#include "lha7668-8.h"
#include "main.h"

uint8_t adc_init_count = 0;

lhl_lha7668_ctx_t lha7668_ctx = { 0 };  /* LHA7668B official driver handle */

void LHA7668_Platform_Init(void *handle)
{
    lhl_lha7668_ctx_t *dev_ctx = (lhl_lha7668_ctx_t *)handle;
    dev_ctx->io.cs.port = GPIOA;
    dev_ctx->io.cs.pin = GPIO_PIN_4;
}

void LHA7668_Platform_Set(const void *port, const uint32_t pin)
{
    if (port == NULL) return;
    
    // 将 void* 转换为 GPIO_TypeDef* 以符合 APM32 标准库操作
    GPIO_T *gpio_port = (GPIO_T *)port;
    
    // 使用标准库函数设置引脚为高电平
    // 注意：BOARD_GPIOOn 内部也是调用类似的 GPIO_SetBits
    GPIO_SetBit(gpio_port, (uint16_t)pin);
}

void LHA7668_Platform_Reset(const void *port, const uint32_t pin)
{
    if (port == NULL) return;
    
    // 将 void* 转换为 GPIO_TypeDef* 以符合 APM32 标准库操作
    GPIO_T *gpio_port = (GPIO_T *)port;

    // 使用标准库函数设置引脚为低电平
    GPIO_ResetBit(gpio_port, (uint16_t)pin);
}

int32_t LHA7668_Platform_ReadWrite(uint8_t *txdata, uint8_t *rxdata, const uint16_t size)
{
    uint16_t i;

    for (i = 0; i < size; i++) {
        // 1. 等待发送缓冲区空 (TXE)
        while (SPI_I2S_ReadStatusFlag(SPI1, SPI_FLAG_TXBE) == RESET);

        // 2. 发送一个字节
        SPI_I2S_TxData(SPI1, txdata[i]);

        // 3. 等待接收缓冲区非空 (RXNE)
        while (SPI_I2S_ReadStatusFlag(SPI1, SPI_FLAG_RXBNE) == RESET);

        // 4. 读取接收到的字节并存入 rxdata
        rxdata[i] = (uint8_t)SPI_I2S_RxData(SPI1);
    }

    return 0;
}

uint32_t int_adcData(void)
{
    LHL_LHA7668_Start(&lha7668_ctx, LHA7668_MODE_SINGLE_SHOT);
    while (LHL_LHA7668_Get_Flag(&lha7668_ctx, LHA7668_STATUS_RDY_FLAG) == LHA7668_SET);
    LHL_LHA7668_Get_Data(&lha7668_ctx);
    return lha7668_ctx.data;
}
float adcData(void)
{
    LHL_LHA7668_Start(&lha7668_ctx, LHA7668_MODE_SINGLE_SHOT);
    while (LHL_LHA7668_Get_Flag(&lha7668_ctx, LHA7668_STATUS_RDY_FLAG) == LHA7668_SET);
    LHL_LHA7668_Get_Data(&lha7668_ctx);
    return LHL_LHA7668_Get_mVoltage(lha7668_ctx.data, LHA7668_BIPOLAR, LHA7668_PGA_X128, 2500);
}

void lha7668_init(uint8_t rate)
{
    LHA7668_Platform_Init(&lha7668_ctx);
    LHL_LHA7668_Init(&lha7668_ctx);
    LHL_LHA7668_Reset(&lha7668_ctx);

    uint8_t whoamI = 0xFF;
    adc_init_count = 0;
    while (1) {
        whoamI = LHL_LHA7668_Get_ID(&lha7668_ctx);
        if (whoamI == LHA7668B_4) {
            break;
        } else {
            adc_init_count++;
        }
    }
    LHL_LHA7668_Stop(&lha7668_ctx);

    lha7668_ctx.CHANNEL.ENABLE = LHA7668_ENABLE;
    lha7668_ctx.CHANNEL.SETUP = LHA7668_SETUP_0;
    lha7668_ctx.CHANNEL.AINP = LHA7668_AIN0;
    lha7668_ctx.CHANNEL.AINM = LHA7668_AIN1;
    LHL_LHA7668_Set_Channel(&lha7668_ctx, LHA7668_CHANNEL_0);
    lha7668_ctx.CHANNEL.ENABLE = LHA7668_ENABLE;
    lha7668_ctx.CHANNEL.SETUP = LHA7668_SETUP_0;
    lha7668_ctx.CHANNEL.AINP = LHA7668_AIN2;
    lha7668_ctx.CHANNEL.AINM = LHA7668_AIN3;
    LHL_LHA7668_Set_Channel(&lha7668_ctx, LHA7668_CHANNEL_1);
    lha7668_ctx.CHANNEL.ENABLE = LHA7668_ENABLE;
    lha7668_ctx.CHANNEL.SETUP = LHA7668_SETUP_0;
    lha7668_ctx.CHANNEL.AINP = LHA7668_AIN4;
    lha7668_ctx.CHANNEL.AINM = LHA7668_AIN5;
    LHL_LHA7668_Set_Channel(&lha7668_ctx, LHA7668_CHANNEL_2);
    lha7668_ctx.CHANNEL.ENABLE = LHA7668_ENABLE;
    lha7668_ctx.CHANNEL.SETUP = LHA7668_SETUP_0;
    lha7668_ctx.CHANNEL.AINP = LHA7668_AIN6;
    lha7668_ctx.CHANNEL.AINM = LHA7668_AIN7;
    LHL_LHA7668_Set_Channel(&lha7668_ctx, LHA7668_CHANNEL_3);
    lha7668_ctx.CHANNEL.ENABLE = LHA7668_ENABLE;
    lha7668_ctx.CHANNEL.SETUP = LHA7668_SETUP_0;
    lha7668_ctx.CHANNEL.AINP = LHA7668_AIN8;
    lha7668_ctx.CHANNEL.AINM = LHA7668_AIN9;
    LHL_LHA7668_Set_Channel(&lha7668_ctx, LHA7668_CHANNEL_4);
    lha7668_ctx.CHANNEL.ENABLE = LHA7668_ENABLE;
    lha7668_ctx.CHANNEL.SETUP = LHA7668_SETUP_0;
    lha7668_ctx.CHANNEL.AINP = LHA7668_AIN10;
    lha7668_ctx.CHANNEL.AINM = LHA7668_AIN11;
    LHL_LHA7668_Set_Channel(&lha7668_ctx, LHA7668_CHANNEL_5);

    lha7668_ctx.SETUP.BIPOLAR = LHA7668_BIPOLAR;
    lha7668_ctx.SETUP.AIN_BUFP = LHA7668_ENABLE;
    lha7668_ctx.SETUP.AIN_BUFM = LHA7668_ENABLE;
    lha7668_ctx.SETUP.REF_SEL = LHA7668_REF_REFIN1;
    lha7668_ctx.SETUP.REF_BUFP = LHA7668_DISABLE;
    lha7668_ctx.SETUP.REF_BUFM = LHA7668_DISABLE;
    lha7668_ctx.SETUP.PGA = LHA7668_PGA_X128;
    lha7668_ctx.SETUP.FILTER = LHA7668_FILTER_SINC3;
    lha7668_ctx.SETUP.FS = rate;
    LHL_LHA7668_Set_Setup(&lha7668_ctx, LHA7668_SETUP_0);
    LHL_LHA7668_Set_Setup(&lha7668_ctx, LHA7668_SETUP_1);
    LHL_LHA7668_Set_Setup(&lha7668_ctx, LHA7668_SETUP_2);
    LHL_LHA7668_Set_Setup(&lha7668_ctx, LHA7668_SETUP_3);
    LHL_LHA7668_Set_Setup(&lha7668_ctx, LHA7668_SETUP_4);
    LHL_LHA7668_Set_Setup(&lha7668_ctx, LHA7668_SETUP_5);
    lha7668_ctx.ADC_CTRL.POWER_MODE = LHA7668_FULL_POWER;
    lha7668_ctx.ADC_CTRL.CLK_SEL = LHA7668_CLK_INTL;
    lha7668_ctx.ADC_CTRL.DATA_STATUS = LHA7668_ENABLE;
    lha7668_ctx.ADC_CTRL.CS_EN = LHA7668_DISABLE;
    lha7668_ctx.ADC_CTRL.DOUT_RDY_DEL = LHA7668_DISABLE;
    LHL_LHA7668_Set_ADC(&lha7668_ctx);
}
