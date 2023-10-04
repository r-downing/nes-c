#include <CppUTest/TestHarness.h>
#include <CppUTestExt/MockSupport.h>

extern "C" {
#include <nes_bus.h>
#include <stdlib.h>
#include <string.h>
}

extern const struct NesCart::NesCartMapperInterface mapper_000;

TEST_GROUP(NesBusTestGroup) {
    NesBus bus;

    TEST_SETUP() {
        bus.cart.mapper = &mapper_000;
    }

    TEST_TEARDOWN() {
        mock().checkExpectations();
        mock().clear();
    }
};

TEST(NesBusTestGroup, test_ppu_cart_mirroring) {
    // https://www.nesdev.org/wiki/Mirroring#Nametable_Mirroring
    bus.vram[0][1] = 0xAA;
    bus.vram[1][1] = 0xBB;

    bus.cart.mirror_type = NES_CART_MIRROR_HORIZONTAL;

    uint8_t ret;

    ret = nes_bus_ppu_read(&bus, 0x2001);
    CHECK_EQUAL(ret, 0xAA);

    ret = nes_bus_ppu_read(&bus, 0x2401);
    CHECK_EQUAL(ret, 0xAA);

    ret = nes_bus_ppu_read(&bus, 0x2801);
    CHECK_EQUAL(ret, 0xBB);

    ret = nes_bus_ppu_read(&bus, 0x2C01);
    CHECK_EQUAL(ret, 0xBB);

    bus.cart.mirror_type = NES_CART_MIRROR_VERTICAL;

    ret = nes_bus_ppu_read(&bus, 0x2001);
    CHECK_EQUAL(ret, 0xAA);

    ret = nes_bus_ppu_read(&bus, 0x2401);
    CHECK_EQUAL(ret, 0xBB);

    ret = nes_bus_ppu_read(&bus, 0x2801);
    CHECK_EQUAL(ret, 0xAA);

    ret = nes_bus_ppu_read(&bus, 0x2C01);
    CHECK_EQUAL(ret, 0xBB);

    ret = nes_bus_ppu_read(&bus, 0x3C01);
    CHECK_EQUAL(ret, 0xBB);

    nes_bus_ppu_write(&bus, 0x3C01, 0xEE);
    CHECK_EQUAL(0xEE, bus.vram[1][1]);
}
