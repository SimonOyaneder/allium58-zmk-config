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

// La esclava ya no necesita nada sincronizado: su pantalla (night_art.h)
// es autónoma, y Luna vive en la master donde el WPM es local.
// OJO: NO agregar SPLIT_MODS_ENABLE ni CAPS_WORD_ENABLE (ni otros syncs
// split) — en esta placa cuelgan la mitad esclava al arrancar por TRRS
// (verificado por bisección; el "caps word" está implementado a mano en
// keymap.c, solo en la master).

// Apaga el RGB cuando el Mac duerme.
#define RGBLIGHT_SLEEP
