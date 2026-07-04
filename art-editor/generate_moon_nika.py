#!/usr/bin/env python3
"""Pantalla izquierda: luna con Nika sobre cielo negro con estrellas.

Fuente de la luna: nice-luffy-gear-five (MIT). Se invierte para que quede
luna clara / silueta oscura sobre cielo negro. El bitmap original viene con
la cúpula recortada (~4 filas) y una marca suelta más abajo: la cúpula se
cierra espejando el arco inferior por columna y la marca se elimina.

El lienzo es 68x120: la pantalla izquierda deja 16px arriba (fila única
con icono de conexión + % de batería) y 24px abajo (perfiles BT + capa).
"""
import sys
sys.path.insert(0, ".")
from imgtool import parse_c_bytes, unpack_bitmap, rot_ccw, rot_cw, write_png, export_c

SRC = sys.argv[1] if len(sys.argv) > 1 else "nice-luffy-gear-five/boards/shields/nice_luffy_gear_five/assets/right_image.c"

data, _, _ = parse_c_bytes(SRC)
fb = unpack_bitmap(data, 80, 69)          # como está almacenada (framebuffer)
moon = rot_ccw(fb)                        # vista física: 69 ancho x 80 alto

# recortar la columna vacía del borde -> 68 de ancho
col_ink = [sum(moon[y][x] for y in range(len(moon))) for x in range(len(moon[0]))]
moon = [row[1:] for row in moon] if col_ink[0] <= col_ink[-1] else [row[:-1] for row in moon]
w = 68

# quedarnos solo con el disco: cortar en la primera fila vacía
# (más abajo hay una marca suelta del original que se ve como "pedazo cortado")
end = next(y for y in range(len(moon)) if not any(moon[y]))
moon = moon[:end]
h = len(moon)
print(f"disco original: 68x{h} (marca inferior eliminada)")

# ---------- cerrar la cúpula espejando el arco inferior por columna ----------
# ecuador = centro de las filas más anchas
widths = [sum(1 for v in row if v) and (max(x for x, v in enumerate(row) if v)
          - min(x for x, v in enumerate(row) if v) + 1) for row in moon]
maxw = max(widths)
wide_rows = [y for y, wd in enumerate(widths) if wd >= maxw]
cy = sum(wide_rows) / len(wide_rows)

def col_pixels(x):
    return [y for y in range(h) if moon[y][x]]

# cuántas filas agregar arriba: la columna más profunda define la cima esperada
tops_exp = {}
for x in range(w):
    ys = col_pixels(x)
    if ys:
        tops_exp[x] = int(round(2 * cy)) - ys[-1]   # espejo del punto más bajo
pad = max(0, -min(tops_exp.values()))
moon = [[0] * w for _ in range(pad)] + moon
h = len(moon)
print(f"ecuador y={cy:.1f}, +{pad} filas de cúpula")

# rellenar por columna desde la cima esperada hasta el primer pixel actual
for x, t_exp in tops_exp.items():
    t_exp += pad
    ys = [y for y in range(h) if moon[y][x]]
    t_act = ys[0]
    for y in range(max(0, t_exp), t_act):
        moon[y][x] = 1                     # sólido: tras invertir queda blanco

# ---------- invertir: luna clara / silueta oscura, entorno negro ----------
moon = [[1 - v for v in row] for row in moon]

# lienzo 68 x 120, cielo negro (1 = tinta negra)
W, H = 68, 120
art = [[1] * W for _ in range(H)]

oy = (H - h) // 2
for y in range(h):
    for x in range(w):
        art[oy + y][x] = moon[y][x]

def star(cx, cy_, size=1):
    """Estrella blanca: cruz de brazos `size`."""
    for d in range(-size, size + 1):
        if 0 <= cy_ + d < H:
            art[cy_ + d][cx] = 0
        if 0 <= cx + d < W:
            art[cy_][cx + d] = 0

def dot(cx, cy_):
    art[cy_][cx] = 0

# estrellas en el cielo alrededor de la luna
star(10, 7, 2); star(44, 5, 1); star(61, 12, 1); star(27, 16, 1)
dot(26, 3); dot(54, 2); dot(5, 15); dot(65, 19); dot(35, 9); dot(16, 21)
star(9, 104, 1); star(41, 108, 1); star(59, 102, 2); star(25, 112, 1)
dot(20, 110); dot(50, 105); dot(32, 101); dot(65, 109); dot(12, 98); dot(45, 116)

write_png("left_design.png", art, W, H, 4)
fb_out = rot_cw(art)
open("moon_nika.c", "w").write(export_c(fb_out, "moon_nika", fixed_palette=True))
print("export: moon_nika.c", len(fb_out[0]), "x", len(fb_out))
