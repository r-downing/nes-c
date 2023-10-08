#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct NesCart {
    struct {
        void (*callback)(void *arg);
        void *arg;
    } irq;

    struct {
        union {
            size_t size;
            struct __attribute__((__packed__)) {
                size_t : 14;
                size_t banks : 8;  // 16K
                // ...
            };
        };
        const uint8_t *buf;
    } prg_rom;

    struct {
        union {
            size_t size;
            struct __attribute__((__packed__)) {
                size_t : 13;
                size_t banks : 8;  // 8K
                // ...
            };
        };
        uint8_t *buf;
    } chr_rom, prg_ram, chr_ram;

    // cart output signals for routing to console internal vram. will be updated after an attempted
    // nes_cart_ppu_read() or nes_cart_ppu_write()
    struct {
        // https://www.nesdev.org/wiki/Cartridge_connector#Signal_descriptions

        // This signal is used as an input to enable the internal 2k of VRAM (used for name table and attribute tables
        // typically, but could be made for another use). This signal is usually directly connected with PPU /A13, but
        // carts using their own RAM for name table and attribute tables will have their own logic implemented.
        // 1 for enable
        uint8_t VRAM_CE : 1;

        // This is the 1k bank selection input for internal VRAM. This is used to control how the name tables are
        // banked; in other words, this selects nametable mirroring. Connect to PPU A10 for vertical mirroring or PPU
        // A11 for horizontal mirroring. Connect it to a software operated latch to allow bank switching of two
        // separate name tables in single-screen mirroring (as in AxROM). Many mappers have software operated mirroring
        // selection: they multiplex PPU A10 and PPU A11 into this pin, selected by a latch.
        uint8_t VRAM_A10 : 1;
    };

    enum {
        NES_CART_MIRROR_HORIZONTAL = 0,  // VRAM_A10 connects to PPU_A11
        NES_CART_MIRROR_VERTICAL = 1,    // VRAM_A10 connects to PPU_A10
    } mirror_type;

    uint8_t (*ext_vram)[0x800];

    // https://www.nesdev.org/wiki/Mapper
    const struct NesCartMapperInterface {
        const char *name;
        bool (*cpu_write)(struct NesCart *, uint16_t addr, uint8_t val);
        bool (*cpu_read)(struct NesCart *, uint16_t addr, uint8_t *val_out);
        bool (*ppu_write)(struct NesCart *, uint16_t addr, uint8_t val);
        bool (*ppu_read)(struct NesCart *, uint16_t addr, uint8_t *val_out);
        void (*init)(struct NesCart *);
        void (*deinit)(struct NesCart *);
        void (*reset)(struct NesCart *);
    } *mapper;
    uintptr_t mapper_data;

} NesCart;

void nes_cart_init(NesCart *, const char *filename);
void nes_cart_init_from_data(NesCart *, const uint8_t *buf, size_t buf_size);

void nes_cart_deinit(NesCart *);
void nes_cart_reset(NesCart *);

static inline bool nes_cart_cpu_write(NesCart *const cart, uint16_t addr, uint8_t val) {
    return cart->mapper->cpu_write(cart, addr, val);
}

static inline bool nes_cart_cpu_read(NesCart *const cart, uint16_t addr, uint8_t *val_out) {
    return cart->mapper->cpu_read(cart, addr, val_out);
}

static inline bool nes_cart_ppu_write(NesCart *const cart, uint16_t addr, uint8_t val) {
    return cart->mapper->ppu_write(cart, addr, val);
}

static inline bool nes_cart_ppu_read(NesCart *const cart, uint16_t addr, uint8_t *val_out) {
    return cart->mapper->ppu_read(cart, addr, val_out);
}
