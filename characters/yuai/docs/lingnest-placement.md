# LingNest 项目放置说明

项目根目录：`D:\WorkSpace\Personal-Project\LingNest`

本素材包放置于：

```text
LingNest/
  characters/
    yuai/
      assets/             # 运行时必需；加入 Qt Resource System
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
      docs/               # 开发与接入说明；不打包
      prompts/            # 后续重生成提示词；不打包
      references/         # 身份锚点和照片参考；不打包
      decoded/            # 原始生成动作条；不打包
      frames/             # 流水线拆帧中间件；不打包
      final/              # 内部 QA 图集；不打包
      qa/                 # 总览、预览和校验结果；不打包
      README.md
      pet_request.json
      imagegen-jobs.json
```

## 后续代码目录

开始写代码后，建议在 LingNest 根目录增加：

```text
src/pet/
  PetAnimationCatalog.h
  PetAnimationCatalog.cpp
  PetStateController.h
  PetStateController.cpp

qml/pet/
  PetWindow.qml
  PetSprite.qml
  PetStateMachine.qml
```

`PetAnimationCatalog` 读取 `characters/yuai/assets/manifest.json`；`PetStateController` 处理状态优先级和动作切换；QML 文件负责窗口、图像显示、计时与切帧。

## Qt Resource System

只将 `characters/yuai/assets/` 加入 QRC，不要递归打包整个 `characters/yuai/`：

```cmake
file(GLOB_RECURSE YUAI_ASSET_FILES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/characters/yuai/assets/*")

qt_add_resources(${PROJECT_NAME} yuai_assets
    PREFIX "/characters/yuai"
    BASE "${CMAKE_CURRENT_SOURCE_DIR}/characters/yuai"
    FILES ${YUAI_ASSET_FILES}
)
```

运行时 URL：

```text
qrc:/characters/yuai/assets/manifest.json
qrc:/characters/yuai/assets/daze_prone/yuai_daze_prone_00.png
```

清单中的每个文件路径都以 `assets/` 开头，因此代码应在其前面拼接 `qrc:/characters/yuai/`。不要在代码中硬编码帧数和时长，以 `manifest.json` 为准。
