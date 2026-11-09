# IST8310 纾佸姏璁?鈥斺€?CubeMX 閰嶇疆娓呭崟锛圧oboMaster 寮€鍙戞澘 C 鍨嬶級

> 鍒嗘敮锛歚feature/ist8310-mag`锛堣嚜 `feature/bsp-board-io`锛? 
> 鐘舵€侊細**浠呮枃妗?*锛岄┍鍔ㄦ湭瀹炵幇銆傜瓑浣犲湪 CubeMX 閲屽姞濂?**I2C3 + 澶嶄綅/DRDY 鑴?* 骞?Generate Code 鍚庯紝鍐嶅湪鏈垎鏀惤 `UserLib/IST8310`銆? 
> 纭欢渚濇嵁锛歚CubeMx/docs/RoboMaster 寮€鍙戞澘 C 鍨嬬敤鎴锋墜鍐?pdf` / 鍘熺悊鍥撅紱甯冨眬椋庢牸瀵归綈鏈粨 `docs/BOARD_IO.md`銆?
## 1. 涓轰粈涔堣浣犲厛鏀?CubeMX

褰撳墠 `carry_bot_v1.ioc` **鍙湁 I2C1**锛圥B8/PB9锛岀粰 PCA9685 绛夌敤锛夛紝**娌℃湁 I2C3**锛屼篃娌℃湁 IST8310 鐨?RSTN/DRDY GPIO銆? 
纾佸姏璁¤蛋鏉胯浇 **I2C3**锛屽拰 I2C1 涓嶅啿绐侊紱鍦ㄩ┍鍔ㄥ啓姝讳箣鍓嶅繀椤诲厛鎶婂璁句粠 CubeMX 鐢熸垚鍑烘潵锛屽苟 **Keep User Code** 鍐嶆彁浜?`.ioc` + 鐢熸垚鐨?`i2c.c` / `gpio.c` / `main.h` 绛夈€?
鏈垎鏀?**鍏堜笉鍐欓┍鍔?*锛岄伩鍏嶅拰鏈敓鎴愮殑 HAL 瀵逛笉涓娿€?
## 2. CubeMX 鍕鹃€夋竻鍗曪紙璇蜂弗鏍兼寜姝わ級

鎵撳紑宸ョ▼鏍圭洰褰?`carry_bot_v1.ioc`锛圫TM32F407IGHx / Type-C锛夈€?
### 2.1 I2C3锛堢鍔涜鎬荤嚎锛?
| 椤?| 璁剧疆 |
|---|---|
| 澶栬 | **I2C3** 鈫?Mode: **I2C** |
| 閫熷害 | **Fast Mode 400 kHz**锛堜笌鎵嬪唽鏉胯浇 mag 涓€鑷达級 |
| SCL | **PA8** 鈫?`I2C3_SCL` |
| SDA | **PC9** 鈫?`I2C3_SDA` |
| GPIO | 寮€婕?+ 涓婃媺锛圕ube 榛樿 AF 鍗冲彲锛涘嬁鏀规垚鎺ㄦ尳锛?|
| DMA | **v1 鍙笉閰?*锛堝厛杞 `HAL_I2C_Mem_*`锛夛紱鏃ュ悗鍐嶅姞 DMA 鏃跺嬁鍗犵敤 I2C1 宸茬敤鐨?DMA1 Stream0/6 |
| NVIC | 鑻ュ彧鍋氳疆璇㈠彲鍏堝叧 Ev/Er锛涜嫢寮€涓柇锛屼紭鍏堢骇鏀惧湪 FreeRTOS 鍏佽鐨?**5鈥?5** |

> 瀵圭収锛氭湰浠?I2C1 宸叉槸 Fast + DMA锛堣埖鏈猴級銆?*IST8310 蹇呴』鐢?I2C3**锛屼笉瑕佹帴鍒?I2C1銆?
### 2.2 GPIO锛氬浣?/ 鏁版嵁灏辩华

| 缃戠粶鍚嶏紙寤鸿 User Label锛?| 寮曡剼 | 鏂瑰悜 / 妯″紡 | 榛樿鐢靛钩 | 璇存槑 |
|---|---|---|---|---|
| `RSTN_IST8310` | **PG6** | Output Push-Pull | **HIGH** | 浣庢湁鏁堝浣嶏紱绌洪棽蹇呴』鎷夐珮 |
| `DRDY_IST8310` | **PG3** | Input锛堝彲鍏堟棤涓婁笅鎷夛紝鎸夊師鐞嗗浘锛?| 鈥?| 鏁版嵁灏辩华锛?*v1 杞鍙瀵勫瓨鍣紝鍙笉閰?EXTI** |

鍙€夛紙浠ュ悗鍐嶈锛夛細

- PG3 鈫?EXTI falling edge锛屽湪涓柇閲岀疆鏍囧織锛岀鍔涜浠诲姟鍞ら啋璇?XYZ銆? 
- v1锛?*杞鍗冲彲**锛屽厛鎶?I2C 閫氫簡鍐嶅姞 EXTI銆?
### 2.3 澶嶄綅鏃跺簭锛堥┍鍔ㄩ噷瀹炵幇鏃剁敤锛汣ube 渚у彧闇€榛樿 HIGH锛?
鍏稿瀷锛歚RSTN=0` 淇濇寔 鈮?0 ms 鈫?`RSTN=1` 鈫?鍐嶅欢鏃舵暟鍗?ms 鈫?`IsDeviceReady` / 璇?WHO_AM_I銆? 
CubeMX 閲屾妸 PG6 榛樿杈撳嚭楂樺嵆鍙紝涓婄數鍚庣敱椹卞姩鍋氫竴娆¤剦鍐插浣嶃€?
### 2.4 I2C 浠庢満鍦板潃锛堝姟蹇呭垎娓?7-bit / HAL锛?
| 璇存硶 | 鍊?| 鐢ㄥ湪鍝噷 |
|---|---|---|
| **7-bit 鍦板潃** | **`0x0E`** | 鏁版嵁鎵嬪唽 / 閫昏緫鍒嗘瀽浠湴鍧€甯?|
| **HAL `DevAddress`锛堝乏绉?1 浣嶏級** | **`0x1C`**锛堝嵆 `0x0E << 1`锛?| `HAL_I2C_Mem_Read/Write`銆乣HAL_I2C_IsDeviceReady` 绛?|

璇存槑锛歋TM32 HAL 鐨?`DevAddress` 鍙傛暟鏄?**7-bit 鍦板潃宸︾Щ 1 浣嶅悗鐨?8-bit 褰㈠紡**锛堝啓鏂瑰悜浣嶇敱 HAL 鍐嶅鐞嗭級銆? 
鏂囨。/娉ㄩ噴閲岃鍚屾椂鍐欙細`IST8310_I2C_ADDR_7BIT 0x0E` 涓?`IST8310_I2C_ADDR_HAL (0x0E << 1) /* 0x1C */`锛岄伩鍏嶅拰鑸垫満渚?`0x40`/`0x80` 娣风敤涔犳儻鎼炲弽銆?
WHO_AM_I 绛夊瘎瀛樺櫒鍊间互 IST8310 鎵嬪唽涓哄噯锛堥┍鍔ㄥ疄鐜版椂鍐嶉攣杩?`ist8310_reg.h`锛夈€?
### 2.5 Generate Code

1. Project Manager 鈫?Code Generator锛?*Keep User Code when re-generating**锛堟垨绛変环 鈥滃浠界敤鎴蜂唬鐮佲€濓級鍕句笂銆? 
2. Generate Code銆? 
3. 缂栬瘧纭鏃犲洖褰掞紙鐜版湁 USART / I2C1 / SPI1 鎵嬫敼鏂囦欢浠嶅湪锛夈€? 
4. **鎻愪氦**鏇存柊鍚庣殑锛?   - `carry_bot_v1.ioc`
   - `Core/Src/i2c.c` / `Core/Inc/i2c.h`锛堝簲鍑虹幇 `hi2c3`锛?   - `Core/Src/gpio.c`銆乣Core/Inc/main.h`锛坄RSTN_IST8310` / `DRDY_IST8310` 瀹忥級
   - 濡傛湁鍙樺姩鐨?`stm32f4xx_hal_msp.c`銆乣.mxproject` 绛変竴骞剁撼鍏? 
5. 鍛婅瘔鍗忎綔鏂广€孋ubeMX 宸插悎鍏ャ€嶏紝鍐嶅湪鏈垎鏀啓椹卞姩銆?
## 3. 鏈粨钀藉湴璺緞锛堝榻愮幇鏈?QGimbal 椋庢牸锛屽嬁鍙﹁捣鐐夌伓锛?
宸叉湁甯冨眬锛堣 `docs/BOARD_IO.md`锛夛細

```text
BSP/Board/          LED / KEY / CS GPIO
UserLib/BMI088/     SPI 浼犳劅鍣ㄩ┍鍔?Applications/       boardTask / imuTask
```

IST8310 寤鸿锛?*瀹炵幇闃舵**鍐嶅垱寤猴紝鏈?PR 涓嶅缓绌虹洰褰曞埛灞忥級锛?
| 灞傜骇 | 寤鸿璺緞 | 鑱岃矗 |
|---|---|---|
| 鏉跨骇鑴?| `BSP/Board/` 澧炶ˉ鎴栬杽灏佽 `board_ist8310_gpio.*`锛堝彲閫夛級 | RSTN 鑴夊啿銆佽 DRDY |
| 鍣ㄤ欢椹卞姩 | **`UserLib/IST8310/`**锛堝鏍?`UserLib/BMI088`锛?| 瀵勫瓨鍣ㄣ€佸崟娆?杩炵画娴嬨€乆YZ 鎹㈢畻 |
| 浠诲姟 | `Applications/` 鈫?濡?`mag_task.*` | 鍛ㄦ湡璇绘暟锛涘彲涓庣幇鏈?SMOKE UART 鎷间竴琛?|

CMake锛氭寜 BMI088 鍚屾牱鏂瑰紡鍦?`UserLib/IST8310/CMakeLists.txt` + 涓婂眰 `add_subdirectory`锛?*涓嶈**濉炶繘 `app/`锛堥偅鏄?Link/ZDT/Servo 鍗忚灞傞鏍硷級銆?
璋冭瘯/鐑у綍锛氭湰鍒嗘敮宸插甫鑷?bsp 鐨?`openocd.cfg` + `.vscode`锛圗LF 鍥哄畾涓?`carry_bot_v1.elf`锛夛紝worktree 鐩綍鍚嶄笉褰卞搷銆?
## 4. 楠屾敹锛圕ubeMX 瀹屾垚鍚庛€侀┍鍔ㄥ墠锛?
- [ ] `.ioc` 鍙 I2C3 @ PA8/PC9 Fast Mode  
- [ ] `hi2c3` 宸茬敓鎴愶紱PG6 榛樿楂樸€丳G3 涓鸿緭鍏? 
- [ ] Debug 鏋勫缓閫氳繃锛涘師鏈?BMI088 SMOKE / LED/KEY 琛屼负鏈潖  
- [ ] 锛堝彲閫夛級鐢?`HAL_I2C_IsDeviceReady(&hi2c3, 0x1C, 鈥?` 鍦ㄤ复鏃?USER CODE 閲屾帰涓€涓嬶紝搴旇繑鍥?`HAL_OK`

## 5. 闈炵洰鏍囷紙鏈垎鏀綋鍓嶏級

- 涓嶅疄鐜板畬鏁?IST8310 椹卞姩 / magTask  
- 涓嶆敼 I2C1 / PCA9685 / USART DMA 鍒嗛厤  
- 涓嶆妸 mag 鍗忚鎺ュ埌 Bridge锛堜互鍚庡彟寮€浠诲姟锛?
---

**浣犵幇鍦ㄩ渶瑕佸仛鐨勶細** 鎸?搂2 鏀?CubeMX 鈫?Generate 鈫?缂栬瘧 鈫?鎶婄敓鎴愭枃浠?commit 鍒?`feature/ist8310-mag`锛堟垨寮€ follow-up commit锛夈€傚畬鎴愬悗鍠婁竴澹板嵆鍙紑濮嬪啓 `UserLib/IST8310`銆?
## 6. 瀹炵幇鐘舵€侊紙鏈垎鏀悗缁彁浜わ級

- **I2C3**锛氬凡浠庢湰鏈?CubeMX 鐢熸垚缁撴灉鍚堝叆锛坄.ioc` / `i2c.c` / `dma.c` / `main.c` / `stm32f4xx_it.*`锛夈€?- **PG6 / PG3**锛欳ube 鏈厤锛涚敱 `UserLib/IST8310` 鍦?`IST8310_Init` 閲?**杞垵濮嬪寲**锛圙PIOG 鏃堕挓銆丷STN 鎺ㄦ尳榛樿楂樸€丏RDY 杈撳叆锛夈€?- 椹卞姩锛歚UserLib/IST8310`锛坄Init` / `WhoAmI` / `ReadRaw` / `Read`鈫捖礣锛夈€?- 鍐掔儫锛歚imuTask` SMOKE 琛屽鍔?`merr/mok/mx/my/mz`锛埪礣脳10锛夈€?
