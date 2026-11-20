# 截图目录（占位）

这里用于放**实机拍摄/截屏**的界面照，供根目录 README 引用。目前为空，等真机照片。

---

## 一、命名规范

`<固件>-<界面>-<可选说明>.png`，全小写、连字符分隔：

```
bruce-mainmenu.png            Bruce 主菜单
bruce-rf-bruteforce.png       RF 暴力破解（显示协议/位数/进度）
bruce-rf-unilarm.png          UNILARM 拨码（地址段 + 按键位）
bruce-emv-card.png            EMV 读卡（含卡组织识别）
launcher-mainmenu.png         Launcher 主菜单（SD/OTA/WUI/CFG/APP）
launcher-sd-list.png          Launcher 从 SD 装固件
launcher-settings.png         Launcher 设置页
launcher-webui.png            浏览器里的 WebUI（可选）
```

## 二、格式与尺寸

| 项 | 建议 |
|---|---|
| 格式 | PNG（无压缩损失的界面截图）；手机拍的用 JPG 也行，命名后缀对应改 |
| 分辨率 | 原机 240×135，截屏请**放大 3–4 倍**（最近邻，别用双线性，否则点阵字发虚）；手机拍整机也接受 |
| 体积 | 单张 ≤ 500 KB，超大就压一下 |
| 数量 | 每个固件 3–6 张，别把整个菜单树都拍一遍 |

## 三、怎么放进 README

根 README 的「截图」段落预留了位置，替换成：

```markdown
| Bruce 主菜单 | RF 暴力破解 | UNILARM 拨码 |
|---|---|---|
| ![主菜单](docs/screenshots/bruce-mainmenu.png) | ![暴力破解](docs/screenshots/bruce-rf-bruteforce.png) | ![UNILARM](docs/screenshots/bruce-rf-unilarm.png) |

| Launcher 主菜单 | SD 安装 | 设置 |
|---|---|---|
| ![主菜单](docs/screenshots/launcher-mainmenu.png) | ![SD](docs/screenshots/launcher-sd-list.png) | ![设置](docs/screenshots/launcher-settings.png) |
```

表格并排比单张竖着堆好看，也省滚动。

## 四、想拍哪几张（建议清单）

优先拍**只有你这个固件才有**的界面，通用界面价值低：

- [ ] Bruce 主菜单（证明中文生效）
- [ ] `RF → Bruteforce` 菜单（协议/位数/重复次数中文标签 + 十六进制进度）
- [ ] `RF → UNILARM (DIP)`（地址段 16 段、按键位 4 选 1）
- [ ] `RFID → Read EMV`（卡组织名兜底显示）
- [ ] Launcher 主菜单（SD/OTA/WUI/CFG 中文）
- [ ] Launcher 设置页（亮度/息屏/界面颜色中文）

拍完直接拖进本目录，按上面的命名规则改名即可，我来更新根 README 的引用。
