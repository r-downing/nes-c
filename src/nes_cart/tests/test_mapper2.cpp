// Todo - mapper2 tests
#include <CppUTest/TestHarness.h>
#include <CppUTestExt/MockSupport.h>

extern "C" {
#include <nes_cart.h>
#include <stdlib.h>
#include <string.h>
}

extern const struct NesCart::NesCartMapperInterface mapper2;

TEST_GROUP(Mapper2TestGroup) {
    NesCart cart;
    TEST_SETUP() {
        cart.mapper = &mapper2;
        cart.prg_rom.size = 0x10000;
        cart.prg_rom.buf = (uint8_t *)malloc(cart.prg_rom.size);
    }

    TEST_TEARDOWN() {
        mock().checkExpectations();
        mock().clear();
        free((void *)cart.prg_rom.buf);
    }
};

TEST(Mapper2TestGroup, test_basic) {
    uint8_t *const wbuf = (uint8_t *)cart.prg_rom.buf;
    wbuf[0] = 0x11;
    wbuf[0x4000] = 0x22;
    wbuf[0x8000] = 0x33;
    wbuf[0xC000] = 0x44;

    CHECK_FALSE(mapper2.cpu_write(&cart, 0x7000, 0));
    uint8_t val = 0;
    CHECK_FALSE(mapper2.cpu_read(&cart, 0x7000, &val));
    val = 0;
    CHECK(mapper2.cpu_write(&cart, 0x8000, 0));
    CHECK(mapper2.cpu_read(&cart, 0x8000, &val));
    CHECK_EQUAL(0x11, val);
    CHECK(mapper2.cpu_read(&cart, 0xC000, &val));
    CHECK_EQUAL(0x44, val);

    CHECK(mapper2.cpu_write(&cart, 0x8000, 1));
    CHECK(mapper2.cpu_read(&cart, 0x8000, &val));
    CHECK_EQUAL(0x22, val);
    CHECK(mapper2.cpu_read(&cart, 0xC000, &val));
    CHECK_EQUAL(0x44, val);

    CHECK(mapper2.cpu_write(&cart, 0x8000, 2));
    CHECK(mapper2.cpu_read(&cart, 0x8000, &val));
    CHECK_EQUAL(0x33, val);
    CHECK(mapper2.cpu_read(&cart, 0xC000, &val));
    CHECK_EQUAL(0x44, val);

    CHECK(mapper2.cpu_write(&cart, 0x8000, 3));
    CHECK(mapper2.cpu_read(&cart, 0x8000, &val));
    CHECK_EQUAL(0x44, val);
    CHECK(mapper2.cpu_read(&cart, 0xC000, &val));
    CHECK_EQUAL(0x44, val);
}