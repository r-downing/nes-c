#include <nes_cart.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mappers/impl.h"
#include "nes_cart_impl.h"

static const struct NesCartMapperInterface *const mapper_table[] = {
    [0] = &mapper_000,
    // Todo - mapper1 https://www.nesdev.org/wiki/MMC1
    [2] = &mapper_002,
    [3] = &mapper_003,
    // ...
    [66] = &mapper_066,
    // Todo - mapper4+ https://www.nesdev.org/wiki/Mapper
};

void nes_cart_deinit(NesCart *cart) {
    const typeof(cart->mapper->deinit) deinit = cart->mapper->deinit;
    if (deinit) {
        deinit(cart);
    }
    free((void *)cart->chr_rom.buf);
    free((void *)cart->prg_rom.buf);
    free(cart->chr_ram.buf);
    free(cart->prg_ram.buf);
}

void nes_cart_reset(NesCart *cart) {
    const typeof(cart->mapper->reset) reset = cart->mapper->reset;
    if (reset) {
        reset(cart);
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

    cart->prg_rom.banks = header.prg_rom_size_16KB;
    cart->prg_rom.buf = malloc(cart->prg_rom.size);
    read_func(arg, (void *)cart->prg_rom.buf, cart->prg_rom.size);

    if (0 != header.chr_rom_size_8KB) {
        cart->chr_rom.banks = header.chr_rom_size_8KB;
        cart->chr_rom.buf = malloc(cart->chr_rom.size);
        read_func(arg, (void *)cart->chr_rom.buf, cart->chr_rom.size);
    } else {
        cart->chr_ram.buf = malloc(0x2000);  // Todo - nes2.0 supports chr-ram size
    }

    if (header.flags6.has_pers_mem) {
        cart->prg_ram.banks = (header.prg_ram_size_8KB ? header.prg_ram_size_8KB : 1);
        cart->prg_ram.buf = malloc(cart->prg_ram.size);
    }

    const size_t mapper_num = (header.flags7.mapper_high << 4) | header.flags6.mapper_low;
    assert(mapper_num < (sizeof(mapper_table) / sizeof(mapper_table[0])));
    cart->mapper = mapper_table[mapper_num];
    assert(cart->mapper);
    assert(cart->mapper->cpu_read);
    assert(cart->mapper->cpu_write);
    assert(cart->mapper->ppu_read);
    assert(cart->mapper->ppu_write);

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
