/*!
 * @file        can_cmd.c
 *
 * @brief       CAN 命令通道：0x201 收命令，0x101/0x102/0x103 出六维力
 *
 * @note        命令码与 RS485 完全一致，处理逻辑复用 cmd_dispatch()，
 *              这里只负责 CAN 自己的组帧/拆帧。
 */

#include "can_cmd.h"
#include "Board.h"
#include "rs485_cmd.h"
#include "sensor_types.h"
#include <string.h>

extern volatile uint32_t g_tick_ms;

volatile uint16_t g_can_id_cmd = 0x201;   /* 0x201，收命令 + 发应答 */

/* ================================================================
 *  接收队列：中断只做入队，命令分发放在主循环
 *
 *  环形缓冲，CAN_RX_QUEUE_NUM 必须是 2 的幂（用掩码回绕）。
 *  旧实现先自增 s_rx_head 再写 s_rxq[s_rx_head]，而 s_rx_head 是 uint8_t
 *  自增到 255 才回绕 —— 从第 64 帧开始就一直写到数组外面（越界约 1.5KB），
 *  会踩坏后面的全局变量；主循环里 s_rx_tail = s_rx_head 还会把中间的
 *  帧全部跳过。改成标准环形缓冲：先判满，写当前槽位，再回绕推进。
 * ================================================================ */
#define CAN_RX_QUEUE_NUM    64              /* 必须是 2 的幂 */
#define CAN_RX_QUEUE_MASK   (CAN_RX_QUEUE_NUM - 1U)

static uint8_t           s_rxq[CAN_RX_QUEUE_NUM][CAN_DATA_LENGTH];
static volatile uint8_t  s_rx_head;   /* 中断写 */
static uint8_t           s_rx_tail;   /* 主循环读 */

void can_rx_frame_isr(const uint8_t *data, uint8_t len)
{
    uint8_t next = (uint8_t)((s_rx_head + 1U) & CAN_RX_QUEUE_MASK);

    (void)len;                              /* 固件永远收 8 字节帧 */
    if (next == s_rx_tail) {
        return;                             /* 队列满：丢这帧，绝不能覆盖未读数据 */
    }
    memcpy(s_rxq[s_rx_head], data, CAN_DATA_LENGTH);
    s_rx_head = next;
}

/* ================================================================
 *  发送
 * ================================================================ */
static void can_send(uint32_t std_id, const uint8_t *data)
{
    CAN_TxMessage_T msg;

    msg.stdID          = std_id;
    msg.extID          = 0;
    msg.typeID         = CAN_TYPEID_STD;
    msg.remoteTxReq    = CAN_RTXR_DATA;
    msg.dataLengthCode = CAN_DATA_LENGTH;
    memcpy(msg.data, data, CAN_DATA_LENGTH);

    CAN_TxMessage(CAN1, &msg);
}

/* 发出任意标准帧（固定 8 字节），用于上电通知等自定义报文 */
void can_cmd_send_frame(uint32_t std_id, const uint8_t *data)
{
    CAN_TxMessage_T msg;

    msg.stdID          = std_id;
    msg.extID          = 0;
    msg.typeID         = CAN_TYPEID_STD;
    msg.remoteTxReq    = CAN_RTXR_DATA;
    msg.dataLengthCode = CAN_DATA_LENGTH;
    memcpy(msg.data, data, CAN_DATA_LENGTH);

    (void)CAN_TxMessage(CAN1, &msg);
}

/* 把一段字节流按 8 字节一帧发出，数据靠前（左对齐）、尾部补 0 */
static void can_stream_send(const uint8_t *s, uint16_t n)
{
    uint8_t frame[CAN_DATA_LENGTH];

    while (n > 0U) {
        uint16_t chunk = (n > CAN_DATA_LENGTH) ? CAN_DATA_LENGTH : n;

        memset(frame, 0, sizeof(frame));
        memcpy(frame, s, chunk);
        can_send(g_can_id_cmd, frame);

        s += chunk;
        n = (uint16_t)(n - chunk);
    }
}

/* 应答帧：[CMD|0x80][DATA...] 数据靠前（左对齐）、尾部补 0
   例：查询版本号 -> 86 01 07 01 00 00 00 00
   超过 8 字节的应答拆成多帧连续发送（后续帧同样左对齐）。 */
static void can_respond(uint8_t cmd, const uint8_t *data, uint16_t len)
{
    uint8_t frame[CAN_DATA_LENGTH];

    memset(frame, 0, sizeof(frame));
    frame[0] = (uint8_t)(cmd | RS485_RESP_FLAG);

    if (len == 0U) {
        can_send(g_can_id_cmd, frame);
        return;
    }

    if (len <= (CAN_DATA_LENGTH - 1U)) {
        memcpy(frame + 1, data, len);
        can_send(g_can_id_cmd, frame);
    } else {
        /* 超过一帧：首帧命令码 + 7 字节数据，剩余部分继续流式发送 */
        memcpy(frame + 1, data, CAN_DATA_LENGTH - 1);
        can_send(g_can_id_cmd, frame);
        can_stream_send(data + (CAN_DATA_LENGTH - 1),
                        (uint16_t)(len - (CAN_DATA_LENGTH - 1)));
    }
}

static void float_to_le(uint8_t *dst, float f)
{
    uint32_t u; memcpy(&u, &f, 4);
    dst[0] = (uint8_t)(u);
    dst[1] = (uint8_t)(u >> 8);
    dst[2] = (uint8_t)(u >> 16);
    dst[3] = (uint8_t)(u >> 24);
}

/* 六维力输出：0x101 = Fx Fy，0x102 = Fz Tx，0x103 = Ty Tz */
static const uint32_t canForceId[3] = {
    CAN_ID_TX_FORCE,     /* 0x101: force[0] force[1] */
    CAN_ID_TX_MOMENT,    /* 0x102: force[2] force[3] */
    CAN_ID_TX_FYFZ       /* 0x103: force[4] force[5] */
};

static void can_send_force(uint8_t cmd)
{
    uint8_t i, j;
    uint8_t frame[CAN_DATA_LENGTH];

    (void)cmd;

    for (i = 0; i < 3; i++) {
        for (j = 0; j < 2; j++) {
            float f = g_sensor.force[i * 2 + j];   /* volatile，只读一次 */
            float_to_le(frame + j * 4, f);
        }
        can_send(canForceId[i], frame);
    }
}

/* ================================================================
 *  命令分发
 * ================================================================ */
static const cmd_transport_t g_can_transport = {
    .respond         = can_respond,
    .send_force      = can_send_force,
    .send_text       = can_stream_send,
    .continuous_mode = 2,             /* g_sys.send_mode = 2 -> CAN 连续发送 */
};

void can_cmd_process(void)
{
    while (s_rx_tail != s_rx_head) {
        uint8_t frame[CAN_DATA_LENGTH];
        uint8_t cmd;

        memcpy(frame, (const void *)s_rxq[s_rx_tail], CAN_DATA_LENGTH);
        s_rx_tail = (uint8_t)((s_rx_tail + 1U) & CAN_RX_QUEUE_MASK);

        cmd = frame[0];                            /* 命令码在第一个字节 */

        /* 矩阵功能 CAN 侧暂不实现 */
        if (cmd == RS485_CMD_SET_MATRIX || cmd == RS485_CMD_QUERY_MATRIX) {
            continue;
        }

        /* 参数从 data[1] 开始，高字节在前（大端），例如：
           09 00 02 00 00 00 00 00 -> 0x09 修改 CAN ID，参数 00 02 -> ID = 0x0002
           02 00 00 00 00 00 00 00 -> 0x02 启动连续发送（无参数）
           04 00 01 00 00 00 00 00 -> 0x04 改波特率，参数 01 -> 460800 */
        cmd_dispatch(&g_can_transport, cmd, &frame[1], CAN_DATA_LENGTH - 1);
    }
}

void can_cmd_send_continuous(void)
{
    static uint32_t s_last_send_ms = 0;
    uint32_t interval = (g_sys.freq_mode == 0) ? 2U : 1U;   /* 500Hz or 1kHz */

    if ((uint32_t)(g_tick_ms - s_last_send_ms) >= interval) {
        s_last_send_ms = g_tick_ms;
        can_send_force(RS485_CMD_START);
    }
}
