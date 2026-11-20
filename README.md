# Cardputer ADV 固件合集（Bruce 合并版 + M5Launcher 中文版）

面向 **M5Stack Cardputer / Cardputer ADV**（ESP32-S3FN8，8MB Flash）的两个自制固件：

| 固件 | 说明 | 版本 |
|---|---|---|
| **Bruce 合并版** | Bruce 1.17 中文 + JosephCGS ADV 的 CC1101/NRF24/IR 支持 + 自加 Princeton/PT2262 暴力破解与 UNILARM 拨码 + 中文化扩到 678 条 | 1.17-zh-merged |
| **M5Launcher 中文版** | 官方 bmorcelli/Launcher 3.0.0 全界面中文化（自带 12px 点阵字库与查表层） | 3.0.0-zh |

固件二进制在 [Releases](../../releases) 里，仓库里放的是文档与源码补丁。校验值见 `SHA256SUMS.txt`。

---

## 版本对比

| | Bruce 合并版 | Launcher 中文版 |
|---|---|---|
| 上游 | Bruce 1.17 中文（`sunshinexk/Bruce-Cardputer-zh`） | M5Launcher 3.0.0（`bmorcelli/Launcher`） |
| 用途 | 多工具射频/红外/RFID 固件 | 固件启动器（从 SD/OTA/WebUI 装别的固件） |
| 体积 | 5,022,896 B | 1,507,584 B |
| app 分区 | 4.9MB（占 97.0%） | 2.0MB（占 68.8%，已从 1.35MB 扩大） |
| 中文方式 | 上游自带中文字库 + 词条扩到 678 条 | 自建查表层 + 12px 点阵字库（264 字） |
| 特色功能 | ProtoPirate、Princeton/PT2262 暴力破解、UNILARM 拨码、ADV CC1101/NRF24/IR 引脚 | 全界面中文，可按 `;` `.` 导航装固件 |
| 分区表 | app0 0x10000 / 0x4E0000 | app0 0x10000 / 0x200000 |

两者都是 0x0 全量镜像，互刷必须整片写。

**截图**（待补：`docs/screenshots/`，把实机拍的界面照丢进去即可）

---

## 补丁怎么用

仓库里的 `patches/` 同时提供两种形态：

**A. `git apply` 补丁（推荐）**

```bash
# Bruce
git clone https://github.com/sunshinexk/Bruce-Cardputer-zh
cd Bruce-Cardputer-zh && git checkout <1.17-zh 对应提交>
git apply /path/to/bruce-1.17-zh-merged.patch

# Launcher
git clone https://github.com/bmorcelli/Launcher && cd Launcher
git checkout 3.0.0 && git submodule update --init
git apply /path/to/launcher-3.0.0-zh.patch
cp /path/to/ardgfx.cpp /path/to/ardgfx.h lib/DisplayDrivers/src/backends/
cp /path/to/zh_lang.h /path/to/zh_font.h lib/DisplayDrivers/src/
```

两个补丁均已用 `git apply --check` 验证过能干净应用。

**B. 整文件替换**

`patches/bruce/interface.cpp`、`emv_reader.cpp` 是**完整文件**而非 diff——因为本地留存的"原始"副本在改完后被覆盖，无法产出可信 diff。这两个文件直接覆盖到源码同路径即可（内容是 ADV 引脚默认值 + EMV AID→卡组织兜底）。

---

## 刷写

两个固件都是 **0x0 全量镜像**：

```bash
esptool.py --chip esp32s3 -p COM5 -b 460800 write_flash \
  --flash_mode dio --flash_freq 80m --flash_size 8MB \
  0x0 "Bruce-CardputerADV-1.17-zh-CC1101-NRF24-IR_合并.bin"
```

M5Burner → 自定义固件 → 选 bin → 烧写，也可以。

> 两个固件的分区表不同（Bruce app 4.9MB / Launcher app 2MB），**切换时务必刷 0x0 全量**，不要只刷 app。

---

## Bruce 合并版做了什么

**来源**
- Bruce 1.17 中文固件：`sunshinexk/Bruce-Cardputer-zh`（v1.17-zh）
- ADV 硬件支持思路：`JosephCGS/ADV-CC1101-NRF24L01-IR`（1.16.1 魔改）

**合并策略**：保留 1.17 全部中文与 1.17 新特性（ProtoPirate、Chrysler/Ford/Kia/Mazda、RTL 32-bit seed search 等 31 种 SubGHz 协议），把 ADV 的 Cap-CC1101 引脚写成默认值，EMV 增加 AID→卡组织兜底识别。

**ADV 默认引脚**（编译进固件，开箱即用）

```
CC1101: CS=5  GDO0=15  POWER_EN=3  SW0=13  SCK=40  MISO=39  MOSI=14
NRF24:  CS=6  IO0=4
IR TX:  44（可选 3/4/5/6/13/15）
```

关键点：`CC1101_bus.io0` 才是真正的 GDO0（`rf_listen.cpp` 在它上面挂中断），原默认 io0=5 与 Cap-CC1101 的 CS=5 冲突，故改为 CS=5 / GDO0=15；`rf_utils.cpp` 检测到 `cs == CAP_CC1101_SS_PIN` 会自动拉高 POWER_EN 并按频段切换 SW0。

**RF 暴力破解增强**
- 新增 `Princeton 24bit`（PT2262，`te_short=390 / te_long=1170`）与 `LinearDelta3 8bit`
- 新增 `Bits` 位宽选项（4~满位）：24bit 全扫要 4 天，12bit 约 2 分钟
- 新增 `RF → UNILARM (DIP)` 独立菜单：25bit 协议不做 2^25 全扫，改成 16 段 × 4096 定向发射，每段约 5 分钟，可中断

**中文化**：`lang/zh_CN.json` 由 413 条扩到 678 条，经 `tools/gen_zh_lang.py` 生成（该表对 `ZH_STRINGS[]` 做 strcmp 二分查找，**必须按字节序排序**，手改会静默失效）。

**已知限制**
- app 占分区 97.0%，余 ~154KB，再加功能需精简
- UNILARM 脉形取自社区实测（bit0=150/650µs、bit1=550/250µs），非官方编码器常量；门机无反应时改 `ul_send_frame()` 重编
- 无 ADVLED 功能（JosephCGS 私有实现，上游源码不存在，无法复刻）

---

## M5Launcher 中文版做了什么

官方 Launcher 3.0.0 **没有多语言机制**，显示库也只支持 8px 英文点阵，所以做了三层：

1. **查表层**（295 条，`patches/launcher/zh_lang.h`）：在显示后端最底层拦截 `print()/drawString()/drawCentreString()/drawRightString()/textWidth()`，先过英中查表。业务代码零改动，菜单/弹窗/安装/备份/OTA 流程自动生效；未命中保留英文，不会乱码。另含最长前缀匹配，`Installing xxx` 也能译成"正在安装 xxx"。
2. **字库层**（264 字 / 12×12 点阵，`patches/launcher/zh_font.h`）：系统 `Microsoft YaHei UI` 单色渲染 + 阈值二值化，只收录实际用到的字，共 23KB，二分查找 + 行程填充绘制。
3. **布局**：Launcher 字宽模型是"每字节 6px"，一个汉字 2 字节 = 12px，与 12×12 点阵天然对齐，居中/右对齐/翻页公式无需改动。

**与原版差异**
- `app0` 从 1.35MB 扩到 2MB（原版英文已用到 104%，加字库必溢出；扩后占 68.8%）
- WebUI 资源改为本地 gzip（原版走 toptal 在线压缩，断网无法编译）
- 库子模块直接铺进 `lib/`（无 git 联网环境可编译）

浏览器里的 Web 管理界面仍是英文（在电脑/手机渲染，不影响设备屏幕）。

**未在真机验证**：中文观感与个别界面偏移需实机查看；8px 小字行里的 12px 汉字可能轻微压行，把词条改短即可。

---

## 仓库结构

```
docs/                                   固件说明、功能使用说明、Launcher 中文说明
bruce-1.17-zh-merged.patch             Bruce 改动（git apply 即用）
launcher-3.0.0-zh.patch                  Launcher 改动（git apply 即用）
patches/bruce/                          Bruce 改动文件（整文件备用）
patches/launcher/                       Launcher 汉化层文件（ardgfx / zh_lang / zh_font / 翻译表）
patches/launcher/tools/                 翻译表与字库生成脚本
SHA256SUMS.txt                          固件校验值
```

---

## 许可

- **Bruce**：上游为 GPL-3.0，本仓库的二进制基于其修改版，改动点全部以补丁形式提供；完整源码请取 `sunshinexk/Bruce-Cardputer-zh` 后按 `patches/` 应用。
- **M5Launcher**：上游 `bmorcelli/Launcher` 为 MIT，修改部分同样以补丁形式提供。
- 文档部分 CC BY-NC-SA 4.0。

固件仅限自有设备与授权环境使用；deauth、干扰、门禁/车辆/银行卡相关功能在多数地区受法律限制。
