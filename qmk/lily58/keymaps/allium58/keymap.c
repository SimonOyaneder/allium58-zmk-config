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

#include "bongo.h"

/*
 * Pantallas OLED (128x32):
 *   - Mitad izquierda (master): panel de estado con dos zonas —
 *       texto (x 0-59): capa como badge invertido, mods C A ^ S que se
 *       iluminan al presionarlos, MAYUS y el WPM numérico;
 *       gráfico (x 64-127): curva del WPM, 64 muestras cada 500 ms
 *       (~32 s de historia) sobre una línea base, estilo nice!view.
 *   - Mitad derecha (esclava): Bongo Cat tamborileando según el WPM
 *     (ver bongo.h; necesita SPLIT_WPM_ENABLE para conocer el WPM).
 * Ambas se apagan solas tras OLED_TIMEOUT sin teclear (30 min, config.h)
 * y despiertan con cualquier pulsación.
 */

#define WPM_GRAPH_X 64
#define WPM_GRAPH_W 64
#define WPM_GRAPH_MAX 100   // wpm que toca el techo del gráfico
#define WPM_SAMPLE_MS 500

oled_rotation_t oled_init_user(oled_rotation_t rotation) {
    if (!is_keyboard_master()) return OLED_ROTATION_180;
    return rotation;
}

static void render_wpm_graph(void) {
    static uint8_t  samples[WPM_GRAPH_W] = {0};
    static uint8_t  head = 0;
    static uint32_t sample_timer = 0;

    if (timer_elapsed32(sample_timer) > WPM_SAMPLE_MS) {
        sample_timer = timer_read32();
        samples[head] = get_current_wpm();
        head = (head + 1) % WPM_GRAPH_W;
    }

    uint8_t prev_y = 31;
    for (uint8_t i = 0; i < WPM_GRAPH_W; i++) {
        uint16_t wpm = samples[(head + i) % WPM_GRAPH_W];
        if (wpm > WPM_GRAPH_MAX) wpm = WPM_GRAPH_MAX;
        uint8_t y = 31 - (wpm * 31 / WPM_GRAPH_MAX);

        // columna de 32 bits: punto de la curva, unido en vertical con el
        // punto anterior para que la línea no se corte, más la línea base
        uint32_t col = (uint32_t)1 << 31;
        uint8_t y0 = y < prev_y ? y : prev_y;
        uint8_t y1 = y > prev_y ? y : prev_y;
        for (uint8_t yy = y0; yy <= y1; yy++) col |= (uint32_t)1 << yy;
        prev_y = y;

        for (uint8_t page = 0; page < 4; page++) {
            oled_write_raw_byte((col >> (8 * page)) & 0xFF, page * 128 + WPM_GRAPH_X + i);
        }
    }
}

static void render_status(void) {
    oled_set_cursor(0, 0);
    switch (get_highest_layer(layer_state)) {
        case _HIGH: oled_write_P(PSTR(" High "), true); break;
        case _LOW:  oled_write_P(PSTR(" Low  "), true); break;
        default:    oled_write_P(PSTR(" Mac  "), false); break;
    }

    uint8_t mods = get_mods() | get_oneshot_mods();
    oled_set_cursor(0, 1);
    oled_write_P(PSTR("C"), mods & MOD_MASK_GUI);
    oled_write_P(PSTR(" "), false);
    oled_write_P(PSTR("A"), mods & MOD_MASK_ALT);
    oled_write_P(PSTR(" "), false);
    oled_write_P(PSTR("^"), mods & MOD_MASK_CTRL);
    oled_write_P(PSTR(" "), false);
    oled_write_P(PSTR("S"), mods & MOD_MASK_SHIFT);

    oled_set_cursor(0, 2);
    bool caps = host_keyboard_led_state().caps_lock;
    oled_write_P(caps ? PSTR("MAYUS") : PSTR("     "), caps);

    oled_set_cursor(0, 3);
    oled_write(get_u8_str(get_current_wpm(), ' '), false);
    oled_write_P(PSTR(" wpm"), false);

    render_wpm_graph();
}

bool oled_task_user(void) {
    if (is_keyboard_master()) {
        render_status();
    } else {
        render_bongo();
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
