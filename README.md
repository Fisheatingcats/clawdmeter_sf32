# Clawdmeter · SF32 移植版

基于 [HermannBjorgvin/Clawdmeter](https://github.com/HermannBjorgvin/Clawdmeter) 的 AI 用量监控仪表盘，移植到 SiFli SF32LB52X（立创黄山派）平台。

上游项目的 MCP 服务端已适配：[Fisheatingcats/clawdmeter_mcp](https://github.com/Fisheatingcats/clawdmeter_mcp)

> 📋 更新日志见 [CHANGELOG.md](CHANGELOG.md)

## 项目简介

Clawdmeter 是一个运行在手表/小屏设备上的 **AI Agent 用量监控器**，通过 BLE 与电脑端 MCP 服务器通信，实时显示：

- **Session 用量**：当前会话的 API 配额使用百分比
- **Weekly 用量**：每周配额使用百分比
- **Agent 状态**：正在执行的任务（如 "Accomplishing"、"Computing"）
- **像素动画**：根据用量速率自动切换的 20×20 像素生物动画

## 当前进度

| 模块 | 状态 | 说明 |
|------|------|------|
| UI 页面 | ✅ 已完成 | 3 个页面，LVGL 横向滚动 snap 切换 |
| 像素动画 | ✅ 已完成 | 13 个动画，20 秒自动轮播 |
| 自定义字体 | ✅ 已完成 | Styrene / Tiempos / Mono 共 9 个字号 |
| BLE 通信 | 🔲 待开发 | 与 MCP 服务端的 GATT 数据交换 |
| 麦克风接口 | 📋 计划中 | 预留音频采集能力 |
| 喇叭接口 | 📋 计划中 | 预留音频播放能力 |

## UI 页面

三个页面通过左右滑动切换（LVGL 原生 scroll snap）：

### 1. Splash — 像素动画

全屏 20×20 像素画布，动画来自 [claudepix.vercel.app](https://claudepix.vercel.app)。13 个动画按类别分组：

- **Idle**：breathe、blink、look around
- **Expression**：wink、surprise、sleep
- **Work**：coding、think
- **Dance**：sway、bounce、dj variants

### 2. Usage — 用量仪表盘

- 顶部 Logo + "Usage" 标题
- **Current** 面板：会话用量百分比 + 进度条 + 重置倒计时
- **Weekly** 面板：每周用量百分比 + 进度条 + 重置倒计时
- 底部动画状态文字（spinner 循环 + 旋转消息）

### 3. Bluetooth — 连接状态

- 蓝牙图标 + 连接状态（Connected / Advertising / Disconnected）
- 设备名称、MAC 地址
- 重置蓝牙按钮（垃圾桶图标 + "Reset Bluetooth"，点击清除配对并重新广播）
- 底部版本信息

## 硬件平台

| 项目 | 规格 |
|------|------|
| 芯片 | SF32LB52X (Cortex-M33) |
| 开发板 | 立创黄山派 (sf32lb52-lchspi-ulp) |
| 屏幕 | CO5300 1.85" AMOLED, 390×450, QSPI |
| 触摸 | FT6146, I2C |
| 内存 | 128M PSRAM + NOR Flash |
| 音频 | AW8155 功放（预留） |

## 目录结构

```
clawdmeter/
├── project/                        # 构建配置
│   ├── SConstruct                  # SCons 顶层构建脚本
│   ├── build_and_uart_download.ps1 # 编译 + 烧录脚本
│   ├── proj.conf                   # Kconfig 项目配置
│   └── ...
├── src/
│   ├── main.c                      # 入口，初始化 LVGL + Clawdmeter UI
│   ├── clawdmeter_ui.h             # 页面管理器接口 + 颜色/尺寸常量
│   ├── clawdmeter_ui.c             # LVGL scroll snap 页面管理
│   ├── clawdmeter_pages.c          # 3 个页面实现
│   ├── clawdmeter_assets/
│   │   ├── font_styrene_*.c        # Styrene 字体 (14/16/20/24/28/48px)
│   │   ├── font_tiempos_*.c        # Tiempos 字体 (34/56px)
│   │   ├── font_mono_32.c          # Mono 字体 (32px)
│   │   ├── icons.h                 # 蓝牙 + 垃圾桶 + 电池图标 (48×48)
│   │   ├── splash_animations.h     # 13 个像素动画数据
│   │   └── logo.h                  # Logo (80×80 RGB565A8，暂未使用)
│   └── SConscript                  # 源文件构建脚本
└── serial_monitor.py               # 串口监控工具 (COM21, 1Mbaud)
```

## 构建与烧录

### 环境要求

- SiFli SDK v2.4
- ARM GCC 14.2+
- Python 3.12+（SCons 构建依赖）
- PowerShell（**不要用 Git Bash**，SDK 的 MSYSTEM 检查会失败）

### 编译

```powershell
cd projects/clawdmeter/project
scons --board=sf32lb52-lchspi-ulp -j8
```

或使用构建脚本（编译 + 烧录）：

```powershell
.\build_and_uart_download.ps1 -ComPort COM21
```

### 烧录

```powershell
cd build_sf32lb52-lchspi-ulp_hcpu
sftool -p COM21 -c SF32LB52 -m nor write_flash `
    bootloader/bootloader.bin@0x12010000 `
    main.bin@0x12020000 `
    ftab/ftab.bin@0x12000000
```

### 串口监控

```bash
python serial_monitor.py
```

## 技术要点

### 页面管理

使用 LVGL v8 原生的横向滚动 + scroll snap，三个页面并排放置在可滚动容器中：

```c
lv_obj_set_scroll_dir(scr, LV_DIR_HOR);
lv_obj_set_scroll_snap_x(scr, LV_SCROLL_SNAP_CENTER);
```

页面常驻内存，无需手动 create/destroy，完全由 LVGL 处理滑动和 snap 动画。

### 像素动画

动画数据由 [claudepix.vercel.app](https://claudepix.vercel.app) 生成，格式为 20×20 网格 + 10 色 RGB565 调色板。每帧 400 字节，通过 `lv_canvas` 渲染到 PSRAM 缓冲区。

### 触摸驱动修复

上游 ft6146 驱动存在 bug：`touch_num==0` 时读取过期 I2C 寄存器数据，导致 UP 事件坐标始终为 (0,0)。修复方式为缓存最后有效坐标。同时修复了 `drv_touch.c` 中 MOVE 事件被静默丢弃的问题。

## 致谢

- [Clawdmeter](https://github.com/HermannBjorgvin/Clawdmeter) — 原始项目
- [clawdmeter_mcp](https://github.com/Fisheatingcats/clawdmeter_mcp) — MCP 服务端
- [claudepix.vercel.app](https://claudepix.vercel.app) — 像素动画素材
- [SiFli SDK](https://docs.sifli.com) — SF32 开发框架
