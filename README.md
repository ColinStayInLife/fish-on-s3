# 🎣 FishOn-S3 — 多竿智能鱼口监控系统

[![PlatformIO CI](https://github.com/ColinStayInLife/fish-on-s3/actions/workflows/platformio.yml/badge.svg)](https://github.com/ColinStayInLife/fish-on-s3/actions/workflows/platformio.yml)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)

**基于 M5Stack CoreS3 SE 的多竿智能鱼口监控系统。** 用 IMU 传感器 + BLE 无线传输，一个屏幕看所有竿的鱼口状态，识别大鱼/小鱼/闹窝。

```
┌─────────────┐       BLE       ┌──────────────┐
│  竿梢子机 ×8  │ ←───────────→ │  CoreS3 SE    │
│  ESP32-C3 +  │  广播信号       │  彩色仪表盘    │
│  MPU6050 IMU │               │  鱼口分析     │
│  ¥14/竿      │               │  气象推送     │
└─────────────┘               └──────────────┘
```

## ✨ 特性

- **多竿监控** — 1 个主机同时监控 8 根竿，BLE 无线
- **智能鱼口识别** — IMU 振动模式分析，区分大鱼咬钩 vs 小鱼闹窝
- **彩色仪表盘** — 1.9" 触控彩屏，每竿状态一目了然
- **声光报警** — 不同鱼种不同提醒音
- **渔获日志** — SD 卡记录每次中鱼的时间、鱼种、环境数据
- **气象集成** — 气压趋势、月相、最佳出钓时段
- **飞书推送** — 重大鱼口远程通知到手机
- **免子机模式** — 直接把 CoreS3 SE 放竿架上凑合用

## 🏗️ 系统架构

```
┌──────────────────────────────────────────────────────────┐
│                      FishOn-S3                            │
├────────────────┬─────────────────────────────────────────┤
│  竿梢子机 ×1-8  │          桌面主机 ×1                     │
├────────────────┼─────────────────────────────────────────┤
│  ESP32-C3      │  M5Stack CoreS3 SE                       │
│  MPU6050 IMU   │  1.9" IPS 320×170 触摸                   │
│  CR2032 电池   │  MPU6886 IMU (自校准/免子机模式)         │
│  IPX4 防水壳   │  BLE 扫描 → 接收所有竿信号               │
│  ~¥14/竿       │  WiFi → 气象/飞书推送                    │
│                │  喇叭 → 声音提醒                          │
│                │  SD卡 → 渔获日志                         │
└────────────────┴─────────────────────────────────────────┘
```

## 📦 硬件需求

### 主机 (已有)
- M5Stack CoreS3 SE × 1

### 竿梢子机 (每竿 DIY)
| 组件 | 型号 | 单价 |
|:----|:----|:----|
| 主控 | ESP32-C3 开发板 | ~¥6 |
| IMU | MPU6050 模块 | ~¥3 |
| 电源 | CR2032 电池座 | ~¥1 |
| 外壳 | 热缩管 / 3D打印 | ~¥2 |
| 导线 | 若干 | ~¥1 |
| **合计** | | **~¥14/竿** |

### 8竿全套成本
- CoreS3 SE: ¥0 (已有)
- 竿梢子机 ×8: ¥112
- **总计: ¥112**

## 📋 功能矩阵

| 功能 | 优先级 | 状态 | 说明 |
|:----|:------|:----|:-----|
| 多竿监测 | P0 | 📝 规划 | BLE扫描8根竿梢信号 |
| 鱼口检测 | P0 | 📝 规划 | IMU振动模式分析 |
| 大鱼/小鱼区分 | P0 | 📝 规划 | AI模式识别 |
| 声光报警 | P0 | 📝 规划 | 不同鱼种不同提醒 |
| 仪表盘显示 | P0 | 📝 规划 | 所有竿状态一览 |
| 历史记录 | P1 | 📝 规划 | 渔获日志存储 |
| 气象集成 | P1 | 📝 规划 | 气压+潮汐+最佳时间 |
| 飞书推送 | P2 | 📝 规划 | 重大鱼口远程通知 |
| 免子机模式 | P1 | 📝 规划 | 直接用主机读取钓竿振动 |

## 🎯 开发路线

```
Phase 1: IMU数据采集Demo         → 2天  ⬅ 进行中
Phase 2: BLE扫描 + 多竿识别      → 3天
Phase 3: 鱼口检测算法            → 5天
Phase 4: 完整UI仪表盘            → 5天
Phase 5: 竿梢子机固件            → 3天
Phase 6: 实钓测试 + 调优         → 3天
```

## 🚀 快速开始

```bash
# 克隆
git clone git@github.com:ColinStayInLife/fish-on-s3.git
cd fish-on-s3

# Phase 1: 运行 IMU Demo (PlatformIO)
cd firmware/phase1-imu-demo
pio run -t upload
```

## 📁 项目结构

```
fish-on-s3/
├── firmware/          # 固件代码 (PlatformIO/Arduino)
│   ├── phase1-imu-demo/
│   ├── phase2-ble-scan/
│   ├── phase3-fish-detect/
│   ├── phase4-ui/
│   └── rod-sensor/    # 竿梢子机固件
├── hardware/          # 硬件设计
│   ├── schematics/    # 电路图
│   ├── pcb/           # PCB设计
│   └── enclosure/     # 3D打印外壳
├── docs/              # 文档
│   ├── product-definition.md  # 产品定义说明书
│   └── -- 更多文档建设中
├── research/          # 市场调研
│   └── market-research.md     # 市场调研报告
└── README.md          # 本文件
```

## 🎣 使用场景

| 场景 | 说明 |
|:----|:-----|
| 🌙 **夜钓海竿阵** | 6根竿，屏幕一目了然，不用打手电筒 |
| 🐟 **守大物** | 过滤小鱼闹窝，只提醒大鱼爆口 |
| 🚣 **筏钓/桥钓** | IMU比肉眼提前1-2秒发现鱼口 |
| 🏕️ **休闲坐钓** | 不需要子机，直接放竿架上凑合用 |

## 📄 许可证

MIT License

## 👨‍💻 作者

[ColinStayInLife](https://github.com/ColinStayInLife)

---

> **正在开发中** — 欢迎 Star ⭐ 和 Issue 💡
