#include <CppUTest/TestHarness.h>
#include <CppUTestExt/MockSupport.h>

extern "C" {
#include <nes_bus.h>
#include <stdlib.h>
#include <string.h>
}

static bool cpu_write(NesCart *, uint16_t addr, uint8_t val) {
    mock().actualCall(__func__).withParameter("addr", addr).withParameter("val", val);
    return mock().returnBoolValueOrDefault(false);
}

static bool cpu_read(NesCart *, uint16_t addr, uint8_t *val_out) {
    mock().actualCall(__func__).withParameter("addr", addr).withOutputParameter("val_out", val_out);
    return mock().returnBoolValueOrDefault(false);
}

static bool ppu_write(NesCart *, uint16_t addr, uint8_t val) {
    mock().actualCall(__func__).withParameter("addr", addr).withParameter("val", val);
    return mock().returnBoolValueOrDefault(false);
}

static bool ppu_read(NesCart *, uint16_t addr, uint8_t *val_out) {
    mock().actualCall(__func__).withParameter("addr", addr).withOutputParameter("val_out", val_out);
    return mock().returnBoolValueOrDefault(false);
}

static const NesCart::NesCartMapperInterface mapper = {
    .name = "MOCK",
    .cpu_write = cpu_write,
    .cpu_read = cpu_read,
    .ppu_write = ppu_write,
    .ppu_read = ppu_read,
    NULL,
    NULL,
    NULL,
};

namespace mock_nes_cart {
void expect_cpu_write(uint16_t addr, uint8_t val, bool ret = false) {
    mock().expectOneCall("cpu_write").withParameter("addr", addr).withParameter("val", val).andReturnValue(ret);
}

void expect_cpu_read(uint16_t addr, uint8_t val, bool ret = false) {
    mock()
        .expectOneCall("cpu_read")
        .withParameter("addr", addr)
        .withOutputParameterReturning("val_out", &val, sizeof(val))
        .andReturnValue(ret);
}

void expect_ppu_write(uint16_t addr, uint8_t val, bool ret = false) {
    mock().expectOneCall("ppu_write").withParameter("addr", addr).withParameter("val", val).andReturnValue(ret);
}

void expect_ppu_read(uint16_t addr, uint8_t val, bool ret = false) {
    mock()
        .expectOneCall("ppu_read")
        .withParameter("addr", addr)
        .withOutputParameterReturning("val_out", &val, sizeof(val))
        .andReturnValue(ret);
}

}  // namespace mock_nes_cart

TEST_GROUP(NesBusTestGroup) {
    NesBus bus;

    TEST_SETUP() {
        bus.cart.mapper = &mapper;
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

    mock_nes_cart::expect_ppu_read(0x2001, 0, false);
    ret = nes_bus_ppu_read(&bus, 0x2001);
    CHECK_EQUAL(ret, 0xAA);

    mock_nes_cart::expect_ppu_read(0x2401, 0, false);
    ret = nes_bus_ppu_read(&bus, 0x2401);
    CHECK_EQUAL(ret, 0xAA);

    mock_nes_cart::expect_ppu_read(0x2801, 0, false);
    ret = nes_bus_ppu_read(&bus, 0x2801);
    CHECK_EQUAL(ret, 0xBB);

    mock_nes_cart::expect_ppu_read(0x2C01, 0, false);
    ret = nes_bus_ppu_read(&bus, 0x2C01);
    CHECK_EQUAL(ret, 0xBB);

    bus.cart.mirror_type = NES_CART_MIRROR_VERTICAL;

    mock_nes_cart::expect_ppu_read(0x2001, 0, false);
    ret = nes_bus_ppu_read(&bus, 0x2001);
    CHECK_EQUAL(ret, 0xAA);

    mock_nes_cart::expect_ppu_read(0x2401, 0, false);
    ret = nes_bus_ppu_read(&bus, 0x2401);
    CHECK_EQUAL(ret, 0xBB);

    mock_nes_cart::expect_ppu_read(0x2801, 0, false);
    ret = nes_bus_ppu_read(&bus, 0x2801);
    CHECK_EQUAL(ret, 0xAA);

    mock_nes_cart::expect_ppu_read(0x2C01, 0, false);
    ret = nes_bus_ppu_read(&bus, 0x2C01);
    CHECK_EQUAL(ret, 0xBB);

    mock_nes_cart::expect_ppu_read(0x3C01, 0, false);
    ret = nes_bus_ppu_read(&bus, 0x3C01);
    CHECK_EQUAL(ret, 0xBB);

    mock_nes_cart::expect_ppu_write(0x3C01, 0xEE);
    nes_bus_ppu_write(&bus, 0x3C01, 0xEE);
    CHECK_EQUAL(0xEE, bus.vram[1][1]);
}
