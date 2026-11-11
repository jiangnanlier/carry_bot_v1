# 分支逐条审计（docs/BRANCH_AUDIT.md）

> 审计方式：对 21 个分支逐个 `git log --no-merges` 取独有提交、`git diff --shortstat main <branch>` 取改动量、
> `git ls-tree -d` 比对模块目录、关键文件逐行读码，最后用本机 `arm-none-eabi-gcc 14.3.1` 真实构建三条主线。
> 所有结论都可复现，命令见文末。

---

## 0. 判定口径

| 标签 | 含义 |
|---|---|
| **主干** | 该线上能力最全、可直接作为后续开发基线 |
| **中间态** | 同一线里的历史快照，已被主干包含，无需单独维护 |
| **一次性** | 为验证某个硬件/接线而临时改的分支，不应合并 |
| **空壳** | 分支名承诺的功能在代码里不存在 |
| **重复** | tree 与另一分支完全相同 |

---

## 1. 基线层

### `main` — 主干（唯一基线）
- 独有提交：0，HEAD `9272aae`
- 目录：只有 `app/BT`
- 内容：USART6（PG14/PG9，9600）DMA + IDLE 分帧 → `LinkFrame` 队列 → `BT_Task`；协议层两个实现二选一（`BT_LINK_BT` 原样回显 / `BT_LINK_RPI` CRC8 帧骨架，只实现 PING）
- 附带的工程化：CI（hygiene + cmake-configure 两个 job）、`.gitignore` 扩容（build/、cmake-build-*、`*.elf/bin/hex/map/o`、`*.bak`…）、PR 模板
- 结论：**干净起点，无业务逻辑**。任何新特性都应从这里或从最高点分支切出。

### `chore/repo-hygiene-ci` — 重复（可删）
- 独有提交：1（`579b7a5` chore: expand gitignore, add PR template and CI skeleton）
- tree 与 `main` **完全一致**（`git diff main chore/repo-hygiene-ci` 为空）——它已经通过 PR #1 并入 main。
- 结论：**保留只会造成「哪个才是最新」的误判，可删**。

---

## 2. 执行线（Link / ZDT / Bridge / Servo）

链路：`zdt-motor-bus` → `bridge-bt-commands` → `bridge-test-fwd` → `zdt-wheel-telem`／`uart2-motor-dump`／`usart1-sniff-fwd` → `usart1-hello-tx`；
另有一条平行支 `cursor/bridge-motor-optimize-b5d6`，和这条链的最高点 `cursor/absolute-motor-goto-e246` 互不包含。

### `feature/zdt-motor-bus` — 中间态
- 8 提交 / 25 文件 / +1141 −211
- `app/ZDT`：Emm V5 帧组装（0xF3 使能、0xF6 速度、0xFD 位置、0xFE 停、0x0A 零位、0x36/0x35/0x3A 读）、TAIL 0x6B、65536 计数/圈、地址槽、USART1 DMA 收发、`Zdt_Task` 命令队列
- 缺：文本命令层、绝对定位、方向映射
- 结论：地基，已被后续全部执行线分支包含。

### `feature/bridge-bt-commands` — 中间态
- 14 提交 / 35 文件 / +2296 −213
- 新增 `app/Servo`（PCA9685 @I2C1 PB8/PB9）+ `app/Bridge`：`BridgeProtocol final : LinkProtocol`，按 CR/LF 切行（上限 96），首字母分派 `M`/`S`
- A 档命令：`M?` `ME0/1` `MG` `MX` `MS` `ML` `MJ`；`S?` `S0U` `S1U` `S0C` `S0O` `S1M`
- 结论：第一个「人能用的命令集」，仍无绝对定位。

### `feature/bridge-test-fwd` — 中间态
- 16 提交 / 35 文件 / +2563 −213
- 加 `test`：四轮 `BRIDGE_TEST_VEL_RPM=60` 前进 2 s，逐轮打印 `E a=N ACK rx=<hex>` / `V a=N ACK rx=<hex>` / `X a=N ACK` / `D a=N ACK`
- 注意：这些 echo 行会把 USART6 9600 bps 塞满，实测需配 `flush_reply()` 的 `vTaskDelay(40+len/2)` 才不丢字
- 结论：首次端到端跑通电机，调试价值大于功能价值。

### `feature/zdt-wheel-telem` — 中间态（但有接口增量）
- 17 提交 / 35 文件 / +2746 −213
- `LinkProtocol` 增加 `virtual bool wants_idle_tick()` + `virtual void on_idle()`，Base 任务每轮调用
- `telem` / `telem1` / `W?` 开，`telem0` / `W0` 关；`on_idle` 按 `BRIDGE_TELEM_PERIOD_MS/4=250ms` 节流，每次只查**一个**轮子（3→4→5→6 轮转），保证 USART6 仍可响应
- 注意：它把 `LinkProtocol` 的虚表改了，与 ABS 的 `link_protocol.h` 二进制不兼容
- 结论：想用「主动上报」，必须先把虚表统一。

### `feature/uart2-motor-dump` — 中间态（ABS 的父）
- 23 提交 / 38 文件 / +2879 −219
- 把 USART1 的 TX 从 PB6 改到 **PA9**（注释「shell 串口2 TXD」），PB7 保持 RX；`Core/Src/usart.c` 加 `__HAL_RCC_GPIOA_CLK_ENABLE()`
- 期间三次反复：先镜像 motor/lift 流量（`a858a97`）→ 改非阻塞 hex dump（`afadf53`）→ 全删只留纯 ZDT 二进制（`2a7be07`）
- 结论：**ABS 的直接祖先**，PA9 的改动从这里开始。

### `cursor/absolute-motor-goto-e246` — **主干（执行线最高点）**
- 32 提交 / 39 文件 / +3633 −219（其中 9 个非 merge 提交是它独有的）
- 独有能力：
  - `MA` 绝对定位：先读 0x36 拿起点 → `Zdt_GoPosUnits(err)` → `Zdt_WaitMotionSettled`（0x3A bit1 到位，退化判据 0x35 RPM==0，连续 2 次稳定）→ 最多 4 趟，收敛阈值 3°
  - `MZ` 零位（0x0A），广播地址被拒
  - `MR` dump 0x36 / 0x1F / 0x3A 原始 hex
  - `ver` 回 `OK VER <branch> <hash> <desc>`（由 CMake 每次构建生成 `fw_version.h`）
  - `test1m` 前进约 1 m（位置模式）
  - 方向符号表 `zdt_dir_map.h`：FL/RL = −1，其余 +1（提交 `1892b21` 修正）
  - 软限位 `zdt_limits.h`（默认关）
- 关键修复链：`7b6b2cc` 65536/圈（原来错用 3200，差 20 倍）→ `111e8f0` 0xFD 方向位与 0x36 编码符号相反 → `482173b` QueryRead 改轮询 RX 避开 ISR+task 信号量互锁
- 缺点：舵机仍在 I2C1 PB8/PB9；缺 telem；缺 AHRS
- 结论：**执行线合并基线**。

### `cursor/bridge-motor-optimize-b5d6` — **主干（次选，仅 2 提交增量）**
- 25 提交 / 38 文件 / +2963 −220
- 与 ABS 的 merge-base = `903b6c6`，**互不包含**
- 独有 2 提交：
  - `edf087b` refactor(bridge): integer test1m geometry, MG cap, unified I/O tags —— 几何参数全改整数（`BRIDGE_WHEEL_DIAMETER_MM 100U`、`BRIDGE_WHEEL_GEAR_X100 100U`、`BRIDGE_TEST1M_DIST_MM 1000U`），新增 `BRIDGE_MG_MAX_DEG 3600L` 拒绝超大单次角度
  - `d0d0ed7` docs
- 结论：**edf087b 必须 cherry-pick 到 ABS**，其余已被 ABS 覆盖。

### `feature/servo-pca9685` — **主干（硬件迁移，必须 cherry-pick）**
- 9 提交 / 32 文件 / +1191 −299
- 独有 `55218f6`：PCA9685 从 I2C1（PB8/PB9）整体迁到 **I2C2（PF0=SDA / PF1=SCL，AF4）**，DMA1 Stream2/CH3（RX）+ Stream7/CH7（TX），并释放 PB8/PB9 给相机 FPC
- 驱动本体：`HAL_I2C_Mem_Write_DMA` / `Mem_Read_DMA` + `HAL_I2C_Master_Abort_IT`，总线 mutex + 完成信号量，50 Hz，PRESCALE 121，脉宽 200–2800 µs，夹爪 1000（合）–1500（开）
- 注意：该分支的 `usart.c` **仍是 PB6/PB7**，与 ABS 的 PA9 冲突
- 结论：**只 cherry-pick 55218f6**，别整支合并。

### `feature/usart1-sniff-fwd` — 一次性（但值得留档）
- 18 提交 / 35 文件 / +2246 −213
- `Zdt_SetRxHook()` 挂 USART1 原始字节钩子 → 入队 → `Link_Send()` 原样转发到 USART6；开机 banner `SNIFF USART1@115200 -> USART6@9600`；`fwd` / `fwd0` / `fwd?` 控制，Link_Send 忙时重试 20 次
- 结论：**排线排查神器**，不进主线，但建议保留分支或把 hook 机制抽成可选编译项。

### `feature/usart1-hello-tx` — 一次性（不要合）
- 20 提交 / 37 文件 / +2277 −229
- **删掉了 linkTask / zdtTask / servoTask 全部模块**，`freertos.c` 只剩 `defaultTask`；USART1 用 PA9 发 `hello`，USART6 发 `hello1`，PH10 心跳，失败发 `fail`
- 唯一价值：它的 `.ioc` 是**唯一把 PA9 改对的**（`Mcu.Pin6=PA9`、`PA9.Signal=USART1_TX`）——ABS 的 `.ioc` 仍写 PB6
- 结论：**只取它的 .ioc 引脚定义**，代码别合。

---

## 3. 感知线（BSP / BMI088 / IST8310 / AHRS）

链路：`bsp-board-io` → `ist8310-mag` → `wheel-ahrs` → `cursor/gyro-mag-ahrs-polish-0c06`。

### `feature/debug-openocd` — 主干（工具链）
- 2 提交 / 3 文件 / +98
- `openocd.cfg`：`interface/cmsis-dap.cfg` + `transport swd` + `adapter speed 1000` + `target/stm32f4x.cfg` + `reset_config none`（无线探针没接 nRST，靠 SYSRESETREQ）
- `.vscode/launch.json`（Debug / Attach 两套）+ `tasks.json`（Build / Flash）
- 注意：全部路径硬编码 `C:/Users/37963/...`，换机器即失效
- 结论：**采纳配置，但要把路径改成 workspaceFolder 变量**。

### `feature/imu-bmi088` — 重复（可删）
- tree 与 `debug-openocd` 完全相同（同内容不同 sha：`8d651cf`/`1051f1a` vs `5b9b7d0`/`598e413`）
- 结论：分支名承诺的 BMI088 驱动**不在这个分支里**（在 bsp-board-io），可删。

### `feature/bsp-board-io` — 中间态
- 5 提交 / 28 文件 / +1278 −16
- `BSP/Board`：`Board_LedSet(R/G/B)`（PH12/PH11/PH10 高有效）、`Board_KeyPressed()`（PA0 低有效）
- `UserLib/BMI088`：SPI1 轮询 Mode 3（SCK/MISO/MOSI = PB3/PB4/PA7，CS accel=PA4 / gyro=PB0），量程 ±3 g / ±500 dps，含自检与 chip-id 校验
- `Applications`：`boardTask`(256*4, Low) + `imuTask`(512*4, BelowNormal)，1 Hz 通过 `BT_Send()` 往 USART6 打 `SMOKE ...`
- 注意：`BT_Send()` 与 Link 抢同一个串口
- 结论：感知线地基。

### `feature/ist8310-mag` — 中间态
- 8 提交 / 40 文件 / +1879 −29
- I2C3（SCL=PA8 / SDA=PC9，400 kHz），7bit 地址 0x0E（HAL 0x1C），WHO_AM_I 期望 0x10
- RSTN=PG6 / DRDY=PG3 **软 GPIO**（不在 .ioc 里，代码里手工初始化）
- 初始化序列：RSTN 低 50 ms → 高 50 ms → 读 WAI → `AVGCNTL=0x09` / `PDCNTL=0xC0` / `CNTL1=0x0B`（200 Hz 连续）
- 注意：读磁力计用的是**阻塞** `HAL_I2C_Mem_Read(&hi2c3, ...)`，I2C3 配了 DMA 但没用上
- 结论：9 轴的第一半。

### `feature/wheel-ahrs` — 中间态
- 11 提交 / 48 文件 / +2486 −31
- `UserLib/AHRS`：Mahony 6 轴 + 9 轴两种更新，`wheel_ahrs_state_t{roll,pitch,yaw,q[4],mag_used,update_count}`
- 磁融合门控：模长需落在 20–120 µT，且 `mag_ready` + `gyro_calibrated` 才走 9 轴，否则回落 IMU
- 开机 gyro bias 标定（静止 2 s 取均值）
- 结论：姿态闭环成型。

### `cursor/gyro-mag-ahrs-polish-0c06` — **主干（感知线最高点）**
- 14 提交 / 48 文件 / +2663 −31
- 3 个加固提交：
  - `fba6a10` robust BMI088 gyro bias cal（warmup + trimmed mean，去掉首尾极值再平均）
  - `3308b2a` IST8310 DRDY wait and mag_ready streak（连续 N 次成功才置 ready）
  - `a1cdbf9` gate AHRS mag fuse；SMOKE 增加 `mrdy` 字段 + `GYROCAL MOTION` 告警
- 缺点：`app/BT`（未改名 Link）、无电机/舵机、SMOKE 直写 USART6
- 结论：**感知线合并基线**。

---

## 4. 文档与占位分支

### `docs/dev-handbook` — **主干（文档资产，应保留）**
- 5 提交 / 15 文件 / +365 −142
- `README.md`：项目定位、硬件清单、构建命令、当前进度表
- `docs/DEVELOPMENT.md`：项目地图、目标架构、6 个里程碑、协作分工（Mini_Agent / 嵌入式 / Camelia / 3D）、已知风险（全工程只能有一份 UART 回调、NVIC 5–15、F103 参考工程不能照搬、nano.specs 无 %f）
- 结论：**全仓最有价值的文档**，合并后按实际状态改写「待办」即可。

### `feature/mecanum-kinematics` — 空壳
- 7 提交 / 18 文件 / +463 −142
- 实际内容：`app/BT → app/Link` 改名 + OpenOCD 2 提交 + README/DEVELOPMENT
- **一行运动学都没有**（没有逆解、没有轮速分配、没有 MFW/MFGB，尽管 DEVELOPMENT.md 里承诺了）
- 结论：名不副实，**可归档删除**。

### `feature/usart-module-port` — 重复 + 空壳
- 7 提交 / 18 文件 / +463 −142
- tree 与 `mecanum-kinematics` **完全相同**
- 结论：**可删**。

---

## 5. 目录演进总览

| 分支 | app/ 下目录 |
|---|---|
| `main` / `chore` / `debug-openocd` / `imu-bmi088` | `BT` |
| `docs/dev-handbook` / `mecanum-kinematics` / `usart-module-port` | `Link` |
| `zdt-motor-bus` | `Link` + `ZDT` |
| `servo-pca9685` | `Link` + `Servo` |
| `bsp-board-io` | `BT` + `BMI088` + `Applications` + `BSP` |
| `ist8310-mag` | 上者 + `IST8310` |
| `wheel-ahrs` / `ahrs-polish` | 上者 + `AHRS` |
| 其余 8 支执行线 | `Link` + `ZDT` + `Bridge` + `Servo` |

关键分水岭是提交 `a734437`（`app/BT → app/Link` 改名）：执行线都吃了它，感知线都没吃。

---

## 6. 复现命令

```bash
# 拉全部分支
git clone --no-checkout https://github.com/vesperadev/carry_bot_v1.git work
cd work && git fetch origin "+refs/heads/*:refs/remotes/origin/*"

# 每支的独有提交与改动量
for b in $(git branch -r | grep -v HEAD | sed "s#origin/##"); do
  echo "== $b"; git log --no-merges --pretty="%h %s" main..origin/$b
  git diff --shortstat main origin/$b
done

# tree 相同判定（找出重复分支）
for b in $(git branch -r | grep -v HEAD | sed "s#origin/##"); do
  echo "$(git rev-parse origin/$b^{tree}) $b"
done | sort

# 合并冲突预演
git merge-tree --write-tree origin/cursor/absolute-motor-goto-e246 origin/cursor/gyro-mag-ahrs-polish-0c06 | head -40

# 真实构建（需 arm-none-eabi-gcc + cmake + ninja）
git worktree add ../absbuild origin/cursor/absolute-motor-goto-e246
cd ../absbuild && cmake --preset Debug && cmake --build --preset Debug
```
