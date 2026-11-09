## Summary
<!-- 改了什么 / What changed -->

## Module / branch
- [ ] Link / BT（上位机链路）
- [ ] Bridge（蓝牙命令）
- [ ] ZDT（Emm V5 电机总线）
- [ ] Servo（PCA9685）
- [ ] Motion（麦轮）
- [ ] docs / CI / hygiene

## Test plan
- [ ] 本机 CMake 配置或编译通过（注明 preset / 工具链）
- [ ] （如涉及）板级验证：现象与命令

## Checklist
- [ ] 不提交 `build/` 或 `*.elf` / `*.bin` / `*.hex`
- [ ] CubeMX 重新生成后已核对 USART/I2C 与 DMA
- [ ] `main` 合并后仍应可配置/可编译
- [ ] 一个 PR 只做一件事
