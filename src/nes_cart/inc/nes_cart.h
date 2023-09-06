#pragma once

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    uint8_t num_prg_banks;
    uint8_t num_chr_banks;
    uint8_t num_prg_ram_banks;

    const uint8_t *prg_rom;
    const uint8_t *chr_rom;

    uint8_t *prg_ram;

    const void *_priv_intf;
    void *_priv_data;

} NesCart;

void nes_cart_init(NesCart *, const char *filename);

void nes_cart_deinit(NesCart *);

bool nes_cart_cpu_write(NesCart *, uint16_t addr, uint8_t val);
bool nes_cart_cpu_read(NesCart *, uint16_t addr, uint8_t *val_out);
