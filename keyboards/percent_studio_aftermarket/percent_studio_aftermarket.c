/*
Copyright 2026 Jubakuba (Jubakuba@gmail.com)

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "quantum.h"

// 1. Define our color states
typedef enum {
    COLOR_NONE,
    COLOR_BACKSPACE,
    COLOR_ALPHA,
    COLOR_NUM,
    COLOR_SYMBOL,
    COLOR_SPACE,
    COLOR_QUANTUM,
    COLOR_OTHER
} key_type_t;

// 2. The Shift Register: Tracks the 4 LEDs (Index 0 = LED 1, Index 3 = LED 4)
key_type_t led_history[4] = {COLOR_NONE, COLOR_NONE, COLOR_NONE, COLOR_NONE};

// 3. Helper function to categorize the keycode
key_type_t get_key_category(uint16_t keycode) {
    uint8_t base_keycode = keycode & 0xFF; // Extract base code for Mod-Taps
    bool shifted = (get_mods() & (MOD_BIT(KC_LSFT) | MOD_BIT(KC_RSFT)));
    switch (keycode) {
        case QK_MOMENTARY ... QK_MOMENTARY_MAX:           // MO(layer)
        case QK_TOGGLE_LAYER ... QK_TOGGLE_LAYER_MAX:     // TG(layer)
        case QK_DEF_LAYER ... QK_DEF_LAYER_MAX:           // DF(layer)
        case QK_TO ... QK_TO_MAX:                         // TO(layer)
        case QK_ONE_SHOT_LAYER ... QK_ONE_SHOT_LAYER_MAX: // OSL(layer)
        case QK_LAYER_TAP_TOGGLE ... QK_LAYER_TAP_TOGGLE_MAX: // TT(layer)
        case QK_LAYER_MOD ... QK_LAYER_MOD_MAX:           // LM(layer, mod)
        case QK_LAYER_TAP ... QK_LAYER_TAP_MAX:           // LT(layer, key)
        case QK_MOD_TAP ... QK_MOD_TAP_MAX:
            return COLOR_QUANTUM; 
    }
    switch (base_keycode) {
        case KC_BSPC:
            return COLOR_BACKSPACE;
        // Alphas
        case KC_A ... KC_Z:
            return COLOR_ALPHA;
        case KC_SPACE:
            return COLOR_SPACE;
        // Alphas
        case KC_1 ... KC_0:
            if (shifted) {
                return COLOR_SYMBOL;
            }
            return COLOR_NUM;


        // Symbols (- = [ ] \ ; ' ` , . /)
        // In QMK, KC_MINUS (45) through KC_SLASH (56) covers the standard symbols neatly.
        case KC_MINUS ... KC_SLASH: 
            return COLOR_SYMBOL;

        // Ignore completely blank/transparent keys so they don't advance the LEDs
        case KC_NO:
        case KC_TRNS:
            return COLOR_NONE;

        // Space, Shift, Enter, and everything else
        default:
            return COLOR_OTHER;
    }
}

// 4. Function to apply the logic to the physical LEDs
void update_custom_leds(void) {
    // Check system Caps Lock state
    bool caps_on = host_keyboard_led_state().caps_lock;

    for (int i = 0; i < 4; i++) {
        // OVERRIDE: If this is LED 1 (index 0) and Caps is ON, force it Red.
        if (i == 0 && caps_on) {
            rgblight_sethsv_at(HSV_RED, i);
            continue; // Skip the rest of the loop for this LED
        }

        // NORMAL BEHAVIOR: Read from our shift register
        switch(led_history[i]) {
            case COLOR_BACKSPACE:
                rgblight_sethsv_at(HSV_RED, i);
                break;
            case COLOR_ALPHA:
                rgblight_sethsv_at(HSV_BLUE, i);
                break;
            case COLOR_NUM:
                rgblight_sethsv_at(HSV_WHITE, i);
                break;
            case COLOR_SYMBOL:
                rgblight_sethsv_at(HSV_YELLOW, i);
                break;
            case COLOR_OTHER:
                rgblight_sethsv_at(HSV_ORANGE, i);
                break;
            case COLOR_SPACE:
                rgblight_sethsv_at(HSV_GREEN, i);
                break;
            case COLOR_QUANTUM:
                rgblight_sethsv_at(HSV_PURPLE, i);
                break;
            default:
                rgblight_sethsv_at(HSV_BLUE, i); // Or a default background color
                break;
        }
    }
}

// 5. Hook into the key press event to advance the shift register
bool process_record_kb(uint16_t keycode, keyrecord_t *record) {
    if (record->event.pressed) {
        key_type_t new_color = get_key_category(keycode);

        // Only shift the LEDs if a valid key was pressed
        if (new_color != COLOR_NONE) {
            
            // Shift the array to the "left" (LED 2 takes 3, LED 1 takes 2)
            led_history[0] = led_history[1];
            led_history[1] = led_history[2];
            led_history[2] = led_history[3];
            
            // Push the new keystroke color into LED 4 (Index 3)
            led_history[3] = new_color;

            // Instantly update the LEDs to reflect the new state
            update_custom_leds();
        }
    }
    return process_record_user(keycode, record);
}

// 6. Hook into OS LED updates (so Caps Lock updates instantly even if toggled by the OS)
bool led_update_kb(led_t led_state) {
    update_custom_leds();
    return led_update_user(led_state);
}