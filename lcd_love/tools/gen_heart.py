# -*- coding: utf-8 -*-
"""
生成心形轮廓点表（供 User/heart_table.h 使用）

当前使用参数方程：
    x = 16 sin^3 t
    y = 13 cos t - 5 cos 2t - 2 cos 3t - cos 4t
（顶部双凸、底部尖角效果好；两侧略偏直）

想进一步优化两侧弧度时，把下面的采样部分换成贝塞尔曲线即可，
例如用 4 段三次贝塞尔：
    顶部凹陷 → 左最宽点 → 底部尖端 → 右最宽点 → 回到顶部凹陷
控制点决定两侧的弧度，改完重新运行本脚本 + 重新编译。

运行：python tools/gen_heart.py
输出：../User/heart_table.h（并打印 ASCII 预览）
"""
import math
import os

N = 72          # 采样点数
PREVIEW_R = 22  # ASCII 预览用的半宽


def raw(t):
    x = 16.0 * math.sin(t) ** 3
    y = (13.0 * math.cos(t) - 5.0 * math.cos(2 * t)
         - 2.0 * math.cos(3 * t) - math.cos(4 * t))
    return x, y


pts = [raw(2 * math.pi * i / N) for i in range(N)]
scale = 16.0
xs = [p[0] / scale for p in pts]
ys = [p[1] / scale for p in pts]
y_top, y_bot = max(ys), min(ys)

# ---------- ASCII 预览（与固件相同的扫描线填充） ----------
rows = int((y_top - y_bot) * PREVIEW_R)
for row in range(rows):
    yu = y_top - row / PREVIEW_R
    cross = []
    for i in range(N):
        j = (i + 1) % N
        y1, y2 = ys[i], ys[j]
        if (y1 <= yu < y2) or (y2 <= yu < y1):
            t = (yu - y1) / (y2 - y1)
            cross.append(xs[i] + t * (xs[j] - xs[i]))
    cross.sort()
    line = ["."] * (2 * PREVIEW_R + 1)
    for a in range(0, len(cross) - 1, 2):
        x1 = int(round(cross[a] * PREVIEW_R)) + PREVIEW_R
        x2 = int(round(cross[a + 1] * PREVIEW_R)) + PREVIEW_R
        for c in range(max(0, x1), min(2 * PREVIEW_R, x2) + 1):
            line[c] = "#"
    print("".join(line))

# ---------- 输出点表 ----------
out = []
out.append("/* 心形轮廓点表（由 tools/gen_heart.py 生成）")
out.append(" * 参数方程：x = 16 sin^3 t, y = 13 cos t - 5 cos2t - 2 cos3t - cos4t")
out.append(" * 归一化：x / 16，y / 16；x ∈ [%.4f, %.4f]，y ∈ [%.4f, %.4f]"
           % (min(xs), max(xs), y_bot, y_top))
out.append(" */")
out.append("#ifndef __HEART_TABLE_H")
out.append("#define __HEART_TABLE_H")
out.append("")
out.append("#define HEART_N %d" % N)
out.append("static const float g_heartX[HEART_N] = {")
for i in range(0, N, 8):
    out.append("    " + ", ".join("%+.5ff" % v for v in xs[i:i + 8]) + ",")
out.append("};")
out.append("static const float g_heartY[HEART_N] = {")
for i in range(0, N, 8):
    out.append("    " + ", ".join("%+.5ff" % v for v in ys[i:i + 8]) + ",")
out.append("};")
out.append("#define HEART_Y_TOP (%.5ff)" % y_top)
out.append("#define HEART_Y_BOT (%.5ff)" % y_bot)
out.append("")
out.append("#endif")
out.append("")

here = os.path.dirname(os.path.abspath(__file__))
path = os.path.join(here, os.pardir, "User", "heart_table.h")
with open(path, "w", encoding="utf-8") as f:
    f.write("\n".join(out))
print("\n已生成:", os.path.normpath(path))
