#include QMK_KEYBOARD_H

/*
 * Port a QMK del keymap ZMK del Allium58 (config/lily58.keymap), para el
 * Lily58 cableado con Pro Micro RP2040. Mismas tres capas: Mac (base),
 * High (símbolos de programación) y Low (F-keys, flechas, media).
 *
 * Todo el keymap asume macOS con la distribución "Española — ISO".
 * Ñ, ´¨, coma/punto/guión con sus Shift y ¡¿ son nativos de esa distribución;
 * los símbolos de programación de la capa High se generan con los combos
 * Shift/Option propios del teclado español de Mac.
 *
 * OJO: macOS intercambia los scancodes GRAVE y NON_US_BACKSLASH en teclados
 * ISO. Por eso aquí <, > se envían con KC_GRV / S(KC_GRV), y \ con
 * A(KC_NUBS). No revertir: enviar KC_NUBS para < da "º".
 *
 * Diferencias con la versión ZMK (todo lo demás es idéntico):
 *   - Sin Bluetooth ni ext_power (hardware cableado sin pantallas). En el
 *     hueco de BT CLR (capa Low) va QK_BOOT: entra al bootloader RP2040
 *     para flashear sin tocar el botón físico.
 */

enum layers { _MAC, _HIGH, _LOW };

enum custom_keycodes {
    // En ES-ISO ` y ^ son teclas muertas: se componen con espacio
    // para obtener el carácter suelto (macros &es_grave / &es_caret en ZMK).
    ES_GRAVE = SAFE_RANGE,
    ES_CARET,
};

bool process_record_user(uint16_t keycode, keyrecord_t *record) {
    switch (keycode) {
        case ES_GRAVE:
            if (record->event.pressed) {
                tap_code(KC_LBRC);
                tap_code(KC_SPC);
            }
            return false;

        case ES_CARET:
            if (record->event.pressed) {
                tap_code16(S(KC_LBRC));
                tap_code(KC_SPC);
            }
            return false;
    }
    return true;
}

#ifdef OLED_ENABLE

/*
 * Pantallas OLED (128x32, 4 líneas de 21 caracteres):
 *   - Mitad izquierda (master): capa activa, modificadores, mayúsculas y WPM.
 *   - Mitad derecha (esclava): logo QMK (glifos 0x80-0xD4 de la fuente por
 *     defecto del driver).
 * Ambas se apagan solas tras 60 s sin teclear (OLED_TIMEOUT por defecto)
 * y despiertan con cualquier pulsación.
 */

oled_rotation_t oled_init_user(oled_rotation_t rotation) {
    if (!is_keyboard_master()) return OLED_ROTATION_180;
    return rotation;
}

static void render_status(void) {
    oled_write_P(PSTR("Capa: "), false);
    switch (get_highest_layer(layer_state)) {
        case _HIGH: oled_write_ln_P(PSTR("High"), false); break;
        case _LOW:  oled_write_ln_P(PSTR("Low"), false); break;
        default:    oled_write_ln_P(PSTR("Mac"), false); break;
    }

    uint8_t mods = get_mods() | get_oneshot_mods();
    oled_write_P(mods & MOD_MASK_GUI ? PSTR("CMD ") : PSTR("    "), false);
    oled_write_P(mods & MOD_MASK_ALT ? PSTR("OPT ") : PSTR("    "), false);
    oled_write_P(mods & MOD_MASK_CTRL ? PSTR("CTL ") : PSTR("    "), false);
    oled_write_ln_P(mods & MOD_MASK_SHIFT ? PSTR("SHFT") : PSTR("    "), false);

    oled_write_ln_P(host_keyboard_led_state().caps_lock ? PSTR("MAYUS") : PSTR("     "), false);

    oled_write_P(PSTR("WPM: "), false);
    oled_write_ln(get_u8_str(get_current_wpm(), ' '), false);
}

static void render_logo(void) {
    static const char PROGMEM qmk_logo[] = {
        0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8A, 0x8B, 0x8C, 0x8D, 0x8E, 0x8F, 0x90, 0x91, 0x92, 0x93, 0x94,
        0xA0, 0xA1, 0xA2, 0xA3, 0xA4, 0xA5, 0xA6, 0xA7, 0xA8, 0xA9, 0xAA, 0xAB, 0xAC, 0xAD, 0xAE, 0xAF, 0xB0, 0xB1, 0xB2, 0xB3, 0xB4,
        0xC0, 0xC1, 0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7, 0xC8, 0xC9, 0xCA, 0xCB, 0xCC, 0xCD, 0xCE, 0xCF, 0xD0, 0xD1, 0xD2, 0xD3, 0xD4, 0x00
    };
    oled_write_P(qmk_logo, false);
}

bool oled_task_user(void) {
    if (is_keyboard_master()) {
        render_status();
    } else {
        render_logo();
    }
    return false;
}

#endif

const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {

    /* MAC (base)
     * ,-----------------------------------------.                    ,-----------------------------------------.
     * | ESC  |  1   |  2   |  3   |  4   |  5   |                    |  6   |  7   |  8   |  9   |  0   | ' ?  |
     * |------+------+------+------+------+------|                    |------+------+------+------+------+------|
     * | Tab  |  Q   |  W   |  E   |  R   |  T   |                    |  Y   |  U   |  I   |  O   |  P   |BkSp  |
     * |------+------+------+------+------+------|                    |------+------+------+------+------+------|
     * | Ctrl |  A   |  S   |  D   |  F   |  G   |-------.    ,-------|  H   |  J   |  K   |  L   |  Ñ   | ´ ¨  |
     * |------+------+------+------+------+------|       |    |       |------+------+------+------+------+------|
     * |Shift |  Z   |  X   |  C   |  V   |  B   |-------|    |-------|  N   |  M   | , ;  | . :  | - _  |Shift |
     * `-----------------------------------------/       /     \      \-----------------------------------------'
     *                   | LAlt | HIGH | LCmd | /Space  /       \Space \  |Enter | LOW  | RAlt |
     *                   `----------------------------'           '------''--------------------'
     */
    [_MAC] = LAYOUT(
        KC_ESC,  KC_1, KC_2, KC_3, KC_4, KC_5,                        KC_6, KC_7, KC_8,    KC_9,   KC_0,    KC_MINS,
        KC_TAB,  KC_Q, KC_W, KC_E, KC_R, KC_T,                        KC_Y, KC_U, KC_I,    KC_O,   KC_P,    KC_BSPC,
        KC_LCTL, KC_A, KC_S, KC_D, KC_F, KC_G,                        KC_H, KC_J, KC_K,    KC_L,   KC_SCLN, KC_QUOT,
        KC_LSFT, KC_Z, KC_X, KC_C, KC_V, KC_B, XXXXXXX,     XXXXXXX, KC_N, KC_M, KC_COMM, KC_DOT, KC_SLSH, KC_RSFT,
                   KC_LALT, MO(_HIGH), KC_LGUI, KC_SPC,     KC_SPC, KC_ENT, MO(_LOW), KC_RALT
    ),

    /* HIGH (símbolos de programación)
     * ,-----------------------------------------.                    ,-----------------------------------------.
     * |Emoji |      |      |      |      |      |                    |      |      |      |      |      |  ?   |
     * |------+------+------+------+------+------|                    |------+------+------+------+------+------|
     * |      |      |      |      |      |      |                    |  \   |  +   |  *   |  [   |  ]   |  `   |
     * |------+------+------+------+------+------|                    |------+------+------+------+------+------|
     * |      | 🖱◀  | 🖱▶  |      |      |      |-------.    ,-------|  '   |  <   |  >   |  {   |  }   |  ^   |
     * |------+------+------+------+------+------|Captura|    |       |------+------+------+------+------+------|
     * |      |      |      |      |      |      |-------|    |-------|  ¿   |  ¡   |      |      |      |      |
     * `-----------------------------------------/       /     \      \-----------------------------------------'
     *
     * A/S de la mano izquierda son atrás/adelante con los botones 4/5 del
     * mouse (MS_BTN4/MS_BTN5), el mismo HID de los botones laterales de un
     * mouse Logitech MX. Emoji abre el selector de emojis (⌃⌘Espacio).
     * Captura (tecla interior izquierda) es ⇧⌘4.
     */
    [_HIGH] = LAYOUT(
        C(G(KC_SPC)), _______, _______, _______, _______, _______,                         _______,    _______, _______,    _______,    _______,    S(KC_MINS),
        _______,      _______, _______, _______, _______, _______,                         A(KC_NUBS), KC_RBRC, S(KC_RBRC), A(KC_LBRC), A(KC_RBRC), ES_GRAVE,
        _______,      MS_BTN4, MS_BTN5, _______, _______, _______,                         KC_MINS,    KC_GRV,  S(KC_GRV),  A(KC_QUOT), A(KC_BSLS), ES_CARET,
        _______,      _______, _______, _______, _______, _______, S(G(KC_4)),    _______, S(KC_EQL),  KC_EQL,  _______,    _______,    _______,    _______,
                                     _______, _______, _______, _______,    _______, _______, _______, _______
    ),

    /* LOW (funciones, flechas, escritorios, media y RGB)
     * ,-----------------------------------------.                    ,-----------------------------------------.
     * |      |  F1  |  F2  |  F3  |  F4  |  F5  |                    |  F6  |  F7  |  F8  |  F9  | F10  | F11  |
     * |------+------+------+------+------+------|                    |------+------+------+------+------+------|
     * |RGB ⏻ |EFX ▶ |EFX ◀ |      |      | BOOT |                    |      | ⌃ <- |  ^   | ⌃ -> |      | F12  |
     * |------+------+------+------+------+------|                    |------+------+------+------+------+------|
     * |      | HUE+ | SAT+ | BRI+ | VEL+ |      |-------.    ,-------|      |  <-  |  v   |  ->  |      |      |
     * |------+------+------+------+------+------| Play  |    | Mute  |------+------+------+------+------+------|
     * |      | HUE- | SAT- | BRI- | VEL- |      |-------|    |-------|      |      |      |      |      |LOCK  |
     * `-----------------------------------------/       /     \      \-----------------------------------------'
     *
     * RGB en la mano izquierda: encender/apagar, efecto siguiente/anterior,
     * y columnas de matiz/saturación/brillo/velocidad (fila 3 sube, fila 4
     * baja). Los ajustes persisten en la EEPROM emulada del RP2040.
     * ⌃← / ⌃→ cambian de escritorio en macOS. LOCK (⌃⌘Q) bloquea la pantalla.
     * BOOT (posición de BT CLR en ZMK) entra al bootloader RP2040 para flashear.
     */
    [_LOW] = LAYOUT(
        _______, KC_F1,   KC_F2,   KC_F3,   KC_F4,   KC_F5,                       KC_F6,   KC_F7,      KC_F8,   KC_F9,      KC_F10,  KC_F11,
        UG_TOGG, UG_NEXT, UG_PREV, _______, _______, QK_BOOT,                     _______, C(KC_LEFT), KC_UP,   C(KC_RGHT), _______, KC_F12,
        _______, UG_HUEU, UG_SATU, UG_VALU, UG_SPDU, _______,                     _______, KC_LEFT,    KC_DOWN, KC_RGHT,    _______, _______,
        XXXXXXX, UG_HUED, UG_SATD, UG_VALD, UG_SPDD, _______, KC_MPLY,   KC_MUTE, _______, _______,    _______, _______,    _______, C(G(KC_Q)),
                                _______, _______, _______, _______,    _______, _______, _______, _______
    ),
};
