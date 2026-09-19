# Qt/QML 接入与项目放置建议

## 推荐项目结构

将本素材包中的整个 `assets/` 目录原样复制到项目的 `resources/pets/yuai/` 下。这样 `manifest.json` 内已有的 `assets/...` 路径不需要重写。

```text
YourDesktopPetProject/
  CMakeLists.txt
  src/
    main.cpp
    pet/
      PetAnimationCatalog.h
      PetAnimationCatalog.cpp
      PetStateController.h
      PetStateController.cpp
  qml/
    Main.qml
    pet/
      PetWindow.qml
      PetSprite.qml
      PetStateMachine.qml
  resources/
    pets/
      yuai/
        assets/
          manifest.json
          idle/
          blink/
          happy/
          thinking/
          talk/
          wave/
          daze_prone/
          sleep/
          walk_left/
          walk_right/
          avatar/
          icons/
  art-source/                 # 可选：进仓库但不打入程序
    yuai/
      references/
      prompts/
      decoded/
      qa/
```

运行时必须放进项目并参与构建的只有素材包中的 `assets/`。`references/`、`prompts/`、`decoded/`、`frames/`、`final/`、`qa/` 和 `docs/` 是美术源文件、生成记录及验收材料，不应加入 QRC；若希望以后可重生成，可复制到项目的 `art-source/yuai/`。

代码职责建议如下：

- `PetAnimationCatalog`：加载并校验 `manifest.json`，把动作名映射为帧 URL 与时长。
- `PetStateController`：管理 idle、发呆、行走、对话、睡眠和短动作的优先级与恢复状态。
- `PetSprite.qml`：只负责显示当前 PNG 帧，不承载业务状态。
- `PetStateMachine.qml`：负责计时、切帧、循环/单次播放和动作结束回调。

## 推荐读取方式

运行时读取 `qrc:/pets/yuai/assets/manifest.json`，按动作取得 `files` 与 `durations_ms`。清单中的文件路径以 `qrc:/pets/yuai/` 为基准拼接，例如 `assets/daze_prone/yuai_daze_prone_00.png` 对应 `qrc:/pets/yuai/assets/daze_prone/yuai_daze_prone_00.png`。使用同一个 `Image` 组件切换 `source`，并保持固定 `width: 192`、`height: 208` 与底部锚点，可避免状态切换时窗口跳动。

```qml
Image {
    id: petFrame
    width: 192
    height: 208
    fillMode: Image.PreserveAspectFit
    smooth: true
    mipmap: true
    source: currentFrameUrl
}
```

## 状态优先级

建议使用：`sleep < idle/daze_prone/blink < walk < thinking < talk < wave/happy`。高优先级短动作结束后回到先前的持续状态。`daze_prone` 可在 30～90 秒无交互后随机进入并循环；点击、拖拽、开始行走、AI 回复或睡眠切换时立即退出。blink 不应单独常驻循环，可每 3～7 秒按概率插入 idle。

## 资源打包

- 开发阶段可直接从磁盘读取 `resources/pets/yuai/assets/`。
- 发布阶段建议将 PNG 与 manifest 加入 Qt Resource System。CMake 可按目录递归收集资源：

```cmake
file(GLOB_RECURSE YUAI_ASSET_FILES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/resources/pets/yuai/assets/*")

qt_add_resources(${PROJECT_NAME} yuai_assets
    PREFIX "/pets/yuai"
    BASE "${CMAKE_CURRENT_SOURCE_DIR}/resources/pets/yuai"
    FILES ${YUAI_ASSET_FILES}
)
```

- 保留 PNG 原始 alpha，不要在构建前转 JPEG。
- Windows 无边框透明窗口中建议开启 `WA_TranslucentBackground`，QML 场景背景设为透明。

## 性能

第一阶段总帧数较少，逐 PNG 加载足够。若以后角色/皮肤变多，可在启动时预加载当前动作与 idle，其他动作按需缓存。不要把所有高分辨率生成条带直接用于运行时；使用 `assets/` 中已归一化的 192×208 帧。
