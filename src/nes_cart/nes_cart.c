#include <nes_cart.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "nes_cart_impl.h"

extern const MapperInterface mapper0;

static const MapperInterface *const mapper_table[] = {
    &mapper0,
};

void nes_cart_deinit(NesCart *cart) {
    const MapperInterface *const mapper = (MapperInterface *)cart->_priv_intf;
    if (mapper->deinit) {
        mapper->deinit(cart);
    }
    free(cart->chr_rom.buf);
    free(cart->prg_rom.buf);
    free(cart->chr_ram.buf);
    free(cart->prg_ram.buf);
}

void nes_cart_reset(NesCart *cart) {
    const MapperInterface *const mapper = (MapperInterface *)cart->_priv_intf;
    if (mapper->reset) {
        mapper->reset(cart);
    }
}

static size_t read_from_file(void *arg, void *const dest, size_t size) {
    return fread(dest, size, 1, (FILE *)arg);
}

typedef struct {
    const uint8_t *buf;
    size_t size;
    size_t offset;
} buf_ctx;

static size_t read_from_buf(void *arg, void *const dest, size_t size) {
    buf_ctx *const ctx = (buf_ctx *)arg;
    const size_t remaining = ctx->size - ctx->offset;
    const size_t read_size = (size <= remaining) ? size : remaining;
    memcpy(dest, &ctx->buf[ctx->offset], read_size);
    ctx->offset += read_size;
    return read_size;
}

static void _nes_cart_init_from_src(NesCart *const cart, size_t (*read_func)(void *arg, void *dest, size_t size),
                                    void *arg) {
    memset(cart, 0, sizeof(*cart));

    RawCartridgeHeader header = {0};
    read_func(arg, &header, sizeof(header));

    if (0 != memcmp(header_cookie, header.header_cookie, sizeof(header_cookie))) {
        return;  // Todo - better return codes
    }

    if (header.flags6.has_trainer) {
        assert(0);  // Todo - support cartridge trainers
        uint8_t trainer[512];
        read_func(arg, trainer, sizeof(trainer));
    }

    cart->prg_rom.size = header.prg_rom_size_16KB * 0x4000;
    cart->prg_rom.buf = malloc(cart->prg_rom.size);
    read_func(arg, cart->prg_rom.buf, cart->prg_rom.size);

    if (0 != header.chr_rom_size_8KB) {
        cart->chr_rom.size = header.chr_rom_size_8KB * 0x2000;
        cart->chr_rom.buf = malloc(cart->chr_rom.size);
        read_func(arg, cart->chr_rom.buf, cart->chr_rom.size);
    } else {
        cart->chr_ram.buf = malloc(0x2000);  // Todo - nes2.0 supports chr-ram size
    }

    if (header.flags6.has_pers_mem) {
        cart->prg_ram.size = (header.prg_ram_size_8KB ? header.prg_ram_size_8KB : 1) * 0x2000;
        cart->prg_ram.buf = malloc(cart->prg_ram.size);
    }

    const size_t mapper_num = (header.flags7.mapper_high << 4) | header.flags6.mapper_low;
    assert(mapper_num < (sizeof(mapper_table) / sizeof(mapper_table[0])));
    cart->_priv_intf = mapper_table[mapper_num];
    if (NULL != mapper_table[mapper_num]->init) {
        mapper_table[mapper_num]->init(cart);
    }
    cart->mirror_type = header.flags6.mirroring;
}

void nes_cart_init(NesCart *const cart, const char *const filename) {
    FILE *const f = fopen(filename, "rb");
    assert(f);
    _nes_cart_init_from_src(cart, read_from_file, f);
    fclose(f);
}

void nes_cart_init_from_data(NesCart *const cart, const uint8_t *const buf, size_t buf_size) {
    _nes_cart_init_from_src(cart, read_from_buf, &(buf_ctx){.buf = buf, .offset = 0, .size = buf_size});
}

// Todo - inline these nes-cart functions
bool nes_cart_prg_write(NesCart *cart, uint16_t addr, uint8_t val) {
    return ((MapperInterface *)cart->_priv_intf)->prg_write(cart, addr, val);
}

bool nes_cart_prg_read(NesCart *cart, uint16_t addr, uint8_t *val_out) {
    return ((MapperInterface *)cart->_priv_intf)->prg_read(cart, addr, val_out);
}

bool nes_cart_ppu_write(NesCart *const cart, uint16_t addr, uint8_t val) {
    return ((MapperInterface *)cart->_priv_intf)->ppu_write(cart, addr, val);
}

bool nes_cart_ppu_read(NesCart *const cart, uint16_t addr, uint8_t *val_out) {
    return ((MapperInterface *)cart->_priv_intf)->ppu_read(cart, addr, val_out);
}
