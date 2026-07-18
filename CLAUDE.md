# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Qué es este repo

Config ZMK para un Lily58 ("Allium58_1") con controladores nice!nano y pantallas nice!view. Sin encoder, sin OLED. El teclado se usa en macOS con la distribución **"Española — ISO"** — esa premisa atraviesa todo el repo.

## Build

No hay build local: el firmware se compila en GitHub Actions (`.github/workflows/build.yml` → workflow reusable de zmkfirmware). Los targets están en `build.yaml`:

- `lily58_left` + `nice_view_adapter nice_view_gem` (con ZMK Studio habilitado)
- `lily58_right` + `nice_view_adapter nice_view_gem`
- `settings_reset`

El workflow de build ignora cambios en `keymap-drawer/**`, `keymap_drawer.config.yaml` y `**/*.md` para no compilar firmware por cambios de docs.

## Keymap (config/lily58.keymap)

Tres capas: `Mac` (base), `High` (símbolos de programación), `Low` (F-keys, BT, Studio). Los keycodes son US pero la salida real es ES-ISO, así que **el nombre del keycode no coincide con el símbolo producido**.

Reglas críticas que no hay que "corregir":

- macOS intercambia los scancodes `GRAVE` y `NON_US_BACKSLASH` en teclados ISO: `<` y `>` se envían con `GRAVE`/`LS(GRAVE)`, y `\` con `LA(NON_US_BACKSLASH)`. Enviar `NON_US_BACKSLASH` para `<` produce `º`.
- En ES-ISO `` ` `` y `^` son teclas muertas: las macros `&es_grave` y `&es_caret` las componen con espacio.
- `&bspc_qmark` es un mod-morph: Backspace, y con Shift → `?`.

## Al cambiar el keymap, sincronizar tres cosas

1. **`keymap_drawer.config.yaml`** — el `raw_binding_map` reetiqueta cada binding con el símbolo que realmente produce ES-ISO. Cualquier binding nuevo cuya salida difiera del keycode US necesita entrada aquí.
2. **La imagen del keymap** — se regenera sola al hacer push (`.github/workflows/draw-keymaps.yml` comitea `docs: regenerate keymap image via keymap-drawer`). Localmente:
   ```bash
   keymap -c keymap_drawer.config.yaml parse -z config/lily58.keymap > keymap-drawer/lily58.yaml
   keymap -c keymap_drawer.config.yaml draw keymap-drawer/lily58.yaml > keymap-drawer/lily58.svg
   ```
3. **`trainer/index.html`** — página estática autocontenida para practicar el layout; sus diagramas y pistas replican el keymap a mano y quedan desactualizados si no se tocan.
4. **`qmk/lily58/keymaps/allium58/`** — port QMK del keymap para el otro Lily58 (cableado, Pro Micro RP2040). Replica capas y macros a mano; ver `qmk/README.md`. El workflow de build ignora `qmk/**`.

## Shield nice_view_gem (boards/shields/nice_view_gem)

Shield **local** (copia adaptada de M165437/nice-view-gem con arte custom); `zephyr/module.yml` expone `boards/` como board root y por eso NO debe añadirse el módulo externo al `config/west.yml` (duplicaría el shield).

El CMakeLists divide por rol del split:

- **Central (izquierda)**: `widgets/screen.c` + `assets/moon_nika.c` — luna con Nika, fila superior con conexión + batería, puntos de perfil BT abajo.
- **Periférico (derecha)**: `widgets/screen_peripheral.c` + `assets/night_mountains.c` — paisaje nocturno.

Color: el pipeline del nice!view en zmk main **ya invierte** los colores LVGL al panel (blanco LVGL = pixel negro). Por eso `config/lily58.conf` NO define `CONFIG_NICE_VIEW_WIDGET_INVERTED` — no agregarlo.

## Pipeline de arte (art-editor/)

Arte 1-bit para las pantallas: canvas lógico 68×136, guardado **rotado 90° CW** en formato LVGL9 I1 dentro de los arrays C de `assets/`.

- `imgtool.py` — decodifica arrays C ↔ PNG, rotaciones, export al formato del shield.
- `generate_moon_nika.py` / `generate_night_mountains.py` — regeneran el arte **desde cero**. OJO: los `.c` de `assets/` son la fuente de verdad; hay retoques manuales (hechos con el editor) que se pierden si se reinstala la salida de estos scripts.
- `index.html` — editor visual que importa/exporta el formato del shield directamente.

## Convenciones

Commits en español, formato conventional commits (`feat:`, `fix:`, `docs:`, ...).
