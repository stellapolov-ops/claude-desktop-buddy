# claude-desktop-buddy

**[English](README.md) | 中文**

> **anthropics 官方 reference 固件的扩展版本** —— 在原版的「按键远程审批」基础上，加入**语音输入 + 多段 draft 累积**。让你**不用键盘**就能给 Claude Code 发指令。

---

## 引入了什么新特性

这是 [anthropics/claude-desktop-buddy](https://github.com/anthropics/claude-desktop-buddy) 的扩展版本。官方 buddy 已经做了"通过 BLE 远程审批 Claude Code 工具调用"（按 A 同意 / B 拒绝），这个 fork 在它的基础上添加四个能力：

### 1. 中文 / 英文语音输入流水线

长按 M5StickC Plus 的 B 键开始录音，松开后：

- 麦克风 PDM 采样（16 kHz 单声道）
- 实时 IMA ADPCM 编码（4:1 压缩，BLE 带宽友好）
- 通过 BLE 上传到 PC 端 m5buddy server
- server 调用 whisper.cpp 做转写（中英文皆可）
- 转写文本通过 Claude Code channel notification 推到对话上下文

**效果**：不用键盘，**对着 M5 说话给 Claude Code 发指令**。

### 2. LCD 预览 + Draft buffer 状态机

录音转写后，**文字在 M5 屏幕上预览**——你能看到 whisper 听写到的内容：

- 听到对了 → **A 短按** append 这段到 draft buffer
- 听错了 → **B 短按** 丢弃这段重录

Draft buffer 支持：

- **多段累积**：例如先录"分析下 logs/error.log"，再录"输出到 /tmp/report.md"
- **A 长按 submit**：把整个 draft 一次性提交给 Claude Code
- **B 长按 discard**：清空整个 draft

**效果**：复杂指令可以**拆分录制**，每段听写完确认无误再继续，**避免"长录音中间错一个字得全部重来"**。

### 3. 审批抢占（Approval preempt）

正在录音时，如果 Claude Code 发来一个**审批请求**（比如刚才已经触发的 Bash 命令），M5 会：

1. **自动暂停录音**
2. **弹出审批 prompt**（按 A/B 决策）
3. 处理完审批后**自动恢复录音**

**效果**：**录音中不会错过关键审批**——这是物理设备和软件 UI 的体验差异（软件可以叠 modal，物理设备必须切换状态）。

### 4. 录音状态可视化 + 硬件自检

底层硬件支持：

- PDM 数字麦 → I2S 接口
- 录音中**屏幕显示状态**（buddy 角色动画变化）
- LED 闪烁 + buzzer 提示音
- 上电做硬件自检（麦克风通路、屏幕、按键）

---

## 跟谁配合用

需要配合 PC 端 server：**[stellapolov-ops/claude-code-m5buddy](https://github.com/stellapolov-ops/claude-code-m5buddy)**（TypeScript + Bun + BLE central + whisper-cli）。

完整数据流：

```
M5StickC Plus                          macOS / PC m5buddy server
─────────────                          ─────────────────────────
长按 B 录音           ─ BLE 上传 →    ADPCM 解码 → WAV
LCD 预览转写文字     ← BLE 推送 ─     whisper 转写
A 短按 append        ─ BLE 通知 →    添加到 draft buffer
A 长按 submit        ─ BLE 通知 →    channel notification → Claude Code 对话
```

---

## 仓库结构

- **`main` 分支**：跟 anthropics 上游同步，便于吸收日后官方更新
- **`phase2-voice-stt-preview` 分支**：本 fork 的扩展功能在这里（**默认显示**）
- **License**：跟上游一致（MIT）

---

## 烧录固件

详细烧录步骤参见 anthropics 原版 README（[英文版](README.md)），固件主体兼容。扩展的语音模块新增了几个源文件（`src/audio/*.cpp`），跟着 PlatformIO 配置自动一起编译。

```bash
# 用 PlatformIO（推荐）
pio run -e m5stick-c-plus -t upload

# 或用 Arduino IDE：选 M5StickC-Plus board，打开 src/main.cpp，编译上传
```

---

## 协议参考

完整 BLE Nordic UART Service 协议字段、JSON schema 在 [REFERENCE.md](REFERENCE.md)。本 fork 在原协议基础上扩展了**音频相关的几个 GATT characteristics + JSON 命令类型**，具体定义见 REFERENCE.md。

---

## License

[MIT](LICENSE) —— 跟 anthropics 上游一致。

---

## 致谢

感谢 [anthropics 团队](https://github.com/anthropics) 把 Claude desktop 的硬件接口开放，给社区一个简单清晰的 BLE 协议参考实现。这个 fork 只是在他们的基础上做了一层"语音输入"的扩展，主体 buddy BLE 框架的设计/实现都是上游团队完成的。
