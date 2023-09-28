#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef enum {
    NES_CART_MIRROR_HORIZONTAL = 0,
    NES_CART_MIRROR_VERTICAL = 1,
} NesCartMirrorType;

typedef struct {
    size_t size;
    uint8_t *buf;
} _ubuf;

typedef struct {
    _ubuf prg_rom;
    _ubuf chr_rom;
    _ubuf prg_ram;
    _ubuf chr_ram;

    const void *_priv_intf;
    void *_priv_data;

    NesCartMirrorType mirror_type;

} NesCart;

void nes_cart_init(NesCart *, const char *filename);
void nes_cart_init_from_data(NesCart *, const uint8_t *buf, size_t buf_size);

void nes_cart_deinit(NesCart *);
void nes_cart_reset(NesCart *);

bool nes_cart_prg_write(NesCart *, uint16_t addr, uint8_t val);
bool nes_cart_prg_read(NesCart *, uint16_t addr, uint8_t *val_out);

bool nes_cart_ppu_write(NesCart *, uint16_t addr, uint8_t val);
bool nes_cart_ppu_read(NesCart *, uint16_t addr, uint8_t *val_out);
