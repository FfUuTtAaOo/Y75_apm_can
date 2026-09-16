# -*- coding: utf-8 -*-
"""
CAN 协议配置

帧布局（均使用小端 float32）：
    0x101 发送: data[0:4] = FX,  data[4:8] = FY
    0x102 发送: data[0:4] = FZ,  data[4:8] = TX
    0x103 发送: data[0:4] = TY,  data[4:8] = TZ
    0x201 接收: 上位机 -> 设备，触发设备发送六维力数据

修改这里即可适配新的 CAN ID 或字段布局。
"""

# ---------------------------------------------------------------------------
# CAN 帧 ID
# ---------------------------------------------------------------------------
CAN_ID_TX_FORCE = 0x101   # FX, FY
CAN_ID_TX_MOMENT = 0x102  # FZ, TX
CAN_ID_TX_FYFZ = 0x103    # TY, TZ
CAN_ID_RX_CMD = 0x201     # 指令帧

# 每帧内字段布局: CAN_ID -> [(字段名, 数据起始偏移字节)]
CAN_CHANNEL_LAYOUT = {
    CAN_ID_TX_FORCE:  [("FX", 0), ("FY", 4)],
    CAN_ID_TX_MOMENT: [("FZ", 0), ("TX", 4)],
    CAN_ID_TX_FYFZ:   [("TY", 0), ("TZ", 4)],
}

# 六维力字段显示顺序（曲线图、表格共用）
FORCE_FIELDS = ["FX", "FY", "FZ", "TX", "TY", "TZ"]
FIELD_UNITS = {
    "FX": "N", "FY": "N", "FZ": "N",
    "TX": "N·m", "TY": "N·m", "TZ": "N·m",
}

# 曲线滚动窗口长度（采样点个数）
PLOT_POINTS = 500

# ---------------------------------------------------------------------------
# ZLG ControlCAN 设备类型
# ---------------------------------------------------------------------------
VCI_USBCAN1 = 3    # USBCAN-I
VCI_USBCAN2 = 4    # USBCAN-II

DEVICE_TYPES = {
    "USBCAN-I": VCI_USBCAN1,
    "USBCAN-II": VCI_USBCAN2,
}

# 波特率 -> (Timing0, Timing1)，出厂默认 500k 与板卡一致
CAN_BAUD_TABLE = {
    1000: (0x00, 0x14),
    800:  (0x00, 0x16),
    500:  (0x00, 0x1C),
    250:  (0x01, 0x1C),
    125:  (0x03, 0x1C),
    100:  (0x04, 0x1C),
    50:   (0x09, 0x1C),
    20:   (0x18, 0x1C),
    10:   (0x31, 0x1C),
}
DEFAULT_BAUD = 500  # kbps，与板卡 APM32F103 CAN 一致
