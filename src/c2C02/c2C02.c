#include <c2C02.h>

void c2C02_cycle(C2C02 *const c) {
    c->dots++;
    if (c->dots >= 341) {
        c->dots = 0;
        c->scanlines++;
        if (c->scanlines >= 261) {
            c->scanlines = 0;
        }
    }
}

uint8_t c2C02_read_reg(C2C02 *const c, const uint8_t addr) {
    return 0;  // Todo
}

void c2C02_write_reg(C2C02 *const c, const uint8_t addr, const uint8_t val) {
    // Todo
}
