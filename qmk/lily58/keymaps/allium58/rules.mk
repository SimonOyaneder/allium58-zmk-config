# Pro Micro RP2040: el converter oficial genera un .uf2 para RP2040.
# Si tu controlador es otro clon, cambia el target: kb2040 (Adafruit),
# elite_pi, blok, sparkfun_pm2040...
CONVERT_TO = promicro_rp2040

MOUSEKEY_ENABLE = yes  # MS_BTN4/MS_BTN5 (atrás/adelante) en la capa High
EXTRAKEY_ENABLE = yes  # play/pause y mute en la capa Low
OLED_ENABLE = yes      # pantallas: estado en la izquierda, Luna en la derecha
WPM_ENABLE = yes       # contador de palabras por minuto (panel y Luna)
CAPS_WORD_ENABLE = yes # doble Shift = mayúsculas por una palabra
RGBLIGHT_ENABLE = yes  # underglow + por tecla (70 LEDs con el target lily58/light)
