# carry_bot_v1 全分支审计摘要

> 审计对象：`vesperadev/carry_bot_v1`（21 个分支 / 71 个提交）
> 镜像目标：`jiangnanlier/carry_bot_v1`
> 方式：逐分支读码 + 本机真实构建（arm-none-eabi-gcc 14.3.1 + CMake 4.4 + Ninja）+ `git merge-tree` 冲突实测
> 详细逐分支审计：[docs/BRANCH_AUDIT.md](docs/BRANCH_AUDIT.md)

---

## 一、结论先行

**这不是「一个项目的 21 个分支」，而是两条互不相交的技术主线，外加一批一次性诊断分支。**

| 主线 | 最高点分支 | 目录 | 能否构建 | 覆盖能力 |
|---|---|---|---|---|
| **执行线** | `cursor/absolute-motor-goto-e246` | `app/Link` + `app/ZDT` + `app/Bridge` + `app/Servo` | ✅ 链接通过 | 蓝牙文本命令 → Emm V5 六路电机 + 绝对定位闭环 + PCA9685 夹爪/转盘 |
| **感知线** | `cursor/gyro-mag-ahrs-polish-0c06` | `app/BT` + `UserLib/{BMI088,IST8310,AHRS}` + `BSP/Board` + `Applications` | ✅ 链接通过 | BMI088 六轴 + IST8310 磁力计 + Mahony 姿态 → 串口 SMOKE/Euler |

两条线**从未合并过**：`git merge-tree` 实测 4 处硬冲突，另有一处**原理图级的 DMA 通道冲突**（见第四节）。这是本次审计最重要的发现。

其余 15 个分支基本是这两条线在某一时刻的快照、或一次性硬件诊断（串口 hello、嗅探转发、OpenOCD 配置）。

---

## 二、分支全景

改动规模 = `git diff --shortstat main <branch>`。

### 2.1 基线

| 分支 | 独有提交 | 改动 | 内容 | 评价 |
|---|---|---|---|---|
| `main` | 0 | — | 仅 `app/BT`：USART6 DMA+IDLE 收帧、队列、任务、BT 原样回显、RPI CRC8 帧骨架；含 CI 与 .gitignore 扩容 | 干净的起点，无业务 |
| `chore/repo-hygiene-ci` | 1 | 0 | tree 与 main 完全一致（已由 PR #1 并入） | **可删**，无内容 |

### 2.2 执行线（电机 / 舵机 / 桥接）

| 分支 | 独有提交 | 改动 | 内容 | 评价 |
|---|---|---|---|---|
| `feature/zdt-motor-bus` | 8 | +1141/−211 | Emm V5 帧编码/解析骨架，USART1 DMA，地址槽 | 打地基，无命令层 |
| `feature/bridge-bt-commands` | 14 | +2296/−213 | Bridge A 档文本命令 `M?/ME/MG/MX/MS/ML/MJ` + `S?/S0U/S1U/S0C/S0O/S1M` | 首个可用命令集 |
| `feature/bridge-test-fwd` | 16 | +2563/−213 | 加 `test`：四轮 60 rpm 前进 2 s；逐轮打印 `E/V/X/D a=N ACK rx=hex` 调试行 | 首次端到端跑通电机 |
| `feature/zdt-wheel-telem` | 17 | +2746/−213 | `LinkProtocol` 新增 `wants_idle_tick()/on_idle()`；1 Hz 轮流查四轮 0x36；`telem/telem0/W?` | 主动上报雏形 |
| `feature/uart2-motor-dump` | 23 | +2879/−219 | 串口2 原始 dump；USART1 TX 由 PB6 改 **PA9**；含完整 781 行 Bridge | ABS 的祖先 |
| `cursor/absolute-motor-goto-e246` | **32** | +3633/−219 | `MA` 绝对定位（0x36 起读 + 最多 4 趟闭环 + 0x3A 稳定判定）、`MZ` 零位、`MR` 原始寄存器 dump、`ver`、`test1m`、方向符号表、软限位 | **执行线最高点** |
| `cursor/bridge-motor-optimize-b5d6` | 25 | +2963/−220 | 从 `903b6c6` 分出、与 ABS **平行**：`test1m` 改整数几何（规避 nano.specs 无 %f）、`BRIDGE_MG_MAX_DEG=3600` 单次角度上限、统一 I/O 标签 | 只差 2 个提交，与 ABS 互不包含 |
| `feature/servo-pca9685` | 9 | +1191/−299 | PCA9685 驱动（DMA + mutex + 完成信号量）；**I2C1 PB8/PB9 → I2C2 PF0/PF1**，DMA1 Stream2/CH3 + Stream7/CH7 | 与 ABS 的 I2C1 版本**不兼容**，需 cherry-pick |

### 2.3 感知线（IMU / 磁力计 / 姿态）

| 分支 | 独有提交 | 改动 | 内容 | 评价 |
|---|---|---|---|---|
| `feature/bsp-board-io` | 5 | +1278/−16 | `BSP/Board`（LED PH12/PH11/PH10、KEY PA0）+ `UserLib/BMI088`（SPI1 轮询，CS PA4/PB0）+ `Applications` boardTask/imuTask + 1 Hz SMOKE | 感知线地基 |
| `feature/ist8310-mag` | 8 | +1879/−29 | IST8310 走 I2C3（PA8/PC9）、7bit 0x0E、WHO_AM_I 0x10、RSTN PG6 / DRDY PG3 软 GPIO、200 Hz 连续模式 | 9 轴的第一半 |
| `feature/wheel-ahrs` | 11 | +2486/−31 | Mahony 6/9 轴融合，输出 roll/pitch/yaw，开机 gyro bias 标定，`gcal` 命令 | 姿态闭环 |
| `cursor/gyro-mag-ahrs-polish-0c06` | 14 | +2663/−31 | 3 个加固提交：鲁棒 gyro bias（warmup + trimmed mean）、IST8310 DRDY 等待 + mag_ready 连续计数、磁融合门控（20–120 µT 才走 9 轴） | **感知线最高点** |
| `feature/debug-openocd` | 2 | +98 | `openocd.cfg`（CMSIS-DAP / 1000 kHz / `reset_config none`）+ `.vscode/launch.json` + `tasks.json` | 有用，但路径硬编码 |
| `feature/imu-bmi088` | 2 | +98 | tree 与 debug-openocd **完全相同**（同内容不同 sha） | **可删** |

### 2.4 诊断 / 占位分支

| 分支 | 独有提交 | 改动 | 真实内容 | 评价 |
|---|---|---|---|---|
| `feature/usart1-sniff-fwd` | 18 | +2246/−213 | `Zdt_SetRxHook()` 挂 USART1 原始字节钩子，原样转发到 USART6；`fwd/fwd0/fwd?` | 排线神器，可保留 |
| `feature/usart1-hello-tx` | 20 | +2277/−229 | 删光所有模块任务，只剩 defaultTask：PA9@115200 发 `hello`、PG14@9600 发 `hello1`、PH10 心跳、失败发 `fail` | 一次性验证，别合 |
| `feature/mecanum-kinematics` | 7 | +463/−142 | **没有任何运动学代码**：只有 `app/BT→app/Link` 改名 + OpenOCD + 文档 | 名不副实 |
| `feature/usart-module-port` | 7 | +463/−142 | tree 与 mecanum-kinematics **完全相同**，同样只有改名 + OpenOCD | 名不副实 |
| `docs/dev-handbook` | 5 | +365/−142 | `README.md`（项目状态/进度表）+ `docs/DEVELOPMENT.md`（项目地图、架构目标、6 个里程碑、协作分工、已知风险） | **文档质量高，应保留** |

---

## 三、推荐合并路线

目标：一条能同时跑「蓝牙命令 → 电机 + 舵机」和「姿态解算」的主线。

1. **立基线**：从 `cursor/absolute-motor-goto-e246` 切 `integration`。它已经包含 Link/ZDT/Bridge/Servo + `ver/test1m/MA/MZ`，是执行线最全的一支。
2. **补舵机迁移**：cherry-pick `55218f6`（PCA9685 迁 I2C2/PF0/PF1）。ABS 里舵机还在 I2C1/PB8/PB9，而 PB8/PB9 要留给相机 FPC。
3. **收编优化提交**：cherry-pick `edf087b`（整数几何 + `MG` 上限）。它会与 ABS 的 float 版 `test1m` 冲突，以整数版为准——nano.specs 下 `%f` 会让 Bridge 任务直接卡死。
4. **合并感知线**（唯一需要手写的部分）：
   - 删 `app/BT`，统一用 `app/Link`；`BT_Send()` → `Link_Send()`；`BT_`* 宏 → `LINK_`*。
   - **删掉 `app/BT/bt.cpp` 里的 `HAL_UARTEx_RxEventCallback` / `HAL_UART_ErrorCallback` 强定义**，只保留 `app/ZDT/zdt.cpp` 里按 `Instance` 分发的唯一副本，把 USART6 分支转调 `Link_UART_RxEventCallback`。
   - 修 `Core/Src/freertos.c` 任务表（见第五节预算）。
   - 保留 `imu_task` 的 SMOKE 输出，但改走 Link 的发送队列，别再抢 USART6。
5. **统一 DMA 分配**（硬冲突，必须先解决）：IST8310 改**阻塞读**、释放 I2C3 的 DMA，把 `DMA1_Stream2/CH3` 让给 PCA9685 的 `I2C2_RX`。IST8310 是 200 Hz 读 6 字节，400 kHz 下阻塞耗时 <100 µs，不构成瓶颈。
6. **同步 .ioc**：USART1 TX 定 PA9（代码已经改了，.ioc 还写着 PB6，用 CubeMX 重新生成会把引脚打回 PB6）；I2C2、I2C3、SPI1 一并回写进 .ioc。
7. **收尾**：把 `docs/DEVELOPMENT.md` 与 `README.md` 从 `docs/dev-handbook` 拿过来，按合并后的真实状态改写「待办」与「已知风险」。

---

## 四、必读的 8 个坑

1. **两条主线从没合过**。`git merge-tree --write-tree` 实测冲突 4 处：`CMakeLists.txt`、`Core/Src/freertos.c`（任务表）、`app/BT/CMakeLists.txt`（modify/delete）、`app/Link/link_proto_debug.cpp`。
2. **`DMA1_Stream2/CH3` 被两边同时占用**：PCA9685 的 `I2C2_RX` 和 IST8310 的 `I2C3_RX`。这是硬件级冲突，不是文本冲突，git 合并不报错但上板必坏。
3. **`HAL_UARTEx_RxEventCallback` 全工程只能有一份强定义**。执行线已在 `zdt.cpp` 按 `Instance` 分发解决；感知线仍在 `bt.cpp` 直接定义。合并必须先删后者。
4. **`LinkProtocol` 接口已分叉**：`feature/zdt-wheel-telem` 加了 `wants_idle_tick()` / `on_idle()` 两个虚函数，ABS 版没有。任一方向合并都要先统一虚表，否则 `override` 编不过。
5. **nano.specs 没有浮点 printf**。任何 `%f` 都会让调用任务卡死（不是打印错误，是 hang）。`test1m` 的 float 几何只用于计算、打印时强转整数；优化分支干脆全改整数。
6. **ZDT 位置单位是 65536/圈，不是 3200**（`7b6b2cc` 修过一次 20 倍偏差）。另外 **0xFD 的方向位与 0x36 回读的编码符号相反**（`111e8f0`），所以 `zdt_dir_map.h` 里要区分 `Zdt_ApplyDirBit()`（命令侧）与 `Zdt_PosWireDirBit()`（回读侧）。
7. **软限位默认是关的**（`f0de2bc`）。`zdt_limits.h` 只给 lift/turn 填了占位区间，轮子永远不夹。别把它当安全网。
8. **`.vscode` 与文档里的工具链路径全是 `C:/Users/37963/...`**，换机器即失效；`Core/Inc/Backup/*.bak`、`Core/Src/Backup/*.bak` 仍被 git 跟踪（.gitignore 后来加了 `*.bak` 但历史文件没清）。

---

## 五、构建实测与资源预算

本机 `arm-none-eabi-gcc 14.3.1` + `CMake 4.4` + `Ninja`，`cmake --preset Debug` 全量构建：

| 分支 | 目标数 | RAM | FLASH | 结果 |
|---|---|---|---|---|
| `main` | 44 | 22 784 B（17.38%） | 36 860 B（3.52%） | 零错误 |
| `cursor/absolute-motor-goto-e246` | 49 | 24 072 B（18.37%） | 66 236 B（6.32%） | 零错误 |
| `cursor/gyro-mag-ahrs-polish-0c06` | 61 | 23 792 B（18.15%） | 61 276 B（5.84%） | 零错误 |

合并后 FreeRTOS 堆预算（`configTOTAL_HEAP_SIZE = 15360`，heap_4）：

| 项 | 字节 |
|---|---|
| link 任务栈 2048 + zdt 1024 + servo 1024 | 4 096 |
| board 1024 + imu 2048 + default 512 | 3 584 |
| 6 个 TCB（约 88 B）+ link 队列 4×262 + zdt 队列 8×68 + servo 队列 8×8 | ≈ 2 300 |
| **合计** | **≈ 10 KB / 15 KB** |

理论装得下，余量不足 5 KB，任何一处栈峰值超出就是随机死机。**建议合并时把堆抬到 20480**，并在集成后逐个任务读 `uxTaskGetStackHighWaterMark()` 再收紧。

---

## 六、清理建议

- 直接删：`chore/repo-hygiene-ci`（与 main 同 tree）、`feature/imu-bmi088`（与 debug-openocd 同 tree）。
- 归档后删：`feature/usart1-hello-tx`（一次性诊断）、`feature/mecanum-kinematics` 与 `feature/usart-module-port`（无实际内容）。
- 保留：`feature/usart1-sniff-fwd`（串口排线时真有用）、`docs/dev-handbook`（文档资产）。
- 待合并后删：其余 feature 分支在进入 `integration` 后即可删除，只留最高点两支作历史。

---

## 七、命令速查（执行线最高点，通过 USART6 蓝牙，CR/LF 结尾）

```text
ver                                 → OK VER <branch> <hash> <desc>
M?                                  → 命令帮助
M1E1 / M2E0                         → 使能 / 失能（1=升杆 2=转杆 3=FL 4=RL 5=FR 6=RR）
M3G+90                              → 相对 +90°（先过软限位再发帧，回复带 lim=）
M2A+90                              → 绝对定位（读 0x36 → 多趟逼近 → 0x3A 判定到位）
M2Z                                 → 零位（0x0A），广播地址被拒
M3R                                 → dump 0x36/0x1F/0x3A 原始回复
M3X                                 → 立即停止
MS5                                 → 把默认地址设为 5
ML / MJ3                            → 列全部槽 / 单槽详情
S?                                  → 舵机帮助
S0C / S0O                           → 夹爪 1000 µs 合 / 1500 µs 开
S1M                                 → 转盘回中 1500 µs
test                                → 四轮 60 rpm 前进 2 s（逐轮打印 ACK）
test1m                              → 四轮前进约 1 m（几何参数见 bridge_config.h）
```

> 两字母 `M` 开头（B/C 档）目前统一返回 `ERR not implemented (B-tier)`，是预留位不是 bug。
