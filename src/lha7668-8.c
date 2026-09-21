#include "lha7668-8.h"
#include "main.h"
#include <string.h>

uint8_t adc_init_count = 0;

lhl_lha7668_ctx_t lha7668_ctx = { 0 };  /* LHA7668B official driver handle */

/* 轮询 DRDY 的最大次数。
   轮询一次 = 读一次 STATUS 寄存器（含 CS 翻转），约 10us；
   5000 次 ≈ 50ms，远超单次转换时间，同时保证绝不会在中断里死等。 */
#define LHA7668_RDY_POLL_MAX    5000U

/* 初始化时读取芯片 ID 的最大尝试次数：ID 对不上也不能把 main() 卡死 */
#define LHA7668_ID_RETRY_MAX    100U

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
    uint32_t poll = LHA7668_RDY_POLL_MAX;

    LHL_LHA7668_Start(&lha7668_ctx, LHA7668_MODE_SINGLE_SHOT);
    /* 有上限的等待：TMR2 中断里一次要调 6 次，死等会把主循环彻底饿死 */
    while (LHL_LHA7668_Get_Flag(&lha7668_ctx, LHA7668_STATUS_RDY_FLAG) == LHA7668_SET) {
        if (--poll == 0U) {
            return lha7668_ctx.data;      /* 超时：返回上一次的值，绝不卡死 */
        }
    }
    LHL_LHA7668_Get_Data(&lha7668_ctx);
    return lha7668_ctx.data;
}

/* 编译期确认使能的通道数正好等于六维力通道数 */
typedef char adc_ch_num_check_t[LHA7668_ADC_CH_NUM == ADC_CHANNEL_NUM ? 1 : -1];

/*!
 * @brief   整组读取一颗 LHA7668 上全部使能通道的一次转换结果
 *
 * @param   ctx    : 芯片句柄
 * @param   buf    : 输出缓冲，长度 >= ch_num
 * @param   ch_num : 该芯片使能的通道数
 *
 * @retval  0 读取成功；-1 超时或通道序列异常（此时整组数据不更新）
 *
 * @note    每次 DRDY 后读一通道，用 STATUS.CH_ACTIVE 决定放到 buf 的哪个槽位，
 *          因此即使某次转换晚了/跳了，也不会出现通道错位；
 *          只要有一个通道缺漏就整组丢弃，各通道保留上一次的有效值。
 */
static int32_t lha7668_read_group(lhl_lha7668_ctx_t *ctx, uint32_t *buf, uint8_t ch_num)
{
    uint8_t  i;
    uint16_t seen = 0;
    uint32_t poll;

    LHL_LHA7668_Start(ctx, LHA7668_MODE_SINGLE_SHOT);

    for (i = 0; i < ch_num; i++) {
        /* RDY = 1 表示转换未完成，等它变 0（数据就绪） */
        poll = LHA7668_RDY_POLL_MAX;
        while (LHL_LHA7668_Get_Flag(ctx, LHA7668_STATUS_RDY_FLAG) == LHA7668_SET) {
            if (--poll == 0U) {
                return -1;                 /* 超时：整组丢弃 */
            }
        }

        LHL_LHA7668_Get_Data(ctx);

        uint8_t ch = (uint8_t)(ctx->status & LHA7668_STATUS_CH_ACTIVE_MSK);
        if (ch >= ch_num) {
            return -1;                     /* 序列异常：出现了不存在的通道 */
        }
        if (seen & (uint16_t)(1U << ch)) {
            return -1;                     /* 序列异常：通道重复，缺了其它通道 */
        }
        seen |= (uint16_t)(1U << ch);

        buf[ch] = ctx->data;
    }

    return 0;
}

/*!
 * @brief   一次读取六路 ADC 原始值（FX/FY/FZ/TX/TY/TZ）
 *
 * @param   raw : 输出缓冲，长度 ADC_CHANNEL_NUM(6)
 *
 * @retval  0 成功，-1 失败（此时 raw 内容不会被改写）
 */
int32_t int_adcData_6ch(uint32_t *raw)
{
    uint32_t tmp[ADC_CHANNEL_NUM];

    if (lha7668_read_group(&lha7668_ctx, tmp, (uint8_t)LHA7668_ADC_CH_NUM) != 0) {
        return -1;
    }

    memcpy(raw, tmp, sizeof(tmp));
    return 0;
}

float adcData(void)
{
    uint32_t poll = LHA7668_RDY_POLL_MAX;

    LHL_LHA7668_Start(&lha7668_ctx, LHA7668_MODE_SINGLE_SHOT);
    while (LHL_LHA7668_Get_Flag(&lha7668_ctx, LHA7668_STATUS_RDY_FLAG) == LHA7668_SET) {
        if (--poll == 0U) {
            break;                        /* 超时：用上一次的值，绝不卡死 */
        }
    }
    LHL_LHA7668_Get_Data(&lha7668_ctx);
    return LHL_LHA7668_Get_mVoltage(lha7668_ctx.data, LHA7668_BIPOLAR, LHA7668_PGA_X128, 2500);
}

void lha7668_init(uint8_t rate)
{
    LHA7668_Platform_Init(&lha7668_ctx);
    LHL_LHA7668_Init(&lha7668_ctx);
    LHL_LHA7668_Reset(&lha7668_ctx);

    /* 注意：这里绝对不能用 while(1) 死等 ID。
       一旦 SPI 不通或芯片型号判断错（4 通道 0x18 / 8 通道 0x28），
       main() 就永远走不到 CAN 初始化之后的代码，表现为"CAN 发什么都没反应"。 */
    uint8_t whoamI = 0xFF;
    adc_init_count = 0;
    while (adc_init_count < LHA7668_ID_RETRY_MAX) {
        whoamI = LHL_LHA7668_Get_ID(&lha7668_ctx);
        adc_init_count++;
        if ((whoamI == LHA7668B_8)) {
            break;
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
