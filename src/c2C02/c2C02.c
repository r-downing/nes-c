#include <assert.h>
#include <c2C02.h>
#include <stddef.h>
#include <string.h>

#define ARRAY_LEN(arr) (sizeof(arr) / sizeof((arr)[0]))

static const uint8_t system_colors[][3] = {
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

inline static uint16_t mirror_palette_ram_addr(uint16_t addr) {
    addr &= 0x1F;
    // $3F10/$3F14/$3F18/$3F1C are mirrors of $3F00/$3F04/$3F08/$3F0C
    if ((0x10 == (addr & 0x13))) {
        addr &= (~0x10);
    }
    return addr;
}

inline static uint8_t bus_read(const C2C02 *const c, const uint16_t addr) {
    if (addr >= 0x3F00) {
        return c->palette_ram[mirror_palette_ram_addr(addr)];
    }
    return c->bus->read(c->bus_ctx, addr);
}

inline static void bus_write(C2C02 *const c, uint16_t addr, uint8_t val) {
    if (addr >= 0x3F00) {
        c->palette_ram[mirror_palette_ram_addr(addr)] = val;
        return;
    }
    c->bus->write(c->bus_ctx, addr, val);
}

uint8_t c2C02_read_reg(C2C02 *const c, const uint8_t addr) {
    // https://www.nesdev.org/wiki/PPU_registers
    switch (addr & 0x7) {
        // case 0x1: mask not readable
        case 0x2: {  // https://www.nesdev.org/wiki/PPU_registers#PPUSTATUS
            c->last_status_read_clocks = c->clocks;
            c->open_bus.val = (c->open_bus.val & (~0xE0)) | (c->status.u8 & 0xE0);
            c->open_bus.fresh[0] |= 0xE0;
            c->status.vblank = 0;
            c->pending_nmi = false;
            c->address_latch = false;
            return c->open_bus.val;
        }
        // case 0x3: OAM addr not readable
        case 0x4: {  // https://www.nesdev.org/wiki/PPU_registers#OAMDATA
            // Todo -Reading OAMDATA while the PPU is rendering will expose internal OAM accesses during sprite
            // evaluation and loading; https://www.nesdev.org/wiki/PPU_registers#OAMDATA

            // nes ppu-open-bus test says these bits should always be cleared when reading oam
            c->open_bus.val = c->oam.u8_arr[c->oam.addr] & 0b11100011;
            c->open_bus.fresh[0] = 0xFF;
            return c->open_bus.val;
        }
        // case 0x5: // scroll not readable
        // case 0x6: // addr not readable
        case 0x7: {  // https://www.nesdev.org/wiki/PPU_registers#PPUDATA
            uint8_t ret = c->data_read_buffer;
            c->data_read_buffer = bus_read(c, c->vram_address.addr);
            if (c->vram_address.addr >= 0x3F00) {
                ret = c->data_read_buffer;  // reading from palette, no delay
                c->open_bus.fresh[0] |= 0x3F;
                c->open_bus.val = (c->open_bus.val & (~0x3F)) | (ret & 0x3F);
                ret = c->open_bus.val;
            } else {
                c->open_bus.val = ret;
                c->open_bus.fresh[0] = 0xFF;
            }
            c->vram_address.addr += (c->ctrl.vram_inc ? 32 : 1);
            return ret;
        }
    }
    return c->open_bus.val;
}

static void write_scroll_reg(C2C02 *const c, const uint8_t val) {
    const struct __attribute__((__packed__)) {
        uint8_t fine : 3;
        uint8_t coarse : 5;
    } *const reg = (void *)&val;

    if (!c->address_latch) {
        c->fine_x = reg->fine;
        c->temp_vram_address.coarse_x = reg->coarse;
    } else {
        c->temp_vram_address.fine_y = reg->fine;
        c->temp_vram_address.coarse_y = reg->coarse;
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
    c->open_bus.val = val;  // https://www.nesdev.org/wiki/Open_bus_behavior#PPU_open_bus
    c->open_bus.fresh[0] = 0xFF;
    switch (addr & 0x7) {
        case 0x0: {  // https://www.nesdev.org/wiki/PPU_registers#PPUCTRL
            c->ctrl.u8 = val;
            c->temp_vram_address.nametable_x = c->ctrl.nametable_x;  // Todo - confirm this?
            c->temp_vram_address.nametable_y = c->ctrl.nametable_y;
            // Todo - trigger nmi if enabling during vblank. not sure of exact timing
            break;
        }
        case 0x1: {  // https://www.nesdev.org/wiki/PPU_registers#PPUMASK
            c->mask.u8 = val;
            break;
        }
        case 0x3: {  // https://www.nesdev.org/wiki/PPU_registers#OAMADDR
            c->oam.addr = val;
            break;
        }
        case 0x4: {  // https://www.nesdev.org/wiki/PPU_registers#OAMDATA
            // Todo - perform a glitchy increment of OAMADDR, bumping only the high 6 bits (i.e., it bumps the
            // [n] value in PPU sprite evaluation – it's plausible that it could bump the low bits instead depending on
            // the current status of sprite evaluation). This extends to DMA transfers via OAMDMA, since that uses
            // writes to $2004. For emulation purposes, it is probably best to completely ignore writes during
            // rendering. https://www.nesdev.org/wiki/PPU_registers#OAMDATA
            if (c->scanline >= 240) {
                c->oam.u8_arr[c->oam.addr++] = val;
            }
            break;
        }
        case 0x5: {  // https://www.nesdev.org/wiki/PPU_registers#PPUSCROLL
            write_scroll_reg(c, val);
            break;
        }
        case 0x6: {  // https://www.nesdev.org/wiki/PPU_registers#PPUADDR
            write_ppu_addr_reg(c, val);
            break;
        }
        case 0x7: {  // https://www.nesdev.org/wiki/PPU_registers#PPUDATA
            bus_write(c, c->vram_address.addr, val);
            c->vram_address.addr += (c->ctrl.vram_inc ? 32 : 1);
            break;
        }
    }
}

#if 0
static void render_palettes_on_bottom(const C2C02 *const c) {
    for (int i = 0; i < 32; i++) {
        const uint8_t *color = system_colors[bus_read(c, 0x3F00 + i)];
        c->draw_pixel(c->draw_ctx, i, 240, color[0], color[1], color[2]);
    }

    for (int i = 0; i < 32; i++) {
        const uint8_t *color = system_colors[c->palette_ram[i]];
        c->draw_pixel(c->draw_ctx, i + 64, 240, color[0], color[1], color[2]);
    }
}
#endif

typedef union __attribute__((__packed__)) {
    uint16_t u16;
    struct __attribute__((__packed__)) {
        uint16_t coarse_x : 5;
        uint16_t coarse_y : 5;
        uint16_t nametable_x : 1;  // +0x400
        uint16_t nametable_y : 1;  // +0x800
        uint16_t base : 4;         // 2 (0x2000)
    };
} nametable_addr_t;

inline static uint16_t get_nametable_address(uint8_t coarse_x, uint8_t coarse_y, uint8_t nametable_x,
                                             uint8_t nametable_y) {
    return (nametable_addr_t){
        .coarse_x = coarse_x,
        .coarse_y = coarse_y,
        .nametable_x = nametable_x,
        .nametable_y = nametable_y,
        .base = 2,
    }
        .u16;
}

inline static uint16_t get_attribute_table_address(uint16_t nametable_address) {
    const nametable_addr_t nt = {.u16 = nametable_address};

    // https://www.nesdev.org/wiki/PPU_scrolling#Tile_and_attribute_fetching
    return (union __attribute__((__packed__)) {
               uint16_t u16;
               struct __attribute__((__packed__)) {
                   uint16_t coarse_x_div_4 : 3;
                   uint16_t coarse_y_div_4 : 3;
                   uint16_t attribute_offset_15 : 4;  // const offset of 960
                   uint16_t nametable_x : 1;
                   uint16_t nametable_y : 1;
                   uint16_t nametable_base_2 : 4;  // const base of 0x2000
               };
           }){
        .coarse_x_div_4 = nt.coarse_x >> 2,
        .coarse_y_div_4 = nt.coarse_y >> 2,
        .attribute_offset_15 = 15,
        .nametable_x = nt.nametable_x,
        .nametable_y = nt.nametable_y,
        .nametable_base_2 = 2,
    }
        .u16;
}

inline static uint16_t get_pattern_table_address(uint8_t fine_y_offset, uint8_t bitplane, uint8_t nametable_value,
                                                 uint8_t pattern_table) {
    // https://www.nesdev.org/wiki/PPU_pattern_tables
    return (union __attribute__((__packed__)) {
               uint16_t u16;
               struct __attribute__((__packed__)) {
                   uint16_t fine_y_offset : 3;    // Fine Y offset, the row number within a tile
                   uint16_t bitplane : 1;         // 0 for low
                   uint16_t nametable_value : 8;  // 4b for column, 4b for row
                   uint16_t pattern_table : 1;    // from ctrl reg (for bg or sprite)
                   uint16_t : 3;                  // unused
               };
           }){
        .fine_y_offset = fine_y_offset,
        .bitplane = bitplane,
        .nametable_value = nametable_value,
        .pattern_table = pattern_table,
    }
        .u16;
}

inline static uint8_t get_palette_num(uint8_t attribute_value, uint8_t coarse_x, uint8_t coarse_y) {
    const int shift = ((coarse_y & 2) << 1) | (coarse_x & 2);
    return (attribute_value >> shift) & 0b11;
}

inline static void draw_pixel(const C2C02 *const c, const uint8_t x, const uint8_t y, uint8_t system_color) {
    if (c->draw_pixel) {
        if (c->mask.grayscale) {
            system_color &= 0xF0;
        }
        const uint8_t *const color = system_colors[system_color];
        c->draw_pixel(c->draw_ctx, x, y, color[0], color[1], color[2]);
    }
}

static uint8_t bit_reverse(uint8_t bits) {
    uint8_t ret = 0;
    for (int i = 0; i < 8; i++) {
        ret = (ret << 1) | (bits & 1);
        bits >>= 1;
    }
    return ret;
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

static void _inc_vert_v(C2C02 *const c) {
    if (c->mask.show_background || c->mask.show_sprites) {
        if (++c->vram_address.fine_y == 0) {
            // rolled over
            if (++c->vram_address.coarse_y == 30) {
                c->vram_address.coarse_y = 0;
                c->vram_address.nametable_y++;
            }
        }
    }
}

static void _inc_hori_v(C2C02 *const c) {
    if (c->mask.show_background || c->mask.show_sprites) {
        if (++c->vram_address.coarse_x == 0) {
            c->vram_address.nametable_x++;
        }
    }
}

static void _transfer_hori_v(C2C02 *const c) {
    if (c->mask.show_background || c->mask.show_sprites) {
        c->vram_address.nametable_x = c->temp_vram_address.nametable_x;
        c->vram_address.coarse_x = c->temp_vram_address.coarse_x;
    }
}

static void _transfer_vert_v(C2C02 *const c) {
    if (c->mask.show_background || c->mask.show_sprites) {
        c->vram_address.fine_y = c->temp_vram_address.fine_y;
        c->vram_address.nametable_y = c->temp_vram_address.nametable_y;
        c->vram_address.coarse_y = c->temp_vram_address.coarse_y;
    }
}

static void _load_shifters(C2C02 *const c) {
    // Todo - separate the actual shifting of the shifters... Should only happen w/ rendering enabled
    typeof(&c->bg_reg.shifters) const shifters = &c->bg_reg.shifters;
    typeof(&c->bg_reg.next) const next = &c->bg_reg.next;
    shifters->pattern.hi = shifters->pattern.hi | next->pattern.hi;
    shifters->pattern.lo = shifters->pattern.lo | next->pattern.lo;
    shifters->attr.hi = shifters->attr.hi | ((next->attr & 2) ? 0xFF : 0);
    shifters->attr.lo = shifters->attr.lo | ((next->attr & 1) ? 0xFF : 0);
}

// 240-260: idle except setting vblank
static void _non_render_scanlines(C2C02 *const c) {
    const int clocks_since_status = c->clocks - c->last_status_read_clocks;

    // https://www.nesdev.org/wiki/PPU_frame_timing#VBL_Flag_Timing
    // Reading one PPU clock before reads it as clear and never sets the flag or generates NMI for that frame. Reading
    // on the same PPU clock or one later reads it as set, clears it, and suppresses the NMI for that frame
    if (c->scanline == 241) {
        if (c->dot == 0) {
            if (clocks_since_status > 0) {
                c->status.vblank = 1;
            }
        } else if (c->dot == 1) {
            if (c->status.vblank && c->nmi.callback) {
                c->pending_nmi = true;
            }
        } else if (c->dot == 2) {
            if (c->pending_nmi && c->ctrl.nmi_at_vblank && c->nmi.callback) {
                c->nmi.callback(c->nmi.ctx);
            }
            c->pending_nmi = false;
        }
    }
}

static void _render_scanlines(C2C02 *const c) {
    // Todo - skip certain loads and shifts depending on if rendering enabled
    if ((c->scanline == 0) && (c->dot == 0) && c->mask.show_background && (c->frames & 1)) {
        c->dot++;  // skip (0,0) on bg-enabled + odd-frame
    }

    if ((c->dot > 0 && c->dot <= 256) || (c->dot > 320 && c->dot <= 336)) {
        c->bg_reg.shifters.attr.hi <<= 1;
        c->bg_reg.shifters.attr.lo <<= 1;
        c->bg_reg.shifters.pattern.hi <<= 1;
        c->bg_reg.shifters.pattern.lo <<= 1;

        switch (c->dot & 7) {
            case 1: {               // NT
                _load_shifters(c);  // shifters reloaded @ ticks 9, 17, 25... missing 257, but doesn't matter
                c->bg_reg.next.nt = bus_read(c, 0x2000 | (c->vram_address._u16 & 0xFFF));
                break;
            }
            case 3: {  // AT
                const uint8_t attr_byte =
                    bus_read(c, get_attribute_table_address(0x2000 | (c->vram_address._u16 & 0xFFF)));

                c->bg_reg.next.attr = get_palette_num(attr_byte, c->vram_address.coarse_x, c->vram_address.coarse_y);
                break;
            }
            case 5: {  // BG lsb
                c->bg_reg.next.pattern.lo =
                    bus_read(c, get_pattern_table_address(c->vram_address.fine_y, 0, c->bg_reg.next.nt,
                                                          c->ctrl.background_pattern_table));
                break;
            }
            case 7: {  // BG msb,  inc hori_v, draw last 8, load shifters
                c->bg_reg.next.pattern.hi =
                    bus_read(c, get_pattern_table_address(c->vram_address.fine_y, 1, c->bg_reg.next.nt,
                                                          c->ctrl.background_pattern_table));
                break;
            }
            case 0: {
                _inc_hori_v(c);
                break;
            }
        }  // switch
    }      // dot 1-256, 321-336

    if ((c->dot > 257) && (c->dot < 321)) {
        switch (c->dot & 0x7) {
            case 2:
                bus_read(c, 0x2000 | (c->vram_address._u16 & 0xFFF));
                break;
            case 4:
                bus_read(c, get_attribute_table_address(0x2000 | (c->vram_address._u16 & 0xFFF)));
                break;
        }
    }

    if (c->dot == 256) {
        _inc_vert_v(c);
    } else if (c->dot == 257) {
        _transfer_hori_v(c);
    } else if ((c->dot == 338) || (c->dot == 340)) {
        // Both of the bytes fetched here are the same nametable byte that will be fetched at the beginning of the next
        // scanline (tile 3, in other words). At least one mapper -- MMC5 -- is known to use this string of three
        // consecutive nametable fetches to clock a scanline counter.  https://www.nesdev.org/wiki/PPU_rendering
        c->bg_reg.next.nt = bus_read(c, 0x2000 | (c->vram_address._u16 & 0xFFF));  // unused NT fetches
    }

    if (c->scanline == -1) {
        if (c->dot == 0) {
            c->status.vblank = 0;
            c->status.sprite_0_hit = 0;
            c->status.sprite_overflow = 0;
        }
        if ((280 <= c->dot) && (c->dot <= 304)) {
            _transfer_vert_v(c);
        }
    }

    //// sprite eval - https://www.nesdev.org/wiki/PPU_sprite_evaluation
    if (c->dot == 64) {  // if((c->dot >= 1) && (c->dot <= 64)) {
        // Todo - byte-by-byte
        memset(c->oam2.sprites, 0xFF, sizeof(c->oam2.sprites));
    } else if (c->dot == 256) {
        // Todo - load sprites into secondary OAM piecewise
        c->sprite_reg.n = 0;
        c->sprite_reg.sprite0_present = false;
        for (size_t i = 0; i < ARRAY_LEN(c->oam.sprites); i++) {
            const _c2C02_sprite *const sprite = &c->oam.sprites[i];
            const int height = c->ctrl.sprite_size ? 16 : 8;
            if ((c->scanline >= sprite->y) && (c->scanline < (sprite->y + height))) {
                if (i == 0) {
                    c->sprite_reg.sprite0_present = true;
                }
                c->oam2.sprites[c->sprite_reg.n++] = *sprite;
                if (c->sprite_reg.n >= 8) {
                    break;  // Todo - buggy sprite overflow...
                }
            }
        }
    } else if ((c->dot >= 257) && (c->dot <= 320)) {
        if (((c->dot & 7) == 6) || ((c->dot & 7) == 0)) {
            const uint8_t oam2_idx = (c->dot - 257) >> 3;
            const _c2C02_sprite *const sprite = &c->oam2.sprites[oam2_idx];
            typeof(&c->sprite_reg.shifters[oam2_idx]) const shifter = &c->sprite_reg.shifters[oam2_idx];

            const uint8_t plane = ((c->dot & 7) == 0) ? 1 : 0;
            uint8_t *const dest = (plane) ? &shifter->pattern_hi : &shifter->pattern_lo;

            shifter->attr = sprite->attributes.u8;
            shifter->x = sprite->x;

            if (c->ctrl.sprite_size == 0) {
                const uint8_t fine_y =
                    sprite->attributes.flip_vertical ? (7 - (c->scanline - sprite->y)) : (c->scanline - sprite->y);
                *dest =
                    bus_read(c, get_pattern_table_address(fine_y, plane, sprite->tile, c->ctrl.sprite_pattern_table));
            } else {  // 8x16 sprites
                const uint8_t y =
                    sprite->attributes.flip_vertical ? (15 - (c->scanline - sprite->y)) : (c->scanline - sprite->y);
                const uint8_t coarse_y = y >> 3;
                const uint8_t fine_y = y & 7;

                *dest = bus_read(c, get_pattern_table_address(fine_y, plane, (sprite->_8x16.tile << 1) | coarse_y,
                                                              sprite->_8x16.bank));
            }

            if (!sprite->attributes.flip_horizontal) {
                *dest = bit_reverse(*dest);
            }
            if (oam2_idx >= c->sprite_reg.n) {
                *dest = 0;  // "Unused sprites are loaded with an all-transparent set of values."
                            // https://www.nesdev.org/wiki/PPU_rendering
            }
        }
    }

    if ((c->dot >= 1) && (c->dot <= 256) && (c->scanline >= 0)) {
        int palette_idx = 0;

        if (c->mask.show_background) {
            typeof(&c->bg_reg.shifters) const shifters = &c->bg_reg.shifters;
            const uint16_t mux = 0x8000 >> c->fine_x;
            const uint8_t p0 = (shifters->pattern.lo & mux) ? 1 : 0;
            const uint8_t p1 = (shifters->pattern.hi & mux) ? 1 : 0;
            const int val = ((p1 << 1) | p0);

            const uint8_t b0 = (shifters->attr.lo & mux) ? 1 : 0;
            const uint8_t b1 = (shifters->attr.hi & mux) ? 1 : 0;
            const uint8_t palette_num = (b1 << 1) | b0;

            if (c->mask.background_left || (c->dot > 8)) {
                palette_idx = (val) ? ((palette_num << 2) | val) : 0;
            }
        }

        if (c->mask.show_sprites) {
            // for each sprite on this scanline
            for (size_t i = 0; i < ARRAY_LEN(c->sprite_reg.shifters); i++) {
                typeof(&c->sprite_reg.shifters[i]) const shifter = &c->sprite_reg.shifters[i];
                if (shifter->x != 0) {  // inactive
                    continue;
                }
                const _c2C02_sprite_attr attrs = {.u8 = shifter->attr};
                const uint8_t p0 = shifter->pattern_lo & 1;
                const uint8_t p1 = shifter->pattern_hi & 1;
                const int val = ((p1 << 1) | p0);
                if ((i == 0) && val && palette_idx && c->sprite_reg.sprite0_present) {
                    // non-transparent sprite px on non-transparent bg pix for sprite0
                    c->status.sprite_0_hit = 1;  // Todo - technically sprite_0_hit only happens dot >=2
                }
                if (val) {
                    if (c->mask.sprites_left || (c->dot > 8)) {
                        if ((palette_idx == 0) || (attrs.priority == 0)) {
                            palette_idx = ((attrs.palette + 4) << 2) | val;
                        }
                    }
                    break;  // https://www.nesdev.org/wiki/PPU_sprite_priority
                }
            }

            // update sprite shifters
            for (size_t i = 0; i < ARRAY_LEN(c->sprite_reg.shifters); i++) {
                if (c->sprite_reg.shifters[i].x) {
                    c->sprite_reg.shifters[i].x--;
                } else {
                    c->sprite_reg.shifters[i].pattern_lo >>= 1;
                    c->sprite_reg.shifters[i].pattern_hi >>= 1;
                }
            }
        }

        const int cc = bus_read(c, 0x3F00 | palette_idx);
        draw_pixel(c, c->dot - 1, c->scanline, cc);
    }
}

void c2C02_cycle(C2C02 *const c) {
    // https://www.nesdev.org/w/images/default/4/4f/Ppu.svg
    if (c->scanline < 240) {
        _render_scanlines(c);
    } else {  // scanlines 240+ idle, except for setting vblank
        _non_render_scanlines(c);
    }

    // progress the scan position
    c->clocks++;
    c->dot++;
    if (c->dot > 340) {
        c->dot = 0;
        c->scanline++;
        if (c->scanline > 260) {
            c->scanline = -1;
            c->frames++;
            if ((c->frames & 7) == 0) {
                // bits that were refreshed in oldest cycle and not any more recent ones decay to 0
                const uint8_t decayed =
                    c->open_bus.fresh[4] & (~(c->open_bus.fresh[2] | c->open_bus.fresh[1] | c->open_bus.fresh[0]));
                c->open_bus.val &= ~decayed;
                c->open_bus.shift_out <<= 8;
            }
        }
    }
}
