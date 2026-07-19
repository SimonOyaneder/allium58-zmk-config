#!/usr/bin/env python3
"""Pantalla OLED derecha (32x128 vertical): "Cordillera y luna".

Hermana del arte nice!view del Allium58: luna llena con cráteres, doble
cordillera en dither Bayer y lago abajo. Exporta night_art.h con:
  - night_base[512]  frame base en formato de buffer OLED (páginas de 8px,
                     LSB arriba, ancho lógico 32 con rotación 270)
  - night_stars[][2] estrellas que titilan (se redibujan por tick)
  - night_bands[][3] franjas de reflejo del lago (y, x0, x1), se dibujan
                     desplazándose en el firmware

Regenerar: python3 generate_night_art.py  (escribe night_art.h al lado
y un preview PNG x6)
"""
import math
import os
from PIL import Image

W, H = 32, 128
BAYER = [[0, 8, 2, 10], [12, 4, 14, 6], [3, 11, 1, 9], [15, 7, 13, 5]]
art = [[0] * W for _ in range(H)]  # 1 = pixel encendido


def put(x, y, v=1):
    if 0 <= x < W and 0 <= y < H:
        art[y][x] = v


def bayer(x, y, l):
    return BAYER[y % 4][x % 4] < l


def star_cross(cx, cy, size=1):
    for d in range(-size, size + 1):
        put(cx, cy + d)
        put(cx + d, cy)


# ---------- estrellas ----------
# Las de cruz son fijas (parte del frame); las puntuales titilan.
TWINKLE = [
    (4, 5), (14, 3), (27, 7), (9, 12), (29, 15), (3, 20), (12, 26),
    (24, 30), (6, 34), (17, 38), (28, 42), (4, 47), (21, 50), (10, 55),
    (26, 58), (7, 63), (15, 68), (23, 72),
]
for x, y in TWINKLE:
    put(x, y)
star_cross(5, 8, 1)
star_cross(27, 40, 1)
star_cross(8, 52, 1)
star_cross(25, 66, 2)
star_cross(4, 30, 1)

# ---------- luna llena con sombra y cráteres ----------
MX, MY, R = 21, 20, 8
for y in range(MY - R - 1, MY + R + 2):
    for x in range(MX - R - 1, MX + R + 2):
        d = math.hypot(x - MX, y - MY)
        if d <= R:
            v = 1
            if d >= R - 2.2:
                ang = math.atan2(y - MY, x - MX)
                if -0.5 < ang < 2.2 and bayer(x, y, 7):
                    v = 0
            put(x, y, v)
for cx, cy, cr in [(18, 18, 2), (24, 16, 1), (23, 23, 1)]:
    for y in range(cy - cr, cy + cr + 1):
        for x in range(cx - cr, cx + cr + 1):
            d = math.hypot(x - cx, y - cy)
            if cr - 0.8 <= d <= cr + 0.4:
                put(x, y, 0)


# ---------- doble cordillera ----------
def ridge_y(peaks, x):
    for (x0, y0), (x1, y1) in zip(peaks, peaks[1:]):
        if x0 <= x <= x1:
            t = (x - x0) / (x1 - x0)
            return y0 + (y1 - y0) * t
    return peaks[-1][1]


far = [(0, 86), (9, 80), (18, 85), (26, 78), (31, 83)]
for x in range(W):
    for y in range(int(ridge_y(far, x)), 100):
        put(x, y, 1 if (x + y) % 2 == 0 else 0)

near = [(0, 102), (7, 94), (17, 100), (25, 92), (31, 98)]
for x in range(W):
    t = int(ridge_y(near, x))
    for y in range(t, 112):
        put(x, y, 1 if bayer(x, y, min(12, 4 + (y - t))) else 0)
    put(x, t, 1)
    put(x, t - 1, 0)

# ---------- orilla; el agua la dibuja el firmware (reflejos móviles) ----------
for x in range(W):
    put(x, 112)
BANDS = [(115, 10, 22), (118, 7, 18), (121, 13, 25), (124, 9, 20), (126, 14, 24)]

# ---------- export ----------
buf = bytearray(512)
for y in range(H):
    for x in range(W):
        if art[y][x]:
            buf[x + (y // 8) * W] |= 1 << (y % 8)

here = os.path.dirname(os.path.abspath(__file__))


def fmt_rows(items, per):
    out = []
    for i in range(0, len(items), per):
        out.append("    " + ", ".join(items[i:i + per]) + ",")
    return "\n".join(out)


with open(os.path.join(here, "night_art.h"), "w") as f:
    f.write("#pragma once\n\n")
    f.write("/*\n * Generado por generate_night_art.py — no editar a mano.\n")
    f.write(" * \"Cordillera y luna\" 32x128 + listas de animación.\n */\n\n")
    f.write("#define NIGHT_TICK_MS 800\n")
    f.write("#define NIGHT_STAR_COUNT %d\n" % len(TWINKLE))
    f.write("#define NIGHT_BAND_COUNT %d\n\n" % len(BANDS))
    f.write("static const char PROGMEM night_base[512] = {\n")
    f.write(fmt_rows(["0x%02x" % b for b in buf], 16))
    f.write("\n};\n\n")
    f.write("static const uint8_t night_stars[NIGHT_STAR_COUNT][2] = {\n")
    f.write(fmt_rows(["{%d, %d}" % (x, y) for x, y in TWINKLE], 6))
    f.write("\n};\n\n")
    f.write("static const uint8_t night_bands[NIGHT_BAND_COUNT][3] = {\n")
    f.write(fmt_rows(["{%d, %d, %d}" % b for b in BANDS], 5))
    f.write("\n};\n")

# preview con los reflejos en fase 0
prev = [row[:] for row in art]
for y, x0, x1 in BANDS:
    for x in range(x0, x1):
        if ((x + y) & 3) < 2:
            prev[y][x] = 1
img = Image.new('L', (W, H), 0)
for y in range(H):
    for x in range(W):
        if prev[y][x]:
            img.putpixel((x, y), 255)
img.resize((W * 6, H * 6), Image.NEAREST).save(os.path.join(here, "night_art_preview.png"))
print("ok: night_art.h + preview")
