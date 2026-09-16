# -*- coding: utf-8 -*-
"""CAN 帧解析：把原始 CAN 帧转换为六维力数据字典。"""
import struct

from config import CAN_CHANNEL_LAYOUT


def parse_can_frame(can_id, data):
    """
    解析一帧 CAN 数据。

    :param can_id: 帧 ID（int）
    :param data:   8 字节原始数据（bytes-like）
    :return: dict {字段名: float}；非六维力帧返回 None
    """
    layout = CAN_CHANNEL_LAYOUT.get(can_id)
    if layout is None:
        return None

    if data is None or len(data) < 8:
        return None

    result = {}
    for name, offset in layout:
        result[name] = struct.unpack("<f", bytes(data[offset:offset + 4]))[0]
    return result
