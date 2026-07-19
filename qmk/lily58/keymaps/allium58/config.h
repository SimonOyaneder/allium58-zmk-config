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

// Sincroniza el WPM a la mitad esclava: Luna (OLED derecha) decide sus
// frames según el WPM.
// OJO: NO agregar SPLIT_MODS_ENABLE ni CAPS_WORD_ENABLE — en esta placa
// cuelgan la mitad esclava al arrancar por TRRS (verificado por bisección;
// el "caps word" está implementado a mano en keymap.c, solo en la master).
#define SPLIT_WPM_ENABLE

// Apaga el RGB cuando el Mac duerme.
#define RGBLIGHT_SLEEP
