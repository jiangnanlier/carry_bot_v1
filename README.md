# carry_bot_v1

RoboMaster / 智能物流搬运机器人主控固件。

| 项 | 值 |
|---|---|
| 主控 | **STM32F407IGH6**（DJI RoboMaster 开发板 C 型，UFBGA176） |
| 时钟 | HSE 12 MHz → PLL → 168 MHz SYSCLK / 48 MHz USB |
| RTOS | FreeRTOS（heap_4 + CMSIS-RTOS V2） |
| 工具链 | `arm-none-eabi-gcc` 14.3.1 + CMake ≥3.22 + Ninja |
| 构建 | `cmake --preset Debug && cmake --build --preset Debug` → `build/Debug/carry_bot_v1.elf` |

## 当前状态

- `main`：蓝牙链路基线（`app/BT`，USART6 9600 + DMA/IDLE 收帧、队列、协议层可插拔）
- 硬件适配：C 板时钟、USART1 引脚、LED/KEY 已按手册与官方例程对齐 → [docs/DJI_C_BOARD.md](docs/DJI_C_BOARD.md)
- 全仓 21 个分支的审计结论 → [SUMMARY.md](SUMMARY.md) / [docs/BRANCH_AUDIT.md](docs/BRANCH_AUDIT.md)

## 目录

```
Core/            CubeMX 生成层（外设初始化 / 中断 / FreeRTOS 入口）
Drivers/         STM32F4 HAL + CMSIS（CubeMX 生成，不手改）
Middlewares/     FreeRTOS
app/BT/          USART6 蓝牙链路（基线唯一自研模块）
cmake/           工具链文件 + CubeMX 构建清单
docs/            硬件适配 / 分支审计 / CI 说明
```

业务模块（`app/Link`、`app/ZDT`、`app/Bridge`、`app/Servo`、`UserLib/*`、`BSP/*`）目前只存在于功能分支，
合并路线见 `SUMMARY.md` 第三节。

## 快速开始

```bash
cmake --preset Debug
cmake --build --preset Debug
```

烧录（OpenOCD + CMSIS-DAP）：

```bash
openocd -f openocd.cfg -c "program build/Debug/carry_bot_v1.elf verify reset exit"
```

`openocd.cfg` 与 `.vscode/launch.json` 在 `feature/debug-openocd` 分支；注意其中的绝对路径需按本机修改。
