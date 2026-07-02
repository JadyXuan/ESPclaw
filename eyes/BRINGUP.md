# Fursuit Eyes 烧录与调试记录

> 分支：`fursuit-firmware`  
> 目标板：ESP32-S3 + 5" JC8048W550 RGB 屏（800×480）  
> 串口：COM5  
> 编译环境：ESP-IDF v5.3.2（`D:\GitHub\ESPclaw\v5.3.2\esp-idf`）

---

## 一、硬件信息

从 esptool 日志确认：

```
Chip is ESP32-S3 (QFN56) (revision v0.2)
Features: WiFi, BLE, Embedded PSRAM 8MB (AP_3v3)
Crystal is 40MHz
MAC: 3c:84:27:c8:04:bc
Flash will be erased from 0x00000000 to 0x00005fff...
Flash will be erased from 0x00010000 to 0x000fbfff...
```

- 芯片：ESP32-S3，8MB PSRAM
- 烧录配置：DIO、80MHz、Flash 16MB（`sdkconfig.defaults` 已修改）

---

## 二、环境准备与构建命令

### 1. 激活 ESP-IDF（Anaconda Prompt / CMD）

```cmd
cd /d D:\GitHub\ESPclaw\v5.3.2\esp-idf
export.bat
```

### 2. 编译并烧录

```cmd
cd /d D:\GitHub\ESPclaw\eyes
idf.py set-target esp32s3
idf.py build
idf.py -p COM5 flash monitor
```

退出监视器：`Ctrl + ]`。

### 3. 若未安装 ESP-IDF 工具链

首次需运行：

```cmd
cd /d D:\GitHub\ESPclaw\v5.3.2\esp-idf
install.bat esp32s3
```

---

## 三、调试过程分步记录

### 2026-07-03 问题 1：`CMakeLists.txt` 引用不存在的 `components` 目录

**报错：**

```text
CMake Error at D:/GitHub/ESPclaw/v5.3.2/esp-idf/tools/cmake/project.cmake:465 (message):
  Directory specified in EXTRA_COMPONENT_DIRS doesn't exist:
  D:/GitHub/ESPclaw/eyes/components
```

**原因：** 仓库里 `eyes/` 下没有 `components` 目录，依赖由 `main/idf_component.yml` 管理。  
**修复：** 删除 `eyes/CMakeLists.txt` 里的 `EXTRA_COMPONENT_DIRS` 指向。

### 2026-07-03 问题 2：LovyanGFX 依赖无法从组件仓库下载

**报错：**

```text
WARNING: Component "lovyan03/lovyangfx" not found
ERROR: Version solving failed:
    - no versions of lovyan03/LovyanGFX match ^1.2.0
```

**原因：** LovyanGFX 未发布在 ESP 官方组件仓库，原 `idf_component.yml` 写法不合法。  
**修复：** 改为本地 Git 依赖并手动克隆到 `eyes/components/LovyanGFX`：

```yaml
LovyanGFX:
  override_path: "../components/LovyanGFX"
```

### 2026-07-03 问题 3：`arduino` 组件名错误

**报错：**

```text
Failed to resolve component 'arduino'.
```

**原因：** 从组件仓库下载的 Arduino 包名为 `arduino-esp32`，不是 `arduino`。  
**修复：** `main/CMakeLists.txt` 中 `REQUIRES` 改为 `arduino-esp32`。

### 2026-07-03 问题 4：`esp_now` 组件名错误

**报错：**

```text
Failed to resolve component 'esp_now'.
```

**原因：** ESP-NOW 已内嵌到 `esp_wifi` 组件，不是独立组件。  
**修复：** 从 `REQUIRES` 中移除 `esp_now`，保留 `esp_wifi`。

### 2026-07-03 问题 5：启动后反复重启（`RTC_SW_CPU_RST`）

**现象：** 固件成功烧录，但串口反复重启：

```text
Fursuit Eyes starting...
Init display...
Display init OK
ESP-ROM:esp32s3-20210327
...
rst:0xc (RTC_SW_CPU_RST)
```

**排查步骤：**

| 步骤 | 修改 | 结果 |
|------|------|------|
| 1 | 暂时禁用 GT911 触摸初始化 | 仍重启 |
| 2 | 将 `ESPNOW_CHANNEL` 从 `0` 改为 `1` | 仍重启 |
| 3 | 将 Flash 大小改为 16MB | 仍重启 |
| 4 | 在 `setup()` 里插入分步日志 | 定位到崩溃在 `Display init OK` 之后，`fillScreen` 之前/期间 |
| 5 | 去掉 `display.setBrightness()`/`setRotation()`，改用 GPIO2 直接控制背光 | 屏幕能闪一下，但仍重启 |
| 6 | 跳过 `display.fillScreen()` | 崩溃点移到 `Create sprites...` |
| 7 | 降低 PCLK 从 16MHz 到 10MHz | 仍卡在 `Create sprites...` |
| 8 | 为 sprite 设置 `setPsram(true)` | **仍卡在 `Create sprites...`** |

**当前最新报错（2026-07-03 02:35）：**

```text
Fursuit Eyes starting...
Init display...
Display init OK
Backlight ON
fillScreen...
Display ready (skipped fillScreen)
Create sprites...
ESP-ROM:esp32s3-20210327
Build:Mar 27 2021
rst:0xc (RTC_SW_CPU_RST),boot:0x8 (SPI_FAST_FLASH_BOOT)
Saved PC:0x4037bf52
--- 0x4037bf52: esp_cpu_wait_for_intr at D:/GitHub/ESPclaw/v5.3.2/esp-idf/components/esp_hw_support/cpu.c:64
```

**分析：**
- `RTC_SW_CPU_RST` 通常表示软件复位（`esp_restart()`）或任务看门狗触发。
- `esp_cpu_wait_for_intr` 是 idle 循环函数，崩溃点不一定在这里，只是最后保存的 PC。
- 屏幕能“一闪而过”，说明 RGB 屏初始化、背光、PCLK 基本正确。
- 现在崩溃发生在 `spriteL.createSprite(EYE_SIZE, EYE_SIZE)` 或 `spriteR.createSprite(EYE_SIZE, EYE_SIZE)` 期间。
- 即使设置 `setPsram(true)`，仍然崩溃，可能原因：
  - `LGFX_Sprite` 的 parent 是 RGB 面板的 `display`，sprite 创建时内部状态异常；
  - PSRAM 分配失败或未正确初始化；
  - 在 `setup()` 任务中调用 `createSprite` 时触发内存对齐或中断问题；
  - `display.init()` 后未完全稳定就创建 sprite。

---

## 四、当前代码状态（关键修改）

### `eyes/CMakeLists.txt`

已移除 `EXTRA_COMPONENT_DIRS` 指向不存在的 `components` 目录。

### `eyes/main/idf_component.yml`

```yaml
dependencies:
  espressif/arduino-esp32:
    version: "^3.1.0"
  LovyanGFX:
    override_path: "../components/LovyanGFX"
```

### `eyes/main/CMakeLists.txt`

```cmake
idf_component_register(
    SRCS "main.cpp" "eye_renderer.cpp"
    INCLUDE_DIRS "."
    REQUIRES arduino-esp32 LovyanGFX esp_wifi
)
```

### `eyes/main/lgfx_jc8048w550.h`

- 禁用 `Light_PWM` 和 GT911 触摸；
- PCLK 从 16MHz 降到 10MHz；
- 面板配置 `use_psram = 1`。

### `eyes/main/main.cpp`

- `setup()` 中分步打印日志；
- 跳过 `display.fillScreen()`；
- GPIO2 直接控制背光；
- `spriteL.setPsram(true)` / `spriteR.setPsram(true)` 在 `createSprite()` 之前。

### `eyes/main/protocol.h`

`ESPNOW_CHANNEL` 从 `0` 改为 `1`。

### `eyes/sdkconfig.defaults`

增加 `CONFIG_ESPTOOLPY_FLASHSIZE_16MB=y`。

---

## 五、下一步建议

1. **确认 PSRAM 是否真正可用**
   在 `setup()` 最前面加 `heap_caps_get_free_size(MALLOC_CAP_SPIRAM)` 打印，确认 PSRAM 已初始化。

2. **减小 sprite 尺寸测试**
   临时把 `config.h` 中 `EYE_SIZE` 从 `400` 改为 `200`：

   ```cpp
   #ifdef BOARD_JC8048W550
   #define SCREEN_W 800
   #define SCREEN_H 480
   #define EYE_SIZE 200
   #endif
   ```

   如果 200 能跑、400 不能跑，说明是 sprite 内存分配问题，需要进一步检查 PSRAM 或分配方式。

3. **改用 `display` 作为 parent 是否正常**
   可尝试将 sprite 先设为无 parent 或检查 `LGFX_Sprite` 构造函数与 RGB 面板的兼容性。

4. **捕获更完整的崩溃信息**
   配置 `CONFIG_ESP_COREDUMP_ENABLE=y` 或开启 `panic handler` 打印 backtrace，定位真正的崩溃函数。

5. **检查是否需要在 menuconfig 中开启 PSRAM 相关选项**
   虽然 `sdkconfig.defaults` 已开启，但可通过 `idf.py menuconfig` 确认：
   - `Component config → ESP32S3-Specific → SPIRAM`
   - `Component config → PSRAM → Enable external PSRAM` 等是否生效。

---

## 六、相关命令速查

```cmd
# 清理构建
del D:\GitHub\ESPclaw\eyes\sdkconfig
rmdir /s /q D:\GitHub\ESPclaw\eyes\build

# 重新配置并构建
idf.py set-target esp32s3
idf.py build

# 烧录并看串口
idf.py -p COM5 flash monitor

# 仅烧录
idf.py -p COM5 flash

# 进入下载模式（按住 BOOT，点 RESET，松 BOOT）
```

---

## 七、Git 提交说明

`eyes/managed_components/` 和 `eyes/build/` 是编译时自动生成的，**不要提交**。

已在 `eyes/.gitignore` 中忽略：

- `build/`
- `managed_components/`
- `components/LovyanGFX/`（需本地手动克隆）
- `sdkconfig` / `sdkconfig.old`

若误执行了 `git add eyes/` 并看到大量 `LF will be replaced by CRLF` warning，这是 Windows 换行符提示，**无害**；真正的问题是 staged 了不该提交的文件。

正确提交方式：

```cmd
cd /d D:\GitHub\ESPclaw
git reset HEAD eyes/
git add eyes/.gitignore eyes/BRINGUP.md eyes/CMakeLists.txt eyes/sdkconfig.defaults eyes/main/ eyes/platformio.ini eyes/dependencies.lock
git status
git commit -m "docs(eyes): add BRINGUP.md and fix build config for JC8048W550"
git push origin fursuit-firmware
```

`dependencies.lock` 可以提交（锁定依赖版本）；`managed_components/` 不要提交。

---

*文档更新：2026-07-03*
