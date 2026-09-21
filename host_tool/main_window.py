# -*- coding: utf-8 -*-
"""主窗口：连接管理、指令发送、六维力实时曲线、数据表格。"""
import os
import time
from collections import deque
from collections import OrderedDict

import pyqtgraph as pg
from PySide6.QtCore import Qt, QTimer
from PySide6.QtGui import QRegularExpressionValidator, QColor
from PySide6.QtWidgets import (
    QApplication, QWidget, QMainWindow, QLabel, QPushButton, QComboBox,
    QSpinBox, QLineEdit, QCheckBox, QGridLayout, QHBoxLayout, QVBoxLayout,
    QGroupBox, QTableWidget, QTableWidgetItem, QFileDialog, QMessageBox,
    QSplitter, QFrame, QHeaderView,
)
from PySide6.QtCore import QRegularExpression

from config import (
    CAN_ID_RX_CMD, FORCE_FIELDS, FIELD_UNITS, PLOT_POINTS,
    DEVICE_TYPES, CAN_BAUD_TABLE, DEFAULT_BAUD,
)
from can_engine import create_engine, CANError
from data_parser import parse_can_frame

# 六维力曲线配色
FIELD_COLORS = {
    "FX": "#e74c3c", "FY": "#e67e22", "FZ": "#2980b9",
    "TX": "#16a085", "TY": "#27ae60", "TZ": "#8e44ad",
}


class MainWindow(QMainWindow):
    def __init__(self):
        super().__init__()
        self.setWindowTitle("六维力传感器上位机 (CAN)")
        self.resize(1280, 860)

        self.engine = None          # 当前引擎（ZLG 或 模拟）
        self.running = False

        # 曲线数据缓冲：字段名 -> deque(最大值列表)；用统一采样序号作 x 轴
        self._sample = 0
        self._buffers = OrderedDict((f, deque(maxlen=PLOT_POINTS)) for f in FORCE_FIELDS)
        self._curves = {}
        self._latest = {}           # 字段名 -> 最新值
        self._rx_count = 0
        self._rate_count = 0        # 上次刷新时的帧计数（用于算速率）
        self._rx_per_frame = {0x101: 0, 0x102: 0, 0x103: 0}

        self._build_ui()
        self._timer = QTimer(self)
        self._timer.setInterval(50)
        self._timer.timeout.connect(self._on_timer)

        self._periodic_timer = QTimer(self)
        self._periodic_timer.timeout.connect(self._send_cmd)

    # ------------------------------------------------------------------ UI
    def _build_ui(self):
        central = QWidget()
        self.setCentralWidget(central)
        root = QVBoxLayout(central)
        root.setContentsMargins(8, 8, 8, 8)
        root.setSpacing(6)

        root.addWidget(self._build_conn_group())
        root.addWidget(self._build_plot_group(), stretch=1)

        bottom = QSplitter(Qt.Horizontal)
        bottom.addWidget(self._build_cmd_group())
        bottom.addWidget(self._build_table_group())
        bottom.setSizes([380, 560])
        root.addWidget(bottom, stretch=0)

        self.statusBar().showMessage("未连接")

    def _build_conn_group(self):
        g = QGroupBox("连接设置")
        lay = QHBoxLayout(g)

        lay.addWidget(QLabel("接口:"))
        self.cmb_mode = QComboBox()
        self.cmb_mode.addItem("ZLG USB-CAN (ControlCAN.dll)", "zlg")
        self.cmb_mode.addItem("模拟模式 (无需硬件)", "sim")
        self.cmb_mode.currentIndexChanged.connect(self._on_mode_changed)
        lay.addWidget(self.cmb_mode)

        lay.addWidget(QLabel("设备类型:"))
        self.cmb_devtype = QComboBox()
        for name in DEVICE_TYPES:
            self.cmb_devtype.addItem(name, DEVICE_TYPES[name])
        lay.addWidget(self.cmb_devtype)

        lay.addWidget(QLabel("设备索引:"))
        self.spin_devidx = QSpinBox()
        self.spin_devidx.setRange(0, 15)
        self.spin_devidx.setValue(0)
        lay.addWidget(self.spin_devidx)

        lay.addWidget(QLabel("CAN通道:"))
        self.cmb_can = QComboBox()
        self.cmb_can.addItems(["CAN0", "CAN1"])
        lay.addWidget(self.cmb_can)

        lay.addWidget(QLabel("波特率(kbps):"))
        self.cmb_baud = QComboBox()
        for k in sorted(CAN_BAUD_TABLE, reverse=True):
            self.cmb_baud.addItem(str(k), k)
        self.cmb_baud.setCurrentText(str(DEFAULT_BAUD))
        lay.addWidget(self.cmb_baud)

        lay.addWidget(QLabel("DLL:"))
        self.edit_dll = QLineEdit()
        self.edit_dll.setPlaceholderText("留空则搜索系统路径")
        self.edit_dll.setMinimumWidth(140)
        lay.addWidget(self.edit_dll)
        btn_dll = QPushButton("...")
        btn_dll.setFixedWidth(30)
        btn_dll.clicked.connect(self._pick_dll)
        lay.addWidget(btn_dll)

        self.btn_scan = QPushButton("扫描设备")
        self.btn_scan.clicked.connect(self._scan_devices)
        lay.addWidget(self.btn_scan)

        self.btn_conn = QPushButton("连接")
        self.btn_conn.setFixedWidth(80)
        self.btn_conn.clicked.connect(self._toggle_connect)
        lay.addWidget(self.btn_conn)
        lay.addStretch(1)
        return g

    def _build_plot_group(self):
        g = QGroupBox("六维力实时曲线")
        lay = QGridLayout(g)
        lay.setSpacing(4)
        for i, field in enumerate(FORCE_FIELDS):
            p = pg.PlotWidget(background="#ffffff")
            p.setTitle(f"{field} ({FIELD_UNITS[field]})")
            p.setLabel("left", field)
            p.showGrid(x=True, y=True, alpha=0.3)
            p.setMouseEnabled(x=False, y=False)
            curve = p.plot(pen=pg.mkPen(FIELD_COLORS[field], width=2))
            self._curves[field] = curve
            lay.addWidget(p, i // 3, i % 3)
        return g

    def _build_cmd_group(self):
        g = QGroupBox("指令发送 (0x201)")
        lay = QVBoxLayout(g)

        row = QHBoxLayout()
        row.addWidget(QLabel("数据:"))
        self.hex_edits = []
        # 默认 "03 00 00 00 00 00 00 00" = 单次读取六维力
        default_bytes = ["03", "00", "00", "00", "00", "00", "00", "00"]
        for i in range(8):
            e = QLineEdit(default_bytes[i])
            e.setFixedWidth(30)
            e.setAlignment(Qt.AlignCenter)
            e.setValidator(QRegularExpressionValidator(
                QRegularExpression("[0-9A-Fa-f]{0,2}")))
            self.hex_edits.append(e)
            row.addWidget(e)
        row.addStretch(1)
        lay.addLayout(row)

        row2 = QHBoxLayout()
        self.chk_periodic = QCheckBox("周期发送")
        self.chk_periodic.stateChanged.connect(self._on_periodic_toggled)
        row2.addWidget(self.chk_periodic)
        row2.addWidget(QLabel("间隔(ms):"))
        self.spin_period = QSpinBox()
        self.spin_period.setRange(10, 60000)
        self.spin_period.setValue(200)
        row2.addWidget(self.spin_period)
        self.btn_send = QPushButton("发送")
        self.btn_send.clicked.connect(self._send_cmd)
        row2.addWidget(self.btn_send)
        row2.addStretch(1)
        lay.addLayout(row2)

        tip = QLabel(
            "说明: data[0]=命令码, 参数从 data[1] 起(高字节在前)。\n"
            "  03=单次读取  01=停止  02=连续发送  04 00 01=波特率460800\n"
            "  06=查版本  07=查状态  09 00 02=修改CAN ID为0x0002\n"
            "  33=kg  35=N(均存EEPROM)  30=校零  36=取消校零")
        tip.setWordWrap(True)
        tip.setStyleSheet("color:#888; font-size:11px;")
        lay.addWidget(tip)
        return g

    def _build_table_group(self):
        g = QGroupBox("最新数据")
        lay = QVBoxLayout(g)
        self.table = QTableWidget(len(FORCE_FIELDS) + 2, 3)
        self.table.setHorizontalHeaderLabels(["字段", "数值", "单位"])
        self.table.verticalHeader().setVisible(False)
        self.table.horizontalHeader().setSectionResizeMode(1, QHeaderView.Stretch)
        for r, f in enumerate(FORCE_FIELDS):
            item = QTableWidgetItem(f)
            item.setFlags(item.flags() & ~Qt.ItemIsEditable)
            self.table.setItem(r, 0, item)
            val = QTableWidgetItem("--")
            val.setFlags(val.flags() & ~Qt.ItemIsEditable)
            self.table.setItem(r, 1, val)
            unit = QTableWidgetItem(FIELD_UNITS[f])
            unit.setFlags(unit.flags() & ~Qt.ItemIsEditable)
            self.table.setItem(r, 2, unit)
        r = len(FORCE_FIELDS)
        self.table.setItem(r, 0, QTableWidgetItem("接收帧数"))
        self.table.setItem(r, 1, QTableWidgetItem("0"))
        self.table.setItem(r, 2, QTableWidgetItem(""))
        r += 1
        self.table.setItem(r, 0, QTableWidgetItem("接收速率"))
        self.table.setItem(r, 1, QTableWidgetItem("0 帧/s"))
        self.table.setItem(r, 2, QTableWidgetItem(""))
        lay.addWidget(self.table)
        return g

    # ------------------------------------------------------------- 事件处理
    def _pick_dll(self):
        path, _ = QFileDialog.getOpenFileName(self, "选择 ControlCAN.dll", "",
                                              "DLL (*.dll)")
        if path:
            self.edit_dll.setText(path)

    def _on_mode_changed(self):
        sim = self.current_mode() == "sim"
        for w in (self.cmb_devtype, self.spin_devidx, self.cmb_can,
                  self.cmb_baud, self.edit_dll, self.btn_scan):
            w.setEnabled(not sim)

    def current_mode(self):
        return self.cmb_mode.currentData()

    def _engine_kwargs(self):
        if self.current_mode() == "sim":
            return {"mode": "sim"}
        return {
            "mode": "zlg",
            "dll_path": self.edit_dll.text().strip(),
            "device_type": self.cmb_devtype.currentData(),
            "device_index": self.spin_devidx.value(),
            "can_index": self.cmb_can.currentIndex(),
            "baud": self.cmb_baud.currentData(),
        }

    def _scan_devices(self):
        try:
            eng = create_engine(**self._engine_kwargs())
            devs = eng.scan_devices()
        except CANError as e:
            QMessageBox.warning(self, "扫描设备", str(e))
            return
        if not devs:
            QMessageBox.information(self, "扫描设备",
                                    "未找到 USB-CAN 设备，请检查驱动与连接。")
            return
        lines = [f"找到 {len(devs)} 个设备："]
        for d in devs:
            lines.append(f"  [{d['index']}] {d['name']}  SN:{d['sn']}  CAN通道:{d['can_num']}")
        QMessageBox.information(self, "扫描设备", "\n".join(lines))
        # 自动选中第一个设备
        self.spin_devidx.setValue(devs[0]["index"])

    def _toggle_connect(self):
        if self.running:
            self._disconnect()
        else:
            self._connect()

    def _connect(self):
        try:
            self.engine = create_engine(**self._engine_kwargs())
            self.engine.open()
        except CANError as e:
            self.engine = None
            QMessageBox.critical(self, "连接失败", str(e))
            return
        self.running = True
        self.btn_conn.setText("断开")
        self.statusBar().showMessage(
            "已连接 | 接口: %s | 波特率: %d kbps"
            % ("ZLG " + self.cmb_devtype.currentText(),
               self.cmb_baud.currentData()))
        self.btn_send.setEnabled(True)
        self._reset_plots()
        self._timer.start()
        self.chk_periodic.setEnabled(True)
        self._on_periodic_toggled()

    def _disconnect(self):
        self._timer.stop()
        self._periodic_timer.stop()
        if self.engine:
            self.engine.close()
            self.engine = None
        self.running = False
        self.btn_conn.setText("连接")
        self.btn_send.setEnabled(False)
        self.chk_periodic.setEnabled(False)
        self.statusBar().showMessage("已断开")

    def _reset_plots(self):
        self._sample = 0
        self._rx_count = 0
        self._rx_per_frame = {0x101: 0, 0x102: 0, 0x103: 0}
        self._latest.clear()
        for f in FORCE_FIELDS:
            self._buffers[f].clear()
            self._curves[f].setData([], [])
        for r in range(len(FORCE_FIELDS)):
            self.table.item(r, 1).setText("--")

    # ------------------------------------------------------------- 数据刷新
    def _on_timer(self):
        if not self.engine:
            return
        frames = self.engine.receive(timeout=0)
        if frames:
            for fr in frames:
                parsed = parse_can_frame(fr.can_id, fr.data)
                if parsed is None:
                    continue
                self._rx_count += 1
                self._rx_per_frame[fr.can_id] = self._rx_per_frame.get(fr.can_id, 0) + 1
                self._sample += 1
                for f, v in parsed.items():
                    self._buffers[f].append(v)
                    self._latest[f] = v
                # 每收到一帧就刷新曲线（画在统一时间轴上）
                x = list(range(self._sample))
                for f in FORCE_FIELDS:
                    buf = self._buffers[f]
                    if len(buf) > 0:
                        self._curves[f].setData(x[-len(buf):], list(buf))
            self._update_table()

    def _update_table(self):
        for r, f in enumerate(FORCE_FIELDS):
            v = self._latest.get(f)
            self.table.item(r, 1).setText("--" if v is None else f"{v:.3f}")
        total = len(FORCE_FIELDS)
        self.table.item(total, 1).setText(str(self._rx_count))
        # 定时器周期 50ms -> 每秒 20 次刷新
        self.table.item(total + 1, 1).setText(
            f"{(self._rx_count - self._rate_count) * 20:.0f} 帧/s")
        self._rate_count = self._rx_count

    # ------------------------------------------------------------- 指令发送
    def _send_cmd(self):
        if not self.running:
            return
        try:
            data = bytes(int(e.text().strip() or "0", 16) for e in self.hex_edits)
        except ValueError:
            self.statusBar().showMessage("数据格式错误：请输入 0~FF 的十六进制字节", 3000)
            return
        try:
            self.engine.send(CAN_ID_RX_CMD, data)
            self.statusBar().showMessage(
                f"已发送 0x{CAN_ID_RX_CMD:03X}: " +
                " ".join(f"{b:02X}" for b in data), 3000)
        except CANError as e:
            self.statusBar().showMessage(f"发送失败: {e}", 3000)

    def _on_periodic_toggled(self):
        if self.chk_periodic.isChecked() and self.running:
            self._periodic_timer.start(self.spin_period.value())
        else:
            self._periodic_timer.stop()

    def closeEvent(self, event):
        self._disconnect()
        super().closeEvent(event)
