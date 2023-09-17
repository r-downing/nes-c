#pragma once

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    uint8_t (*read)(void *bus_ctx, uint16_t addr);
    bool (*write)(void *bus_ctx, uint16_t addr, uint8_t val);
} C2C02BusInterface;

typedef union __attribute__((__packed__)) {
    uint16_t _u16;
    struct __attribute__((__packed__)) {
        uint16_t addr : 14;
        uint16_t : 2;
    };
    struct __attribute__((__packed__)) {
        uint8_t lo;
        uint8_t hi : 6;
        uint8_t : 2;
    };
    struct __attribute__((__packed__)) {
        uint16_t coarse_x : 5;
        uint16_t coarse_y : 5;
        uint16_t nametable_x : 1;
        uint16_t nametable_y : 1;
        uint16_t fine_y : 3;
        uint16_t : 1;
    };
    struct __attribute__((__packed__)) {
        uint16_t : 2;
        uint16_t coarse_x_attr : 3;  // high bits of coarse... maps to attr table
        uint16_t : 2;
        uint16_t coarse_y_attr : 3;
        uint16_t : 6;
    };

} C2C02_loopy_register;

typedef struct C2C02 {
    struct {
        void (*callback)(void *ctx);
        void *ctx;
    } nmi;

    const C2C02BusInterface *bus;
    void *bus_ctx;

    void (*draw_pixel)(void *draw_ctx, int x, int y, uint8_t r, uint8_t g, uint8_t b);
    void *draw_ctx;

    // private
    int dot;
    int scanline;

    bool address_latch;
    uint8_t data_read_buffer;

    // https://www.nesdev.org/wiki/PPU_registers
    union __attribute__((__packed__)) {
        uint8_t u8;
        struct __attribute__((__packed__)) {
            uint8_t nametable_x : 1;  // base 0x2000, + 0x400
            uint8_t nametable_y : 1;  // base 0x2000 + 0x800

            // VRAM address increment per CPU read/write of PPUDATA
            // (0: add 1, going across; 1: add 32, going down)
            uint8_t vram_inc : 1;

            uint8_t sprite_pattern_table : 1;      // for 8x8 sprites (0: $0000; 1: $1000; ignored in 8x16 mode)
            uint8_t background_pattern_table : 1;  // 1: (0: $0000; 1: $1000)
            uint8_t sprite_size : 1;               // 0: 8x8 pixels; 1: 8x16 pixels
            uint8_t : 1;                           // Todo - ppu master/slave select
            uint8_t nmi_at_vblank : 1;
        };
    } ctrl;

    union __attribute__((__packed__)) {
        uint8_t u8;
        struct __attribute__((__packed__)) {
            uint8_t stale_bus_contents : 5;
            uint8_t sprite_overflow : 1;
            uint8_t sprite_0_hit : 1;
            uint8_t vblank : 1;
        };
    } status;

    union __attribute__((__packed__)) {
        uint8_t u8;
        struct __attribute__((__packed__)) {
            uint8_t grayscale : 1;
            uint8_t background_left : 1;  // Show background in leftmost 8 pixels of screen

            uint8_t sprites_left : 1;  // Show sprites in leftmost 8 pixels of screen

            uint8_t show_background : 1;
            uint8_t show_sprites : 1;
            uint8_t emphasize_red : 1;
            uint8_t emphasize_green : 1;
            uint8_t emphasize_blue : 1;
        };
    } mask;

    C2C02_loopy_register vram_address;
    C2C02_loopy_register temp_vram_address;
    uint8_t fine_x;

    uint8_t palette_ram[0x20];

    struct {
        uint8_t data[0x100];
        uint8_t addr;
    } oam;

    uint32_t frames;
} C2C02;

void c2C02_cycle(C2C02 *);

uint8_t c2C02_read_reg(C2C02 *, uint8_t addr);
void c2C02_write_reg(C2C02 *, uint8_t addr, uint8_t val);
