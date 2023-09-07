
#include <c6502.h>
#include <nes_bus.h>

// https://www.nesdev.org/wiki/CPU_memory_map

bool nes_bus_write(NesBus *bus, uint16_t addr, uint8_t val) {
    if (nes_cart_prg_write(&bus->cart, addr, val)) {
        return true;
    }
    if (addr < 0x2000) {
        bus->ram[addr & 0x7FF] = val;
        return true;
    }
    if (addr < 0x4000) {
        c2C02_write_reg(&bus->ppu, addr & 0x7, val);
        return true;
    }
    if (addr < 0x4020) {
        return true;  // ToDo: APU and IO
    }
    return false;
}

uint8_t nes_bus_read(NesBus *bus, uint16_t addr) {
    uint8_t val;
    if (nes_cart_prg_read(&bus->cart, addr, &val)) {
        return val;
    }
    if (addr < 0x2000) {
        return bus->ram[addr & 0x7FF];
    }
    if (addr < 0x4000) {
        return c2C02_read_reg(&bus->ppu, addr & 0x7);
    }
    if (addr < 0x4020) {
        return 0;  // ToDo: APU and IO
    }
    return 0;
}

static const C6502BusInterface bus_interface = {
    .read = (uint8_t(*)(void *, uint16_t))nes_bus_read,
    .write = (bool (*)(void *, uint16_t, uint8_t))nes_bus_write,
};

void nes_bus_init(NesBus *const bus) {
    bus->cpu.bus_ctx = bus;
    bus->cpu.bus_interface = &bus_interface;
    nes_bus_reset(bus);
}

void nes_bus_cycle(NesBus *const bus) {
    // ToDo - ppu cycle
    if (3 == ++bus->cpu_subcycle_count) {
        bus->cpu_subcycle_count = 0;
        c6202_cycle(&bus->cpu);
    }
    c2C02_cycle(&bus->ppu);
}

void nes_bus_reset(NesBus *const bus) {
    c6502_reset(&bus->cpu);
    // ToDo - reset other stuff
}
