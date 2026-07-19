#include QMK_KEYBOARD_H
#include "transactions.h"
#include <string.h>

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
    // Palanca del tragamonedas (tecla interior derecha, sobre el pulgar).
    SLOT_SPIN,
};

static void cwl_process(uint16_t keycode, keyrecord_t *record);

#ifdef OLED_ENABLE
static uint32_t luna_jump_timer = 0;  // espacio -> Luna salta (OLED izquierda)

/*
 * Tragamonedas 🎰 — la lógica corre en la master (palanca SLOT_SPIN,
 * puntos en EEPROM) y el juego se DIBUJA en la pantalla derecha: el
 * estado viaja por una transacción split custom (RPC_ID_SLOT_SYNC, cada
 * 150 ms). Probado en esta placa: las transacciones custom NO la cuelgan
 * (el problema real era solo CAPS_WORD_ENABLE).
 * Matemática de casino: 8 símbolos, la jugada cuesta 5. Par = devuelve
 * los 5 (33% de las veces), trío +100 (1.4%), trío de 7 +777 (0.2%).
 * Esperanza por jugada ≈ -0.5: la casa tiene ~9% de ventaja, como todo
 * casino digno. Se permite deuda (hasta -9999) — el casino siempre fía;
 * el marcador se invierte cuando estás en rojo.
 */
static uint8_t  slot_state = 0;  // 0 sin juego, 1 girando, 2 resultado
static uint32_t slot_timer = 0;
static uint32_t slot_step  = 0;
static uint8_t  slot_reel[3];
static bool     slot_stopped[3];
static int32_t  slot_points = 0;
static int32_t  slot_win = 0;
static uint32_t slot_rng = 12345;

typedef struct {
    uint8_t  state;
    uint8_t  reel[3];
    uint8_t  stopped;  // bits 0-2
    int16_t  points;   // con signo: la deuda existe
    int16_t  win;
} slot_sync_t;
static slot_sync_t slot_rx;          // lo último recibido en la esclava
static uint32_t    slot_rx_time = 0;

static void slot_sync_handler(uint8_t in_buflen, const void* in_data, uint8_t out_buflen, void* out_data) {
    if (in_buflen == sizeof(slot_sync_t)) {
        memcpy(&slot_rx, in_data, sizeof(slot_sync_t));
        slot_rx_time = timer_read32();
    }
}

static void slot_evaluate(void) {
    uint8_t a = slot_reel[0], b = slot_reel[1], d = slot_reel[2];
    if (a == b && b == d)                slot_win = (a == 0) ? 777 : 100;
    else if (a == b || b == d || a == d) slot_win = 5;   // par: recuperas la apuesta
    else                                 slot_win = 0;
    slot_points += slot_win - 5;  // la jugada cuesta 5 (y se puede deber)
    if (slot_points < -9999) slot_points = -9999;
    if (slot_points > 32767) slot_points = 32767;
    eeconfig_update_user((uint32_t)slot_points);
    if (slot_win > 0) luna_jump_timer = timer_read32();  // Luna celebra
}

// Avanza el juego (solo master, desde housekeeping)
static void slot_task_master(void) {
    if (slot_state == 1) {
        if (timer_elapsed32(slot_step) > 90) {
            slot_step = timer_read32();
            for (uint8_t i = 0; i < 3; i++) {
                if (!slot_stopped[i]) {
                    slot_rng = slot_rng * 1103515245u + 12345u;
                    slot_reel[i] = (slot_rng >> 16) % 8;
                }
            }
        }
        static const uint16_t stop_at[3] = {900, 1500, 2100};
        uint32_t el = timer_elapsed32(slot_timer);
        bool all = true;
        for (uint8_t i = 0; i < 3; i++) {
            if (el >= stop_at[i]) slot_stopped[i] = true;
            if (!slot_stopped[i]) all = false;
        }
        if (all) {
            slot_state = 2;
            slot_timer = timer_read32();
            slot_evaluate();
        }
    } else if (slot_state == 2 && timer_elapsed32(slot_timer) > 5000) {
        slot_state = 0;  // la derecha vuelve a la noche
    }
}
#endif

void keyboard_post_init_user(void) {
#ifdef OLED_ENABLE
    uint32_t raw = eeconfig_read_user();
    slot_points = (raw == 0xFFFFFFFFu) ? 0 : (int32_t)raw;  // EEPROM virgen
    if (slot_points < -9999 || slot_points > 32767) slot_points = 0;
    transaction_register_rpc(RPC_ID_SLOT_SYNC, slot_sync_handler);
#endif
}

bool process_record_user(uint16_t keycode, keyrecord_t *record) {
    cwl_process(keycode, record);
#ifdef OLED_ENABLE
    if (record->event.pressed && keycode == KC_SPC) {
        luna_jump_timer = timer_read32();
    }
#endif
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

        case SLOT_SPIN:
#ifdef OLED_ENABLE
            if (record->event.pressed && slot_state != 1) {
                slot_rng ^= timer_read32();  // entropía del momento del tirón
                slot_state = 1;
                slot_timer = timer_read32();
                slot_step  = timer_read32();
                for (uint8_t i = 0; i < 3; i++) slot_stopped[i] = false;
            }
#endif
            return false;
    }
    return true;
}

/*
 * "Caps word" casero: mayúsculas por una palabra, activado con doble toque
 * de Shift (izquierdo o derecho). NO usar CAPS_WORD_ENABLE de QMK: en esta
 * placa cuelga la mitad esclava al arrancar conectada por TRRS (verificado
 * por bisección de builds — la feature toca la sincronización split).
 * Esta versión corre 100% en la master y está adaptada a ES-ISO: pone en
 * mayúscula letras y Ñ (KC_SCLN), y no se corta con la tecla muerta de
 * acento (KC_QUOT), el guión (KC_SLSH), números ni Backspace.
 */
#define CWL_TAP_TERM 300      // ventana del doble toque de Shift (ms)
#define CWL_IDLE_TIMEOUT 5000 // se apaga solo tras 5 s sin teclear

static bool     cwl_active = false;
static bool     cwl_shift_solo = false; // el Shift actual no acompañó a otra tecla
static uint16_t cwl_last_tap = 0;
static uint16_t cwl_activity = 0;

static void cwl_process(uint16_t keycode, keyrecord_t *record) {
    switch (keycode) {
        case KC_LSFT:
        case KC_RSFT:
            if (record->event.pressed) {
                cwl_shift_solo = true;
            } else if (cwl_shift_solo) {
                if (timer_elapsed(cwl_last_tap) < CWL_TAP_TERM) {
                    cwl_active   = true;
                    cwl_activity = timer_read();
                }
                cwl_last_tap = timer_read();
            }
            return;
        default:
            if (!record->event.pressed) return;
            cwl_shift_solo = false;
            if (!cwl_active) return;
            cwl_activity = timer_read();
            switch (keycode) {
                case KC_A ... KC_Z:
                case KC_SCLN:  // Ñ
                    add_weak_mods(MOD_BIT(KC_LSFT));
                    break;
                case KC_1 ... KC_0:
                case KC_QUOT:  // ´ (tecla muerta: acentos)
                case KC_SLSH:  // - (guión en ES-ISO)
                case KC_BSPC:
                case KC_DEL:
                    break;
                default:
                    cwl_active = false;  // fin de la palabra
            }
    }
}

void housekeeping_task_user(void) {
    if (cwl_active && timer_elapsed(cwl_activity) > CWL_IDLE_TIMEOUT) {
        cwl_active = false;
    }
#ifdef OLED_ENABLE
    if (is_keyboard_master()) {
        slot_task_master();
        static uint32_t last_sync = 0;
        if (timer_elapsed32(last_sync) > 150) {
            last_sync = timer_read32();
            slot_sync_t d = {
                .state   = slot_state,
                .reel    = {slot_reel[0], slot_reel[1], slot_reel[2]},
                .stopped = (uint8_t)((slot_stopped[0] ? 1 : 0) | (slot_stopped[1] ? 2 : 0) | (slot_stopped[2] ? 4 : 0)),
                .points  = (int16_t)slot_points,
                .win     = (int16_t)slot_win,
            };
            transaction_rpc_send(RPC_ID_SLOT_SYNC, sizeof(d), &d);
        }
    }
#endif
}

#ifdef OLED_ENABLE

#include "luna.h"

/*
 * Pantallas OLED. En el Lily58 van montadas en vertical (el lado largo
 * apunta hacia arriba), así que la izquierda se dibuja rotada 270° como
 * panel vertical de 32x128 px (5 columnas x 16 líneas de texto):
 *   - badge de capa arriba (invertido en High/Low)
 *   - mods en grilla: C A / ^ S (se iluminan al presionarlos)
 *   - CAPS cuando el caps word casero está activo (doble Shift)
 *   - WPM numérico + récord de la sesión (max)
 *   - abajo, Luna (luna.h) con reacciones en tiempo real: la master
 *     conoce todas las teclas, así que aquí sí puede saltar con espacio,
 *     ladrar con Shift y agacharse con Ctrl/Alt/Cmd; el resto por WPM.
 * La derecha (esclava): "Cordillera y luna" (night_art.h), arte 1-bit
 * hermano del nice!view del Allium58, con estrellas que titilan y los
 * reflejos del lago desplazándose — es 100% autónomo (nada viaja por el
 * TRRS) y el movimiento constante evita el burn-in del timeout largo.
 * Si en tu montaje alguna pantalla queda de cabeza, cambia su
 * OLED_ROTATION_270 por OLED_ROTATION_90 (y avísame para dejarlo fijo).
 * Ambas se apagan tras OLED_TIMEOUT sin teclear (30 min, config.h).
 */

#include "night_art.h"

#define LUNA_JUMP_MS 250    // cuánto dura el salto tras presionar espacio

oled_rotation_t oled_init_user(oled_rotation_t rotation) {
    return OLED_ROTATION_270;
}

/*
 * Noche animada: el frame base se dibuja una vez; cada tick solo se
 * retocan las estrellas (titilan con un pseudo-aleatorio determinista)
 * y las franjas de reflejo del lago (se desplazan, filas alternadas en
 * direcciones opuestas para que parezca agua).
 */
static bool night_base_drawn = false;  // se resetea al volver del tragamonedas

static void render_night(void) {
    static uint32_t tick_timer = 0;
    static uint8_t  tick = 0;

    if (!night_base_drawn) {
        oled_set_cursor(0, 0);
        oled_write_raw_P(night_base, sizeof(night_base));
        night_base_drawn = true;
    }
    if (timer_elapsed32(tick_timer) < NIGHT_TICK_MS) return;
    tick_timer = timer_read32();
    tick++;

    for (uint8_t i = 0; i < NIGHT_STAR_COUNT; i++) {
        bool on = ((tick * 31u + i * 17u) % 7u) < 5u;
        oled_write_pixel(night_stars[i][0], night_stars[i][1], on);
    }

    for (uint8_t b = 0; b < NIGHT_BAND_COUNT; b++) {
        uint8_t y  = night_bands[b][0];
        uint8_t ph = (y & 1) ? tick : (uint8_t)(0u - tick);
        for (uint8_t x = night_bands[b][1]; x < night_bands[b][2]; x++) {
            oled_write_pixel(x, y, ((x + ph) & 3) < 2);
        }
    }
}

static void render_luna_master(void) {
    static uint32_t anim_timer = 0;
    static uint8_t  frame = 0;
    if (timer_elapsed32(anim_timer) > LUNA_FRAME_DURATION) {
        anim_timer = timer_read32();
        frame = (frame + 1) % LUNA_FRAMES;
    }

    bool jumping = timer_elapsed32(luna_jump_timer) < LUNA_JUMP_MS;
    // al saltar se dibuja una fila más arriba y se limpia el piso;
    // al aterrizar, se limpia la fila del salto
    luna_clear_row(jumping ? LUNA_ROW + 2 : LUNA_ROW - 1);

    uint8_t mods = get_mods();
    uint8_t wpm  = get_current_wpm();
    const char(*frames)[3][LUNA_CHUNK];
    if (mods & MOD_MASK_SHIFT)     frames = luna_bark;
    else if (mods & MOD_MASK_CAG)  frames = luna_sneak;
    else if (wpm <= LUNA_MIN_WALK) frames = luna_sit;
    else if (wpm <= LUNA_MIN_RUN)  frames = luna_walk;
    else                           frames = luna_run;
    luna_draw_at(frames, frame, jumping ? LUNA_ROW - 1 : LUNA_ROW);
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
    oled_write_P(cwl_active ? PSTR("CAPS ") : PSTR("     "), cwl_active);

    uint8_t wpm = get_current_wpm();
    static uint8_t wpm_max = 0;  // récord de la sesión (desde que se conectó)
    if (wpm > wpm_max) wpm_max = wpm;

    oled_set_cursor(0, 6);
    oled_write(get_u8_str(wpm, ' '), false);
    oled_set_cursor(1, 7);
    oled_write_P(PSTR("wpm"), false);

    oled_set_cursor(0, 8);
    oled_write(get_u8_str(wpm_max, ' '), false);
    oled_set_cursor(1, 9);
    oled_write_P(PSTR("max"), false);

    render_luna_master();
}


// El casino en la pantalla derecha: dibuja lo que llegó por el sync
static void render_slot_right(void) {
    oled_set_cursor(0, 0);
    oled_write_P(PSTR("SLOT "), true);
    oled_set_cursor(0, 2);
    oled_write_P(PSTR("+---+"), false);
    oled_set_cursor(0, 3);
    oled_write_P(PSTR("|"), false);
    for (uint8_t i = 0; i < 3; i++) {
        char s[2] = {"7$*#OX%&"[slot_rx.reel[i] % 8], 0};
        oled_write(s, !(slot_rx.stopped & (1 << i)));  // invertido mientras gira
    }
    oled_write_P(PSTR("|"), false);
    oled_set_cursor(0, 4);
    oled_write_P(PSTR("+---+"), false);

    oled_set_cursor(0, 6);
    oled_write_P(PSTR("PTS"), false);
    oled_set_cursor(0, 7);
    // marcador con signo, alineado a la derecha en 5 columnas;
    // invertido cuando estás en deuda
    {
        char     buf[6];
        bool     neg = slot_rx.points < 0;
        uint16_t a   = neg ? -slot_rx.points : slot_rx.points;
        buf[5] = '\0';
        for (int8_t i = 4; i >= 0; i--) {
            if (a != 0 || i == 4) {
                buf[i] = '0' + a % 10;
                a /= 10;
            } else if (neg) {
                buf[i] = '-';
                neg = false;
            } else {
                buf[i] = ' ';
            }
        }
        oled_write(buf, slot_rx.points < 0);
    }

    oled_set_cursor(0, 9);
    if (slot_rx.state == 2) {
        if (slot_rx.win >= 777)      oled_write_P(PSTR("777!!"), true);
        else if (slot_rx.win >= 100) oled_write_P(PSTR("+100!"), true);
        else if (slot_rx.win > 0)    oled_write_P(PSTR("par  "), false);
        else                         oled_write_P(PSTR("nada "), false);
    } else {
        oled_write_P(PSTR("     "), false);
    }
}

bool oled_task_user(void) {
    if (is_keyboard_master()) {
        render_status();
    } else {
        // juego activo = estado reciente y distinto de 0; si el sync se
        // corta más de 2 s, vuelve la noche por seguridad
        bool gaming = slot_rx.state != 0 && timer_elapsed32(slot_rx_time) < 2000;
        static bool was_gaming = false;
        if (gaming != was_gaming) {
            was_gaming = gaming;
            oled_clear();
            night_base_drawn = false;
        }
        if (gaming) {
            render_slot_right();
        } else {
            render_night();
        }
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
     *
     * La tecla interior derecha (sobre el pulgar, antes muerta) es la
     * palanca del tragamonedas 🎰: cada toque hace girar los rodillos en
     * la pantalla izquierda.
     */
    [_MAC] = LAYOUT(
        KC_ESC,  KC_1, KC_2, KC_3, KC_4, KC_5,                        KC_6, KC_7, KC_8,    KC_9,   KC_0,    KC_MINS,
        KC_TAB,  KC_Q, KC_W, KC_E, KC_R, KC_T,                        KC_Y, KC_U, KC_I,    KC_O,   KC_P,    KC_BSPC,
        KC_LCTL, KC_A, KC_S, KC_D, KC_F, KC_G,                        KC_H, KC_J, KC_K,    KC_L,   KC_SCLN, KC_QUOT,
        KC_LSFT, KC_Z, KC_X, KC_C, KC_V, KC_B, XXXXXXX,     SLOT_SPIN, KC_N, KC_M, KC_COMM, KC_DOT, KC_SLSH, KC_RSFT,
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
