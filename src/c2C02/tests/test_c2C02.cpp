#include <CppUTest/TestHarness.h>
#include <CppUTestExt/MockSupport.h>

extern "C" {
#include <c2C02.h>
#include <stdlib.h>
#include <string.h>
}

static uint8_t mock_bus_read(void *, uint16_t addr) {
    return mock().actualCall(__func__).withParameter("addr", addr).returnUnsignedIntValueOrDefault(0);
}

static bool mock_bus_write(void *, uint16_t addr, uint8_t val) {
    return mock()
        .actualCall(__func__)
        .withParameter("addr", addr)
        .withParameter("val", val)
        .returnBoolValueOrDefault(false);
}

static C2C02BusInterface bus = {
    .read = mock_bus_read,
    .write = mock_bus_write,
};

namespace mock_bus {
void expect_read(uint16_t addr, uint8_t ret) {
    mock().expectOneCall("mock_bus_read").withParameter("addr", addr).andReturnValue(ret);
}
void expect_write(uint16_t addr, uint8_t val, bool ret) {
    mock().expectOneCall("mock_bus_write").withParameter("addr", addr).withParameter("val", val).andReturnValue(ret);
}
}  // namespace mock_bus

TEST_GROUP(C2C02TestGroup) {
    C2C02 c;

    TEST_SETUP() {
        c.bus = &bus;
    }

    TEST_TEARDOWN() {
        mock().checkExpectations();
        mock().clear();
    }
};

TEST(C2C02TestGroup, test_write_ppu_addr_reg) {
    c2C02_write_reg(&c, 0x06, 0x01);
    CHECK_EQUAL(c.vram_address.addr, 0);
    CHECK(c.address_latch);
    c2C02_write_reg(&c, 0x06, 0x02);
    CHECK_FALSE(c.address_latch);
    CHECK_EQUAL(c.vram_address.addr, 0x0102);
}

TEST(C2C02TestGroup, test_write_ppu_data) {
    c.vram_address.addr = 0x102;
    mock_bus::expect_write(0x102, 0x34, true);
    c2C02_write_reg(&c, 0x07, 0x34);
    CHECK_EQUAL(c.vram_address.addr, 0x103);
    c.ctrl.vram_inc = 1;
    mock_bus::expect_write(0x103, 0x35, true);
    c2C02_write_reg(&c, 0x07, 0x35);
    CHECK_EQUAL(c.vram_address.addr, 0x123);
}

TEST(C2C02TestGroup, test_read_ppu_data) {
    c.data_read_buffer = 0x55;
    c.vram_address.addr = 0x2021;

    uint8_t ret;

    mock_bus::expect_read(0x2021, 0x66);
    ret = c2C02_read_reg(&c, 0x7);
    CHECK_EQUAL(ret, 0x55);

    c.ctrl.vram_inc = 1;
    mock_bus::expect_read(0x2022, 0x77);
    ret = c2C02_read_reg(&c, 0x7);
    CHECK_EQUAL(ret, 0x66);
    CHECK_EQUAL(c.data_read_buffer, 0x77);

    mock_bus::expect_read(0x2042, 0x88);
    ret = c2C02_read_reg(&c, 0x7);
}

TEST(C2C02TestGroup, test_write_ppu_data_to_palette) {
    c.vram_address.addr = 0x3F01;
    c2C02_write_reg(&c, 0x07, 0x12);
    CHECK_EQUAL(c.palette_ram[1], 0x12);
    // Todo - check mirroring
}

TEST(C2C02TestGroup, test_read_ppu_data_palette) {
    for (uint8_t i = 0; i < sizeof(c.palette_ram); i++) {
        c.palette_ram[i] = i;
    }

    c.vram_address.addr = 0x3F00;
    CHECK_EQUAL(c2C02_read_reg(&c, 0x07), 0);
    c.vram_address.addr = 0x3F01;
    CHECK_EQUAL(c2C02_read_reg(&c, 0x07), 1);
    c.vram_address.addr = 0x3F02;
    CHECK_EQUAL(c2C02_read_reg(&c, 0x07), 2);
    c.vram_address.addr = 0x3F03;
    CHECK_EQUAL(c2C02_read_reg(&c, 0x07), 3);
    c.vram_address.addr = 0x3F04;
    CHECK_EQUAL(c2C02_read_reg(&c, 0x07), 4);

    c.vram_address.addr = 0x3F10;
    CHECK_EQUAL(c2C02_read_reg(&c, 0x07), 0);
    c.vram_address.addr = 0x3F11;
    CHECK_EQUAL(c2C02_read_reg(&c, 0x07), 0x11);
    c.vram_address.addr = 0x3F12;
    CHECK_EQUAL(c2C02_read_reg(&c, 0x07), 0x12);
    c.vram_address.addr = 0x3F13;
    CHECK_EQUAL(c2C02_read_reg(&c, 0x07), 0x13);
    c.vram_address.addr = 0x3F14;
    CHECK_EQUAL(c2C02_read_reg(&c, 0x07), 4);
    c.vram_address.addr = 0x3F15;
    CHECK_EQUAL(c2C02_read_reg(&c, 0x07), 0x15);
    c.vram_address.addr = 0x3F16;
    CHECK_EQUAL(c2C02_read_reg(&c, 0x07), 0x16);
    c.vram_address.addr = 0x3F17;
    CHECK_EQUAL(c2C02_read_reg(&c, 0x07), 0x17);
    c.vram_address.addr = 0x3F18;
    CHECK_EQUAL(c2C02_read_reg(&c, 0x07), 8);

    // c.vram_address.addr = 0x3F0;
    // CHECK_EQUAL(c2C02_read_reg(&c, 0x07), 0);
}
