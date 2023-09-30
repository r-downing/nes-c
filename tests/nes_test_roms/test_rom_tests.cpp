#include <CppUTest/TestHarness.h>
#include <CppUTestExt/MockSupport.h>

extern "C" {
#include <nes_bus.h>
#include <stdlib.h>
#include <string.h>
}

static const int ppu_cycles_per_sec = (21477272 / 4);

TEST_GROUP(TestRomTests) {
    NesBus bus;
    TEST_SETUP() {
        memset(bus.ram, 0xA3, sizeof(bus.ram));
    }

    TEST_TEARDOWN() {
        mock().checkExpectations();
        mock().clear();
    }

    void test_vbl_nmi_timing(const char *const rom_file, float seconds) {
        nes_cart_init(&bus.cart, rom_file);
        nes_bus_init(&bus);
        for (int i = 0; i < ppu_cycles_per_sec * seconds; i++) {
            nes_bus_cycle(&bus);
        }

        CHECK_EQUAL(bus.ram[0xF8], 1);
    }
};

TEST(TestRomTests, test_vbl_nmi_timing_1_frame_basics) {
    test_vbl_nmi_timing(ROMS_FOLDER "/nes-test-roms/vbl_nmi_timing/1.frame_basics.nes", 3);
}

TEST(TestRomTests, test_vbl_nmi_timing_2_vbl_timing) {
    test_vbl_nmi_timing(ROMS_FOLDER "/nes-test-roms/vbl_nmi_timing/2.vbl_timing.nes", 3);
}

TEST(TestRomTests, test_vbl_nmi_timing_3_even_odd_frames) {
    test_vbl_nmi_timing(ROMS_FOLDER "/nes-test-roms/vbl_nmi_timing/3.even_odd_frames.nes", 3);
}

TEST(TestRomTests, test_vbl_nmi_timing_4_vbl_clear_timing) {
    test_vbl_nmi_timing(ROMS_FOLDER "/nes-test-roms/vbl_nmi_timing/4.vbl_clear_timing.nes", 3);
}

TEST(TestRomTests, test_vbl_nmi_timing_5_nmi_suppression) {
    test_vbl_nmi_timing(ROMS_FOLDER "/nes-test-roms/vbl_nmi_timing/5.nmi_suppression.nes", 3);
}

TEST(TestRomTests, test_vbl_nmi_timing_6_nmi_disable) {
    test_vbl_nmi_timing(ROMS_FOLDER "/nes-test-roms/vbl_nmi_timing/6.nmi_disable.nes", 3);
}

TEST(TestRomTests, test_vbl_nmi_timing_7_nmi_timing) {
    test_vbl_nmi_timing(ROMS_FOLDER "/nes-test-roms/vbl_nmi_timing/7.nmi_timing.nes", 3);
}
