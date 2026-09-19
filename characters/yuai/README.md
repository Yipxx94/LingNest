# 渝爱 AI 桌宠第一阶段素材包

本目录包含一套可直接供 Qt 6 / QML 桌宠原型使用的透明 PNG 素材，以及参考图筛选、角色视觉规范、命名与尺寸规范、生成提示词和 QA 产物。原始目录 `C:\Users\xxye01\Downloads\yuai` 全程只读，未覆盖或改名。

## 可直接接入的内容

- `assets/idle/`：6 帧待机/呼吸
- `assets/blink/`：6 帧独立眨眼
- `assets/happy/`：5 帧开心反应
- `assets/thinking/`：6 帧思考
- `assets/talk/`：6 帧口型循环
- `assets/wave/`：4 帧挥手
- `assets/daze_prone/`：6 帧趴伏发呆、呼吸与慢眨眼循环
- `assets/sleep/`：8 帧入睡与呼吸
- `assets/walk_left/`：8 帧向左行走
- `assets/walk_right/`：8 帧向右行走
- `assets/avatar/`：512 与 256 像素头像
- `assets/icons/`：256、128、64、48、32 像素图标
- `assets/manifest.json`：QML 可读的帧数、时长、文件路径与规格

所有动作帧均为 `192×208`、RGBA、透明背景 PNG，命名规则为 `yuai_<action>_<frame>.png`。

## 关键文档

- `docs/visual-spec.md`：角色视觉特征与必须保持的辨识点
- `docs/asset-plan.md`：第一阶段清单、目录、尺寸、命名和时序
- `docs/qml-integration.md`：Qt/QML 接入建议
- `references/selection.md`：20 张本地图的筛选结果与 7 张主参考图说明
- `references/papa/`：新增趴姿参考图、审片表与筛选说明
- `prompts/actions/`：每个动作与头像/图标的可复用生成提示词

## QA 与可追溯内容

- `qa/contact-sheet.png`：所有动作的总览图
- `qa/action-previews/`：使用最终动作名的逐动作 GIF 预览
- `qa/previews/`：生成流水线内部状态名的 GIF 预览
- `qa/review.json`：结构检查结果
- `references/source-review/`：20 张原始参考图的分辨率、哈希、清晰度统计与总览图
- `decoded/`、`frames/`、`final/`：生成条带、拆帧和中间图集，便于后续修订

`final/spritesheet.*` 是内部 QA 图集，行名沿用生成流水线状态名；实际 Qt/QML 代码应以 `assets/manifest.json` 和 `assets/` 中的动作名为准。

## 当前范围

这是第一阶段原型素材，目标是验证桌面显示、交互状态切换和角色一致性。`daze_prone` 建议在持续无操作后进入，并在点击、拖拽、对话或移动开始时立即退出。素材没有加入服装或永久配饰；竹子、胡萝卜、玩偶等都被视为临时场景道具。发布或商业使用前，仍需确认“渝爱”形象、照片和衍生素材的授权范围。
