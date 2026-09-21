/**
 * @file        at24c04.h
 * @brief       AT24C04 I2C EEPROM 驱动模块（I2C2）
 *
 * @note        AT24C04 概要：
 *              - 容量：4Kbit = 512 字节（2 个 256 字节块）
 *              - 接口：I2C，最大 400kHz（Fast Mode）
 *              - 页大小：16 字节（单次页写入最多 16 字节）
 *              - 写入周期：最大 5ms（自定时，期间芯片不响应 I2C）
 *
 *              I2C 设备地址（7 位）：
 *              AT24C04 的设备地址由以下部分组成：
 *              +---+---+---+---+---+---+---+---+
 *              | 1 | 0 | 1 | 0 | A2| P0|R/W|
 *              +---+---+---+---+---+---+---+---+
 *              - 高 4 位固定为 1010（EEPROM 族识别码）
 *              - A2：由芯片的 A2 引脚决定（硬件接地 = 0）
 *              - P0：内部地址第 9 位（A8），用于选择 256 字节块
 *              - R/W：0=写，1=读
 *
 *              寻址方式：
 *              - 地址 0x000 ~ 0x0FF (块 0)：设备地址 = 0xA0 (写) / 0xA1 (读)
 *              - 地址 0x100 ~ 0x1FF (块 1)：设备地址 = 0xA2 (写) / 0xA3 (读)
 *              块选择通过将 A8 嵌入设备地址实现（而非独立的第二个地址字节）
 *
 *              硬件连接（I2C2）：
 *              - PB10 → SCL  (I2C2 时钟)
 *              - PB11 → SDA  (I2C2 数据)
 *
 *              I2C2 工作在 100kHz 标准模式下（也可配置为 400kHz Fast Mode）。
 *              本驱动使用阻塞式（轮询）方式进行所有 I2C 操作，适合裸机应用。
 */

#ifndef __BSP_I2C_EEPROM_H
#define __BSP_I2C_EEPROM_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes */
#include "apm32f10x.h"
#include "apm32f10x_gpio.h"
#include "apm32f10x_i2c.h"
#include "apm32f10x_rcm.h"
#include <stdint.h>

/*===========================================================================
 * 宏定义 - I2C1 / AT24C04 配置
 *===========================================================================*/

/** I2C1 时钟频率：100kHz（标准模式） */
#define EEPROM_I2C_SPEED        100000

/** I2C1 外设 */
#define EEPROM_I2C              I2C2

/** AT24C04 设备基础地址（7 位地址左移 1 位后的 8 位值，不含 P0 位） */
#define AT24C04_BASE_ADDR       0xA0

/** AT24C04 容量（字节） */
#define AT24C04_SIZE            512

/** AT24C04 页大小（一次页写入最多 16 字节） */
#define AT24C04_PAGE_SIZE       16

/** 写入周期超时（ms），AT24C04 自定时写入最长 5ms */
#define EEPROM_WRITE_TIMEOUT_MS 10

/* ---- I2C1 引脚 ---- */
#define EEPROM_SCL_GPIO_PORT    GPIOB
#define EEPROM_SCL_GPIO_CLK     RCM_APB2_PERIPH_GPIOB
#define EEPROM_SCL_PIN          GPIO_PIN_10

#define EEPROM_SDA_GPIO_PORT    GPIOB
#define EEPROM_SDA_GPIO_CLK     RCM_APB2_PERIPH_GPIOB
#define EEPROM_SDA_PIN          GPIO_PIN_11

#define EEPROM_I2C_CLK          RCM_APB1_PERIPH_I2C2

/*===========================================================================
 * 函数声明
 *===========================================================================*/

/**
 * @brief   向 AT24C04 写入一个字节
 * @param   addr  写入地址（0~511）
 * @param   data  写入的数据字节
 * @note    单个字节写入使用标准 I2C 字节写操作。
 *          写入后会自动等待芯片内部写入周期完成。
 *          如需连续写入多个字节，建议使用 BSP_EEPROM_WritePage()。
 */
void BSP_EEPROM_WriteByte(uint16_t addr, uint8_t data);

/**
 * @brief   从 AT24C04 读取一个字节
 * @param   addr  读取地址（0~511）
 * @return  读取到的数据字节（0x00~0xFF）
 * @note    使用 I2C 随机读（Random Read）操作：
 *          先发"虚拟写"（发送地址），再发读命令接收数据。
 */
uint8_t BSP_EEPROM_ReadByte(uint16_t addr);

/**
 * @brief   向 AT24C04 执行页写入（最多一页 16 字节）
 * @param   addr  起始写入地址（0~511）
 * @param   pData 数据缓冲区指针
 * @param   len   写入字节数（1~16，超过 16 将截断为 16）
 * @note    AT24C04 页写入自动处理跨页回绕：
 *          如果起始地址 + len 超过当前页边界，写入会在
 *          页边界处回绕到当前页的起始位置。
 *          如果需要跨页写入大量数据，请多次调用此函数。
 *          调用此函数比逐个字节调用 BSP_EEPROM_WriteByte() 效率更高。
 */
void BSP_EEPROM_WritePage(uint16_t addr, const uint8_t *pData, uint16_t len);

/**
 * @brief   从 AT24C04 连续读取多个字节
 * @param   addr  起始读取地址（0~511）
 * @param   pData 接收数据缓冲区指针
 * @param   len   读取字节数
 * @note    使用 I2C 连续读（Sequential Read）操作。
 *          可以跨越 256 字节块边界自动寻址。
 *          如果地址读取到末尾（511），会自动回绕到地址 0。
 */
void BSP_EEPROM_ReadBytes(uint16_t addr, uint8_t *pData, uint16_t len);

#ifdef __cplusplus
}
#endif

#endif /* __BSP_I2C_EEPROM_H */
