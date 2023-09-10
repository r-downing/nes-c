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

#define BIT(x, bit) (((x) >> (bit)) & 1)
typedef union __attribute__((__packed__)) {
    uint8_t u8_arr[16];
    struct __attribute__((__packed__)) {
        uint8_t hi_plane[8];
        uint8_t lo_plane[8];
    };
} nes_chr_tile;

/* https://www.nesdev.org/wiki/PPU_nametables
Conceptually, the PPU does this 33 times for each scanline:

- Fetch a nametable entry from $2000-$2FBF.
- Fetch the corresponding attribute table entry from $23C0-$2FFF and increment the current VRAM address within the same
row.
- Fetch the low-order byte of an 8x1 pixel sliver of pattern table from $0000-$0FF7 or $1000-$1FF7.
- Fetch the high-order byte of this sliver from an address 8 bytes higher.
- Turn the attribute data and the pattern table data into palette indices, and combine them with data from sprite data
using priority.

It also does a fetch of a
34th (nametable, attribute, pattern) tuple that is never used, but some mappers rely on this fetch for timing purposes.
*/
#include <stdio.h>
static void simple_render(C2C02 *const c) {
    const uint16_t nametable_base_addr = 0x2000;
    // const uint16_t nametable_base_addr = 0x2000 + (c->ctrl.nametable_x ? 0x400 : 0) + (c->ctrl.nametable_y ? 0x800 : 0);

    const uint16_t pattern_base_addr = (c->ctrl.background_pattern_table ? 0x1000 : 0);

    printf("CTRL %x\n", c->ctrl.u8);
    for (int ti = 0; ti < 32 * 30; ti++) {
        const uint8_t nametable_entry = bus_read(c, (nametable_base_addr + ti));
        const uint8_t tile_x = ti % 32;
        const uint8_t tile_y = ti / 32;
        nes_chr_tile tile = {0};
        for (size_t ss = 0; ss < sizeof(tile); ss++) {
            tile.u8_arr[ss] = bus_read(c, ss + (pattern_base_addr + (nametable_entry * sizeof(tile))));
        }
        for (int y = 0; y < 8; y++) {
            for (int x = 0; x < 8; x++) {
                const uint8_t cc = (BIT(tile.hi_plane[y], 7 - x) << 1) | BIT(tile.lo_plane[y], 7 - x);
                c->draw_pixel(c->draw_ctx, x + (8 * tile_x), y + (8 * tile_y), (cc & 1) ? 255 : 0, (cc & 2) ? 255 : 0,
                              0);
            }
        }
    }
}

void c2C02_cycle(C2C02 *const c) {
    // https://www.nesdev.org/w/images/default/d/d1/Ntsc_timing.png

    if (c->scanline >= 240) {
        // idle, post-render
        if ((1 == c->dot) && (241 == c->scanline)) {
            c->status.vblank = 1;
            if (c->ctrl.nmi_at_vblank && c->nmi_callback) {
                c->nmi_callback(c->nmi_ctx);
                simple_render(c);
            }
        }
    } else {  // scanline -1 --> 239
        if ((1 == c->dot) && (-1 == c->scanline)) {
            c->status.vblank = 0;
            // Todo - sprite 0 overflow?
        }
        // Todo - if dot in orange ranges...
    }

    if ((339 == c->dot) && (-1 == c->scanline) && (c->mask.show_background || c->mask.show_sprites)) {
        c->dot++;  // odd frames skip last cycle of pre-render scanline when rendering enabled
    }

    c->dot++;

    if (c->dot > 340) {
        c->dot = 0;
        c->scanline++;
        if (c->scanline > 260) {
            c->scanline = -1;
            c->frames++;
        }
    }
}
