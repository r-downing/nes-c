// https://www.nesdev.org/wiki/GxROM

#include "../nes_cart_impl.h"

// Bank select ($8000-$FFFF)
typedef union {
    uintptr_t uintp;
    struct __attribute__((__packed__)) {
        uint8_t chr_rom_bank : 2;  // Select 8 KB CHR ROM bank for PPU $0000-$1FFF
        uint8_t : 2;
        uint8_t prg_rom_bank : 2;  // Select 32 KB PRG ROM bank for CPU $8000-$FFFF
        uint8_t : 2;
    };
} mapper_066_reg;

bool mapper_066_cpu_write(NesCart *const cart, uint16_t addr, uint8_t val) {
    if (addr < 0x8000) {
        return false;
    }
    cart->mapper_data = val;
    return true;
}

bool mapper_066_cpu_read(NesCart *const cart, uint16_t addr, uint8_t *const val_out) {
    if (addr < 0x8000) {
        return false;
    }
    const mapper_066_reg *const reg = (mapper_066_reg *)(&cart->mapper_data);
    if (reg->prg_rom_bank >= cart->prg_rom.banks) {
        return false;
    }
    *val_out = cart->prg_rom.buf[(reg->prg_rom_bank << 15) | (addr & 0x7FFF)];
    return true;
}

bool mapper_000_ppu_write(NesCart *const cart, uint16_t addr, uint8_t val);

bool mapper_066_ppu_read(NesCart *const cart, uint16_t addr, uint8_t *const val_out) {
    if (addr >= 0x2000) {  // route to console vram
        const mapper_ppu_addr *const ppu_addr = (mapper_ppu_addr *)&addr;
        cart->VRAM_CE = 1;  // ~ppu_addr->A13
        cart->VRAM_A10 = (cart->mirror_type == NES_CART_MIRROR_VERTICAL) ? ppu_addr->A10 : ppu_addr->A11;
        return false;
    }
    const mapper_066_reg *const reg = (mapper_066_reg *)(&cart->mapper_data);
    if (reg->chr_rom_bank >= cart->chr_rom.banks) {
        return false;
    }
    *val_out = cart->chr_rom.buf[(reg->chr_rom_bank << 13) | (addr & 0x1FFF)];
    return true;
}

const struct NesCartMapperInterface mapper_066 = {
    .name = "GxROM",
    .cpu_write = mapper_066_cpu_write,
    .cpu_read = mapper_066_cpu_read,
    .ppu_write = mapper_000_ppu_write,
    .ppu_read = mapper_066_ppu_read,
};
