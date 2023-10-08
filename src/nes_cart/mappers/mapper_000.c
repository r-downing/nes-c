// https://www.nesdev.org/wiki/NROM

#include "../nes_cart_impl.h"

bool mapper_000_cpu_write(NesCart *const cart, uint16_t addr, uint8_t val) {
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

bool mapper_000_cpu_read(NesCart *const cart, uint16_t addr, uint8_t *const val_out) {
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

bool mapper_000_ppu_write(NesCart *const cart, uint16_t addr, uint8_t val) {
    const mapper_ppu_addr *const ppu_addr = (mapper_ppu_addr *)&addr;
    cart->VRAM_CE = ppu_addr->A13;
    if (addr >= 0x2000) {
        cart->VRAM_A10 = (cart->mirror_type == NES_CART_MIRROR_VERTICAL) ? ppu_addr->A10 : ppu_addr->A11;
        return false;
    }
    if (cart->chr_ram.buf) {
        cart->chr_ram.buf[addr] = val;
        return true;
    }
    return false;
}

bool mapper_000_ppu_read(NesCart *const cart, uint16_t addr, uint8_t *const val_out) {
    const mapper_ppu_addr *const ppu_addr = (mapper_ppu_addr *)&addr;
    cart->VRAM_CE = ppu_addr->A13;
    if (addr >= 0x2000) {
        cart->VRAM_A10 = (cart->mirror_type == NES_CART_MIRROR_VERTICAL) ? ppu_addr->A10 : ppu_addr->A11;
        return false;
    }
    if (cart->chr_ram.buf) {
        *val_out = cart->chr_ram.buf[addr];
        return true;
    }

    *val_out = cart->chr_rom.buf[addr];
    return true;
}

const struct NesCartMapperInterface mapper_000 = {
    .name = "NROM",
    .cpu_write = mapper_000_cpu_write,
    .cpu_read = mapper_000_cpu_read,
    .ppu_write = mapper_000_ppu_write,
    .ppu_read = mapper_000_ppu_read,
};
