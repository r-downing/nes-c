#include <nes_gamepad.h>

uint8_t nes_gamepad_read_reg(NesGamepad *const g) {
    uint8_t status_bit = 1;  // will read all 1's after shifting through all buttons
    if (g->_shift < 8) {
        status_bit = g->u8 >> g->_shift;
        g->_shift++;
    }
    if (g->_strobe) {
        g->_shift = 0;  // strobe enabled, shift continuously reloaded
    }
    const union __attribute__((__packed__)) {
        struct __attribute__((__packed__)) {
            uint8_t status_bit : 1;
            uint8_t expansion_status_bit : 1;
            uint8_t microphone_status_bit : 1;
            uint8_t : 1;
            uint8_t residual_addr_msb : 4;
        };
        uint8_t u8;
    } output_reg = {
        .status_bit = status_bit,
        .residual_addr_msb = 0x4,  // not driven - retains previous byte on bus (controller addr)
    };

    return output_reg.u8;
}

void nes_gamepad_write_reg(NesGamepad *const g, const uint8_t val) {
    g->_strobe = val & 1;
    if (g->_strobe) {
        g->_shift = 0;
    }
}