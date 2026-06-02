# Changelog

本文件记录 Clawdmeter SF32 移植版的版本变更。格式遵循 [Keep a Changelog](https://keepachangelog.com/zh-CN/1.1.0/)。

## [Unreleased]

### 新增
- 蓝牙页面"重置蓝牙"按钮（垃圾桶图标 + "Reset Bluetooth"，移植自原始 ESP32 工程）
- 蓝牙页面 MAC 地址字段（Address: --:--:--:--:--:--，待 BLE 模块接入）
- `ble_clear_bonds()` 存根函数（当前仅打印日志，等 BLE 实现后替换）

### 变更
- 蓝牙页面移除"Enabled: no"标签，Device 标签上移至原 enabled 位置

## [0.1.0] - 2025-06-03

### 新增
- 3 个 UI 页面：Splash 像素动画、Usage 用量仪表盘、Bluetooth 连接状态
- LVGL 横向滚动 + scroll snap 页面切换
- 13 个 20×20 像素动画，20 秒自动轮播
- 自定义字体：Styrene（14/16/20/24/28/48px）、Tiempos（34/56px）、Mono（32px）
- CO5300 AMOLED 屏幕驱动（QSPI）
- FT6146 触摸驱动（修复 UP 事件坐标 bug）
- Logo TRUE_COLOR_ALPHA 格式转换
