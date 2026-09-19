# LingNest 架构设计

## 设计目标

LingNest 是多角色桌宠框架，而不是围绕「渝爱」写死的单角色程序。MVP 保持实现克制，同时保留清楚的替换边界：角色包可新增，动画播放器可替换，AI Provider 可替换，记忆检索可演进，Windows 能力集中封装。

核心原则：

- QML 负责展示与轻量视觉状态，C++ 负责业务规则、I/O 和生命周期。
- 依赖由应用组合根注入，不使用全局可变状态或万能 Singleton。
- 角色身份、人格、动画清单和素材全部数据化。
- 网络和 SQLite 工作不得阻塞 GUI 线程。
- MVP 只实现当前用例，不提前建设插件市场、复杂 Agent 或行为树。

## 目录结构

```text
LingNest/
├─ CMakeLists.txt
├─ CMakePresets.json
├─ src/
│  ├─ app/             # 组合根、应用生命周期、依赖装配
│  ├─ core/            # 日志、事件、时钟与调度基础设施
│  ├─ pet/             # PetState、有限状态机、桌宠实体与行为规则
│  ├─ character/       # 角色模型、角色包校验与 CharacterLoader
│  ├─ animation/       # IAnimationPlayer、动画目录与播放控制
│  ├─ interaction/     # 点击、双击、拖动、菜单等用户意图
│  ├─ ai/              # IAIProvider、上下文构建、兼容 OpenAI 的实现
│  ├─ memory/          # SQLite 仓储、会话和长期记忆接口
│  ├─ config/          # 默认值、本地配置与敏感配置抽象
│  ├─ platform/
│  │  └─ windows/      # DPI、屏幕、凭据、开机启动等 Windows 能力
│  └─ plugin/          # 未来扩展接口；MVP 不加载第三方插件
├─ qml/
│  ├─ Main.qml         # 仅作为 QML 入口
│  ├─ windows/         # PetWindow、对话输入浮层、气泡与 SettingsWindow
│  └─ components/      # PetSprite、消息项、气泡等可复用视图
├─ characters/
│  └─ yuai/
│     ├─ character.json
│     ├─ prompt.md
│     └─ assets/
│        └─ manifest.json
├─ resources/          # 应用级图标、字体、Windows 资源
├─ tests/              # 按模块组织的单元与集成测试
├─ docs/
└─ config/             # 可提交的默认配置；用户配置写入应用数据目录
```

当前仓库中的 `references/`、`prompts/`、`decoded/`、`frames/`、`final/` 和 `qa/` 是美术生产与验收资料，不会打包进应用。运行时只需要角色入口文件、人格 prompt 和 `assets/`。

## 模块边界

### App 与 Core

`app` 是唯一组合根，创建并连接长生命周期对象。`core` 提供日志、事件和可替换时钟等小型公共能力，不承载角色或 UI 业务，也不变成服务定位器。

### Character

`CharacterLoader` 输入角色目录，校验 `character.json`、人格文件和动画清单，输出不可变的 `CharacterDefinition`。核心代码只按角色 ID 工作；新增 `cat`、`fox` 或 `capybara` 不需要修改状态机。

角色包路径在开发时来自磁盘，发布时可打入 Qt Resource System。加载层对上层暴露统一 URL，因此调用方不关心来源是文件还是 `qrc`。

### Pet 与 Animation

`PetState` 当前包含 `Idle`、`Blink`、`Walk`、`Talk`、`Happy`、`Wave`、`Daze`、`Sleep`、`Thinking`。眨眼与挥手虽然短暂，但进入显式状态以便统一处理中断、优先级与播放完成回退。

`PetStateMachine` 是可单测的 C++ 有限状态机，只处理事件、守卫条件和转换，不直接操作 QML。角色包的 `stateAnimations` 把通用状态映射到角色动画名，`IAnimationPlayer` 隔离具体播放器。首版实现 PNG Sequence，并支持从指定帧开始循环，用于“前导动作一次＋稳定姿势循环”。未来的 Live2D/VRM 播放器可复用状态机。

`PetScheduler` 使用独立计时器调度眨眼、散步、趴伏和睡眠，因此长周期行为不会阻塞短周期动作。`PetWindowController` 消费 Walk 信号，以设备无关像素更新窗口位置；抵达屏幕边缘时把新方向反馈给状态机，使移动方向和动画始终一致。

### Interaction 与 UI

QML 捕获鼠标输入并转换为明确意图：点击、双击、拖动开始/移动/结束、右键菜单。C++ Controller 决定业务反应。对话输入浮层与设置窗口都独立于桌宠窗口，关闭它们不会结束应用。

`InteractionController` 使用系统双击间隔延迟提交单击，从而把单击与双击可靠区分；QML 在超过系统拖动阈值后才进入 Windows 原生窗口移动，避免逐帧改坐标造成闪烁与漂移。`PetWindowController` 为菜单和对话浮层提供基于当前屏幕可用区域的统一边界计算。`SpeechBubbleController` 管理独立透明回复气泡的位置、屏幕边界翻转、思考状态与隐藏计时。`SystemTrayController` 属于应用层，只使用 LingNest 品牌图标和角色无关操作；当前角色名称通过运行时状态注入，切换角色、对话、暂停等操作复用 `InteractionController` 的业务入口，退出时先移除托盘图标。`ChatController` 保留内部 `ChatMessageModel` 与持久化历史，并通过结果信号直接驱动桌宠回复气泡；它使用注入的 `IAIProvider`、`MemoryRepository` 与 `PromptBuilder` 异步发送有界上下文。清空对话会取消在途请求并删除会话记录，但保留长期记忆；成功与失败分别驱动桌宠退出 Thinking 状态。

### AI

`IAIProvider` 以请求 ID 返回异步结果并支持取消。`OpenAICompatibleProvider` 使用 `QNetworkAccessManager` 调用 Chat Completions，覆盖配置校验、超时、网络错误、HTTP 错误、API 错误和 JSON 校验。每个请求使用独立超时计时器，完成、取消与超时只结算一次。API Key 通过凭据接口取得，不进入普通 JSON、日志或请求诊断文本。

`AISettingsController` 是设置窗口与配置/凭据实现之间的边界：普通模型参数由 `ConfigManager` 原子保存，密钥由 `ISecretStore` 单独保存。Windows 的 `WindowsCredentialStore` 使用 Credential Manager；QML 只能看到“是否已配置”，不能读取现有密钥。

上下文由 `PromptBuilder` 组合：角色 prompt、相关长期记忆、最近 N 条对话和本轮用户消息。普通桌宠动作由本地规则决定，不为每个动作调用模型。

### Memory

`MemoryRepository` 隔离 SQLite，每个实例使用唯一连接名，迁移在启动阶段串行执行。MVP 表包括 `conversation`、`memory` 与 `user_profile`，所有查询均参数化；WAL、外键和忙等待在连接建立时统一配置。`ExplicitMemoryParser` 只处理明确的“记住……”表达，将偏好、关系、事件和事实规范化并去重，同时提取姓名、生日和所在地等用户资料。`PromptBuilder` 按固定层级构建上下文，在硬字符预算内优先选择相关长期记忆和较新的对话。未来语义检索可通过新的检索实现扩展，不改变聊天用例。

### Config 与 Platform

非敏感配置写入 Qt 标准应用数据目录，包括角色、窗口位置、置顶、主动互动和模型参数。API Key 由 `ISecretStore` 管理；Windows 实现使用 Credential Manager。

Qt 6 以设备无关像素处理窗口坐标。位置记录同时保存屏幕标识和相对可用区域的位置；恢复时若屏幕缺失或布局变化，会把窗口钳制到当前屏幕可用区域。Windows 特有的任务栏行为、DPI 与开机启动集中在 `platform/windows`。

## 运行时依赖方向

```text
QML views -> UI/Interaction controllers -> application use cases
                                      -> Pet / Character / Animation interfaces
                                      -> AI / Memory / Config interfaces

app composition root -> concrete Qt, SQLite and Windows implementations
```

领域状态机不依赖 QML、网络或 SQLite。具体基础设施依赖接口，应用组合根负责连接两端。

## 错误与日志

可恢复错误通过结构化结果返回给调用方，用户可理解的消息与技术诊断分离。Logger 支持 DEBUG、INFO、WARNING、ERROR，按日期写入本地日志目录；Release 默认过滤 Debug，并对 API Key、Authorization header 和用户敏感内容脱敏。

## 测试策略

- `CharacterLoader`：合法包、缺失文件、非法路径和清单不一致。
- `ConfigManager`：默认值、迁移、损坏文件和位置恢复。
- `PetStateMachine`：用户事件、超时、短动作恢复和非法转换。
- `InteractionController`：单击延迟、双击抑制、暂停与退出信号。
- `SystemTrayController`：菜单内容、动作路由以及暂停/可见状态同步。
- QML 窗口：点击/拖动/右键路由、气泡边缘翻转、聊天窗实际渲染。
- `ChatController`：异步发送、清空取消、空消息过滤、会话恢复与显式记忆注入。
- `OpenAICompatibleProvider`：请求格式、成功响应、配置错误、取消、超时、传输错误、HTTP/API 错误与非法 JSON。
- `AISettingsController`：参数持久化、输入校验以及普通 JSON 与安全凭据的隔离。
- `MemoryRepository`：迁移、事务、重开持久化、最近对话和清空隔离。
- `PromptBuilder`：顺序、预算、裁剪与敏感数据边界。
- 启动冒烟测试：加载 QML 根对象并立即退出。

首版优先使用 Qt Test，避免仅为测试框架引入 GoogleTest 下载依赖；若后续已有统一 C++ 测试基础设施，再评估切换。
