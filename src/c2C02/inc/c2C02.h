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
} C2C02_loopy_register;

typedef struct C2C02 {
    void (*nmi_callback)(void *nmi_ctx);
    void *nmi_ctx;

    const C2C02BusInterface *bus;
    void *bus_ctx;

    // private
    int dots;
    int scanlines;

    bool address_latch;

    union __attribute__((__packed__)) {
        uint8_t u8;
        struct __attribute__((__packed__)) {
            uint8_t nametable_x : 1;
            uint8_t nametable_y : 1;

            // VRAM address increment per CPU read/write of PPUDATA
            // (0: add 1, going across; 1: add 32, going down)
            uint8_t vram_inc : 1;

            uint8_t : 4;  // Todo
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

    C2C02_loopy_register vram_address;
    C2C02_loopy_register temp_vram_address;
    uint8_t fine_x;

    uint8_t palette_ram[0x20];
} C2C02;

void c2C02_cycle(C2C02 *);

uint8_t c2C02_read_reg(C2C02 *, uint8_t addr);
void c2C02_write_reg(C2C02 *, uint8_t addr, uint8_t val);
