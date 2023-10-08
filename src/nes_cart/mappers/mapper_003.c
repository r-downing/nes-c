// https://www.nesdev.org/wiki/INES_Mapper_003

#include "../nes_cart_impl.h"

bool mapper_003_cpu_write(NesCart *const cart, uint16_t addr, uint8_t val) {
    if (addr < 0x8000) {
        return false;
    }
    cart->mapper_data = val;
    return true;
}

bool mapper_003_cpu_read(NesCart *const cart, uint16_t addr, uint8_t *const val_out) {
    if (addr < 0x8000) {
        return false;
    }
    // no prg_ram
    *val_out = cart->prg_rom.buf[addr & (cart->prg_rom.size - 1)];
    return true;
}

bool mapper_000_ppu_write(NesCart *const cart, uint16_t addr, uint8_t val);

bool mapper_003_ppu_read(NesCart *const cart, uint16_t addr, uint8_t *const val_out) {
    const mapper_ppu_addr *const ppu_addr = (mapper_ppu_addr *)&addr;
    cart->VRAM_CE = ppu_addr->A13;
    if (addr >= 0x2000) {
        cart->VRAM_A10 = (cart->mirror_type == NES_CART_MIRROR_VERTICAL) ? ppu_addr->A10 : ppu_addr->A11;
        return false;
    }
    *val_out = cart->chr_rom.buf[((cart->mapper_data & 3) << 13) | (addr & 0x1FFF)];
    return true;
}

const struct NesCartMapperInterface mapper_003 = {
    .name = "CNROM",
    .cpu_write = mapper_003_cpu_write,
    .cpu_read = mapper_003_cpu_read,
    .ppu_write = mapper_000_ppu_write,
    .ppu_read = mapper_003_ppu_read,
};
