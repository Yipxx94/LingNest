# 第一阶段素材规划与规格

## 目录结构

```text
yuai-ai-desktop-pet-assets/
  assets/
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
    manifest.json
  docs/
  prompts/actions/
  references/
  qa/
```

## 动作清单

| 动作 | 帧数 | 默认用途 | 循环建议 |
| --- | ---: | --- | --- |
| idle | 6 | 呼吸、微抬头、轻眨眼 | 常驻循环 |
| blink | 6 | 可插入 idle 的独立眨眼 | 随机触发，不连续循环 |
| happy | 5 | 点击、表扬、成功反馈 | 播放 1 次后回 idle |
| thinking | 6 | AI 请求处理中、思考 | 可循环 |
| talk | 6 | TTS/文字回复时的口型 | 回复期间循环 |
| wave | 4 | 启动、问候、主动提醒 | 播放 1～2 次 |
| daze_prone | 6 | 长时间无交互时趴伏发呆 | 可循环；任意交互立即退出 |
| sleep | 8 | 长时间无交互或夜间状态 | 最后 4 帧可循环 |
| walk_left | 8 | 向屏幕左侧移动 | 移动期间循环 |
| walk_right | 8 | 向屏幕右侧移动 | 移动期间循环 |

精确帧时长位于 `assets/manifest.json`。

## 尺寸与锚点

- 动作帧：`192×208 px`，RGBA PNG，透明背景。
- 坐姿与行走基线：建议在 QML 中按单元格底部对齐；清单记录的参考基线为 `y = 198`。
- 安全边距：耳尖、爪尖与毛发边缘距离单元格至少 5 px；不要在 QML 中再次裁剪透明区。
- 主视觉：`references/canonical-base.png`，用于后续所有扩展动作的身份锁定。
- 趴姿主参考：`references/papa/papa-front-prone.jpg`；三分之四与侧面体积参考见同目录。
- 头像：512 与 256 正方形透明 PNG。
- 图标：256、128、64、48、32 正方形透明 PNG。

## 命名规范

- 角色 ID：`yuai`
- 动作目录：小写 snake_case，例如 `walk_left`
- 帧文件：`yuai_<action>_<两位序号>.png`
- 序号从 `00` 开始，按播放顺序递增。
- 皮肤扩展建议：`yuai_<skin>_<action>_<frame>.png`，例如 `yuai_spring_wave_00.png`。
- 不在文件名中使用中文、空格、括号或版本日期，避免 QRC、CMake 和跨平台路径问题。

## 生成与验收规则

- 每个动作都必须引用 `references/canonical-base.png`，不能只靠文字重新生成。
- 同一动作的一整排帧应一次生成，避免逐帧单独生成造成身份跳动。
- 检查帧数、透明度、裁切、体型、耳位、眼斑、基线与动作语义。
- 动画播放时不得出现明显缩放跳变、反向步态、重复静帧或额外道具。
- avatar/icons 从批准的主视觉确定性裁切，避免另行生成产生脸型漂移。

## 后续阶段建议

1. 加入触摸头部、拖拽、跌落/弹起等交互动作。
2. 为 talk 增加音素口型映射（闭口、半开、全开、圆唇）。
3. 增加 16 方向注视帧，供鼠标跟随。
4. 角色稳定后再制作节日皮肤与道具层。
