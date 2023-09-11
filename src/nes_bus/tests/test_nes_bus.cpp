#include <CppUTest/TestHarness.h>
#include <CppUTestExt/MockSupport.h>

extern "C" {
#include <nes_bus.h>
#include <stdlib.h>
#include <string.h>
}

bool nes_cart_prg_write(NesCart *, uint16_t addr, uint8_t val) {
    mock().actualCall(__func__).withParameter("addr", addr).withParameter("val", val);
    return mock().returnBoolValueOrDefault(false);
}

bool nes_cart_prg_read(NesCart *, uint16_t addr, uint8_t *val_out) {
    mock().actualCall(__func__).withParameter("addr", addr).withOutputParameter("val_out", val_out);
    return mock().returnBoolValueOrDefault(false);
}

bool nes_cart_ppu_write(NesCart *, uint16_t addr, uint8_t val) {
    mock().actualCall(__func__).withParameter("addr", addr).withParameter("val", val);
    return mock().returnBoolValueOrDefault(false);
}

bool nes_cart_ppu_read(NesCart *, uint16_t addr, uint8_t *val_out) {
    mock().actualCall(__func__).withParameter("addr", addr).withOutputParameter("val_out", val_out);
    return mock().returnBoolValueOrDefault(false);
}

namespace mock_nes_cart {
void expect_nes_cart_prg_write(uint16_t addr, uint8_t val, bool ret = false) {
    mock()
        .expectOneCall("nes_cart_prg_write")
        .withParameter("addr", addr)
        .withParameter("val", val)
        .andReturnValue(ret);
}

void expect_nes_cart_prg_read(uint16_t addr, uint8_t val, bool ret = false) {
    mock()
        .expectOneCall("nes_cart_prg_read")
        .withParameter("addr", addr)
        .withOutputParameterReturning("val_out", &val, sizeof(val))
        .andReturnValue(ret);
}

void expect_nes_cart_ppu_write(uint16_t addr, uint8_t val, bool ret = false) {
    mock()
        .expectOneCall("nes_cart_prg_write")
        .withParameter("addr", addr)
        .withParameter("val", val)
        .andReturnValue(ret);
}

void expect_nes_cart_ppu_read(uint16_t addr, uint8_t val, bool ret = false) {
    mock()
        .expectOneCall("nes_cart_ppu_read")
        .withParameter("addr", addr)
        .withOutputParameterReturning("val_out", &val, sizeof(val))
        .andReturnValue(ret);
}

}  // namespace mock_nes_cart

TEST_GROUP(NesBusTestGroup) {
    NesBus bus;

    TEST_SETUP() {}

    TEST_TEARDOWN() {
        mock().checkExpectations();
        mock().clear();
    }
};

// TEST(NesBusTestGroup, test_todo) {
//     mock_nes_cart::expect_nes_cart_prg_write(123, 45);
//     nes_bus_cpu_write(&bus, 123, 45);

//     // Todo
// }

TEST(NesBusTestGroup, test_ppu_cart_mirroring) {
    // https://www.nesdev.org/wiki/Mirroring#Nametable_Mirroring
    bus.vram[0][1] = 0xAA;
    bus.vram[1][1] = 0xBB;

    bus.cart.mirror_type = NES_CART_MIRROR_HORIZONTAL;

    uint8_t ret;

    mock_nes_cart::expect_nes_cart_ppu_read(0x2001, 0, false);
    ret = nes_bus_ppu_read(&bus, 0x2001);
    CHECK_EQUAL(ret, 0xAA);

    mock_nes_cart::expect_nes_cart_ppu_read(0x2401, 0, false);
    ret = nes_bus_ppu_read(&bus, 0x2401);
    CHECK_EQUAL(ret, 0xAA);

    mock_nes_cart::expect_nes_cart_ppu_read(0x2801, 0, false);
    ret = nes_bus_ppu_read(&bus, 0x2801);
    CHECK_EQUAL(ret, 0xBB);

    mock_nes_cart::expect_nes_cart_ppu_read(0x2C01, 0, false);
    ret = nes_bus_ppu_read(&bus, 0x2C01);
    CHECK_EQUAL(ret, 0xBB);

    bus.cart.mirror_type = NES_CART_MIRROR_VERTICAL;

    mock_nes_cart::expect_nes_cart_ppu_read(0x2001, 0, false);
    ret = nes_bus_ppu_read(&bus, 0x2001);
    CHECK_EQUAL(ret, 0xAA);

    mock_nes_cart::expect_nes_cart_ppu_read(0x2401, 0, false);
    ret = nes_bus_ppu_read(&bus, 0x2401);
    CHECK_EQUAL(ret, 0xBB);

    mock_nes_cart::expect_nes_cart_ppu_read(0x2801, 0, false);
    ret = nes_bus_ppu_read(&bus, 0x2801);
    CHECK_EQUAL(ret, 0xAA);

    mock_nes_cart::expect_nes_cart_ppu_read(0x2C01, 0, false);
    ret = nes_bus_ppu_read(&bus, 0x2C01);
    CHECK_EQUAL(ret, 0xBB);
}