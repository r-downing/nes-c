#include <c2C02.h>

static uint8_t *mirror_palette_ram_addr(C2C02 *const c, uint16_t addr) {
    addr &= 0x1F;
    // $3F10/$3F14/$3F18/$3F1C are mirrors of $3F00/$3F04/$3F08/$3F0C
    if ((0x10 == (addr & 0x13))) {
        addr &= (~0x10);
    }
    return &c->palette_ram[addr];
}

static uint8_t bus_read(C2C02 *const c, uint16_t addr) {
    if (addr >= 0x3F00) {
        return *mirror_palette_ram_addr(c, addr);
    }
    return c->bus->read(c->bus_ctx, addr);
}

static void bus_write(C2C02 *const c, uint16_t addr, uint8_t val) {
    if (addr >= 0x3F00) {
        *mirror_palette_ram_addr(c, addr) = val;
        return;
    }
    c->bus->write(c->bus_ctx, addr, val);
}

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
    switch (addr & 0x7) {
        case 0x2: {  // status
            const uint8_t ret = (c->status.u8 & 0xE0) | (0 /* Todo - ppu stale data */);
            c->status.vblank = 0;
            c->address_latch = false;
            return ret;
        }
        case 0x3:
            return 0;  // Todo - OAM addr
        case 0x4:
            return 0;  // Todo - OAM data
        case 0x7:
            (void)bus_read;  // Todo cleanup
            return 0;        // Todo - PPU Data
        default:
            return 0;
    }
    return 0;  // Todo
}

static void write_scroll_reg(C2C02 *const c, const uint8_t val) {
    const union __attribute__((__packed__)) {
        uint8_t u8;
        struct __attribute__((__packed__)) {
            uint8_t fine : 3;
            uint8_t coarse : 5;
        };
    } reg = {.u8 = val};

    if (!c->address_latch) {
        c->fine_x = reg.fine;
        c->temp_vram_address.coarse_x = reg.coarse;
    } else {
        c->temp_vram_address.fine_y = reg.fine;
        c->temp_vram_address.coarse_y = reg.coarse;
    }
    c->address_latch = !c->address_latch;
}

static void write_ppu_addr_reg(C2C02 *const c, const uint8_t val) {
    if (!c->address_latch) {
        c->temp_vram_address._u16 = c->temp_vram_address.lo;  // clear extra high bits
        c->temp_vram_address.hi = val;
    } else {
        c->temp_vram_address.lo = val;
        c->vram_address = c->temp_vram_address;
    }
    c->address_latch = !c->address_latch;
}

void c2C02_write_reg(C2C02 *const c, const uint8_t addr, const uint8_t val) {
    switch (addr & 0x7) {
        case 0x0: {  // control
            c->ctrl.u8 = val;
            break;
        }
        case 0x1: {
            c->mask.u8 = val;
            break;
        }
        case 0x3: {
            break;  // Todo OAM addr
        }
        case 0x4: {
            break;  // Todo OAM data
        }
        case 0x5: {
            write_scroll_reg(c, val);
            break;
        }
        case 0x6: {
            write_ppu_addr_reg(c, val);
            break;
        }
        case 0x7: {
            bus_write(c, c->vram_address.addr, val);
            c->vram_address.addr += (c->ctrl.vram_inc ? 32 : 1);
            break;
        }
    }
}

void c2C02_dma() {
    // Todo
}