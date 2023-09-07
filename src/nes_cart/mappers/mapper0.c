#include "../nes_cart_impl.h"

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

static bool mapper0_chr_write(NesCart *const mapper, uint16_t addr, uint8_t val) {
    return false;  // Todo
}

static bool mapper0_chr_read(NesCart *const mapper, uint16_t addr, uint8_t *const val_out) {
    return false;  // Todo
}

const MapperInterface mapper0 = {
    .prg_write = mapper0_prg_write,
    .prg_read = mapper0_prg_read,
    .chr_write = mapper0_chr_write,
    .chr_read = mapper0_chr_read,
};
