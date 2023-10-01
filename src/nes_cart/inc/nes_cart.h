#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef enum {
    NES_CART_MIRROR_HORIZONTAL = 0,
    NES_CART_MIRROR_VERTICAL = 1,
} NesCartMirrorType;

typedef struct NesCart {
    struct {
        size_t size;
        const uint8_t *buf;
    } prg_rom, chr_rom;

    struct {
        size_t size;
        uint8_t *buf;
    } prg_ram, chr_ram;

    NesCartMirrorType mirror_type;

    // https://www.nesdev.org/wiki/Mapper
    const struct NesCartMapperInterface {
        bool (*cpu_write)(struct NesCart *, uint16_t addr, uint8_t val);
        bool (*cpu_read)(struct NesCart *, uint16_t addr, uint8_t *val_out);
        bool (*ppu_write)(struct NesCart *, uint16_t addr, uint8_t val);
        bool (*ppu_read)(struct NesCart *, uint16_t addr, uint8_t *val_out);
        void (*init)(struct NesCart *);
        void (*deinit)(struct NesCart *);
        void (*reset)(struct NesCart *);
    } *mapper;
    void *mapper_data;

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
