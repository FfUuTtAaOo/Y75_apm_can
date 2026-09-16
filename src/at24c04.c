/**
 * @file        at24c04.c
 * @brief       AT24C04 I2C EEPROM 驱动实现
 *
 * @note        实现说明：
 *              1. 使用 Geehy APM32 标准外设驱动库的 I2C API
 *              2. 所有 I2C 操作均为阻塞式（轮询标志位）
 *              3. 使用 I2C_EVENT_T 枚举值检测 I2C 总线事件
 *              4. AT24C04 的地址分为两部分：
 *                 - 设备地址中的 P0 位（A8：第 9 个地址位）
 *                 - 内存地址字节（A7~A0：低 8 位）
 *
 *              I2C 操作流程说明：
 *
 *              【写操作 - 字节写】
 *              START → 设备地址(W) → ACK → 内存地址 → ACK → 数据 → ACK → STOP
 *
 *              【读操作 - 随机读】
 *              START → 设备地址(W) → ACK → 内存地址 → ACK →
 *              RESTART → 设备地址(R) → ACK → 读数据 → NACK → STOP
 *
 *              【写操作 - 页写】
 *              START → 设备地址(W) → ACK → 内存地址 → ACK →
 *              数据0 → ACK → 数据1 → ACK → ... → 数据N → ACK → STOP
 *
 *              AT24C04 I2C 地址格式：
 *              ┌───┬───┬───┬───┬───┬───┬───┬───┐
 *              │ 1 │ 0 │ 1 │ 0 │ A2│ P0│R/W│
 *              └───┴───┴───┴───┴───┴───┴───┴───┘
 *              对于 8 位 I2C 地址：0xA0 | (P0 << 1) | R/W
 *
 *              注意：本文件中使用的 I2C 地址是 7 位左对齐格式。
 *              实际发送时 I2C 硬件会将 R/W 位添加到最低位。
 */
#include "at24c04.h"
#include <stdio.h>
#include "apm32f10x_i2c.h"

/*===========================================================================
 * 静态辅助函数声明
 *===========================================================================*/

/**
 * @brief   根据 AT24C04 内存地址生成对应的 I2C 设备地址
 * @param   addr  内存地址（0~511）
 * @param   isRead  0=写操作，1=读操作
 * @return  7 位设备地址（左对齐，不含 R/W 位）
 *
 * @note    AT24C04 将 A8（第 9 位内存地址）嵌入 I2C 设备地址的 P0 位。
 *          地址范围 → 设备地址：
 *            0x000-0x0FF (A8=0) → 0xA0 | 写/读
 *            0x100-0x1FF (A8=1) → 0xA2 | 写/读
 */
static uint8_t GetDeviceAddr(uint16_t addr, uint8_t isRead)
{
    uint8_t devAddr = AT24C04_BASE_ADDR;

    /* 如果内存地址在高 256 字节块，设置 P0 位 */
    if (addr & 0x0100)
    {
        devAddr |= 0x02;  /* P0 = 1，对应设备地址 bit1 */
    }

    /* AT24C04 使用 7 位地址，I2C_Tx7BitAddress 需要左对齐的地址值
       这里返回 7 位地址左移 1 位后的值（标准 I2C 库格式） */
    return devAddr;
}

/**
 * @brief   等待 AT24C04 内部写入周期完成
 * @param   addr  正在写入的内存地址（用于确定设备地址块）
 * @return  0 = 完成，-1 = 超时
 *
 * @note    AT24C04 在内部写入周期中不会响应 I2C 总线（ACK 轮询）。
 *          通过连续发送 START + 设备地址，直到收到 ACK 即表示写入完成。
 *          最大写入周期 ≤ 5ms，超时设为 10ms。
 */
static int8_t WaitWriteComplete(uint16_t addr)
{
    uint32_t timeout = EEPROM_WRITE_TIMEOUT_MS;
    uint8_t  devAddr = GetDeviceAddr(addr, 0);

    while (timeout--)
    {
        /* 发送 START 条件 */
        I2C_EnableGenerateStart(EEPROM_I2C);

        /* 等待 START 条件发送完成 */
        while (I2C_ReadStatusFlag(EEPROM_I2C, I2C_FLAG_START) == 0)
        {
            /* 等待... */
        }

        /* 发送设备地址（写模式） */
        I2C_Tx7BitAddress(EEPROM_I2C, devAddr, I2C_DIRECTION_TX);

        /* 轮询等待地址发送完成或超时 1ms */
        volatile uint32_t delay = 1000;
        while (delay)
        {
            /* 如果收到地址 ACK，说明写入完成 */
            if (I2C_ReadEventStatus(EEPROM_I2C, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED) == SUCCESS)
            {
                I2C_EnableGenerateStop(EEPROM_I2C);
                return 0;   /* 写入完成 */
            }

            /* 检测 NACK 或其他错误 */
            if (I2C_ReadStatusFlag(EEPROM_I2C, I2C_FLAG_AE))
            {
                I2C_ClearStatusFlag(EEPROM_I2C, I2C_FLAG_AE);
                break;  /* 该次轮询失败，继续重试 */
            }
            delay--;
        }

        I2C_EnableGenerateStop(EEPROM_I2C);

        /* 短暂延时后重试（约 1ms 的软件延时） */
        volatile uint32_t wait = SystemCoreClock / 3000;
        while (wait--)
        {
            /* 空循环 */
        }
    }

    return -1;  /* 超时 */
}

/*===========================================================================
 * BSP_EEPROM_WriteByte - 写入单字节
 *===========================================================================*/

/**
 * @brief   向 AT24C04 指定地址写入一个字节
 * @param   addr  内存地址（0~511）
 * @param   data  要写入的数据
 *
 * @note    I2C 字节写操作序列（Byte Write）：
 *          ┌───┐    ┌──────────┬─────┬──────────┬─────┬─────┬─────┬────┐
 *          │ S │    │DevAddr+W │ ACK │MemAddr   │ ACK │Data │ ACK │ P  │
 *          └───┘    └──────────┴─────┴──────────┴─────┴─────┴─────┴────┘
 *
 *          写入后芯片进入自定时写入周期（最长 5ms），
 *          期间不响应 I2C 总线。本函数会通过 ACK 轮询等待写入完成。
 */
void BSP_EEPROM_WriteByte(uint16_t addr, uint8_t data)
{
    uint8_t devAddr = GetDeviceAddr(addr, 0);       /* 设备地址（写） */
    uint8_t memAddr = (uint8_t)(addr & 0xFF);        /* 内存地址低 8 位 */

    /*---- 1. 等待总线空闲 ----*/
    while (I2C_ReadStatusFlag(EEPROM_I2C, I2C_FLAG_BUSBSY))
    {
        /* I2C 总线忙，等待释放 */
    }

    /*---- 2. 发送 START 条件 ----*/
    I2C_EnableGenerateStart(EEPROM_I2C);
    while (I2C_ReadStatusFlag(EEPROM_I2C, I2C_FLAG_START) == 0)
    {
        /* 等待 START 条件发送完成（主模式已激活） */
    }

    /*---- 3. 发送设备地址（写模式） ----*/
    I2C_Tx7BitAddress(EEPROM_I2C, devAddr, I2C_DIRECTION_TX);
    while (I2C_ReadEventStatus(EEPROM_I2C, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED) == 0)
    {
        /* 等待设备地址 ACK */
    }

    /*---- 4. 发送内存地址 ----*/
    I2C_TxData(EEPROM_I2C, memAddr);
    while (I2C_ReadEventStatus(EEPROM_I2C, I2C_EVENT_MASTER_BYTE_TRANSMITTED) == 0)
    {
        /* 等待内存地址发送完成并收到 ACK */
    }

    /*---- 5. 发送数据字节 ----*/
    I2C_TxData(EEPROM_I2C, data);
    while (I2C_ReadEventStatus(EEPROM_I2C, I2C_EVENT_MASTER_BYTE_TRANSMITTED) == 0)
    {
        /* 等待数据发送完成并收到 ACK */
    }

    /*---- 6. 发送 STOP 条件 ----*/
    I2C_EnableGenerateStop(EEPROM_I2C);

    /*---- 7. 等待内部写入周期完成（ACK 轮询） ----*/
    WaitWriteComplete(addr);
}

/*===========================================================================
 * BSP_EEPROM_ReadByte - 读取单字节
 *===========================================================================*/

/**
 * @brief   从 AT24C04 指定地址读取一个字节
 * @param   addr  内存地址（0~511）
 * @return  读取的数据（0x00~0xFF）
 *
 * @note    I2C 随机读操作序列（Random Read）：
 *          ┌───┐    ┌──────────┬─────┬──────────┬─────┐
 *          │ S │    │DevAddr+W │ ACK │MemAddr   │ ACK │  (虚拟写)
 *          └───┘    └──────────┴─────┴──────────┴─────┘
 *          ┌──────┐ ┌──────────┬─────┬──────┬──────┬──────┐
 *          │  RS  │ │DevAddr+R │ ACK │ Data │ NACK │  P   │  (读)
 *          └──────┘ └──────────┴─────┴──────┴──────┴──────┘
 *
 *          先做一次"虚拟写"操作（写设备地址 + 内存地址，不写数据），
 *          然后发送 RESTART + 设备地址（读），接收数据后主机发 NACK 结束。
 */
uint8_t BSP_EEPROM_ReadByte(uint16_t addr)
{
    uint8_t devAddr = GetDeviceAddr(addr, 0);        /* 设备地址（写模式，用于虚拟写） */
    uint8_t memAddr = (uint8_t)(addr & 0xFF);         /* 内存地址低 8 位 */
    uint8_t data;

    /*---- 1. 等待总线空闲 ----*/
    while (I2C_ReadStatusFlag(EEPROM_I2C, I2C_FLAG_BUSBSY))
    {
        /* 等待 */
    }

    /*---- 2. 发送 START + 设备地址(W) + 内存地址（虚拟写） ----*/
    I2C_EnableGenerateStart(EEPROM_I2C);
    while (I2C_ReadStatusFlag(EEPROM_I2C, I2C_FLAG_START) == 0)
    {
        /* 等待 START */
    }

    I2C_Tx7BitAddress(EEPROM_I2C, devAddr, I2C_DIRECTION_TX);
    while (I2C_ReadEventStatus(EEPROM_I2C, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED) == 0)
    {
        /* 等待设备地址 ACK */
    }

    I2C_TxData(EEPROM_I2C, memAddr);
    while (I2C_ReadEventStatus(EEPROM_I2C, I2C_EVENT_MASTER_BYTE_TRANSMITTED) == 0)
    {
        /* 等待内存地址 ACK */
    }

    /*---- 3. 发送 RESTART + 设备地址(R) ----*/
    I2C_EnableGenerateStart(EEPROM_I2C);   /* 重复 START */
    while (I2C_ReadStatusFlag(EEPROM_I2C, I2C_FLAG_START) == 0)
    {
        /* 等待 RESTART */
    }

    /* 发送设备地址（读模式） */
    I2C_Tx7BitAddress(EEPROM_I2C, devAddr, I2C_DIRECTION_RX);
    while (I2C_ReadEventStatus(EEPROM_I2C, I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED) == 0)
    {
        /* 等待设备地址 ACK（读模式） */
    }

    /*---- 4. 读取数据（单字节，主机发送 NACK） ----*/
    /* 禁止 ACK → 主机收到数据后发送 NACK（通知从机停止发送） */
    I2C_DisableAcknowledge(EEPROM_I2C);

    /* 等待接收缓冲区非空 */
    while (I2C_ReadEventStatus(EEPROM_I2C, I2C_EVENT_MASTER_BYTE_RECEIVED) == 0)
    {
        /* 等待数据接收 */
    }
    data = I2C_RxData(EEPROM_I2C);

    /*---- 5. 发送 STOP ----*/
    I2C_EnableGenerateStop(EEPROM_I2C);

    /* 重新使能 ACK（为下次操作准备） */
    I2C_EnableAcknowledge(EEPROM_I2C);

    return data;
}

/*===========================================================================
 * BSP_EEPROM_WritePage - 页写入
 *===========================================================================*/

/**
 * @brief   向 AT24C04 执行页写入（最多 16 字节）
 * @param   addr   起始地址（0~511）
 * @param   pData  数据缓冲区
 * @param   len    写入字节数（超过 16 字节将截断）
 *
 * @note    页写入操作序列：
 *          START → DevAddr(W) → MemAddr → Data0 → Data1 → ... → DataN → STOP
 *
 *          页边界问题：
 *          AT24C04 的页大小为 16 字节，页地址边界为 16 的整数倍。
 *          如果写入超过当前页的剩余空间，数据会从当前页起始地址
 *          回绕覆盖已写入的数据，而不是跨到下一页。
 *
 *          调用者应确保参数合理性：
 *          - 不跨越页边界的写入交给此函数
 *          - 跨页写入需要分多次调用此函数
 *
 *          页写入比逐字节写入（BSP_EEPROM_WriteByte）效率更高：
 *          - 逐字节写：N × (5ms 写入周期) = 耗时很长
 *          - 页写：只需 1 次 5ms 写入周期
 */
void BSP_EEPROM_WritePage(uint16_t addr, const uint8_t *pData, uint16_t len)
{
    uint8_t  devAddr;
    uint8_t  memAddr;
    uint16_t i;

    /* 参数检查 */
    if (len == 0 || pData == NULL)
    {
        return;
    }

    /* 限制单页写入长度 */
    if (len > AT24C04_PAGE_SIZE)
    {
        len = AT24C04_PAGE_SIZE;
    }

    devAddr = GetDeviceAddr(addr, 0);
    memAddr = (uint8_t)(addr & 0xFF);

    /*---- 1. 等待总线空闲 ----*/
    while (I2C_ReadStatusFlag(EEPROM_I2C, I2C_FLAG_BUSBSY))
    {
        /* 等待 */
    }

    /*---- 2. 发送 START + 设备地址(W) + 内存地址 ----*/
    I2C_EnableGenerateStart(EEPROM_I2C);
    while (I2C_ReadStatusFlag(EEPROM_I2C, I2C_FLAG_START) == 0)
    {
        /* 等待 START */
    }

    I2C_Tx7BitAddress(EEPROM_I2C, devAddr, I2C_DIRECTION_TX);
    while (I2C_ReadEventStatus(EEPROM_I2C, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED) == 0)
    {
        /* 等待 ACK */
    }

    I2C_TxData(EEPROM_I2C, memAddr);
    while (I2C_ReadEventStatus(EEPROM_I2C, I2C_EVENT_MASTER_BYTE_TRANSMITTED) == 0)
    {
        /* 等待 ACK */
    }

    /*---- 3. 连续发送数据字节 ----*/
    for (i = 0; i < len; i++)
    {
        I2C_TxData(EEPROM_I2C, pData[i]);
        while (I2C_ReadEventStatus(EEPROM_I2C, I2C_EVENT_MASTER_BYTE_TRANSMITTED) == 0)
        {
            /* 等待每个字节的 ACK */
        }
    }

    /*---- 4. 发送 STOP ----*/
    I2C_EnableGenerateStop(EEPROM_I2C);

    /*---- 5. 等待写入周期完成 ----*/
    WaitWriteComplete(addr);
}

/*===========================================================================
 * BSP_EEPROM_ReadBytes - 连续读多字节
 *===========================================================================*/

/**
 * @brief   从 AT24C04 连续读取多个字节
 * @param   addr   起始地址（0~511）
 * @param   pData  接收缓冲区
 * @param   len    读取字节数
 *
 * @note    连续读（Sequential Read）操作：
 *          先通过虚拟写设置内部地址计数器，然后连续读取。
 *          AT24C04 内部地址计数器自动递增。
 *          读取到地址 511 后自动回绕到 0。
 *
 *          主机在每个字节后发送 ACK（而非 NACK），
 *          以告知从机继续发送下一个字节。
 *          最后一个字节后发送 NACK + STOP 结束传输。
 */
void BSP_EEPROM_ReadBytes(uint16_t addr, uint8_t *pData, uint16_t len)
{
    uint8_t  devAddr;
    uint8_t  memAddr;
    uint16_t i;

    if (len == 0 || pData == NULL)
    {
        return;
    }

    devAddr = GetDeviceAddr(addr, 0);
    memAddr = (uint8_t)(addr & 0xFF);

    /*---- 1. 等待总线空闲 ----*/
    while (I2C_ReadStatusFlag(EEPROM_I2C, I2C_FLAG_BUSBSY))
    {
        /* 等待 */
    }

    /*---- 2. 虚拟写：START + DevAddr(W) + MemAddr ----*/
    I2C_EnableGenerateStart(EEPROM_I2C);
    while (I2C_ReadStatusFlag(EEPROM_I2C, I2C_FLAG_START) == 0)
    {
        /* 等待 START */
    }

    I2C_Tx7BitAddress(EEPROM_I2C, devAddr, I2C_DIRECTION_TX);
    while (I2C_ReadEventStatus(EEPROM_I2C, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED) == 0)
    {
        /* 等待 ACK */
    }

    I2C_TxData(EEPROM_I2C, memAddr);
    while (I2C_ReadEventStatus(EEPROM_I2C, I2C_EVENT_MASTER_BYTE_TRANSMITTED) == 0)
    {
        /* 等待 ACK */
    }

    /*---- 3. RESTART + DevAddr(R)，开始接收数据 ----*/
    I2C_EnableGenerateStart(EEPROM_I2C);
    while (I2C_ReadStatusFlag(EEPROM_I2C, I2C_FLAG_START) == 0)
    {
        /* 等待 RESTART */
    }

    I2C_Tx7BitAddress(EEPROM_I2C, devAddr, I2C_DIRECTION_RX);
    while (I2C_ReadEventStatus(EEPROM_I2C, I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED) == 0)
    {
        /* 等待 ACK */
    }

    /*---- 4. 连续读取数据 ----*/
    for (i = 0; i < len; i++)
    {
        /* 如果是最后一个字节，发送 NACK */
        if (i == len - 1)
        {
            I2C_DisableAcknowledge(EEPROM_I2C);
        }

        /* 等待数据接收 */
        while (I2C_ReadEventStatus(EEPROM_I2C, I2C_EVENT_MASTER_BYTE_RECEIVED) == 0)
        {
            /* 等待 */
        }
        pData[i] = I2C_RxData(EEPROM_I2C);
    }

    /*---- 5. STOP ----*/
    I2C_EnableGenerateStop(EEPROM_I2C);

    /* 重新使能 ACK */
    I2C_EnableAcknowledge(EEPROM_I2C);
}
