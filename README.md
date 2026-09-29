# LingNest

LingNest 是一个面向 Windows 10/11 的多角色 AI 桌宠框架。首个角色是熊猫「渝爱」，但角色数据、动画、人格与应用核心从一开始就保持解耦。

当前已完成工程骨架、桌宠窗口、动画状态机、桌面交互、轻量对话、AI Provider 与 SQLite 记忆：应用会加载配置化的渝爱角色包，显示透明、无边框、置顶的动态桌宠，支持平滑拖动、屏幕边界约束、位置持久化、点击反馈、右键菜单、贴近桌宠的对话输入、OpenAI 兼容服务以及跨重启的对话与长期记忆。

## 首个发布候选

当前源码版本号为 `v0.1.3`（候选版，改进菜单选中对比度、底部操作栏悬停稳定性及首次对话失焦关闭）。Windows x64 首选交付物是单文件安装程序；同时提供完整的免安装 ZIP。两种形式都由 Release 构建、`windeployqt`、应用本地 VC++ 运行库、角色运行资源和用户文档组成。构建过程会拒绝 API Key、用户配置、数据库、日志和调试符号进入发布包，并同时生成 SHA-256 与机器可读清单。

## 下载

- **v0.1.3（候选版）**：[安装包与便携包](packages/v0.1.3/) · [更新说明](docs/release-notes-v0.1.3.md)
- **v0.1.2（候选版）**：[安装包与便携包](packages/v0.1.2/) · [更新说明](docs/release-notes-v0.1.2.md)
- **v0.1.1（已发布版本）**：[GitHub Release 下载页](https://github.com/Yipxx94/LingNest/releases/tag/v0.1.1) · [更新说明](docs/release-notes-v0.1.1.md)
- **v0.1.0（历史版本）**：[GitHub Release 下载页](https://github.com/Yipxx94/LingNest/releases/tag/v0.1.0) · [更新说明](docs/release-notes-v0.1.0.md)

推荐普通用户下载 `v0.1.1` 安装版。`v0.1.3` 候选包及校验文件保存在 [`packages/v0.1.3/`](packages/v0.1.3/)；正式 GitHub Release 仍待跨环境签收。已发布版本文件、SHA-256 校验值和清单统一维护在 [`packages/`](packages/)；`v0.1.0` 发布时尚未提供安装版。

本机只有 Qt 5.15.2 兼容工具链，因此首包仍是兼容构建；Qt 5 的公网 HTTPS 依赖已经停止维护的 OpenSSL 1.1.1，本项目不会从其他软件目录拼装该依赖。正式联网发行前应使用 Qt 6 重建并在干净 Windows 上签收。本地 HTTP 兼容服务与其余桌宠能力不受此限制。

已实现的窗口行为：

- 仅显示渝爱透明 PNG，窗口大小随角色 `scale` 调整
- Windows 工具窗口，无普通标题栏和任务栏按钮
- 默认始终置顶
- 左键拖动，释放时保存位置
- 使用原生窗口移动避免拖动时闪烁或漂移
- 保存屏幕名称、绝对位置和相对位置
- 显示器或 DPI 布局变化后将位置恢复到可见工作区
- 同一 Windows 用户会话只允许运行一个 LingNest 实例
- Windows 通知区域显示“开放巢＋对话种子”LingNest 品牌图标，左键唤回桌宠、双击打开聊天
- 托盘右键菜单提供聊天、显示/隐藏桌宠、切换角色、AI 设置、互动、暂停活动与安全退出

已实现的动画与行为能力：

- 从角色动画清单加载逐帧 URL、独立帧时长和默认循环设置
- `IAnimationPlayer` 抽象与 PNG Sequence 播放器实现
- `Idle`、`Blink`、`Walk`、`Talk`、`Happy`、`Wave`、`Daze`、`Sleep`、`Thinking` 状态机
- Blink、Happy、Talk、Wave 单次播放后自动回到 Idle
- Thinking 持续到收到回复；Walk、Daze 与 Sleep 会被用户活动打断
- 每 3～7 秒尝试自然眨眼，每 30～90 秒尝试趴伏发呆，每 60～120 秒尝试散步
- 散步期间窗口按约 48 像素/秒真实移动，碰到当前屏幕工作区边缘时自动转向并切换动画
- 连续 15 分钟无操作后进入 Sleep；入睡前导帧只播放一次，之后仅循环睡眠呼吸帧
- 应用启动后会播放一次 Wave，方便确认短动作链路
- 普通行为完全本地决定，不调用 LLM

已实现的交互与聊天能力：

- 单击进入 Happy 并显示随机本地气泡，双击打开贴近桌宠的对话输入条
- 拖动阈值与双击延时仲裁，避免拖动误触点击
- 右键菜单采用紧凑的浅色圆角浮层，提供对话、AI 设置、角色状态、暂停/继续与退出
- 右键菜单的“动作预览”子菜单可直接预览待机、眨眼、开心、挥手、趴伏、左右行走和睡眠
- 气泡作为独立透明窗口，跟随桌宠、自动避开屏幕边缘并定时隐藏
- 对话输入条无标题栏、自动跟随桌宠并钳制在当前屏幕内；Enter 发送，Esc 收起
- 发送后输入条立即收起，桌宠先显示动态思考气泡，再直接用回复气泡回答，不打开传统消息列表窗口
- `IAIProvider` 隔离聊天用例与具体模型服务，当前实现兼容 Chat Completions API
- 角色独立的 `prompt.md` 会作为 system message 注入，请求在 GUI 线程上异步完成
- 支持请求取消、超时、网络、HTTP、API 与 JSON 格式错误，并通过桌宠气泡显示可理解的错误
- 对话历史保存在本地 SQLite，关闭并重启后会恢复最近消息
- “记住……”指令会保存为结构化长期记忆；姓名、生日与所在地会进入用户资料
- Prompt 按角色人格、长期记忆、最近对话和当前消息的固定顺序构建，并在字符预算内裁剪
- 对话控制器保留清空能力；清空对话记录不会误删用户明确要求长期记住的内容

## 配置 AI 服务

右键桌宠并选择“设置”，填写服务的 Base URL、模型、Temperature、Max Tokens、超时时间和 API Key，然后保存。Base URL 可以是服务根路径（例如包含 `/v1` 的地址），程序会自动追加 `/chat/completions`；也可以直接填写完整的 Chat Completions 地址。

模型参数保存在 Qt 标准应用配置目录的 `config.json` 中。API Key 不会出现在该文件、源码或日志里，而是通过 `ISecretStore` 保存到当前 Windows 用户的 Credential Manager。设置页不会回显已有密钥；密钥输入框留空表示继续使用原密钥。

## 技术基线

- C++20
- Qt 6（Core、Gui、Qml、Quick）
- CMake 3.21+
- Ninja
- MSVC v143 / Visual Studio 2022 工具集
- Windows 10/11

工程优先查找 Qt 6。为便于尚未安装 Qt 6 的开发机验证骨架，CMake 暂时允许回退到 Qt 5.15；这不是产品发布目标。

## 用 Qt Creator 构建

1. 用 Qt Creator 打开根目录的 `CMakeLists.txt`。
2. 选择 64 位 Qt 6 + MSVC v143 Kit。
3. 配置并构建 `LingNest` 目标。
4. 运行 `LingNest`。

## 命令行构建

先进入已加载 MSVC v143 环境的 Developer PowerShell，并确保 Qt 6 的 `bin` 在 `PATH` 中、Qt 安装前缀能被 CMake 找到。若没有配置全局前缀，可以在首次配置时传入：

```powershell
cmake --preset debug -DCMAKE_PREFIX_PATH=C:/Qt/6.x.x/msvc2022_64
cmake --build --preset debug
ctest --preset debug
```

启动程序：

```powershell
./build/debug/LingNest.exe
```

生成 Windows Release 安装程序与便携包（需要 Inno Setup 6）：

```powershell
cmake --preset release -DCMAKE_PREFIX_PATH=C:/Qt/6.x.x/msvc2022_64
cmake --build --preset release
ctest --preset release
cmake --build --preset release --target package_installer
```

可直接交付的产物位于仓库根目录的 `dist/`。推荐分发其中的 `*-setup.exe`；便携用户必须完整解压 `*-portable.zip` 后运行。`build/release/LingNest.exe` 只是开发构建产物，不能单独复制到其他电脑。

## 角色包

运行时角色位于 `characters/<character-id>/`。渝爱的入口文件是：

- `characters/yuai/character.json`：身份、资源入口与可用动画
- `characters/yuai/prompt.md`：独立人格提示词
- `characters/yuai/assets/manifest.json`：动画帧和时序的唯一清单
- `characters/yuai/assets/`：透明 PNG、头像与图标

动画清单沿用已有的 `assets/manifest.json`，不另建内容重复的 `animations.json`，避免两份时序数据逐渐不一致。
`character.json` 中的 `stateAnimations` 将通用桌宠状态映射到角色自己的动作名，因此未来角色不必复制渝爱的文件命名。

## 文档

- [架构设计](docs/architecture.md)
- [MVP 路线图](docs/roadmap.md)
- [用户指南](docs/user-guide.md)
- [隐私说明](docs/privacy.md)
- [第三方组件与素材记录](docs/third-party-notices.md)
- [v0.1.3 候选发布说明](docs/release-notes-v0.1.3.md)
- [v0.1.3 验证记录](docs/release-validation-v0.1.3.md)
- [v0.1.2 候选发布说明](docs/release-notes-v0.1.2.md)
- [v0.1.2 验证记录](docs/release-validation-v0.1.2.md)
- [v0.1.1 已发布版本说明](docs/release-notes-v0.1.1.md)
- [v0.1.0 历史发布说明](docs/release-notes-v0.1.0.md)

## 本地数据与密钥

运行时配置、数据库、日志和密钥不进入 Git。窗口位置与非敏感 AI 参数以原子写入方式保存在 Qt 标准应用配置目录的 `config.json` 中；聊天记录、结构化长期记忆与用户资料保存在同一应用数据目录的 `memory.sqlite3` 中。API Key 通过独立凭据接口保存到 Windows Credential Manager，不会写入源码、角色包或普通配置文件。

## 开源许可

LingNest 使用 [MIT License](LICENSE) 开源。
