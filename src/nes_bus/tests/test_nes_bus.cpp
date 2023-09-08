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

bool nes_cart_prg_read(NesCart *, uint16_t, uint8_t *) {
    return false;
}

bool nes_cart_ppu_write(NesCart *, uint16_t, uint8_t) {
    return false;
}
bool nes_cart_ppu_read(NesCart *, uint16_t, uint8_t *) {
    return false;
}

namespace mock_nes_cart {
void expect_nes_cart_prg_write(uint16_t addr, uint8_t val, bool ret = false) {
    mock()
        .expectOneCall("nes_cart_prg_write")
        .withParameter("addr", addr)
        .withParameter("val", val)
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

TEST(NesBusTestGroup, test_todo) {
    // mock().expectOneCall("nes_cart_prg_write").withParameter("addr", 0);
    mock_nes_cart::expect_nes_cart_prg_write(123, 45);
    nes_bus_cpu_write(&bus, 123, 45);

    // Todo
}
