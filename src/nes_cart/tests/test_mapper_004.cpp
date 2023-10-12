// Todo - mapper_002 tests
#include <CppUTest/TestHarness.h>
#include <CppUTestExt/MockSupport.h>

extern "C" {
#include <nes_cart.h>
#include <stdlib.h>
#include <string.h>
}

extern const struct NesCart::NesCartMapperInterface mapper_004;

TEST_GROUP(mapper_004TestGroup) {
    NesCart cart;
    TEST_SETUP() {
        cart.mapper = &mapper_004;
        cart.prg_rom.banks = 16;
        cart.prg_rom.buf = (uint8_t *)malloc(cart.prg_rom.size);
        memset((void *)cart.prg_rom.buf, 0, cart.prg_rom.size);

        cart.chr_rom.banks = 8;
        cart.chr_rom.buf = (uint8_t *)malloc(cart.chr_rom.size);
        memset(cart.chr_rom.buf, 0, cart.chr_rom.size);

        mapper_004.init(&cart);
    }

    TEST_TEARDOWN() {
        mock().checkExpectations();
        mock().clear();
        free((void *)cart.prg_rom.buf);
        free((void *)cart.chr_rom.buf);
    }
};

TEST(mapper_004TestGroup, test_prg_mapping) {
    CHECK_EQUAL(32 * 0x2000, cart.prg_rom.size);
    uint8_t *const rom = (uint8_t *)cart.prg_rom.buf;

    // set R6 to bank 15
    CHECK(mapper_004.cpu_write(&cart, 0x8000, 0x06));  // Bank select ($8000-$9FFE, even)
    CHECK(mapper_004.cpu_write(&cart, 0x8001, 15));    // Bank data ($8001-$9FFF, odd)

    // set R7 to bank 21
    CHECK(mapper_004.cpu_write(&cart, 0x9FFE, 0x07));  // Bank select ($8000-$9FFE, even)
    CHECK(mapper_004.cpu_write(&cart, 0x9FFF, 21));    // Bank data ($8001-$9FFF, odd)

    rom[(15 * 0x2000) + 0] = 80;       // $8000-$9FFF R6
    rom[(15 * 0x2000) + 0x1FFF] = 81;  // $8000-$9FFF R6

    rom[(21 * 0x2000) + 2] = 70;       // $A000-$BFFF R7
    rom[(21 * 0x2000) + 0x1FFF] = 71;  // $A000-$BFFF R7

    rom[(30 * 0x2000) + 3] = 60;       // $C000-$DFFF (-2)
    rom[(30 * 0x2000) + 0x1FFF] = 61;  // $C000-$DFFF (-2)
    rom[(31 * 0x2000) + 1] = 50;       // $E000-$FFFF (-1)
    rom[(31 * 0x2000) + 0x1FFF] = 51;  // $E000-$FFFF (-1)

    uint8_t val = 0;

    CHECK(mapper_004.cpu_read(&cart, 0x8000, &val));
    CHECK_EQUAL(80, val);

    CHECK(mapper_004.cpu_read(&cart, 0x9FFF, &val));
    CHECK_EQUAL(81, val);

    CHECK(mapper_004.cpu_read(&cart, 0xA002, &val));
    CHECK_EQUAL(70, val);

    CHECK(mapper_004.cpu_read(&cart, 0xBFFF, &val));
    CHECK_EQUAL(71, val);

    CHECK(mapper_004.cpu_read(&cart, 0xC003, &val));
    CHECK_EQUAL(60, val);

    CHECK(mapper_004.cpu_read(&cart, 0xDFFF, &val));
    CHECK_EQUAL(61, val);

    CHECK(mapper_004.cpu_read(&cart, 0xE001, &val));
    CHECK_EQUAL(50, val);

    CHECK(mapper_004.cpu_read(&cart, 0xFFFF, &val));
    CHECK_EQUAL(51, val);

    // swap ($8000-$9FFF) and ($C000-$DFFF)
    CHECK(mapper_004.cpu_write(&cart, 0x9000, 0b01000000));  // Bank select ($8000-$9FFE, even)

    CHECK(mapper_004.cpu_read(&cart, 0xC000, &val));
    CHECK_EQUAL(80, val);

    CHECK(mapper_004.cpu_read(&cart, 0xDFFF, &val));
    CHECK_EQUAL(81, val);

    CHECK(mapper_004.cpu_read(&cart, 0xA002, &val));
    CHECK_EQUAL(70, val);

    CHECK(mapper_004.cpu_read(&cart, 0xBFFF, &val));
    CHECK_EQUAL(71, val);

    CHECK(mapper_004.cpu_read(&cart, 0x8003, &val));
    CHECK_EQUAL(60, val);

    CHECK(mapper_004.cpu_read(&cart, 0x9FFF, &val));
    CHECK_EQUAL(61, val);

    CHECK(mapper_004.cpu_read(&cart, 0xE001, &val));
    CHECK_EQUAL(50, val);

    CHECK(mapper_004.cpu_read(&cart, 0xFFFF, &val));
    CHECK_EQUAL(51, val);
}

TEST(mapper_004TestGroup, test_chr_mapping) {
    CHECK_EQUAL(8 * 0x2000, cart.chr_rom.size);
    uint8_t *const rom = (uint8_t *)cart.chr_rom.buf;

    CHECK(mapper_004.cpu_write(&cart, 0x8000, 0));
    CHECK(mapper_004.cpu_write(&cart, 0x8001, 10));

    CHECK(mapper_004.cpu_write(&cart, 0x8000, 1));
    CHECK(mapper_004.cpu_write(&cart, 0x8001, 15));  // 14 - low bit ignored for 2k banks

    CHECK(mapper_004.cpu_write(&cart, 0x8000, 2));
    CHECK(mapper_004.cpu_write(&cart, 0x8001, 20));

    CHECK(mapper_004.cpu_write(&cart, 0x8000, 3));
    CHECK(mapper_004.cpu_write(&cart, 0x8001, 25));

    CHECK(mapper_004.cpu_write(&cart, 0x8000, 4));
    CHECK(mapper_004.cpu_write(&cart, 0x8001, 30));

    CHECK(mapper_004.cpu_write(&cart, 0x8000, 5));
    CHECK(mapper_004.cpu_write(&cart, 0x8001, 47));

    rom[(10 * 0x400) + 7] = 12;
    rom[(11 * 0x400) + 3] = 13;

    rom[(14 * 0x400) + 2] = 23;
    rom[(15 * 0x400) + 7] = 34;

    rom[(20 * 0x400) + 7] = 45;
    rom[(25 * 0x400) + 6] = 56;
    rom[(30 * 0x400) + 5] = 67;
    rom[(47 * 0x400) + 4] = 78;

    uint8_t val = 0;

    CHECK(mapper_004.ppu_read(&cart, 0x0007, &val));
    CHECK_EQUAL(12, val);

    CHECK(mapper_004.ppu_read(&cart, 0x0403, &val));
    CHECK_EQUAL(13, val);

    CHECK(mapper_004.ppu_read(&cart, 0x0802, &val));
    CHECK_EQUAL(23, val);

    CHECK(mapper_004.ppu_read(&cart, 0x0C07, &val));
    CHECK_EQUAL(34, val);

    CHECK(mapper_004.ppu_read(&cart, 0x1007, &val));
    CHECK_EQUAL(45, val);

    CHECK(mapper_004.ppu_read(&cart, 0x1406, &val));
    CHECK_EQUAL(56, val);

    CHECK(mapper_004.ppu_read(&cart, 0x1805, &val));
    CHECK_EQUAL(67, val);

    CHECK(mapper_004.ppu_read(&cart, 0x1C04, &val));
    CHECK_EQUAL(78, val);

    // A12 inversion
    CHECK(mapper_004.cpu_write(&cart, 0x8000, 0b10000000));

    CHECK(mapper_004.ppu_read(&cart, 0x1007, &val));
    CHECK_EQUAL(12, val);

    CHECK(mapper_004.ppu_read(&cart, 0x1403, &val));
    CHECK_EQUAL(13, val);

    CHECK(mapper_004.ppu_read(&cart, 0x1802, &val));
    CHECK_EQUAL(23, val);

    CHECK(mapper_004.ppu_read(&cart, 0x1C07, &val));
    CHECK_EQUAL(34, val);
    //
    CHECK(mapper_004.ppu_read(&cart, 0x0007, &val));
    CHECK_EQUAL(45, val);

    CHECK(mapper_004.ppu_read(&cart, 0x0406, &val));
    CHECK_EQUAL(56, val);

    CHECK(mapper_004.ppu_read(&cart, 0x0805, &val));
    CHECK_EQUAL(67, val);

    CHECK(mapper_004.ppu_read(&cart, 0x0C04, &val));
    CHECK_EQUAL(78, val);
}

TEST(mapper_004TestGroup, test_vram_mirroring) {}  // Todo
