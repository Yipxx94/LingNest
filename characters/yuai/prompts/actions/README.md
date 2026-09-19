# 动作生成提示词

这些提示词用于以后重生成或扩展素材。每次调用都应至少附上：

1. `references/canonical-base.png`（authoritative identity reference）
2. 对应的 `references/layout-guides/*.png`（仅作槽位布局，不允许画出导线）
3. 必要时附一张最相关的真实照片

一次生成一整条动作帧，不要逐帧单独生成。图片生成器若支持原生透明背景，应要求 genuine transparent background；若流水线需要色键，则统一使用纯 `#FF00FF`，之后再确定性去除。

`avatar` 和 `icons` 当前成品由已批准主视觉确定性裁切得到，提示词只用于需要重新设计构图时。
