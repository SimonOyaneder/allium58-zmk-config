# allium58-zmk-config

Configuración ZMK para un Lily58 usado con la distribución de macOS **"Español — ISO"**.

<img src="keymap-drawer/lily58.svg" alt="keymap" width="900">

_Imagen generada con [caksoylar/keymap-drawer](https://github.com/caksoylar/keymap-drawer)._

## Regenerar la imagen del keymap

La imagen se regenera **automáticamente** en cada `push` que modifique
`config/*.keymap` o `keymap_drawer.config.yaml`, mediante el workflow
[`.github/workflows/draw-keymaps.yml`](.github/workflows/draw-keymaps.yml)
(que reusa el action oficial de keymap-drawer y commitea el `.svg` + `.yaml`).

Para regenerarla en local:

```sh
pipx install keymap-drawer        # o: pip install keymap-drawer
keymap -c keymap_drawer.config.yaml parse -z config/lily58.keymap > keymap-drawer/lily58.yaml
keymap -c keymap_drawer.config.yaml draw  keymap-drawer/lily58.yaml > keymap-drawer/lily58.svg
```

### Por qué hace falta configuración extra para el español

El keymap emite **keycodes US** (`&kp SEMI`, `&kp SQT`, `&kp LA(RBKT)`…), pero
macOS los interpreta con la distribución **"Español — ISO"**, así que el carácter
que sale de cada tecla no coincide con el nombre del keycode. `keymap_drawer.config.yaml`
usa `parse_config.raw_binding_map` para reetiquetar cada tecla con el símbolo real
que produce esa distribución (`Ñ`, `´`/`¨`, `¿` `¡`, y todos los símbolos de
programación del layer *High*: `\ + * [ ] ' < > { } ^ ...`).
