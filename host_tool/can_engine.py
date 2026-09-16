# -*- coding: utf-8 -*-
"""
CAN 通信引擎

提供两种后端：
    1. ZLGEngine  — 通过 ctypes 调用周立功 ControlCAN.dll（USBCAN-I/II）
    2. SimEngine  — 纯软件模拟，生成正弦变化的六维力数据，方便无硬件调试

统一接口：
    open() / close() / send(id, data) / receive(timeout) / is_open
"""
import ctypes
import math
import os
import queue
import struct
import threading
import time

from config import VCI_USBCAN1, VCI_USBCAN2, CAN_BAUD_TABLE, DEFAULT_BAUD


# ---------------------------------------------------------------------------
# ZLG ControlCAN 结构体（与 ControlCAN.h 保持一致，Windows 自然对齐）
# ---------------------------------------------------------------------------
class VCI_CAN_OBJ(ctypes.Structure):
    _fields_ = [
        ("ID", ctypes.c_uint),          # 帧 ID
        ("TimeStamp", ctypes.c_uint),   # 时间戳
        ("TimeFlag", ctypes.c_ubyte),   # 是否使用时间戳
        ("SendType", ctypes.c_ubyte),   # 发送类型: 0 正常发送, 1 单次发送
        ("RemoteFlag", ctypes.c_ubyte), # 远程帧标志
        ("ExternFlag", ctypes.c_ubyte), # 扩展帧标志
        ("DataLen", ctypes.c_ubyte),    # 数据长度
        ("Data", ctypes.c_ubyte * 8),   # 数据
        ("Reserved", ctypes.c_ubyte * 3),
    ]


class VCI_INIT_CONFIG(ctypes.Structure):
    _fields_ = [
        ("AccCode", ctypes.c_uint),
        ("AccMask", ctypes.c_uint),
        ("Reserved", ctypes.c_uint),
        ("Filter", ctypes.c_ubyte),   # 0 接收所有帧, 1 单滤波, 2 双滤波
        ("Timing0", ctypes.c_ubyte),
        ("Timing1", ctypes.c_ubyte),
        ("Mode", ctypes.c_ubyte),     # 0 正常模式, 1 只听模式
    ]


class VCI_BOARD_INFO(ctypes.Structure):
    _fields_ = [
        ("hw_Version", ctypes.c_ushort),
        ("fw_Version", ctypes.c_ushort),
        ("dr_Version", ctypes.c_ushort),
        ("in_Version", ctypes.c_ushort),
        ("irq_Num", ctypes.c_ushort),
        ("can_Num", ctypes.c_ubyte),
        ("str_Serial_Num", ctypes.c_char * 20),
        ("str_hw_Type", ctypes.c_char * 40),
        ("Reserved", ctypes.c_ushort * 4),
    ]
    _pack_ = 1


class CANError(Exception):
    pass


# ---------------------------------------------------------------------------
# 统一接收到的数据点
# ---------------------------------------------------------------------------
class CanFrame:
    __slots__ = ("can_id", "data", "timestamp")

    def __init__(self, can_id, data, timestamp=None):
        self.can_id = can_id
        self.data = data          # bytes 长度 8
        self.timestamp = timestamp if timestamp is not None else time.time()


# ---------------------------------------------------------------------------
# ZLG 引擎
# ---------------------------------------------------------------------------
class ZLGEngine:
    def __init__(self, dll_path=None, device_type=VCI_USBCAN2,
                 device_index=0, can_index=0, baud=DEFAULT_BAUD):
        self.dll_path = dll_path or ""
        self.device_type = device_type
        self.device_index = device_index
        self.can_index = can_index
        self.baud = baud

        self._dll = None
        self._opened = False
        self._rx_thread = None
        self._stop_evt = threading.Event()
        self._queue = queue.Queue(maxsize=4096)

    # -- dll 加载 ---------------------------------------------------------
    def _load_dll(self):
        names = []
        if self.dll_path:
            names.append(self.dll_path)
        names.append("ControlCAN.dll")  # 系统搜索路径（PATH / 当前目录）
        for n in names:
            try:
                return ctypes.WinDLL(n)
            except OSError:
                continue
        raise CANError(
            "无法加载 ControlCAN.dll，请检查：\n"
            "1) 已安装周立功驱动并将 ControlCAN.dll 放在程序目录或 PATH 中\n"
            "2) dll 位数（32/64）与当前 Python 位数一致"
        )

    @staticmethod
    def _check(ret, action):
        if not ret:
            raise CANError(f"{action}失败 (返回值 {ret})，请检查设备连接与驱动")

    # -- 设备扫描 ----------------------------------------------------------
    def scan_devices(self):
        """调用 VCI_FindUsbDevice2 枚举 USB-CAN 设备。"""
        if self._dll is None:
            self._dll = self._load_dll()
        try:
            find = self._dll.VCI_FindUsbDevice2
        except AttributeError:
            return []  # 旧版本 dll 无此接口
        info = (VCI_BOARD_INFO * 10)()
        n = find(info)
        result = []
        for i in range(n):
            name = info[i].str_hw_Type.decode("gbk", "replace").strip()
            sn = info[i].str_Serial_Num.decode("ascii", "replace").strip()
            result.append({"index": i, "name": name or "USB-CAN", "sn": sn,
                           "can_num": info[i].can_Num})
        return result

    # -- 打开/关闭 ----------------------------------------------------------
    def open(self):
        if self._opened:
            return
        if self._dll is None:
            self._dll = self._load_dll()

        t0, t1 = CAN_BAUD_TABLE.get(self.baud, CAN_BAUD_TABLE[DEFAULT_BAUD])
        cfg = VCI_INIT_CONFIG()
        cfg.AccCode = 0x00000000
        cfg.AccMask = 0xFFFFFFFF   # 接收所有帧
        cfg.Filter = 0             # 不滤波
        cfg.Timing0 = t0
        cfg.Timing1 = t1
        cfg.Mode = 0               # 正常模式

        if not self._dll.VCI_OpenDevice(self.device_type, self.device_index, 0):
            raise CANError("打开设备失败，请检查设备类型 / 索引 / 驱动是否安装")
        try:
            self._check(self._dll.VCI_InitCAN(self.device_type, self.device_index,
                                              self.can_index, ctypes.byref(cfg)),
                        "初始化 CAN")
            self._check(self._dll.VCI_StartCAN(self.device_type, self.device_index,
                                               self.can_index), "启动 CAN")
        except CANError:
            self._dll.VCI_CloseDevice(self.device_type, self.device_index)
            raise

        self._opened = True
        self._stop_evt.clear()
        self._rx_thread = threading.Thread(target=self._rx_loop, daemon=True)
        self._rx_thread.start()

    def close(self):
        if not self._opened:
            return
        self._opened = False
        self._stop_evt.set()
        if self._rx_thread and self._rx_thread.is_alive():
            self._rx_thread.join(timeout=1.0)
        try:
            self._dll.VCI_ResetCAN(self.device_type, self.device_index, self.can_index)
            self._dll.VCI_CloseDevice(self.device_type, self.device_index)
        except Exception:
            pass
        self._rx_thread = None

    @property
    def is_open(self):
        return self._opened

    # -- 接收线程 ----------------------------------------------------------
    def _rx_loop(self):
        n_recv = self._dll.VCI_GetReceiveNum
        recv = self._dll.VCI_Receive
        while not self._stop_evt.is_set():
            try:
                count = n_recv(self.device_type, self.device_index, self.can_index)
                if count > 0:
                    arr = (VCI_CAN_OBJ * count)()
                    got = recv(self.device_type, self.device_index, self.can_index,
                               arr, count, 50)
                    for i in range(got):
                        obj = arr[i]
                        data = bytes(obj.Data[:obj.DataLen])
                        self._queue.put(CanFrame(obj.ID, data), block=False)
                else:
                    time.sleep(0.001)
            except Exception:
                if not self._stop_evt.is_set():
                    time.sleep(0.01)

    def receive(self, timeout=0):
        """取出当前已缓冲的所有帧，返回列表。timeout<=0 时直接清空队列。"""
        frames = []
        if timeout <= 0:
            while True:
                try:
                    frames.append(self._queue.get_nowait())
                except queue.Empty:
                    break
            return frames
        deadline = time.time() + timeout
        while time.time() < deadline:
            try:
                frames.append(self._queue.get_nowait())
            except queue.Empty:
                if time.time() >= deadline:
                    break
                time.sleep(0.001)
        return frames

    def send(self, can_id, data):
        if not self._opened:
            raise CANError("CAN 未连接")
        obj = VCI_CAN_OBJ()
        obj.ID = can_id
        obj.SendType = 0
        obj.RemoteFlag = 0
        obj.ExternFlag = 0
        obj.DataLen = len(data)
        for i, b in enumerate(data):
            obj.Data[i] = b
        self._check(self._dll.VCI_Transmit(self.device_type, self.device_index,
                                           self.can_index, ctypes.byref(obj), 1),
                    "发送帧")


# ---------------------------------------------------------------------------
# 模拟引擎（无硬件时调试用）
# ---------------------------------------------------------------------------
class SimEngine:
    """周期性产生 0x101/0x102/0x103 三帧，六维力为正弦变化。"""

    def __init__(self, interval_ms=10, **_):
        self.interval = interval_ms / 1000.0
        self._opened = False
        self._thread = None
        self._stop_evt = threading.Event()
        self._queue = queue.Queue(maxsize=4096)
        self._t = 0.0

    def open(self):
        if self._opened:
            return
        self._opened = True
        self._stop_evt.clear()
        self._thread = threading.Thread(target=self._loop, daemon=True)
        self._thread.start()

    def close(self):
        self._opened = False
        self._stop_evt.set()
        if self._thread and self._thread.is_alive():
            self._thread.join(timeout=1.0)
        self._thread = None

    @property
    def is_open(self):
        return self._opened

    def _loop(self):
        while not self._stop_evt.is_set():
            t = self._t
            fx = 50.0 * math.sin(2 * math.pi * 0.8 * t) + 2.0
            fy = 40.0 * math.sin(2 * math.pi * 0.6 * t + 1.0) - 1.0
            fz = 120.0 + 30.0 * math.sin(2 * math.pi * 0.3 * t)
            tx = 8.0 * math.sin(2 * math.pi * 1.2 * t + 0.5)
            ty = 6.0 * math.sin(2 * math.pi * 0.9 * t + 2.0)
            tz = 4.0 * math.sin(2 * math.pi * 1.5 * t + 3.0)
            frames = [
                (0x101, fx, fy),
                (0x102, fz, tx),
                (0x103, ty, tz),
            ]
            for can_id, v1, v2 in frames:
                data = struct.pack("<ff", v1, v2)
                self._queue.put(CanFrame(can_id, data), block=False)
            self._t += self.interval
            self._stop_evt.wait(self.interval)

    def receive(self, timeout=0):
        frames = []
        if timeout <= 0:
            while True:
                try:
                    frames.append(self._queue.get_nowait())
                except queue.Empty:
                    break
            return frames
        deadline = time.time() + timeout
        while time.time() < deadline:
            try:
                frames.append(self._queue.get_nowait())
            except queue.Empty:
                if time.time() >= deadline:
                    break
                time.sleep(0.001)
        return frames

    def send(self, can_id, data):
        """模拟模式下，0x201 指令帧作为回显触发，立即在回读队列中复现。"""
        if not self._opened:
            raise CANError("CAN 未连接")
        if can_id == 0x201:
            t = self._t
            fx = 50.0 * math.sin(2 * math.pi * 0.8 * t) + 2.0
            fy = 40.0 * math.sin(2 * math.pi * 0.6 * t + 1.0) - 1.0
            fz = 120.0 + 30.0 * math.sin(2 * math.pi * 0.3 * t)
            tx = 8.0 * math.sin(2 * math.pi * 1.2 * t + 0.5)
            ty = 6.0 * math.sin(2 * math.pi * 0.9 * t + 2.0)
            tz = 4.0 * math.sin(2 * math.pi * 1.5 * t + 3.0)
            for can_id_f, v1, v2 in ((0x101, fx, fy), (0x102, fz, tx), (0x103, ty, tz)):
                data_b = struct.pack("<ff", v1, v2)
                self._queue.put(CanFrame(can_id_f, data_b), block=False)


# ---------------------------------------------------------------------------
def create_engine(mode, **kwargs):
    """工厂函数：根据模式创建引擎实例。"""
    if mode == "sim":
        return SimEngine(**kwargs)
    return ZLGEngine(**kwargs)
