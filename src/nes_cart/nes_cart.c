#include <assert.h>
#include <nes_cart.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "nes_cart_impl.h"

///// mapper 0
static bool mapper0_prg_write(NesCart *const mapper, uint16_t addr, uint8_t val) {
    if (addr < 0x6000) {
        return false;
    }
    if (addr < 0x8000) {
        if (NULL == mapper->prg_ram) {
            // ToDo - warn accessing missing prg ram
            return false;
        }
        mapper->prg_ram[addr & ((0x2000 * mapper->num_prg_banks) - 1)] = val;
    }
    // ToDo - warn writing prg mem
    return false;
    // &mapper->prg_rom[addr & ((mapper->num_prg_banks > 1) ? 0x7FFF : 0x3FFF)];
}

static bool mapper0_prg_read(NesCart *const mapper, uint16_t addr, uint8_t *const val_out) {
    if (addr < 0x6000) {
        return false;
    }
    if (addr < 0x8000) {
        if (NULL == mapper->prg_ram) {
            // ToDo - warn accessing missing prg ram
            return false;
        }
        *val_out = mapper->prg_ram[addr & ((0x2000 * mapper->num_prg_banks) - 1)];
        return true;
    }

    *val_out = mapper->prg_rom[addr & ((mapper->num_prg_banks > 1) ? 0x7FFF : 0x3FFF)];
    return true;
}

static const MapperInterface mapper0 = {
    .prg_write = mapper0_prg_write,
    .prg_read = mapper0_prg_read,
};

////// mapper 0

static const MapperInterface *const mapper_table[] = {
    &mapper0,
};

void nes_cart_deinit(NesCart *cart) {
    const MapperInterface *const mapper = (MapperInterface *)cart->_priv_intf;
    if (mapper->deinit) {
        mapper->deinit(cart);
    }
    free((void *)cart->chr_rom);
    free((void *)cart->prg_rom);
    free(cart->prg_ram);
}

void nes_cart_init(NesCart *const cart, const char *const filename) {
    memset(cart, 0, sizeof(*cart));

    FILE *const f = fopen(filename, "rb");
    if (NULL == f) {
        return;
    }

    RawCartridgeHeader header = {0};
    fread(&header, sizeof(header), 1, f);

    if (0 != memcmp(header_cookie, header.header_cookie, sizeof(header_cookie))) {
        fclose(f);
        return;
    }

    if (header.flags6.has_trainer) {
        fseek(f, 512, SEEK_CUR);
    }
    cart->num_prg_banks = header.prg_rom_size_16KB;
    cart->num_chr_banks = header.chr_rom_size_8KB;
    cart->prg_rom = malloc(header.prg_rom_size_16KB * 0x4000);
    fread((void *)cart->prg_rom, (header.prg_rom_size_16KB * 0x4000), 1, f);
    cart->chr_rom = malloc(header.chr_rom_size_8KB * 0x2000);
    fread((void *)cart->chr_rom, (header.chr_rom_size_8KB * 0x2000), 1, f);
    fclose(f);

    if (header.flags6.has_pers_mem) {
        cart->num_prg_ram_banks = (header.prg_ram_size_8KB ? header.prg_ram_size_8KB : 1);
        cart->prg_ram = malloc(cart->num_prg_ram_banks * 0x2000);
    }

    const uint8_t mapper_num = (header.flags7.mapper_high << 4) | header.flags6.mapper_low;
    cart->_priv_intf = mapper_table[mapper_num];
    if (NULL != mapper_table[mapper_num]->init) {
        mapper_table[mapper_num]->init(cart);
    }
}

bool nes_cart_cpu_write(NesCart *mapper, uint16_t addr, uint8_t val) {
    return ((MapperInterface *)mapper->_priv_intf)->prg_write(mapper, addr, val);
}

bool nes_cart_cpu_read(NesCart *mapper, uint16_t addr, uint8_t *val_out) {
    return ((MapperInterface *)mapper->_priv_intf)->prg_read(mapper, addr, val_out);
}
