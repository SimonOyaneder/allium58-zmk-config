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

// Transacción custom master->esclava: el estado del tragamonedas se
// dibuja en la pantalla derecha. Probado en esta placa: las transacciones
// custom NO cuelgan la esclava.
#define SPLIT_TRANSACTION_IDS_USER RPC_ID_SLOT_SYNC

// OJO: NO activar CAPS_WORD_ENABLE — demostrado por bisección que cuelga
// la mitad esclava al arrancar por TRRS (el "caps word" está implementado
// a mano en keymap.c, solo en la master). SPLIT_MODS_ENABLE quedó sin
// probar aislado; si algún día se quiere, probar con cautela.

// Apaga el RGB cuando el Mac duerme.
#define RGBLIGHT_SLEEP
