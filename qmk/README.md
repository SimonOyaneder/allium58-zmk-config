# Port QMK — Lily58 con Pro Micro RP2040

Réplica del keymap ZMK de este repo (`config/lily58.keymap`) para el otro
Lily58, el cableado con Pro Micro RP2040 y QMK. Mismas tres capas
(Mac/High/Low), mismo truco ES-ISO de macOS y mismas macros de teclas muertas.

## Qué cambia respecto a la versión ZMK

| ZMK | QMK |
|---|---|
| `&bt BT_SEL / BT_CLR` (capa Low) | Sin efecto en un teclado cableado. En la posición de BT CLR va `QK_BOOT` (bootloader RP2040, útil para flashear) |
| `&ext_power EP_TOG` (pantallas nice!view) | Nada (`XXXXXXX`) — este teclado no tiene pantallas |
| ZMK Studio | No portado (si lo quieres, el equivalente es Via/Vial) |

Todo lo demás es idéntico tecla por tecla.

## Opción A — compilar en la nube con qmk_userspace (sin instalar nada)

1. Crea un repo desde la plantilla [qmk/qmk_userspace](https://github.com/qmk/qmk_userspace)
   ("Use this template").
2. Copia la carpeta `qmk/lily58/` de este repo dentro de `keyboards/` del
   userspace, quedando `keyboards/lily58/keymaps/allium58/`.
3. En `qmk.json` del userspace agrega el build:
   ```json
   { "userspace_version": "1.0", "build_targets": [["lily58/light", "allium58"]] }
   ```
   (`lily58/light` = mismo Lily58 pero con los 70 LEDs RGB definidos —
   underglow + por tecla; la matriz es idéntica a `rev1`.)
4. Haz push: GitHub Actions compila y deja el `.uf2` como artifact del workflow.

## Opción B — compilar local

```bash
brew install qmk/qmk/qmk
qmk setup            # clona qmk_firmware en ~/qmk_firmware
mkdir -p ~/qmk_firmware/keyboards/lily58/keymaps
cp -r lily58/keymaps/allium58 ~/qmk_firmware/keyboards/lily58/keymaps/
qmk compile -kb lily58/light -km allium58
```

El `CONVERT_TO = promicro_rp2040` ya está en `rules.mk`, así que la salida es
un `.uf2` (en `~/qmk_firmware/`).

## Flashear

1. Pon la mitad en modo bootloader: doble toque al botón RESET del Pro Micro
   RP2040 (o mantén BOOT al conectarlo). Aparece el volumen `RPI-RP2`.
   Con el firmware ya instalado también sirve la tecla BOOT de la capa Low.
2. Arrastra el `.uf2` al volumen. Se reinicia solo.
3. Repite con la otra mitad — **el mismo archivo para ambas**.
4. Conecta el USB a la mitad **izquierda** (QMK asume master izquierda por
   defecto; si usas la derecha, descomenta `MASTER_RIGHT` en `config.h`).

## Verificación rápida tras flashear

Con macOS en "Española — ISO":

- `High + U` debe escribir `+` y `High + Y` debe escribir `\`.
- `High + J` debe escribir `<` y `High + K` debe escribir `>` (si salen `º`/`ª`,
  el sistema no está en la distribución correcta).
- `High + Backspace` debe escribir `` ` `` suelto y `High + ´¨` (la tecla a la
  derecha de Ñ) un `^` suelto (macros de tecla muerta + espacio).

## Mantenimiento

Si cambias `config/lily58.keymap` (ZMK), este port **no se actualiza solo**:
replica el cambio a mano en `keymap.c`, igual que con `trainer/index.html`.
