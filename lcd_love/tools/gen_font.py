# -*- coding: utf-8 -*-
"""
用 Windows GDI 生成汉字点阵字模（无需 Pillow）。
输出格式：行优先，每行 4 字节（32 像素），字节内 bit0 = 最左像素
（与教程 lcd.c 中 LCD_WriteChineseChar 的取位方式一致）
"""
import ctypes
import ctypes.wintypes as wt
import os

SIZE = 32          # 目标点阵 32x32
CANVAS = 64        # 渲染画布
FONT_SIZE = 48
FONT_CANDIDATES = ["SimHei", "Microsoft YaHei", "SimSun", "KaiTi"]
CHARS = "老婆我爱你!"

gdi32 = ctypes.WinDLL('gdi32', use_last_error=True)
user32 = ctypes.WinDLL('user32', use_last_error=True)


class BITMAPINFOHEADER(ctypes.Structure):
    _fields_ = [('biSize', wt.DWORD), ('biWidth', ctypes.c_long), ('biHeight', ctypes.c_long),
                ('biPlanes', wt.WORD), ('biBitCount', wt.WORD), ('biCompression', wt.DWORD),
                ('biSizeImage', wt.DWORD), ('biXPelsPerMeter', ctypes.c_long),
                ('biYPelsPerMeter', ctypes.c_long), ('biClrUsed', wt.DWORD),
                ('biClrImportant', wt.DWORD)]


class BITMAPINFO(ctypes.Structure):
    _fields_ = [('bmiHeader', BITMAPINFOHEADER), ('bmiColors', wt.DWORD * 3)]


hdc = gdi32.CreateCompatibleDC(0)
bmi = BITMAPINFO()
bmi.bmiHeader.biSize = ctypes.sizeof(BITMAPINFOHEADER)
bmi.bmiHeader.biWidth = CANVAS
bmi.bmiHeader.biHeight = -CANVAS          # top-down
bmi.bmiHeader.biPlanes = 1
bmi.bmiHeader.biBitCount = 32
bmi.bmiHeader.biCompression = 0
bits = ctypes.c_void_p()
hbm = gdi32.CreateDIBSection(hdc, ctypes.byref(bmi), 0, ctypes.byref(bits), None, 0)
if not hbm:
    raise RuntimeError("CreateDIBSection failed: %s" % ctypes.get_last_error())
gdi32.SelectObject(hdc, hbm)
gdi32.SetBkMode(hdc, 1)                   # TRANSPARENT
gdi32.SetTextColor(hdc, 0x00FFFFFF)


def use_font(name):
    hf = gdi32.CreateFontW(-FONT_SIZE, 0, 0, 0, 700, 0, 0, 0, 134,
                           0, 0, 4, 0x40, name)   # 134=GB2312, 4=ANTIALIASED, 0x40=FF_SWISS
    if not hf:
        return None
    gdi32.SelectObject(hdc, hf)
    return hf


def render(ch):
    brush = gdi32.CreateSolidBrush(0x00000000)
    r = wt.RECT(0, 0, CANVAS, CANVAS)
    user32.FillRect(hdc, ctypes.byref(r), brush)
    gdi32.DeleteObject(brush)
    gdi32.TextOutW(hdc, 2, 2, ch, 1)
    buf = ctypes.cast(bits, ctypes.POINTER(ctypes.c_ubyte))
    px = [[0] * CANVAS for _ in range(CANVAS)]
    for y in range(CANVAS):
        for x in range(CANVAS):
            o = (y * CANVAS + x) * 4
            lum = (buf[o + 2] * 3 + buf[o + 1] * 6 + buf[o]) // 10
            px[y][x] = 1 if lum > 90 else 0
    return px


def normalize(px):
    """裁剪到字形外框，等比缩放并居中到 32x32"""
    ys = [y for y in range(CANVAS) if any(px[y])]
    xs = [x for x in range(CANVAS) if any(px[y][x] for y in range(CANVAS))]
    if not ys or not xs:
        return None
    x0, x1, y0, y1 = xs[0], xs[-1], ys[0], ys[-1]
    w, h = x1 - x0 + 1, y1 - y0 + 1
    scale = min(SIZE / w, SIZE / h)
    nw = max(1, int(round(w * scale)))
    nh = max(1, int(round(h * scale)))
    ox, oy = (SIZE - nw) // 2, (SIZE - nh) // 2
    out = [[0] * SIZE for _ in range(SIZE)]
    for j in range(nh):
        sy = y0 + int(j * h / nh)
        for i in range(nw):
            sx = x0 + int(i * w / nw)
            if px[sy][sx]:
                out[oy + j][ox + i] = 1
    return out


def to_bytes(out):
    data = []
    for row in range(SIZE):
        for b in range(SIZE // 8):
            v = 0
            for bit in range(8):
                if out[row][b * 8 + bit]:
                    v |= (1 << bit)
            data.append(v)
    return data


font_used = None
for name in FONT_CANDIDATES:
    if use_font(name):
        test = normalize(render(CHARS[0]))
        if test and any(any(r) for r in test):
            font_used = name
            break
if not font_used:
    raise RuntimeError("没有可用的中文字体")
print("使用字体:", font_used)

grids = []
for ch in CHARS:
    g = normalize(render(ch))
    if g is None:
        raise RuntimeError("字符渲染失败: %s" % ch)
    grids.append(g)
    print("\n=== %s ===" % ch)
    for row in g:
        print("".join("##" if v else ".." for v in row))

here = os.path.dirname(os.path.abspath(__file__))
out_path = os.path.join(here, os.pardir, "User", "love_font.h")
lines = []
lines.append("/*")
lines.append(" * 字模: \"%s\"" % CHARS)
lines.append(" * 生成: gen_font.py (Windows GDI, 字体 %s)" % font_used)
lines.append(" * 格式: 32x32, 行优先, 每行4字节, 字节内 bit0 = 最左像素")
lines.append(" */")
lines.append("#ifndef __LOVE_FONT_H")
lines.append("#define __LOVE_FONT_H")
lines.append("")
lines.append("#include <stdint.h>")
lines.append("")
lines.append("#define LOVE_FONT_SIZE 32")
lines.append("#define LOVE_FONT_COUNT %d" % len(grids))
lines.append("")
lines.append("static const uint8_t love_font[LOVE_FONT_COUNT][128] = {")
for g in grids:
    data = to_bytes(g)
    body = ", ".join("0x%02X" % v for v in data)
    wrapped = []
    for i in range(0, len(data), 16):
        wrapped.append("    " + ", ".join("0x%02X" % v for v in data[i:i + 16]) + ",")
    wrapped[-1] = wrapped[-1].rstrip(",")
    lines.append("{")
    lines.extend(wrapped)
    lines.append("},")
lines.append("};")
lines.append("")
lines.append("#endif")
lines.append("")

with open(out_path, "w", encoding="utf-8") as f:
    f.write("\n".join(lines))
print("\n已生成:", out_path)
