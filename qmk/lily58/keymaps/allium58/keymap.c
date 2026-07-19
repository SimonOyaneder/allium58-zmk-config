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

/*
 * Caps Word adaptado a ES-ISO: además de las letras hay que mantener en
 * mayúscula la Ñ (KC_SCLN), y dejar pasar sin apagarlo la tecla muerta de
 * acento (KC_QUOT, para Á É Í Ó Ú) y el guión (KC_SLSH en ES-ISO).
 */
bool caps_word_press_user(uint16_t keycode) {
    switch (keycode) {
        case KC_A ... KC_Z:
        case KC_SCLN:  // Ñ
            add_weak_mods(MOD_BIT(KC_LSFT));
            return true;
        case KC_1 ... KC_0:
        case KC_QUOT:  // ´ (tecla muerta: acentos)
        case KC_SLSH:  // - (guión en ES-ISO)
        case KC_BSPC:
        case KC_DEL:
            return true;
        default:
            return false;  // cualquier otra tecla termina Caps Word
    }
}

#ifdef OLED_ENABLE

#include "luna.h"

/*
 * Pantallas OLED. En el Lily58 van montadas en vertical (el lado largo
 * apunta hacia arriba), así que la izquierda se dibuja rotada 270° como
 * panel vertical de 32x128 px (5 columnas x 16 líneas de texto):
 *   - badge de capa arriba (invertido en High/Low)
 *   - mods en grilla: C A / ^ S (se iluminan al presionarlos)
 *   - CAPS cuando Caps Word está activo (doble Shift = mayúsculas por
 *     una palabra)
 *   - WPM numérico
 *   - abajo, gráfico horizontal del WPM: 32 muestras cada 1 s (~32 s de
 *     historia) deslizando de derecha a izquierda sobre una línea base.
 * La derecha (esclava), también en vertical: Luna, la perrita de
 * HellSingCoder, corriendo en el piso de la pantalla según el WPM
 * (ver luna.h).
 * Si en tu montaje alguna pantalla queda de cabeza, cambia su
 * OLED_ROTATION_270 por OLED_ROTATION_90 (y avísame para dejarlo fijo).
 * Ambas se apagan tras OLED_TIMEOUT sin teclear (30 min, config.h).
 */

#define WPM_GRAPH_TOP 64    // el gráfico ocupa la mitad inferior (y 64-127)
#define WPM_GRAPH_MAX 100   // wpm que toca el techo del gráfico
#define WPM_SAMPLE_MS 1000

oled_rotation_t oled_init_user(oled_rotation_t rotation) {
    return OLED_ROTATION_270;
}

static void render_wpm_graph(void) {
    static uint8_t  samples[OLED_DISPLAY_HEIGHT] = {0};   // 32 muestras
    static uint8_t  head = 0;
    static uint32_t sample_timer = 0;

    if (timer_elapsed32(sample_timer) > WPM_SAMPLE_MS) {
        sample_timer = timer_read32();
        samples[head] = get_current_wpm();
        head = (head + 1) % OLED_DISPLAY_HEIGHT;
    }

    uint8_t prev_cy = 127;
    for (uint8_t x = 0; x < OLED_DISPLAY_HEIGHT; x++) {
        uint16_t wpm = samples[(head + x) % OLED_DISPLAY_HEIGHT];
        if (wpm > WPM_GRAPH_MAX) wpm = WPM_GRAPH_MAX;
        uint8_t cy = 127 - (wpm * (127 - WPM_GRAPH_TOP) / WPM_GRAPH_MAX);

        // curva unida en vertical con el punto anterior + línea base;
        // se redibuja la zona completa, así se borra el frame anterior
        uint8_t top = cy < prev_cy ? cy : prev_cy;
        uint8_t bot = cy > prev_cy ? cy : prev_cy;
        for (uint8_t y = WPM_GRAPH_TOP; y < 128; y++) {
            oled_write_pixel(x, y, (y >= top && y <= bot) || y == 127);
        }
        prev_cy = cy;
    }
}

static void render_status(void) {
    oled_set_cursor(0, 0);
    switch (get_highest_layer(layer_state)) {
        case _HIGH: oled_write_P(PSTR("HIGH "), true); break;
        case _LOW:  oled_write_P(PSTR(" LOW "), true); break;
        default:    oled_write_P(PSTR(" MAC "), false); break;
    }

    uint8_t mods = get_mods() | get_oneshot_mods();
    oled_set_cursor(1, 2);
    oled_write_P(PSTR("C"), mods & MOD_MASK_GUI);
    oled_write_P(PSTR(" "), false);
    oled_write_P(PSTR("A"), mods & MOD_MASK_ALT);
    oled_set_cursor(1, 3);
    oled_write_P(PSTR("^"), mods & MOD_MASK_CTRL);
    oled_write_P(PSTR(" "), false);
    oled_write_P(PSTR("S"), mods & MOD_MASK_SHIFT);

    oled_set_cursor(0, 5);
    bool cw = is_caps_word_on();
    oled_write_P(cw ? PSTR("CAPS ") : PSTR("     "), cw);

    oled_set_cursor(0, 6);
    oled_write(get_u8_str(get_current_wpm(), ' '), false);
    oled_set_cursor(1, 7);
    oled_write_P(PSTR("wpm"), false);

    render_wpm_graph();
}

bool oled_task_user(void) {
    if (is_keyboard_master()) {
        render_status();
    } else {
        render_luna();
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
