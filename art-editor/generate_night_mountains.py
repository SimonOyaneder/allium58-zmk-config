#!/usr/bin/env python3
"""5 variantes originales del paisaje nocturno para la pantalla derecha.

Instalada en el shield: V3 (right_v3 -> assets/night_mountains.c).

Principios tomados del estudio de la técnica de hammerbeam (sin copiar sus
obras): varias densidades de dithering como tonos, trazos que siguen la forma,
siluetas con detalle interno, nubes en franjas y bandas de descanso.
Lienzo 68x128 vertical; 1 = tinta negra, 0 = blanco.
"""
import math
import sys
sys.path.insert(0, ".")
from imgtool import rot_cw, write_png, export_c

W, H = 68, 128
BAYER = [[0, 8, 2, 10], [12, 4, 14, 6], [3, 11, 1, 9], [15, 7, 13, 5]]


def bayer(x, y, level16):
    return BAYER[y % 4][x % 4] < level16


def new_canvas(fill=1):
    return [[fill] * W for _ in range(H)]


def put(art, x, y, v):
    if 0 <= x < W and 0 <= y < H:
        art[y][x] = v


def star(art, cx, cy, size=1):
    for d in range(-size, size + 1):
        put(art, cx, cy + d, 0)
        put(art, cx + d, cy, 0)


def sprinkle(art, y0, y1, dens, seed):
    rnd = seed
    for y in range(y0, y1):
        for x in range(W):
            rnd = (rnd * 1103515245 + 12345) & 0x7FFFFFFF
            if rnd / 0x7FFFFFFF < dens and art[y][x] == 1:
                put(art, x, y, 0)


def moon(art, cx, cy, r, craters=True):
    """Disco blanco con sombra dithered en el borde inferior derecho."""
    for y in range(cy - r - 1, cy + r + 2):
        for x in range(cx - r - 1, cx + r + 2):
            d = math.hypot(x - cx, y - cy)
            if d <= r:
                put(art, x, y, 0)
                if d >= r - 2.6:
                    ang = math.atan2(y - cy, x - cx)
                    if -0.5 < ang < 2.2 and bayer(x, y, 7):
                        put(art, x, y, 1)
    if craters:
        for dx, dy, cr in [(-5, -2, 2), (2, -5, 1), (4, 3, 2), (-2, 4, 1)]:
            for y in range(cy + dy - cr, cy + dy + cr + 1):
                for x in range(cx + dx - cr, cx + dx + cr + 1):
                    d = math.hypot(x - cx - dx, y - cy - dy)
                    if cr - 0.8 <= d <= cr + 0.4:
                        put(art, x, y, 1)


def ridge_y(peaks, x):
    for (x0, y0), (x1, y1) in zip(peaks, peaks[1:]):
        if x0 <= x <= x1:
            t = (x - x0) / (x1 - x0)
            return y0 + (y1 - y0) * t
    return peaks[-1][1]


def pine(art, cx, base, h_, half, v=1):
    """Abeto con borde irregular y espigas de ramas."""
    for dy in range(h_ + 1):
        y = base - dy
        frac = 1 - dy / (h_ + 1)
        spanw = half * frac
        # borde irregular + espigas cada 3 filas
        spanw += (1 if dy % 3 == 1 else 0) - (0.5 if dy % 3 == 2 else 0)
        s = max(0, int(round(spanw)))
        for x in range(cx - s, cx + s + 1):
            put(art, x, y, v)
    put(art, cx, base - h_ - 1, v)


def flow_stroke(art, x0, y0, dxdy, length, v=1, width=2):
    """Trazo que baja siguiendo la pendiente (dxdy = avance x por fila)."""
    fx = float(x0)
    for i in range(length):
        y = y0 + i
        for wdx in range(width):
            put(art, int(fx) + wdx, y, v)
        fx += dxdy


def cloud(art, cx, cy, wspan, hspan, seed=1):
    """Franja de nube: núcleo blanco, contorno y sombra dithered abajo."""
    for y in range(cy - hspan, cy + hspan + 1):
        t = abs(y - cy) / (hspan + 0.001)
        rowspan = int(wspan * math.sqrt(max(0, 1 - t * t)))
        # extremos desplazados para silueta orgánica
        shift = int(3 * math.sin((y + seed) * 1.7))
        for x in range(cx - rowspan + shift, cx + rowspan + shift):
            if y >= cy and bayer(x, y, 5):
                put(art, x, y, 1)      # sombra inferior
            else:
                put(art, x, y, 0)
    # contorno inferior
    for x in range(cx - wspan + 1, cx + wspan - 1):
        put(art, x, cy + hspan, 1)


def export(art, name):
    write_png(f"{name}.png", art, W, H, 4)
    fb = rot_cw(art)
    open(f"{name}.c", "w").write(export_c(fb, "night_mountains", fixed_palette=True))
    print(f"{name}: ok")


# ======================================================================
# V1 - Cara nevada: pico dominante, cara en sombra tallada con trazos
# ======================================================================
def v1():
    art = new_canvas(1)
    sprinkle(art, 0, 46, 0.02, 111)
    star(art, 10, 8, 2); star(art, 26, 15, 1)
    star(art, 58, 38, 1); star(art, 7, 30, 1); star(art, 33, 5, 1); star(art, 61, 7, 1)
    moon(art, 50, 17, 10)

    # pico dominante: cresta desde (2,90) sube a (26,46) baja a (66,86)
    peaks = [(0, 92), (26, 46), (50, 76), (67, 82)]
    for x in range(W):
        top = ridge_y(peaks, x)
        for y in range(int(top), 100):
            depth = y - top
            if x <= 26:
                # cara iluminada: dither suave que se abre al bajar
                put(art, x, y, 1 if bayer(x, y, min(6, int(depth * 0.35))) else 0)
            else:
                # cara en sombra: base clara con textura media
                put(art, x, y, 1 if bayer(x, y, min(9, 2 + int(depth * 0.5))) else 0)
    # trazos de flujo en la cara en sombra (bajan hacia la derecha)
    for sx, ln, k in [(27, 40, 0.55), (30, 34, 0.7), (34, 28, 0.9), (39, 22, 1.0), (45, 16, 1.1), (52, 12, 1.2)]:
        y0 = int(ridge_y(peaks, sx)) + 2
        flow_stroke(art, sx, y0, k, ln, v=1, width=2)
        flow_stroke(art, sx - 2, y0 + 2, k, max(4, ln - 8), v=0, width=1)  # luz al lado
    # línea de cresta nítida
    for x in range(W):
        put(art, x, int(ridge_y(peaks, x)), 1)
        put(art, x, int(ridge_y(peaks, x)) - 1, 0)

    # banda de niebla blanca amplia
    for y in range(100, 116):
        for x in range(W):
            put(art, x, y, 1 if bayer(x, y, 2) else 0)

    # suelo oscuro y bosque en silueta negra recortado sobre la niebla
    for y in range(116, H):
        for x in range(W):
            put(art, x, y, 1)
    for cx, h_, half in [(3, 9, 3), (10, 13, 4), (17, 8, 3), (25, 15, 4), (33, 10, 3),
                         (41, 14, 4), (49, 9, 3), (57, 13, 4), (65, 10, 3)]:
        pine(art, cx, 119, h_, half, v=1)
    # brillos del suelo
    for x, y in [(6, 124), (18, 126), (30, 123), (44, 125), (56, 124), (63, 127), (12, 122), (37, 127)]:
        put(art, x, y, 0)
    export(art, "right_v1")


# ======================================================================
# V2 - Ladera diagonal: bandas de hachurado siguiendo la pendiente,
#      un abeto alto contra el cielo
# ======================================================================
def v2():
    art = new_canvas(1)
    sprinkle(art, 0, 40, 0.02, 222)
    star(art, 14, 10, 2); star(art, 40, 6, 1); star(art, 8, 28, 1); star(art, 33, 20, 1)
    moon(art, 54, 13, 9)

    # ladera nevada: diagonal desde (0,52) hasta (67,96)
    def slope_top(x):
        return 52 + x * 44 / 67

    for x in range(W):
        top = slope_top(x)
        for y in range(int(top), H):
            depth = y - top
            band = int(depth / 7)  # bandas paralelas a la pendiente
            if band == 0:
                put(art, x, y, 0)                                   # nieve
            elif band % 2 == 1:
                put(art, x, y, 1 if bayer(x, y, 6) else 0)          # gris claro
            else:
                put(art, x, y, 1 if bayer(x, y, 11) else 0)         # gris oscuro
    # rocas dispersas en la ladera
    for rx, ry in [(12, 74), (24, 84), (38, 90), (52, 104), (18, 96), (44, 112), (30, 106), (58, 118)]:
        for dy in range(3):
            for dx in range(4 - dy):
                put(art, rx + dx, ry + dy, 1)
            put(art, rx - 1, ry + 2, 0)

    # abeto protagonista contra el cielo: halo blanco + silueta negra
    pine(art, 16, 76, 36, 10, v=0)
    pine(art, 16, 74, 34, 8, v=1)
    # abetos pequeños al pie, también recortados
    for cx, h_, half in [(38, 12, 4), (50, 9, 3), (61, 14, 4), (28, 8, 3)]:
        base = 124 - (67 - cx) // 8
        pine(art, cx, base + 1, h_ + 1, half + 1, v=0)
        pine(art, cx, base, h_, half, v=1)
    export(art, "right_v2")


# ======================================================================
# V3 - Nubes nocturnas: luna grande cruzada por franjas de nubes,
#      cordillera baja al fondo
# ======================================================================
def v3():
    art = new_canvas(1)
    sprinkle(art, 0, 84, 0.025, 333)
    star(art, 8, 6, 1); star(art, 60, 10, 2); star(art, 12, 44, 1)
    star(art, 56, 52, 1); star(art, 30, 60, 1); star(art, 63, 30, 1)

    moon(art, 34, 26, 16)
    # nubes cruzando la luna y el cielo
    cloud(art, 12, 16, 14, 3, seed=2)
    cloud(art, 52, 40, 16, 4, seed=5)
    cloud(art, 58, 8, 12, 3, seed=8)
    cloud(art, 10, 56, 14, 3, seed=11)
    cloud(art, 48, 66, 16, 3, seed=14)

    # cordillera lejana en dos capas de dither
    far = [(0, 88), (14, 80), (30, 86), (44, 78), (58, 84), (67, 80)]
    for x in range(W):
        top = ridge_y(far, x)
        for y in range(int(top), 100):
            put(art, x, y, 1 if (x + y) % 2 == 0 else 0)
    near = [(0, 100), (10, 92), (24, 98), (40, 90), (54, 97), (67, 93)]
    for x in range(W):
        top = ridge_y(near, x)
        for y in range(int(top), 108):
            depth = y - top
            put(art, x, y, 1 if bayer(x, y, min(12, 5 + int(depth))) else 0)
        put(art, x, int(ridge_y(near, x)), 1)
        put(art, x, int(ridge_y(near, x)) - 1, 0)

    # lago oscuro con destellos horizontales de la luna
    for y in range(108, H):
        for x in range(W):
            put(art, x, y, 1)
    for y, x0, x1 in [(112, 26, 44), (115, 22, 40), (118, 30, 48), (121, 24, 38), (124, 32, 46), (126, 27, 41)]:
        for x in range(x0, x1):
            if (x + y) % 3 != 0:
                put(art, x, y, 0)
    export(art, "right_v3")


# ======================================================================
# V4 - Capas de niebla: cordilleras que retroceden en 5 tonos
# ======================================================================
def v4():
    art = new_canvas(1)
    sprinkle(art, 0, 48, 0.02, 444)
    star(art, 12, 7, 2); star(art, 44, 12, 1); star(art, 60, 5, 1)
    star(art, 7, 24, 1); star(art, 36, 28, 1); star(art, 58, 34, 1)
    moon(art, 22, 18, 10)

    layers = [
        ([(0, 58), (16, 50), (34, 56), (52, 48), (67, 54)], 66, 3),
        ([(0, 70), (12, 62), (28, 68), (46, 60), (67, 66)], 80, 6),
        ([(0, 84), (18, 74), (36, 82), (56, 72), (67, 78)], 94, 9),
        ([(0, 98), (14, 88), (32, 96), (50, 86), (67, 92)], 110, 13),
        ([(0, 112), (20, 102), (42, 110), (60, 100), (67, 106)], 128, 16),
    ]
    for peaks, ybot, level in layers:
        for x in range(W):
            top = ridge_y(peaks, x)
            for y in range(int(top), min(ybot, H)):
                put(art, x, y, 1 if bayer(x, y, level) else 0)
            # cresta definida con línea clara arriba
            put(art, x, int(top) - 1, 0)

    # abetos diminutos en la capa más cercana (silueta negra ya, puntas claras)
    for cx, h_ in [(6, 7), (16, 9), (26, 6), (36, 10), (46, 7), (56, 9), (64, 6)]:
        base = int(ridge_y(layers[4][0], cx)) + 4
        pine(art, cx, base + 1, h_ + 1, 4, v=0)
        pine(art, cx, base, h_, 3, v=1)
    export(art, "right_v4")


# ======================================================================
# V5 - Lago espejo: montaña y luna reflejadas en agua con cortes de olas
# ======================================================================
def v5():
    art = new_canvas(1)
    HORIZON = 78
    sprinkle(art, 0, 40, 0.02, 555)
    star(art, 10, 8, 2); star(art, 30, 14, 1); star(art, 60, 20, 1); star(art, 52, 4, 1)
    moon(art, 51, 16, 9)

    peaks = [(0, 66), (14, 54), (30, 38), (44, 58), (56, 50), (67, 62)]
    for x in range(W):
        top = ridge_y(peaks, x)
        for y in range(int(top), HORIZON):
            depth = y - top
            if depth < 3:
                put(art, x, y, 0)                                    # nieve
            elif x < 30:
                put(art, x, y, 1 if bayer(x, y, min(6, int(depth * 0.5))) else 0)
            else:
                put(art, x, y, 1 if bayer(x, y, min(10, 3 + int(depth * 0.6))) else 0)
    for sx, ln in [(32, 22), (38, 16), (46, 12)]:
        flow_stroke(art, sx, int(ridge_y(peaks, sx)) + 2, 0.8, ln, v=1, width=2)

    # orilla
    for x in range(W):
        put(art, x, HORIZON, 1)
        put(art, x, HORIZON + 1, 1)

    # reflejo: espejo vertical comprimido, roto por olas horizontales
    for y in range(HORIZON + 2, H):
        dy = y - (HORIZON + 2)
        src_y = HORIZON - 1 - int(dy * 1.35)
        for x in range(W):
            v = art[src_y][x] if 0 <= src_y < HORIZON else 1
            if y % 4 == 0 or (y % 4 == 1 and x % 2 == 0):
                v = 1                                                # corte de ola
            put(art, x, y, v)
    # destello de luna en el agua
    for y, x0, x1 in [(84, 46, 58), (88, 43, 55), (92, 48, 60), (97, 45, 56), (102, 49, 58), (108, 46, 54)]:
        for x in range(x0, x1):
            if (x + y) % 2 == 0:
                put(art, x, y, 0)
    export(art, "right_v5")


v1(); v2(); v3(); v4(); v5()
