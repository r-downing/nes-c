#include <c2C02.h>

void c2C02_cycle(C2C02 *c) {
    c->dots++;
    if (c->dots >= 341) {
        c->dots = 0;
        c->scanlines++;
        if (c->scanlines >= 261) {
            c->scanlines = 0;
        }
    }
}