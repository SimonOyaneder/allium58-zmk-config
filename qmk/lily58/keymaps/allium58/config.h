#pragma once

/*
 * QMK asume por defecto que la mitad master (la del USB) es la izquierda.
 * Si conectas el cable a la mitad derecha, descomenta:
 */
// #define MASTER_RIGHT

/*
 * Pantallas: apagado automático a los 30 min sin teclear (default: 60 s).
 * Ojo con el burn-in de los OLED: contenido estático mucho rato deja sombra
 * permanente con los meses; el timeout largo lo acelera.
 */
#define OLED_TIMEOUT 1800000

// Sincroniza WPM y modificadores a la mitad esclava: Luna (OLED derecha)
// decide sus frames según el WPM y reacciona a Shift/Ctrl/Alt/Cmd.
#define SPLIT_WPM_ENABLE
#define SPLIT_MODS_ENABLE

// Caps Word: mayúsculas por una palabra. Se activa con doble toque de
// Shift izquierdo o presionando ambos Shift a la vez; se apaga solo al
// terminar la palabra (espacio o cualquier tecla no alfanumérica).
#define DOUBLE_TAP_SHIFT_TURNS_ON_CAPS_WORD
#define BOTH_SHIFTS_TURNS_ON_CAPS_WORD

// Apaga el RGB cuando el Mac duerme.
#define RGBLIGHT_SLEEP
