
#include <c2C02.h>
#include <c6502.h>
#include <nes_bus.h>

// https://www.nesdev.org/wiki/CPU_memory_map

bool nes_bus_cpu_write(NesBus *bus, uint16_t addr, uint8_t val) {
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

uint8_t nes_bus_cpu_read(NesBus *bus, uint16_t addr) {
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
    .read = (uint8_t(*)(void *, uint16_t))nes_bus_cpu_read,
    .write = (bool (*)(void *, uint16_t, uint8_t))nes_bus_cpu_write,
};

bool nes_bus_ppu_write(NesBus *bus, uint16_t addr, uint8_t val) {
    (void)bus;
    (void)addr;
    (void)val;
    return false;  // Todo
}

uint8_t nes_bus_ppu_read(NesBus *bus, uint16_t addr) {
    (void)bus;
    (void)addr;
    return 0;  // Todo
}
static const C2C02BusInterface ppu_bus_interface = {
    .read = (uint8_t(*)(void *, uint16_t))nes_bus_ppu_read,
    .write = (bool (*)(void *, uint16_t, uint8_t))nes_bus_ppu_write,
};

void nes_bus_init(NesBus *const bus) {
    bus->cpu.bus_ctx = bus;
    bus->cpu.bus_interface = &bus_interface;
    bus->ppu.bus_ctx = bus;
    bus->ppu.bus = &ppu_bus_interface;
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
