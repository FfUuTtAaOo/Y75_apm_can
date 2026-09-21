#include "rs485_cmd.h"
#include "rs485.h"
#include "sensor_types.h"
#include "calibration.h"
#include "flash_storage.h"
#include "filter.h"
#include <string.h>
#include "at24c04.h"
#include "can_cmd.h"

extern uint8_t ether_flag;

/* ================================================================
 *  Internal helpers
 * ================================================================ */

static void float_to_le(uint8_t *dst, float f)
{
    uint32_t u; memcpy(&u, &f, 4);
    dst[0] = (uint8_t)(u);
    dst[1] = (uint8_t)(u >> 8);
    dst[2] = (uint8_t)(u >> 16);
    dst[3] = (uint8_t)(u >> 24);
}

static float le_to_float(const uint8_t *src)
{
    uint32_t u = ((uint32_t)src[3] << 24)
               | ((uint32_t)src[2] << 16)
               | ((uint32_t)src[1] << 8)
               | ((uint32_t)src[0]);
    float f; memcpy(&f, &u, 4); return f;
}

/* Send a response frame: AA 55 [cmd|0x80] [data...] 0D 0A */
static void respond(uint8_t cmd, const uint8_t *data, uint16_t len)
{
    rs485_send_frame((uint8_t)(cmd | RS485_RESP_FLAG), data, len);
}

/* Map a baud-rate code (1..5) to the actual baud rate.
 * Returns 0 when the code is not one of the supported values. */
static uint32_t baud_from_code(uint8_t code)
{
    switch (code) {
    case RS485_BAUD_460800: return 460800U;
    case RS485_BAUD_256000: return 256000U;
    case RS485_BAUD_115200: return 115200U;
    case RS485_BAUD_19200:  return 19200U;
    case RS485_BAUD_9600:   return 9600U;
    default:                return 0U;
    }
}

/* Minimal float → ASCII: "-ddd.dddddd" (6 decimals), NUL-terminated.
   Written by hand because the firmware links with newlib-nano without
   -u _printf_float, so snprintf("%f") cannot print floats at all. */
static int fmt_float(char *dst, int max, float v)
{
    static const uint32_t pow10[6] = { 1UL, 10UL, 100UL, 1000UL, 10000UL, 100000UL };
    uint32_t ip, fp;
    char tmp[12];
    int n = 0, k = 0, i;

    if (max < 2) return 0;

    if (v != v) {                                  /* NaN guard */
        static const char nan_txt[] = "NaN";
        for (i = 0; nan_txt[i] != '\0' && n < max - 1; i++) {
            dst[n++] = nan_txt[i];
        }
        dst[n] = '\0';
        return n;
    }

    if (v < 0.0f) {
        if (n < max - 1) dst[n++] = '-';
        v = -v;
    }

    ip = (uint32_t)v;
    fp = (uint32_t)((v - (float)ip) * 1000000.0f + 0.5f);
    if (fp >= 1000000UL) { ip += 1UL; fp = 0UL; }  /* rounding carry */

    do {                                           /* integer part */
        tmp[k++] = (char)('0' + (ip % 10UL));
        ip /= 10UL;
    } while (ip != 0UL && k < (int)sizeof(tmp));
    while (k > 0 && n < max - 1) {
        dst[n++] = tmp[--k];
    }

    if (n < max - 1) dst[n++] = '.';

    for (i = 5; i >= 0; i--) {                     /* 6 decimals, zero-padded */
        if (n < max - 1) {
            dst[n++] = (char)('0' + ((fp / pow10[i]) % 10UL));
        }
    }

    dst[n] = '\0';
    return n;
}

/* Persist the data output unit (1 = kg, 2 = N) at EE_ADDR_DATA_FORMAT (0x7C)
   so it survives a power cycle.  Only 0x33 and 0x35 store a value; on boot,
   any other content in that byte (e.g. blank 0xFF) makes the firmware fall
   back to 2 (N). */
static void data_format_save(uint8_t fmt)
{
    BSP_EEPROM_WriteByte(EE_ADDR_DATA_FORMAT, fmt);
}

/* Persist the CAN command-channel ID at EE_ADDR_CAN_ID (0x98/0x99),
   big-endian: 0x98 = high byte, 0x99 = low byte. */
static void can_id_save(uint16_t id)
{
    BSP_EEPROM_WriteByte(EE_ADDR_CAN_ID,      (uint8_t)(id >> 8));   /* 高字节 */
    BSP_EEPROM_WriteByte(EE_ADDR_CAN_ID + 1,  (uint8_t)(id));        /* 低字节 */
}

void eeprom_config_load(void)
{
    uint8_t  fmt;
    uint8_t  ee[2];
    uint16_t id;

    /* Data unit: only 1 (kg) and 2 (N) are valid; blank/other -> 2 (N) */
    fmt = BSP_EEPROM_ReadByte(EE_ADDR_DATA_FORMAT);
    if (fmt == 1U || fmt == 2U) {
        g_sys.data_format = fmt;
    } else {
        g_sys.data_format = 2U;
    }

    /* CAN ID: valid range 1..0x7FF (11-bit standard ID); blank/other -> default.
       大端存储：ee[0] = 高字节，ee[1] = 低字节 */
    BSP_EEPROM_ReadBytes(EE_ADDR_CAN_ID, ee, 2);
    id = (uint16_t)(((uint16_t)ee[0] << 8) | (uint16_t)ee[1]);
    if (id >= 1U && id <= 0x7FFU) {
        g_can_id_cmd = id;
    }
}

/* Apply a new baud rate: save it to EEPROM, then re-init USART1 and restart
 * DMA reception.  Must be called AFTER the ACK has been transmitted so the
 * host can still read the response at the old baud rate. */
static void apply_baud(uint32_t baud)
{
    // uint8_t  ee[4];
    // uint16_t i;

    // uart1_bsp = baud;

    // /* Persist (little-endian), same layout as the legacy GM_MOD_BAUD command */
    // for (i = 0; i < 4; i++) {
    //     ee[i] = (uint8_t)(baud >> (i * 8));
    // }
    // at24c04_write(UART_BAUD_ADDR, ee, 4);

    // /* Wait until the last ACK byte has completely shifted out */
    // while (__HAL_UART_GET_FLAG(&huart1, UART_FLAG_TC) == RESET) {}
    // __HAL_UART_CLEAR_FLAG(&huart1, UART_FLAG_TC);

    // /* Stop DMA reception, then re-configure the UART at the new baud rate */
    // if (huart1.hdmarx) {
    //     HAL_UART_DMAStop(&huart1);
    // }
    // HAL_UART_DeInit(&huart1);

    // huart1.Instance          = USART1;
    // huart1.Init.BaudRate     = baud;
    // huart1.Init.WordLength   = UART_WORDLENGTH_8B;
    // huart1.Init.StopBits     = UART_STOPBITS_1;
    // huart1.Init.Parity       = UART_PARITY_NONE;
    // huart1.Init.Mode         = UART_MODE_TX_RX;
    // huart1.Init.HwFlowCtl    = UART_HWCONTROL_NONE;
    // huart1.Init.OverSampling = UART_OVERSAMPLING_16;
    // HAL_UART_Init(&huart1);

    // /* Re-enable IDLE-line detection and restart DMA reception */
    // __HAL_UART_ENABLE_IT(&huart1, UART_IT_IDLE);
    // rs485_rx_restart();
}

/* ================================================================
 *  rs485_send_data_frame
 * ================================================================ */
void rs485_send_data_frame(uint8_t cmd)
{
    uint8_t d[28];
    uint8_t i;

    /* debug_mode: 0 = framed output, 1 = raw floats + 00 00 80 7F,
       anything else = silent, nothing is sent at all. */
    if (g_sys.debug_mode != 0U && g_sys.debug_mode != 1U) {
        return;
    }

    for (i = 0; i < 6; i++) {
        float f = g_sensor.force[i];        /* read volatile once */
        float_to_le(d + i * 4, f);
    }

    if (g_sys.debug_mode == 1) {
        /* Debug mode (only 1): [6 × float32 LE] 00 00 80 7F */
        d[24] = 0x00;
        d[25] = 0x00;
        d[26] = 0x80;
        d[27] = 0x7F;
        just_float_send_raw(d, 28);
    } else {
        /* debug_mode == 0 — standard HEX frame:
           AA 55 [cmd] [6 × float32 LE] 0D 0A
           0x02 for the continuous stream, 0x03 for a single-shot reply.
           Shift the payload right by one to make room for the CMD byte. */
        for (i = 24; i >= 1; i--) {
            d[i] = d[i - 1];
        }
        d[0] = cmd;
        rs485_send_raw(d, 1 + 24);
    }
}

/* ================================================================
 *  cmd_dispatch — 与传输方式无关的命令处理核心
 * ================================================================ */
static void tx_ack(const cmd_transport_t *tx, uint8_t cmd)
{
    tx->respond(cmd, NULL, 0);
}

void cmd_dispatch(const cmd_transport_t *tx, uint8_t cmd, const uint8_t *payload, uint16_t len)
{
    uint8_t buf[150];   /* enough for 144-byte matrix ack */

    switch (cmd) {

    /* ---- 0x01  Stop continuous send ---- */
    case RS485_CMD_STOP:
        g_sys.send_mode = 0;
        tx_ack(tx, cmd);
        break;

    /* ---- 0x02  Start continuous send ---- */
    case RS485_CMD_START:
        g_sys.send_mode = tx->continuous_mode;
        tx_ack(tx, cmd);
        break;

    /* ---- 0x03  Single-shot request ---- */
    case RS485_CMD_SINGLE:
        /* AA 55 03 [6×float32] 0D 0A (RS485)
           0x101 / 0x102 / 0x103 三帧 (CAN)
           — 内容与 0x02 连续流一致，只是命令字节不同，便于上位机区分。 */
        tx->send_force(RS485_CMD_SINGLE);
        break;

    /* ---- 0x04  Set baud rate: AA 55 04 [code] 0D 0A ---- */
    case RS485_CMD_SET_BAUD: {
        uint32_t baud;
        if (len < 1) { g_sys.comm_error_cnt++; break; }
        baud = baud_from_code(payload[0]);
        if (baud == 0U) { g_sys.comm_error_cnt++; break; }   /* invalid code */

        tx_ack(tx, cmd);               /* ACK first, at the OLD baud rate          */
        apply_baud(baud);       /* persist + re-init USART1 at the new rate */
        break;
    }

    /* ---- 0x08  Set frequency: AA 55 08 [0|1] 0D 0A ---- */
    case RS485_CMD_SET_FREQ:
        if (len < 1) { g_sys.comm_error_cnt++; break; }
        g_sys.freq_mode = (payload[0] == 0) ? 0U : 1U;
        tx_ack(tx, cmd);
        break;

    /* ---- 0x09  Set CAN ID: AA 55 09 [id_hi] [id_lo] 0D 0A ----
       参数为 2 字节 ID，**高字节在前**（大端），有效范围 1..0x7FF。
       CAN 帧：09 00 02 00 00 00 00 00 -> data[0]=命令码, data[1..2]=ID。
       应答先用旧 ID 发出，再写 EEPROM (0x98/0x99) 并切换到新 ID。 */
    case RS485_CMD_SET_CAN_ID: {
        uint16_t id;
        if (len < 2) { g_sys.comm_error_cnt++; break; }
        id = (uint16_t)(((uint16_t)payload[0] << 8) | (uint16_t)payload[1]);
        if (id < 1U || id > 0x7FFU) { g_sys.comm_error_cnt++; break; }

        tx_ack(tx, cmd);        /* ACK 仍以旧 ID 发出 */
        can_id_save(id);        /* 保存到 EEPROM，约 2 个 5ms 写入周期 */
        g_can_id_cmd = id;      /* 切换到新 ID（收命令 + 发应答同时生效） */
        break;
    }

    /* ---- 0x05  Query serial number ---- */
    case RS485_CMD_QUERY_SN:
        tx->send_text((const uint8_t *)g_config.sn, (uint16_t)strlen(g_config.sn));
        break;

    /* ---- 0x06  Query firmware version ---- */
    case RS485_CMD_QUERY_VER:
        buf[0] = (uint8_t)(g_config.fw_version >> 16);
        buf[1] = (uint8_t)(g_config.fw_version >> 8);
        buf[2] = (uint8_t)(g_config.fw_version);
        tx->respond(cmd, buf, 3);
        break;

    /* ---- 0x07  Query system status ---- */
    case RS485_CMD_QUERY_STATUS:
        buf[0] = g_sys.send_mode;
        buf[1] = g_sys.data_format;
        buf[2] = g_sys.freq_mode;
        buf[3] = g_sys.debug_mode;
        buf[4] = (uint8_t)(g_sys.status_flags);
        buf[5] = (uint8_t)(g_sys.status_flags >> 8);
        buf[6] = (uint8_t)(g_sys.comm_error_cnt);
        buf[7] = (uint8_t)(g_sys.comm_error_cnt >> 8);
        tx->respond(cmd, buf, 8);
        break;

    /* ---- 0x30  Zero calibration (20-point force average) ---- */
    case RS485_CMD_ZERO:
        calib_zero_start();
        g_sys.zero_calib_busy = 1;
        tx_ack(tx, cmd);
        break;

    case RS485_CMD_HEX_SEND:
        g_sys.debug_mode = 0;
        tx_ack(tx, cmd);
        break;

    case RS485_CMD_ASC_SEND:
        g_sys.debug_mode = 1;
        tx_ack(tx, cmd);
        break;

    /* ---- 0x36  Cancel zero ---- */
    case RS485_CMD_UNZERO:
        calib_zero_cancel();
        // flash_save_zero();
        tx_ack(tx, cmd);
        break;


    /* ---- 0x37  Query 6×6 decoupling matrix, plain ASCII text ----
       One line per row, 6 lines total, no binary frame header:
           matrix1: 0.298418, -0.211604, ...\r\n
       Row order matches the EEPROM layout and the per-row upload of 0x20. */
    case RS485_CMD_QUERY_MATRIX: {
        static const char pfx[] = "matrix";
        char line[112];

        for (int r = 0; r < 6; r++) {
            int n = 0;
            int cap = (int)sizeof(line);

            /* "matrix1: " … "matrix6: " */
            memcpy(line, pfx, sizeof(pfx) - 1);
            n  = (int)(sizeof(pfx) - 1);
            line[n++] = (char)('1' + r);
            line[n++] = ':';
            line[n++] = ' ';

            for (int c = 0; c < 6; c++) {
                n += fmt_float(line + n, cap - n, g_matrix.m[r][c]);
                if (c != 5) {
                    if (n < cap - 1) line[n++] = ',';
                    if (n < cap - 1) line[n++] = ' ';
                }
            }
            if (n < cap - 2) {
                line[n++] = '\r';
                line[n++] = '\n';
            }
            /* Plain text, no AA 55 header — same style as the 0x05 SN query,
               so the output is readable as-is in a terminal. */
            tx->send_text((const uint8_t *)line, (uint16_t)n);
        }
        break;
    }

    /* ---- 0x34  Set data format ---- */
    case RS485_CMD_SET_FORMAT:
        g_sys.data_format = 0;
        FloatFilter_Init();
        tx_ack(tx, cmd);
        break;

    /* ---- 0x33  Set data format: kg (persisted) ---- */
    case RS485_CMD_SET_FORMAT_KG:
        g_sys.data_format = 1;
        data_format_save(1);
        FloatFilter_Init();
        tx_ack(tx, cmd);
        break;

    /* ---- 0x35  Set data format: N (persisted) ---- */
    case RS485_CMD_SET_FORMAT_N:
        g_sys.data_format = 2;
        data_format_save(2);
        FloatFilter_Init();
        tx_ack(tx, cmd);
        break;

    /* ---- 0x20  Upload decoupling matrix (6×6 × 4 = 144 bytes) ---- */
    case RS485_CMD_SET_MATRIX:
        if (len < 25) { g_sys.comm_error_cnt++; break; }
        for (int i = 0; i < 6; i++) {
            g_matrix.m[payload[0]][i] = le_to_float(payload + 1 + i * 4);
        }
        flash_save_matrix(payload[0]);
        tx_ack(tx, (uint8_t)(cmd | payload[0]));
        break;

    /* ---- 0x35  Debug mode toggle ---- */
    // case RS485_CMD_DEBUG_MODE:
    //     g_sys.debug_mode = (uint8_t)(g_sys.debug_mode ? 0U : 1U);
    //     buf[0] = g_sys.debug_mode;
        // respond(cmd, buf, 1);
    //     break;

    default:
        /* Unknown command — silently ignored */
        g_sys.comm_error_cnt++;
        break;
    }
}

/* ================================================================
 *  rs485_cmd_dispatch — RS485 通道（AA 55 ... 0D 0A）
 * ================================================================ */
static const cmd_transport_t g_rs485_transport = {
    .respond         = respond,
    .send_force      = rs485_send_data_frame,
    .send_text       = original_send,
    .continuous_mode = 1,          /* g_sys.send_mode = 1 -> RS485 连续发送 */
};

void rs485_cmd_dispatch(uint8_t cmd, const uint8_t *payload, uint16_t len)
{
    cmd_dispatch(&g_rs485_transport, cmd, payload, len);
}
