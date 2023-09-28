
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
        if (addr == 0x4014) {
            nes_bus_ppu_dma(bus, val);
            bus->cpu.current_op_cycles_remaining += 513 + (bus->cpu.total_cycles & 1);  // cpu skips cycles for DMA
        }
        if (addr == 0x4016) {
            nes_gamepad_write_reg(&bus->gamepad, val);
            // Todo - write to second gamepad
        }
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
        if (addr == 0x4016) {
            return nes_gamepad_read_reg(&bus->gamepad);
        }
        // Todo - 0x4017 read from second gamepad
        return 0;  // ToDo: APU and IO
    }
    return 0;
}

static const C6502BusInterface bus_interface = {
    .read = (uint8_t(*)(void *, uint16_t))nes_bus_cpu_read,
    .write = (bool (*)(void *, uint16_t, uint8_t))nes_bus_cpu_write,
};

// https://www.nesdev.org/wiki/PPU_memory_map

inline static uint8_t *vram_mirroring(NesBus *const bus, uint16_t addr) {
    const union __attribute__((__packed__)) {
        uint16_t u16;
        struct __attribute__((__packed__)) {
            uint16_t addr : 10;
            uint16_t v : 1;
            uint16_t h : 1;
            uint16_t : 4;
        };
    } mirror = {.u16 = addr};

    if (NES_CART_MIRROR_HORIZONTAL == bus->cart.mirror_type) {
        return &bus->vram[mirror.h][mirror.addr];
    } else {
        return &bus->vram[mirror.v][mirror.addr];
    }
}

bool nes_bus_ppu_write(NesBus *bus, uint16_t addr, uint8_t val) {
    if (nes_cart_ppu_write(&bus->cart, addr, val)) {
        return true;  // probably CHR-rom or CHR-ram
    }
    if ((0x2000 <= addr) && (addr < 0x3000)) {
        *vram_mirroring(bus, addr) = val;
        return true;
    }
    return false;  // Todo
}

uint8_t nes_bus_ppu_read(NesBus *bus, uint16_t addr) {
    uint8_t val;
    if (nes_cart_ppu_read(&bus->cart, addr, &val)) {
        return val;  // probably CHR-rom or CHR-ram
    }
    if ((0x2000 <= addr) && (addr < 0x3F00)) {
        return *vram_mirroring(bus, addr);
    }
    return 0;  // should never get something < 0x3000
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
