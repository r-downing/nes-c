#include <c2C02.h>

const uint8_t system_colors[][3] = {
    {0x80, 0x80, 0x80}, {0x00, 0x3D, 0xA6}, {0x00, 0x12, 0xB0}, {0x44, 0x00, 0x96}, {0xA1, 0x00, 0x5E},
    {0xC7, 0x00, 0x28}, {0xBA, 0x06, 0x00}, {0x8C, 0x17, 0x00}, {0x5C, 0x2F, 0x00}, {0x10, 0x45, 0x00},
    {0x05, 0x4A, 0x00}, {0x00, 0x47, 0x2E}, {0x00, 0x41, 0x66}, {0x00, 0x00, 0x00}, {0x05, 0x05, 0x05},
    {0x05, 0x05, 0x05}, {0xC7, 0xC7, 0xC7}, {0x00, 0x77, 0xFF}, {0x21, 0x55, 0xFF}, {0x82, 0x37, 0xFA},
    {0xEB, 0x2F, 0xB5}, {0xFF, 0x29, 0x50}, {0xFF, 0x22, 0x00}, {0xD6, 0x32, 0x00}, {0xC4, 0x62, 0x00},
    {0x35, 0x80, 0x00}, {0x05, 0x8F, 0x00}, {0x00, 0x8A, 0x55}, {0x00, 0x99, 0xCC}, {0x21, 0x21, 0x21},
    {0x09, 0x09, 0x09}, {0x09, 0x09, 0x09}, {0xFF, 0xFF, 0xFF}, {0x0F, 0xD7, 0xFF}, {0x69, 0xA2, 0xFF},
    {0xD4, 0x80, 0xFF}, {0xFF, 0x45, 0xF3}, {0xFF, 0x61, 0x8B}, {0xFF, 0x88, 0x33}, {0xFF, 0x9C, 0x12},
    {0xFA, 0xBC, 0x20}, {0x9F, 0xE3, 0x0E}, {0x2B, 0xF0, 0x35}, {0x0C, 0xF0, 0xA4}, {0x05, 0xFB, 0xFF},
    {0x5E, 0x5E, 0x5E}, {0x0D, 0x0D, 0x0D}, {0x0D, 0x0D, 0x0D}, {0xFF, 0xFF, 0xFF}, {0xA6, 0xFC, 0xFF},
    {0xB3, 0xEC, 0xFF}, {0xDA, 0xAB, 0xEB}, {0xFF, 0xA8, 0xF9}, {0xFF, 0xAB, 0xB3}, {0xFF, 0xD2, 0xB0},
    {0xFF, 0xEF, 0xA6}, {0xFF, 0xF7, 0x9C}, {0xD7, 0xE8, 0x95}, {0xA6, 0xED, 0xAF}, {0xA2, 0xF2, 0xDA},
    {0x99, 0xFF, 0xFC}, {0xDD, 0xDD, 0xDD}, {0x11, 0x11, 0x11}, {0x11, 0x11, 0x11}};

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
        case 0x7: {
            uint8_t ret = c->data_read_buffer;
            c->data_read_buffer = bus_read(c, c->vram_address.addr);
            if (c->vram_address.addr >= 0x3F00) {
                ret = c->data_read_buffer;  // reading from palette, no delay
            }
            c->vram_address.addr += (c->ctrl.vram_inc ? 32 : 1);
            return ret;
        }
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

static void render_palettes_on_bottom(C2C02 *const c) {
    for (int i = 0; i < 32; i++) {
        const uint8_t *color = system_colors[bus_read(c, 0x3F00 + i)];
        c->draw_pixel(c->draw_ctx, i, 240, color[0], color[1], color[2]);
    }

    for (int i = 0; i < 32; i++) {
        const uint8_t *color = system_colors[c->palette_ram[i]];
        c->draw_pixel(c->draw_ctx, i + 64, 240, color[0], color[1], color[2]);
    }
}

static void simple_render(C2C02 *const c) {
    render_palettes_on_bottom(c);

    const uint16_t nametable_base = 0x2000 + (c->ctrl.nametable_x ? 0x400 : 0) + (c->ctrl.nametable_y ? 0x800 : 0);

    for (int i = 0; i < 0x03c0; i++) {
        const uint8_t tile_idx = bus_read(c, nametable_base + i);
        const uint8_t tile_x = i & 0x1F;
        const uint8_t tile_y = i >> 5;

        const union __attribute__((__packed__)) {
            uint16_t u16;
            struct __attribute__((__packed__)) {
                uint16_t coarse_x_div_4 : 3;
                uint16_t coarse_y_div_4 : 3;
                uint16_t attribute_offset_15 : 4;
                uint16_t nametable_x : 1;
                uint16_t nametable_y : 1;
                uint16_t nametable_base_2 : 4;
            };
        } attribute_table_addr = {
            .coarse_x_div_4 = tile_x / 4,
            .coarse_y_div_4 = tile_y / 4,
            .attribute_offset_15 = 15,
            .nametable_x = c->ctrl.nametable_x,
            .nametable_y = c->ctrl.nametable_y,
            .nametable_base_2 = 2,
        };
        const int attr_byte = bus_read(c, attribute_table_addr.u16);

        int pswitch = ((tile_x % 4) / 2) | ((tile_y % 4) & 2);
        int pallet_idx = (attr_byte >> (2 * pswitch)) & 0b11;

        pallet_idx *= 4;

        for (int y = 0; y < 8; y++) {
            int lower = bus_read(c, (c->ctrl.background_pattern_table << 12) + tile_idx * 16 + y);
            int upper = bus_read(c, (c->ctrl.background_pattern_table << 12) + tile_idx * 16 + y + 8);
            for (int x = 7; x >= 0; x--) {
                const int val = ((1 & upper) << 1) | (1 & lower);
                upper >>= 1;
                lower >>= 1;
                const uint8_t *cc;  // = system_colors[val];
                if (val == 0) {
                    cc = system_colors[bus_read(c, 0x3F00)];
                } else {
                    cc = system_colors[bus_read(c, 0x3F00 + (pallet_idx + val))];
                }
                c->draw_pixel(c->draw_ctx, tile_x * 8 + x, tile_y * 8 + y, cc[0], cc[1], cc[2]);
            }
        }
    }
}

/* https://www.nesdev.org/wiki/PPU_nametables
Conceptually, the PPU does this 33 times for each scanline:

- Fetch a nametable entry from $2000-$2FBF.
- Fetch the corresponding attribute table entry from $23C0-$2FFF and increment the current VRAM address within the same
row.
- Fetch the low-order byte of an 8x1 pixel sliver of pattern table from $0000-$0FF7 or $1000-$1FF7.
- Fetch the high-order byte of this sliver from an address 8 bytes higher.
- Turn the attribute data and the pattern table data into palette indices, and combine them with data from sprite data
using priority.

It also does a fetch of a 34th (nametable, attribute, pattern) tuple that is never used, but some mappers rely on this
fetch for timing purposes.
*/
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
        if (0 == c->dot) {
            // idle
        } else if ((c->dot < 256) || (c->dot >= 321)) {
            switch (c->dot & 7) {
                case 1: {
                    // Todo - NT byte
                    break;
                }
                case 3: {
                    // Todo - At byte
                    break;
                }
                case 5: {
                    // Todo - low BG tile byte
                    break;
                }
                case 7: {
                    // Todo - hight tile byte
                    break;
                }
                case 0: {
                    // Inc hori(v)
                    if (c->mask.show_background || c->mask.show_sprites) {
                        if (++c->vram_address.coarse_x == 0) {
                            c->vram_address.nametable_x = ~c->vram_address.nametable_x;
                        }
                    }
                    break;
                }
            }  // switch
        } else if (c->dot == 256) {
            // Todo - inc vert_v
        } else if (c->dot == 257) {
            // Todo - hori_v = hori_t
        }
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
