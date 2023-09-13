#pragma once

#include <stdbool.h>
#include <stdint.h>

/* Basic NES gamepad implementation, as described here:
https://www.nesdev.org/wiki/Standard_controller

Set button states by accessing the button member, e.g.:
`gamepad.button.A = 1;`

*/
typedef struct {
    union __attribute__((__packed__)) {
        struct __attribute__((__packed__)) {
            uint8_t A : 1;
            uint8_t B : 1;
            uint8_t Select : 1;
            uint8_t Start : 1;
            uint8_t Up : 1;
            uint8_t Down : 1;
            uint8_t Left : 1;
            uint8_t Right : 1;
        } buttons;
        uint8_t u8;
    };

    int _shift;
    bool _strobe;
} NesGamepad;

uint8_t nes_gamepad_read_reg(NesGamepad *);

void nes_gamepad_write_reg(NesGamepad *, uint8_t val);
