# M5Launcher 3.0.0 中文版（Cardputer / Cardputer ADV）

来源：官方 [bmorcelli/Launcher](https://github.com/bmorcelli/Launcher) **3.0.0**（tag `3.0.0`，2026-11-06 发布）
平台：ESP32-S3 / 8MB Flash，适用于 **M5Stack Cardputer 与 Cardputer ADV**（两者共用同一构建）
构建：PlatformIO + pioarduino `espressif32` 55.3.39，Arduino-ESP32 3.3.9

---

## 一、文件清单

| 文件 | 大小 | 用途 |
|---|---|---|
| `Launcher-Cardputer-ADV-3.0.0-zh_合并.bin` | 1,507,584 B | **0x0 全量镜像，推荐直接用这个** |
| `firmware_app.bin` | 1,442,048 B | 仅 app 镜像，写到 `0x10000` |
| `bootloader.bin` | 19,984 B | `0x0` |
| `partitions.bin` | 3,072 B | `0x8000`（本版已改为 2MB app0，见第四节） |
| `翻译对照表.tsv` | 295 条 | 英→中对照，可直接增删 |
| `zh_lang.h` / `zh_font.h` | — | 生成的查表 + 12px 点阵字库（264 字） |
| `ardgfx.cpp` | — | 改动过的显示后端（含中文绘制层） |

SHA256：

```
合并镜像    FBD751D3D4C386F5A88E77FB46E6C801FAB4775B9792F5DB01521B7B10F649B3
firmware    F7B7DFBD97BBB8EDEB187A27FD8BAED1FA02FA3CDB3330AB67BA9A894BC1624C
bootloader  4C4B11A021D8C4E98A6172675B1BF34C4AA228564E8A3B2BC73F9A53624C1492
partitions  28D2980356DB0188F4A8EB71631B0C75B343DDCB48A37096A9B77A4B789FB407
```

---

## 二、刷写

```bash
esptool.py --chip esp32s3 -p COM5 -b 460800 write_flash \
  --flash_mode dio --flash_freq 80m --flash_size 8MB \
  0x0 "Launcher-Cardputer-ADV-3.0.0-zh_合并.bin"
```

或 M5Burner → 「自定义固件」选这个 bin → 一键烧写。

**注意**：分区表变了（app0 从 1.35MB 扩到 2MB），必须刷 **0x0 全量镜像**，不要只刷 app。
刷之前原有 Launcher 里装过的 app 会一并清掉，重装即可。

---

## 三、汉化做了什么

官方源码**没有任何多语言机制**，显示库也只支持 8px 英文点阵。因此不是"替换字符串"那么简单，实际做了三层：

**1. 查表层（`zh_lang.h`，295 条）**
加在显示后端的最底层：所有 `tft->print()` / `drawString()` / `drawCentreString()` / `drawRightString()` / `textWidth()` 拿到的字符串，先过一遍英中查表，命中就换中文。好处是**不改 Launcher 业务代码**，菜单、弹窗、状态条、安装流程自动全部生效；未命中的字符串原样显示英文，不会乱码或崩溃。
另外做了"最长前缀匹配"，所以 `Installing xxx`、`Backing up xxx`、`Restored to xxx` 这类拼接串也能换成"正在安装 xxx"。

**2. 字库层（`zh_font.h`，264 字 / 12×12 点阵）**
从 Windows 自带 `Microsoft YaHei UI` 在 12px 下单色渲染，阈值二值化，只收录翻译表里真正出现的字，共 23 KB。
字库和查表都是二分查找（码点有序），147 个点/字按行程填充绘制，单字约 0.1 ms 级。

**3. 布局层**
Launcher 的字宽模型是"每字节 6px"——一个汉字（2 字节）正好 12px，与 12×12 点阵**天然对齐**，所以居中、右对齐、翻页算宽度都不用改公式。
中文绘制固定 12px（不随 `setTextSize(2)` 放大），避免 24px 汉字压到相邻行；菜单行高本来就是 16px + 4px 间距，12px 汉字放得下。

---

## 四、与官方版的差异（除汉化外）

| 项 | 官方 3.0.0 | 本版 | 为什么 |
|---|---|---|---|
| 分区表 | `app0 = 0x150000`（1.35MB） | `app0 = 0x200000`（2MB） | 官方英文版已用到 1.35MB 的 104%，加字库必然溢出；扩到 2MB 后占用 68.8%，还剩 ~600KB |
| WebUI 资源 | 在线调用 toptal 压缩 API | 本地直接 gzip（不压缩） | 断网环境可编译；代价是体积略大，已由扩分区吸收 |
| 库子模块 | `git submodule` | 已铺进 `lib/` | 无 git 联网环境可编译 |

Web 管理界面（浏览器里的那套 HTML）**仍是英文**——它在电脑/手机浏览器里渲染，不是设备屏幕，翻译它对本机使用没有影响，所以没动。

---

## 五、verification 与已知限制

已验证：
- 编译零错误，`merge_bin` 成功，镜像 1,507,584 B
- 二进制内可定位到 UTF-8 中文串：`主菜单`、`在线安装`、`返回`、`固件列表获取失败`
- 字库 264 字、查表 295 条，全部落在已生成数据内（无缺字）

**尚未在真机上验证**（我没有你的设备）：中文的实际观感、个别界面的对齐偏移需要你上手看。可能遇到的两类小问题：

1. **8px 小字行里的中文**（少数状态行用 `_fp=1`，行高 8px）：12px 汉字会轻微压到下一行。若某处明显，把 `zh.tsv` 里那条改成更短的中文（如"启动失败"→"失败"）最省事。
2. **超长中文被截断**：翻页/滚动逻辑按字节数估算宽度，中文偶尔会在尾部少显示一个字。

串口控制台（USB 连电脑敲 `help` 那套）保持英文——那是给终端用的。

---

## 六、以后想自己加词条

工具都在 `C:\launcher`：

```powershell
# 1) 改翻译表（Tab 分隔：英文<TAB>中文）
notepad C:\launcher\zh.tsv
# 2) 重新生成查表 + 收集用到的汉字
& python C:\launcher\gen_lang.py
# 3) 重新生成 12px 点阵字库（会自动出预览图 font_preview.png）
& pwsh -NoProfile -File C:\launcher\gen_font.ps1
# 4) 编译
$env:PLATFORMIO_CORE_DIR='C:\pio\core'
& python -m platformio run -e m5stack-cardputer -d 'C:\launcher\Launcher-3.0.0'
```

增量编译约 2 分钟（只改字库/查表），改 `build_flags` 才会全量（约 30 分钟）。
生成物：`C:\launcher\Launcher-3.0.0\Launcher-m5stack-cardputer.bin`

---

## 七、回滚

刷回官方英文版（从 GitHub Releases 下 `Launcher-m5stack-cardputer.bin`，同样写 0x0），或刷回你之前的 Bruce 合并镜像：

```bash
esptool.py --chip esp32s3 -p COM5 -b 460800 write_flash --flash_mode dio --flash_freq 80m --flash_size 8MB 0x0 "Bruce-CardputerADV-1.17-zh-CC1101-NRF24-IR_合并.bin"
```

注意 Bruce 那个镜像用的是 0x4E0000 的 app 分区，写 0x0 会连分区表一起换回来，属正常。
