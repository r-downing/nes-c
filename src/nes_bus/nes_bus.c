
#include <c6502.h>
#include <nes_bus.h>

// https://www.nesdev.org/wiki/CPU_memory_map

void nes_bus_write(NesBus *bus, uint16_t addr, uint8_t val) {
    if (nes_cart_cpu_write(&bus->cart, addr, val)) {
        return;
    }
    if (addr < 0x2000) {
        bus->ram[addr & 0x7FF] = val;
    }
    if (addr < 0x4000) {
        return;  // addr & 0x7; ToDo: PPU
    }
    if (addr < 0x4020) {
        return;  // ToDo: APU and IO
    }
}

uint8_t nes_bus_read(NesBus *bus, uint16_t addr) {
    uint8_t val;
    if (nes_cart_cpu_read(&bus->cart, addr, &val)) {
        return val;
    }
    if (addr < 0x2000) {
        return bus->ram[addr & 0x7FF];
    }
    if (addr < 0x4000) {
        return 0;  // addr & 0x7; ToDo: PPU
    }
    if (addr < 0x4020) {
        return 0;  // ToDo: APU and IO
    }
    return 0;
}

void nes_bus_connect_cpu(NesBus *const bus) {
    // cpu->bus = ToDo - move ctx out of businterface struct
}

void nes_bus_cycle(NesBus *const bus) {
    // c6202_cycle(&bus->cpu);
    // todo - 3 ppu cycles for every cpu cycle
}