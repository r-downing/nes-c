
#include <c2C02.h>
#include <c6502.h>
#include <nes_bus.h>

// https://www.nesdev.org/wiki/CPU_memory_map

static void nes_bus_ppu_dma(NesBus *bus, const uint16_t page) {
    for (int i = 0; i < 0x100; i++) {
        c2C02_write_reg(&bus->ppu, 0x2004 & 0x7, nes_bus_cpu_read(bus, (page << 8) | i));
    }
}

bool nes_bus_cpu_write(NesBus *bus, uint16_t addr, uint8_t val) {
    if (nes_cart_cpu_write(&bus->cart, addr, val)) {
        return true;
    }
    if (addr < 0x2000) {
        bus->ram[addr & (sizeof(bus->ram) - 1)] = val;
        return true;
    }
    if (addr < 0x4000) {
        c2C02_write_reg(&bus->ppu, addr & 0x7, val);
        return true;
    }
    if (addr < 0x4020) {
        if (addr == 0x4014) {
            nes_bus_ppu_dma(bus, val);
            bus->cpu.current_op_cycles_remaining += 513 + (bus->cpu.total_cycles & 1);  // cpu skips cycles for DMA
        }
        if (addr == 0x4016) {
            nes_gamepad_write_reg(&bus->gamepad[0], val);
            nes_gamepad_write_reg(&bus->gamepad[1], val);
        }
        return true;  // ToDo: APU
    }
    return false;
}

uint8_t nes_bus_cpu_read(NesBus *bus, uint16_t addr) {
    uint8_t val;
    if (nes_cart_cpu_read(&bus->cart, addr, &val)) {
        return val;
    }
    if (addr < 0x2000) {
        return bus->ram[addr & (sizeof(bus->ram) - 1)];
    }
    if (addr < 0x4000) {
        return c2C02_read_reg(&bus->ppu, addr & 0x7);
    }
    if (addr < 0x4020) {
        if (addr == 0x4016) {
            return nes_gamepad_read_reg(&bus->gamepad[0]);
        } else if (addr == 0x4017) {
            return nes_gamepad_read_reg(&bus->gamepad[1]);
        }
        return 0;  // ToDo: APU
    }
    return 0;
}

static const C6502BusInterface bus_interface = {
    .read = (uint8_t(*)(void *, uint16_t))nes_bus_cpu_read,
    .write = (bool (*)(void *, uint16_t, uint8_t))nes_bus_cpu_write,
};

// https://www.nesdev.org/wiki/PPU_memory_map

bool nes_bus_ppu_write(NesBus *bus, uint16_t addr, uint8_t val) {
    if (nes_cart_ppu_write(&bus->cart, addr, val)) {
        return true;  // probably CHR-rom or CHR-ram
    }
    if (bus->cart.VRAM_CE) {
        bus->vram[bus->cart.VRAM_A10][addr & 0x3FF] = val;
        return true;
    }
    return false;  // Todo
}

uint8_t nes_bus_ppu_read(NesBus *bus, uint16_t addr) {
    uint8_t val;
    if (nes_cart_ppu_read(&bus->cart, addr, &val)) {
        return val;  // probably CHR-rom or CHR-ram
    }
    if (bus->cart.VRAM_CE) {
        return bus->vram[bus->cart.VRAM_A10][addr & 0x3FF];
    }
    return 0;  // should never get something < 0x3F00
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
    bus->ppu.nmi.callback = (void (*)(void *))c6502_nmi;
    bus->ppu.nmi.ctx = &bus->cpu;
    bus->cart.irq.callback = (void (*)(void *))c6502_irq;
    bus->cart.irq.arg = &bus->cpu;
    nes_bus_reset(bus);
}

void nes_bus_cycle(NesBus *const bus) {
    c2C02_cycle(&bus->ppu);

    if (3 == ++bus->cpu_subcycle_count) {
        bus->cpu_subcycle_count = 0;
        c6502_cycle(&bus->cpu);
    }
}

void nes_bus_reset(NesBus *const bus) {
    c6502_reset(&bus->cpu);
    // ToDo - reset other stuff
}
