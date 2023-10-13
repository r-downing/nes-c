// https://www.nesdev.org/wiki/MMC3

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "../nes_cart_impl.h"

typedef union __attribute__((__packed__)) {
    uint32_t u32;
    struct __attribute__((__packed__)) {
        uint32_t offset : 10;
        uint32_t bank : 22;
    } _1k;
    struct __attribute__((__packed__)) {
        uint32_t offset : 11;
        uint32_t bank : 21;
    } _2k;
    struct __attribute__((__packed__)) {
        uint32_t offset : 12;
        uint32_t bank : 20;
    } _4k;
    struct __attribute__((__packed__)) {
        uint32_t offset : 13;
        uint32_t bank : 19;
    } _8k;
    struct __attribute__((__packed__)) {
        uint32_t offset : 14;
        uint32_t bank : 18;
    } _16k;
} bank_addr;

typedef struct __attribute__((__packed__)) {
    union __attribute__((__packed__)) {  // Bank select ($8000-$9FFE, even)
        uint8_t u8;
        struct __attribute__((__packed__)) {
            uint8_t target_R : 3;  // Specify which bank reg (R) to update on next write to  ($8001-$9FFF, odd)
            uint8_t : 2;
            uint8_t : 1;                    /// Nothing on the MMC3, see MMC6
            uint8_t prg_rom_bank_mode : 1;  // 0: $8000-$9FFF swappable, $C000-$DFFF fixed to 2nd-last bank. 1: swapped
            uint8_t chr_A12_inversion : 1;
        };
    } ctrl;
    uint8_t R[8];
    struct {
        bool enable;
        uint8_t reload_value;
        uint8_t counter;
        uint8_t edge;
    } irq;
    struct {
        uint8_t vram_mirroring : 1;  // Mirroring ($A000-$BFFE, even) (0: vert; 1: hori). Unused if 4-screen VRAM
        uint8_t : 7;                 // unused
    };
} mapper_004_reg;

/* R6 and R7 will ignore the top two bits, as the MMC3 has only 6 PRG ROM address lines. Some romhacks rely on an 8-bit
extension of R6/7 for oversized PRG-ROM, but this is deliberately not supported by many emulators. See iNES Mapper 004
below.
R0 and R1 ignore the bottom bit, as the value written still counts banks in 1KB units but odd numbered banks can't be
selected. */
static const uint8_t reg_masking[] = {
    0xFE, 0xFE, 0xFF, 0xFF, 0xFF, 0xFF, 0x3F, 0x3F,
};

bool mapper_004_cpu_write(NesCart *const cart, const uint16_t addr, uint8_t val) {
    if (addr < 0x6000) {
        return false;
    }
    if (addr < 0x8000) {
        if (NULL == cart->prg_ram.buf) {
            return false;  // accessing missing prg ram
        }
        cart->prg_ram.buf[addr & (cart->prg_ram.size - 1)] = val;
        return true;
    }
    mapper_004_reg *const reg = (mapper_004_reg *)cart->mapper_data;
    switch (addr & 0xE001) {
        case 0x8000:  // Bank select ($8000-$9FFE, even)
            reg->ctrl.u8 = val;
            break;
        case 0x8001:  // Bank data ($8001-$9FFF, odd)
            reg->R[reg->ctrl.target_R] = val & reg_masking[reg->ctrl.target_R];
            break;
        case 0xA000:  // Mirroring ($A000-$BFFE, even)
            reg->vram_mirroring = val;
            break;
        case 0xA001:
            // prg-ram protect. not implemented
            break;
        case 0xC000:  // IRQ latch ($C000-$DFFE, even)
            reg->irq.reload_value = val;
            break;
        case 0xC001:               // IRQ reload ($C001-$DFFF, odd)
            reg->irq.counter = 0;  // reloads it at the NEXT rising edge of the PPU addr
            break;
        case 0xE000:
            reg->irq.enable = false;
            cart->irq_out = false;
            break;
        case 0xE001:
            reg->irq.enable = true;
            break;
        default:
            assert(0);
    }
    return true;
}

// get target 8k prg-rom bank for given address in 0x8000-0xFFFF https://www.nesdev.org/wiki/MMC3#PRG_Banks
static inline uint8_t get_prg_bank_8k(const NesCart *const cart, const uint16_t addr) {
    const unsigned int n_8k_banks = (bank_addr){.u32 = cart->prg_rom.size}._8k.bank;  // convert from 16k-banks
    const mapper_004_reg *const reg = (mapper_004_reg *)cart->mapper_data;

    switch (({
        const uint16_t base = addr & 0xE000;  // prg_rom_bank_mode - swap 8000 and C000
        (reg->ctrl.prg_rom_bank_mode && ((0x8000 == base) || (0xC000 == base))) ? (base ^ 0x4000) : base;
    })) {
        case 0x8000:  // $8000-$9FFF
            return (n_8k_banks - 1) & reg->R[6];
        case 0xA000:  //$A000-$BFFF
            return (n_8k_banks - 1) & reg->R[7];
        case 0xC000:  // $C000-$DFFF
            return n_8k_banks - 2;
        case 0xE000:  // $E000-$FFFF
            return n_8k_banks - 1;
        default:
            assert(0);
    }
}

// https://www.nesdev.org/wiki/MMC3#CHR_Banks
static inline uint8_t get_chr_bank_1k(const NesCart *const cart, const uint16_t addr) {
    const mapper_004_reg *const reg = (mapper_004_reg *)cart->mapper_data;

    switch ((addr ^ (reg->ctrl.chr_A12_inversion << 12)) & 0x1C00) {
        case 0x0000:  // 0x0000-0x03FF - 2K span
            return reg->R[0];
        case 0x0400:  // 0x0400-0x07FF - 2K span second half
            return reg->R[0] + 1;
        case 0x0800:  // 0x0800-0x0BFF - 2K span
            return reg->R[1];
        case 0x0C00:  // 0x0C00-0x0FFF - 2K span second half
            return reg->R[1] + 1;
        case 0x1000:  // 0x1000-0x13FF
            return reg->R[2];
        case 0x1400:  // 0x1400-0x17FF
            return reg->R[3];
        case 0x1800:  // 0x1800-0x1BFF
            return reg->R[4];
        case 0x1C00:  // 0x1C00-0x1FFF
            return reg->R[5];
        default:
            assert(0);
    }
}

bool mapper_004_cpu_read(NesCart *const cart, const uint16_t addr, uint8_t *const val_out) {
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

    const uint8_t prg_bank_8k = get_prg_bank_8k(cart, addr);
    const bank_addr out_addr = {._8k = {.bank = prg_bank_8k, .offset = addr}};
    assert(out_addr.u32 < cart->prg_rom.size);
    *val_out = cart->prg_rom.buf[out_addr.u32];
    return true;
}

static inline void check_irq(NesCart *const cart, mapper_004_reg *const reg, const uint16_t addr) {
    const mapper_ppu_addr *const ppu_addr = (mapper_ppu_addr *)&addr;

    if ((0 == reg->irq.edge) && ppu_addr->A12) {  // rising
        if (0 == reg->irq.counter) {
            reg->irq.counter = reg->irq.reload_value;
        } else {
            reg->irq.counter--;
            if (0 == reg->irq.counter) {
                if (reg->irq.enable) {
                    cart->irq_out = true;
                }
                // if (reg->irq.enable && cart->irq.callback) {
                //     cart->irq.callback(cart->irq.arg);
                // }
            }
        }
    }
    reg->irq.edge = ppu_addr->A12;
}

bool mapper_004_ppu_write(NesCart *const cart, uint16_t addr, uint8_t val) {
    mapper_004_reg *const reg = (mapper_004_reg *)cart->mapper_data;
    const mapper_ppu_addr *const ppu_addr = (mapper_ppu_addr *)&addr;

    check_irq(cart, reg, addr);

    if (addr >= 0x2000) {
        if (cart->ext_vram) {
            cart->VRAM_CE = 0;
            *(cart->ext_vram)[addr & (sizeof(*cart->ext_vram) - 1)] = val;
            return true;
        } else {
            cart->VRAM_CE = 1;
            cart->VRAM_A10 = (reg->vram_mirroring == 0) ? ppu_addr->A10 : ppu_addr->A11;
            return false;
        }
    }
    cart->VRAM_CE = 0;
    return false;
}

bool mapper_004_ppu_read(NesCart *const cart, uint16_t addr, uint8_t *const val_out) {
    mapper_004_reg *const reg = (mapper_004_reg *)cart->mapper_data;
    const mapper_ppu_addr *const ppu_addr = (mapper_ppu_addr *)&addr;

    check_irq(cart, reg, addr);

    if (addr >= 0x2000) {
        if (cart->ext_vram) {
            cart->VRAM_CE = 0;
            *val_out = *(cart->ext_vram)[addr & (sizeof(*cart->ext_vram) - 1)];
            return true;
        } else {
            cart->VRAM_CE = 1;
            cart->VRAM_A10 = (reg->vram_mirroring == 0) ? ppu_addr->A10 : ppu_addr->A11;
            return false;
        }
    }
    cart->VRAM_CE = 0;

    const uint8_t chr_bank_1k = get_chr_bank_1k(cart, addr);
    *val_out = cart->chr_rom.buf[(bank_addr){._1k = {.bank = chr_bank_1k, .offset = addr}}.u32];
    return true;
}

void mapper_004_init(NesCart *const cart) {
    mapper_004_reg *const reg = malloc(sizeof(*reg));
    memset(reg, 0, sizeof(*reg));
    cart->mapper_data = (uintptr_t)reg;
}

void mapper_004_deinit(NesCart *const cart) {
    free((void *)cart->mapper_data);
}

void mapper_004_cpu_clock(NesCart *const cart) {
    mapper_004_reg *const reg = (mapper_004_reg *)cart->mapper_data;
    check_irq(cart, reg, *cart->addr_in);
}

const struct NesCartMapperInterface mapper_004 = {
    .name = "MMC3",
    .cpu_write = mapper_004_cpu_write,
    .cpu_read = mapper_004_cpu_read,
    .ppu_write = mapper_004_ppu_write,
    .ppu_read = mapper_004_ppu_read,
    .cpu_clock = mapper_004_cpu_clock,
    .init = mapper_004_init,
    .deinit = mapper_004_deinit,
};
