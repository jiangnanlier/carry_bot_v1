# IST8310 磁力计 —— CubeMX 配置清单（RoboMaster 开发板 C 型）

> 分支：`feature/ist8310-mag`（自 `feature/bsp-board-io`）  
> 状态：**仅文档**，驱动未实现。等你在 CubeMX 里加好 **I2C3 + 复位/DRDY 脚** 并 Generate Code 后，再在本分支落 `UserLib/IST8310`。  
> 硬件依据：`CubeMx/docs/RoboMaster 开发板 C 型用户手册.pdf` / 原理图；布局风格对齐本仓 `docs/BOARD_IO.md`。

## 1. 为什么要你先改 CubeMX

当前 `carry_bot_v1.ioc` **只有 I2C1**（PB8/PB9，给 PCA9685 等用），**没有 I2C3**，也没有 IST8310 的 RSTN/DRDY GPIO。  
磁力计走板载 **I2C3**，和 I2C1 不冲突；在驱动写死之前必须先把外设从 CubeMX 生成出来，并 **Keep User Code** 再提交 `.ioc` + 生成的 `i2c.c` / `gpio.c` / `main.h` 等。

本分支 **先不写驱动**，避免和未生成的 HAL 对不上。

## 2. CubeMX 勾选清单（请严格按此）

打开工程根目录 `carry_bot_v1.ioc`（STM32F407IGHx / Type-C）。

### 2.1 I2C3（磁力计总线）

| 项 | 设置 |
|---|---|
| 外设 | **I2C3** → Mode: **I2C** |
| 速度 | **Fast Mode 400 kHz**（与手册板载 mag 一致） |
| SCL | **PA8** → `I2C3_SCL` |
| SDA | **PC9** → `I2C3_SDA` |
| GPIO | 开漏 + 上拉（Cube 默认 AF 即可；勿改成推挽） |
| DMA | **v1 可不配**（先轮询 `HAL_I2C_Mem_*`）；日后再加 DMA 时勿占用 I2C1 已用的 DMA1 Stream0/6 |
| NVIC | 若只做轮询可先关 Ev/Er；若开中断，优先级放在 FreeRTOS 允许的 **5–15** |

> 对照：本仓 I2C1 已是 Fast + DMA（舵机）。**IST8310 必须用 I2C3**，不要接到 I2C1。

### 2.2 GPIO：复位 / 数据就绪

| 网络名（建议 User Label） | 引脚 | 方向 / 模式 | 默认电平 | 说明 |
|---|---|---|---|---|
| `RSTN_IST8310` | **PG6** | Output Push-Pull | **HIGH** | 低有效复位；空闲必须拉高 |
| `DRDY_IST8310` | **PG3** | Input（可先无上下拉，按原理图） | — | 数据就绪；**v1 轮询可读寄存器，可不配 EXTI** |

可选（以后再说）：

- PG3 → EXTI falling edge，在中断里置标志，磁力计任务唤醒读 XYZ。  
- v1：**轮询即可**，先把 I2C 通了再加 EXTI。

### 2.3 复位时序（驱动里实现时用；Cube 侧只需默认 HIGH）

典型：`RSTN=0` 保持 ≥50 ms → `RSTN=1` → 再延时数十 ms → `IsDeviceReady` / 读 WHO_AM_I。  
CubeMX 里把 PG6 默认输出高即可，上电后由驱动做一次脉冲复位。

### 2.4 I2C 从机地址（务必分清 7-bit / HAL）

| 说法 | 值 | 用在哪里 |
|---|---|---|
| **7-bit 地址** | **`0x0E`** | 数据手册 / 逻辑分析仪地址帧 |
| **HAL `DevAddress`（左移 1 位）** | **`0x1C`**（即 `0x0E << 1`） | `HAL_I2C_Mem_Read/Write`、`HAL_I2C_IsDeviceReady` 等 |

说明：STM32 HAL 的 `DevAddress` 参数是 **7-bit 地址左移 1 位后的 8-bit 形式**（写方向位由 HAL 再处理）。  
文档/注释里请同时写：`IST8310_I2C_ADDR_7BIT 0x0E` 与 `IST8310_I2C_ADDR_HAL (0x0E << 1) /* 0x1C */`，避免和舵机侧 `0x40`/`0x80` 混用习惯搞反。

WHO_AM_I 等寄存器值以 IST8310 手册为准（驱动实现时再锁进 `ist8310_reg.h`）。

### 2.5 Generate Code

1. Project Manager → Code Generator：**Keep User Code when re-generating**（或等价 “备份用户代码”）勾上。  
2. Generate Code。  
3. 编译确认无回归（现有 USART / I2C1 / SPI1 手改文件仍在）。  
4. **提交**更新后的：
   - `carry_bot_v1.ioc`
   - `Core/Src/i2c.c` / `Core/Inc/i2c.h`（应出现 `hi2c3`）
   - `Core/Src/gpio.c`、`Core/Inc/main.h`（`RSTN_IST8310` / `DRDY_IST8310` 宏）
   - 如有变动的 `stm32f4xx_hal_msp.c`、`.mxproject` 等一并纳入  
5. 告诉协作方「CubeMX 已合入」，再在本分支写驱动。

## 3. 本仓落地路径（对齐现有 QGimbal 风格，勿另起炉灶）

已有布局（见 `docs/BOARD_IO.md`）：

```text
BSP/Board/          LED / KEY / CS GPIO
UserLib/BMI088/     SPI 传感器驱动
Applications/       boardTask / imuTask
```

IST8310 建议（**实现阶段**再创建，本 PR 不建空目录刷屏）：

| 层级 | 建议路径 | 职责 |
|---|---|---|
| 板级脚 | `BSP/Board/` 增补或薄封装 `board_ist8310_gpio.*`（可选） | RSTN 脉冲、读 DRDY |
| 器件驱动 | **`UserLib/IST8310/`**（对标 `UserLib/BMI088`） | 寄存器、单次/连续测、XYZ 换算 |
| 任务 | `Applications/` → 如 `mag_task.*` | 周期读数；可与现有 SMOKE UART 拼一行 |

CMake：按 BMI088 同样方式在 `UserLib/IST8310/CMakeLists.txt` + 上层 `add_subdirectory`；**不要**塞进 `app/`（那是 Link/ZDT/Servo 协议层风格）。

调试/烧录：本分支已带自 bsp 的 `openocd.cfg` + `.vscode`（ELF 固定为 `carry_bot_v1.elf`），worktree 目录名不影响。

## 4. 验收（CubeMX 完成后、驱动前）

- [ ] `.ioc` 可见 I2C3 @ PA8/PC9 Fast Mode  
- [ ] `hi2c3` 已生成；PG6 默认高、PG3 为输入  
- [ ] Debug 构建通过；原有 BMI088 SMOKE / LED/KEY 行为未坏  
- [ ] （可选）用 `HAL_I2C_IsDeviceReady(&hi2c3, 0x1C, …)` 在临时 USER CODE 里探一下，应返回 `HAL_OK`

## 5. 非目标（本分支当前）

- 不实现完整 IST8310 驱动 / magTask  
- 不改 I2C1 / PCA9685 / USART DMA 分配  
- 不把 mag 协议接到 Bridge（以后另开任务）

---

**你现在需要做的：** 按 §2 改 CubeMX → Generate → 编译 → 把生成文件 commit 到 `feature/ist8310-mag`（或开 follow-up commit）。完成后喊一声即可开始写 `UserLib/IST8310`。
