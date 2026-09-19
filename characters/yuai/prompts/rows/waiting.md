Create one horizontal animation strip for Codex pet `yuai`, state `waiting`.

Use the attached canonical base for identity. Use the attached layout guide only for slot count, spacing, centering, and padding; do not draw the guide.

Output exactly 6 full-body frames in one left-to-right row on flat pure magenta #FF00FF. Treat the row as 6 invisible equal-width slots: one centered complete pose per slot, evenly spaced, with no overlap, clipping, empty slots, labels, or borders.

Identity: same pet in every frame: 真实大熊猫渝爱的半写实柔和绘制桌宠；保留圆而宽的白色脸盘、蓬松双颊、小而圆且高位的黑耳、细长水滴形黑眼圈、短而宽的白鼻梁与黑鼻、圆润敦实体型、黑色四肢与肩带、自然绒毛。无固定服装或配饰；竹子、胡萝卜、玩偶均为临时道具，不进入基础身份。不要过度Q版，不改变眼斑和头脸比例。. Preserve silhouette, face, proportions, markings, palette, material, style, and props.
Style: Pet-safe sprite: compact full-body mascot, readable in a 192x208 cell, clear silhouette, simple face, stable palette/materials, and crisp edges for chroma-key extraction. Style `auto`: Infer the most appropriate pet-safe style from the user request and reference images, then keep that exact style consistent across every row. User style notes: semi-realistic soft-painted fur cutout based closely on the real animal; compact readable silhouette for 192x208 desktop-pet cells; mild simplification only, never chibi, plush, anime, or toy-like; no text, watermark, scenery, shadow, or detached effects.
Animation continuity: keep apparent pet scale and baseline stable within the row unless the state itself intentionally changes vertical position, such as `jumping`. Move the pose within the slot instead of redrawing the pet larger or smaller frame to frame.

State action: Needs-input loop: expectant asking pose for approval, help, or user input.

State requirements:
- Show that Codex needs approval, help, or user input through an expectant asking pose.
- Keep the motion patient and readable, without turning it into ordinary idle or review.

Clean extraction: crisp opaque edges, safe padding, no scenery, text, guide marks, checkerboard, shadows, glows, motion blur, speed lines, dust, detached effects, stray pixels, or chroma-key colors inside the pet.
