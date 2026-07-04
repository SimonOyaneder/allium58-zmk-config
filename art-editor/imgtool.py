#!/usr/bin/env python3
"""Herramientas para arte 1-bit de nice!view: decodificar arrays C de LVGL a PNG y viceversa."""
import re
import struct
import sys
import zlib


def write_png(path, pixels, w, h, scale=1):
    """pixels: lista de filas, cada fila lista de 0/1 (1 = negro sobre blanco)."""
    sw, sh = w * scale, h * scale
    raw = b""
    for y in range(sh):
        row = bytearray([0])  # filtro 0
        srcy = y // scale
        for x in range(sw):
            v = pixels[srcy][x // scale]
            row.append(0 if v else 255)  # 1 = tinta negra
        raw += bytes(row)

    def chunk(tag, data):
        c = struct.pack(">I", len(data)) + tag + data
        return c + struct.pack(">I", zlib.crc32(tag + data) & 0xFFFFFFFF)

    ihdr = struct.pack(">IIBBBBB", sw, sh, 8, 0, 0, 0, 0)  # gray 8-bit
    png = (b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", ihdr)
           + chunk(b"IDAT", zlib.compress(raw)) + chunk(b"IEND", b""))
    with open(path, "wb") as f:
        f.write(png)


def parse_c_bytes(path):
    """Extrae los bytes de pixeles de un .c de LVGL (descarta las 2 lineas de paleta)."""
    src = open(path).read()
    m = re.search(r"_map\[\]\s*=\s*\{(.*?)\};", src, re.S)
    body = m.group(1)
    lines = [l for l in body.splitlines() if "Color of index" not in l and not l.strip().startswith("#")]
    data = [int(v, 16) for v in re.findall(r"0x[0-9a-fA-F]{1,2}", "\n".join(lines))]
    wm = re.search(r"\.header\.w\s*=\s*(\d+)", src)
    hm = re.search(r"\.header\.h\s*=\s*(\d+)", src)
    return data, (int(wm.group(1)) if wm else None), (int(hm.group(1)) if hm else None)


def unpack_bitmap(data, w, h):
    stride = (w + 7) // 8
    px = []
    for y in range(h):
        row = []
        for x in range(w):
            b = data[y * stride + (x >> 3)] if y * stride + (x >> 3) < len(data) else 0
            row.append((b >> (7 - (x & 7))) & 1)
        px.append(row)
    return px


def rot_cw(px):
    """Rota 90 grados en sentido horario."""
    h, w = len(px), len(px[0])
    return [[px[h - 1 - x][y] for x in range(h)] for y in range(w)]


def rot_ccw(px):
    """Rota 90 grados en sentido antihorario."""
    h, w = len(px), len(px[0])
    return [[px[x][w - 1 - y] for x in range(h)] for y in range(w)]


def pack_bytes(px):
    h, w = len(px), len(px[0])
    stride = (w + 7) // 8
    out = []
    for y in range(h):
        for b in range(stride):
            byte = 0
            for bit in range(8):
                x = b * 8 + bit
                if x < w and px[y][x]:
                    byte |= 1 << (7 - bit)
            out.append(byte)
    return out


def export_c(px, sym, fixed_palette=True):
    """Genera un .c estilo LVGL9 (como los assets del shield).

    OJO (validado en hardware, zmk main + LVGL 9): el render de imágenes I1
    ignora la paleta incrustada y usa los bits crudos con la convención de
    zmk stock: bit 0 = NEGRO, bit 1 = BLANCO. Por eso aquí se empaqueta
    invertido respecto al diseño (1 = tinta negra -> bit 0) y la paleta se
    escribe fija en esa misma convención (solo documental).
    """
    h, w = len(px), len(px[0])
    data = pack_bytes([[1 - v for v in row] for row in px])
    stride = (w + 7) // 8
    lines = []
    up = sym.upper()
    lines.append("#include <lvgl.h>\n")
    lines.append("#ifndef LV_ATTRIBUTE_MEM_ALIGN\n#define LV_ATTRIBUTE_MEM_ALIGN\n#endif\n")
    lines.append(f"#ifndef LV_ATTRIBUTE_IMG_{up}\n#define LV_ATTRIBUTE_IMG_{up}\n#endif\n")
    lines.append(f"const LV_ATTRIBUTE_MEM_ALIGN LV_ATTRIBUTE_LARGE_CONST LV_ATTRIBUTE_IMG_{up} uint8_t {sym}_map[] = {{")
    lines.append("    0x00, 0x00, 0x00, 0xff, /*Color of index 0*/")
    lines.append("    0xff, 0xff, 0xff, 0xff, /*Color of index 1*/\n")
    for y in range(h):
        row = data[y * stride:(y + 1) * stride]
        lines.append("    " + ", ".join(f"0x{v:02x}" for v in row) + ",")
    lines.append("};\n")
    lines.append(f"const lv_img_dsc_t {sym} = {{")
    lines.append("    .header.cf = LV_COLOR_FORMAT_I1,")
    lines.append(f"    .header.w = {w},")
    lines.append(f"    .header.h = {h},")
    lines.append(f"    .data_size = {len(data) + 8},")
    lines.append(f"    .data = {sym}_map,")
    lines.append("};")
    return "\n".join(lines) + "\n"


if __name__ == "__main__":
    # uso: imgtool.py decode <in.c> <out.png> [w h] [cw|ccw] [inv] [scale]
    # `inv`: para assets con la convención de zmk main/LVGL9 (bit 0 = negro)
    if sys.argv[1] == "decode":
        path, out = sys.argv[2], sys.argv[3]
        data, w, h = parse_c_bytes(path)
        args = sys.argv[4:]
        rot = None
        scale = 3
        nums = [a for a in args if a.isdigit()]
        if len(nums) >= 2:
            w, h = int(nums[0]), int(nums[1])
        if len(nums) == 3:
            scale = int(nums[2])
        if "cw" in args:
            rot = "cw"
        if "ccw" in args:
            rot = "ccw"
        stride = (w + 7) // 8
        print(f"bytes={len(data)} w={w} h={h} esperado={stride * h}")
        px = unpack_bitmap(data, w, h)
        if "inv" in args:
            px = [[1 - v for v in row] for row in px]
        if rot == "cw":
            px = rot_cw(px)
        elif rot == "ccw":
            px = rot_ccw(px)
        write_png(out, px, len(px[0]), len(px), scale)
        print(f"ok -> {out} ({len(px[0])}x{len(px)})")
