#include "../nes_cart_impl.h"

static bool mapper0_prg_write(NesCart *const cart, uint16_t addr, uint8_t val) {
    if (addr < 0x6000) {
        return false;
    }
    if (addr < 0x8000) {
        if (NULL == cart->prg_ram) {
            // ToDo - warn accessing missing prg ram
            return false;
        }
        cart->prg_ram[addr & ((0x2000 * cart->num_prg_banks) - 1)] = val;
    }
    // ToDo - warn writing prg mem
    return false;
    // &mapper->prg_rom[addr & ((mapper->num_prg_banks > 1) ? 0x7FFF : 0x3FFF)];
}

static bool mapper0_prg_read(NesCart *const cart, uint16_t addr, uint8_t *const val_out) {
    if (addr < 0x6000) {
        return false;
    }
    if (addr < 0x8000) {
        if (NULL == cart->prg_ram) {
            // ToDo - warn accessing missing prg ram
            return false;
        }
        *val_out = cart->prg_ram[addr & ((0x2000 * cart->num_prg_banks) - 1)];
        return true;
    }

    *val_out = cart->prg_rom[addr & ((cart->num_prg_banks > 1) ? 0x7FFF : 0x3FFF)];
    return true;
}

static bool mapper0_ppu_write(NesCart *const cart, uint16_t addr, uint8_t val) {
    (void)cart;
    (void)addr;
    (void)val;
    // Todo - chr-ram
    return false;
}

static bool mapper0_ppu_read(NesCart *const cart, uint16_t addr, uint8_t *const val_out) {
    if (addr < 0x2000) {
        *val_out = cart->chr_rom[addr];
        return true;
    }
    // Todo - chr-ram
    return false;
}

const MapperInterface mapper0 = {
    .prg_write = mapper0_prg_write,
    .prg_read = mapper0_prg_read,
    .ppu_write = mapper0_ppu_write,
    .ppu_read = mapper0_ppu_read,
};
