#include <CppUTest/TestHarness.h>
#include <CppUTestExt/MockSupport.h>

extern "C" {
#include <nes_cart.h>
#include <stdlib.h>
#include <string.h>
}

TEST_GROUP(NesCartTestGroup) {
    NesCart cart;
    TEST_SETUP() {}

    TEST_TEARDOWN() {
        mock().checkExpectations();
        mock().clear();
    }
};

TEST(NesCartTestGroup, test_init_from_data) {
    uint8_t buf[0x7011] = {0};

    buf[0] = 'N';
    buf[1] = 'E';
    buf[2] = 'S';
    buf[3] = 0x1A;
    buf[4] = 1;
    buf[5] = 2;
    buf[6] = 0b00000011;
    buf[7] = 0;
    buf[0x10] = 0x10;
    buf[0x400F] = 111;
    buf[0x4010] = 123;
    buf[0x7010] = 45;

    nes_cart_init_from_data(&cart, buf, sizeof(buf));
    CHECK_EQUAL(cart.mirror_type, NES_CART_MIRROR_VERTICAL);

    CHECK_EQUAL(cart.prg_rom.size, 0x4000);
    CHECK_EQUAL(cart.chr_rom.size, 0x4000);
    CHECK_EQUAL(cart.prg_rom.buf[0], 0x10);
    CHECK_EQUAL(cart.prg_rom.buf[0x3FFF], 111);
    CHECK_EQUAL(cart.chr_rom.buf[0], 123);
    CHECK_EQUAL(cart.chr_rom.buf[0x3000], 45);
}

extern const struct NesCart::NesCartMapperInterface mapper_002;

TEST(NesCartTestGroup, test_init_from_data_mapper_002) {
    uint8_t buf[0x7011] = {0};

    buf[0] = 'N';
    buf[1] = 'E';
    buf[2] = 'S';
    buf[3] = 0x1A;
    buf[4] = 1;
    buf[5] = 2;
    buf[6] = 0b00100010;
    buf[7] = 0;
    buf[0x10] = 0x10;
    buf[0x400F] = 111;
    buf[0x4010] = 123;
    buf[0x7010] = 45;

    nes_cart_init_from_data(&cart, buf, sizeof(buf));
    CHECK_EQUAL(cart.mirror_type, NES_CART_MIRROR_HORIZONTAL);

    CHECK_EQUAL(cart.prg_rom.size, 0x4000);
    CHECK_EQUAL(cart.chr_rom.size, 0x4000);
    CHECK_EQUAL(cart.prg_rom.buf[0], 0x10);
    CHECK_EQUAL(cart.prg_rom.buf[0x3FFF], 111);
    CHECK_EQUAL(cart.chr_rom.buf[0], 123);
    CHECK_EQUAL(cart.chr_rom.buf[0x3000], 45);

    CHECK_EQUAL(cart.mapper, &mapper_002);
}
