# DJI 开发板 C 型 硬件适配说明

> 适用对象：`carry_bot_v1`（STM32F407IGH6 / UFBGA176 / HSE 12 MHz）
> 事实来源：DJI《RoboMaster 开发板 C 型用户手册》(中/英) + 官方例程仓库 `RoboMaster/Development-Board-C-Examples`（21 个例程）
> 校验方式：本机 `arm-none-eabi-gcc 14.3.1` + CMake 4.4 + Ninja，`cmake --preset Debug` 全量构建

---

## 1. 结论速览

工程原本就是按 C 板（`STM32F407IGH6 / UFBGA176`）建的，引脚资源不存在"端口缺失"问题；
真正的问题集中在 **时钟源、USART1 引脚归属、片上外设没被启用** 三处，前两处已在本仓库修正。

| 项目 | 手册/官方例程 | 修正前仓库 | 修正后 |
|---|---|---|---|
| 系统时钟源 | HSE **12 MHz**，PLLM=6 / PLLN=168 / PLLP=2 / PLLQ=7 | HSI 16 MHz，PLLM=8 / PLLQ=4（HSE 从未启用） | ✅ 改 HSE 12 MHz |
| `HSE_VALUE` | `12000000U` | `25000000U`（死配置） | ✅ 改 12 MHz |
| USART1 | **PA9**(TX) + PB7(RX)，AF7 | PB6(TX) + PB7(RX)（PB6 实际是 CAN2_TX） | ✅ 改 PA9 |
| LED | PH10 蓝 / PH11 绿 / PH12 红，**高电平点亮** | 引脚宏与初始化都缺失 | ✅ 已补齐 |
| KEY | PA0，上拉，按下为低 | 缺失 | ✅ 已补齐 |
| 编译 | — | 36860 B FLASH | 37064 B FLASH（+204 B）/ RAM 22784 B 不变 |

---

## 2. C 板板载网络与引脚（手册第 17~18 页对照表）

| 网络名 | 引脚 | 网络名 | 引脚 |
|---|---|---|---|
| LED_R | PH12 | UART3_RX (DBUS) | PC11 |
| LED_G | PH11 | I2C1_SCL / SDA | PB8 / PB9 |
| LED_B | PH10 | PCLK_OUT | PA6 |
| TIM3_CH3 (激光) | PC8 | DCMI_HREF / VSYNC | PH8 / PI5 |
| USB_DM / DP | PA11 / PA12 | DCMI_D0..D7 | PH9, PC7, PE0, PE1, PE4, PI4, PE5, PE6 |
| KEY | PA0 | TIM4_CH3 (蜂鸣器) | PD14 |
| I2C2_SCL / SDA | PF1 / PF0 | ADC_BAT | PF10 (ADC3_IN8) |
| SPI2_CS / CLK / MISO / MOSI | PB12 / PB13 / PB14 / PB15 | TIM10_CH1 (IMU 加热) | PF6 |
| **UART6_TX / RX** | **PG14 / PG9** | INT1_Accel / INT1_Gyro | PC4 / PC5 |
| **UART1_TX / RX** | **PA9 / PB7** | CS1_Accel / CS1_Gyro | PA4 / PB0 |
| CAN1_TX / RX | PD1 / PD0 | SPI1_CLK / MOSI / MISO | PB3 / PA7 / PB4 |
| CAN2_TX / RX | PB6 / PB5 | RSTN / DRDY_IST8310 | PG6 / PG3 |
| TIM1_CH1..CH4 | PE9 / PE11 / PE13 / PE14 | I2C3_SCL / SDA | PA8 / PC9 |
| TIM8_CH1..CH3 | PC6 / PI6 / PI7 | | |

**关键点：`PB6` 在 C 板上是 `CAN2_TX`，不是 USART1_TX。** 用 PB6 发 UART 波形会被 CAN 收发器吃掉。

---

## 3. 已实施的修正（本次改动）

### 3.1 时钟：HSI → HSE 12 MHz

`Core/Src/main.c` `SystemClock_Config()`：

```c
/* DJI RoboMaster Development Board Type-C: 12 MHz HSE crystal on PH0/PH1. */
RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
RCC_OscInitStruct.HSEState       = RCC_HSE_ON;
RCC_OscInitStruct.PLL.PLLState   = RCC_PLL_ON;
RCC_OscInitStruct.PLL.PLLSource  = RCC_PLLSOURCE_HSE;
RCC_OscInitStruct.PLL.PLLM = 6;    /* 12 MHz / 6 = 2 MHz VCO in */
RCC_OscInitStruct.PLL.PLLN = 168;  /* x168 = 336 MHz VCO       */
RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;  /* = 168 MHz SYSCLK */
RCC_OscInitStruct.PLL.PLLQ = 7;    /* = 48 MHz (USB/SDIO)      */
```

`Core/Inc/stm32f4xx_hal_conf.h`：

```c
#define HSE_VALUE    12000000U  /* DJI Type-C board crystal = 12 MHz */
```

为什么必须改：原来 PLL 跑 HSI，HSE 完全没启用。**即使晶振坏了也照样跑 168 MHz**，
但 CAN/ USB / SDIO 这些依赖 HSE 精度的外设会全部不准。改完后如果 HSE 起振失败，
`HAL_RCC_OscConfig` 会进 `Error_Handler()`，故障立刻暴露——这是想要的行为。

`carry_bot_v1.ioc` 同步：`RCC.HSE_VALUE=12000000`、`RCC.PLLM=6`、`RCC.PLLQ=7`、
`RCC.PLLSourceVirtual=RCC_PLLSOURCE_HSE`、`RCC.48MHZClocksFreq_Value=48000000`、
`RCC.PLLQCLKFreq_Value=48000000`、`RCC.RTCHSEDivFreq_Value=6000000`。

### 3.2 USART1：PB6 → PA9

`Core/Src/usart.c` `HAL_UART_MspInit()`：

```c
__HAL_RCC_GPIOB_CLK_ENABLE();
__HAL_RCC_GPIOA_CLK_ENABLE();
/* PA9 = USART1_TX (4-pin UART port), PB7 = USART1_RX; PB6 is CAN2_TX here */
GPIO_InitStruct.Pin       = GPIO_PIN_7;
GPIO_InitStruct.Mode      = GPIO_MODE_AF_PP;
GPIO_InitStruct.Pull      = GPIO_NOPULL;
GPIO_InitStruct.Speed     = GPIO_SPEED_FREQ_VERY_HIGH;
GPIO_InitStruct.Alternate = GPIO_AF7_USART1;
HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

GPIO_InitStruct.Pin       = GPIO_PIN_9;
HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
```

`HAL_UART_MspDeInit()` 同步改成 `HAL_GPIO_DeInit(GPIOA, GPIO_PIN_9)` + `HAL_GPIO_DeInit(GPIOB, GPIO_PIN_7)`。

### 3.3 LED / KEY 引脚初始化

`Core/Inc/main.h` 新增引脚宏（原先 Private defines 段是空的）：

```c
#define LED_R_Pin GPIO_PIN_12
#define LED_R_GPIO_Port GPIOH
#define LED_G_Pin GPIO_PIN_11
#define LED_G_GPIO_Port GPIOH
#define LED_B_Pin GPIO_PIN_10
#define LED_B_GPIO_Port GPIOH
#define KEY_Pin GPIO_PIN_0
#define KEY_GPIO_Port GPIOA
```

`Core/Src/gpio.c` `MX_GPIO_Init()` 新增实际配置（原来是空函数，只开时钟）：

```c
/* 先压成 LOW（=灭）再切输出，避免使能输出瞬间闪一下 */
HAL_GPIO_WritePin(GPIOH, LED_R_Pin|LED_G_Pin|LED_B_Pin, GPIO_PIN_RESET);

GPIO_InitStruct.Pin   = LED_R_Pin|LED_G_Pin|LED_B_Pin;
GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
GPIO_InitStruct.Pull  = GPIO_NOPULL;
GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
HAL_GPIO_Init(GPIOH, &GPIO_InitStruct);

GPIO_InitStruct.Pin  = KEY_Pin;
GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
GPIO_InitStruct.Pull = GPIO_PULLUP;
HAL_GPIO_Init(KEY_GPIO_Port, &GPIO_InitStruct);
```

`carry_bot_v1.ioc` 同步补 `PH10/PH11/PH12 = GPIO_Output`、`PA0-WKUP = GPIO_Input`（`GPIO_PULLUP`），
`Mcu.Pin6` 由 `PB6` 改为 `PA9`，`Mcu.PinsNb` 12 → 16。

---

## 4. LED 极性：高电平点亮（含证据）

这一条在工程里被反复搞错，写清楚：

| 证据 | 内容 |
|---|---|
| 英文用户手册 p.9 | "When the output level of the IO port is **high**, the corresponding status LED will be on." |
| 中文用户手册 | "当 IO 口输出**高电平**时，对应的 LED 指示灯点亮" |
| 官方 `1.light_led/Src/main.c` | 循环里三行 `HAL_GPIO_WritePin(LED_x, **GPIO_PIN_SET**)` 以点亮 |
| 官方 `6.key_exit/Src/main.c` | 注释 `//set led dark` 下写 `**GPIO_PIN_RESET**`；按下 KEY 点亮写 `GPIO_PIN_SET` |
| 官方 `15.freeRTOS_LED/*_led_task.c` | 亮 = `SET`，灭 = `RESET` |
| 官方 `.ioc` | `PH10/11/12 PinState=GPIO_PIN_SET`（复位后即点亮） |

所以：**`SET` = 亮，`RESET` = 灭**。`ahrs` 分支 `BSP/Board/board_led.h` 注释的 "IO high = LED on" 是对的，不需要反转。
注意手册那句 "common anode" 与 "high = on" 在电路上矛盾（共阳应当低电平点亮），属官方笔误；
三份例程代码 + `.ioc` 的 `PinState` 才是可执行的事实。

想在复位后保持熄灭，就要像本仓库这样做一次 `GPIO_PIN_RESET` 预置（CubeMX 默认是 SET）。

---

## 5. 三色 LED 亮度调节（可选，TIM5 PWM）

只用 GPIO 只能亮/灭。要呼吸灯或调光，用 TIM5 三个通道（官方 `4.PWM_light`）：

| 通道 | 引脚 | 颜色 |
|---|---|---|
| TIM5_CH1 | PH10 | 蓝 |
| TIM5_CH2 | PH11 | 绿 |
| TIM5_CH3 | PH12 | 红 |

配置（`MX_TIM5_Init`）：`Prescaler=0`、`Period=65535`、`OCMode=TIM_OCMODE_PWM1`、`OCPolarity=TIM_OCPOLARITY_HIGH`；
APB1 定时器时钟 84 MHz → PWM 载频 ≈ **1281 Hz**（无可见闪烁）。
GPIO 需切 `GPIO_MODE_AF_PP` + `GPIO_AF2_TIM5`（**不是**普通输出），并 `HAL_TIM_PWM_Start` 三个通道。

官方封装函数（`4.PWM_light/bsp/boards/bsp_led.c`）可直接抄：

```c
void aRGB_led_show(uint32_t aRGB)   /* 0xaaRRGGBB */
{
    uint8_t  alpha = (aRGB & 0xFF000000) >> 24;
    uint16_t red   = ((aRGB & 0x00FF0000) >> 16) * alpha;
    uint16_t green = ((aRGB & 0x0000FF00) >>  8) * alpha;
    uint16_t blue  = ((aRGB & 0x000000FF)      ) * alpha;
    __HAL_TIM_SetCompare(&htim5, TIM_CHANNEL_1, blue);
    __HAL_TIM_SetCompare(&htim5, TIM_CHANNEL_2, green);
    __HAL_TIM_SetCompare(&htim5, TIM_CHANNEL_3, red);
}
```

注意 `alpha` 乘法用 8 位 × 8 位溢出到 16 位没问题，但 `alpha` 为 255 时 `red` 最大 65535，正好对上 `Period`。

---

## 6. 串口接线与丝印陷阱

手册明确写了两条容易踩的：

1. **丝印与 MCU 串口不对应**：外壳丝印 `UART1` → STM32 的 **USART6**；丝印 `UART2` → STM32 的 **USART1**。
2. **UART6 与裁判系统电源管理模块线序相同**，互连时 TX/RX 必须交叉。

| 口 | MCU | 引脚 | 针序 |
|---|---|---|---|
| 4-pin UART（丝印 UART2） | USART1 | PA9 / PB7 | 1=RXD 2=TXD 3=GND 4=5V |
| 3-pin UART（丝印 UART1） | USART6 | PG14 / PG9 | 1=GND 2=TXD 3=RXD |

本仓库当前配置：USART6 = **9600 8N1**（对蓝牙/上位机），USART1 = **115200 8N1**（对 Emm V5 电机总线）。
两个波特率都可在 `Core/Src/usart.c` 的 `MX_USARTx_UART_Init()` 里改，`app/Link`、`app/ZDT` 也会在运行时校正。

---

## 7. DMA 分配总表（F407 全片 16 条流）

申请外设前先看这张表，**同一条流不能被两个外设同时占用**，而 CubeMX 只会对部分组合报错。

| 流 / 通道 | 当前用途 | 来源 |
|---|---|---|
| DMA1_Stream0 / CH1 | I2C1_RX | `Core/Src/i2c.c` |
| DMA1_Stream6 / CH1 | I2C1_TX | `Core/Src/i2c.c` |
| DMA1_Stream2 / CH3 | I2C3_RX（ahrs 分支） | `ahrs:Core/Src/i2c.c` |
| DMA1_Stream4 / CH3 | I2C3_TX（ahrs 分支） | `ahrs:Core/Src/i2c.c` |
| DMA2_Stream2 / CH4 | USART1_RX | `Core/Src/usart.c` |
| DMA2_Stream7 / CH4 | USART1_TX | `Core/Src/usart.c` |
| DMA2_Stream1 / CH5 | USART6_RX | `Core/Src/usart.c` |
| DMA2_Stream6 / CH5 | USART6_TX | `Core/Src/usart.c` |
| DMA2_Stream2 / CH3 | SPI1_RX（官方例程预留） | 官方 `13.spi_bmi088` |
| DMA2_Stream3 / CH3 | SPI1_TX（官方例程预留） | 官方 `13.spi_bmi088` |

### ⚠️ 已知冲突：`DMA1_Stream2 / CH3`

- `feature/servo-pca9685` 分支：PCA9685 挂在 **I2C2（PF0/PF1）**，`I2C2_RX = DMA1_Stream2 / CH3`
- `cursor/gyro-mag-ahrs-polish-0c06` 分支：IST8310 挂在 **I2C3（PA8/PC9）**，`I2C3_RX = DMA1_Stream2 / CH3`

两条线合并时 git **不会报冲突**（改的是不同的 `i2c.c` 段落），但上板必坏。
推荐做法：IST8310 本来就是 `HAL_I2C_Mem_Read()` 阻塞读（`UserLib/IST8310/ist8310.c`），
直接把 I2C3 的 DMA 去掉、把 `DMA1_Stream2` 留给 PCA9685。

---

## 8. 其它可用但本仓库尚未使用的外设

| 资源 | 引脚 | 官方参考 | 备注 |
|---|---|---|---|
| 蜂鸣器 | PD14 = TIM4_CH3（AF2） | `17.chassis_task/Src/tim.c` | 需要 `HAL_TIM_PWM_Start` |
| IMU 加热电阻 | PF6 = TIM10_CH1 | `16.imu_temperature_control_task` | PID 稳到 40.0 °C，`Kp=1600 / Ki=0.2 / Kd=0`，输出上限 4500 |
| 电池电压 | PF10 = ADC3_IN8 | 官方手册 | 分压比见原理图 |
| 激光 | PC8 = TIM3_CH3 | 官方手册 | PWM 调亮度 |
| 双 CAN | PD0/PD1（CAN1）、PB5/PB6（CAN2） | `14.CAN` | 最高 1 Mbps，可直接控 RoboMaster 电调 |
| DBUS 遥控 | PC11 = USART3_RX | `9.remote_control_dma` | 100 kbps，DMA1_Stream1 CIRCULAR |
| 扩展 I2C2 | PF0/PF1 | 官方手册 | 8-pin 自定义 IO，可 IIC 或 SPI |
| 扩展 SPI2 | PB12..PB15 | 官方手册 | 同上 |
| USB | PA11/PA12 | 官方手册 | 全速 Device |
| 相机 FPC | DCMI 一组 + I2C1 | 官方手册 | 18-pin |

**BMI088 相关：** SPI1 = PB3(SCK) / PB4(MISO) / PA7(MOSI)，CS1_ACCEL = PA4，CS1_GYRO = PB0，都是软片选。
官方例程一律 `CPOL=HIGH / CPHA=2EDGE`（Mode 3）、`NSS=SOFT`、`BaudRatePrescaler=256`。
`ahrs` 分支的 `Core/Src/spi.c` 用的是 `PRESCALER_8`（84/8 = 10.5 MHz），**超出 BMI088 的 10 MHz 上限**，建议改成 `PRESCALER_32`（2.6 MHz）或直接对齐官方的 256。

---

## 9. FreeRTOS 堆预算

`configTOTAL_HEAP_SIZE = 15360`（15 KB，heap_4），两条线的任务栈加起来已接近上限：

| 来源 | 任务 / 对象 | 栈（字节） |
|---|---|---|
| 基线 | `defaultTask` | 128×4 = 512 |
| 基线 | `btTask` / `linkTask` | 256×4 / 512×4 = 1024 / 2048 |
| 执行线 | `zdtTask` | 256×4 = 1024 |
| 执行线 | `servoTask` | 256×4 = 1024 |
| 感知线 | `boardTask` | 256×4 = 1024 |
| 感知线 | `imuTask` | 512×4 = 2048 |
| FreeRTOS 内置 | Timer Task | 256 words ≈ 1024 |

栈合计已 ≈ 9.7 KB，再加 6~7 个 TCB（每个 ~88 B）、link 队列 4×(2+256)、zdt 队列 8×(2+64)、servo 队列 8×4、Timer 队列 10×16 —— **15 KB 基本见底**。

合并两条主线时建议：
1. `configTOTAL_HEAP_SIZE` 提到 **20480**（20 KB，RAM 128 KB 完全够）；
2. 保留 `configUSE_TRACE_FACILITY 1` 与 `INCLUDE_uxTaskGetStackHighWaterMark`，跑起来后逐个读水位再收紧；
3. 不要开 `configUSE_TIMERS`（当前 =1，白占 1 KB + 队列），除非真的要用 `osTimer`。

---

## 10. 验证清单（上板前逐条打勾）

- [ ] `cmake --preset Debug && cmake --build --preset Debug` 零错误零警告
- [ ] `arm-none-eabi-size build/Debug/carry_bot_v1.elf`：RAM ≈ 22.8 KB、FLASH ≈ 37 KB
- [ ] 上电后 3.3 V / 5 V 正常，PH0/PH1 有 12 MHz 波形（示波器或频率计）
- [ ] 若 HSE 起不来会停在 `Error_Handler()`（死循环）——这本身就是诊断信号
- [ ] PH10/PH11/PH12 上电默认熄灭（`MX_GPIO_Init` 预置 LOW）
- [ ] PA0 按下时读到低电平（`HAL_GPIO_ReadPin(KEY_GPIO_Port, KEY_Pin) == GPIO_PIN_RESET`）
- [ ] 4-pin UART（丝印 UART2）接 USB-TTL：TX=PA9、RX=PB7，115200 能收到 ZDT 总线回复
- [ ] 3-pin UART（丝印 UART1）接蓝牙：TX=PG14、RX=PG9，9600 能收到 `M?` 帮助文本
- [ ] `PB6` 不要接任何 UART 设备（它是 CAN2_TX）

---

## 11. 参考链接

- 产品页：https://www.robomaster.com/zh-CN/products/components/general/development-board-type-c
- 用户手册（中）：https://rm-static.djicdn.com/tem/35228/RoboMaster%20%20%E5%BC%80%E5%8F%91%E6%9D%BF%20C%20%E5%9E%8B%E7%94%A8%E6%88%B7%E6%89%8B%E5%86%8C.pdf
- 用户手册（英）：https://rm-static.djicdn.com/tem/35228/RoboMaster%20Development%20Board%20Type%20C%20User%20Manual.pdf
- 官方例程：https://github.com/RoboMaster/Development-Board-C-Examples
- STM32F407 参考手册 RM0090：https://www.st.com/resource/en/reference_manual/dm00031020-stm32f405-415-stm32f407-417-stm32f427-437-and-stm32f429-439-advanced-arm-based-32-bit-mcus-stmicroelectronics.pdf
