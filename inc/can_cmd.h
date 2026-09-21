#ifndef __CAN_CMD_H__
#define __CAN_CMD_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/* ================================================================
 *  CAN 命令通道
 *
 *  接收：CAN ID 0x201（可用 0x09 命令修改），固定 8 字节，
 *        **第一个字节 (data[0]) 为命令码**，参数从 data[1] 开始、
 *        高字节在前（大端），命令码与 RS485 完全一致。
 *        例：02 00 00 00 00 00 00 00  ->  启动连续发送
 *            06 00 00 00 00 00 00 00  ->  查询固件版本号
 *            04 00 01 00 00 00 00 00  ->  设波特率，代码 01 = 460800
 *            09 00 02 00 00 00 00 00  ->  修改 CAN ID，参数 00 02 -> 0x0002
 *
 *  应答：CAN ID 同上，8 字节，**[CMD|0x80] 在 data[0]，数据紧跟在后面**，
 *        尾部补 0（左对齐）。
 *        例：查询版本号 -> 86 01 07 01 00 00 00 00
 *        无数据的 ACK：86 00 00 00 00 00 00 00
 *        超过 8 字节的应答会拆成多帧发送（后续帧同样左对齐、尾部补 0）。
 *
 *  六维力数据：拆成 3 帧发送，每帧 2 个 float32（小端）
 *        0x101 -> Fx, Fy
 *        0x102 -> Fz, Tx
 *        0x103 -> Ty, Tz
 *
 *  暂不实现：0x20 烧录矩阵、0x37 查询矩阵。
 * ================================================================ */

/** 当前 CAN 命令通道 ID（收命令 + 发应答），默认 0x201，
 *  可通过 0x09 命令修改并保存到 EEPROM 0x98/0x99 */
extern volatile uint16_t g_can_id_cmd;

/** CAN 接收中断里调用：把收到的 0x201 帧放入待处理队列 */
void can_rx_frame_isr(const uint8_t *data, uint8_t len);

/** 主循环里调用：取出队列中的命令帧并分发 */
void can_cmd_process(void);

/** 主循环里调用：send_mode == 2 时按 500Hz/1kHz 连续发送六维力 */
void can_cmd_send_continuous(void);

/** 发送一帧自定义 8 字节标准帧（如上电完成通知），不做任何组帧处理 */
void can_cmd_send_frame(uint32_t std_id, const uint8_t *data);

#ifdef __cplusplus
}
#endif

#endif /* __CAN_CMD_H__ */
