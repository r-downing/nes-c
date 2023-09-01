#include <assert.h>
#include <nes_cart.h>
#include <stddef.h>
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

void nes_cart_deinit(NesCart *mapper) {
    const MapperInterface *const intf = (MapperInterface *)mapper->_priv_intf;
    intf->deinit(mapper);
    free((void *)mapper->chr_rom);
    free((void *)mapper->prg_rom);
    free(mapper->prg_ram);
}

void nes_cart_init(NesCart *const mapper /** ToDo file */) {
    memset(mapper, 0, sizeof(*mapper));

    RawCartridgeHeader header = {0};
    // ToDo - read header from file

    (void)memcmp(header_cookie, header.header_cookie, sizeof(header_cookie));  // ToDo - verify cookie

    if (header.flags6.has_trainer) {
        // ToDo - read past 512b trainer
    }
    mapper->num_prg_banks = header.prg_rom_size_16KB;
    mapper->num_chr_banks = header.chr_rom_size_8KB;
    mapper->prg_rom = malloc(header.prg_rom_size_16KB * 0x4000);
    // ToDo - read data from file
    mapper->chr_rom = malloc(header.chr_rom_size_8KB * 0x2000);
    // ToDo - read data from file

    if (header.flags6.has_pers_mem) {
        mapper->num_prg_ram_banks = (header.prg_ram_size_8KB ? header.prg_ram_size_8KB : 1);
        mapper->prg_ram = malloc(mapper->num_prg_ram_banks * 0x2000);
    }

    const uint8_t mapper_num = (header.flags7.mapper_high << 4) | header.flags6.mapper_low;
    mapper->_priv_intf = mapper_table[mapper_num];
    if (NULL != mapper_table[mapper_num]->init) {
        mapper_table[mapper_num]->init(mapper);
    }
}

bool nes_cart_cpu_write(NesCart *mapper, uint16_t addr, uint8_t val) {
    return ((MapperInterface *)mapper->_priv_intf)->prg_write(mapper, addr, val);
}

bool nes_cart_cpu_read(NesCart *mapper, uint16_t addr, uint8_t *val_out) {
    return ((MapperInterface *)mapper->_priv_intf)->prg_read(mapper, addr, val_out);
}
