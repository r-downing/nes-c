#pragma once

#include <c2C02.h>
#include <c6502.h>
#include <nes_cart.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct {
    uint8_t ram[0x800];
    NesCart cart;
    C6502 cpu;
    C2C02 ppu;

    int cpu_subcycle_count;
} NesBus;

bool nes_bus_write(NesBus *, uint16_t addr, uint8_t val);
uint8_t nes_bus_read(NesBus *, uint16_t addr);

void nes_bus_init(NesBus *const bus);

void nes_bus_cycle(NesBus *);

void nes_bus_reset(NesBus *);
