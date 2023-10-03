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

bool mapper_003_ppu_write(NesCart *const /*cart*/, uint16_t /*addr*/, uint8_t /*val*/) {
    // Todo - confirm no CHR RAM for this mapper
    return false;
}

bool mapper_003_ppu_read(NesCart *const cart, uint16_t addr, uint8_t *const val_out) {
    if (addr >= 0x2000) {
        return false;
    }
    *val_out = cart->chr_rom.buf[((cart->mapper_data & 3) << 13) | addr];
    return true;
}

const struct NesCartMapperInterface mapper_003 = {
    .name = "CNROM",
    .cpu_write = mapper_003_cpu_write,
    .cpu_read = mapper_003_cpu_read,
    .ppu_write = mapper_003_ppu_write,
    .ppu_read = mapper_003_ppu_read,
};
