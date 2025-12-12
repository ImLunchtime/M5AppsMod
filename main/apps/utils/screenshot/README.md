# 截图功能说明

本文档详细说明本项目（ESP-IDF + M5Stack/M5GFX）的截屏功能工作原理、触发方式、数据处理流程、集成位置与注意事项，便于二次开发与维护。

## 概览
- 截图保存为 `BMP` 格式，24 位 BGR，无压缩，行数据按 4 字节对齐。
- 默认保存路径：`/sdcard/m5apps/screenshots/`。
- 文件名：`m5apps_{timestamp}.bmp`，其中 `timestamp` 为 `millis()` 的 8 位十六进制。
- 支持在多处界面中随时截屏，触发后会在系统栏显示成败提示。

## 触发与交互
- 触发组合键：`CTRL + SPACE`。
- 入口函数：`UTILS::SCREENSHOT_TOOLS::check_and_handle_screenshot`（`main/apps/utils/screenshot/screenshot_tools.cpp:232`）。
  - 读取键盘状态：`hal->keyboard()->keysState().ctrl` 与 `...space`。
  - 播放按键音，`waitForRelease(KEY_NUM_SPACE)` 防抖。
  - 调用 `take_screenshot` 执行实际保存。
  - 使用系统栏 `canvas_system_bar` 显示“Screenshot saved/failed”的提示，并根据结果着色（绿/红），随后延时 1 秒并请求刷新。

## 数据采集与文件写入
- 核心实现：`UTILS::SCREENSHOT_TOOLS::take_screenshot`（`main/apps/utils/screenshot/screenshot_tools.cpp:29`）。
- 主要步骤：
  1. SD 卡挂载：若未挂载则临时挂载；结束后若是临时挂载，则执行 `eject()`（保证外部状态一致）。
  2. 目录管理：确保 `/sdcard/m5apps` 与 `/sdcard/m5apps/screenshots` 存在，使用 `stat/mkdir` 创建（容错 `EEXIST`）。
  3. 分辨率：通过 `hal->display()->width()/height()` 获取屏幕宽高。
  4. BMP 头：构造 14 字节文件头与 40 字节 `BITMAPINFOHEADER`，设置宽高、24bpp、无压缩；行大小按 `((width * 3 + 3) / 4) * 4` 计算并填充。
  5. 分块读取：每次读取 `CHUNK_ROWS = 15` 行以降低内存占用：
     - 使用 `hal->display()->readRectRGB(0, startY, width, chunkH, rgb_chunk)` 从显示设备抓取 RGB 像素数据。
     - 逐像素转换为 BMP 需要的 BGR 顺序，并在每行尾部 `memset` 0 进行 4 字节对齐。
     - 按 BMP 底向上的行顺序写入（从屏幕底部行开始），保证图像方向正确。
  6. 错误处理：若任一步失败，打印日志、关闭文件并删除不完整文件。
  7. 清理：释放缓冲区并关闭文件；如在函数内挂载 SD，则执行卸载。

## 存储格式细节
- 颜色顺序：屏幕读取为 `RGB`，写入 BMP 为 `BGR`（逐像素交换 R/B）。
- 行对齐：BMP 要求每行字节数是 4 的倍数，使用 0 填充尾部。
- 行顺序：BMP 数据自底向上，因此写入时对每个分块内的行进行反向输出，整体自屏幕底部往上写入。

## 内存与性能策略
- 分块参数：`CHUNK_ROWS = 15`，单块缓冲大小约为 `width * CHUNK_ROWS * 3 + row_size` 字节。
- 优势：避免为整屏分配巨大的缓冲，降低内存峰值；写入过程按块推进，IO 更稳定。
- 影响：截图期间存在同步读写，UI 会有短暂停顿；系统栏提示用于反馈进度与结果。

## 集成位置
- 启动器：在键盘状态更新周期中统一处理快捷键（`main/apps/launcher/launcher.cpp:405`）。
- 各类对话/设置界面：在输入处理循环中统一调用，保证随时可截屏：
  - `main/apps/utils/ui/dialog.cpp:167, 541, 849, 1137`
- HAL 设备层：
  - 设备抽象与接口见 `main/hal/hal.h:58`（`display/keyboard/sdcard` 等 getter）。
  - Cardputer 显示初始化使用 `M5GFX`（`main/hal/hal_cardputer.cpp:18`），`readRectRGB` 能直接从当前帧缓冲读取像素。

## 依赖组件
- 日志：`esp_log`。
- 显示：`M5GFX`/`LGFX_Device`，使用 `readRectRGB` 抓取像素。
- 存储：`SDCard` 抽象（通过 HAL 提供），标准 C 文件 API `fopen/fwrite/fclose`。
- 键盘：HAL 提供的键盘状态与防抖工具（`waitForRelease`）。

## 出错与保护机制
- SD 未插入或挂载失败：函数返回 `false`，系统栏显示失败并播放错误音。
- 目录创建失败：记录日志并中止保存。
- 写文件失败：记录日志并删除部分生成的 BMP 文件，避免留下损坏文件。

## 使用建议
- 由于为 BMP 无压缩格式，文件体积与屏幕分辨率线性相关；若需更小体积可考虑扩展为 PNG/JPEG（需引入相应编码器）。
- 若在截图过程中 UI 短暂停顿属正常现象，建议在功能调用点保持统一的提示反馈（当前系统栏方案已涵盖）。

---

如需修改分块策略或输出格式，可直接在 `screenshot_tools.cpp` 中调整 `CHUNK_ROWS`、像素转换与 BMP 头部构造逻辑。
