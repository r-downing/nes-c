#pragma once
#include <assert.h>
#include <nes_cart.h>
#include <stddef.h>
#include <stdint.h>

// https://www.nesdev.org/wiki/INES
static const uint8_t header_cookie[4] = {'N', 'E', 'S', 0x1A};

typedef union __attribute__((__packed__)) {
    uint8_t buf[16];
    struct __attribute__((__packed__)) {
        /** Constant $4E $45 $53 $1A (ASCII "NES" followed by MS-DOS end-of-file) */
        uint8_t header_cookie[4];

        /** Size of PRG ROM in 16 KB units */
        uint8_t prg_rom_size_16KB;

        /** Size of CHR ROM in 8 KB units (value 0 means the board uses CHR RAM) */
        uint8_t chr_rom_size_8KB;

        struct __attribute__((__packed__)) {
            /**
             * Mirroring: 0: horizontal (vertical arrangement) (CIRAM A10 = PPU A11)
             *            1: vertical (horizontal arrangement) (CIRAM A10 = PPU A10)
             */
            uint8_t mirroring : 1;

            /** 1: Cartridge contains battery-backed PRG RAM ($6000-7FFF) or other persistent memory */
            uint8_t has_pers_mem : 1;

            /** 1: 512-byte trainer at $7000-$71FF (stored before PRG data) */
            uint8_t has_trainer : 1;

            /** 1: Ignore mirroring control or above mirroring bit; instead provide four-screen VRAM */
            uint8_t four_screen_vram : 1;

            /** Lower nybble of mapper number */
            uint8_t mapper_low : 4;
        } flags6;

        struct __attribute__((__packed__)) {
            uint8_t vs_unisystem : 1;

            /** PlayChoice-10 (8 KB of Hint Screen data stored after CHR data) */
            uint8_t playchoice_10 : 1;

            /** If equal to 2, flags 8-15 are in NES 2.0 format */
            uint8_t nes_2p0 : 2;

            /** Upper nybble of mapper number */
            uint8_t mapper_high : 4;
        } flags7;
    };

    /** Size of PRG RAM in 8 KB units (Value 0 infers 8 KB for compatibility; see PRG RAM circuit)
     * This was a later extension to the iNES format and not widely used. NES 2.0 is recommended for specifying PRG RAM
     * size instead. */
    uint8_t prg_ram_size_8KB;
} RawCartridgeHeader;

static_assert(sizeof(((RawCartridgeHeader *)NULL)->buf) == sizeof(RawCartridgeHeader));

static inline void nes_cart_irq(const NesCart *const cart) {
    if (cart->irq.callback) {
        cart->irq.callback(cart->irq.arg);
    }
}

typedef struct __attribute__((__packed__)) {
    uint16_t : 10;  // 0-9
    uint16_t A10 : 1;
    uint16_t A11 : 1;
    uint16_t A12 : 1;
    uint16_t A13 : 1;
    uint16_t : 2;  // 14-15
} mapper_ppu_addr;
