
#include <c6502.h>
#include <nes_bus.h>

void nes_bus_write(NesBus *bus, uint16_t addr, uint8_t val) {}

uint8_t nes_bus_read(NesBus *bus, uint16_t addr) {
    if (addr < 0x2000) {
        return bus->ram[addr & 0x7FF];
    }
    if (addr < 0x4000) {
        // addr & 0x7; ToDo: PPU
        return 0;
    }
    return 0;
}
