
#include <stdbool.h>
#include <stdint.h>

typedef struct C2C02 {
    int dots;
    int scanlines;
} C2C02;

void c2C02_cycle(C2C02 *);

uint8_t c2C02_read_reg(C2C02 *, uint8_t addr);
void c2C02_write_reg(C2C02 *, uint8_t addr, uint8_t val);
