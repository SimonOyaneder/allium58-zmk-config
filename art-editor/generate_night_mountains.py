#!/usr/bin/env python3
"""Pantalla derecha: paisaje nocturno "Nubes nocturnas" (V3), lienzo 68x144.

Luna grande entre franjas de nubes, doble cordillera en dither y lago con
destellos. Técnica inspirada en el estilo 1-bit de hammerbeam (obra original).
La pantalla deja 16px arriba para la fila única de estado (icono + %).
"""
import math
import sys
sys.path.insert(0, ".")
from imgtool import rot_cw, write_png, export_c

W, H = 68, 144
art = [[1] * W for _ in range(H)]  # 1 = tinta negra: cielo nocturno

BAYER = [[0, 8, 2, 10], [12, 4, 14, 6], [3, 11, 1, 9], [15, 7, 13, 5]]


def bayer(x, y, level16):
    return BAYER[y % 4][x % 4] < level16


def put(x, y, v):
    if 0 <= x < W and 0 <= y < H:
        art[y][x] = v


def star(cx, cy, size=1):
    for d in range(-size, size + 1):
        put(cx, cy + d, 0)
        put(cx + d, cy, 0)


rnd = 333


def rand():
    global rnd
    rnd = (rnd * 1103515245 + 12345) & 0x7FFFFFFF
    return rnd / 0x7FFFFFFF


# ---------- cielo estrellado ----------
for y in range(0, 100):
    for x in range(W):
        if rand() < 0.025 and art[y][x] == 1:
            put(x, y, 0)

star(8, 6, 1); star(60, 10, 2); star(12, 44, 1); star(56, 52, 1)
star(30, 60, 1); star(63, 30, 1); star(20, 76, 1); star(48, 84, 1); star(7, 90, 1)

# ---------- luna llena con sombra y cráteres ----------
MX, MY, R = 34, 26, 16
for y in range(MY - R - 1, MY + R + 2):
    for x in range(MX - R - 1, MX + R + 2):
        d = math.hypot(x - MX, y - MY)
        if d <= R:
            put(x, y, 0)
            if d >= R - 2.6:
                ang = math.atan2(y - MY, x - MX)
                if -0.5 < ang < 2.2 and bayer(x, y, 7):
                    put(x, y, 1)

for cx, cy, r in [(29, 24, 2), (36, 21, 1), (38, 29, 2), (32, 31, 1)]:
    for y in range(cy - r, cy + r + 1):
        for x in range(cx - r, cx + r + 1):
            d = math.hypot(x - cx, y - cy)
            if r - 0.8 <= d <= r + 0.4:
                put(x, y, 1)


# ---------- nubes en franjas ----------
def cloud(cx, cy, wspan, hspan, seed=1):
    for y in range(cy - hspan, cy + hspan + 1):
        t = abs(y - cy) / (hspan + 0.001)
        rowspan = int(wspan * math.sqrt(max(0, 1 - t * t)))
        shift = int(3 * math.sin((y + seed) * 1.7))
        for x in range(cx - rowspan + shift, cx + rowspan + shift):
            if y >= cy and bayer(x, y, 5):
                put(x, y, 1)
            else:
                put(x, y, 0)
    for x in range(cx - wspan + 1, cx + wspan - 1):
        put(x, cy + hspan, 1)


cloud(12, 16, 14, 3, seed=2)
cloud(52, 40, 16, 4, seed=5)
cloud(58, 8, 12, 3, seed=8)
cloud(10, 60, 14, 3, seed=11)
cloud(46, 70, 16, 3, seed=14)
cloud(14, 84, 12, 3, seed=17)


# ---------- doble cordillera ----------
def ridge_y(peaks, x):
    for (x0, y0), (x1, y1) in zip(peaks, peaks[1:]):
        if x0 <= x <= x1:
            t = (x - x0) / (x1 - x0)
            return y0 + (y1 - y0) * t
    return peaks[-1][1]


far = [(0, 104), (14, 96), (30, 102), (44, 94), (58, 100), (67, 96)]
for x in range(W):
    top = ridge_y(far, x)
    for y in range(int(top), 116):
        put(x, y, 1 if (x + y) % 2 == 0 else 0)

near = [(0, 116), (10, 108), (24, 114), (40, 106), (54, 113), (67, 109)]
for x in range(W):
    top = ridge_y(near, x)
    for y in range(int(top), 124):
        depth = y - top
        put(x, y, 1 if bayer(x, y, min(12, 5 + int(depth))) else 0)
    put(x, int(ridge_y(near, x)), 1)
    put(x, int(ridge_y(near, x)) - 1, 0)

# ---------- lago oscuro con destellos de la luna ----------
for y in range(124, H):
    for x in range(W):
        put(x, y, 1)
for y, x0, x1 in [(128, 26, 44), (131, 22, 40), (134, 30, 48), (137, 24, 38), (140, 32, 46), (142, 27, 41)]:
    for x in range(x0, x1):
        if (x + y) % 3 != 0:
            put(x, y, 0)

write_png("right_design.png", art, W, H, 4)
fb_out = rot_cw(art)
open("night_mountains.c", "w").write(export_c(fb_out, "night_mountains"))
print("export: night_mountains.c", len(fb_out[0]), "x", len(fb_out))
