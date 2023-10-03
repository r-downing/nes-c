// https://www.nesdev.org/wiki/UxROM

#include "../nes_cart_impl.h"

bool mapper_002_cpu_write(NesCart *const cart, uint16_t addr, uint8_t val) {
    if (addr < 0x8000) {
        return false;
    }
    cart->mapper_data = val;
    return true;
}

static bool mapper_002_cpu_read(NesCart *const cart, uint16_t addr, uint8_t *const val_out) {
    if (addr < 0x8000) {
        return false;
    }
    const uint8_t bank = (addr >= 0xC000) ? (cart->prg_rom.banks - 1) : ((uint8_t)cart->mapper_data);
    *val_out = cart->prg_rom.buf[(bank << 14) | (addr & ((1 << 14) - 1))];
    return true;
}

// same as NROM
bool mapper_000_ppu_write(NesCart *const cart, uint16_t addr, uint8_t val);
bool mapper_000_ppu_read(NesCart *const cart, uint16_t addr, uint8_t *const val_out);

const struct NesCartMapperInterface mapper_002 = {
    .name = "UxROM",
    .cpu_write = mapper_002_cpu_write,
    .cpu_read = mapper_002_cpu_read,
    .ppu_write = mapper_000_ppu_write,
    .ppu_read = mapper_000_ppu_read,
};
