// https://www.nesdev.org/wiki/NROM

#include "../nes_cart_impl.h"

bool mapper0_cpu_write(NesCart *const cart, uint16_t addr, uint8_t val) {
    if (addr < 0x6000) {
        return false;
    }
    if (addr < 0x8000) {
        if (NULL == cart->prg_ram.buf) {
            return false;  // ToDo - warn accessing missing prg ram
        }
        cart->prg_ram.buf[addr & (cart->prg_ram.size - 1)] = val;
    }
    return false;
}

bool mapper0_cpu_read(NesCart *const cart, uint16_t addr, uint8_t *const val_out) {
    if (addr < 0x6000) {
        return false;
    }
    if (addr < 0x8000) {
        if (NULL == cart->prg_ram.buf) {
            return false;  // ToDo - warn accessing missing prg ram
        }
        *val_out = cart->prg_ram.buf[addr & (cart->prg_ram.size - 1)];
        return true;
    }

    *val_out = cart->prg_rom.buf[addr & (cart->prg_rom.size - 1)];
    return true;
}

bool mapper0_ppu_write(NesCart *const cart, uint16_t addr, uint8_t val) {
    if (addr >= 0x2000) {
        return false;
    }
    if (cart->chr_ram.buf) {
        cart->chr_ram.buf[addr] = val;
        return true;
    }
    return false;
}

bool mapper0_ppu_read(NesCart *const cart, uint16_t addr, uint8_t *const val_out) {
    if (addr >= 0x2000) {
        return false;
    }
    if (cart->chr_ram.buf) {
        *val_out = cart->chr_ram.buf[addr];
        return true;
    }

    *val_out = cart->chr_rom.buf[addr];
    return true;
}

const struct NesCartMapperInterface mapper0 = {
    .name = "NROM",
    .cpu_write = mapper0_cpu_write,
    .cpu_read = mapper0_cpu_read,
    .ppu_write = mapper0_ppu_write,
    .ppu_read = mapper0_ppu_read,
};
