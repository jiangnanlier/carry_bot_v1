# Board IO (DJI RoboMaster Type-C)

Pin map verified against `CubeMx/docs/RoboMaster 开发板 C 型用户手册.pdf` and QGimbal `Core/Inc/main.h`.

| Function | Net | GPIO | Notes |
|---|---|---|---|
| LED_R | LED_R | PH12 | High = on |
| LED_G | LED_G | PH11 | High = on |
| LED_B | LED_B | PH10 | High = on |
| KEY | KEY | PA0 | Pressed = low (10k pull-up) |
| CS1_ACCEL | CS1_Accel | PA4 | Soft NSS, idle high |
| CS1_GYRO | CS1_Gyro | PB0 | Soft NSS, idle high |
| SPI1_SCK | SPI1_CLK | PB3 | AF5 |
| SPI1_MISO | SPI1_MISO | PB4 | AF5 |
| SPI1_MOSI | SPI1_MOSI | PA7 | AF5 |

## Layout (QGimbal-style)

- `BSP/Board` — LED / KEY / CS GPIO helpers
- `UserLib/BMI088` — polling SPI driver (C port of QGimbal `UserLib/BMI088`)
- `Applications` — `boardTask` (heartbeat + key), `imuTask` (init + 100 Hz read)

## CubeMX / DMA caveats

- `Core/Src/spi.c` is **hand-maintained** (polling, Mode 3). Opening CubeMX and enabling SPI1 is recommended later so `.ioc` matches; until then keep USER CODE and this file.
- USART1/6 and I2C1 DMA streams are **unchanged** on this branch.
- Free for future SPI DMA (same as QGimbal): `DMA2_Stream0` (RX) + `DMA2_Stream3` (TX), channel 3 — do **not** steal USART/I2C streams.

## Smoke test

1. Build & flash `build/Debug/carry_bot_v1.elf`.
2. **LED**: green should blink ~1 Hz after boot.
3. **KEY**: hold KEY → red solid; click KEY cycles heartbeat → R → G → B → …
4. **BMI088**: in debugger watch `g_bmi088_last_err` (0 = OK), `g_bmi088_ok_count` increasing, `g_bmi088.accel/gyro/temperature_c` changing when board is moved. Init fail → blue LED toggles while retrying.
